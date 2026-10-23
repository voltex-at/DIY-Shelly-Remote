#include "dsr_workspace.h"
#include "dsr_storage.h"
#include "dsr_activity.h"
#include "dsr_actions.h"
#include "dsr_automation_store.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SemaphoreHandle_t mutex;
static QueueHandle_t queue;
static char *current;
static uint32_t revision;
static bool busy;
static char status[96];
static bool auto_pending,auto_done;
static uint32_t auto_generation;
static esp_err_t auto_result;
static dsr_connection_t bind_request;
static bool bind_waiting,bind_busy;
static uint32_t bind_room=UINT32_MAX;
static dsr_result_t bind_result_code=DSR_OK;
static char bind_message[64];
typedef struct {union {char *json;dsr_auto_record_t *record;};uint32_t revision;bool automatic;} request_t;
static bool workspace_ready(void){dsr_config_health_t h;dsr_config_health_snapshot(&h);return h.workspace==DSR_WORKSPACE_READY&&mutex&&queue&&current;}
static void config_task(void *argument)
{
 (void)argument;request_t request;
 for(;;){
  if(xQueueReceive(queue,&request,portMAX_DELAY)!=pdTRUE)continue;
  if(request.automatic){
   esp_err_t result=dsr_auto_store_save(request.record);free(request.record);
   xSemaphoreTake(mutex,portMAX_DELAY);
   auto_generation=request.revision;auto_result=result;auto_pending=false;auto_done=true;
   xSemaphoreGive(mutex);dsr_activity_end();continue;
  }
  char error[96]={0};char *previous=dsr_workspace_snapshot();
  char *next=dsr_config_replace(previous,request.json,request.revision,error);free(previous);free(request.json);
  if(next&&!dsr_config_store_save("/dsr",next,error)){free(next);next=NULL;}
  xSemaphoreTake(mutex,portMAX_DELAY);
  /* replace already validated the exact revision+1 document and save ACKed
   * its readback. Do not allocate/reparse after commit and risk publishing0
   * during low memory while the durable document is actually revision+1. */
  if(next){free(current);current=next;revision=request.revision+1u;snprintf(status,sizeof(status),"Saved revision %lu",(unsigned long)revision);}
  else snprintf(status,sizeof(status),"%s",error[0]?error:"Save failed");
  busy=false;xSemaphoreGive(mutex);
  dsr_activity_end();
 }
}
esp_err_t dsr_workspace_start(void)
{
 if(mutex)return ESP_ERR_INVALID_STATE;
 dsr_config_health_workspace(DSR_WORKSPACE_MOUNTING,ESP_OK);
 esp_err_t err=dsr_storage_mount();if(err!=ESP_OK){dsr_config_health_workspace(DSR_WORKSPACE_MOUNTING,err);return err;}
 dsr_config_health_workspace(DSR_WORKSPACE_LOADING,ESP_OK);
 bool found=false,legacy=false;char error[96]={0};current=dsr_config_store_load("/dsr",&found,error);
 if(!found){if(error[0]){dsr_config_health_workspace(DSR_WORKSPACE_LOADING,ESP_FAIL);return ESP_FAIL;}dsr_config_health_workspace(DSR_WORKSPACE_PROVISIONING,ESP_OK);current=dsr_config_default();if(!current){dsr_config_health_workspace(DSR_WORKSPACE_PROVISIONING,ESP_ERR_NO_MEM);return ESP_ERR_NO_MEM;}
  if(!dsr_config_store_save("/dsr",current,error)){free(current);current=NULL;dsr_config_health_workspace(DSR_WORKSPACE_PROVISIONING,ESP_FAIL);return ESP_FAIL;}}
 if(found){
  /* A separately strict schema1 validator authorizes only this RAM upgrade.
   * Never refresh/GC legacy checkpoints at boot: first explicit Save is the
   * only durable1->2 transition and retains the old revision until its ACK. */
  char *canonical=dsr_config_upgrade(current,&legacy,error);free(current);current=canonical;
  if(!current){dsr_config_health_workspace(DSR_WORKSPACE_LOADING,ESP_FAIL);return ESP_FAIL;}
 }
 /* Refresh the older checkpoint with the already verified boot document.
  * Power loss retains the newest record. Readback must succeed before GC.
  * This also retires old photo references without requiring a second edit. */
 if(found&&!legacy)dsr_config_store_save("/dsr",current,error);
 /* Boot-time GC precedes cache/LVGL construction; immutable live pointers are
  * never freed. Any remaining corrupt checkpoint conservatively skips GC. */
 unsigned removed=0;if(!legacy)dsr_config_store_collect_photos("/dsr",&removed,error);
 dsr_config_health_workspace(DSR_WORKSPACE_ALLOCATING,ESP_OK);
 dsr_photos_load();revision=dsr_config_revision(current);
 if(!revision){free(current);current=NULL;dsr_config_health_workspace(DSR_WORKSPACE_ALLOCATING,ESP_ERR_NO_MEM);return ESP_ERR_NO_MEM;}
 mutex=xSemaphoreCreateMutex();queue=xQueueCreate(1,sizeof(request_t));
 if(!mutex||!queue){dsr_config_health_workspace(DSR_WORKSPACE_ALLOCATING,ESP_ERR_NO_MEM);return ESP_ERR_NO_MEM;}
 dsr_config_health_workspace(DSR_WORKSPACE_TASK,ESP_OK);
 if(xTaskCreate(config_task,"dsr_config",8192,NULL,3,NULL)!=pdPASS){dsr_config_health_workspace(DSR_WORKSPACE_TASK,ESP_ERR_NO_MEM);return ESP_ERR_NO_MEM;}
 dsr_config_health_workspace(DSR_WORKSPACE_READY,ESP_OK);return ESP_OK;
}
dsr_snapshot_result_t dsr_workspace_snapshot_checked(char **out)
{
 if(!out)return DSR_SNAPSHOT_UNAVAILABLE;
 *out=NULL;dsr_config_health_t health;dsr_config_health_snapshot(&health);
 if(health.workspace!=DSR_WORKSPACE_READY||!mutex||!current)return DSR_SNAPSHOT_UNAVAILABLE;
 xSemaphoreTake(mutex,portMAX_DELAY);char *copy=malloc(strlen(current)+1);if(copy)strcpy(copy,current);xSemaphoreGive(mutex);
 if(!copy)return DSR_SNAPSHOT_NO_MEMORY;
 *out=copy;return DSR_SNAPSHOT_OK;
}
char *dsr_workspace_snapshot(void){char *copy=NULL;(void)dsr_workspace_snapshot_checked(&copy);return copy;}
uint32_t dsr_workspace_revision(void)
{if(!workspace_ready())return 0;xSemaphoreTake(mutex,portMAX_DELAY);uint32_t r=revision;xSemaphoreGive(mutex);return r;}
bool dsr_workspace_submit(const char *json,uint32_t expected)
{
 if(!workspace_ready()||!json||strlen(json)>DSR_CONFIG_MAX_JSON)return false;
 char *copy=malloc(strlen(json)+1);if(!copy)return false;strcpy(copy,json);
 xSemaphoreTake(mutex,portMAX_DELAY);
 bool ok=!busy&&revision==expected;
 if(ok){ok=dsr_activity_begin();if(ok){request_t req={.json=copy,.revision=expected};ok=xQueueSend(queue,&req,0)==pdTRUE;if(ok){busy=true;snprintf(status,sizeof(status),"Saving…");}else dsr_activity_end();}}
 xSemaphoreGive(mutex);if(!ok)free(copy);return ok;
}
void dsr_workspace_status(bool *pending,char message[96])
{if(!workspace_ready()){*pending=false;snprintf(message,96,"Configuration storage unavailable");return;}xSemaphoreTake(mutex,portMAX_DELAY);*pending=busy;snprintf(message,96,"%s",status);xSemaphoreGive(mutex);}
bool dsr_workspace_auto_submit(const dsr_auto_record_t *record,uint32_t generation)
{
 if(!workspace_ready()||!record||!generation||record->generation!=generation)return false;
 dsr_auto_record_t *copy=malloc(sizeof(*copy));if(!copy)return false;*copy=*record;
 xSemaphoreTake(mutex,portMAX_DELAY);bool ok=!auto_pending&&!auto_done;
 if(ok){
  ok=dsr_activity_begin();
  if(ok){request_t req={.record=copy,.revision=generation,.automatic=true};
   ok=xQueueSend(queue,&req,0)==pdTRUE;if(ok)auto_pending=true;else dsr_activity_end();}
 }
 xSemaphoreGive(mutex);if(!ok)free(copy);return ok;
}
bool dsr_workspace_auto_take_result(uint32_t *generation,esp_err_t *result)
{
 if(!workspace_ready()||!generation||!result)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool found=auto_done;
 if(found){*generation=auto_generation;*result=auto_result;auto_done=false;}
 xSemaphoreGive(mutex);return found;
}
bool dsr_workspace_bind_with_room(const dsr_connection_t *connection,uint32_t room)
{
 if(!workspace_ready()||!dsr_connection_valid(connection,false))return false;
 xSemaphoreTake(mutex,portMAX_DELAY);
 bool ok=!bind_busy&&dsr_activity_begin();if(ok){bind_request=*connection;bind_room=room;bind_result_code=DSR_BUSY;bind_waiting=bind_busy=true;snprintf(bind_message,sizeof(bind_message),"Checking Shelly identity…");}xSemaphoreGive(mutex);return ok;
}
bool dsr_workspace_bind(const dsr_connection_t *connection){return dsr_workspace_bind_with_room(connection,UINT32_MAX);}
bool dsr_workspace_take_bind_with_room(dsr_connection_t *connection,uint32_t *room)
{
 if(!workspace_ready())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool found=bind_waiting;
 if(found){*connection=bind_request;if(room)*room=bind_room;bind_room=UINT32_MAX;memset(&bind_request,0,sizeof(bind_request));bind_waiting=false;}xSemaphoreGive(mutex);return found;
}
void dsr_workspace_bind_result_stage(dsr_result_t result,dsr_bind_stage_t stage)
{if(!mutex)return;xSemaphoreTake(mutex,portMAX_DELAY);bool completed=bind_busy;bind_busy=false;bind_result_code=result;snprintf(bind_message,sizeof(bind_message),"%s",result==DSR_OK?"Shelly verified and saved":stage==DSR_BIND_PRIVATE?"Workspace saved; private binding unconfirmed; add again":stage==DSR_BIND_WORKSPACE?"Shelly verified; workspace unconfirmed; add again":result==DSR_AUTH_REQUIRED?"Authentication failed; check Shelly credentials":result==DSR_OFFLINE?"Shelly offline / Wi-Fi disconnected":result==DSR_CONFLICT?"Configuration changed; reload and retry adding the device":"Shelly check failed; previous binding retained");xSemaphoreGive(mutex);if(completed)dsr_activity_end();}
void dsr_workspace_bind_result(dsr_result_t result)
{dsr_workspace_bind_result_stage(result,DSR_BIND_VERIFY);}
bool dsr_workspace_mapping_matches(const dsr_connection_t *c)
{
 if(!c||!c->layout_id)return false;
 char *json=NULL;if(dsr_workspace_snapshot_checked(&json)!=DSR_SNAPSHOT_OK)return false;
 cJSON *root=dsr_config_json_depth(json)?cJSON_Parse(json):NULL;free(json);char error[96];bool matches=false;
 if(root&&dsr_config_validate(root,error)){const cJSON *v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(root,"devices")){
  const cJSON *id=cJSON_GetObjectItemCaseSensitive(v,"id"),*host=cJSON_GetObjectItemCaseSensitive(v,"host"),*channel=cJSON_GetObjectItemCaseSensitive(v,"channel"),*kind=cJSON_GetObjectItemCaseSensitive(v,"kind");
  if(!dsr_endpoint_layout_matches(c->device.endpoint_kind,kind->valuestring))continue;
  if(id->valuedouble==c->layout_id&&channel->valuedouble==c->device.channel&&!strcmp(host->valuestring,c->device.host))matches=true;
 }}
 cJSON_Delete(root);return matches;
}
dsr_result_t dsr_workspace_bind_result_code(void)
{if(!mutex)return DSR_IO;xSemaphoreTake(mutex,portMAX_DELAY);dsr_result_t result=bind_result_code;xSemaphoreGive(mutex);return result;}
void dsr_workspace_bind_status(bool *pending,char message[64])
{if(!mutex){*pending=false;message[0]=0;return;}xSemaphoreTake(mutex,portMAX_DELAY);*pending=bind_busy;snprintf(message,64,"%s",bind_message);xSemaphoreGive(mutex);}

