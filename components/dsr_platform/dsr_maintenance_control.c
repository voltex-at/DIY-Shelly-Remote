#include "dsr_maintenance_control.h"
#include "dsr_maintenance_grant.h"
#include "dsr_maintenance.h"
#include "dsr_maintenance_private.h"
#include "dsr_maintenance_io.h"
#include "dsr_maintenance_store.h"
#include "dsr_activity_internal.h"
#include "dsr_backup_sources.h"
#include "dsr_restart_internal.h"
#include "dsr_boot.h"
#include "esp_timer.h"
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
static portMUX_TYPE control_lock=portMUX_INITIALIZER_UNLOCKED;
static void lock(void){portENTER_CRITICAL(&control_lock);}
static void unlock(void){portEXIT_CRITICAL(&control_lock);}
#else
#include <stdatomic.h>
static atomic_flag control_lock=ATOMIC_FLAG_INIT;
static void lock(void){while(atomic_flag_test_and_set_explicit(&control_lock,memory_order_acquire)){};}
static void unlock(void){atomic_flag_clear_explicit(&control_lock,memory_order_release);}
#endif
static struct {
 dsr_maint_grant_t grant;dsr_maint_stage_t *candidate;
 uint32_t revision;uint8_t photos,connections,rules;bool job_busy,committed;
} control;
typedef struct {dsr_maint_request_t request;bool usb;} confirm_job_t;
static uint32_t clock_ms(void){return (uint32_t)((uint64_t)esp_timer_get_time()/1000);}
static void wipe(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
static bool live(void){uint8_t previous=control.grant.state;bool ok=dsr_maint_grant_live(&control.grant,dsr_maintenance_generation(),clock_ms());if(previous!=control.grant.state)control.revision++;return ok;}
static bool matching(const dsr_maint_request_t *r)
{return r&&r->op==control.grant.request.op&&!memcmp(r->nonce,control.grant.request.nonce,16)&&!memcmp(r->session,control.grant.request.session,32)&&!memcmp(r->candidate_sha256,control.grant.request.candidate_sha256,32);}
dsr_result_t dsr_maintenance_request(const dsr_maint_request_t *r)
{
 if(!r||r->op==DSR_MAINT_IMPORT_COMMIT)return DSR_INVALID;
 if(!dsr_boot_ready()||dsr_activity_restart_pending())return DSR_BUSY;
 lock();dsr_result_t result=control.job_busy||control.candidate||control.committed?DSR_BUSY:
  dsr_maint_grant_begin(&control.grant,r,dsr_maintenance_generation(),clock_ms());
 if(result==DSR_OK){control.photos=control.connections=control.rules=0;control.revision++;}unlock();return result;
}
bool dsr_maintenance_take(const dsr_maint_request_t *r)
{
 lock();bool ok=!control.job_busy&&!control.committed&&dsr_maint_grant_take(&control.grant,r,dsr_maintenance_generation(),clock_ms());if(ok)control.revision++;unlock();return ok;
}
void dsr_maintenance_finished(const uint8_t id[16],dsr_result_t result)
{if(!id)return;lock();if(!memcmp(id,control.grant.request.nonce,16)){dsr_maint_grant_finish(&control.grant,result);control.revision++;}unlock();}
bool dsr_maintenance_running(const dsr_maint_request_t *r)
{
 lock();bool ok=matching(r)&&control.grant.state==DSR_GRANT_RUNNING&&control.grant.generation==dsr_maintenance_generation()&&(uint32_t)(clock_ms()-control.grant.started)<DSR_MAINT_CONSENT_MS;
 if(!ok&&matching(r))dsr_maint_grant_finish(&control.grant,DSR_CONFLICT);
 unlock();return ok;
}
void dsr_maintenance_session_changed(const uint8_t session[32])
{if(!session)return;lock();if(!control.committed){dsr_maint_grant_invalidate_session(&control.grant,session);if(control.grant.state==DSR_GRANT_RUNNING&&!memcmp(session,control.grant.request.session,32))dsr_maint_grant_finish(&control.grant,DSR_CONFLICT);control.revision++;}unlock();}
bool dsr_maintenance_session_matches(const uint8_t session[32])
{if(!session)return false;lock();bool ok=!memcmp(session,control.grant.request.session,32);unlock();return ok;}
bool dsr_maintenance_cancel(const uint8_t id[16])
{
 if(!id)return false;
 lock();bool ok=false;if(!control.committed&&!memcmp(id,control.grant.request.nonce,16)){
  ok=dsr_maint_grant_cancel(&control.grant,id);if(control.grant.state==DSR_GRANT_RUNNING&&!control.job_busy){dsr_maint_grant_finish(&control.grant,DSR_CONFLICT);ok=true;}if(ok)control.revision++;
 }unlock();return ok;
}
void dsr_maintenance_status(dsr_maint_status_t *out)
{
 if(!out)return;
 memset(out,0,sizeof(*out));lock();(void)live();dsr_maint_grant_t *g=&control.grant;
 out->revision=control.revision;out->op=(uint8_t)g->request.op;out->result=g->result;out->photos=control.photos;out->connections=control.connections;out->rules=control.rules;
 memcpy(out->request_id,g->request.nonce,16);if(control.candidate)memcpy(out->candidate_id,g->request.nonce,16);
 out->phase=control.committed?DSR_MAINT_COMMITTED:control.job_busy?DSR_MAINT_WAITING:
  g->state==DSR_GRANT_WAIT?DSR_MAINT_CONFIRM:g->state==DSR_GRANT_READY?DSR_MAINT_GRANTED:
  g->state==DSR_GRANT_RUNNING?(g->request.op==DSR_MAINT_EXPORT?DSR_MAINT_CAPTURING:DSR_MAINT_VALIDATING):
  g->state==DSR_GRANT_DONE?(g->result==DSR_OK?DSR_MAINT_COMPLETE:DSR_MAINT_FAULT):DSR_MAINT_IDLE;
 out->can_confirm=g->state==DSR_GRANT_WAIT&&!control.job_busy&&!control.committed;out->usb_required=g->usb_required;unlock();
}
bool dsr_maintenance_sensitive(void)
{lock();(void)live();bool result=control.job_busy||control.candidate||control.committed||control.grant.state==DSR_GRANT_WAIT||control.grant.state==DSR_GRANT_READY||control.grant.state==DSR_GRANT_RUNNING;unlock();return result;}
dsr_result_t dsr_maintenance_candidate(dsr_maint_stage_t *stage,const uint8_t session[32])
{
 if(!stage||!session)return DSR_INVALID;
 dsr_maint_record_t record;dsr_maint_request_t request={.op=DSR_MAINT_IMPORT_COMMIT};
 if(!dsr_maint_stage_record(stage,&record))return DSR_INVALID;
 dsr_result_t result=dsr_maint_io_random(request.nonce,16);if(result!=DSR_OK)goto done;
 memcpy(request.session,session,32);memcpy(request.candidate_sha256,record.candidate_sha256,32);
 lock();if(control.job_busy||control.candidate||control.committed||control.grant.state!=DSR_GRANT_RUNNING||control.grant.request.op!=DSR_MAINT_IMPORT_UPLOAD||memcmp(session,control.grant.request.session,32)||record.generation!=control.grant.generation||record.generation!=dsr_maintenance_generation())result=DSR_CONFLICT;
 else{
  dsr_maint_grant_finish(&control.grant,DSR_OK);result=dsr_maint_grant_begin(&control.grant,&request,record.generation,clock_ms());
  if(result==DSR_OK){control.candidate=stage;control.photos=record.photos;control.connections=record.connections;control.rules=record.rules;control.revision++;}
 }unlock();
done:
 wipe(&record,sizeof(record));wipe(&request,sizeof(request));return result;
}
static dsr_result_t stop_bonds(const dsr_maint_lease_t *lease)
{cJSON *root=cJSON_CreateObject();if(!root)return DSR_FULL;dsr_result_t r=dsr_maintenance_bonds_capture(lease,root);dsr_backup_json_discard(root);return r;}
static dsr_result_t reset_commit(const dsr_maint_lease_t *lease)
{
 dsr_maint_record_t record={.sequence=1,.generation=lease->generation,.op=DSR_MAINT_RESET,.phase=DSR_MAINT_PREPARING};
 dsr_result_t result=dsr_backup_device_id(record.device);if(result==DSR_OK)result=dsr_maint_io_identifier(record.operation_id);
 if(result!=DSR_OK)goto done;
 if(!dsr_activity_maintenance_commit_begin(lease->reservation,lease->generation)){result=DSR_CONFLICT;goto done;}
 result=dsr_maint_store_begin(&record);
 if(result==DSR_OK){record.phase=DSR_MAINT_COMMIT;result=dsr_maint_store_write(&record);}
 if(result!=DSR_OK){
  dsr_maint_record_t current;bool found,pending;dsr_result_t read=dsr_maint_store_load(&current,&found,&pending);
  if(read==DSR_OK&&found&&pending&&!memcmp(current.operation_id,record.operation_id,16)&&current.phase==DSR_MAINT_PREPARING){
   if(dsr_maint_store_finish(&current,true)==DSR_OK)(void)dsr_activity_maintenance_commit_abort(lease->reservation,lease->generation);
  }else if(read==DSR_OK&&!found&&!pending)(void)dsr_activity_maintenance_commit_abort(lease->reservation,lease->generation);
  wipe(&current,sizeof(current));
 }
done:
 wipe(&record,sizeof(record));return result;
}
static void confirm_run(void *owned)
{
 confirm_job_t *j=owned;dsr_result_t result=DSR_CONFLICT;dsr_maint_lease_t lease={0};dsr_maint_stage_t *stage=NULL;
 lock();bool valid=matching(&j->request)&&live();if(valid)stage=control.candidate;unlock();if(!valid)goto done;
 /* Complete expensive readback and the acknowledged BLE stop first. Release
  * admission before asking the sole ADC worker, then reacquire and recheck the
  * original dataset. No stale power sample spans file IO or the BLE timeout. */
 if(!dsr_maintenance_acquire(&lease)){result=DSR_BUSY;goto done;}
 result=stage?dsr_maint_stage_barrier(stage,&lease):stop_bonds(&lease);dsr_maintenance_release(&lease);memset(&lease,0,sizeof(lease));if(result!=DSR_OK)goto done;
 dsr_ota_power_result_t power=DSR_OTA_POWER_BUSY;uint32_t sampled=clock_ms();result=dsr_ota_power_acquire(&power);
 if(result==DSR_OK)dsr_ota_power_release();else goto done;
 if(!dsr_maintenance_acquire(&lease)){result=DSR_BUSY;goto done;}
 lock();valid=matching(&j->request)&&!control.committed&&(uint32_t)(clock_ms()-sampled)<DSR_MAINT_POWER_MS&&
  dsr_maint_grant_confirm(&control.grant,j->request.nonce,lease.generation,clock_ms(),j->usb,power);
 if(valid)valid=dsr_maint_grant_take(&control.grant,&j->request,lease.generation,clock_ms());
 result=valid?DSR_OK:control.grant.result;if(!valid&&result==DSR_OK)result=DSR_BUSY;unlock();if(!valid)goto done;
 result=stage?dsr_maint_stage_commit_verified(stage,&lease):reset_commit(&lease);
 if(result==DSR_OK){
  lock();control.committed=true;control.revision++;unlock();
  result=dsr_restart_request_maintenance(lease.reservation,lease.generation)==DSR_RESTART_ACCEPTED?DSR_OK:DSR_BUSY;
 }
done:
 if(dsr_activity_maintenance_committed()){lock();control.committed=true;unlock();}
 dsr_maintenance_release(&lease);lock();control.job_busy=false;
 if(matching(&j->request)){control.grant.result=result;if(control.grant.state==DSR_GRANT_RUNNING)dsr_maint_grant_finish(&control.grant,result);control.revision++;}unlock();wipe(j,sizeof(*j));free(j);
}
dsr_result_t dsr_maintenance_confirm(const uint8_t id[16],bool usb)
{
 if(!id||!dsr_boot_ready())return DSR_INVALID;
 confirm_job_t *j=calloc(1,sizeof(*j));if(!j)return DSR_FULL;
 lock();dsr_result_t result=DSR_CONFLICT;
 if(!control.committed&&!control.job_busy&&live()&&control.grant.state==DSR_GRANT_WAIT&&!memcmp(id,control.grant.request.nonce,16)){
  if(control.grant.request.op==DSR_MAINT_EXPORT||control.grant.request.op==DSR_MAINT_IMPORT_UPLOAD){
   result=dsr_maint_grant_confirm(&control.grant,id,dsr_maintenance_generation(),clock_ms(),false,DSR_OTA_POWER_UNCHECKED)?DSR_OK:DSR_CONFLICT;
  }else{j->request=control.grant.request;j->usb=usb;control.grant.usb_required=false;control.job_busy=true;result=DSR_OK;}
  control.revision++;
 }
 bool submit=result==DSR_OK&&control.job_busy;unlock();
 if(submit){if(dsr_workspace_maintenance_submit(confirm_run,j))return DSR_OK;lock();control.job_busy=false;control.grant.result=DSR_BUSY;control.revision++;unlock();result=DSR_BUSY;}
 wipe(j,sizeof(*j));free(j);return result;
}
void dsr_maintenance_poll(void)
{
 dsr_maint_stage_t *stage=NULL;lock();(void)live();
 if(!control.job_busy&&!control.committed&&control.candidate&&control.grant.state==DSR_GRANT_DONE){stage=control.candidate;control.job_busy=true;}unlock();
 if(!stage)return;
 dsr_maint_lease_t lease={0};dsr_result_t result=DSR_BUSY;if(dsr_maintenance_acquire(&lease)){result=dsr_maint_stage_cancel(stage);dsr_maintenance_release(&lease);}
 lock();if(result==DSR_OK)control.candidate=NULL;control.job_busy=false;unlock();if(result==DSR_OK)dsr_maint_stage_free(stage);
}
