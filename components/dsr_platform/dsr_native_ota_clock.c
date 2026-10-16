#include "dsr_ota_fetch.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include <time.h>
/* Sole I/O worker only. Lazily start SDK SNTP after an explicit LCD check.
 * This supplies plausible UTC for normal X.509 checks, never publisher trust.
 * No date-verification bypass. Each wait is1s with cooperative cancellation. */
dsr_ota_fetch_result_t dsr_native_ota_clock(const dsr_ota_fetch_control_t *control)
{
 static bool initialized;
 if(time(NULL)>=(time_t)1735689600)return DSR_FETCH_OK;
 if(control&&control->cancelled&&control->cancelled(control->context))return DSR_FETCH_CANCELLED;
 if(!initialized){
  esp_sntp_config_t config=ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  if(esp_netif_sntp_init(&config)!=ESP_OK)return DSR_FETCH_CLOCK;
  initialized=true;
 }
 for(unsigned i=0;i<20;i++){
  if(control&&control->cancelled&&control->cancelled(control->context))return DSR_FETCH_CANCELLED;
  (void)esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000));
  if(time(NULL)>=(time_t)1735689600)return DSR_FETCH_OK;
 }
 return DSR_FETCH_CLOCK;
}
