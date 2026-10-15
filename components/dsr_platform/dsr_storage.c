#include "dsr_storage.h"
#include "esp_spiffs.h"
#include "esp_partition.h"
#include "esp_log.h"
#include <stdint.h>

esp_err_t dsr_storage_mount(void)
{
    const esp_vfs_spiffs_conf_t conf = {.base_path = "/dsr",
        .partition_label = "dsrdata", .max_files = 8,
        .format_if_mount_failed = false};
    esp_err_t error = esp_vfs_spiffs_register(&conf);
    if (error == ESP_OK) return ESP_OK;
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
            return read_error;
        }
        for (size_t i = 0; i < count; i++) if (bytes[i] != 0xff) {
            ESP_LOGW("dsr_storage", "nonempty filesystem not mounted; data preserved");
            return error;
        }
    }
    ESP_LOGI("dsr_storage", "confirmed fully erased data partition; first provisioning");
    error = esp_spiffs_format("dsrdata");
    if (error != ESP_OK) {
        ESP_LOGE("dsr_storage", "first provisioning failed: %s", esp_err_to_name(error));
        return error; /* Preserve the real error, e.g. NO_MEM, not old ESP_FAIL. */
    }
    error = esp_vfs_spiffs_register(&conf);
    if (error != ESP_OK) ESP_LOGE("dsr_storage", "mount after provisioning: %s", esp_err_to_name(error));
    return error;
}