/* A single flag and timestamp cross the HTTP/LVGL boundary. The consumer
 * reserves ACTIVE under the same mutex, so a second request cannot race the
 * screen transition. No credentials, flash queue or long-lived lease here. */
#define AP_OPEN_TTL_MS UINT32_C(10000)
static dsr_ap_handoff_t ap_handoff;
static uint32_t ap_requested_ms;
static void ap_expire(uint32_t now_ms)
{
 /* Callers capture the shared clock before taking this mutex; another task
  * may publish a newer request first. A stale sample is not an expired TTL. */
 if(ap_handoff==DSR_AP_HANDOFF_REQUESTED&&(int32_t)(now_ms-ap_requested_ms)>=(int32_t)AP_OPEN_TTL_MS){ap_handoff=DSR_AP_HANDOFF_EXPIRED;ap_requested_ms=0;}
}
void dsr_workspace_ap_publish(bool available,bool active,uint32_t now_ms)
{
 if(!workspace_ready())return;
 xSemaphoreTake(mutex,portMAX_DELAY);
 if(active||!available){ap_handoff=active?DSR_AP_HANDOFF_ACTIVE:DSR_AP_HANDOFF_UNAVAILABLE;ap_requested_ms=0;}
 else if(ap_handoff==DSR_AP_HANDOFF_UNAVAILABLE||ap_handoff==DSR_AP_HANDOFF_ACTIVE)ap_handoff=DSR_AP_HANDOFF_READY;
 else ap_expire(now_ms);
 xSemaphoreGive(mutex);
}
bool dsr_workspace_request_ap_open(uint32_t now_ms)
{
 if(!workspace_ready())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);ap_expire(now_ms);
 bool accepted=(ap_handoff==DSR_AP_HANDOFF_READY||ap_handoff==DSR_AP_HANDOFF_EXPIRED)&&dsr_activity_begin();
 if(accepted){ap_handoff=DSR_AP_HANDOFF_REQUESTED;ap_requested_ms=now_ms;dsr_activity_end();}
 xSemaphoreGive(mutex);return accepted;
}
bool dsr_workspace_take_ap_open(uint32_t now_ms)
{
 if(!workspace_ready())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);ap_expire(now_ms);
 bool accepted=ap_handoff==DSR_AP_HANDOFF_REQUESTED&&dsr_activity_begin();
 if(accepted){ap_handoff=DSR_AP_HANDOFF_ACTIVE;ap_requested_ms=0;dsr_activity_end();}
 xSemaphoreGive(mutex);return accepted;
}
dsr_ap_handoff_t dsr_workspace_ap_handoff(uint32_t now_ms)
{
 if(!workspace_ready())return DSR_AP_HANDOFF_UNAVAILABLE;
 xSemaphoreTake(mutex,portMAX_DELAY);ap_expire(now_ms);dsr_ap_handoff_t value=ap_handoff;xSemaphoreGive(mutex);return value;
}

