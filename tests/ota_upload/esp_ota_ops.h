#ifndef TEST_OTA_OPS_H
#define TEST_OTA_OPS_H
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_partition.h"
typedef uint32_t esp_ota_handle_t;
esp_err_t esp_ota_begin(const esp_partition_t *partition,size_t size,esp_ota_handle_t *handle);
esp_err_t esp_ota_write(esp_ota_handle_t handle,const void *data,size_t length);
esp_err_t esp_ota_end(esp_ota_handle_t handle);
esp_err_t esp_ota_abort(esp_ota_handle_t handle);
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start);
const esp_partition_t *esp_ota_get_running_partition(void);
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *partition);
#endif
