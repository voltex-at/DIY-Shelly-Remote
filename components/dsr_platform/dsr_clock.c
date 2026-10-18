#include "dsr_clock.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include <stdatomic.h>
#include <time.h>
/* Sole owner of the SDK SNTP service (native OTA maps to dsr_clock_sync).
 * Since 0.7.51 the LCD header clock starts it once Wi-Fi is up; the service
 * runs in the background and never blocks the UI or the worker. Normal
 * certificate/date checks stay enabled. */
static atomic_bool started;
bool dsr_clock_start(void)
{
 if(atomic_exchange(&started,true))return true;
 esp_sntp_config_t config=ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
 if(esp_netif_sntp_init(&config)==ESP_OK)return true;
 atomic_store(&started,false);return false;
}
dsr_clock_result_t dsr_clock_sync(bool (*cancelled)(void *),void *context)
{
 if(time(NULL)>=(time_t)1735689600)return DSR_CLOCK_READY;
 if(cancelled&&cancelled(context))return DSR_CLOCK_CANCELLED;
 if(!dsr_clock_start())return DSR_CLOCK_UNAVAILABLE;
 for(unsigned i=0;i<20;i++){
  if(cancelled&&cancelled(context))return DSR_CLOCK_CANCELLED;
  (void)esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000));
  if(time(NULL)>=(time_t)1735689600)return DSR_CLOCK_READY;
 }
 return DSR_CLOCK_UNAVAILABLE;
}
