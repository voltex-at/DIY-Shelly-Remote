#ifndef TEST_NETIF_H
#define TEST_NETIF_H
#include "esp_err.h"
#include <stdint.h>
typedef struct {int dummy;} esp_netif_t;
typedef struct {uint8_t bytes[4];} test_ip4_t;
typedef struct {esp_netif_t *esp_netif;struct {test_ip4_t ip;} ip_info;} ip_event_got_ip_t;
#define IPSTR "%u.%u.%u.%u"
#define IP2STR(ip) (ip)->bytes[0],(ip)->bytes[1],(ip)->bytes[2],(ip)->bytes[3]
esp_err_t esp_netif_init(void);
esp_netif_t *esp_netif_create_default_wifi_sta(void);
#endif
