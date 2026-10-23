#include "dsr_automation.h"
#include "dsr_automation_model.h"
#include "dsr_automation_config.h"
#include "dsr_actions.h"
#include "dsr_workspace.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
 dsr_auto_model_t model;
 dsr_auto_rules_t public_rules,staged;
 dsr_auto_record_t journal;
 dsr_auto_effect_t effect;
 uint32_t approvals[DSR_AUTO_RULES],run_id,read_job,read_started,revision;
 struct {uint32_t id,until;uint8_t failures;} rate[DSR_AUTO_PARTICIPANTS];
 dsr_result_t config_error;
 bool journal_submitted,online,configured,config_invalid;
} automation_t;
_Static_assert(sizeof(automation_t)<=12288,"bounded automation PSRAM context");
static automation_t *ctx;
static SemaphoreHandle_t mutex;
static bool take(void){return ctx&&mutex&&xSemaphoreTake(mutex,portMAX_DELAY)==pdTRUE;}
static void give(void){xSemaphoreGive(mutex);}
static uint32_t wall(void){time_t t=time(NULL);return t>=1735689600&&(uint64_t)t<=UINT32_MAX?(uint32_t)t:0;}
static uint32_t clock_ms(void){return (uint32_t)(esp_timer_get_time()/1000);}
static const dsr_auto_rule_t *find_rule(const dsr_auto_rules_t *r,uint32_t id)
{for(unsigned i=0;i<r->count;i++)if(r->items[i].id==id)return &r->items[i];return NULL;}
static const dsr_fleet_slot_t *find_slot(const dsr_fleet_t *f,uint32_t id)
{for(unsigned i=0;i<f->count;i++)if(f->slots[i].connection.layout_id==id)return &f->slots[i];return NULL;}
static int find_key(const dsr_auto_rules_t *r,uint32_t id)
{for(unsigned i=0;i<r->key_count;i++)if(r->keys[i].layout_id==id)return (int)i;return -1;}
static bool has_approval(uint32_t id)
{for(unsigned i=0;i<DSR_AUTO_RULES;i++)if(ctx->approvals[i]==id)return true;return false;}
static bool rate_ready(uint32_t id,uint32_t now)
{for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++)if(ctx->rate[i].id==id)return (int32_t)(now-ctx->rate[i].until)>=0;return true;}
static void rate_result(uint32_t id,dsr_result_t result,uint32_t now)
{
 int at=-1;
 for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++)if(ctx->rate[i].id==id){at=(int)i;break;}
 if(result==DSR_OK){if(at>=0)memset(&ctx->rate[at],0,sizeof(ctx->rate[at]));return;}
 if(result!=DSR_RATE_LIMITED)return;
 if(at<0)for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++)if(!ctx->rate[i].id){at=(int)i;break;}
 if(at<0)for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++)if(find_key(&ctx->public_rules,ctx->rate[i].id)<0){at=(int)i;memset(&ctx->rate[i],0,sizeof(ctx->rate[i]));break;}
 if(at<0)return; /* one slot per bounded participant; never evict an applicable cooldown */
 ctx->rate[at].id=id;if(ctx->rate[at].failures<3)ctx->rate[at].failures++;
 ctx->rate[at].until=now+(DSR_FLEET_RATE_LIMIT_MS<<(ctx->rate[at].failures-1));
}
static dsr_auto_key_t actual_key(const dsr_connection_t *c,uint8_t role)
{
 dsr_auto_key_t k={.layout_id=c->layout_id,.generation=(uint8_t)c->device.generation,.channel=c->device.channel,.kind=role};
 memcpy(k.host,c->device.host,sizeof(k.host));memcpy(k.device_id,c->device.device_id,sizeof(k.device_id));return k;
}
static bool capability(const dsr_connection_t *c,uint8_t role)
{
 if(c->legacy_fallback||c->device.generation!=DSR_GEN2_PLUS||c->device.endpoint_kind!=DSR_ENDPOINT_BLU||
   !(c->telemetry.blu.flags&DSR_BLU_VALID))return false;
 return role==DSR_AUTO_TRV?(c->telemetry.blu.errors&DSR_BLU_TRV)!=0:
   (c->telemetry.blu.flags&DSR_BLU_HAS_OPEN)&&!(c->telemetry.blu.errors&DSR_BLU_TRV);
}
static bool public_allows(const dsr_auto_effect_t *e)
{
 const dsr_auto_rule_t *wanted=find_rule(&ctx->public_rules,e->rule_id),*enrolled=find_rule(&ctx->model.record.rules,e->rule_id);
 return wanted&&wanted->enabled&&dsr_auto_rule_same(&ctx->public_rules,wanted,&ctx->model.record.rules,enrolled);
}
static bool uncertain_target(uint32_t id)
{
 if(ctx->model.fault!=DSR_IO)return false;
 int at=find_key(&ctx->public_rules,id);if(at<0)return false;
 for(unsigned i=0;i<ctx->public_rules.count;i++)if(ctx->public_rules.items[i].enabled&&
   (ctx->public_rules.items[i].targets&(1u<<(unsigned)at)))return true;
 return false;
}
esp_err_t dsr_automation_init(void)
{
 if(ctx)return ESP_ERR_INVALID_STATE;
 automation_t *next=heap_caps_calloc(1,sizeof(*next),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 if(!next)return ESP_ERR_NO_MEM;
 SemaphoreHandle_t lock=xSemaphoreCreateMutex();if(!lock){free(next);return ESP_ERR_NO_MEM;}
 dsr_auto_record_t *boot=malloc(sizeof(*boot));if(!boot){vSemaphoreDelete(lock);free(next);return ESP_ERR_NO_MEM;}
 bool found=false;esp_err_t loaded=ESP_FAIL;
 if(!dsr_workspace_auto_boot(boot,&found,&loaded))loaded=ESP_FAIL;
 dsr_auto_init(&next->model);
 if(loaded!=ESP_OK)next->model.fault=DSR_IO;
 else if(found&&!dsr_auto_recover(&next->model,boot))next->model.fault=DSR_IO;
 free(boot);char *json=dsr_workspace_snapshot();cJSON *root=json?cJSON_Parse(json):NULL;free(json);char error[96];
 if(!dsr_auto_rules_parse(root,&next->public_rules,error)){cJSON_Delete(root);vSemaphoreDelete(lock);free(next);return ESP_FAIL;}
 const cJSON *rev=cJSON_GetObjectItemCaseSensitive(root,"revision");next->revision=cJSON_IsNumber(rev)?(uint32_t)rev->valueint:0;
 cJSON_Delete(root);ctx=next;mutex=lock;dsr_auto_pause(&ctx->model,clock_ms(),wall());return ESP_OK;
}
bool dsr_automation_config_allowed(const cJSON *candidate,char error[96])
{
 dsr_auto_rules_t *rules=malloc(sizeof(*rules));if(!rules){snprintf(error,96,"Not enough memory");return false;}
 if(!dsr_auto_rules_parse(candidate,rules,error)){free(rules);return false;}
 if(!take()){free(rules);error[0]=0;return true;}
 bool allowed=true;
 for(unsigned i=0;i<ctx->model.record.rules.count;i++)if(ctx->model.record.cycles[i].active){
  const dsr_auto_rule_t *old=&ctx->model.record.rules.items[i],*next=find_rule(rules,old->id);
  if(next&&next->enabled&&!dsr_auto_rule_same(&ctx->model.record.rules,old,rules,next)){allowed=false;break;}
 }
 give();free(rules);if(!allowed)snprintf(error,96,"Disable the active automation before changing its contacts, targets or temperature");else error[0]=0;
 return allowed;
}
void dsr_automation_config_published(const cJSON *root)
{
 if(!take())return;
 dsr_auto_rules_t *rules=&ctx->staged;char error[96];
 if(!dsr_auto_rules_parse(root,rules,error)){ctx->config_invalid=true;ctx->config_error=DSR_IO;give();return;}
 uint32_t approved[DSR_AUTO_RULES]={0};
 for(unsigned i=0;i<rules->count;i++){
  const dsr_auto_rule_t *n=&rules->items[i],*old=find_rule(&ctx->public_rules,n->id);
  if(n->enabled&&(!old||!old->enabled||!dsr_auto_rule_same(rules,n,&ctx->public_rules,old)||has_approval(n->id)))approved[i]=n->id;
 }
 const cJSON *rev=cJSON_GetObjectItemCaseSensitive(root,"revision");ctx->revision=cJSON_IsNumber(rev)?(uint32_t)rev->valueint:0;
 memcpy(ctx->approvals,approved,sizeof(approved));ctx->public_rules=*rules;ctx->configured=false;ctx->config_invalid=false;give();
}
static bool enrollment_same(const dsr_auto_rules_t *next,const dsr_auto_rule_t *r)
{
 const dsr_auto_rules_t *old=&ctx->model.record.rules;const dsr_auto_rule_t *before=find_rule(old,r->id);
 if(!before||!before->enabled||!dsr_auto_rule_same(next,r,old,before))return false;
 uint16_t wanted=(uint16_t)(r->sensors|r->targets),previous=(uint16_t)(before->sensors|before->targets);
 for(unsigned i=0;i<next->key_count;i++)if(wanted&(1u<<i)){
  int at=find_key(old,next->keys[i].layout_id);
  if(at<0||!(previous&(1u<<(unsigned)at))||!dsr_auto_key_equal(&next->keys[i],&old->keys[at]))return false;
 }
 return true;
}
static void configure(const dsr_fleet_t *fleet)
{
 if(ctx->configured||ctx->model.pending.kind||ctx->model.fault==DSR_IO)return;
 dsr_auto_rules_t *rules=&ctx->staged;*rules=ctx->public_rules;uint16_t enabled=0;
 for(unsigned i=0;i<rules->count;i++)if(rules->items[i].enabled)enabled|=(uint16_t)(rules->items[i].sensors|rules->items[i].targets);
 for(unsigned i=0;i<rules->key_count;i++){
  const dsr_fleet_slot_t *s=find_slot(fleet,rules->keys[i].layout_id);
  if(s&&s->layout_enabled&&capability(&s->connection,rules->keys[i].kind))rules->keys[i]=actual_key(&s->connection,rules->keys[i].kind);
  else if(enabled&(1u<<i)){ctx->config_error=DSR_OFFLINE;return;}
 }
 for(unsigned i=0;i<rules->count;i++)if(rules->items[i].enabled&&!enrollment_same(rules,&rules->items[i])){
  if(!has_approval(rules->items[i].id)){ctx->config_error=DSR_CONFLICT;return;}rules->enroll=true;
 }
 dsr_result_t result=dsr_auto_configure(&ctx->model,rules);ctx->config_error=result;
 if(result==DSR_OK){ctx->configured=true;memset(ctx->approvals,0,sizeof(ctx->approvals));}
}
bool dsr_automation_manual_ready(uint32_t id,uint32_t intent)
{
 if(!intent||!take())return false;
 bool unknown=uncertain_target(id)&&find_key(&ctx->model.record.rules,id)<0;
 bool ready=!unknown&&dsr_auto_manual(&ctx->model,id,intent);give();return ready;
}
dsr_result_t dsr_automation_manual_result(uint32_t id,uint32_t intent)
{
 if(!take())return DSR_IO;
 bool unknown=uncertain_target(id)&&find_key(&ctx->model.record.rules,id)<0;
 bool ready=!unknown&&dsr_auto_manual(&ctx->model,id,intent);dsr_result_t result=ready?DSR_OK:ctx->model.fault==DSR_IO?DSR_IO:DSR_BUSY;
 give();return result;
}
bool dsr_automation_auto_allowed(uint32_t rule,uint32_t cycle,uint32_t generation,uint32_t id,uint8_t half)
{
 if(!take())return false;
 const dsr_auto_effect_t *e=&ctx->effect;
 bool allowed=ctx->online&&!ctx->config_invalid&&e->rule_id==rule&&e->cycle==cycle&&e->generation==generation&&e->layout_id==id&&
  e->target_half==half&&public_allows(e)&&dsr_auto_effect_allowed(&ctx->model,e,clock_ms(),wall());give();return allowed;
}
bool dsr_automation_binding_matches(uint32_t id,const dsr_connection_t *c)
{
 if(!c||!take())return false;
 int i=find_key(&ctx->model.record.rules,id);bool matches=false;
 if(i>=0){const dsr_auto_key_t *k=&ctx->model.record.rules.keys[i];dsr_auto_key_t actual=actual_key(c,k->kind);matches=dsr_auto_key_equal(k,&actual);}
 give();return matches;
}
bool dsr_automation_job_owned(uint32_t id)
{if(!id||!take())return false;bool owned=ctx->read_job==id;give();return owned;}
dsr_result_t dsr_automation_worker_allowed(uint32_t id,const dsr_connection_t *c)
{
 if(!id||!take())return DSR_OK;
 dsr_result_t result=DSR_OK;
 if(ctx->read_job==id){
  int i=find_key(&ctx->model.record.rules,ctx->effect.layout_id);
  if(i<0||!c||!ctx->online||!dsr_auto_effect_allowed(&ctx->model,&ctx->effect,clock_ms(),wall()))result=DSR_CONFLICT;
  else {dsr_auto_key_t k=actual_key(c,DSR_AUTO_TRV);if(!dsr_auto_key_equal(&k,&ctx->model.record.rules.keys[i]))result=DSR_IDENTITY_MISMATCH;}
 }
 give();return result;
}
bool dsr_automation_prepare_read(const dsr_fleet_t *fleet,uint32_t job,uint32_t now,dsr_job_t *out)
{
 if(!fleet||!job||!out||!take())return false;
 bool ready=ctx->online&&ctx->effect.kind==DSR_AUTO_EFFECT_READ_TRV&&!ctx->read_job;
 const dsr_fleet_slot_t *s=ready?find_slot(fleet,ctx->effect.layout_id):NULL;
 if(ready&&(!s||!s->layout_enabled||s->queued||s->mailbox_pending||s->in_flight||s->blocked||
  !rate_ready(ctx->effect.layout_id,now)||(s->result==DSR_RATE_LIMITED&&(int32_t)(now-s->next_poll)<0))){
  dsr_result_t reason=s&&s->blocked&&s->result!=DSR_OK?s->result:!s||!s->layout_enabled?DSR_OFFLINE:DSR_BUSY;
  dsr_auto_defer_read(&ctx->model,&ctx->effect,now,reason);ctx->effect=(dsr_auto_effect_t){0};ready=false;
 }
 if(ready){*out=(dsr_job_t){.id=job,.kind=DSR_JOB_TRV_READ,.connection=s->connection};ctx->read_job=job;ctx->read_started=now;}
 give();return ready;
}
void dsr_automation_read_rejected(uint32_t id,dsr_result_t result)
{
 if(!take())return;
 if(ctx->read_job==id){ctx->read_job=0;if(result!=DSR_BUSY){dsr_auto_complete(&ctx->model,&ctx->effect,result);ctx->effect=(dsr_auto_effect_t){0};}}
 give();
}
void dsr_automation_complete_read(const dsr_event_t *event,uint32_t now)
{
 if(!event||!take())return;
 if(ctx->read_job!=event->id){give();return;}
 ctx->model.now_ms=now;int i=find_key(&ctx->model.record.rules,ctx->effect.layout_id);dsr_result_t result=event->result;
 if(i>=0&&result==DSR_OK){
  dsr_auto_key_t key={.layout_id=ctx->effect.layout_id,.generation=(uint8_t)event->device.generation,.channel=event->device.channel,.kind=DSR_AUTO_TRV};
  memcpy(key.host,event->device.host,16);memcpy(key.device_id,event->device.device_id,64);
  if(!dsr_auto_key_equal(&key,&ctx->model.record.rules.keys[i]))result=DSR_IDENTITY_MISMATCH;
  else {dsr_auto_observation_t o={.key=key,.sample_ms=now,.read_id=ctx->effect.id,.started_ms=ctx->read_started,
   .target_c=event->telemetry.trv_target_c,.valid=(event->telemetry.blu.errors&DSR_BLU_TRV_TARGET)!=0};dsr_auto_observe(&ctx->model,&o);}
 }
 rate_result(ctx->effect.layout_id,result,now);
 dsr_auto_complete(&ctx->model,&ctx->effect,result);ctx->read_job=0;ctx->effect=(dsr_auto_effect_t){0};give();
}
void dsr_automation_tick(dsr_fleet_t *fleet,uint32_t now,uint32_t utc,bool online)
{
 if(!fleet||!ctx)return;
 uint32_t ack=0;esp_err_t saved=ESP_OK;bool got_ack=dsr_workspace_auto_take_result(&ack,&saved);
 uint32_t run=ctx->run_id;dsr_result_t outcome=DSR_BUSY;bool done=run&&dsr_actions_auto_result(run,&outcome);
 bool attempted=done&&dsr_actions_auto_attempted(run);
 if(!take())return;
 ctx->model.now_ms=now;
 for(unsigned i=0;i<fleet->count;i++)for(unsigned j=0;j<DSR_AUTO_PARTICIPANTS;j++)
  if(ctx->rate[j].id==fleet->slots[i].connection.layout_id&&!fleet->slots[i].queued&&
   (int32_t)(ctx->rate[j].until-now)>0&&(int32_t)(ctx->rate[j].until-fleet->slots[i].next_poll)>0)
    fleet->slots[i].next_poll=ctx->rate[j].until;
 if(ctx->online&&!online)dsr_auto_pause(&ctx->model,now,utc);
 ctx->online=online;
 if(got_ack&&ctx->effect.kind==DSR_AUTO_EFFECT_JOURNAL&&ctx->journal_submitted&&ctx->effect.generation==ack){
  dsr_auto_complete(&ctx->model,&ctx->effect,saved==ESP_OK?DSR_OK:DSR_IO);ctx->journal_submitted=false;ctx->effect=(dsr_auto_effect_t){0};
 }
 if(done&&ctx->run_id==run){
  if(outcome!=DSR_OK&&!attempted)dsr_auto_unqueue(&ctx->model,&ctx->effect);
  else dsr_auto_complete(&ctx->model,&ctx->effect,outcome);
  ctx->run_id=0;ctx->effect=(dsr_auto_effect_t){0};
 }
 if(!ctx->read_job&&!ctx->run_id&&ctx->model.record.generation!=ctx->model.durable_generation&&
   (ctx->effect.kind==DSR_AUTO_EFFECT_READ_TRV||ctx->effect.kind==DSR_AUTO_EFFECT_SET_TRV)){
  dsr_auto_unqueue(&ctx->model,&ctx->effect);ctx->effect=(dsr_auto_effect_t){0};
 }
 if(!online&&!ctx->read_job&&!ctx->run_id&&ctx->effect.kind==DSR_AUTO_EFFECT_READ_TRV){dsr_auto_unqueue(&ctx->model,&ctx->effect);ctx->effect=(dsr_auto_effect_t){0};}
 configure(fleet);uint16_t participants=0;
 for(unsigned r=0;r<ctx->model.record.rules.count;r++)if(ctx->model.record.rules.items[r].enabled)
  participants|=(uint16_t)(ctx->model.record.rules.items[r].sensors|ctx->model.record.rules.items[r].targets);
 for(unsigned i=0;i<ctx->model.record.rules.key_count;i++){
  if(!(participants&(1u<<i)))continue;
  const dsr_auto_key_t *k=&ctx->model.record.rules.keys[i];const dsr_fleet_slot_t *s=find_slot(fleet,k->layout_id);if(!s)continue;
  dsr_auto_observation_t o={.key=actual_key(&s->connection,k->kind),.sample_ms=s->sample_ms,.source_utc=s->connection.telemetry.blu_updated_s,
   .valid=online&&s->layout_enabled&&s->sample_valid&&!s->blocked&&s->connection.device.online&&capability(&s->connection,k->kind),
   .open=(s->connection.telemetry.blu.flags&DSR_BLU_OPEN)!=0};
  /* TRV mirror has read_id0: key verification only, never direct proof. */
  dsr_auto_observe(&ctx->model,&o);
 }
 if((ctx->effect.kind==DSR_AUTO_EFFECT_SET_TRV||ctx->effect.kind==DSR_AUTO_EFFECT_READ_TRV)&&!ctx->run_id&&!ctx->read_job&&
   (!public_allows(&ctx->effect)||!dsr_auto_effect_allowed(&ctx->model,&ctx->effect,now,utc))){
  dsr_auto_unqueue(&ctx->model,&ctx->effect);ctx->effect=(dsr_auto_effect_t){0};
 }
 if(!ctx->effect.kind){dsr_auto_effect_t effect;if(dsr_auto_next(&ctx->model,now,utc,&effect)){
  ctx->effect=effect;if(effect.kind==DSR_AUTO_EFFECT_JOURNAL)ctx->journal=ctx->model.record;}}
 dsr_auto_effect_t effect=ctx->effect;bool submit=effect.kind==DSR_AUTO_EFFECT_JOURNAL&&!ctx->journal_submitted;
 const dsr_fleet_slot_t *target=effect.kind==DSR_AUTO_EFFECT_SET_TRV?find_slot(fleet,effect.layout_id):NULL;
 bool execute=online&&target&&target->layout_enabled&&!target->blocked&&!target->in_flight&&!target->queued&&
  rate_ready(effect.layout_id,now)&&!(target->result==DSR_RATE_LIMITED&&(int32_t)(now-target->next_poll)<0)&&effect.kind==DSR_AUTO_EFFECT_SET_TRV&&!ctx->run_id;give();
 if(submit&&dsr_workspace_auto_submit(&ctx->journal,effect.generation)){
  if(take()){if(ctx->effect.id==effect.id)ctx->journal_submitted=true;give();}
 }
 if(execute){uint32_t next_run=0;dsr_result_t result=dsr_actions_auto_request(effect.rule_id,effect.cycle,effect.generation,&effect.layout_id,&effect.target_half,1,&next_run);
  if(take()){if(ctx->effect.id==effect.id){
   if(result==DSR_OK)ctx->run_id=next_run;
   else if(result!=DSR_BUSY){dsr_auto_unqueue(&ctx->model,&effect);ctx->effect=(dsr_auto_effect_t){0};ctx->configured=false;}
  }give();}
 }
}
void dsr_automation_snapshot(dsr_auto_snapshot_t *out)
{
 if(!out)return;
 memset(out,0,sizeof(*out));if(!take())return;
 dsr_auto_snapshot_t model;dsr_auto_model_snapshot(&ctx->model,&model);*out=model;out->count=ctx->public_rules.count;out->revision=ctx->revision;
 for(unsigned i=0;i<out->count;i++){
  const dsr_auto_rule_t *r=&ctx->public_rules.items[i];out->rules[i]=(dsr_auto_rule_status_t){.rule_id=r->id,.state=r->enabled?DSR_AUTO_WAITING:DSR_AUTO_DISABLED,.result=ctx->config_error};
  for(unsigned j=0;j<model.count;j++)if(model.rules[j].rule_id==r->id&&ctx->configured){out->rules[i]=model.rules[j];break;}
  if(!r->enabled)out->rules[i].state=DSR_AUTO_DISABLED;
  if(ctx->model.fault!=DSR_OK){out->rules[i].result=ctx->model.fault;if(r->enabled)out->rules[i].state=DSR_AUTO_ERROR;}
 }
 give();
}
