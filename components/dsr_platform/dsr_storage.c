#include "dsr_storage.h"
#include "dsr_storage_progress.h"
#include "esp_spiffs.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "dsr_config_health.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#if configUSE_PREEMPTION != 1 || configUSE_TIME_SLICING != 1
#error "Blank SPIFFS provisioning requires preemptive idle-priority time slicing"
#endif

/* The interrupted-format classifier encodes the exact on-flash layout that
 * IDF 5.5.4 SPIFFS_format() -> spiffs_erase_block() writes. For each 4 KiB
 * block, strictly in ascending order: sector erase; 16-bit erase count (always
 * 0 during format) in the last two bytes of the single 256-byte object-lookup
 * page; then the 16-bit length magic just before it. Another geometry or
 * option must fail the build rather than silently mis-classify user data. */
#if defined(CONFIG_SPIFFS_PAGE_SIZE) && CONFIG_SPIFFS_PAGE_SIZE != 256
#error "dsr_storage remnant classifier assumes 256-byte SPIFFS pages"
#endif
#if defined(ESP_PLATFORM) && (!defined(CONFIG_SPIFFS_USE_MAGIC) || !defined(CONFIG_SPIFFS_USE_MAGIC_LENGTH))
#error "dsr_storage remnant classifier requires SPIFFS length magic"
#endif
#define BLOCK_BYTES 4096u
#define PAGE_BYTES 256u
#define HEADER_OFFSET (PAGE_BYTES-4u) /* magic lo, magic hi, erase lo, erase hi */

typedef enum {SCAN_BLANK,SCAN_REMNANT,SCAN_NONEMPTY,SCAN_READ_ERROR} scan_result_t;

static uint8_t header_expected(uint32_t block_count,uint32_t block,size_t index)
{
    const uint16_t magic=(uint16_t)(0x20140529u^PAGE_BYTES^(block_count-block));
    return index==0?(uint8_t)magic:index==1?(uint8_t)(magic>>8):0u;
}

static const dsr_storage_observer_t *observer;
static _Atomic(const esp_partition_t *) progress_partition;
void dsr_storage_set_observer(const dsr_storage_observer_t *value){observer=value;}

static bool header_exact_at(const esp_partition_t *partition,uint32_t block_count,uint32_t block)
{
    uint8_t bytes[4];
    if(esp_partition_read(partition,(size_t)block*BLOCK_BYTES+HEADER_OFFSET,bytes,sizeof(bytes))!=ESP_OK)return false;
    for(size_t i=0;i<sizeof(bytes);i++)if(bytes[i]!=header_expected(block_count,block,i))return false;
    return true;
}
/* The formatter writes headers in strictly ascending block order, so the
 * complete prefix is found by binary search (ceil(log2(993)) = 10 reads).
 * A block being erased reads as 0xFF and can only LOWER the answer. */
uint32_t dsr_storage_format_progress(void)
{
    const esp_partition_t *partition=atomic_load(&progress_partition);
    if(!partition)return 0;
    const uint32_t block_count=(uint32_t)(partition->size/BLOCK_BYTES);
    uint32_t low=0,high=block_count;
    while(low<high){
        const uint32_t middle=low+(high-low+1u)/2u;
        if(header_exact_at(partition,block_count,middle-1u))low=middle;else high=middle-1u;
    }
    return low;
}

/* Streams the whole partition in <=1 KiB sequential reads (bounded stack).
 * BLANK: every byte 0xFF.
 * REMNANT: exactly what an interrupted SPIFFS_format() of THIS geometry
 * leaves on a previously blank partition: blocks 0..N-1 (N>=0) carry the
 * exact format header, block N may carry a NOR-partial program of its header
 * (bits cleared only where the target has zeros), every later block and
 * every other byte of every block is 0xFF. No object-lookup entry, object
 * page, name or user byte can exist in a REMNANT.
 * Anything else, including a header for another geometry: NONEMPTY. */
static scan_result_t scan_partition(const esp_partition_t *partition,esp_err_t *read_error,uint32_t *complete_out)
{
    const bool block_layout=partition->size%BLOCK_BYTES==0&&partition->size>=BLOCK_BYTES;
    const uint32_t block_count=(uint32_t)(partition->size/BLOCK_BYTES);
    uint32_t complete=0;bool tail=false,any_nonblank=false,foreign=false;
    bool header_exact=true,header_blank=true;
    uint8_t bytes[1024];
    for (size_t offset = 0; offset < partition->size; offset += sizeof(bytes)) {
        size_t count = partition->size - offset;
        if (count > sizeof(bytes)) count = sizeof(bytes);
        esp_err_t error = esp_partition_read(partition, offset, bytes, count);
        if (error != ESP_OK) {*read_error=error;return SCAN_READ_ERROR;}
        for (size_t i = 0; i < count && !foreign; i++) {
            const size_t at=offset+i,in_block=at%BLOCK_BYTES;
            const uint8_t got=bytes[i];
            if(!block_layout){if(got!=0xff)foreign=true;continue;}
            if(in_block==0){header_exact=true;header_blank=true;}
            if(in_block>=HEADER_OFFSET&&in_block<PAGE_BYTES){
                const uint8_t want=header_expected(block_count,(uint32_t)(at/BLOCK_BYTES),in_block-HEADER_OFFSET);
                if((got&want)!=want){foreign=true;break;} /* a bit the formatter never clears */
                if(got!=want)header_exact=false;
                if(got!=0xff){header_blank=false;any_nonblank=true;}
            } else if(got!=0xff){foreign=true;break;}
            if(in_block==BLOCK_BYTES-1u){
                /* Formatter order is strictly ascending: complete* partial? blank* */
                if(!tail){
                    if(header_exact)complete++;
                    else tail=true;
                } else if(!header_blank)foreign=true;
            }
        }
        if (foreign) {
            ESP_LOGW("dsr_storage","foreign_byte block=%u",(unsigned)(offset/BLOCK_BYTES));
            return SCAN_NONEMPTY;
        }
        if ((offset+count) % (256*1024) == 0 || offset+count == partition->size) {
            ESP_LOGI("dsr_storage","blank_check checked=%u total=%u",(unsigned)(offset+count),(unsigned)partition->size);
            vTaskDelay(1); /* Give idle/radio work CPU even during the read scan. */
        }
    }
    if(!any_nonblank)return SCAN_BLANK;
    /* A complete header on EVERY block plus a mount failure is not an
     * interrupted format; keep the conservative pre-existing refusal. */
    if(complete>=block_count)return SCAN_NONEMPTY;
    *complete_out=complete;
    return SCAN_REMNANT;
}

