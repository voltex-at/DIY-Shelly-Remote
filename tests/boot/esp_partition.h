#pragma once
#include <stddef.h>
#include "esp_err.h"
typedef struct { size_t size; } esp_partition_t;
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_SUBTYPE_DATA_SPIFFS 0x82
const esp_partition_t *esp_partition_find_first(unsigned type, unsigned subtype, const char *label);
esp_err_t esp_partition_read(const esp_partition_t *partition, size_t offset, void *bytes, size_t count);
