#ifndef TEST_INSTALL_PARTITION_H
#define TEST_INSTALL_PARTITION_H
#include <stdint.h>
typedef struct {uint8_t type,subtype;uint32_t address,size;char label[17];} esp_partition_t;
#define ESP_PARTITION_TYPE_APP 0
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_SUBTYPE_APP_OTA_0 0x10
#define ESP_PARTITION_SUBTYPE_APP_OTA_1 0x11
#define ESP_PARTITION_SUBTYPE_DATA_SPIFFS 0x82
const esp_partition_t *esp_partition_find_first(unsigned type,unsigned subtype,const char *label);
#endif
