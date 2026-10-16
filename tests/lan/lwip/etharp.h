#pragma once
#include "esp_netif.h"
#include <stddef.h>
#define ARP_TABLE_SIZE 16
struct netif {int dummy;};
struct eth_addr {int dummy;};
int etharp_get_entry(size_t index,ip4_addr_t **ip,struct netif **netif,struct eth_addr **mac);
