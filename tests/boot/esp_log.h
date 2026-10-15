#pragma once
#include <stdio.h>
#include "esp_err.h"
static inline const char *esp_err_to_name(esp_err_t error) { (void)error; return "test"; }
#define ESP_LOGI(tag,...) do { (void)(tag); if (0) printf(__VA_ARGS__); } while (0)
#define ESP_LOGW(tag,...) ESP_LOGI(tag,__VA_ARGS__)
#define ESP_LOGE(tag,...) ESP_LOGI(tag,__VA_ARGS__)
