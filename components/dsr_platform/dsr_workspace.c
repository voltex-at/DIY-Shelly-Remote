#include "dsr_workspace.h"
#include "esp_spiffs.h"
#include "esp_partition.h"
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
 }
}
esp_err_t dsr_workspace_start(void)
{
 if(mutex)return ESP_ERR_INVALID_STATE;
 const esp_vfs_spiffs_conf_t conf={.base_path="/dsr",.partition_label="dsrdata",.max_files=8,.format_if_mount_failed=false};
 esp_err_t err=esp_vfs_spiffs_register(&conf);
 if(err!=ESP_OK){
  /* Only an entirely erased data partition may be provisioned automatically.
   * A corrupt/nonempty filesystem must be recovered explicitly, never erased. */
  const esp_partition_t *part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"dsrdata");
  if(!part)return err;
  uint8_t data[1024];bool erased=true;
  for(size_t at=0;at<part->size&&erased;at+=sizeof(data)){
   if(esp_partition_read(part,at,data,sizeof(data))!=ESP_OK)return err;
   for(unsigned i=0;i<sizeof(data);i++)if(data[i]!=255){erased=false;break;}
  }
  if(!erased||esp_spiffs_format("dsrdata")!=ESP_OK)return err;
  err=esp_vfs_spiffs_register(&conf);if(err!=ESP_OK)return err;
 }
 bool found=false;char error[96]={0};current=dsr_config_store_load("/dsr",&found,error);
 if(!found){if(error[0])return ESP_FAIL;current=dsr_config_default();if(!current)return ESP_ERR_NO_MEM;
  if(!dsr_config_store_save("/dsr",current,error)){free(current);current=NULL;return ESP_FAIL;}}
 /* Refresh the older checkpoint with the already verified boot document.
  * Power loss retains the newest record. Readback must succeed before GC.
  * This also retires old photo references without requiring a second edit. */
 if(found)dsr_config_store_save("/dsr",current,error);
 /* Boot-time GC precedes cache/LVGL construction; immutable live pointers are
  * never freed. Any remaining corrupt checkpoint conservatively skips GC. */
 unsigned removed=0;dsr_config_store_collect_photos("/dsr",&removed,error);
 dsr_photos_load();revision=dsr_config_revision(current);mutex=xSemaphoreCreateMutex();queue=xQueueCreate(1,sizeof(request_t));
 if(!mutex||!queue)return ESP_ERR_NO_MEM;
 return xTaskCreate(config_task,"dsr_config",8192,NULL,3,NULL)==pdPASS?ESP_OK:ESP_ERR_NO_MEM;
}
char *dsr_workspace_snapshot(void)
{
 if(!mutex)return NULL;
 xSemaphoreTake(mutex,portMAX_DELAY);
 char *copy=malloc(strlen(current)+1);if(copy)strcpy(copy,current);xSemaphoreGive(mutex);return copy;
}
uint32_t dsr_workspace_revision(void)
{if(!mutex)return 0;xSemaphoreTake(mutex,portMAX_DELAY);uint32_t r=revision;xSemaphoreGive(mutex);return r;}
bool dsr_workspace_submit(const char *json,uint32_t expected)
{
 if(!mutex||!json||strlen(json)>DSR_CONFIG_MAX_JSON)return false;
 char *copy=malloc(strlen(json)+1);if(!copy)return false;strcpy(copy,json);
 xSemaphoreTake(mutex,portMAX_DELAY);
 bool ok=!busy&&revision==expected;
 if(ok){request_t req={copy,expected};ok=xQueueSend(queue,&req,0)==pdTRUE;if(ok){busy=true;snprintf(status,sizeof(status),"Saving…");}}
 xSemaphoreGive(mutex);if(!ok)free(copy);return ok;
}
void dsr_workspace_status(bool *pending,char message[96])
{if(!mutex){*pending=false;snprintf(message,96,"Configuration storage unavailable");return;}xSemaphoreTake(mutex,portMAX_DELAY);*pending=busy;snprintf(message,96,"%s",status);xSemaphoreGive(mutex);}
bool dsr_workspace_bind_with_room(const dsr_connection_t *connection,uint32_t room)
{
 if(!mutex||!dsr_connection_valid(connection,false))return false;
 xSemaphoreTake(mutex,portMAX_DELAY);
 bool ok=!bind_busy;if(ok){bind_request=*connection;bind_room=room;bind_result_code=DSR_BUSY;bind_waiting=bind_busy=true;snprintf(bind_message,sizeof(bind_message),"Checking Shelly identity…");}xSemaphoreGive(mutex);return ok;
}
bool dsr_workspace_bind(const dsr_connection_t *connection){return dsr_workspace_bind_with_room(connection,UINT32_MAX);}
bool dsr_workspace_take_bind_with_room(dsr_connection_t *connection,uint32_t *room)
{
 if(!mutex)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool found=bind_waiting;
 if(found){*connection=bind_request;if(room)*room=bind_room;bind_room=UINT32_MAX;memset(&bind_request,0,sizeof(bind_request));bind_waiting=false;}xSemaphoreGive(mutex);return found;
}
bool dsr_workspace_take_bind(dsr_connection_t *connection){return dsr_workspace_take_bind_with_room(connection,NULL);}
void dsr_workspace_bind_result(dsr_result_t result)
{if(!mutex)return;xSemaphoreTake(mutex,portMAX_DELAY);bind_busy=false;bind_result_code=result;snprintf(bind_message,sizeof(bind_message),"%s",result==DSR_OK?"Shelly verified and saved":result==DSR_AUTH_REQUIRED?"Authentication failed; check Shelly credentials":result==DSR_OFFLINE?"Shelly offline / Wi-Fi disconnected":"Shelly check failed; previous binding retained");xSemaphoreGive(mutex);}
dsr_result_t dsr_workspace_bind_result_code(void)
{if(!mutex)return DSR_IO;xSemaphoreTake(mutex,portMAX_DELAY);dsr_result_t result=bind_result_code;xSemaphoreGive(mutex);return result;}
void dsr_workspace_bind_status(bool *pending,char message[64])
{if(!mutex){*pending=false;message[0]=0;return;}xSemaphoreTake(mutex,portMAX_DELAY);*pending=bind_busy;snprintf(message,64,"%s",bind_message);xSemaphoreGive(mutex);}
