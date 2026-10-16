#include "dsr_ap_private.h"
#include "dsr_discovery.h"
#include <ctype.h>
#include <stdatomic.h>
#include <string.h>
static atomic_bool cancelled,sensitive,visible;
void dsr_ap_visibility(bool value){atomic_store(&visible,value);}
void dsr_ap_prepare(void){atomic_store(&cancelled,false);}
void dsr_ap_cancel(void){atomic_store(&cancelled,true);dsr_wifi_cancel_connect();}
bool dsr_ap_cancelled(void){return atomic_load(&cancelled);}
bool dsr_ap_sensitive(void){return atomic_load(&visible)||atomic_load(&sensitive)||dsr_wifi_temporary_active();}
void dsr_ap_execute(const dsr_job_t *job,dsr_event_t *event)
{
 event->ap_write_result=DSR_INVALID;
#ifndef DSR_BOARD_ES3C35P
 (void)job;event->result=DSR_UNSUPPORTED;
#else
 if(job->kind==DSR_JOB_AP_SCAN){event->result=dsr_wifi_scan(&event->scan);return;}
 if(job->kind==DSR_JOB_AP_RESTORE){event->result=dsr_wifi_restore_home(&event->wifi_detail);if(!dsr_wifi_temporary_active())dsr_lan_resume();atomic_store(&sensitive,false);return;}
 if(job->kind==DSR_JOB_AP_INSPECT){
  atomic_store(&sensitive,true);event->result=dsr_lan_suspend();
  if(event->result!=DSR_OK||dsr_ap_cancelled()){if(dsr_ap_cancelled())event->result=DSR_BUSY;dsr_lan_resume();atomic_store(&sensitive,false);return;}
  event->result=dsr_wifi_temporary_connect(&job->ap,&event->wifi_detail);
  if(event->result==DSR_OK&&!dsr_ap_cancelled())event->result=dsr_http_ap_inspect(job->ap.ssid,&event->ap_identity);
  if(dsr_ap_cancelled())event->result=DSR_BUSY;
  if(event->result!=DSR_OK){dsr_wifi_status_t detail;if(dsr_wifi_restore_home(&detail)!=DSR_OK)event->wifi_detail=detail;if(!dsr_wifi_temporary_active())dsr_lan_resume();atomic_store(&sensitive,false);}
  return;
 }
 dsr_settings_t home={0};esp_err_t error=dsr_settings_load(&home);
 if(error!=ESP_OK||!dsr_wifi_settings_valid(&home)||strcmp(home.ssid,job->settings.ssid))event->result=DSR_IO;
 else if(job->kind==DSR_JOB_AP_VERIFY){
  event->result=dsr_ap_cancelled()?DSR_BUSY:dsr_http_ap_verify_lan(&job->ap_identity,job->connection.device.host,&home);
 }else if(job->kind==DSR_JOB_AP_PROVISION){
  if(!job->ap_confirmed||!dsr_wifi_temporary_active()||dsr_ap_cancelled())event->ap_write_result=DSR_INVALID;
  else event->ap_write_result=dsr_http_ap_provision(&job->ap_identity,&home);
  event->result=DSR_OK; /* A successful home restore is not a provisioning ACK. */
 }
 memset(&home,0,sizeof(home));
 if(job->kind==DSR_JOB_AP_PROVISION){
  dsr_result_t restored=dsr_wifi_restore_home(&event->wifi_detail);
  if(restored!=DSR_OK)event->result=restored;
  if(!dsr_wifi_temporary_active())dsr_lan_resume();
  atomic_store(&sensitive,false);
 }
#endif
}
