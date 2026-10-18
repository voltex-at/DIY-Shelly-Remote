#include "dsr_device_settings.h"
#include "dsr_activity.h"
#include "dsr_fleet.h"
#include "dsr_discovery.h"
#include "dsr_blu.h"
#include "freertos/task.h"
#include <ctype.h>
#include <math.h>
#include <stdatomic.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
 dsr_device_settings_snapshot_t view,working;
 dsr_connection_t connection;
 cJSON *methods,*baseline;
 char value[257];
 unsigned action,selection;
 bool queued,running,closing;
} session_t;
static session_t *session;
static SemaphoreHandle_t mutex;
static atomic_bool sensitive;
void dsr_device_settings_visibility(bool visible){atomic_store(&sensitive,visible);}
bool dsr_device_settings_sensitive(void){return atomic_load(&sensitive);}
static void wipe(void *p,size_t n){volatile unsigned char *v=p;while(n--)*v++=0;}
static bool lock(void){return mutex&&xSemaphoreTake(mutex,portMAX_DELAY)==pdTRUE;}
static void unlock(void){xSemaphoreGive(mutex);}
static void dispose(void)
{if(!session)return;dsr_setting_json_delete(session->methods);dsr_setting_json_delete(session->baseline);wipe(session,sizeof(*session));free(session);session=NULL;}
esp_err_t dsr_device_settings_init(void)
{if(!mutex)mutex=xSemaphoreCreateMutex();return mutex?ESP_OK:ESP_ERR_NO_MEM;}
dsr_result_t dsr_device_settings_prepare(uint32_t id,const dsr_connection_t *c,unsigned action,unsigned selection,const char *value)
{
 if(!id||!c||c->device.generation!=DSR_GEN2_PLUS||!dsr_connection_valid(c,false)||action>DSR_SETTINGS_BLU_ADD||!memchr(c->device.device_id,0,sizeof(c->device.device_id))||!c->device.device_id[0]||(value&&strlen(value)>256))return DSR_INVALID;
 if(dsr_device_settings_init()!=ESP_OK||!lock())return DSR_IO;
 if(session&&(session->queued||session->running)){unlock();return DSR_BUSY;}
 if(action==DSR_SETTINGS_LOAD||action==DSR_SETTINGS_BLU_ADD){dispose();
#ifdef ESP_PLATFORM
  session=heap_caps_calloc(1,sizeof(*session),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
  session=calloc(1,sizeof(*session));
#endif
  if(session)session->connection=*c;
 }else if(!session||session->closing||!dsr_endpoint_equal(&c->device,&session->connection.device)||strcmp(c->device.host,session->connection.device.host)||c->layout_id!=session->connection.layout_id){unlock();return DSR_IDENTITY_MISMATCH;}
 if(!session){unlock();return DSR_IO;}
 if(action==DSR_SETTINGS_GROUP&&selection>=session->view.group_count){unlock();return DSR_INVALID;}
 if(action==DSR_SETTINGS_SAVE&&(!session->view.ready||!session->baseline||!value)){unlock();return DSR_INVALID;}
 session->action=action;session->selection=selection;session->view.operation_id=id;session->view.busy=session->queued=true;session->view.result=DSR_BUSY;session->view.uncertain=false;session->view.restart_required=false;session->closing=false;
 if(value)snprintf(session->value,sizeof(session->value),"%s",value);else wipe(session->value,sizeof(session->value));
 session->view.serial++;unlock();return DSR_OK;
}
bool dsr_device_settings_admitted(const dsr_job_t *job)
{if(!job||job->kind!=DSR_JOB_DEVICE_SETTINGS||!lock())return false;bool ok=session&&session->queued&&!session->running&&!session->closing&&session->view.operation_id==job->id&&dsr_endpoint_equal(&session->connection.device,&job->connection.device)&&!strcmp(session->connection.device.host,job->connection.device.host)&&session->connection.layout_id==job->connection.layout_id;unlock();return ok;}
void dsr_device_settings_reject(uint32_t id)
{if(!lock())return;if(session&&session->queued&&session->view.operation_id==id){session->queued=false;session->view.busy=false;session->view.result=DSR_BUSY;wipe(session->value,sizeof(session->value));}unlock();}
static dsr_result_t guard(void)
{if(!lock())return DSR_BUSY;bool ok=session&&!session->closing;unlock();if(!ok)return DSR_BUSY;return !session->connection.layout_id||dsr_workspace_mapping_matches(&session->connection)?DSR_OK:DSR_CONFLICT;}
static dsr_result_t rpc(const char *method,const cJSON *params,cJSON **out,bool mutation,bool *attempted)
{dsr_result_t r=guard();return r==DSR_OK?dsr_http_settings_rpc(&session->connection,method,params,out,mutation,attempted):r;}
static cJSON *parameters(const dsr_setting_group_t *g)
{cJSON *p=cJSON_CreateObject();if(p&&!g->singleton&&!cJSON_AddNumberToObject(p,"id",g->id)){cJSON_Delete(p);return NULL;}return p;}
static dsr_result_t get_group(const dsr_setting_group_t *g,cJSON **out)
{char method[48];snprintf(method,sizeof(method),"%s.GetConfig",g->component);cJSON *p=parameters(g);if(!p)return DSR_IO;dsr_result_t r=rpc(method,p,out,false,NULL);cJSON_Delete(p);return r;}
static dsr_result_t load(void)
{
 cJSON_Delete(session->methods);session->methods=NULL;
 dsr_result_t r=rpc("Shelly.ListMethods",NULL,&session->methods,false,NULL);if(r!=DSR_OK)return r;
 unsigned count=0,offset=0;
 for(unsigned page=0;page<12;page++){
  cJSON *p=cJSON_CreateObject(),*response=NULL;if(!p)return DSR_IO;
  if(!cJSON_AddNumberToObject(p,"offset",offset)){cJSON_Delete(p);return DSR_IO;}
  r=rpc("Shelly.GetComponents",p,&response,false,NULL);cJSON_Delete(p);
  if(r!=DSR_OK){cJSON_Delete(response);return r;}
  r=dsr_setting_groups(session->methods,response,session->working.groups,&count);
  const cJSON *total=cJSON_GetObjectItemCaseSensitive(response,"total"),*items=cJSON_GetObjectItemCaseSensitive(response,"components");
  unsigned got=(unsigned)cJSON_GetArraySize(items);
  bool more=cJSON_IsNumber(total)&&total->valuedouble>(double)(offset+got);
  cJSON_Delete(response);if(r!=DSR_OK)return r;if(!more){session->working.group_count=(uint8_t)count;return count?DSR_OK:DSR_UNSUPPORTED;}if(!got)return DSR_BAD_RESPONSE;offset+=got;
 }
 return DSR_FULL;
}
static bool has_method(const cJSON *methods,const char *name)
{const cJSON *v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(methods,"methods"))if(cJSON_IsString(v)&&!strcmp(v->valuestring,name))return true;return false;}
static bool address_equal(const char *a,const char *b)
{if(!a||!b||strlen(a)!=17||strlen(b)!=17)return false;for(unsigned i=0;i<17;i++)if(tolower((unsigned char)a[i])!=tolower((unsigned char)b[i]))return false;return true;}
/* Enumerate before every add/retry. Component IDs belong to the gateway; a
 * matching address is reused, never deleted or replaced to make room. */
