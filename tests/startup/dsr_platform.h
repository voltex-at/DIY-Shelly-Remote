#pragma once
#include "esp_err.h"
#include <stdint.h>
typedef struct { char ssid[33], password[65];uint8_t brightness;uint16_t idle_seconds; } dsr_settings_t;
esp_err_t dsr_settings_load(dsr_settings_t *settings);
esp_err_t dsr_worker_start(void);
esp_err_t dsr_wifi_start(const dsr_settings_t *settings);
