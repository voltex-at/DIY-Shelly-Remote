#include "dsr_ui_task.h"
#include "dsr_storage.h"
#include "freertos/task.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); abort(); } } while (0)
static bool task_failure = true;
static unsigned task_calls, delay_calls;
static bool allow_run;
static unsigned held;
static bool startup_ready(void) { return allow_run; }
static void (*task_entry)(void *);
static StackType_t *reserved_stack;
static StaticTask_t *reserved_tcb;
static jmp_buf loop_done;

TaskHandle_t xTaskCreateStatic(void (*entry)(void *), const char *name,
    uint32_t bytes, void *argument, unsigned priority,
    StackType_t *stack, StaticTask_t *tcb)
{
    task_calls++;
    CHECK(!strcmp(name,"dsr_ui") && bytes == 8192 && priority == 4 && !argument);
    CHECK(stack && tcb);
    /* The retry must reuse permanent storage, not an expired main stack. */
    if (reserved_stack) CHECK(stack == reserved_stack && tcb == reserved_tcb);
    reserved_stack = stack; reserved_tcb = tcb; task_entry = entry;
    return task_failure ? NULL : (void *)1;
}
uint32_t lv_timer_handler(void)
{ const uint32_t delays[] = {0, 15, UINT32_MAX}; CHECK(delay_calls < 3); return delays[delay_calls]; }
void vTaskDelay(TickType_t ticks)
{
    if (!allow_run) { CHECK(ticks == 20); held++; longjmp(loop_done,1); }
    const unsigned expected[] = {10, 15, 20}; CHECK(ticks == expected[delay_calls]);
    if (++delay_calls == 3) longjmp(loop_done,1);
}

static esp_partition_t partition = {.size = 0x3e0000};
static bool missing, nonempty;
static esp_err_t mount_error, format_error, read_error, second_mount_error;
static unsigned mount_calls, format_calls, read_calls;
static size_t read_bytes;
esp_err_t esp_vfs_spiffs_register(const esp_vfs_spiffs_conf_t *config)
{
    CHECK(!strcmp(config->base_path,"/dsr") && !strcmp(config->partition_label,"dsrdata"));
    CHECK(config->max_files == 8 && !config->format_if_mount_failed);
    return mount_calls++ == 0 ? mount_error : second_mount_error;
}
const esp_partition_t *esp_partition_find_first(unsigned type,unsigned subtype,const char *label)
{ CHECK(type == 1 && subtype == 0x82 && !strcmp(label,"dsrdata")); return missing ? NULL : &partition; }
esp_err_t esp_partition_read(const esp_partition_t *p,size_t offset,void *bytes,size_t count)
{
    CHECK(p == &partition && offset == read_bytes && count > 0 && count <= 1024 && offset+count <= p->size);
    read_calls++; read_bytes += count; memset(bytes,0xff,count);
    if (nonempty && offset+count == p->size) ((unsigned char *)bytes)[count-1] = 0;
    return read_error;
}
esp_err_t esp_spiffs_format(const char *label)
{ CHECK(!strcmp(label,"dsrdata") && read_bytes == partition.size); format_calls++; return format_error; }
static void reset_storage(void)
{
    missing=nonempty=false; mount_error=ESP_FAIL; format_error=read_error=second_mount_error=ESP_OK;
    mount_calls=format_calls=read_calls=0; read_bytes=0; partition.size=0x3e0000;
}
int main(void)
{
    /* Dynamic task creation is deliberately absent from this link: restoring
     * the 0.7.1 xTaskCreate path breaks the regression target at link time. */
    CHECK(dsr_ui_task_start(NULL) == ESP_ERR_INVALID_ARG && task_calls == 0);
    CHECK(dsr_ui_task_start(startup_ready) == ESP_ERR_NO_MEM);
    task_failure=false; CHECK(dsr_ui_task_start(startup_ready) == ESP_OK);
    CHECK(dsr_ui_task_start(startup_ready) == ESP_ERR_INVALID_STATE && task_calls == 2);
    if (!setjmp(loop_done)) task_entry(NULL);
    CHECK(held == 1 && delay_calls == 0); /* No LVGL callbacks before readiness. */
    allow_run=true;
    if (!setjmp(loop_done)) task_entry(NULL);
    CHECK(delay_calls == 3);
    allow_run=false;
    if (!setjmp(loop_done)) task_entry(NULL);
    CHECK(held == 2 && delay_calls == 3); /* A latched failure closes the gate. */
    reset_storage(); mount_error=ESP_OK;
    CHECK(dsr_storage_mount() == ESP_OK && mount_calls == 1 && !format_calls && !read_calls);
    reset_storage(); mount_error=ESP_ERR_NO_MEM;
    CHECK(dsr_storage_mount() == ESP_ERR_NO_MEM && !read_calls && !format_calls);
    reset_storage(); mount_error=ESP_ERR_INVALID_STATE;
    CHECK(dsr_storage_mount() == ESP_ERR_INVALID_STATE && !read_calls && !format_calls);
    reset_storage(); missing=true;
    CHECK(dsr_storage_mount() == ESP_FAIL && !format_calls);
    reset_storage(); read_error=ESP_ERR_INVALID_SIZE;
    CHECK(dsr_storage_mount() == ESP_ERR_INVALID_SIZE && !format_calls);
    reset_storage(); nonempty=true;
    CHECK(dsr_storage_mount() == ESP_FAIL && !format_calls && mount_calls == 1);
    reset_storage();
    CHECK(dsr_storage_mount() == ESP_OK && format_calls == 1 && mount_calls == 2);
    CHECK(read_bytes == 0x3e0000);
    reset_storage(); format_error=ESP_ERR_NO_MEM;
    CHECK(dsr_storage_mount() == ESP_ERR_NO_MEM && format_calls == 1 && mount_calls == 1);
    reset_storage(); second_mount_error=ESP_ERR_NO_MEM;
    CHECK(dsr_storage_mount() == ESP_ERR_NO_MEM && format_calls == 1 && mount_calls == 2);
    reset_storage(); partition.size=1027;
    CHECK(dsr_storage_mount() == ESP_OK && read_calls == 2 && read_bytes == 1027);
    printf("PASS: %u boot-memory checks; static 8192-byte UI task, retry/guard/timing, blank-only provisioning, NO_MEM/read/nonempty preservation; host fixtures, not physical boot\n",checks);
    return 0;
}
