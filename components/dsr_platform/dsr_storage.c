#include "dsr_storage.h"
#include "esp_spiffs.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "dsr_config_health.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#if configUSE_PREEMPTION != 1 || configUSE_TIME_SLICING != 1
#error "Blank SPIFFS provisioning requires preemptive idle-priority time slicing"
#endif

esp_err_t dsr_storage_mount(void)
{
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
    uint8_t bytes[1024];
    for (size_t offset = 0; offset < partition->size; offset += sizeof(bytes)) {
        size_t count = partition->size - offset;
        if (count > sizeof(bytes)) count = sizeof(bytes);
        esp_err_t read_error = esp_partition_read(partition, offset, bytes, count);
        if (read_error != ESP_OK) {
            ESP_LOGE("dsr_storage", "blank check failed: %s; data preserved", esp_err_to_name(read_error));
            dsr_config_health_storage(DSR_STORAGE_READ_ERROR,read_error);
            return read_error;
        }
        for (size_t i = 0; i < count; i++) if (bytes[i] != 0xff) {
            ESP_LOGW("dsr_storage", "nonempty filesystem not mounted; data preserved");
            dsr_config_health_storage(DSR_STORAGE_NONEMPTY,error);
            return error;
        }
        if ((offset+count) % (256*1024) == 0 || offset+count == partition->size) {
            ESP_LOGI("dsr_storage","blank_check checked=%u total=%u",(unsigned)(offset+count),(unsigned)partition->size);
            vTaskDelay(1); /* Give idle/radio work CPU even during the read scan. */
        }
    }
    /* IDF 5.5.4 esp_spiffs_format() formats an unmounted blank partition twice.
     * Register-with-format does exactly one pass, ONLY after the full blank scan.
     * Formatting is synchronous: temporarily share priority 0 with idle tasks
     * under IDF's preemptive time slicing, rather than monopolising main's core.
     * No watchdog is disabled, fed manually or given an increased timeout.
     * This startup-only path allocates no additional task, stack or monitor. */
    dsr_config_health_storage(DSR_STORAGE_PROVISIONING,ESP_OK);
    const UBaseType_t priority=uxTaskPriorityGet(NULL);
    const int64_t started=esp_timer_get_time();
    ESP_LOGI("dsr_storage","format_started pass=1/1 blocks=%u bytes=%u priority=0",(unsigned)(partition->size/4096),(unsigned)partition->size);
    vTaskPrioritySet(NULL,tskIDLE_PRIORITY);
    vTaskDelay(1);
    conf.format_if_mount_failed=true;
    error = esp_vfs_spiffs_register(&conf);
    vTaskPrioritySet(NULL,priority);
    const int64_t elapsed=esp_timer_get_time()-started;
    ESP_LOGI("dsr_storage","format_finished result=0x%x elapsed_ms=%lld completed=%u/1",(unsigned)error,(long long)(elapsed/1000),error==ESP_OK?1u:0u);
    (void)elapsed; /* Host log shims can discard arguments. */
    if (error == ESP_OK) ESP_LOGI("dsr_storage","mount_ready existing=0");
    else ESP_LOGE("dsr_storage", "first provisioning failed: %s; no retry or erase", esp_err_to_name(error));
    dsr_config_health_storage(error==ESP_OK?DSR_STORAGE_READY:DSR_STORAGE_PROVISION_ERROR,error);
    return error;
}
