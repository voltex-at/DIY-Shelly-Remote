#include "dsr_clock.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include <time.h>
/* Canonical port of the reviewed native OTA lazy20x1s helper. Integrators must
 * replace dsr_native_ota_clock with a thin map to THIS function, not a second
 * esp_netif_sntp_init owner. Normal certificate/date checks stay enabled. */
dsr_clock_result_t dsr_clock_sync(bool (*cancelled)(void *),void *context)
{
 static bool initialized;
 if(time(NULL)>=(time_t)1735689600)return DSR_CLOCK_READY;
 if(cancelled&&cancelled(context))return DSR_CLOCK_CANCELLED;
 if(!initialized){
  esp_sntp_config_t config=ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  if(esp_netif_sntp_init(&config)!=ESP_OK)return DSR_CLOCK_UNAVAILABLE;
  initialized=true;
 }
 for(unsigned i=0;i<20;i++){
  if(cancelled&&cancelled(context))return DSR_CLOCK_CANCELLED;
  (void)esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000));
  if(time(NULL)>=(time_t)1735689600)return DSR_CLOCK_READY;
 }
 return DSR_CLOCK_UNAVAILABLE;
}
