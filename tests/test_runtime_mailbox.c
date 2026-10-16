/* Actual workspace runtime/mailbox functions. RTOS, Flash and transport are
 * explicit MODEL providers; no real Shelly, radio, TLS or storage proof. */
#include "dsr_workspace.h"
#include "dsr_activity.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static unsigned flash_calls,queue_sends;
static int lock,queue;
#define CHECK(value) do{checks++;if(!(value)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#value);abort();}}while(0)
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &lock;}
int xSemaphoreTake(SemaphoreHandle_t handle,TickType_t wait)
{(void)wait;CHECK(handle==&lock&&!lock);lock=1;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t handle)
{CHECK(handle==&lock&&lock);lock=0;return pdTRUE;}
QueueHandle_t xQueueCreate(unsigned count,unsigned bytes)
{CHECK(count==1&&bytes>0);return &queue;}
int xQueueSend(QueueHandle_t handle,const void *value,TickType_t wait)
{(void)value;(void)wait;CHECK(handle==&queue);queue_sends++;return pdTRUE;}
int xQueueReceive(QueueHandle_t handle,void *value,TickType_t wait)
{(void)handle;(void)value;(void)wait;abort();}
int xTaskCreate(void(*callback)(void *),const char *name,unsigned stack,void *argument,unsigned priority,TaskHandle_t *out)
{CHECK(callback&&!strcmp(name,"dsr_config")&&stack==8192&&!argument&&priority==3&&!out);return pdPASS;}
esp_err_t dsr_storage_mount(void)
{flash_calls++;dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);return ESP_OK;}
char *dsr_config_store_load(const char *directory,bool *found,char error[96])
{flash_calls++;CHECK(!strcmp(directory,"/dsr"));*found=true;error[0]=0;char *json=dsr_config_default();cJSON *root=cJSON_Parse(json);free(json);
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(root,"devices"),cJSON_Parse("{\"id\":42,\"name\":\"Published B\",\"kind\":\"relay\",\"host\":\"192.0.2.11\",\"channel\":1,\"room\":0}"));char *out=cJSON_PrintUnformatted(root);cJSON_Delete(root);return out;}
bool dsr_config_store_save(const char *directory,const char *json,char error[96])
{flash_calls++;(void)error;CHECK(!strcmp(directory,"/dsr")&&json);return true;}
bool dsr_config_store_collect_photos(const char *directory,unsigned *removed,char error[96])
{flash_calls++;CHECK(!strcmp(directory,"/dsr"));*removed=0;error[0]=0;return true;}
void dsr_photos_load(void){flash_calls++;}
bool dsr_connection_valid(const dsr_connection_t *connection,bool bound)
{(void)bound;return connection!=NULL;}

