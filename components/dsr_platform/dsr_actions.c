#include "dsr_actions_private.h"
#include "dsr_workspace.h"
#include "dsr_activity.h"
#include "dsr_automation.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
typedef struct {dsr_action_key_t key;dsr_result_t gate;bool pending,fresh,output,online,seen;} endpoint_t;
typedef struct {
 dsr_actions_model_t actor,automatic;dsr_action_log_model_t log;
 endpoint_t endpoints[DSR_MAX_DEVICES];uint32_t boot_id,revision,run_serial;
 uint8_t endpoint_count;bool available,lease,logged[DSR_ACTION_STEPS],summary;
 uint32_t auto_rule,auto_cycle,auto_generation;
 bool auto_lease,auto_logged[DSR_ACTION_STEPS],auto_summary;
} actions_t;
_Static_assert(sizeof(actions_t)<=12288,"actor/cache/log bounded PSRAM context");
static actions_t *actions;
static SemaphoreHandle_t mutex;
static uint32_t now_ms(void){return (uint32_t)(esp_timer_get_time()/1000);}
static uint32_t utc(void){time_t t=time(NULL);return t>=1735689600&&(uint64_t)t<=UINT32_MAX?(uint32_t)t:0;}
static bool take(void){return mutex&&xSemaphoreTake(mutex,portMAX_DELAY)==pdTRUE;}
static void give(void){xSemaphoreGive(mutex);}
bool dsr_actions_native_reserve(uint32_t id)
{
 if(!id||!take())return false;
 dsr_actions_model_t *m=&actions->actor;
 if(m->active)for(unsigned i=m->current;i<m->count;i++)if(m->steps[i].key.layout_id==id&&m->steps[i].phase!=DSR_ACTION_DONE){give();return false;}
 bool found=false;for(unsigned i=0;i<actions->endpoint_count;i++)if(actions->endpoints[i].key.layout_id==id&&actions->endpoints[i].key.trv){actions->endpoints[i].pending=true;found=true;break;}
 give();return found;
}
void dsr_actions_native_release(uint32_t id)
{if(!take())return;for(unsigned i=0;i<actions->endpoint_count;i++)if(actions->endpoints[i].key.layout_id==id)actions->endpoints[i].pending=false;give();}
static dsr_action_key_t key(const dsr_connection_t *c)
{
 dsr_action_key_t k={.layout_id=c->layout_id,.generation=(uint8_t)c->device.generation,.channel=c->device.channel,.kind=c->device.endpoint_kind,.trv=c->device.endpoint_kind==DSR_ENDPOINT_BLU&&(c->telemetry.blu.errors&DSR_BLU_TRV)!=0};
 memcpy(k.device_id,c->device.device_id,64);memcpy(k.host,c->device.host,16);return k;
}
static void log_model(const dsr_actions_model_t *m,uint8_t command,uint8_t outcome,dsr_result_t result,uint32_t layout)
{
 dsr_action_log_entry_t entry={.utc=utc(),.monotonic_ms=now_ms(),.run_id=m->run_id,.object_id=m->object_id,.layout_id=layout,.source=m->source,.kind=m->kind,.command=command,.outcome=outcome,.result=(uint16_t)result};dsr_action_log_add(&actions->log,&entry);
}
static void log_entry(uint8_t command,uint8_t outcome,dsr_result_t result,uint32_t layout)
{log_model(&actions->actor,command,outcome,result,layout);}
static void finish_model(dsr_actions_model_t *m,bool logged[DSR_ACTION_STEPS],bool *summary,bool *lease)
{
 (void)dsr_actions_model_current(m,now_ms());
 for(unsigned i=0;i<m->count;i++)if(!logged[i]&&m->steps[i].phase==DSR_ACTION_DONE){dsr_action_step_t *s=&m->steps[i];log_model(m,(uint8_t)s->command,(uint8_t)s->outcome,s->result,s->key.layout_id);logged[i]=true;}
 if(m->run_id&&!m->active&&!*summary){
  dsr_action_outcome_t outcome=DSR_ACTION_SUCCESS;dsr_result_t result=DSR_OK;
  for(unsigned i=0;i<m->count;i++)if(m->steps[i].outcome!=DSR_ACTION_SUCCESS){if(outcome!=DSR_ACTION_UNCERTAIN){outcome=m->steps[i].outcome;result=m->steps[i].result;}if(m->steps[i].outcome==DSR_ACTION_UNCERTAIN){outcome=DSR_ACTION_UNCERTAIN;result=m->steps[i].result;}}
  log_model(m,0,(uint8_t)outcome,result,0);*summary=true;
 }
 if(*lease&&!m->active){*lease=false;dsr_activity_end();}
}
static void finish_locked(void)
{finish_model(&actions->actor,actions->logged,&actions->summary,&actions->lease);finish_model(&actions->automatic,actions->auto_logged,&actions->auto_summary,&actions->auto_lease);}
static dsr_actions_model_t *scheduled(void)
{return actions->actor.active?&actions->actor:&actions->automatic;}
static dsr_actions_model_t *owned_model(uint32_t id)
{
 dsr_actions_model_t *models[]={&actions->actor,&actions->automatic};
 for(unsigned i=0;i<2;i++){dsr_actions_model_t *m=models[i];if(id&&m->active&&m->current<m->count&&m->steps[m->current].request_id==id)return m;}
 return NULL;
}
static uint32_t manual_intent(const dsr_actions_model_t *m)
{uint32_t h=2166136261u;for(unsigned i=0;i<16;i++){h^=(uint8_t)m->nonce[i];h*=16777619u;}return h?h:1;}
esp_err_t dsr_actions_init(void)
{
 if(actions)return ESP_ERR_INVALID_STATE;
 actions_t *next=heap_caps_calloc(1,sizeof(*next),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!next)return ESP_ERR_NO_MEM;
 SemaphoreHandle_t lock=xSemaphoreCreateMutex();if(!lock){free(next);return ESP_ERR_NO_MEM;}
 do{next->boot_id=esp_random();}while(!next->boot_id);next->actor.boot_id=next->automatic.boot_id=next->boot_id;next->log.epoch=1;actions=next;mutex=lock;return ESP_OK;
}
void dsr_actions_snapshot(dsr_actions_snapshot_t *out)
{
 if(!out)return;
 memset(out,0,sizeof(*out));if(!take())return;
 /* Status is read-only. Exact terminal completion/lease release belongs to
  * the sole UI consumer, never HTTP observing an in-flight timeout. */
 dsr_actions_model_t *m=&actions->actor;out->boot_id=actions->boot_id;out->admission_generation=m->generation;out->revision=actions->revision;out->run_id=m->run_id;out->object_id=m->object_id;out->source=m->source;out->kind=m->kind;out->count=m->count;out->current=m->current;out->active=m->active;out->cancelled=m->cancel;out->available=actions->available&&!m->active;
 memcpy(out->nonce,m->nonce,sizeof(out->nonce));
 for(unsigned i=0;i<m->count;i++)out->steps[i]=(dsr_action_result_t){.layout_id=m->steps[i].key.layout_id,.command=(uint8_t)m->steps[i].command,.outcome=(uint8_t)m->steps[i].outcome,.result=m->steps[i].result};
 give();
}
dsr_result_t dsr_actions_request(const char *json,uint8_t kind,uint8_t source,uint32_t *run_id)
{
 if(!run_id||source>1)return DSR_INVALID;
 dsr_actions_request_t request;if(!dsr_actions_request_parse(json,kind,&request))return DSR_INVALID;
 /* Duplicate is resolved before config parsing: a removed/edited scene or
  * changed revision must not turn a lost ACK into a second physical run. */
 if(!take())return DSR_IO;
 if(dsr_actions_model_duplicate(&actions->actor,request.boot_id,request.nonce)){*run_id=actions->actor.run_id;give();return DSR_OK;}give();
 char *configuration=dsr_workspace_snapshot();if(!configuration)return DSR_IO;
 dsr_actions_plan_t plan;dsr_result_t result=dsr_actions_plan_build(configuration,&request,&plan);free(configuration);if(result!=DSR_OK)return result;
 if(!take())return DSR_IO;
 dsr_actions_model_t *m=&actions->actor;
 if(dsr_actions_model_duplicate(m,request.boot_id,request.nonce)){*run_id=m->run_id;give();return DSR_OK;}
 if(request.boot_id!=actions->boot_id||request.admission_generation!=m->generation||request.revision!=actions->revision){give();return DSR_CONFLICT;}
 if(!actions->available||m->active||!dsr_activity_begin()){give();return DSR_BUSY;}
 for(unsigned i=0;i<plan.count;i++)for(unsigned j=0;j<actions->endpoint_count;j++)if(actions->endpoints[j].key.layout_id==plan.layout_ids[i]&&actions->endpoints[j].pending){dsr_activity_end();give();return DSR_BUSY;}
 /* Temporary heap, bounded independently of the worker task frame. */
 dsr_action_step_t *steps=calloc(DSR_ACTION_STEPS,sizeof(*steps));if(!steps){dsr_activity_end();give();return DSR_IO;}
 for(unsigned i=0;i<plan.count;i++){
  steps[i].key.layout_id=plan.layout_ids[i];steps[i].command=(dsr_action_command_t)plan.commands[i];memcpy(steps[i].value,plan.values[i],3);
  for(unsigned j=0;j<actions->endpoint_count;j++)if(actions->endpoints[j].key.layout_id==plan.layout_ids[i]){steps[i].key=actions->endpoints[j].key;break;}
 }
 if(!kind){int family=-1;for(unsigned i=0;i<plan.count;i++)if(steps[i].key.device_id[0]){
  unsigned k=steps[i].key.kind;
  if(k==DSR_ENDPOINT_HT||(k==DSR_ENDPOINT_BLU&&!steps[i].key.trv)||(family>=0&&family!=(int)k)){free(steps);dsr_activity_end();give();return DSR_UNSUPPORTED;}family=(int)k;
 }}
 uint32_t next=actions->run_serial+1u;if(!next)next=1;
 bool accepted=dsr_actions_model_begin(m,steps,plan.count,next,request.admission_generation,request.boot_id,request.object_id,request.revision,request.nonce,source,kind,now_ms());free(steps);
 if(accepted){
  for(unsigned i=0;i<m->count;i++)if(m->steps[i].command==DSR_ACTION_TARGET){
   (void)dsr_automation_manual_ready(m->steps[i].key.layout_id,manual_intent(m));
   if(actions->automatic.active)for(unsigned j=0;j<actions->automatic.count;j++)
    if(actions->automatic.steps[j].key.layout_id==m->steps[i].key.layout_id)dsr_actions_model_cancel(&actions->automatic);
  }
  actions->run_serial=next;actions->lease=true;memset(actions->logged,0,sizeof(actions->logged));actions->summary=false;*run_id=m->run_id;log_entry(0,DSR_ACTION_WAITING,DSR_OK,0);
 }else dsr_activity_end();give();return accepted?DSR_OK:DSR_CONFLICT;
}
dsr_result_t dsr_actions_auto_request(uint32_t rule,uint32_t cycle,uint32_t generation,const uint32_t *ids,const uint8_t *halves,size_t count,uint32_t *run)
{
 if(!rule||!cycle||!generation||!ids||!halves||!count||count>DSR_ACTION_STEPS||!run)return DSR_INVALID;
 for(size_t i=0;i<count;i++)if(!dsr_automation_auto_allowed(rule,cycle,generation,ids[i],halves[i]))return DSR_CONFLICT;
 if(!take())return DSR_IO;
 finish_locked();if(!actions->available||actions->actor.active||actions->automatic.active||!dsr_activity_begin()){give();return DSR_BUSY;}
 dsr_action_step_t *steps=calloc(count,sizeof(*steps));if(!steps){dsr_activity_end();give();return DSR_IO;}
 bool valid=true;
 for(size_t i=0;i<count;i++){
  steps[i].command=DSR_ACTION_TARGET;steps[i].value[0]=halves[i];
  for(unsigned j=0;j<actions->endpoint_count;j++)if(actions->endpoints[j].key.layout_id==ids[i]&&!actions->endpoints[j].pending){steps[i].key=actions->endpoints[j].key;break;}
  if(!steps[i].key.layout_id||!steps[i].key.trv||steps[i].key.kind!=DSR_ENDPOINT_BLU)valid=false;
 }
 dsr_actions_model_t *m=&actions->automatic;uint32_t next=actions->run_serial+1u;if(!next)next=1;char nonce[17];snprintf(nonce,sizeof(nonce),"%08x%08x",(unsigned)generation,(unsigned)next);
 bool accepted=valid&&dsr_actions_model_begin(m,steps,count,next,m->generation,actions->boot_id,rule,actions->revision,nonce,3,4,now_ms());free(steps);
 if(accepted){actions->auto_rule=rule;actions->auto_cycle=cycle;actions->auto_generation=generation;actions->run_serial=next;
  actions->auto_lease=true;actions->auto_summary=false;memset(actions->auto_logged,0,sizeof(actions->auto_logged));*run=m->run_id;log_model(m,0,DSR_ACTION_WAITING,DSR_OK,0);
 }else dsr_activity_end();give();return accepted?DSR_OK:DSR_CONFLICT;
}
bool dsr_actions_auto_result(uint32_t run,dsr_result_t *result)
{
 if(!run||!result||!take())return false;
 finish_locked();dsr_actions_model_t *m=&actions->automatic;bool done=m->run_id==run&&!m->active;
 if(done){*result=DSR_OK;for(unsigned i=0;i<m->count;i++)if(m->steps[i].outcome!=DSR_ACTION_SUCCESS){*result=m->steps[i].result==DSR_OK?DSR_BUSY:m->steps[i].result;break;}}
 give();return done;
}
void dsr_actions_auto_cancel(uint32_t run)
{if(!take())return;if(actions->automatic.run_id==run)dsr_actions_model_cancel(&actions->automatic);finish_locked();give();}
bool dsr_actions_auto_attempted(uint32_t run)
{
 if(!run||!take())return false;
 bool attempted=false;if(actions->automatic.run_id==run)for(unsigned i=0;i<actions->automatic.count;i++)if(actions->automatic.steps[i].write_attempted)attempted=true;
 give();return attempted;
}
bool dsr_actions_cancel(uint32_t boot_id,uint32_t run_id)
{if(!take())return false;bool ok=actions->boot_id==boot_id&&actions->actor.run_id==run_id&&run_id;if(ok)dsr_actions_model_cancel(&actions->actor);give();return ok;}
bool dsr_actions_active(void){if(!take())return false;bool active=actions->actor.active||actions->automatic.active;give();return active;}
bool dsr_actions_reserved(const dsr_device_t *d)
{
 if(!d||!take())return false;
 bool reserved=false;dsr_actions_model_t *m=&actions->actor;
 if(m->active)for(unsigned i=m->current;i<m->count;i++){const dsr_action_key_t *k=&m->steps[i].key;if(m->steps[i].phase!=DSR_ACTION_DONE&&k->kind==d->endpoint_kind&&k->generation==d->generation&&k->channel==d->channel&&!strcmp(k->device_id,d->device_id)){reserved=true;break;}}
 give();return reserved;
}
bool dsr_actions_publish_begin(uint32_t revision,bool available)
{if(!take())return false;actions->revision=revision;actions->available=available&&revision!=0;actions->endpoint_count=0;return true;}
void dsr_actions_publish_at(const dsr_connection_t *c,bool enabled,bool pending,dsr_result_t gate,bool fresh)
{
 if(!c||!enabled||!c->layout_id||actions->endpoint_count>=DSR_MAX_DEVICES)return;
 unsigned at=actions->endpoint_count++;endpoint_t *e=&actions->endpoints[at];dsr_action_key_t actual=key(c);
 bool output_changed=c->device.endpoint_kind==DSR_ENDPOINT_SWITCH&&e->output!=c->device.output;
 if(e->seen&&dsr_action_key_equal(&e->key,&actual)&&(e->gate!=gate||output_changed||e->online!=c->device.online)){
  /* Observations are not command acknowledgements. Availability/error-only
   * changes carry command0, and a Cover can never acquire Switch ON/OFF. */
  dsr_result_t observed=gate==DSR_OK&&!c->device.online?DSR_OFFLINE:gate;
  dsr_action_log_entry_t entry={.utc=utc(),.monotonic_ms=now_ms(),.layout_id=c->layout_id,.source=2,.kind=2,.command=output_changed?(c->device.output?DSR_ACTION_ON:DSR_ACTION_OFF):0,.outcome=observed==DSR_OK?DSR_ACTION_SUCCESS:DSR_ACTION_FAILED,.result=(uint16_t)observed};dsr_action_log_add(&actions->log,&entry);
 }
 *e=(endpoint_t){.key=actual,.gate=gate,.pending=pending,.fresh=fresh,.output=c->device.output,.online=c->device.online,.seen=true};
}
void dsr_actions_publish_end(void){give();}
uint32_t dsr_actions_target(void)
{if(!take())return 0;finish_locked();dsr_actions_model_t *m=scheduled();int at=dsr_actions_model_current(m,now_ms());uint32_t id=at<0?0:m->steps[at].key.layout_id;give();return id;}
dsr_action_command_t dsr_actions_command(void)
{if(!take())return 0;dsr_actions_model_t *m=scheduled();dsr_action_command_t command=m->active&&m->current<m->count?m->steps[m->current].command:0;give();return command;}
bool dsr_actions_job_owned(uint32_t id)
{if(!id||!take())return false;bool owned=owned_model(id)!=NULL;give();return owned;}
bool dsr_actions_auto_job_owned(uint32_t id)
{if(!id||!take())return false;bool owned=owned_model(id)==&actions->automatic;give();return owned;}
bool dsr_actions_job_attempted(uint32_t id)
{if(!id||!take())return false;dsr_actions_model_t *m=owned_model(id);bool attempted=m&&m->steps[m->current].write_attempted;give();return attempted;}
bool dsr_actions_prepare(const dsr_connection_t *c,dsr_result_t gate,bool fresh,bool capable,uint32_t id,dsr_job_kind_t *kind,bool *on)
{
 if(!c||c->device.endpoint_kind>DSR_ENDPOINT_COVER||!kind||!on||!take())return false;
 finish_locked();int at=dsr_actions_model_current(&actions->actor,now_ms());bool ok=false;
 if(at>=0){dsr_action_key_t actual=key(c);ok=dsr_actions_model_check(&actions->actor,(unsigned)at,&actual,gate,fresh,capable,now_ms());
  if(ok){dsr_action_step_t *s=&actions->actor.steps[at];*kind=s->phase==DSR_ACTION_PROBE_READY?DSR_JOB_PROBE:c->device.endpoint_kind==DSR_ENDPOINT_COVER?DSR_JOB_COVER:DSR_JOB_SET_OUTPUT;*on=s->command==DSR_ACTION_ON;ok=dsr_actions_model_queued(&actions->actor,(unsigned)at,id,now_ms());}
 }finish_locked();give();return ok;
}
bool dsr_actions_prepare_job(dsr_job_t *job,dsr_result_t gate,bool fresh)
{
 if(!job||!job->id||!take())return false;
 finish_locked();dsr_actions_model_t *m=scheduled();int at=dsr_actions_model_current(m,now_ms());bool ok=false;
 if(at>=0){
  dsr_action_step_t *s=&m->steps[at];const dsr_connection_t *c=&job->connection;unsigned command=s->command;bool capable=false;
  if(command==DSR_ACTION_TARGET&&m==&actions->actor&&!dsr_automation_manual_ready(s->key.layout_id,manual_intent(m))){
   dsr_result_t ready=dsr_automation_manual_result(s->key.layout_id,manual_intent(m));
   if(ready==DSR_BUSY){give();return false;}gate=ready;
  }
  if(m==&actions->automatic&&!dsr_automation_auto_allowed(actions->auto_rule,actions->auto_cycle,actions->auto_generation,s->key.layout_id,s->value[0])){
   dsr_actions_model_cancel(m);finish_locked();give();return false;
  }
  if(c->device.endpoint_kind==DSR_ENDPOINT_SWITCH){job->kind=DSR_JOB_SET_OUTPUT;job->on=command==DSR_ACTION_ON;capable=command<=DSR_ACTION_OFF&&c->device.has_switch;}
  else if(c->device.endpoint_kind==DSR_ENDPOINT_COVER){job->kind=DSR_JOB_COVER;job->cover=(dsr_cover_intent_t){.command=command==DSR_ACTION_OPEN?DSR_COVER_OPEN:command==DSR_ACTION_CLOSE?DSR_COVER_CLOSE:command==DSR_ACTION_STOP?DSR_COVER_STOP:command==DSR_ACTION_POSITION?DSR_COVER_GOTO:DSR_COVER_NONE,.position=s->value[0]};capable=command==DSR_ACTION_STOP||dsr_cover_can_move(&c->telemetry.cover,job->cover);}
  else if(c->device.endpoint_kind==DSR_ENDPOINT_LIGHT){job->kind=DSR_JOB_LIGHT;job->light=(dsr_light_intent_t){.command=command==DSR_ACTION_ON?DSR_LIGHT_ON:command==DSR_ACTION_OFF?DSR_LIGHT_OFF:command==DSR_ACTION_BRIGHTNESS?DSR_LIGHT_SET_BRIGHTNESS:DSR_LIGHT_NONE,.brightness=s->value[0]};capable=dsr_light_can_set(&c->telemetry.light,job->light);}
  else if(c->device.endpoint_kind==DSR_ENDPOINT_RGB){job->kind=DSR_JOB_RGB;job->rgb=(dsr_rgb_intent_t){.command=command==DSR_ACTION_ON?DSR_RGB_ON:command==DSR_ACTION_OFF?DSR_RGB_OFF:command==DSR_ACTION_BRIGHTNESS?DSR_RGB_SET_BRIGHTNESS:command==DSR_ACTION_RGB?DSR_RGB_SET_COLOR:DSR_RGB_NONE};if(command==DSR_ACTION_RGB)memcpy(job->rgb.rgb,s->value,3);else job->rgb.brightness=s->value[0];capable=dsr_rgb_can_set(&c->telemetry.rgb,job->rgb);}
  else if(c->device.endpoint_kind==DSR_ENDPOINT_BLU){job->kind=DSR_JOB_TRV;job->trv=(dsr_trv_intent_t){.command=command==DSR_ACTION_TARGET?DSR_TRV_SET_TARGET:0,.half_c=s->value[0]};capable=(c->telemetry.blu.errors&DSR_BLU_TRV)&&dsr_trv_intent_valid(job->trv);}
  dsr_action_key_t actual=key(c);ok=dsr_actions_model_check(m,(unsigned)at,&actual,gate,fresh,capable,now_ms());
  if(ok){if(s->phase==DSR_ACTION_PROBE_READY)job->kind=DSR_JOB_PROBE;ok=dsr_actions_model_queued(m,(unsigned)at,job->id,now_ms());}
 }
 finish_locked();give();return ok;
}
void dsr_actions_unqueue(uint32_t id){if(!take())return;dsr_actions_model_t *m=owned_model(id);if(m)dsr_actions_model_unqueue(m,id);finish_locked();give();}
void dsr_actions_missing(uint32_t id)
{if(!take())return;dsr_actions_model_t *m=scheduled();int at=dsr_actions_model_current(m,now_ms());if(at>=0&&m->steps[at].key.layout_id==id){dsr_action_key_t missing={.layout_id=id};(void)dsr_actions_model_check(m,(unsigned)at,&missing,DSR_IDENTITY_MISMATCH,false,false,now_ms());}finish_locked();give();}
void dsr_actions_complete(uint32_t id,dsr_result_t result){if(!take())return;dsr_actions_model_t *m=owned_model(id);if(m)(void)dsr_actions_model_complete(m,id,result,now_ms());finish_locked();give();}
void dsr_actions_mode_block(void){if(!take())return;actions->available=false;dsr_actions_model_cancel(&actions->actor);dsr_actions_model_cancel(&actions->automatic);finish_locked();give();}
dsr_result_t dsr_actions_worker_allowed(uint32_t id,const dsr_connection_t *c)
{
 if(!c||!take())return DSR_OK;
 dsr_actions_model_t *m=owned_model(id);dsr_result_t result=DSR_OK;
 if(m){
  dsr_action_step_t *s=&m->steps[m->current];
  if(m==&actions->automatic&&(!dsr_automation_auto_allowed(actions->auto_rule,actions->auto_cycle,actions->auto_generation,s->key.layout_id,s->value[0])||
   !dsr_automation_binding_matches(s->key.layout_id,c)))result=DSR_CONFLICT;
  else if(m==&actions->actor&&s->command==DSR_ACTION_TARGET&&!dsr_automation_manual_ready(s->key.layout_id,manual_intent(m)))result=dsr_automation_manual_result(s->key.layout_id,manual_intent(m));
  if(result!=DSR_OK)s->rejected=result;
  else {dsr_action_key_t actual=key(c);if(!dsr_actions_model_worker_allowed(m,id,&actual,now_ms()))result=s->rejected==DSR_OK?DSR_BUSY:s->rejected;}
 }give();return result;
}
void dsr_actions_worker_write_attempted(uint32_t id)
{if(!take())return;dsr_actions_model_t *m=owned_model(id);if(m)(void)dsr_actions_model_write_attempted(m,id);give();}
bool dsr_actions_log(uint32_t epoch,unsigned offset,unsigned limit,dsr_action_log_page_t *page)
{if(!take())return false;bool ok=dsr_action_log_page(&actions->log,epoch,offset,limit,page);give();return ok;}
void dsr_actions_log_clear(void){if(take()){dsr_action_log_clear(&actions->log);give();}}
void dsr_actions_observe(uint32_t id,dsr_action_command_t command,dsr_result_t result)
{
 if(!id||id>1000000||command<DSR_ACTION_ON||command>DSR_ACTION_STOP||!take())return;
 dsr_action_outcome_t outcome=result==DSR_OK?DSR_ACTION_SUCCESS:result==DSR_AUTH_REQUIRED||result==DSR_IDENTITY_MISMATCH||result==DSR_RATE_LIMITED||result==DSR_UNSUPPORTED||result==DSR_INVALID||result==DSR_CONFLICT?DSR_ACTION_FAILED:DSR_ACTION_UNCERTAIN;
 dsr_action_log_entry_t entry={.utc=utc(),.monotonic_ms=now_ms(),.layout_id=id,.source=2,.kind=3,.command=(uint8_t)command,.outcome=(uint8_t)outcome,.result=(uint16_t)result};dsr_action_log_add(&actions->log,&entry);give();
}