/* Mutex also protects cross-task command admission; no HTTP thread may touch
 * LVGL or copy credentials. One mailbox per endpoint prevents duplicate SET. */
static dsr_runtime_device_t runtime[DSR_MAX_DEVICES];
#ifdef ESP_PLATFORM
#include "esp_attr.h"
EXT_RAM_BSS_ATTR
#endif
static dsr_device_info_t runtime_info[DSR_MAX_DEVICES];
static size_t runtime_count;
void dsr_workspace_runtime_info_at(size_t index,const dsr_device_info_t *info)
{if(index<runtime_count)runtime_info[index]=info?*info:(dsr_device_info_t){0};}
bool dsr_workspace_runtime_info(uint32_t id,const char *identity,dsr_device_info_t *info)
{
 if(!mutex||!id||!identity||!info)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool found=false;
 for(size_t i=0;i<runtime_count;i++)if(runtime[i].layout_id==id&&!strcmp(runtime[i].device.device_id,identity)){*info=runtime_info[i];found=true;break;}
 xSemaphoreGive(mutex);return found;
}

bool dsr_workspace_runtime_name(uint32_t id,const char *host,uint8_t channel,char out[48])
{
 if(!mutex||!id||!host||!out)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool found=false;
 for(size_t i=0;i<runtime_count;i++)if(runtime[i].layout_id==id&&runtime[i].device.channel==channel&&!strcmp(runtime[i].device.host,host)&&runtime[i].device.name[0]){dsr_text_copy(out,48,runtime[i].device.name);found=true;break;}
 xSemaphoreGive(mutex);return found;
}