esp_err_t dsr_storage_mount(void)
{
#ifdef ESP_PLATFORM
    /* Committed boot recovery may already have mounted this exact partition;
     * cache/GC are still uninitialized and start only after it returns OK. */
    if(esp_spiffs_mounted("dsrdata")){dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);return ESP_OK;}
#endif
    dsr_config_health_storage(DSR_STORAGE_MOUNTING,ESP_OK);
    esp_vfs_spiffs_conf_t conf = {.base_path = "/dsr",
        .partition_label = "dsrdata", .max_files = 8,
        .format_if_mount_failed = false};
    esp_err_t error = esp_vfs_spiffs_register(&conf);
    if (error == ESP_OK) {dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);ESP_LOGI("dsr_storage","mount_ready existing=1");return ESP_OK;}
    dsr_config_health_storage(DSR_STORAGE_MOUNT_ERROR,error);
    ESP_LOGW("dsr_storage", "mount failed: %s; automatic erase disabled", esp_err_to_name(error));
    /* ESP_FAIL means filesystem mount failure. NO_MEM/INVALID_STATE/etc.
     * are not evidence of an unformatted filesystem, even on a blank chip. */
    if (error != ESP_FAIL) return error;
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "dsrdata");
    if (!partition) return error;
    esp_err_t read_error=ESP_OK;uint32_t remnant_blocks=0;
    const scan_result_t scan=scan_partition(partition,&read_error,&remnant_blocks);
    if (scan == SCAN_READ_ERROR) {
        ESP_LOGE("dsr_storage", "blank check failed: %s; data preserved", esp_err_to_name(read_error));
        dsr_config_health_storage(DSR_STORAGE_READ_ERROR,read_error);
        return read_error;
    }
    if (scan == SCAN_NONEMPTY) {
        ESP_LOGW("dsr_storage", "nonempty filesystem not mounted; data preserved");
        dsr_config_health_storage(DSR_STORAGE_NONEMPTY,error);
        return error;
    }
    if (scan == SCAN_REMNANT)
        ESP_LOGW("dsr_storage","format_remnant complete_blocks=%u/%u user_bytes=0; resuming interrupted first format",
                 (unsigned)remnant_blocks,(unsigned)(partition->size/BLOCK_BYTES));
    /* IDF 5.5.4 esp_spiffs_format() formats an unmounted blank partition twice.
     * Register-with-format does exactly one pass, ONLY after the full scan.
     * Formatting is synchronous: temporarily share priority 0 with idle tasks
     * under IDF's preemptive time slicing, rather than monopolising main's core.
     * No watchdog is disabled, fed manually or given an increased timeout.
     * This startup-only path allocates no additional task, stack or monitor. */
    dsr_config_health_storage(DSR_STORAGE_PROVISIONING,ESP_OK);
    const UBaseType_t priority=uxTaskPriorityGet(NULL);
    const int64_t started=esp_timer_get_time();
    ESP_LOGI("dsr_storage","format_started pass=1/1 blocks=%u bytes=%u priority=0 resume=%u",(unsigned)(partition->size/BLOCK_BYTES),(unsigned)partition->size,scan==SCAN_REMNANT?1u:0u);
    /* Optional LCD progress: begin/end run here in the caller, outside the
     * idle-priority window. The observer reads headers only via Flash. */
    atomic_store(&progress_partition,partition);
    if(observer&&observer->begin)observer->begin(scan==SCAN_REMNANT,(uint32_t)(partition->size/BLOCK_BYTES),remnant_blocks);
    vTaskPrioritySet(NULL,tskIDLE_PRIORITY);
    vTaskDelay(1);
    conf.format_if_mount_failed=true;
    error = esp_vfs_spiffs_register(&conf);
    vTaskPrioritySet(NULL,priority);
    if(observer&&observer->end)observer->end(error);
    atomic_store(&progress_partition,NULL);
    const int64_t elapsed=esp_timer_get_time()-started;
    ESP_LOGI("dsr_storage","format_finished result=0x%x elapsed_ms=%lld completed=%u/1",(unsigned)error,(long long)(elapsed/1000),error==ESP_OK?1u:0u);
    (void)elapsed; /* Host log shims can discard arguments. */
    if (error == ESP_OK) ESP_LOGI("dsr_storage","mount_ready existing=0");
    else ESP_LOGE("dsr_storage", "first provisioning failed: %s; no retry or erase", esp_err_to_name(error));
    dsr_config_health_storage(error==ESP_OK?DSR_STORAGE_READY:DSR_STORAGE_PROVISION_ERROR,error);
    return error;
}
