#include "dsr_workspace.h"
#include "dsr_storage.h"
#include "dsr_activity.h"
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
static dsr_connection_t bind_request;
static bool bind_waiting,bind_busy;
static uint32_t bind_room=UINT32_MAX;
static dsr_result_t bind_result_code=DSR_OK;
static char bind_message[64];
typedef struct {char *json;uint32_t revision;} request_t;
static bool workspace_ready(void){dsr_config_health_t h;dsr_config_health_snapshot(&h);return h.workspace==DSR_WORKSPACE_READY&&mutex&&queue&&current;}
static void config_task(void *argument)
{
 (void)argument;request_t request;
 for(;;){
  if(xQueueReceive(queue,&request,portMAX_DELAY)!=pdTRUE)continue;
  char error[96]={0};char *previous=dsr_workspace_snapshot();
  char *next=dsr_config_replace(previous,request.json,request.revision,error);free(previous);free(request.json);
  if(next&&!dsr_config_store_save("/dsr",next,error)){free(next);next=NULL;}
  xSemaphoreTake(mutex,portMAX_DELAY);
  if(next){free(current);current=next;revision=dsr_config_revision(next);snprintf(status,sizeof(status),"Saved revision %lu",(unsigned long)revision);}
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
 bool found=false;char error[96]={0};current=dsr_config_store_load("/dsr",&found,error);
 if(!found){if(error[0]){dsr_config_health_workspace(DSR_WORKSPACE_LOADING,ESP_FAIL);return ESP_FAIL;}dsr_config_health_workspace(DSR_WORKSPACE_PROVISIONING,ESP_OK);current=dsr_config_default();if(!current){dsr_config_health_workspace(DSR_WORKSPACE_PROVISIONING,ESP_ERR_NO_MEM);return ESP_ERR_NO_MEM;}
  if(!dsr_config_store_save("/dsr",current,error)){free(current);current=NULL;dsr_config_health_workspace(DSR_WORKSPACE_PROVISIONING,ESP_FAIL);return ESP_FAIL;}}
 /* Refresh the older checkpoint with the already verified boot document.
  * Power loss retains the newest record. Readback must succeed before GC.
  * This also retires old photo references without requiring a second edit. */
 if(found)dsr_config_store_save("/dsr",current,error);
 /* Boot-time GC precedes cache/LVGL construction; immutable live pointers are
  * never freed. Any remaining corrupt checkpoint conservatively skips GC. */
 unsigned removed=0;dsr_config_store_collect_photos("/dsr",&removed,error);
 dsr_config_health_workspace(DSR_WORKSPACE_ALLOCATING,ESP_OK);
 dsr_photos_load();revision=dsr_config_revision(current);mutex=xSemaphoreCreateMutex();queue=xQueueCreate(1,sizeof(request_t));
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
 if(ok){ok=dsr_activity_begin();if(ok){request_t req={copy,expected};ok=xQueueSend(queue,&req,0)==pdTRUE;if(ok){busy=true;snprintf(status,sizeof(status),"Saving…");}else dsr_activity_end();}}
 xSemaphoreGive(mutex);if(!ok)free(copy);return ok;
}
void dsr_workspace_status(bool *pending,char message[96])
{if(!workspace_ready()){*pending=false;snprintf(message,96,"Configuration storage unavailable");return;}xSemaphoreTake(mutex,portMAX_DELAY);*pending=busy;snprintf(message,96,"%s",status);xSemaphoreGive(mutex);}
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
bool dsr_workspace_take_bind(dsr_connection_t *connection){return dsr_workspace_take_bind_with_room(connection,NULL);}
void dsr_workspace_bind_result(dsr_result_t result)
{if(!mutex)return;xSemaphoreTake(mutex,portMAX_DELAY);bool completed=bind_busy;bind_busy=false;bind_result_code=result;snprintf(bind_message,sizeof(bind_message),"%s",result==DSR_OK?"Shelly verified and saved":result==DSR_AUTH_REQUIRED?"Authentication failed; check Shelly credentials":result==DSR_OFFLINE?"Shelly offline / Wi-Fi disconnected":result==DSR_CONFLICT?"Configuration changed; reload and retry adding the device":"Shelly check failed; previous binding retained");xSemaphoreGive(mutex);if(completed)dsr_activity_end();}
dsr_result_t dsr_workspace_bind_result_code(void)
{if(!mutex)return DSR_IO;xSemaphoreTake(mutex,portMAX_DELAY);dsr_result_t result=bind_result_code;xSemaphoreGive(mutex);return result;}
void dsr_workspace_bind_status(bool *pending,char message[64])
{if(!mutex){*pending=false;message[0]=0;return;}xSemaphoreTake(mutex,portMAX_DELAY);*pending=bind_busy;snprintf(message,64,"%s",bind_message);xSemaphoreGive(mutex);}

/* Mutex also protects cross-task command admission; no HTTP thread may touch
 * LVGL or copy credentials. One mailbox per endpoint prevents duplicate SET. */
static dsr_runtime_device_t runtime[DSR_MAX_DEVICES];
static dsr_output_request_t outputs[DSR_MAX_DEVICES];
_Static_assert(sizeof(runtime)<=4096,"Runtime fixed storage budget");
_Static_assert(sizeof(outputs)<=4096,"Output mailbox fixed storage budget");
static bool output_waiting[DSR_MAX_DEVICES];
static size_t runtime_count;
static uint32_t output_serial;
static bool endpoint(const dsr_device_t *a,const dsr_device_t *b)
{return a->device_id[0]&&!strcmp(a->device_id,b->device_id)&&a->generation==b->generation&&a->channel==b->channel;}
bool dsr_workspace_runtime_publish_begin(size_t count)
{if(!mutex||count>DSR_MAX_DEVICES)return false;xSemaphoreTake(mutex,portMAX_DELAY);runtime_count=count;return true;}
void dsr_workspace_runtime_publish_end(void){xSemaphoreGive(mutex);}
void dsr_workspace_runtime_publish_at(size_t index,size_t count,const dsr_runtime_device_t *device)
{
 if(!device||index>=count||count!=runtime_count)return;
 runtime[index]=*device;
 for(size_t i=0;i<DSR_MAX_DEVICES;i++)if(output_waiting[i]&&endpoint(&outputs[i].device,&device->device)){runtime[index].pending=true;runtime[index].operation_id=outputs[i].operation_id;}
}
size_t dsr_workspace_runtime_snapshot(dsr_runtime_device_t *devices,size_t capacity)
{
 if(!mutex||!devices)return 0;
 xSemaphoreTake(mutex,portMAX_DELAY);size_t count=runtime_count<capacity?runtime_count:capacity;
 memcpy(devices,runtime,count*sizeof(*devices));xSemaphoreGive(mutex);return count;
}
dsr_result_t dsr_workspace_output_request(const dsr_device_t *device,bool on,uint32_t *operation_id)
{
 if(!mutex||!device||!operation_id)return DSR_INVALID;
 xSemaphoreTake(mutex,portMAX_DELAY);dsr_result_t result=DSR_INVALID;
 for(size_t i=0;i<runtime_count;i++)if(endpoint(&runtime[i].device,device)){
  dsr_runtime_device_t *r=&runtime[i];
  int free_slot=-1;bool queued=false;for(size_t j=0;j<DSR_MAX_DEVICES;j++){if(!output_waiting[j]&&free_slot<0)free_slot=(int)j;if(output_waiting[j]&&endpoint(&outputs[j].device,device))queued=true;}
  if(queued||r->pending||free_slot<0)result=DSR_BUSY;
  else if(!r->device.online||!r->has_sample_time||r->sample_age_ms>15000)result=DSR_OFFLINE;
  else if(!r->device.has_switch)result=DSR_UNSUPPORTED;
  else if(r->error==DSR_AUTH_REQUIRED||r->error==DSR_IDENTITY_MISMATCH||r->error==DSR_RATE_LIMITED)result=r->error;
  else if(!dsr_activity_begin())result=DSR_BUSY;
  else {outputs[free_slot]=(dsr_output_request_t){.device=r->device,.on=on,.operation_id=++output_serial};output_waiting[free_slot]=true;r->pending=true;r->operation_id=outputs[free_slot].operation_id;*operation_id=r->operation_id;result=DSR_OK;}
  break;
 }
 xSemaphoreGive(mutex);return result;
}
bool dsr_workspace_take_output(dsr_output_request_t *request)
{
 if(!mutex||!request)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool found=false;int oldest=-1;
 for(size_t i=0;i<DSR_MAX_DEVICES;i++)if(output_waiting[i]&&(oldest<0||(int32_t)(outputs[i].operation_id-outputs[oldest].operation_id)<0))oldest=(int)i;
 if(oldest>=0){*request=outputs[oldest];output_waiting[oldest]=false;memset(&outputs[oldest],0,sizeof(outputs[oldest]));found=true;}
 xSemaphoreGive(mutex);if(found)dsr_activity_end();return found;
}