static dsr_output_request_t outputs[DSR_MAX_DEVICES];
_Static_assert(sizeof(runtime)<=4096,"Runtime fixed storage budget");
_Static_assert(sizeof(outputs)<=4096,"Output mailbox fixed storage budget");
static bool output_waiting[DSR_MAX_DEVICES];

static uint32_t output_serial;
static bool endpoint(const dsr_device_t *a,const dsr_device_t *b)
{return a->device_id[0]&&!strcmp(a->device_id,b->device_id)&&a->generation==b->generation&&a->endpoint_kind==b->endpoint_kind&&a->channel==b->channel;}
bool dsr_workspace_runtime_publish_begin(size_t count)
{if(!mutex||count>DSR_MAX_DEVICES)return false;xSemaphoreTake(mutex,portMAX_DELAY);runtime_count=count;return true;}
void dsr_workspace_runtime_publish_end(void){xSemaphoreGive(mutex);}
void dsr_workspace_runtime_publish_at(size_t index,size_t count,const dsr_runtime_device_t *device)
{
 if(!device||index>=count||count!=runtime_count)return;
 runtime[index]=*device;
 for(size_t i=0;i<DSR_MAX_DEVICES;i++)if(output_waiting[i]&&endpoint(&outputs[i].device,&device->device)){runtime[index].pending=true;runtime[index].operation_id=outputs[i].operation_id;if(outputs[i].device.endpoint_kind==DSR_ENDPOINT_COVER&&outputs[i].cover.command==DSR_COVER_STOP)runtime[index].stop_pending=true;}
}
size_t dsr_workspace_runtime_snapshot(dsr_runtime_device_t *devices,size_t capacity)
{
 if(!mutex||!devices)return 0;
 xSemaphoreTake(mutex,portMAX_DELAY);size_t count=runtime_count<capacity?runtime_count:capacity;
 memcpy(devices,runtime,count*sizeof(*devices));xSemaphoreGive(mutex);return count;
}
static dsr_result_t intent_request(const dsr_device_t *device,bool on,dsr_cover_intent_t intent,dsr_rgb_intent_t rgb,uint32_t *operation_id)
{
 if(!mutex||!device||!operation_id||device->endpoint_kind>DSR_ENDPOINT_BLU)return DSR_INVALID;
 dsr_light_intent_t light={.command=intent.command,.brightness=intent.position};
 bool is_light=device->endpoint_kind==DSR_ENDPOINT_LIGHT,is_rgb=device->endpoint_kind==DSR_ENDPOINT_RGB,sensor=DSR_ENDPOINT_IS_SENSOR(device->endpoint_kind);
 if(is_rgb&&!dsr_rgb_intent_valid(rgb))return DSR_INVALID;
 if(is_light&&!dsr_light_intent_valid(light))return DSR_INVALID;
 bool cover=device->endpoint_kind==DSR_ENDPOINT_COVER,stop=cover&&intent.command==DSR_COVER_STOP;
 if(cover&&!dsr_cover_intent_valid(intent))return DSR_INVALID;
 xSemaphoreTake(mutex,portMAX_DELAY);dsr_result_t result=DSR_INVALID;
 for(size_t i=0;i<runtime_count;i++)if(endpoint(&runtime[i].device,device)){
  dsr_runtime_device_t *r=&runtime[i];
  int free_slot=-1,queued_slot=-1;for(size_t j=0;j<DSR_MAX_DEVICES;j++){if(!output_waiting[j]&&free_slot<0)free_slot=(int)j;if(output_waiting[j]&&endpoint(&outputs[j].device,device))queued_slot=(int)j;}
  bool replace=stop&&queued_slot>=0&&outputs[queued_slot].cover.command!=DSR_COVER_STOP;if(replace)free_slot=queued_slot;
  if((queued_slot>=0&&!replace)||(!stop&&(r->pending||dsr_actions_reserved(device)))||(stop&&r->stop_pending)||free_slot<0)result=DSR_BUSY;
  else if(sensor?!r->can_refresh:stop?!r->can_stop:(!r->device.online||!r->has_sample_time||r->sample_age_ms>15000))result=DSR_OFFLINE;
  else if(sensor?false:cover?!dsr_cover_can_move(&r->telemetry.cover,intent):is_light?!dsr_light_can_set(&r->telemetry.light,light):is_rgb?!dsr_rgb_can_set(&r->telemetry.rgb,rgb):!r->device.has_switch)result=DSR_UNSUPPORTED;
  else if(!sensor&&(r->error==DSR_AUTH_REQUIRED||r->error==DSR_IDENTITY_MISMATCH||r->error==DSR_RATE_LIMITED))result=r->error;
  else if(!replace&&!dsr_activity_begin())result=DSR_BUSY;
  else {if(!++output_serial)++output_serial;outputs[free_slot]=(dsr_output_request_t){.device=r->device,.on=on,.operation_id=output_serial};if(cover)outputs[free_slot].cover=intent;if(is_light)outputs[free_slot].light=light;if(is_rgb)outputs[free_slot].rgb=rgb;output_waiting[free_slot]=true;r->pending=true;if(stop)r->stop_pending=true;r->operation_id=outputs[free_slot].operation_id;*operation_id=r->operation_id;result=DSR_OK;if(stop)dsr_actions_mode_block();}
  break;
 }
 xSemaphoreGive(mutex);return result;
}
dsr_result_t dsr_workspace_output_request(const dsr_device_t *device,bool on,uint32_t *operation_id)
{if(!device)return DSR_INVALID;if(device->endpoint_kind!=DSR_ENDPOINT_SWITCH)return DSR_UNSUPPORTED;return intent_request(device,on,(dsr_cover_intent_t){0},(dsr_rgb_intent_t){0},operation_id);}
dsr_result_t dsr_workspace_cover_request(const dsr_device_t *device,dsr_cover_intent_t intent,uint32_t *operation_id)
{if(!device)return DSR_INVALID;if(device->endpoint_kind!=DSR_ENDPOINT_COVER)return DSR_UNSUPPORTED;return intent_request(device,false,intent,(dsr_rgb_intent_t){0},operation_id);}
dsr_result_t dsr_workspace_light_request(const dsr_device_t *device,dsr_light_intent_t intent,uint32_t *operation_id)
{if(!device)return DSR_INVALID;if(device->endpoint_kind!=DSR_ENDPOINT_LIGHT)return DSR_UNSUPPORTED;return intent_request(device,false,(dsr_cover_intent_t){.command=intent.command,.position=intent.brightness},(dsr_rgb_intent_t){0},operation_id);}
dsr_result_t dsr_workspace_rgb_request(const dsr_device_t *device,dsr_rgb_intent_t intent,uint32_t *operation_id)
{if(!device)return DSR_INVALID;if(device->endpoint_kind!=DSR_ENDPOINT_RGB)return DSR_UNSUPPORTED;return intent_request(device,false,(dsr_cover_intent_t){0},intent,operation_id);}
dsr_result_t dsr_workspace_ht_refresh(const dsr_device_t *device,uint32_t *operation_id)
{if(!device)return DSR_INVALID;if(!DSR_ENDPOINT_IS_SENSOR(device->endpoint_kind))return DSR_UNSUPPORTED;return intent_request(device,false,(dsr_cover_intent_t){0},(dsr_rgb_intent_t){0},operation_id);}
bool dsr_workspace_take_output(dsr_output_request_t *request)
{
 if(!mutex||!request)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool found=false;int oldest=-1;
 for(size_t i=0;i<DSR_MAX_DEVICES;i++)if(output_waiting[i]){
  bool priority=outputs[i].device.endpoint_kind==DSR_ENDPOINT_COVER&&outputs[i].cover.command==DSR_COVER_STOP;
  bool previous=oldest>=0&&outputs[oldest].device.endpoint_kind==DSR_ENDPOINT_COVER&&outputs[oldest].cover.command==DSR_COVER_STOP;
  if(oldest<0||(priority&&!previous)||(priority==previous&&(int32_t)(outputs[i].operation_id-outputs[oldest].operation_id)<0))oldest=(int)i;
 }
 if(oldest>=0){*request=outputs[oldest];output_waiting[oldest]=false;memset(&outputs[oldest],0,sizeof(outputs[oldest]));found=true;}
 xSemaphoreGive(mutex);if(found)dsr_activity_end();return found;
}
