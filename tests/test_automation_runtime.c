/* Real actor/worker/workspace/model/store; only SDK, radio and clock are fake. */
#define _POSIX_C_SOURCE 200809L
#include "dsr_automation.h"
#include "dsr_automation_model.h"
#include "dsr_automation_config.h"
#include "dsr_automation_store.h"
#include "dsr_workspace.h"
#include "dsr_actions.h"
#include "dsr_activity.h"
#include "dsr_activity_internal.h"
#include "esp_heap_caps.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "automation/nvs_fixture.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static unsigned checks,sets,reads;static uint32_t now,serial=100;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#x);exit(1);}}while(0)
typedef struct{size_t size;bool full;uint8_t bytes[1192];} queue_t;
static queue_t queues[3];static unsigned queue_count,lock_count;static bool locks[4],available;
static void (*config_task)(void*),(*worker_task)(void*);static jmp_buf idle;
static dsr_fleet_t fleet;static bool windows[2],wifi=true,hold_contact;static uint8_t remote[4]={0,0,40,44};
static dsr_http_job_guard_t guard;static dsr_http_job_write_hook_t write_hook;static bool(*urgent)(void);
static char *durable;
static const char *current_scenario;static bool reopened;static uint32_t limited_at;
static void samples(void);
static const char config_json[]="{\"schema\":3,\"revision\":7,\"language\":\"pl\",\"theme\":\"cyber_blue\",\"accent\":5020661,\"active_dashboard\":1,\"rooms\":[],\"devices\":[{\"id\":1,\"name\":\"Window A\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0,\"kind\":\"contact\"},{\"id\":2,\"name\":\"Window B\",\"host\":\"192.0.2.1\",\"channel\":1,\"room\":0,\"kind\":\"contact\"},{\"id\":3,\"name\":\"TRV A\",\"host\":\"192.0.2.1\",\"channel\":2,\"room\":0,\"kind\":\"sensor\"},{\"id\":4,\"name\":\"TRV B\",\"host\":\"192.0.2.1\",\"channel\":3,\"room\":0,\"kind\":\"sensor\"}],\"groups\":[],\"scenes\":[{\"id\":1,\"name\":\"Manual\",\"actions\":[{\"device\":3,\"command\":\"target\",\"value\":50}]}],\"automations\":[{\"id\":7,\"name\":\"Heat\",\"kind\":\"contact_heating\",\"enabled\":false,\"sensors\":[1,2],\"targets\":[3,4],\"target_half\":16}],\"dashboards\":[{\"id\":1,\"name\":\"Home\",\"tiles\":[]}]}";
QueueHandle_t xQueueCreate(unsigned n,unsigned size){CHECK(n==1&&queue_count<3&&size<=1192);queue_t *q=&queues[queue_count++];q->size=size;return q;}
int xQueueSend(QueueHandle_t h,const void *p,TickType_t wait){(void)wait;queue_t *q=h;if(q->full)return pdFALSE;memcpy(q->bytes,p,q->size);q->full=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t h,void *p,TickType_t wait){queue_t *q=h;if(!q->full){if(wait)longjmp(idle,1);return pdFALSE;}memcpy(p,q->bytes,q->size);q->full=false;return pdTRUE;}
void vQueueDelete(QueueHandle_t h){(void)h;}
SemaphoreHandle_t xSemaphoreCreateMutex(void){CHECK(lock_count<4);return &locks[lock_count++];}
SemaphoreHandle_t xSemaphoreCreateBinary(void){return &available;}
int xSemaphoreTake(SemaphoreHandle_t h,TickType_t wait){(void)wait;if(h==&available){if(!available)return pdFALSE;available=false;return pdTRUE;}bool *p=h;CHECK(!*p);*p=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t h){bool *p=h;if(h!=&available)CHECK(*p);*p=h==&available;return pdTRUE;}
void vSemaphoreDelete(SemaphoreHandle_t h){(void)h;}
int xTaskCreate(void(*f)(void*),const char *name,unsigned stack,void *arg,unsigned pri,TaskHandle_t *out){
 CHECK(!arg&&!out);if(!strcmp(name,"dsr_config")){CHECK(stack==8192&&pri==3);config_task=f;}
 else{CHECK(!strcmp(name,"dsr_network")&&stack==10240&&pri==4);worker_task=f;}return pdPASS;
}
void *heap_caps_calloc(size_t n,size_t s,unsigned caps){CHECK(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return calloc(n,s);}
uint32_t esp_random(void){return 12345;}
int64_t esp_timer_get_time(void){return (int64_t)now*1000;}
time_t __wrap_time(time_t *out){time_t value=1800000000+(time_t)(now/1000);if(out)*out=value;return value;}
esp_err_t dsr_storage_mount(void){dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);return ESP_OK;}
char *dsr_config_store_load(const char *d,bool *found,char error[96]){CHECK(!strcmp(d,"/dsr"));*found=true;error[0]=0;return strdup(durable?durable:config_json);}
bool dsr_config_store_save(const char *d,const char *s,char error[96]){CHECK(!strcmp(d,"/dsr")&&s);error[0]=0;free(durable);durable=strdup(s);return durable!=NULL;}
bool dsr_config_store_collect_photos(const char *d,unsigned *n,char error[96]){(void)d;*n=0;error[0]=0;return true;}
void dsr_photos_load(void){}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){return c&&dsr_ipv4_valid(c->device.host)&&(!bound||c->device.device_id[0]);}
bool dsr_wifi_connected(void){return wifi;}
bool dsr_wifi_temporary_active(void){return false;}
void dsr_http_urgent_hook(bool(*f)(void)){urgent=f;}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t f){dsr_http_job_guard_t old=guard;guard=f;return old;}
dsr_http_job_write_hook_t dsr_http_job_write_hook(dsr_http_job_write_hook_t f){dsr_http_job_write_hook_t old=write_hook;write_hook=f;return old;}
dsr_result_t dsr_http_trv_read(dsr_connection_t *c){
 CHECK(c->layout_id==3||c->layout_id==4);if(guard){dsr_result_t r=guard();if(r!=DSR_OK)return r;}
 if(!strcmp(current_scenario,"rate-limit")&&c->layout_id==3){if(!limited_at){limited_at=now;return DSR_RATE_LIMITED;}CHECK(now-limited_at>=60000u);}
 reads++;c->device.online=true;c->telemetry.blu.flags=DSR_BLU_VALID;c->telemetry.blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET|DSR_BLU_TRV_CHECKED;
 c->telemetry.trv_target_c=remote[c->layout_id-1]/2.0f;c->telemetry.trv_current_c=21.5f;return DSR_OK;
}
dsr_result_t dsr_http_trv(dsr_connection_t *c,dsr_trv_intent_t i){
 if(!reopened&&i.half_c>16&&!strcmp(current_scenario,"reopen-before-restore")){
  reopened=true;windows[0]=true;samples();dsr_automation_tick(&fleet,now,1800000000+now/1000,true);
 }
 CHECK(guard&&write_hook&&dsr_trv_intent_valid(i));dsr_result_t r=guard();if(r!=DSR_OK)return r;
 CHECK(auto_commits>0);write_hook();sets++;remote[c->layout_id-1]=i.half_c;
 if(!reopened&&i.half_c>16&&!strcmp(current_scenario,"reopen-after-restore")){
  reopened=true;windows[0]=true;samples();dsr_automation_tick(&fleet,now,1800000000+now/1000,true);
 }
 /* ACK + deliberately stale mirrored target, never proof of actual TRV. */
 c->telemetry.trv_target_c=c->layout_id==3?20:22;c->device.online=true;return DSR_OK;
}
dsr_result_t dsr_http_refresh_info(dsr_connection_t *c,dsr_device_info_t *info){if(info)memset(info,0,sizeof(*info));c->device.online=true;return DSR_OK;}
dsr_result_t dsr_http_set_output(dsr_connection_t *c,bool on){(void)c;(void)on;abort();}
dsr_result_t dsr_http_cover(dsr_connection_t *c,dsr_cover_intent_t i){(void)c;(void)i;abort();}
dsr_result_t dsr_http_light(dsr_connection_t *c,dsr_light_intent_t i){(void)c;(void)i;abort();}
dsr_result_t dsr_http_rgb(dsr_connection_t *c,dsr_rgb_intent_t i){(void)c;(void)i;abort();}
dsr_result_t dsr_http_probe_info(const dsr_connection_t *c,dsr_device_t *d,dsr_telemetry_t *t,dsr_device_info_t *info){(void)c;(void)d;(void)t;(void)info;abort();}
esp_err_t dsr_connection_remove(const dsr_connection_t *c){(void)c;abort();}
esp_err_t dsr_connection_save(const dsr_connection_t *c){(void)c;abort();}
esp_err_t dsr_connection_update_name(const dsr_connection_t *c){(void)c;return ESP_OK;}
void dsr_ap_execute(const dsr_job_t *j,dsr_event_t *e){(void)j;(void)e;abort();}
void dsr_ap_prepare(void){}
void dsr_wifi_prepare_connect(void){}
dsr_result_t dsr_wifi_scan(dsr_wifi_scan_t *s){(void)s;abort();}
dsr_result_t dsr_wifi_connect_saved(const dsr_settings_t *s,dsr_wifi_status_t *d){(void)s;(void)d;abort();}
esp_err_t dsr_settings_load(dsr_settings_t *s){(void)s;abort();}
esp_err_t dsr_settings_save(const dsr_settings_t *s){(void)s;abort();}
esp_err_t dsr_native_ota_init(void){return ESP_OK;}
bool dsr_native_ota_admitted(uint32_t id,dsr_job_kind_t k){(void)id;(void)k;return false;}
void dsr_native_ota_execute(const dsr_job_t *j,dsr_event_t *e){(void)j;(void)e;abort();}
esp_err_t dsr_panel_pairing_revoke(void){return ESP_OK;}
bool dsr_battery_profile_equal(const dsr_battery_profile_t *a,const dsr_battery_profile_t *b){(void)a;(void)b;return false;}
bool dsr_battery_profile_valid(const dsr_battery_profile_t *p){(void)p;return false;}
esp_err_t dsr_battery_profile_load(dsr_battery_profile_t *p){(void)p;return ESP_FAIL;}
esp_err_t dsr_battery_profile_save(const dsr_battery_profile_t *p){(void)p;return ESP_FAIL;}
esp_err_t dsr_battery_read(dsr_battery_sample_t *s){(void)s;return ESP_FAIL;}
void dsr_battery_apply_profile(const dsr_battery_profile_t *p,dsr_battery_sample_t *s){(void)p;(void)s;}
static void pump(void(*f)(void*)){if(!setjmp(idle))f(NULL);}
static void samples(void){
 for(unsigned i=0;i<4;i++){dsr_fleet_slot_t *s=&fleet.slots[i];s->sample_valid=true;s->sample_ms=now;s->connection.device.online=true;
  if(i<2){s->connection.telemetry.blu.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|(windows[i]?DSR_BLU_OPEN:0);if(!hold_contact||i!=1)s->connection.telemetry.blu_updated_s=1800000000+now/1000;}}
}
static void frame(void){
 now+=1000;samples();CHECK(dsr_actions_publish_begin(dsr_workspace_revision(),wifi));
 for(unsigned i=0;i<4;i++)dsr_actions_publish_at(&fleet.slots[i].connection,true,false,DSR_OK,true);
 dsr_actions_publish_end();
 dsr_automation_tick(&fleet,now,1800000000+now/1000,wifi);if(queues[0].full)pump(config_task);
 dsr_job_t j={0};bool direct=dsr_automation_prepare_read(&fleet,++serial,now,&j);
 int at=-1;if(!direct){uint32_t target=dsr_actions_target();if(!target)return;for(unsigned i=0;i<4;i++)if(fleet.slots[i].connection.layout_id==target)at=(int)i;
  CHECK(at>=0);j=(dsr_job_t){.id=serial,.connection=fleet.slots[at].connection};if(!dsr_actions_prepare_job(&j,DSR_OK,true))return;dsr_fleet_started(&fleet,(unsigned)at,&j);}
 CHECK(dsr_worker_submit(&j)==DSR_OK);pump(worker_task);dsr_event_t e;CHECK(dsr_activity_begin());CHECK(dsr_worker_receive(&e)&&e.id==j.id);
 if(direct)dsr_automation_complete_read(&e,now);else{
  bool unsent=dsr_actions_auto_job_owned(e.id)&&!dsr_actions_job_attempted(e.id)&&(e.result==DSR_CONFLICT||e.result==DSR_BUSY);
  CHECK(unsent?dsr_fleet_cancel_started(&fleet,(unsigned)at,e.id):dsr_fleet_complete(&fleet,(unsigned)at,&e,now));dsr_actions_complete(e.id,e.result);
 }dsr_activity_end();
}
static void frames(unsigned n){for(unsigned i=0;i<n;i++)frame();}
static void prepare_reboot(const char *scenario){
 cJSON *r=cJSON_Parse(config_json);CHECK(r);cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(r,"automations"),0),"enabled",cJSON_CreateTrue());
 durable=cJSON_PrintUnformatted(r);dsr_auto_rules_t rules;char error[96];CHECK(dsr_auto_rules_parse(r,&rules,error));cJSON_Delete(r);
 for(unsigned i=0;i<rules.key_count;i++){dsr_auto_key_t *k=&rules.keys[i];strcpy(k->host,"192.0.2.1");snprintf(k->device_id,64,"shellyblugwg3-test_%012u",k->layout_id);k->generation=2;k->channel=(uint8_t)(k->layout_id-1);}
 rules.enroll=true;dsr_auto_model_t model;dsr_auto_init(&model);CHECK(dsr_auto_configure(&model,&rules)==DSR_OK);
 model.record.generation++;model.record.cycles[0]=(dsr_auto_cycle_t){.cycle=1,.active=1};
 uint8_t phase=!strcmp(scenario,"reboot-own")?DSR_AUTO_OWNED:!strcmp(scenario,"reboot-restore")?DSR_AUTO_RESTORE_PREPARED:DSR_AUTO_PREPARED;
 model.record.targets[2]=(dsr_auto_target_t){.baseline=40,.desired=16,.phase=phase};model.record.targets[3]=(dsr_auto_target_t){.baseline=44,.desired=16,.phase=phase};
 if(!strcmp(scenario,"reboot-restore")){model.record.cycles[0].active=DSR_AUTO_CYCLE_RESTORING;model.record.targets[2].desired=40;model.record.targets[3].desired=44;remote[2]=remote[3]=16;}
 CHECK(dsr_auto_store_save(&model.record)==ESP_OK);windows[0]=windows[1]=true;
 if(!strcmp(scenario,"reboot-own")){remote[2]=50;remote[3]=16;}
 else if(!strcmp(scenario,"reboot-prepared"))remote[2]=remote[3]=16;
 if(!strcmp(scenario,"corrupt"))auto_flash[0][auto_lengths[0]-1]^=1;
}
static void manual_scene(unsigned nonce){
 dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);char request[256];snprintf(request,sizeof(request),
  "{\"id\":1,\"revision\":%u,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%016x\"}",s.revision,s.boot_id,s.admission_generation,nonce);
 uint32_t run;CHECK(dsr_actions_request(request,1,1,&run)==DSR_OK);
}
int main(int argc,char **argv){
 CHECK(argc==2);const char *scenario=argv[1];current_scenario=scenario;bool reboot=!strncmp(scenario,"reboot-",7)||!strcmp(scenario,"corrupt");
 auto_flash_reset();if(reboot)prepare_reboot(scenario);
 if(!strcmp(scenario,"manual-read-wait")){
  cJSON *r=cJSON_Parse(config_json);cJSON *step=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"scenes")->child,"actions")->child;
  cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(step,"device"),4);durable=cJSON_PrintUnformatted(r);cJSON_Delete(r);
 }
 CHECK(dsr_workspace_start()==ESP_OK);CHECK(dsr_actions_init()==ESP_OK);CHECK(dsr_automation_init()==ESP_OK);CHECK(dsr_worker_start()==ESP_OK);
 for(unsigned i=0;i<4;i++){dsr_connection_t c={.layout_id=i+1,.device={.host="192.0.2.1",.generation=2,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.channel=i}};
  snprintf(c.device.device_id,64,"shellyblugwg3-test_%012u",i+1);if(i>=2){c.telemetry.blu.flags=DSR_BLU_VALID;c.telemetry.blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET;c.telemetry.trv_target_c=remote[i]/2.0f;}CHECK(dsr_fleet_upsert(&fleet,&c,now)==DSR_OK);}
 if(reboot){
  frames(45);CHECK(sets==0);windows[0]=windows[1]=false;frames(45);
  if(!strcmp(scenario,"reboot-own"))CHECK(sets==1&&remote[2]==50&&remote[3]==44);
  else if(!strcmp(scenario,"reboot-prepared"))CHECK(sets==2&&remote[2]==40&&remote[3]==44);
  else CHECK(sets==0);
  if(!strcmp(scenario,"reboot-restore")){dsr_auto_snapshot_t s;dsr_automation_snapshot(&s);CHECK(s.rules[0].state==DSR_AUTO_CONFIRMING);}
  if(!strcmp(scenario,"corrupt")){manual_scene(0xabc);frames(5);dsr_actions_snapshot_t action;dsr_actions_snapshot(&action);CHECK(sets==0&&!action.active&&action.steps[0].result==DSR_IO);}
  goto finished;
 }
 frames(3);CHECK(sets==0);char *json=dsr_workspace_snapshot();cJSON *root=cJSON_Parse(json);free(json);
 cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(root,"automations"),0),"enabled",cJSON_CreateTrue());
 json=cJSON_PrintUnformatted(root);CHECK(dsr_workspace_submit(json,7));free(json);cJSON_Delete(root);pump(config_task);CHECK(dsr_workspace_revision()==8);
 dsr_auto_snapshot_t pending;dsr_automation_snapshot(&pending);CHECK(pending.count==1&&pending.revision==8&&pending.rules[0].state!=DSR_AUTO_DISABLED);
 windows[0]=windows[1]=true;frames(!strcmp(scenario,"rate-limit")?120:45);CHECK(sets==2&&reads>=4&&remote[2]==16&&remote[3]==16);
 dsr_auto_snapshot_t snapshot;dsr_automation_snapshot(&snapshot);CHECK(snapshot.count==1&&snapshot.rules[0].state==DSR_AUTO_ACTIVE);
 if(!strncmp(scenario,"reopen-",7)){
  windows[0]=windows[1]=false;frames(45);CHECK(reopened&&remote[2]==16&&remote[3]==16);
  unsigned expected=!strcmp(scenario,"reopen-before-restore")?2u:4u;CHECK(sets==expected);
  windows[0]=windows[1]=false;frames(45);
  if(sets!=expected+2||remote[2]!=40||remote[3]!=44){dsr_automation_snapshot(&snapshot);fprintf(stderr,"reopen diagnostic: sets=%u remote=%u/%u state=%u cycle=%u phases=%u/%u base=%u/%u requested=%u/%u\n",sets,remote[2],remote[3],snapshot.rules[0].state,snapshot.rules[0].cycle,snapshot.targets[0].phase,snapshot.targets[1].phase,snapshot.targets[0].baseline,snapshot.targets[1].baseline,snapshot.targets[0].requested,snapshot.targets[1].requested);}
  CHECK(sets==expected+2&&remote[2]==40&&remote[3]==44);goto finished;
 }
 if(!strcmp(scenario,"rebind")){
  strcpy(fleet.slots[2].connection.device.device_id,"replacement");windows[0]=windows[1]=false;frames(45);CHECK(sets==2);
  dsr_automation_snapshot(&snapshot);CHECK(snapshot.rules[0].state==DSR_AUTO_ERROR&&snapshot.rules[0].result==DSR_IDENTITY_MISMATCH);goto finished;
 }
 if(!strcmp(scenario,"disable")){
  json=dsr_workspace_snapshot();root=cJSON_Parse(json);free(json);cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"automations")->child,"enabled",cJSON_CreateFalse());
  json=cJSON_PrintUnformatted(root);cJSON_Delete(root);CHECK(dsr_workspace_submit(json,8));free(json);pump(config_task);CHECK(dsr_workspace_revision()==9);
  windows[0]=windows[1]=false;frames(45);CHECK(sets==2);dsr_automation_snapshot(&snapshot);CHECK(snapshot.rules[0].state==DSR_AUTO_DISABLED);goto finished;
 }
 if(!strcmp(scenario,"active-edit")){
  json=dsr_workspace_snapshot();root=cJSON_Parse(json);free(json);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"automations")->child,"target_half"),18);
  json=cJSON_PrintUnformatted(root);cJSON_Delete(root);CHECK(dsr_workspace_submit(json,8));free(json);pump(config_task);CHECK(dsr_workspace_revision()==8);
  bool busy;char status[96];dsr_workspace_status(&busy,status);CHECK(!busy&&strstr(status,"Disable"));CHECK(sets==2);goto finished;
 }
 if(!strcmp(scenario,"stale")){
  hold_contact=true;fleet.slots[1].connection.telemetry.blu_updated_s=0;windows[0]=windows[1]=false;frames(45);CHECK(sets==2);
  hold_contact=false;frames(45);CHECK(sets==4&&remote[2]==40&&remote[3]==44);goto finished;
 }
 if(!strcmp(scenario,"manual-read-wait")){
  fleet.slots[2].blocked=true;now+=15000;samples();dsr_automation_tick(&fleet,now,1800000000+now/1000,true);
  manual_scene(0xabc);frames(10);CHECK(sets==3&&remote[3]==50);goto finished;
 }
 if(!strcmp(scenario,"manual")||!strcmp(scenario,"manual-failure")||!strcmp(scenario,"manual-retry")){
  manual_scene(0xabc);if(strcmp(scenario,"manual"))auto_flash_fault(3);
  frames(10);dsr_actions_snapshot_t action;dsr_actions_snapshot(&action);
  if(strcmp(scenario,"manual")){
   CHECK(sets==2&&remote[2]==16&&!action.active&&action.steps[0].outcome==DSR_ACTION_FAILED&&action.steps[0].result==DSR_IO);
   if(!strcmp(scenario,"manual-failure"))goto finished;
   auto_flash_fault(0);manual_scene(0xabd);frames(10);dsr_actions_snapshot(&action);
  }
  CHECK(sets==3&&remote[2]==50&&!action.active&&action.steps[0].outcome==DSR_ACTION_SUCCESS);
  windows[0]=windows[1]=false;frames(45);CHECK(sets==4&&remote[2]==50&&remote[3]==44);goto finished;
 }
 windows[0]=false;frames(5);CHECK(sets==2);windows[1]=false;frames(45);CHECK(sets==4&&remote[2]==40&&remote[3]==44);
finished:
 printf("automation runtime %s: %u checks passed; actual config/actor/worker/journal, radio MODEL\n",scenario,checks);free(durable);return 0;
}
