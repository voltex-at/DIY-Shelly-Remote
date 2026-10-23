#include "dsr_device_settings.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"
void vTaskDelay(TickType_t ticks){lv_tick_inc(ticks);}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* Deterministic HTTP only; editor, actor, model and LCD are production code. */
static bool enabled;
static unsigned writes;
static cJSON *settings,*network;
static cJSON *system_config;
static bool general_enabled;
static bool complete;
static unsigned gateway_mode;
static bool gateway_registered;
/* 1: registered/no recent report; 2: fresh report; 3: partial setup;
 * 4: unsupported gateway; 5: authentication rejected. HTTP model only. */
void test_settings_gateway(unsigned mode){gateway_mode=mode;gateway_registered=false;writes=0;}
static dsr_result_t gateway_rpc(const char *method,cJSON **out,bool write,bool *attempted)
{
 if(gateway_mode==5)return DSR_AUTH_REQUIRED;
 if(!strcmp(method,"Shelly.ListMethods"))*out=cJSON_Parse(gateway_mode==4?"{\"methods\":[]}":"{\"methods\":[\"BTHome.AddDevice\",\"BTHome.AddSensor\",\"BTHomeDevice.GetKnownObjects\",\"BluTrv.GetConfig\",\"BluTrv.SetConfig\",\"BLE.StartAssociations\"]}");
 else if(!strcmp(method,"Shelly.GetComponents"))*out=cJSON_Parse(gateway_registered?"{\"offset\":0,\"total\":2,\"components\":[{\"key\":\"bthomedevice:200\",\"config\":{\"addr\":\"06:05:04:03:02:02\"}},{\"key\":\"blutrv:200\",\"config\":{\"addr\":\"06:05:04:03:02:02\",\"trv\":\"bthomedevice:200\"}}]}":"{\"offset\":0,\"total\":0,\"components\":[]}");
 else if(write&&(!strcmp(method,"BTHome.AddDevice")||!strcmp(method,"BLE.StartAssociations")||!strcmp(method,"BluTrv.SetConfig"))){gateway_registered=true;writes++;if(attempted)*attempted=true;*out=cJSON_CreateObject();}
 else if(!strcmp(method,"BTHomeDevice.GetKnownObjects")){if(gateway_mode==3)return DSR_OFFLINE;*out=cJSON_Parse("{\"objects\":[]}");}
 else if(!strcmp(method,"BTHomeDevice.GetStatus")){*out=cJSON_Parse("{\"id\":200,\"battery\":90,\"rssi\":-50,\"errors\":[],\"last_updated_ts\":0}");if(gateway_mode==2)cJSON_ReplaceItemInObjectCaseSensitive(*out,"last_updated_ts",cJSON_CreateNumber((double)time(NULL)));}
 else return DSR_UNSUPPORTED;
 return *out?DSR_OK:DSR_IO;
}
void test_settings_complete(bool active){complete=active;}
/* A capability-rich response exercises every existing allowlisted editor.
 * This is explicitly fixture data, never an invented hardware capability. */
