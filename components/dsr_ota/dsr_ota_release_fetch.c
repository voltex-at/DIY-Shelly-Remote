#include "dsr_ota_release_fetch.h"
#include "dsr_ota_fetch_private.h"
#include "esp_timer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
 unsigned char body[DSR_OTA_RELEASE_MAX+1];
 char url[160];
} release_fetch_t;
_Static_assert(sizeof(release_fetch_t)<=16640,"separate bounded release-body workspace");
dsr_ota_fetch_result_t dsr_ota_fetch_release(const dsr_ota_release_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_release_candidate_t *out,
 dsr_ota_release_result_t *why)
{
 if(why)*why=DSR_RELEASE_INVALID;
 if(!out)return DSR_FETCH_ARGUMENT;
 memset(out,0,sizeof(*out));
 dsr_ota_release_result_t checked=dsr_ota_release_policy_check(policy);
 if(checked!=DSR_RELEASE_OK){if(why)*why=checked;return DSR_FETCH_ARGUMENT;}
 if(time(NULL)<(time_t)1735689600)return DSR_FETCH_CLOCK;
 int64_t started=esp_timer_get_time();
 if(control&&control->cancelled&&control->cancelled(control->context))return DSR_FETCH_CANCELLED;
 release_fetch_t *s=calloc(1,sizeof(*s));if(!s)return DSR_FETCH_NOMEM;
 strcpy(s->url,"https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/tags/");
 strcat(s->url,policy->expected_tag);
 size_t length=0;
 dsr_ota_fetch_result_t result=dsr_ota_fetch_metadata_body(s->url,control,s->body,&length);
 /* Transport clients and their heap context have been destroyed before cJSON.
  * Reuse exactly the received bytes; no NUL-terminated parse or copy. */
 if(result==DSR_FETCH_OK){
  dsr_ota_release_result_t parsed=dsr_ota_release_candidate_parse(s->body,length,policy,out);
  if(why)*why=parsed;
  if(parsed!=DSR_RELEASE_OK)result=DSR_FETCH_RELEASE;
  else if(control&&control->cancelled&&control->cancelled(control->context))result=DSR_FETCH_CANCELLED;
  else{
   int64_t now=esp_timer_get_time();
   if(now<started||now-started>=DSR_OTA_FETCH_BUDGET_US)result=DSR_FETCH_TIMEOUT;
  }
 }
 if(result!=DSR_FETCH_OK)memset(out,0,sizeof(*out));
 free(s);return result;
}
