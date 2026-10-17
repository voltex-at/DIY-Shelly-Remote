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
static unsigned checks,gets,posts,stops,mode;static uint32_t now=120000,run_id;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
typedef struct {size_t size;bool full;unsigned char bytes[sizeof(dsr_event_t)];} queue_t;
static queue_t queues[2];static unsigned queue_count,lock_count;static bool locks[2],available;
static void(*worker)(void *);static jmp_buf idle;
static bool(*urgent)(void);static dsr_http_job_guard_t guard;static dsr_http_job_write_hook_t write_hook;
static bool mapping=true;static dsr_connection_t a,b;static uint32_t outer_id;
QueueHandle_t xQueueCreate(unsigned n,unsigned size){CHECK(n==1&&queue_count<2&&size<=sizeof(queues[0].bytes));queue_t *q=&queues[queue_count++];q->size=size;return q;}
int xQueueSend(QueueHandle_t h,const void *p,TickType_t wait){(void)wait;queue_t *q=h;CHECK(!q->full);memcpy(q->bytes,p,q->size);q->full=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t h,void *p,TickType_t wait){queue_t *q=h;if(!q->full){if(wait)longjmp(idle,1);return pdFALSE;}memcpy(p,q->bytes,q->size);q->full=false;return pdTRUE;}
void vQueueDelete(QueueHandle_t h){(void)h;}
SemaphoreHandle_t xSemaphoreCreateMutex(void){CHECK(lock_count<2);return &locks[lock_count++];}
SemaphoreHandle_t xSemaphoreCreateBinary(void){return &available;}
int xSemaphoreTake(SemaphoreHandle_t h,TickType_t wait){(void)wait;if(h==&available){if(!available)return pdFALSE;available=false;return pdTRUE;}bool *lock=h;CHECK((lock==&locks[0]||lock==&locks[1])&&!*lock);*lock=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t h){bool *lock=h;if(h!=&available)CHECK((lock==&locks[0]||lock==&locks[1])&&*lock);*lock=h==&available;return pdTRUE;}
void vSemaphoreDelete(SemaphoreHandle_t h){(void)h;}
int xTaskCreate(void(*f)(void *),const char *name,unsigned stack,void *arg,unsigned pri,TaskHandle_t *out){CHECK(!strcmp(name,"dsr_network")&&stack==10240&&!arg&&pri==4&&!out);worker=f;return pdPASS;}
void *heap_caps_calloc(size_t n,size_t s,unsigned caps){CHECK(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return calloc(n,s);}
int64_t esp_timer_get_time(void){return (int64_t)now*1000;}
uint32_t esp_random(void){return 12345;}
bool dsr_wifi_connected(void){return true;}
bool dsr_wifi_temporary_active(void){return false;}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){return c&&dsr_ipv4_valid(c->device.host)&&(!bound||c->device.device_id[0]);}
bool dsr_workspace_mapping_matches(const dsr_connection_t *c){return mapping&&c->layout_id>0;}
char *dsr_workspace_snapshot(void){return strdup("{\"schema\":2,\"revision\":7,\"language\":\"pl\",\"theme\":\"graphite\",\"accent\":5020661,\"active_dashboard\":1,\"rooms\":[],\"devices\":[{\"id\":42,\"name\":\"Switch\",\"host\":\"192.0.2.42\",\"channel\":0,\"room\":0,\"kind\":\"relay\"},{\"id\":43,\"name\":\"Cover\",\"host\":\"192.0.2.43\",\"channel\":0,\"room\":0,\"kind\":\"cover\"}],\"groups\":[],\"scenes\":[{\"id\":1,\"name\":\"Switch\",\"actions\":[{\"device\":42,\"command\":\"on\"},{\"device\":42,\"command\":\"off\"}]},{\"id\":2,\"name\":\"Cover\",\"actions\":[{\"device\":43,\"command\":\"open\"}]},{\"id\":3,\"name\":\"Stop\",\"actions\":[{\"device\":43,\"command\":\"stop\"}]}],\"dashboards\":[{\"id\":1,\"name\":\"Home\",\"tiles\":[]}]}");}
void dsr_http_urgent_hook(bool(*f)(void)){urgent=f;}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t f){dsr_http_job_guard_t old=guard;guard=f;return old;}
dsr_http_job_write_hook_t dsr_http_job_write_hook(dsr_http_job_write_hook_t hook){dsr_http_job_write_hook_t previous=write_hook;write_hook=hook;return previous;}
dsr_result_t dsr_http_set_output(dsr_connection_t *c,bool on)
{
 CHECK(guard&&c->device.endpoint_kind==DSR_ENDPOINT_SWITCH);dsr_result_t result=guard();if(result!=DSR_OK)return result;gets++;
 if(mode==1)CHECK(dsr_actions_cancel(12345,run_id));
 if(mode==2){dsr_http_job_guard_t outer=guard;dsr_http_job_write_hook_t outer_write=write_hook;CHECK(dsr_worker_stop_request(&b,outer_id)==DSR_OK);CHECK(urgent&&urgent());CHECK(guard==outer&&write_hook==outer_write);}
 result=guard();if(result!=DSR_OK)return result;CHECK(write_hook);write_hook();posts++;c->device.output=on;c->device.online=true;return DSR_OK;
}
dsr_result_t dsr_http_cover(dsr_connection_t *c,dsr_cover_intent_t intent)
{
 if(intent.command==DSR_COVER_STOP){/* Manual innerSTOP must suspend outer actor cancellation. */if(mode==2)CHECK(!guard&&!write_hook);else if(guard){dsr_result_t result=guard();if(result!=DSR_OK)return result;}if(write_hook)write_hook();stops++;c->device.online=true;return DSR_OK;}
 CHECK(guard);dsr_result_t result=guard();if(result!=DSR_OK)return result;gets++;result=guard();if(result==DSR_OK){CHECK(write_hook);write_hook();posts++;}return result;
}
/* Light is unsupported by the action model and must never reach this provider. */
dsr_result_t dsr_http_light(dsr_connection_t *c,dsr_light_intent_t intent){(void)c;(void)intent;abort();}
dsr_result_t dsr_http_refresh_info(dsr_connection_t *c,dsr_device_info_t *info){(void)info;CHECK(guard);dsr_result_t result=guard();if(result==DSR_OK){gets++;c->device.online=true;}return result;}
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
void dsr_history_service(void){abort();}
void dsr_history_clock_execute(void){abort();}
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
 mode=1;begin(1,1);dsr_job_t j=prepare(101,&a);CHECK(dsr_worker_submit(&j)==DSR_OK);execute();CHECK(gets==1&&!posts);dsr_event_t e=complete(101);CHECK(e.result==DSR_BUSY&&!dsr_actions_target());idle_admission();
 /* Manual STOP uses the SAME numeric ID as outer actor job. It still
  * cancels the plan and its inner transaction suspends/restores outerguard. */
 mode=2;begin(1,2);j=prepare(102,&a);outer_id=102;CHECK(dsr_worker_submit(&j)==DSR_OK);execute();CHECK(gets==2&&!posts&&stops==1&&dsr_worker_stop_pending());e=complete(102);CHECK(e.result==DSR_BUSY&&!dsr_actions_target());CHECK(!dsr_activity_restart_reserve());dsr_stop_result_t stopped;CHECK(dsr_worker_stop_receive(&stopped,false)&&stopped.operation_id==102&&stopped.device.endpoint_kind==DSR_ENDPOINT_COVER);idle_admission();
 /* Cancel/deadline/rebind before queue take: zero first SDK request. */
 mode=0;for(unsigned fault=0;fault<3;fault++){begin(2,3+fault);j=prepare(103+fault,&b);CHECK(dsr_worker_submit(&j)==DSR_OK);unsigned before=gets+posts;if(fault==0)CHECK(dsr_actions_cancel(12345,run_id));else if(fault==1)now+=180000;else mapping=false;execute();e=complete(j.id);CHECK(gets+posts==before&&e.result==(fault==2?DSR_IDENTITY_MISMATCH:DSR_BUSY));mapping=true;(void)dsr_actions_cancel(12345,run_id);CHECK(!dsr_actions_target());idle_admission();}
 /* Tombstone also protects an actor Cover admitted before manual STOP. */
 begin(2,6);j=prepare(106,&b);CHECK(dsr_worker_submit(&j)==DSR_OK);CHECK(dsr_worker_stop_request(&b,106)==DSR_OK);unsigned before=posts;execute();e=complete(106);CHECK(e.result==DSR_BUSY&&posts==before&&!dsr_actions_target());CHECK(dsr_worker_stop_receive(&stopped,false)&&stopped.operation_id==106&&stops==2);idle_admission();
 /* An explicit scene STOP has separate trusted admission and its own guard;
  * it is not mistaken for an unrelated manual operationID or auto-cancelled. */
 begin(3,7);j=prepare(107,&b);CHECK(j.cover.command==DSR_COVER_STOP&&dsr_worker_action_stop_request(&b,107)==DSR_OK);execute();CHECK(dsr_worker_stop_receive(&stopped,false)&&stopped.operation_id==107&&stopped.result==DSR_OK&&stops==3);CHECK(dsr_activity_begin());dsr_actions_complete(107,stopped.result);CHECK(!dsr_actions_target());dsr_activity_end();idle_admission();
 printf("PASS: %u actual actor/worker checks; registered guard/cancel/deadline/rebind, nested manualSTOP same-number IDs, queuedMOVE tombstone, actorSTOP, exact ACK/leases; HTTP transaction models, no radio\n",checks);return 0;
}

/* Separate PR50 runtime revoke provider; its actual panel/NVS path is covered
 * by tools/test_pairing.py. Actor tests never request this job. */
esp_err_t dsr_panel_pairing_revoke(void){return ESP_OK;}

dsr_result_t dsr_http_rgb(dsr_connection_t *c,dsr_rgb_intent_t intent){(void)c;(void)intent;return DSR_INVALID;}
