#ifndef TEST_NVS_H
#define TEST_NVS_H
#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>
typedef int nvs_handle_t;
#define NVS_READONLY 0
#define NVS_READWRITE 1
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *h);
esp_err_t nvs_get_u32(nvs_handle_t h, const char *key, uint32_t *out);
esp_err_t nvs_set_u32(nvs_handle_t h, const char *key, uint32_t value);
esp_err_t nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *out);
esp_err_t nvs_set_u8(nvs_handle_t h, const char *key, uint8_t value);
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *size);
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t size);
esp_err_t nvs_commit(nvs_handle_t h);
void nvs_close(nvs_handle_t h);
#endif