static dsr_result_t find_blu(const char *address,unsigned *cid,unsigned *free_cid)
{
 unsigned offset=0;uint16_t used=0;*cid=0;*free_cid=0;
 for(unsigned page=0;page<12;page++){
  cJSON *p=cJSON_CreateObject(),*response=NULL;if(!p)return DSR_IO;
  cJSON_AddBoolToObject(p,"dynamic_only",true);cJSON_AddNumberToObject(p,"offset",offset);
  dsr_result_t r=rpc("Shelly.GetComponents",p,&response,false,NULL);cJSON_Delete(p);if(r!=DSR_OK){cJSON_Delete(response);return r;}
  const cJSON *items=cJSON_GetObjectItemCaseSensitive(response,"components"),*total=cJSON_GetObjectItemCaseSensitive(response,"total"),*at=cJSON_GetObjectItemCaseSensitive(response,"offset"),*v;
  if(!cJSON_IsArray(items)||!cJSON_IsNumber(total)||total->valuedouble<0||total->valuedouble>1000||!cJSON_IsNumber(at)||at->valuedouble!=offset){cJSON_Delete(response);return DSR_BAD_RESPONSE;}
  cJSON_ArrayForEach(v,items){
   const cJSON *key=cJSON_GetObjectItemCaseSensitive(v,"key");if(!cJSON_IsString(key)){cJSON_Delete(response);return DSR_BAD_RESPONSE;}
   if(strncmp(key->valuestring,"bthomedevice:",13))continue;
   char *end;long id=strtol(key->valuestring+13,&end,10);if(!key->valuestring[13]||*end||id<200||id>299){cJSON_Delete(response);return DSR_BAD_RESPONSE;}
   if(id<216)used|=(uint16_t)(1u<<(id-200));
   const cJSON *cfg=cJSON_GetObjectItemCaseSensitive(v,"config"),*addr=cJSON_GetObjectItemCaseSensitive(cfg,"addr");
   if(!cJSON_IsString(addr)){cJSON_Delete(response);return DSR_BAD_RESPONSE;}
   if(address_equal(addr->valuestring,address)){if(*cid&&*cid!=(unsigned)id){cJSON_Delete(response);return DSR_CONFLICT;}*cid=(unsigned)id;}
  }
  unsigned got=(unsigned)cJSON_GetArraySize(items);bool more=total->valuedouble>offset+got;cJSON_Delete(response);if(!more){for(unsigned i=0;i<16;i++)if(!(used&(1u<<i))){*free_cid=200+i;break;}return *cid>=216?DSR_UNSUPPORTED:DSR_OK;}if(!got)return DSR_BAD_RESPONSE;offset+=got;
 }
 return DSR_FULL;
}
static dsr_result_t register_blu(void)
{
 char *end;unsigned long gen=strtoul(session->value,&end,10);if(!session->value[0]||*end||!gen||gen>UINT32_MAX)return DSR_INVALID;
 char address[18]={0},key[33]={0};
 if(!dsr_blu_gateway_key((uint32_t)gen,address,key))return DSR_AUTH_REQUIRED;
 cJSON *methods=NULL,*response=NULL,*params=NULL;dsr_result_t r=rpc("Shelly.ListMethods",NULL,&methods,false,NULL);unsigned cid=0,free_cid=0;bool attempted=false;
 if(r==DSR_OK&&(!has_method(methods,"BTHome.AddDevice")||!has_method(methods,"BTHome.AddSensor")||!has_method(methods,"BTHomeDevice.GetKnownObjects")))r=DSR_UNSUPPORTED;
 if(r==DSR_OK)r=find_blu(address,&cid,&free_cid);
 if(r==DSR_OK&&!cid){
  if(!free_cid)r=DSR_FULL;
  else {params=cJSON_CreateObject();cJSON *config=cJSON_CreateObject();if(!params||!config){cJSON_Delete(config);r=DSR_IO;}else{cJSON_AddStringToObject(config,"addr",address);if(key[0])cJSON_AddStringToObject(config,"key",key);cJSON_AddItemToObject(params,"config",config);cJSON_AddNumberToObject(params,"id",free_cid);r=rpc("BTHome.AddDevice",params,&response,true,&attempted);}}
  /* Even a lost ACK is resolved by address; AddDevice is never replayed. */
  if(attempted){unsigned found=0,unused=0;dsr_result_t read=find_blu(address,&found,&unused);if(read==DSR_OK&&found){cid=found;r=DSR_OK;}else if(r==DSR_OK)r=read==DSR_OK?DSR_BAD_RESPONSE:read;}
 }
 if(r==DSR_OK&&cid){
  session->working.blu_registered=true;session->working.blu_cid=(uint16_t)cid;
  cJSON_Delete(params);params=cJSON_CreateObject();cJSON_Delete(response);response=NULL;if(!params)r=DSR_IO;else{cJSON_AddNumberToObject(params,"id",cid);r=rpc("BTHomeDevice.GetKnownObjects",params,&response,false,NULL);}
  const cJSON *objects=cJSON_GetObjectItemCaseSensitive(response,"objects"),*obj;
  if(r==DSR_OK&&(!cJSON_IsArray(objects)||cJSON_GetArraySize(objects)>32))r=DSR_BAD_RESPONSE;
  unsigned created=0;
  if(r==DSR_OK)cJSON_ArrayForEach(obj,objects){
   const cJSON *id=cJSON_GetObjectItemCaseSensitive(obj,"obj_id"),*idx=cJSON_GetObjectItemCaseSensitive(obj,"idx"),*component=cJSON_GetObjectItemCaseSensitive(obj,"component");
   if(!cJSON_IsNumber(id)||id->valuedouble<0||id->valuedouble>255||floor(id->valuedouble)!=id->valuedouble||!cJSON_IsNumber(idx)||idx->valuedouble<0||idx->valuedouble>255||floor(idx->valuedouble)!=idx->valuedouble){r=DSR_BAD_RESPONSE;break;}
   if(!dsr_blu_sensor_shown((uint8_t)id->valueint)||id->valueint==0x3a||!cJSON_IsNull(component))continue;
   if(created++==8){r=DSR_FULL;break;}
   cJSON *add=cJSON_CreateObject(),*config=cJSON_CreateObject(),*ack=NULL;if(!add||!config){cJSON_Delete(add);cJSON_Delete(config);r=DSR_IO;break;}
   cJSON_AddStringToObject(config,"addr",address);cJSON_AddNumberToObject(config,"obj_id",id->valueint);cJSON_AddNumberToObject(config,"idx",idx->valueint);cJSON_AddItemToObject(add,"config",config);
   r=rpc("BTHome.AddSensor",add,&ack,true,&attempted);cJSON_Delete(ack);cJSON_Delete(add);if(r!=DSR_OK)break;
  }
  cJSON_Delete(response);response=NULL;
  if(r==DSR_OK){r=rpc("BTHomeDevice.GetStatus",params,&response,false,NULL);dsr_telemetry_t sample;if(r==DSR_OK)r=dsr_blu_parse_device(response,cid,&sample);
   if(r==DSR_OK){time_t now=time(NULL);session->working.blu_receiving=(sample.blu.flags&DSR_BLU_VALID)&&!(sample.blu.errors&DSR_BLU_ERROR_MASK)&&now>1735689600&&sample.blu_updated_s<=(uint64_t)now&&(uint64_t)now-sample.blu_updated_s<=30;}}
 }
 wipe(key,sizeof(key));cJSON_Delete(methods);cJSON_Delete(params);cJSON_Delete(response);return r;
}
void dsr_device_settings_execute(const dsr_job_t *job,dsr_event_t *event)
{
 event->result=DSR_INVALID;if(!dsr_device_settings_admitted(job)||!lock())return;
 session->queued=false;session->running=true;session->working=session->view;unsigned action=session->action;unlock();
 dsr_http_job_guard_t previous=dsr_http_job_guard(guard);dsr_result_t r=guard();cJSON *fresh=NULL,*patch=NULL,*response=NULL,*params=NULL;bool attempted=false,restart=false,uncertain=false;
 if(r==DSR_OK&&(!dsr_wifi_connected()||dsr_wifi_temporary_active()))r=DSR_OFFLINE;
 if(r==DSR_OK&&action==DSR_SETTINGS_LOAD)r=load();
 else if(r==DSR_OK&&action==DSR_SETTINGS_BLU_ADD)r=register_blu();
 else if(r==DSR_OK&&(action==DSR_SETTINGS_GROUP||action==DSR_SETTINGS_SAVE)){
  unsigned group=action==DSR_SETTINGS_GROUP?session->selection:session->working.selected;
  if(group>=session->working.group_count)r=DSR_INVALID;
  else {
   const dsr_setting_group_t *g=&session->working.groups[group];r=get_group(g,&fresh);
   if(r==DSR_OK&&action==DSR_SETTINGS_SAVE){
    unsigned field=session->selection;const dsr_setting_field_t *f=dsr_setting_field(field);bool included=false;
    for(unsigned i=0;i<session->working.field_count;i++)if(session->working.fields[i]==field)included=true;
    if(!f||!included||strcmp(f->component,g->component))r=DSR_INVALID;
    else if(!dsr_setting_equal(fresh,session->baseline))r=DSR_CONFLICT;
    else if(!strcmp(f->component,"WiFi")&&(!strcmp(f->path,"sta.ssid")||!strcmp(f->path,"sta1.ssid"))){
     cJSON *network=cJSON_Parse(session->value);const cJSON *ssid=cJSON_GetObjectItemCaseSensitive(network,"ssid"),*pass=cJSON_GetObjectItemCaseSensitive(network,"pass");
     r=cJSON_IsString(ssid)&&cJSON_IsString(pass)?dsr_setting_network_patch(session->baseline,field,ssid->valuestring,pass->valuestring,&patch):DSR_INVALID;dsr_setting_json_delete(network);
    }else r=dsr_setting_patch(session->baseline,field,session->value,&patch);
    if(r==DSR_OK){params=parameters(g);cJSON *copy=cJSON_Duplicate(patch,true);if(!params||!copy||!cJSON_AddItemToObject(params,"config",copy)){cJSON_Delete(copy);r=DSR_IO;}}
    if(r==DSR_OK){char method[48];snprintf(method,sizeof(method),"%s.SetConfig",g->component);r=rpc(method,params,&response,true,&attempted);restart=cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(response,"restart_required"));}
    if(attempted){
     uncertain=true;
     if(r==DSR_OK){cJSON_Delete(fresh);fresh=NULL;r=get_group(g,&fresh);
      if(r==DSR_OK&&f->type==DSR_SETTING_SECRET){/* Write-only: ACK is not a credential read-back. */}
      else if(r==DSR_OK&&dsr_setting_equal(dsr_setting_value(patch,f->path),dsr_setting_value(fresh,f->path)))uncertain=false;
      else if(r==DSR_OK)r=DSR_BAD_RESPONSE;
     }
    }
   }
   if(r==DSR_OK&&fresh){cJSON_Delete(session->baseline);session->baseline=fresh;fresh=NULL;session->working.selected=(uint8_t)group;session->working.field_count=(uint8_t)dsr_setting_fields(g,session->baseline,session->working.fields,DSR_SETTINGS_FIELDS);session->working.ready=true;}
  }
 }
 (void)dsr_http_job_guard(previous);dsr_setting_json_delete(fresh);dsr_setting_json_delete(patch);dsr_setting_json_delete(response);dsr_setting_json_delete(params);
 if(!lock())return;
 session->running=false;session->working.busy=false;session->working.result=r;session->working.uncertain=uncertain;session->working.restart_required=restart;session->working.serial++;session->view=session->working;wipe(session->value,sizeof(session->value));event->result=r;
 if(session->closing)dispose();
 unlock();
}
void dsr_device_settings_snapshot(dsr_device_settings_snapshot_t *out)
{if(!out)return;memset(out,0,sizeof(*out));out->result=DSR_UNSUPPORTED;if(!lock())return;if(session)*out=session->view;unlock();}
bool dsr_device_settings_value(unsigned field,char *out,size_t size)
{
 if(!out||!size)return false;
 out[0]=0;if(!lock())return false;bool ok=session&&!session->running&&!session->queued&&session->baseline;const dsr_setting_field_t *f=dsr_setting_field(field);
 if(ok&&f){const cJSON *v=dsr_setting_value(session->baseline,f->path);if(f->type==DSR_SETTING_SECRET)snprintf(out,size,"%s","");else if(cJSON_IsBool(v))snprintf(out,size,"%s",cJSON_IsTrue(v)?"true":"false");else if(cJSON_IsNumber(v))snprintf(out,size,"%.10g",v->valuedouble);else if(cJSON_IsString(v))snprintf(out,size,"%s",v->valuestring);else ok=cJSON_IsNull(v);}else ok=false;unlock();return ok;
}
void dsr_device_settings_close(void)
{if(!lock())return;if(session){session->closing=true;if(!session->running)dispose();}unlock();}
bool dsr_device_settings_blu_connection(dsr_connection_t *out)
{if(!out||!lock())return false;bool ok=session&&!session->running&&!session->queued&&session->view.blu_registered&&session->view.blu_cid>=200&&session->view.blu_cid<216;if(ok){*out=session->connection;out->device.channel=(uint8_t)(session->view.blu_cid-200);out->device.endpoint_kind=DSR_ENDPOINT_BLU;}unlock();return ok;}
