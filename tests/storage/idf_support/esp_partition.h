#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
typedef unsigned esp_partition_subtype_t;
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_SUBTYPE_ANY 0xff
#define ESP_PARTITION_SUBTYPE_DATA_SPIFFS 0x82
typedef struct {size_t address, size;bool encrypted,readonly;char label[17];} esp_partition_t;
const esp_partition_t *esp_partition_find_first(unsigned type,unsigned subtype,const char *label);
esp_err_t esp_partition_read(const esp_partition_t *p,size_t offset,void *out,size_t size);
esp_err_t esp_partition_write(const esp_partition_t *p,size_t offset,const void *data,size_t size);
esp_err_t esp_partition_erase_range(const esp_partition_t *p,size_t offset,size_t size);