static dsr_runtime_device_t device(const char *id,unsigned channel)
{
 dsr_runtime_device_t out={.device={.generation=DSR_GEN2_PLUS,.online=true,.has_switch=true},.has_sample_time=true,.sample_age_ms=100};
 snprintf(out.device.device_id,sizeof(out.device.device_id),"%s",id);
 snprintf(out.device.host,sizeof(out.device.host),"192.0.2.%u",channel+10);
 snprintf(out.device.name,sizeof(out.device.name),"Fixture %s",id);out.device.channel=(uint8_t)channel;return out;
}
static void publish(const dsr_runtime_device_t *devices,size_t count)
{CHECK(dsr_workspace_runtime_publish_begin(count));for(size_t i=0;i<count;i++)dsr_workspace_runtime_publish_at(i,count,&devices[i]);dsr_workspace_runtime_publish_end();}
static void admission_open(void)
{CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void take_with_outer(dsr_output_request_t *out)
{
 CHECK(dsr_activity_begin());CHECK(dsr_workspace_take_output(out));
 CHECK(!dsr_activity_restart_reserve()); /* outer UI poll covers the handoff */
 dsr_activity_end();
}
static void cover_mailboxes(void)
{
 dsr_runtime_device_t endpoints[2]={device("shared-physical",0),device("shared-physical",0)};
 endpoints[1].device.endpoint_kind=DSR_ENDPOINT_COVER;endpoints[1].device.has_switch=false;endpoints[1].can_stop=true;endpoints[1].telemetry.cover=(dsr_cover_status_t){.state=DSR_COVER_STATE_STOPPED,.flags=DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,.current_pos=38};publish(endpoints,2);
 uint32_t move=0,stop=0;dsr_output_request_t request;
 CHECK(dsr_workspace_output_request(&endpoints[1].device,true,&move)==DSR_UNSUPPORTED);
 CHECK(dsr_workspace_cover_request(&endpoints[0].device,(dsr_cover_intent_t){.command=DSR_COVER_OPEN},&move)==DSR_UNSUPPORTED);
 CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=67},&move)==DSR_OK);
 CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_CLOSE},&stop)==DSR_BUSY);
 CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_STOP},&stop)==DSR_OK&&stop>move);
 CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_STOP},&move)==DSR_BUSY);
 take_with_outer(&request);CHECK(request.device.endpoint_kind==DSR_ENDPOINT_COVER&&request.cover.command==DSR_COVER_STOP&&request.operation_id==stop&&!dsr_workspace_take_output(&request));admission_open();
 /* Already in-flight movement may still receive STOP with unknown position,
  * calibration/error flags or stale sample; identity/auth remain mandatory. */
 endpoints[1].pending=true;endpoints[1].device.online=false;endpoints[1].sample_age_ms=16000;endpoints[1].telemetry.cover=(dsr_cover_status_t){.state=DSR_COVER_STATE_CALIBRATING,.errors=128};publish(endpoints,2);
 CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_STOP},&stop)==DSR_OK);take_with_outer(&request);CHECK(request.operation_id==stop);admission_open();
 for(unsigned phase=0;phase<3;phase++){
  endpoints[1].pending=false;endpoints[1].error=phase==0?DSR_AUTH_REQUIRED:phase==1?DSR_IDENTITY_MISMATCH:DSR_RATE_LIMITED;publish(endpoints,2);
  CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_STOP},&stop)==endpoints[1].error);admission_open();
 }
 endpoints[1].error=DSR_OK;endpoints[1].can_stop=false;publish(endpoints,2);CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_STOP},&stop)==DSR_OFFLINE);
 endpoints[1].device.online=true;endpoints[1].sample_age_ms=0;endpoints[1].pending=false;publish(endpoints,2);CHECK(dsr_workspace_cover_request(&endpoints[1].device,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=67},&move)==DSR_UNSUPPORTED);admission_open();
}
static void test_ap_handoff(void)
{
 _Static_assert(DSR_AP_HANDOFF_UNAVAILABLE==0&&DSR_AP_HANDOFF_READY==1&&DSR_AP_HANDOFF_REQUESTED==2&&DSR_AP_HANDOFF_ACTIVE==3&&DSR_AP_HANDOFF_EXPIRED==4,"stable handoff states");
 unsigned flashes=flash_calls,sends=queue_sends;
 char *before=dsr_workspace_snapshot();uint32_t previous_revision=dsr_workspace_revision();
 CHECK(before&&dsr_workspace_ap_handoff(100)==DSR_AP_HANDOFF_UNAVAILABLE);
 CHECK(!dsr_workspace_request_ap_open(100)&&!dsr_workspace_take_ap_open(100));admission_open();
 dsr_workspace_ap_publish(true,false,100);CHECK(dsr_workspace_ap_handoff(100)==DSR_AP_HANDOFF_READY);
 CHECK(dsr_workspace_request_ap_open(100));admission_open(); /* Pending owns no lease. */
 CHECK(dsr_workspace_ap_handoff(101)==DSR_AP_HANDOFF_REQUESTED);
 CHECK(!dsr_workspace_request_ap_open(101));admission_open();
 /* An ordinary UI publication while its worker is BUSY preserves the request
  * and its original deadline. Polling must not silently extend the TTL. */
 dsr_workspace_ap_publish(true,false,10099);CHECK(dsr_workspace_ap_handoff(10099)==DSR_AP_HANDOFF_REQUESTED);
 CHECK(dsr_workspace_take_ap_open(10099));admission_open();
 CHECK(dsr_workspace_ap_handoff(10100)==DSR_AP_HANDOFF_ACTIVE);
 CHECK(!dsr_workspace_take_ap_open(10100)&&!dsr_workspace_request_ap_open(10100));admission_open();
 dsr_workspace_ap_publish(true,false,10200);CHECK(dsr_workspace_ap_handoff(10200)==DSR_AP_HANDOFF_READY);

 /* Another task can sample the same monotonic clock before the request but
  * acquire the workspace mutex after it. Older GET/publish/take timestamps
  * must not interpret the unsigned subtraction as almost a full wrap. */
 CHECK(dsr_workspace_request_ap_open(101));
 CHECK(dsr_workspace_ap_handoff(100)==DSR_AP_HANDOFF_REQUESTED);
 dsr_workspace_ap_publish(true,false,100);CHECK(dsr_workspace_ap_handoff(101)==DSR_AP_HANDOFF_REQUESTED);
 CHECK(dsr_workspace_take_ap_open(100));CHECK(dsr_workspace_ap_handoff(102)==DSR_AP_HANDOFF_ACTIVE);admission_open();
 dsr_workspace_ap_publish(true,false,103);

 CHECK(dsr_workspace_request_ap_open(20000));
 CHECK(dsr_workspace_ap_handoff(30000)==DSR_AP_HANDOFF_EXPIRED);
 CHECK(!dsr_workspace_take_ap_open(30000));admission_open();
 dsr_workspace_ap_publish(true,false,30001);CHECK(dsr_workspace_ap_handoff(30001)==DSR_AP_HANDOFF_EXPIRED);
 CHECK(dsr_workspace_request_ap_open(30001));CHECK(dsr_workspace_ap_handoff(30001)==DSR_AP_HANDOFF_REQUESTED);
 CHECK(dsr_workspace_request_ap_open(40001)); /* Retry can observe expiry itself. */
 dsr_workspace_ap_publish(false,false,40002);CHECK(dsr_workspace_ap_handoff(40002)==DSR_AP_HANDOFF_UNAVAILABLE);
 CHECK(!dsr_workspace_take_ap_open(40002)&&!dsr_workspace_request_ap_open(40002));admission_open();
 dsr_workspace_ap_publish(true,false,40003);CHECK(dsr_workspace_ap_handoff(40003)==DSR_AP_HANDOFF_READY);
 CHECK(!dsr_workspace_take_ap_open(40003)); /* Unavailable consumed old intent. */

 CHECK(dsr_workspace_request_ap_open(40000));dsr_workspace_ap_publish(true,true,40001);
 CHECK(dsr_workspace_ap_handoff(40001)==DSR_AP_HANDOFF_ACTIVE&&!dsr_workspace_take_ap_open(40001));
 dsr_workspace_ap_publish(false,true,40002);CHECK(dsr_workspace_ap_handoff(40002)==DSR_AP_HANDOFF_ACTIVE);
 dsr_workspace_ap_publish(false,false,40003);CHECK(dsr_workspace_ap_handoff(40003)==DSR_AP_HANDOFF_UNAVAILABLE);
 dsr_workspace_ap_publish(true,false,40004);CHECK(!dsr_workspace_take_ap_open(40004));admission_open();

 /* Zero is a real timestamp; elapsed arithmetic also crosses UINT32_MAX. */
 CHECK(dsr_workspace_request_ap_open(0));CHECK(dsr_workspace_take_ap_open(9999));
 dsr_workspace_ap_publish(true,false,10000);
 uint32_t near_wrap=UINT32_MAX-5000u;CHECK(dsr_workspace_request_ap_open(near_wrap));
 CHECK(dsr_workspace_ap_handoff(near_wrap+9999u)==DSR_AP_HANDOFF_REQUESTED);
 CHECK(!dsr_workspace_take_ap_open(near_wrap+10000u));
 CHECK(dsr_workspace_ap_handoff(near_wrap+10000u)==DSR_AP_HANDOFF_EXPIRED);admission_open();
 CHECK(dsr_workspace_request_ap_open(near_wrap));CHECK(dsr_workspace_take_ap_open(near_wrap+9999u));
 dsr_workspace_ap_publish(true,false,5000);

 /* Restart closes admission without changing an existing intent or creating
  * a lease. Cancelling that reservation permits the eligible UI to consume. */
 CHECK(dsr_activity_restart_reserve());CHECK(!dsr_workspace_request_ap_open(6000));
 CHECK(dsr_workspace_ap_handoff(6000)==DSR_AP_HANDOFF_READY);dsr_activity_restart_cancel();admission_open();
 CHECK(dsr_workspace_request_ap_open(7000));CHECK(dsr_activity_restart_reserve());
 CHECK(!dsr_workspace_take_ap_open(7001)&&!dsr_workspace_request_ap_open(7001));
 CHECK(dsr_workspace_ap_handoff(7001)==DSR_AP_HANDOFF_REQUESTED);dsr_activity_restart_cancel();
 CHECK(dsr_workspace_take_ap_open(7002));admission_open();
 dsr_workspace_ap_publish(true,false,7003);
 CHECK(dsr_activity_begin());CHECK(dsr_workspace_request_ap_open(8000));CHECK(!dsr_activity_restart_reserve());
 CHECK(dsr_workspace_take_ap_open(8001));CHECK(!dsr_activity_restart_reserve());dsr_activity_end();admission_open();
 dsr_workspace_ap_publish(true,false,8002);

 /* AP messages have no payload. They neither overwrite a credential-bearing
  * binding mailbox nor submit any configuration/flash operation. */
 dsr_connection_t binding={.credentials={.username="fixture-owner",.password="private-binding-value"}};
 strcpy(binding.device.device_id,"shelly-isolated");CHECK(dsr_workspace_bind_with_room(&binding,17));
 CHECK(dsr_workspace_request_ap_open(9000)&&dsr_workspace_take_ap_open(9001));CHECK(!dsr_activity_restart_reserve());
 dsr_connection_t taken;uint32_t room=0;CHECK(dsr_workspace_take_bind_with_room(&taken,&room));
 CHECK(room==17&&!strcmp(taken.device.device_id,binding.device.device_id)&&!strcmp(taken.credentials.username,binding.credentials.username)&&!strcmp(taken.credentials.password,binding.credentials.password));dsr_workspace_bind_result(DSR_OK);admission_open();
 memset(&taken,0,sizeof(taken));memset(&binding,0,sizeof(binding));dsr_workspace_ap_publish(true,false,9002);
 char *after=dsr_workspace_snapshot();CHECK(after&&!strcmp(before,after));free(before);free(after);
 CHECK(dsr_workspace_revision()==previous_revision&&flash_calls==flashes&&queue_sends==sends);admission_open();
}
static unsigned mapping_allocations,mapping_fail;
static void *mapping_alloc(size_t n){if(++mapping_allocations==mapping_fail)return NULL;return malloc(n);}
static void test_mapping_guard(void)
{
 dsr_connection_t c={.device={.host="192.0.2.11",.channel=1},.layout_id=42};CHECK(dsr_workspace_mapping_matches(&c));
 c.layout_id=41;CHECK(!dsr_workspace_mapping_matches(&c));c.layout_id=42;c.device.channel=0;CHECK(!dsr_workspace_mapping_matches(&c));c.device.channel=1;strcpy(c.device.host,"192.0.2.12");CHECK(!dsr_workspace_mapping_matches(&c));strcpy(c.device.host,"192.0.2.11");
 CHECK(!dsr_workspace_mapping_matches(NULL));c.layout_id=0;CHECK(!dsr_workspace_mapping_matches(&c));c.layout_id=42;
 cJSON_Hooks hooks={.malloc_fn=mapping_alloc,.free_fn=free};mapping_allocations=mapping_fail=0;cJSON_InitHooks(&hooks);CHECK(dsr_workspace_mapping_matches(&c));unsigned count=mapping_allocations;
 for(unsigned i=1;i<=count;i++){mapping_allocations=0;mapping_fail=i;CHECK(!dsr_workspace_mapping_matches(&c));}cJSON_InitHooks(NULL);CHECK(dsr_workspace_mapping_matches(&c));
}
int main(void)
{
 dsr_runtime_device_t snapshot[DSR_MAX_DEVICES];dsr_output_request_t request;uint32_t operation=123;
 CHECK(dsr_workspace_runtime_snapshot(snapshot,DSR_MAX_DEVICES)==0);
 dsr_runtime_device_t devices[2]={device("shelly-A",0),device("shelly-B",1)};
 CHECK(dsr_workspace_output_request(&devices[0].device,true,&operation)==DSR_INVALID&&operation==123);admission_open();
 CHECK(dsr_workspace_ap_handoff(0)==DSR_AP_HANDOFF_UNAVAILABLE);
 dsr_workspace_ap_publish(true,true,0);CHECK(dsr_workspace_ap_handoff(0)==DSR_AP_HANDOFF_UNAVAILABLE);
 CHECK(!dsr_workspace_request_ap_open(0)&&!dsr_workspace_take_ap_open(0)&&flash_calls==0&&queue_sends==0);admission_open();
 CHECK(dsr_workspace_start()==ESP_OK);test_mapping_guard();test_ap_handoff();publish(devices,2);
 CHECK(dsr_workspace_runtime_snapshot(snapshot,DSR_MAX_DEVICES)==2&&!strcmp(snapshot[0].device.device_id,"shelly-A")&&!strcmp(snapshot[1].device.device_id,"shelly-B"));

 uint32_t a=0,b=0;CHECK(dsr_workspace_output_request(&devices[0].device,true,&a)==DSR_OK&&a>0);
 CHECK(dsr_workspace_output_request(&devices[0].device,false,&operation)==DSR_BUSY);
 CHECK(dsr_workspace_output_request(&devices[1].device,false,&b)==DSR_OK&&b>a);
 CHECK(dsr_workspace_runtime_snapshot(snapshot,DSR_MAX_DEVICES)==2&&snapshot[0].pending&&snapshot[1].pending&&snapshot[0].operation_id==a&&snapshot[1].operation_id==b);
 CHECK(!dsr_activity_restart_reserve());take_with_outer(&request);
 CHECK(request.operation_id==a&&request.on&&!strcmp(request.device.device_id,"shelly-A"));
 CHECK(!dsr_activity_restart_reserve()); /* B keeps its own mailbox lease */

 /* B was admitted at old mailbox index 1. A then disappears and B compacts
  * to runtime index 0 before the UI consumer takes its accepted command. */
 publish(&devices[1],1);
 CHECK(dsr_workspace_runtime_snapshot(snapshot,DSR_MAX_DEVICES)==1&&!strcmp(snapshot[0].device.device_id,"shelly-B")&&snapshot[0].pending&&snapshot[0].operation_id==b);
 CHECK(dsr_workspace_output_request(&devices[1].device,true,&operation)==DSR_BUSY);
 CHECK(dsr_workspace_output_request(&devices[0].device,true,&operation)==DSR_INVALID);
 take_with_outer(&request);CHECK(request.operation_id==b&&!request.on&&!strcmp(request.device.device_id,"shelly-B"));admission_open();
 CHECK(!dsr_workspace_take_output(&request));

 /* Empty publication must remove the last runtime entry. A mailbox admitted
  * just before removal remains drainable by identity, with no leaked lease. */
 publish(&devices[1],1);CHECK(dsr_workspace_output_request(&devices[1].device,true,&b)==DSR_OK);
 publish(NULL,0);CHECK(dsr_workspace_runtime_snapshot(snapshot,DSR_MAX_DEVICES)==0);
 CHECK(dsr_workspace_output_request(&devices[1].device,false,&operation)==DSR_INVALID);
 take_with_outer(&request);CHECK(request.operation_id==b&&request.on&&!strcmp(request.device.device_id,"shelly-B"));admission_open();

 /* HTTP supplies only the stable tuple; its host cannot redirect a command. */
 publish(devices,2);dsr_device_t tuple=devices[1].device;strcpy(tuple.host,"203.0.113.99");
 CHECK(dsr_workspace_output_request(&tuple,true,&b)==DSR_OK);take_with_outer(&request);
 CHECK(!strcmp(request.device.host,devices[1].device.host)&&strcmp(request.device.host,tuple.host));admission_open();
 publish(devices,2);strcpy(tuple.device_id,"shelly-foreign");CHECK(dsr_workspace_output_request(&tuple,true,&operation)==DSR_INVALID);admission_open();
 tuple=devices[0].device;tuple.generation=DSR_GEN1;CHECK(dsr_workspace_output_request(&tuple,true,&operation)==DSR_INVALID);
 tuple=devices[0].device;tuple.channel=15;CHECK(dsr_workspace_output_request(&tuple,true,&operation)==DSR_INVALID);admission_open();

 dsr_runtime_device_t unavailable=devices[0];unavailable.device.online=false;publish(&unavailable,1);CHECK(dsr_workspace_output_request(&unavailable.device,true,&operation)==DSR_OFFLINE);
 unavailable=devices[0];unavailable.has_sample_time=false;publish(&unavailable,1);CHECK(dsr_workspace_output_request(&unavailable.device,true,&operation)==DSR_OFFLINE);
 unavailable=devices[0];unavailable.sample_age_ms=15001;publish(&unavailable,1);CHECK(dsr_workspace_output_request(&unavailable.device,true,&operation)==DSR_OFFLINE);
 unavailable=devices[0];unavailable.device.has_switch=false;publish(&unavailable,1);CHECK(dsr_workspace_output_request(&unavailable.device,true,&operation)==DSR_UNSUPPORTED);
 const dsr_result_t failures[]={DSR_AUTH_REQUIRED,DSR_IDENTITY_MISMATCH,DSR_RATE_LIMITED};
 for(size_t i=0;i<sizeof(failures)/sizeof(failures[0]);i++){unavailable=devices[0];unavailable.error=failures[i];publish(&unavailable,1);CHECK(dsr_workspace_output_request(&unavailable.device,true,&operation)==failures[i]);}
 admission_open();
 CHECK(!dsr_workspace_runtime_publish_begin(DSR_MAX_DEVICES+1));
 publish(devices,2);CHECK(dsr_workspace_runtime_snapshot(snapshot,1)==1&&!strcmp(snapshot[0].device.device_id,"shelly-A"));
 CHECK(dsr_workspace_output_request(NULL,true,&operation)==DSR_INVALID);
 CHECK(dsr_workspace_output_request(&devices[0].device,true,NULL)==DSR_INVALID);admission_open();
 CHECK(dsr_activity_restart_reserve());CHECK(dsr_workspace_output_request(&devices[0].device,true,&operation)==DSR_BUSY);dsr_activity_restart_cancel();admission_open();
 cover_mailboxes();
 /* Same physical ID/channel is still two independent transport families. */
 dsr_runtime_device_t light[2]={device("shared-light",0),device("shared-light",0)};light[1].device.endpoint_kind=DSR_ENDPOINT_LIGHT;light[1].device.has_switch=false;light[1].telemetry.light=(dsr_light_status_t){.brightness=0,.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS};publish(light,2);
 CHECK(dsr_workspace_output_request(&light[1].device,true,&operation)==DSR_UNSUPPORTED);CHECK(dsr_workspace_light_request(&light[0].device,(dsr_light_intent_t){.command=DSR_LIGHT_ON},&operation)==DSR_UNSUPPORTED);
 CHECK(dsr_workspace_light_request(&light[1].device,(dsr_light_intent_t){.command=DSR_LIGHT_SET_BRIGHTNESS,.brightness=0},&operation)==DSR_OK);CHECK(dsr_workspace_light_request(&light[1].device,(dsr_light_intent_t){.command=DSR_LIGHT_OFF},&operation)==DSR_BUSY);take_with_outer(&request);CHECK(request.device.endpoint_kind==DSR_ENDPOINT_LIGHT&&request.light.command==DSR_LIGHT_SET_BRIGHTNESS&&request.light.brightness==0&&request.operation_id==operation);admission_open();
 for(unsigned condition=0;condition<4;condition++){light[1].pending=false;light[1].device.online=condition!=0;light[1].sample_age_ms=condition==1?15001:0;light[1].error=condition==2?DSR_AUTH_REQUIRED:DSR_OK;light[1].telemetry.light.conditions=condition==3?DSR_LIGHT_UNCALIBRATED:0;publish(light,2);CHECK(dsr_workspace_light_request(&light[1].device,(dsr_light_intent_t){.command=DSR_LIGHT_ON},&operation)==(condition<2?DSR_OFFLINE:condition==2?DSR_AUTH_REQUIRED:DSR_UNSUPPORTED));admission_open();}
 printf("PASS: %u actual workspace/runtime checks; AP handoff TTL/wrap/restart, identity mailboxes, compaction, count0 and per-endpoint admission; RTOS/Flash/HTTP MODEL\n",checks);
 return 0;
}

/* No actor in this isolated workspace fixture. */
bool dsr_actions_reserved(const dsr_device_t *device){(void)device;return false;}
void dsr_actions_mode_block(void){}
