#pragma once
#include "esp_err.h"
#include <stdint.h>
typedef struct {uint8_t bytes[4];} ip4_addr_t;
typedef struct {int dummy;} esp_netif_t;
typedef struct {ip4_addr_t ip,netmask;} esp_netif_ip_info_t;
#define IPSTR "%u.%u.%u.%u"
#define IP2STR(ip) (ip)->bytes[0],(ip)->bytes[1],(ip)->bytes[2],(ip)->bytes[3]
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key);
esp_err_t esp_netif_get_ip_info(esp_netif_t *netif,esp_netif_ip_info_t *out);
