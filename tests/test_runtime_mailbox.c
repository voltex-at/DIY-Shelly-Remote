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
{(void)value;(void)wait;CHECK(handle==&queue);return pdTRUE;}
int xQueueReceive(QueueHandle_t handle,void *value,TickType_t wait)
{(void)handle;(void)value;(void)wait;abort();}
int xTaskCreate(void(*callback)(void *),const char *name,unsigned stack,void *argument,unsigned priority,TaskHandle_t *out)
{CHECK(callback&&!strcmp(name,"dsr_config")&&stack==8192&&!argument&&priority==3&&!out);return pdPASS;}
esp_err_t dsr_storage_mount(void)
{dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);return ESP_OK;}
char *dsr_config_store_load(const char *directory,bool *found,char error[96])
{CHECK(!strcmp(directory,"/dsr"));*found=false;error[0]=0;return NULL;}
bool dsr_config_store_save(const char *directory,const char *json,char error[96])
{(void)error;CHECK(!strcmp(directory,"/dsr")&&json);return true;}
bool dsr_config_store_collect_photos(const char *directory,unsigned *removed,char error[96])
{CHECK(!strcmp(directory,"/dsr"));*removed=0;error[0]=0;return true;}
void dsr_photos_load(void){}
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
int main(void)
{
 dsr_runtime_device_t snapshot[DSR_MAX_DEVICES];dsr_output_request_t request;uint32_t operation=123;
 CHECK(dsr_workspace_runtime_snapshot(snapshot,DSR_MAX_DEVICES)==0);
 dsr_runtime_device_t devices[2]={device("shelly-A",0),device("shelly-B",1)};
 CHECK(dsr_workspace_output_request(&devices[0].device,true,&operation)==DSR_INVALID&&operation==123);admission_open();
 CHECK(dsr_workspace_start()==ESP_OK);publish(devices,2);
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
 printf("PASS: %u actual workspace/runtime checks; identity mailboxes, compaction, count0, per-endpoint admission and restart handoff; RTOS/Flash/HTTP MODEL\n",checks);
 return 0;
}
