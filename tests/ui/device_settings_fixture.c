#include "dsr_device_settings.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
void vTaskDelay(TickType_t ticks){lv_tick_inc(ticks);}
#include <stdlib.h>
#include <string.h>
/* Deterministic HTTP only; editor, actor, model and LCD are production code. */
static bool enabled;
static unsigned writes;
static cJSON *settings,*network;
void test_settings_enable(bool active)
{enabled=active;writes=0;cJSON_Delete(settings);cJSON_Delete(network);settings=network=NULL;if(active){settings=cJSON_Parse("{\"id\":0,\"name\":\"Relay\",\"auto_off\":false,\"auto_off_delay\":60}");network=cJSON_Parse("{\"sta\":{\"enable\":true,\"ssid\":\"Home\",\"ipv4mode\":\"dhcp\",\"ip\":null}}");}}
unsigned test_settings_writes(void){return writes;}
static void merge(cJSON *target,const cJSON *patch)
{const cJSON *v;cJSON_ArrayForEach(v,patch){cJSON *old=cJSON_GetObjectItemCaseSensitive(target,v->string);if(cJSON_IsObject(v)&&cJSON_IsObject(old))merge(old,v);else{cJSON *copy=cJSON_Duplicate(v,true);if(old)cJSON_ReplaceItemInObjectCaseSensitive(target,v->string,copy);else cJSON_AddItemToObject(target,v->string,copy);}}}
dsr_result_t dsr_http_settings_identity(const dsr_connection_t *c){(void)c;return DSR_OK;}
dsr_result_t dsr_http_settings_rpc(const dsr_connection_t *c,const char *method,const cJSON *p,cJSON **out,bool write,bool *attempted)
{
 (void)c;if(attempted)*attempted=false;*out=NULL;if(!enabled)return DSR_OFFLINE;
 if(!strcmp(method,"Shelly.ListMethods"))*out=cJSON_Parse("{\"methods\":[\"Switch.GetConfig\",\"Switch.SetConfig\",\"WiFi.GetConfig\",\"WiFi.SetConfig\"]}");
 else if(!strcmp(method,"Shelly.GetComponents"))*out=cJSON_Parse("{\"offset\":0,\"total\":1,\"components\":[{\"key\":\"switch:0\"}]}");
 else if(!strcmp(method,"Switch.GetConfig"))*out=cJSON_Duplicate(settings,true);
 else if(!strcmp(method,"WiFi.GetConfig"))*out=cJSON_Duplicate(network,true);
 else if(write&&(!strcmp(method,"Switch.SetConfig")||!strcmp(method,"WiFi.SetConfig"))){writes++;if(attempted)*attempted=true;merge(!strcmp(method,"Switch.SetConfig")?settings:network,cJSON_GetObjectItemCaseSensitive(p,"config"));*out=cJSON_Parse("{\"restart_required\":false}");}
 else return DSR_UNSUPPORTED;
 return *out?DSR_OK:DSR_IO;
}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t guard){(void)guard;return NULL;}
bool dsr_workspace_mapping_matches(const dsr_connection_t *c){return c&&c->layout_id!=0;}
