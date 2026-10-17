#ifndef TEST_ESP_EVENT_H
#define TEST_ESP_EVENT_H
#include "esp_err.h"
#include <stdint.h>
typedef const char *esp_event_base_t;
extern const char test_wifi_base[],test_ip_base[];
#define WIFI_EVENT test_wifi_base
#define IP_EVENT test_ip_base
#define ESP_EVENT_ANY_ID -1
#define WIFI_EVENT_STA_START 1
#define WIFI_EVENT_STA_STOP 2
#define WIFI_EVENT_STA_DISCONNECTED 3
#define IP_EVENT_STA_GOT_IP 4
typedef void (*esp_event_handler_t)(void *,esp_event_base_t,int32_t,void *);
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_register(esp_event_base_t base,int32_t id,esp_event_handler_t handler,void *arg);
#endif
