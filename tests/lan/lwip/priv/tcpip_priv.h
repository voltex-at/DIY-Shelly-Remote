#pragma once
typedef int err_t;
#define ERR_OK 0
struct tcpip_api_call_data {int dummy;};
err_t tcpip_api_call(err_t (*fn)(struct tcpip_api_call_data *),struct tcpip_api_call_data *argument);
