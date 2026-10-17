#pragma once
#include "esp_netif.h"
#define ESP_IPADDR_TYPE_V4 0
typedef struct mdns_ip_addr {struct mdns_ip_addr *next;struct {int type;union {ip4_addr_t ip4;}u_addr;}addr;} mdns_ip_addr_t;
typedef struct mdns_result {struct mdns_result *next;unsigned port;mdns_ip_addr_t *addr;} mdns_result_t;
esp_err_t mdns_init(void);
esp_err_t mdns_query_ptr(const char *service,const char *protocol,unsigned timeout,unsigned limit,mdns_result_t **out);
void mdns_query_results_free(mdns_result_t *results);
