#include "dsr_ota_discovery.h"
#include "dsr_ota_fetch_private.h"
#include "esp_timer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

dsr_ota_fetch_result_t dsr_ota_discovery_fetch(dsr_ota_channel_t channel,
 const dsr_ota_fetch_control_t *control,dsr_ota_discovery_t *out,
 dsr_ota_release_result_t *why,int *http_status)
{
 if(why)*why=DSR_RELEASE_INVALID;
 if(http_status)*http_status=0;
 if(!out)return DSR_FETCH_ARGUMENT;
 memset(out,0,sizeof(*out));
 if(channel!=DSR_OTA_STABLE&&channel!=DSR_OTA_DEV)return DSR_FETCH_ARGUMENT;
 if(time(NULL)<(time_t)1735689600)return DSR_FETCH_CLOCK;
 int64_t start=esp_timer_get_time();
 if(control&&control->cancelled&&control->cancelled(control->context))return DSR_FETCH_CANCELLED;
 unsigned char *body=calloc(1,DSR_OTA_RELEASE_MAX+1);if(!body)return DSR_FETCH_NOMEM;
 dsr_ota_fetch_result_t result=DSR_FETCH_RELEASE;
 for(unsigned page=1;page<=(channel==DSR_OTA_DEV?DSR_OTA_DEV_PAGES:1u);page++){
  size_t length=0;unsigned entries=0;
  result=dsr_ota_fetch_discovery_page(channel,page,control,body,&length,http_status);
  if(result!=DSR_FETCH_OK)break;
  dsr_ota_release_result_t parsed=dsr_ota_discovery_parse(body,length,channel,out,&entries);
  if(why)*why=parsed;
  if(control&&control->cancelled&&control->cancelled(control->context)){result=DSR_FETCH_CANCELLED;break;}
  {int64_t now=esp_timer_get_time();if(now<start||now-start>=DSR_OTA_FETCH_BUDGET_US){result=DSR_FETCH_TIMEOUT;break;}}
  if(parsed==DSR_RELEASE_OK)break;
  result=DSR_FETCH_RELEASE;
  /* A one-release page holding a Stable or draft entry: look one page further. */
  if(parsed!=DSR_RELEASE_NOT_FOUND||channel!=DSR_OTA_DEV||entries!=1)break;
 }
 free(body);if(result!=DSR_FETCH_OK)memset(out,0,sizeof(*out));return result;
}
