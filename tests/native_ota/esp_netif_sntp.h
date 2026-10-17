#ifndef TEST_SNTP_H
#define TEST_SNTP_H
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
typedef struct {bool smooth_sync,server_from_dhcp,wait_for_sync,start;const char *servers[1];} esp_sntp_config_t;
#define ESP_NETIF_SNTP_DEFAULT_CONFIG(server) {.wait_for_sync=true,.start=true,.servers={server}}
esp_err_t esp_netif_sntp_init(const esp_sntp_config_t *config);
esp_err_t esp_netif_sntp_sync_wait(TickType_t timeout);
#endif
