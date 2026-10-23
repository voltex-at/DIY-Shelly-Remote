#ifndef DSR_OTA_POWER_INTERNAL_H
#define DSR_OTA_POWER_INTERNAL_H
#include "dsr_ota_power.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
/* Startup/failed-start cleanup only; no running-task destruction. */
esp_err_t dsr_ota_power_init(SemaphoreHandle_t worker_slot);
void dsr_ota_power_deinit(void);
bool dsr_ota_power_poll(void);
#endif
