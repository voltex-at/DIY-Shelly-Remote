#include "dsr_native_ota.h"
#include "dsr_boot.h"
#include "dsr_ota_stage.h"
#include "esp_app_desc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

dsr_ota_fetch_result_t dsr_native_ota_clock(const dsr_ota_fetch_control_t *control);
typedef struct {
 dsr_native_ota_snapshot_t view;
 dsr_ota_discovery_t discovery;
 dsr_ota_verified_t verified;
 dsr_ota_install_report_t report;
 char base[32];
 int64_t available_at;
 dsr_job_kind_t kind;
 bool queued,running,power;
} actor_t;
static actor_t actor;
static SemaphoreHandle_t mutex;
static atomic_bool cancellation;
_Static_assert(sizeof(actor_t)<=2304,"bounded native updater state, no job/event growth");
static bool lock(void){return mutex&&xSemaphoreTake(mutex,portMAX_DELAY)==pdTRUE;}
static void unlock(void){xSemaphoreGive(mutex);}
static bool cancelled(void *context){(void)context;return atomic_load(&cancellation);}
static bool local_version(const char *version,char base[32])
{
 if(!version||!memchr(version,0,32))return false;
 size_t n=0;
 for(unsigned part=0;part<3;part++){
  size_t begin=n;
  while(version[n]>='0'&&version[n]<='9'){if(n>=29)return false;n++;}
  if(n==begin||(n-begin>1&&version[begin]=='0'))return false;
  if(part<2){if(version[n++]!='.')return false;}
 }
 if(version[n]&&version[n]!='-')return false;
 memcpy(base,version,n);base[n]=0;return true;
}
esp_err_t dsr_native_ota_init(void)
{
 if(mutex)return ESP_ERR_INVALID_STATE;
 mutex=xSemaphoreCreateMutex();if(!mutex)return ESP_ERR_NO_MEM;
 const char *version=esp_app_get_description()->version;
 memcpy(actor.view.installed,version,sizeof(actor.view.installed));actor.view.installed[31]=0;
 actor.view.channel=DSR_OTA_STABLE;
 if(!local_version(version,actor.base)){actor.view.state=DSR_NATIVE_FAILED;actor.view.error=DSR_NATIVE_ERROR_LOCAL_VERSION;}
 return ESP_OK;
}
static void forget_candidate(void)
{memset(&actor.discovery,0,sizeof(actor.discovery));memset(&actor.verified,0,sizeof(actor.verified));memset(actor.view.version,0,sizeof(actor.view.version));memset(actor.view.notes,0,sizeof(actor.view.notes));actor.view.total=0;actor.available_at=0;}
dsr_result_t dsr_native_ota_prepare(uint32_t id,dsr_job_kind_t kind,dsr_ota_channel_t channel,bool power)
{
 if(!id||(kind!=DSR_JOB_OTA_CHECK&&kind!=DSR_JOB_OTA_INSTALL)||(channel!=DSR_OTA_STABLE&&channel!=DSR_OTA_DEV))return DSR_INVALID;
 if(!lock())return DSR_UNSUPPORTED;
 dsr_result_t result=DSR_BUSY;
 if(actor.queued||actor.running||actor.view.state==DSR_NATIVE_STAGED||dsr_ota_stage_status()!=DSR_OTA_STAGE_IDLE)goto done;
 if(!actor.base[0]){result=DSR_INVALID;goto done;}
 if(kind==DSR_JOB_OTA_INSTALL){
  if(actor.view.state!=DSR_NATIVE_AVAILABLE||actor.view.channel!=channel){result=DSR_INVALID;goto done;}
  int64_t now=esp_timer_get_time();
  if(now<actor.available_at||now-actor.available_at>=120000000){forget_candidate();actor.view.state=DSR_NATIVE_FAILED;actor.view.error=DSR_NATIVE_ERROR_EXPIRED;result=DSR_INVALID;goto done;}
  if(!power){actor.view.error=DSR_NATIVE_ERROR_POWER;result=DSR_INVALID;goto done;}
 }else forget_candidate();
 actor.kind=kind;actor.queued=true;actor.power=power;actor.view.operation_id=id;actor.view.channel=channel;
 actor.view.state=kind==DSR_JOB_OTA_CHECK?DSR_NATIVE_CHECKING:DSR_NATIVE_INSTALLING;
 actor.view.error=DSR_NATIVE_ERROR_NONE;actor.view.received=0;actor.view.phase=DSR_INSTALL_MANIFEST;
 actor.view.fetch_result=DSR_FETCH_ARGUMENT;actor.view.manifest_result=DSR_MANIFEST_INVALID;actor.view.install_result=DSR_INSTALL_ARGUMENT;actor.view.http_status=0;actor.view.boot_selection_attempted=false;memset(&actor.report,0,sizeof(actor.report));
 atomic_store(&cancellation,false);result=DSR_OK;
done:unlock();return result;
}
bool dsr_native_ota_admitted(uint32_t id,dsr_job_kind_t kind)
{if(!lock())return false;bool admitted=actor.queued&&!actor.running&&actor.view.operation_id==id&&actor.kind==kind;unlock();return admitted;}
void dsr_native_ota_reject(uint32_t id)
{if(!lock())return;if(actor.queued&&actor.view.operation_id==id){actor.queued=false;forget_candidate();actor.view.state=DSR_NATIVE_FAILED;actor.view.error=DSR_NATIVE_ERROR_BUSY;}unlock();}
void dsr_native_ota_cancel(void)
{
 if(!lock())return;
 if(actor.queued||actor.running)atomic_store(&cancellation,true);
 else if(actor.view.state==DSR_NATIVE_AVAILABLE){forget_candidate();actor.view.state=DSR_NATIVE_CANCELLED;}
 unlock();
}
void dsr_native_ota_snapshot(dsr_native_ota_snapshot_t *out)
{
 if(!out)return;
 memset(out,0,sizeof(*out));if(!lock()){out->state=DSR_NATIVE_FAILED;out->error=DSR_NATIVE_ERROR_MEMORY;return;}
 if(actor.view.state==DSR_NATIVE_AVAILABLE){int64_t now=esp_timer_get_time();if(now<actor.available_at||now-actor.available_at>=120000000){forget_candidate();actor.view.state=DSR_NATIVE_FAILED;actor.view.error=DSR_NATIVE_ERROR_EXPIRED;}}
 *out=actor.view;unlock();
}
static void progress(void *context,dsr_ota_install_phase_t phase,uint32_t received,uint32_t total)
{(void)context;if(!lock())return;actor.view.phase=phase;actor.view.received=received;actor.view.total=total;unlock();}
static dsr_native_ota_error_t fetch_error(dsr_ota_fetch_result_t result,int status)
{
 if(result==DSR_FETCH_CLOCK)return DSR_NATIVE_ERROR_CLOCK;
 if(result==DSR_FETCH_NOMEM)return DSR_NATIVE_ERROR_MEMORY;
 if(status==404)return DSR_NATIVE_ERROR_NO_RELEASE;
 return DSR_NATIVE_ERROR_CATALOGUE;
}
void dsr_native_ota_execute(const dsr_job_t *job,dsr_event_t *event)
{
 if(!event)return;
 event->result=DSR_INVALID;
 if(!job||!lock())return;
 if(!actor.queued||actor.running||actor.view.operation_id!=job->id||actor.kind!=job->kind){unlock();return;}
 actor.queued=false;actor.running=true;unlock();
 /* Only this worker writes these private workspaces until the terminal locked
  * publication. Readers receive the separate fixed view, never credentials. */
 dsr_native_ota_error_t error=DSR_NATIVE_ERROR_NONE;
 dsr_ota_fetch_result_t fetch=DSR_FETCH_ARGUMENT;dsr_manifest_result_t manifest=DSR_MANIFEST_INVALID;
 dsr_ota_install_result_t installed=DSR_INSTALL_ARGUMENT;
 bool success=false;int http_status=0;size_t slot_size=0;
 dsr_ota_policy_t policy={actor.base,actor.view.channel,0};
 dsr_ota_fetch_control_t control={cancelled,NULL};
 if(cancelled(NULL))goto finish;
 if(!dsr_boot_ready()){error=DSR_NATIVE_ERROR_BOOT;goto finish;}
 if(!dsr_wifi_connected()||dsr_wifi_temporary_active()){error=DSR_NATIVE_ERROR_OFFLINE;goto finish;}
 if(dsr_ota_install_ready(&slot_size)!=DSR_INSTALL_OK){error=DSR_NATIVE_ERROR_SLOT;goto finish;}
 policy.inactive_slot_size=slot_size;
 if(job->kind==DSR_JOB_OTA_CHECK){
  fetch=dsr_native_ota_clock(&control);
  if(fetch!=DSR_FETCH_OK){error=fetch_error(fetch,0);goto finish;}
  dsr_ota_release_result_t release;
  fetch=dsr_ota_discovery_fetch(policy.selected_channel,&control,&actor.discovery,&release,&http_status);
  if(fetch!=DSR_FETCH_OK){error=fetch==DSR_FETCH_RELEASE&&release==DSR_RELEASE_NOT_FOUND?DSR_NATIVE_ERROR_NO_RELEASE:fetch_error(fetch,http_status);goto finish;}
  fetch=dsr_ota_fetch_manifest(actor.discovery.candidate.manifest_url,&policy,&control,&actor.verified,&manifest);
  if(fetch!=DSR_FETCH_OK){error=DSR_NATIVE_ERROR_MANIFEST;goto finish;}
  success=true;
 }else{
  dsr_ota_install_control_t install_control={cancelled,progress,NULL};
  installed=dsr_ota_install(actor.discovery.candidate.manifest_url,&policy,&actor.verified,&install_control,actor.power,&actor.report);
  fetch=actor.report.fetch_result;manifest=actor.report.manifest_result;
  success=installed==DSR_INSTALL_OK;
  if(!success)error=installed==DSR_INSTALL_CONSENT_CHANGED?DSR_NATIVE_ERROR_CHANGED:DSR_NATIVE_ERROR_INSTALL;
 }
finish:
 if(!lock())return;
 actor.running=false;actor.view.fetch_result=fetch;actor.view.manifest_result=manifest;actor.view.install_result=installed;actor.view.http_status=http_status;
 actor.view.boot_selection_attempted=job->kind==DSR_JOB_OTA_INSTALL&&actor.report.boot_selection_attempted;
 if(success&&job->kind==DSR_JOB_OTA_INSTALL){actor.view.state=DSR_NATIVE_STAGED;actor.view.phase=DSR_INSTALL_STAGED;event->result=DSR_OK;}
 else if(actor.view.boot_selection_attempted){forget_candidate();actor.view.state=DSR_NATIVE_FAILED;actor.view.error=DSR_NATIVE_ERROR_INSTALL;event->result=DSR_IO;}
 else if(cancelled(NULL)){forget_candidate();actor.view.state=DSR_NATIVE_CANCELLED;actor.view.error=DSR_NATIVE_ERROR_NONE;event->result=DSR_IO;}
 else if(success){
  actor.view.state=DSR_NATIVE_AVAILABLE;actor.available_at=esp_timer_get_time();actor.view.total=actor.verified.size;
  memcpy(actor.view.version,actor.verified.version,sizeof(actor.view.version));memcpy(actor.view.notes,actor.discovery.notes,sizeof(actor.view.notes));event->result=DSR_OK;
 }else {forget_candidate();actor.view.state=DSR_NATIVE_FAILED;actor.view.error=error;event->result=DSR_IO;}
 unlock();
}
