/* Actual time gate, modeled SDK SNTP only. No network/TLS/UTC authenticity proof. */
#include "dsr_ota_fetch.h"
#include "esp_netif_sntp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static unsigned checks,init_calls,wait_calls,mode;
static time_t wall;
static bool cancel_flag;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);abort();}}while(0)
dsr_ota_fetch_result_t dsr_native_ota_clock(const dsr_ota_fetch_control_t *control);
time_t __wrap_time(time_t *out){if(out)*out=wall;return wall;}
static bool cancelled(void *context){CHECK(context==&cancel_flag);return cancel_flag;}
esp_err_t esp_netif_sntp_init(const esp_sntp_config_t *config)
{init_calls++;CHECK(config&&config->start&&config->wait_for_sync&&!config->smooth_sync&&!config->server_from_dhcp&&!strcmp(config->servers[0],"pool.ntp.org"));return mode==4&&init_calls==1?ESP_FAIL:ESP_OK;}
esp_err_t esp_netif_sntp_sync_wait(TickType_t timeout)
{CHECK(timeout==1000);wait_calls++;if(mode==2&&wait_calls==3)wall=1735689600;if(mode==3&&wait_calls==4)cancel_flag=true;return ESP_OK;}
int main(int argc,char **argv)
{
 CHECK(argc==2);mode=(unsigned)strtoul(argv[1],NULL,10);dsr_ota_fetch_control_t control={cancelled,&cancel_flag};
 if(mode==0){wall=1735689600;CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_OK&&!init_calls&&!wait_calls);}
 if(mode==1){CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_CLOCK&&init_calls==1&&wait_calls==20);CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_CLOCK&&init_calls==1&&wait_calls==40);}
 if(mode==2){CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_OK&&init_calls==1&&wait_calls==3);CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_OK&&init_calls==1&&wait_calls==3);}
 if(mode==3){CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_CANCELLED&&init_calls==1&&wait_calls==4);}
 if(mode==4){CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_CLOCK&&init_calls==1&&!wait_calls);CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_CLOCK&&init_calls==2&&wait_calls==20);}
 if(mode==5){cancel_flag=true;CHECK(dsr_native_ota_clock(&control)==DSR_FETCH_CANCELLED&&!init_calls&&!wait_calls);}
 printf("PASS: %u actual clock-gate checks, mode%u; bounded SDK waits/cancel, no TLS-date bypass\n",checks,mode);return 0;
}
