#pragma once
#include "esp_netif.h"
char *ip4addr_ntoa_r(const ip4_addr_t *address,char *text,int length);