static cJSON *complete_config(const char *ns)
{
 cJSON *root=cJSON_CreateObject();const dsr_setting_field_t *f;
 for(unsigned i=0;(f=dsr_setting_field(i));i++)if(!strcmp(ns,f->component)){
  char path[96];snprintf(path,sizeof(path),"%s",f->path);cJSON *parent=root;char *key=path,*end;
  while((end=strchr(key,'.'))){*end=0;cJSON *child=cJSON_GetObjectItemCaseSensitive(parent,key);if(!child){child=cJSON_CreateObject();cJSON_AddItemToObject(parent,key,child);}parent=child;key=end+1;}
  if(f->type==DSR_SETTING_BOOL)cJSON_AddBoolToObject(parent,key,true);
  else if(f->type==DSR_SETTING_NUMBER)cJSON_AddNumberToObject(parent,key,f->minimum);
  else if(f->type==DSR_SETTING_ENUM){char first[64];snprintf(first,sizeof(first),"%s",f->choices);char *nl=strchr(first,'\n');if(nl)*nl=0;cJSON_AddStringToObject(parent,key,first);}
  else if(f->type!=DSR_SETTING_SECRET)cJSON_AddStringToObject(parent,key,!strcmp(f->path,"sta.ip")?"192.0.2.50":"Atlas");
 }
 return root;
}
void test_settings_general(bool enabled)
{general_enabled=enabled;cJSON_Delete(system_config);system_config=enabled?cJSON_Parse("{\"device\":{\"name\":\"Decke\",\"eco_mode\":false},\"location\":{\"tz\":\"Europe/Vienna\",\"lat\":48.1968,\"lon\":16.3707},\"sntp\":{\"server\":\"time.google.com\"}}"):NULL;}
void test_settings_enable(bool active)
{enabled=active;writes=0;cJSON_Delete(settings);cJSON_Delete(network);settings=network=NULL;if(active){settings=cJSON_Parse("{\"id\":0,\"name\":\"Relay\",\"auto_off\":false,\"auto_off_delay\":60}");network=cJSON_Parse("{\"sta\":{\"enable\":true,\"ssid\":\"Home\",\"ipv4mode\":\"dhcp\",\"ip\":null}}");}}
unsigned test_settings_writes(void){return writes;}
static void merge(cJSON *target,const cJSON *patch)
{const cJSON *v;cJSON_ArrayForEach(v,patch){cJSON *old=cJSON_GetObjectItemCaseSensitive(target,v->string);if(cJSON_IsObject(v)&&cJSON_IsObject(old))merge(old,v);else{cJSON *copy=cJSON_Duplicate(v,true);if(old)cJSON_ReplaceItemInObjectCaseSensitive(target,v->string,copy);else cJSON_AddItemToObject(target,v->string,copy);}}}
dsr_result_t dsr_http_settings_identity(const dsr_connection_t *c){(void)c;return DSR_OK;}
dsr_result_t dsr_http_settings_rpc(const dsr_connection_t *c,const char *method,const cJSON *p,cJSON **out,bool write,bool *attempted)
{
 (void)c;if(attempted)*attempted=false;*out=NULL;if(!enabled)return DSR_OFFLINE;
 if(gateway_mode)return gateway_rpc(method,out,write,attempted);
 if(complete){
  if(!strcmp(method,"Shelly.ListMethods")){
   *out=cJSON_CreateObject();cJSON *methods=cJSON_AddArrayToObject(*out,"methods");const char *names[]={"Sys","WiFi","BLE","Cloud","MQTT","Switch","Input","Cover","Light","RGB","RGBW","Temperature","Humidity"};
   for(unsigned i=0;i<sizeof(names)/sizeof(*names);i++)for(unsigned n=0;n<2;n++){char m[64];snprintf(m,sizeof(m),"%s.%sConfig",names[i],n?"Set":"Get");cJSON_AddItemToArray(methods,cJSON_CreateString(m));}
   return DSR_OK;
  }
  if(!strcmp(method,"Shelly.GetComponents")){*out=cJSON_Parse("{\"offset\":0,\"total\":8,\"components\":[{\"key\":\"switch:0\"},{\"key\":\"input:0\"},{\"key\":\"cover:0\"},{\"key\":\"light:0\"},{\"key\":\"rgb:0\"},{\"key\":\"rgbw:0\"},{\"key\":\"temperature:0\"},{\"key\":\"humidity:0\"}]}");return DSR_OK;}
  const char *end=strstr(method,".GetConfig");if(end){char ns[32];snprintf(ns,sizeof(ns),"%.*s",(int)(end-method),method);*out=complete_config(ns);return *out?DSR_OK:DSR_IO;}
 }
 if(!strcmp(method,"Shelly.ListMethods"))*out=cJSON_Parse(general_enabled?"{\"methods\":[\"Sys.GetConfig\",\"Sys.SetConfig\",\"Switch.GetConfig\",\"Switch.SetConfig\",\"WiFi.GetConfig\",\"WiFi.SetConfig\"]}":"{\"methods\":[\"Switch.GetConfig\",\"Switch.SetConfig\",\"WiFi.GetConfig\",\"WiFi.SetConfig\"]}");
 else if(!strcmp(method,"Shelly.GetComponents"))*out=cJSON_Parse("{\"offset\":0,\"total\":1,\"components\":[{\"key\":\"switch:0\"}]}");
 else if(!strcmp(method,"Switch.GetConfig"))*out=cJSON_Duplicate(settings,true);
 else if(!strcmp(method,"WiFi.GetConfig"))*out=cJSON_Duplicate(network,true);
 else if(general_enabled&&!strcmp(method,"Sys.GetConfig"))*out=cJSON_Duplicate(system_config,true);
 else if(general_enabled&&write&&!strcmp(method,"Sys.SetConfig")){writes++;if(attempted)*attempted=true;merge(system_config,cJSON_GetObjectItemCaseSensitive(p,"config"));*out=cJSON_Parse("{\"restart_required\":false}");}
 else if(write&&(!strcmp(method,"Switch.SetConfig")||!strcmp(method,"WiFi.SetConfig"))){writes++;if(attempted)*attempted=true;merge(!strcmp(method,"Switch.SetConfig")?settings:network,cJSON_GetObjectItemCaseSensitive(p,"config"));*out=cJSON_Parse("{\"restart_required\":false}");}
 else return DSR_UNSUPPORTED;
 return *out?DSR_OK:DSR_IO;
}
dsr_http_job_guard_t dsr_http_job_guard(dsr_http_job_guard_t guard){(void)guard;return NULL;}
bool dsr_workspace_mapping_matches(const dsr_connection_t *c){return c&&c->layout_id!=0;}
