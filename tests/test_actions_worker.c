/* Actual actor + actual sole worker. HTTP functions are explicit transaction
 * models; separate platform suite exercises guard ordering in actual HTTP. */
#define _POSIX_C_SOURCE 200809L
#include "dsr_actions.h"
#include "dsr_workspace.h"
#include "dsr_native_ota.h"
#include "../components/dsr_platform/dsr_ap_private.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,http_gets,posts,stops,mode;static uint32_t now=120000,run_id;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
typedef struct {size_t size;bool full;unsigned char bytes[sizeof(dsr_event_t)];} queue_t;
static queue_t queues[2];static unsigned queue_count,lock_count;static bool locks[3],available,power_done;static unsigned binary_count;
static void(*worker)(void *);static jmp_buf idle;
static bool(*urgent)(void);static dsr_http_job_guard_t guard;static dsr_http_job_write_hook_t write_hook;
static bool mapping=true;static dsr_connection_t a,b;static uint32_t outer_id;
QueueHandle_t xQueueCreate(unsigned n,unsigned size){CHECK(n==1&&queue_count<2&&size<=sizeof(queues[0].bytes));queue_t *q=&queues[queue_count++];q->size=size;return q;}
int xQueueSend(QueueHandle_t h,const void *p,TickType_t wait){(void)wait;queue_t *q=h;CHECK(!q->full);memcpy(q->bytes,p,q->size);q->full=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t h,void *p,TickType_t wait){queue_t *q=h;if(!q->full){if(wait)longjmp(idle,1);return pdFALSE;}memcpy(p,q->bytes,q->size);q->full=false;return pdTRUE;}
void vQueueDelete(QueueHandle_t h){(void)h;}
SemaphoreHandle_t xSemaphoreCreateMutex(void){CHECK(lock_count<3);return &locks[lock_count++];}
SemaphoreHandle_t xSemaphoreCreateBinary(void){CHECK(binary_count++<2);return binary_count==1?&available:&power_done;}
int xSemaphoreTake(SemaphoreHandle_t h,TickType_t wait){(void)wait;if(h==&available||h==&power_done){bool *v=h;if(!*v)return pdFALSE;*v=false;return pdTRUE;}bool *lock=h;CHECK((lock==&locks[0]||lock==&locks[1]||lock==&locks[2])&&!*lock);*lock=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t h){bool *lock=h;if(h!=&available&&h!=&power_done)CHECK((lock==&locks[0]||lock==&locks[1]||lock==&locks[2])&&*lock);*lock=h==&available||h==&power_done;return pdTRUE;}
void vSemaphoreDelete(SemaphoreHandle_t h){(void)h;}
int xTaskCreate(void(*f)(void *),const char *name,unsigned stack,void *arg,unsigned pri,TaskHandle_t *out){CHECK(!strcmp(name,"dsr_network")&&stack==10240&&!arg&&pri==4&&!out);worker=f;return pdPASS;}
void *heap_caps_calloc(size_t n,size_t s,unsigned caps){CHECK(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return calloc(n,s);}
int64_t esp_timer_get_time(void){return (int64_t)now*1000;}
uint32_t esp_random(void){return 12345;}
bool dsr_wifi_connected(void){return true;}
bool dsr_wifi_temporary_active(void){return false;}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){return c&&dsr_ipv4_valid(c->device.host)&&(!bound||c->device.device_id[0]);}
bool dsr_workspace_mapping_matches(const dsr_connection_t *c){return mapping&&c->layout_id>0;}
static char *saved_configuration(void){return strdup("{\"schema\":3,\"automations\":[],\"revision\":7,\"language\":\"pl\",\"theme\":\"graphite\",\"accent\":5020661,\"active_dashboard\":1,\"rooms\":[],\"devices\":[{\"id\":42,\"name\":\"Switch\",\"host\":\"192.0.2.42\",\"channel\":0,\"room\":0,\"kind\":\"relay\"},{\"id\":43,\"name\":\"Cover\",\"host\":\"192.0.2.43\",\"channel\":0,\"room\":0,\"kind\":\"cover\"}],\"groups\":[],\"scenes\":[{\"id\":1,\"name\":\"Switch\",\"actions\":[{\"device\":42,\"command\":\"on\"},{\"device\":42,\"command\":\"off\"}]},{\"id\":2,\"name\":\"Cover\",\"actions\":[{\"device\":43,\"command\":\"open\"}]},{\"id\":3,\"name\":\"Stop\",\"actions\":[{\"device\":43,\"command\":\"stop\"}]}],\"dashboards\":[{\"id\":1,\"name\":\"Home\",\"tiles\":[]}]}");}
static bool typed_config,probe_close;
char *dsr_workspace_snapshot(void)
{
 char *json=saved_configuration();if(!typed_config&&!probe_close)return json;
 cJSON *r=cJSON_Parse(json);free(json);CHECK(r);cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"groups"),cJSON_Parse("{\"id\":1,\"name\":\"Typed group\",\"members\":[42]}"));
 if(probe_close){cJSON *scene=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(r,"scenes"),1);cJSON *action=cJSON_GetObjectItemCaseSensitive(scene,"actions")->child;cJSON_ReplaceItemInObjectCaseSensitive(action,"command",cJSON_CreateString("close"));}
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"scenes"),cJSON_Parse("{\"id\":4,\"name\":\"TRV target\",\"actions\":[{\"device\":42,\"command\":\"target\",\"value\":43}]}"));json=cJSON_PrintUnformatted(r);cJSON_Delete(r);return json;
}
void dsr_http_urgent_hook(bool(*f)(void)){urgent=f;}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t f){dsr_http_job_guard_t old=guard;guard=f;return old;}
dsr_http_job_write_hook_t dsr_http_job_write_hook(dsr_http_job_write_hook_t hook){dsr_http_job_write_hook_t previous=write_hook;write_hook=hook;return previous;}
static dsr_http_job_progress_hook_t progress_hook;
dsr_http_job_progress_hook_t dsr_http_job_progress_hook(dsr_http_job_progress_hook_t hook){dsr_http_job_progress_hook_t previous=progress_hook;progress_hook=hook;return previous;}
dsr_result_t dsr_http_set_output(dsr_connection_t *c,bool on)
{
 CHECK(guard&&c->device.endpoint_kind==DSR_ENDPOINT_SWITCH);dsr_result_t result=guard();if(result!=DSR_OK)return result;http_gets++;
 if(mode==1)CHECK(dsr_actions_cancel(12345,run_id));
 if(mode==2){dsr_http_job_guard_t outer=guard;dsr_http_job_write_hook_t outer_write=write_hook;dsr_http_job_progress_hook_t outer_progress=progress_hook;CHECK(dsr_worker_stop_request(&b,outer_id)==DSR_OK);CHECK(urgent&&urgent());CHECK(guard==outer&&write_hook==outer_write&&progress_hook==outer_progress);}
 result=guard();if(result!=DSR_OK)return result;CHECK(write_hook);write_hook();posts++;c->device.output=on;c->device.online=true;return DSR_OK;
}
dsr_result_t dsr_http_cover(dsr_connection_t *c,dsr_cover_intent_t intent)
{
 if(intent.command==DSR_COVER_STOP){/* Manual innerSTOP has its own phase observer and suspends outer actor cancellation. */CHECK(progress_hook);if(mode==2)CHECK(!guard&&!write_hook);else if(guard){dsr_result_t result=guard();if(result!=DSR_OK)return result;}if(write_hook)write_hook();progress_hook(DSR_COMMAND_WRITE);stops++;c->device.online=false;progress_hook(DSR_COMMAND_ACK);return DSR_OK;}
 CHECK(guard);dsr_result_t result=guard();if(result!=DSR_OK)return result;http_gets++;result=guard();if(result==DSR_OK){CHECK(write_hook&&progress_hook);write_hook();progress_hook(DSR_COMMAND_WRITE);posts++;progress_hook(DSR_COMMAND_ACK);c->device.online=false;}return result;
}
static dsr_result_t typed_transaction(dsr_connection_t *c)
{
 CHECK(guard&&write_hook&&c->layout_id==42);dsr_result_t r=guard();if(r!=DSR_OK)return r;http_gets++;
 if(mode==1)CHECK(dsr_actions_cancel(12345,run_id));
 if(mode==3)mapping=false;
 r=guard();if(r!=DSR_OK)return r;write_hook();CHECK(progress_hook);progress_hook(DSR_COMMAND_WRITE);posts++;if(mode==4)return DSR_OFFLINE;progress_hook(DSR_COMMAND_CONFIRMED);return DSR_OK;
}
dsr_result_t dsr_http_light(dsr_connection_t *c,dsr_light_intent_t intent){CHECK(intent.command==DSR_LIGHT_SET_BRIGHTNESS&&intent.brightness==65);return typed_transaction(c);}
dsr_result_t dsr_http_trv(dsr_connection_t *c,dsr_trv_intent_t intent){CHECK(intent.command==DSR_TRV_SET_TARGET&&intent.half_c==43);return typed_transaction(c);}
dsr_result_t dsr_http_trv_read(dsr_connection_t *c){(void)c;return DSR_UNSUPPORTED;}
static bool native_probe,expect_telemetry_only;
static unsigned metadata_reads;
dsr_result_t dsr_http_refresh(dsr_connection_t *c){CHECK(guard||native_probe);dsr_result_t result=guard?guard():DSR_OK;if(result==DSR_OK){http_gets++;c->device.online=true;}return result;}
dsr_result_t dsr_http_probe_telemetry(const dsr_connection_t *c,dsr_device_t *d,dsr_telemetry_t *t){(void)c;(void)d;(void)t;abort();}
void dsr_ap_execute(const dsr_job_t *j,dsr_event_t *e){(void)j;(void)e;abort();}
void dsr_ap_prepare(void){}
void dsr_wifi_prepare_connect(void){}
dsr_result_t dsr_wifi_scan(dsr_wifi_scan_t *s){(void)s;abort();}
dsr_result_t dsr_wifi_connect_saved(const dsr_settings_t *s,dsr_wifi_status_t *d){(void)s;(void)d;abort();}
esp_err_t dsr_settings_load(dsr_settings_t *s){(void)s;abort();}
esp_err_t dsr_settings_save(const dsr_settings_t *s){(void)s;abort();}
esp_err_t dsr_connection_remove(const dsr_connection_t *c){(void)c;abort();}
esp_err_t dsr_connection_save(const dsr_connection_t *c){(void)c;abort();}
esp_err_t dsr_native_ota_init(void){return ESP_OK;}
bool dsr_native_ota_admitted(uint32_t id,dsr_job_kind_t kind){(void)id;(void)kind;return false;}
void dsr_native_ota_execute(const dsr_job_t *j,dsr_event_t *e){(void)j;(void)e;abort();}
bool dsr_battery_profile_equal(const dsr_battery_profile_t *a,const dsr_battery_profile_t *b){(void)a;(void)b;abort();}
bool dsr_battery_profile_valid(const dsr_battery_profile_t *p){(void)p;return false;}
esp_err_t dsr_battery_profile_load(dsr_battery_profile_t *p){(void)p;abort();}
esp_err_t dsr_battery_profile_save(const dsr_battery_profile_t *p){(void)p;abort();}
void dsr_battery_apply_profile(const dsr_battery_profile_t *p,dsr_battery_sample_t *s){(void)p;(void)s;abort();}
esp_err_t dsr_battery_read(dsr_battery_sample_t *s){(void)s;abort();}
static void execute(void){if(!setjmp(idle))worker(NULL);CHECK(!guard&&!write_hook);}
static void begin(unsigned scene,unsigned nonce)
{
 CHECK(dsr_actions_publish_begin(7,true));dsr_actions_publish_at(&a,true,false,DSR_OK,true);dsr_actions_publish_at(&b,true,false,DSR_OK,true);dsr_actions_publish_end();dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);char json[256];snprintf(json,sizeof(json),"{\"id\":%u,\"revision\":7,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%016x\"}",scene,s.boot_id,s.admission_generation,nonce);CHECK(dsr_actions_request(json,1,0,&run_id)==DSR_OK&&!dsr_activity_restart_reserve());
}
static dsr_job_t prepare(uint32_t id,const dsr_connection_t *c)
{dsr_job_t j={.id=id,.connection=*c};CHECK(dsr_actions_prepare(c,DSR_OK,true,true,id,&j.kind,&j.on));if(j.kind==DSR_JOB_COVER){dsr_action_command_t cmd=dsr_actions_command();j.cover=(dsr_cover_intent_t){.command=cmd==DSR_ACTION_OPEN?DSR_COVER_OPEN:cmd==DSR_ACTION_CLOSE?DSR_COVER_CLOSE:DSR_COVER_STOP};}return j;}
static dsr_event_t complete(uint32_t id)
{dsr_event_t e;CHECK(dsr_activity_begin());CHECK(dsr_worker_receive(&e)&&e.id==id);CHECK(!dsr_activity_restart_reserve());dsr_actions_complete(e.id,e.result);dsr_activity_end();return e;}
static void idle_admission(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
int main(void)
{
 a=(dsr_connection_t){.layout_id=42,.device={.host="192.0.2.42",.device_id="shelly-A",.generation=2,.has_switch=true,.online=true}};b=a;b.layout_id=43;strcpy(b.device.host,"192.0.2.43");strcpy(b.device.device_id,"shelly-B");b.device.endpoint_kind=DSR_ENDPOINT_COVER;
 CHECK(dsr_worker_start()==ESP_OK&&dsr_actions_init()==ESP_OK);
 /* Cancel between actual worker guard legs: no model POST, no replay. */
 mode=1;begin(1,1);dsr_job_t j=prepare(101,&a);CHECK(dsr_worker_submit(&j)==DSR_OK);execute();CHECK(http_gets==1&&!posts);dsr_event_t e=complete(101);CHECK(e.result==DSR_BUSY&&!dsr_actions_target());idle_admission();
 /* Manual STOP uses the SAME numeric ID as outer actor job. It still
  * cancels the plan and its inner transaction suspends/restores outerguard. */
 mode=2;begin(1,2);j=prepare(102,&a);outer_id=102;CHECK(dsr_worker_submit(&j)==DSR_OK);execute();CHECK(http_gets==2&&!posts&&stops==1&&dsr_worker_stop_pending());e=complete(102);CHECK(e.result==DSR_BUSY&&!dsr_actions_target());CHECK(!dsr_activity_restart_reserve());dsr_stop_result_t stopped;CHECK(dsr_worker_stop_receive(&stopped,false)&&stopped.operation_id==102&&stopped.device.endpoint_kind==DSR_ENDPOINT_COVER);idle_admission();
 /* Cancel/deadline/rebind before queue take: zero first SDK request. */
 mode=0;for(unsigned fault=0;fault<3;fault++){begin(2,3+fault);j=prepare(103+fault,&b);CHECK(dsr_worker_submit(&j)==DSR_OK);unsigned before=http_gets+posts;if(fault==0)CHECK(dsr_actions_cancel(12345,run_id));else if(fault==1)now+=180000;else mapping=false;execute();e=complete(j.id);CHECK(http_gets+posts==before&&e.result==(fault==2?DSR_IDENTITY_MISMATCH:DSR_BUSY));mapping=true;(void)dsr_actions_cancel(12345,run_id);CHECK(!dsr_actions_target());idle_admission();}
 /* Tombstone also protects an actor Cover admitted before manual STOP. */
 begin(2,6);j=prepare(106,&b);CHECK(dsr_worker_submit(&j)==DSR_OK);CHECK(dsr_worker_stop_request(&b,106)==DSR_OK);unsigned before=posts;execute();e=complete(106);CHECK(e.result==DSR_BUSY&&posts==before&&!dsr_actions_target());CHECK(dsr_worker_stop_receive(&stopped,false)&&stopped.operation_id==106&&stops==2);idle_admission();
 /* An explicit scene STOP has separate trusted admission and its own guard;
  * it is not mistaken for an unrelated manual operationID or auto-cancelled. */
 begin(3,7);j=prepare(107,&b);CHECK(j.cover.command==DSR_COVER_STOP&&dsr_worker_action_stop_request(&b,107)==DSR_OK);execute();CHECK(dsr_worker_stop_receive(&stopped,false)&&stopped.operation_id==107&&stopped.result==DSR_OK&&stops==3);CHECK(dsr_activity_begin());dsr_actions_complete(107,stopped.result);CHECK(!dsr_actions_target());dsr_activity_end();idle_admission();
 /* A copied maintenance snapshot yields to the real urgent lane. */
 uint64_t reservation,generation;CHECK(dsr_activity_maintenance_reserve(&reservation,&generation));
 CHECK(dsr_worker_stop_request(&b,108)==DSR_OK&&!dsr_activity_maintenance_valid(reservation,generation));dsr_activity_maintenance_release(reservation);
 execute();CHECK(dsr_worker_stop_receive(&stopped,false)&&stopped.operation_id==108&&stopped.result==DSR_OK&&stops==4);idle_admission();
 typed_config=true;const uint8_t families[]={DSR_ENDPOINT_LIGHT,DSR_ENDPOINT_RGB,DSR_ENDPOINT_BLU};const char *payloads[]={"\"command\":\"brightness\",\"value\":65","\"command\":\"rgb\",\"rgb\":[10,120,250]","\"command\":\"target\",\"value\":43"};
 for(unsigned f=0;f<4;f++)for(unsigned fault=0;fault<4;fault++){
  mode=fault==2?3:fault==3?4:fault;a.device.endpoint_kind=families[f<3?f:2];memset(&a.telemetry,0,sizeof(a.telemetry));if(!f)a.telemetry.light.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS;else if(f==1)a.telemetry.rgb.flags=DSR_RGB_VALID;else a.telemetry.blu.errors=DSR_BLU_TRV;
  CHECK(dsr_actions_publish_begin(7,true));dsr_actions_publish_at(&a,true,false,DSR_OK,true);dsr_actions_publish_end();dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);char json[256];snprintf(json,sizeof(json),"{\"id\":1,\"revision\":7,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%016x\",%s}",s.boot_id,s.admission_generation,100+f*4+fault,payloads[f<3?f:2]);if(f==3)snprintf(json,sizeof(json),"{\"id\":4,\"revision\":7,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%016x\"}",s.boot_id,s.admission_generation,100+f*4+fault);CHECK(dsr_actions_request(json,f==3?1:0,1,&run_id)==DSR_OK);j=(dsr_job_t){.id=200+f*4+fault,.connection=a};CHECK(dsr_actions_prepare_job(&j,DSR_OK,true));unsigned before_posts=posts;CHECK(dsr_worker_submit(&j)==DSR_OK);execute();e=complete(j.id);mapping=true;CHECK(posts==before_posts+(fault==0||fault==3));CHECK(e.result==(!fault?DSR_OK:fault==1?DSR_BUSY:fault==2?DSR_IDENTITY_MISMATCH:DSR_OFFLINE));CHECK(e.command_phase==(!fault?DSR_COMMAND_CONFIRMED:fault==3?DSR_COMMAND_WRITE:DSR_COMMAND_NONE));CHECK(!dsr_actions_target());dsr_actions_snapshot(&s);CHECK(s.steps[0].outcome==(!fault?DSR_ACTION_SUCCESS:fault==2?DSR_ACTION_FAILED:DSR_ACTION_UNCERTAIN));idle_admission();
 }
 mode=0;begin(2,200);j=prepare(1300,&b);CHECK(dsr_worker_submit(&j)==DSR_OK);execute();e=complete(1300);
 CHECK(e.result==DSR_OK&&e.command_phase==DSR_COMMAND_ACK&&!e.device.online);idle_admission();
 CHECK(stopped.command_phase==DSR_COMMAND_ACK);
 /* The real worker must not append optional name/config I/O to the first
  * Cover status after a command. Normal metadata polls remain available. */
 native_probe=expect_telemetry_only=true;unsigned names_before=metadata_reads;
 j=(dsr_job_t){.id=1400,.kind=DSR_JOB_PROBE,.connection=b,.on=true};
 CHECK(dsr_worker_submit(&j)==DSR_OK);execute();e=complete(j.id);
 CHECK(e.result==DSR_OK&&e.device.online&&metadata_reads==names_before);idle_admission();
 expect_telemetry_only=false;j.id++;j.on=false;
 CHECK(dsr_worker_submit(&j)==DSR_OK);execute();e=complete(j.id);
 CHECK(e.result==DSR_OK&&metadata_reads==names_before+1);native_probe=false;idle_admission();
 /* A stale scene/group CLOSE first needs a normal probe. Its command byte
  * (2) must not survive as the PROBE boolean payload. Inspect raw bytes,
  * rather than itself loading an invalid _Bool representation. */
 probe_close=true;begin(2,300);j=(dsr_job_t){.id=1500,.connection=b};
 CHECK(dsr_actions_prepare_job(&j,DSR_OK,false)&&j.kind==DSR_JOB_PROBE);
 const unsigned char *probe_bytes=(const unsigned char *)&j.cover;
 CHECK(probe_bytes[0]==0&&probe_bytes[1]==0);names_before=metadata_reads;
 CHECK(dsr_worker_submit(&j)==DSR_OK);execute();e=complete(j.id);
 CHECK(e.result==DSR_OK&&metadata_reads==names_before+1);CHECK(dsr_actions_cancel(12345,run_id));CHECK(!dsr_actions_target());probe_close=false;idle_admission();
 printf("PASS: %u actual actor/worker checks; typed Light/RGB/TRV guards, cancel before POST, rebind, uncertain no-retry, nested STOP and exact ACK/leases; HTTP transaction models, no radio\n",checks);return 0;
}

/* Separate PR50 runtime revoke provider; its actual panel/NVS path is covered
 * by tools/test_pairing.py. Actor tests never request this job. */
esp_err_t dsr_panel_pairing_revoke(void){return ESP_OK;}

dsr_result_t dsr_http_rgb(dsr_connection_t *c,dsr_rgb_intent_t intent){CHECK(intent.command==DSR_RGB_SET_COLOR&&intent.rgb[0]==10&&intent.rgb[1]==120&&intent.rgb[2]==250);return typed_transaction(c);}

dsr_result_t dsr_http_refresh_info(dsr_connection_t *c,dsr_device_info_t *i){CHECK(!expect_telemetry_only);metadata_reads++;if(i)memset(i,0,sizeof(*i));return dsr_http_refresh(c);}
dsr_result_t dsr_http_probe_info(const dsr_connection_t *c,dsr_device_t *d,dsr_telemetry_t *t,dsr_device_info_t *i){if(i)memset(i,0,sizeof(*i));return dsr_http_probe_telemetry(c,d,t);}
esp_err_t dsr_connection_update_name(const dsr_connection_t *c){(void)c;return ESP_OK;}
