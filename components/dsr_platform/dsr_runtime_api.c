#include "dsr_runtime_api.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

bool dsr_output_parse(const char *json,dsr_device_t *device,bool *on)
{
 if(!json||!device||!on||strlen(json)>256||strstr(json,"\\u0000")||!dsr_config_json_depth(json))return false;
 cJSON *root=cJSON_ParseWithOpts(json,NULL,true);bool valid=cJSON_IsObject(root);unsigned count=0;const cJSON *v;
 cJSON_ArrayForEach(v,root){
  count++;if(!v->string||(strcmp(v->string,"device_id")&&strcmp(v->string,"generation")&&strcmp(v->string,"channel")&&strcmp(v->string,"on")))valid=false;
  for(const cJSON *b=v->next;b;b=b->next)if(b->string&&v->string&&!strcmp(b->string,v->string))valid=false;
 }
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"device_id"),*generation=cJSON_GetObjectItemCaseSensitive(root,"generation"),*channel=cJSON_GetObjectItemCaseSensitive(root,"channel"),*output=cJSON_GetObjectItemCaseSensitive(root,"on");
 valid=valid&&count==4&&cJSON_IsString(id)&&id->valuestring[0]&&strlen(id->valuestring)<sizeof(device->device_id)&&cJSON_IsNumber(generation)&&(generation->valuedouble==1||generation->valuedouble==2)&&cJSON_IsNumber(channel)&&channel->valuedouble>=0&&channel->valuedouble<=15&&channel->valuedouble==channel->valueint&&cJSON_IsBool(output);
 if(valid)for(const unsigned char *p=(const unsigned char *)id->valuestring;*p;p++)if(!(isalnum(*p)||*p=='-'||*p=='_'))valid=false;
 if(valid){memset(device,0,sizeof(*device));strcpy(device->device_id,id->valuestring);device->generation=(dsr_generation_t)generation->valueint;device->channel=(uint8_t)channel->valueint;*on=cJSON_IsTrue(output);}
 cJSON_Delete(root);return valid;
}
bool dsr_cover_parse(const char *json,dsr_device_t *device,dsr_cover_intent_t *intent)
{
 if(!json||!device||!intent||strlen(json)>256||strstr(json,"\\u0000")||!dsr_config_json_depth(json))return false;
 cJSON *root=cJSON_ParseWithOpts(json,NULL,true);bool valid=cJSON_IsObject(root);unsigned count=0;const cJSON *v;
 cJSON_ArrayForEach(v,root){
  count++;if(!v->string||(strcmp(v->string,"device_id")&&strcmp(v->string,"generation")&&strcmp(v->string,"endpoint_kind")&&strcmp(v->string,"channel")&&strcmp(v->string,"command")&&strcmp(v->string,"position")))valid=false;
  for(const cJSON *b=v->next;b;b=b->next)if(b->string&&v->string&&!strcmp(b->string,v->string))valid=false;
 }
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"device_id"),*generation=cJSON_GetObjectItemCaseSensitive(root,"generation"),*kind=cJSON_GetObjectItemCaseSensitive(root,"endpoint_kind"),*channel=cJSON_GetObjectItemCaseSensitive(root,"channel"),*command=cJSON_GetObjectItemCaseSensitive(root,"command"),*position=cJSON_GetObjectItemCaseSensitive(root,"position");
 dsr_cover_intent_t candidate={0};
 if(cJSON_IsString(command))candidate.command=!strcmp(command->valuestring,"open")?DSR_COVER_OPEN:!strcmp(command->valuestring,"close")?DSR_COVER_CLOSE:!strcmp(command->valuestring,"stop")?DSR_COVER_STOP:!strcmp(command->valuestring,"position")?DSR_COVER_GOTO:DSR_COVER_NONE;
 valid=valid&&cJSON_IsString(id)&&id->valuestring[0]&&strlen(id->valuestring)<sizeof(device->device_id)&&cJSON_IsNumber(generation)&&generation->valuedouble==DSR_GEN2_PLUS&&cJSON_IsNumber(kind)&&kind->valuedouble==DSR_ENDPOINT_COVER&&cJSON_IsNumber(channel)&&channel->valuedouble>=0&&channel->valuedouble<=15&&channel->valuedouble==channel->valueint;
 if(candidate.command==DSR_COVER_GOTO){valid=valid&&count==6&&cJSON_IsNumber(position)&&position->valuedouble>=0&&position->valuedouble<=100&&position->valuedouble==position->valueint;if(valid)candidate.position=(uint8_t)position->valueint;}
 else valid=valid&&count==5&&!position;
 valid=valid&&dsr_cover_intent_valid(candidate);
 if(valid)for(const unsigned char *p=(const unsigned char *)id->valuestring;*p;p++)if(!(isalnum(*p)||*p=='-'||*p=='_'))valid=false;
 if(valid){memset(device,0,sizeof(*device));strcpy(device->device_id,id->valuestring);device->generation=DSR_GEN2_PLUS;device->endpoint_kind=DSR_ENDPOINT_COVER;device->channel=(uint8_t)channel->valueint;*intent=candidate;}
 cJSON_Delete(root);return valid;
}
bool dsr_light_parse(const char *json,dsr_device_t *device,dsr_light_intent_t *intent)
{
 if(!json||!device||!intent||strlen(json)>256||strstr(json,"\\u0000")||!dsr_config_json_depth(json))return false;
 cJSON *root=cJSON_ParseWithOpts(json,NULL,true);bool valid=cJSON_IsObject(root);unsigned count=0;const cJSON *v;
 cJSON_ArrayForEach(v,root){
  count++;if(!v->string||(strcmp(v->string,"device_id")&&strcmp(v->string,"generation")&&strcmp(v->string,"endpoint_kind")&&strcmp(v->string,"channel")&&strcmp(v->string,"command")&&strcmp(v->string,"brightness")))valid=false;
  for(const cJSON *b=v->next;b;b=b->next)if(b->string&&v->string&&!strcmp(b->string,v->string))valid=false;
 }
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"device_id"),*generation=cJSON_GetObjectItemCaseSensitive(root,"generation"),*kind=cJSON_GetObjectItemCaseSensitive(root,"endpoint_kind"),*channel=cJSON_GetObjectItemCaseSensitive(root,"channel"),*command=cJSON_GetObjectItemCaseSensitive(root,"command"),*brightness=cJSON_GetObjectItemCaseSensitive(root,"brightness");
 dsr_light_intent_t candidate={0};
 if(cJSON_IsString(command))candidate.command=!strcmp(command->valuestring,"on")?DSR_LIGHT_ON:!strcmp(command->valuestring,"off")?DSR_LIGHT_OFF:!strcmp(command->valuestring,"brightness")?DSR_LIGHT_SET_BRIGHTNESS:DSR_LIGHT_NONE;
 valid=valid&&cJSON_IsString(id)&&id->valuestring[0]&&strlen(id->valuestring)<sizeof(device->device_id)&&cJSON_IsNumber(generation)&&generation->valuedouble==DSR_GEN2_PLUS&&cJSON_IsNumber(kind)&&kind->valuedouble==DSR_ENDPOINT_LIGHT&&cJSON_IsNumber(channel)&&channel->valuedouble>=0&&channel->valuedouble<=15&&channel->valuedouble==channel->valueint;
 if(candidate.command==DSR_LIGHT_SET_BRIGHTNESS){valid=valid&&count==6&&cJSON_IsNumber(brightness)&&brightness->valuedouble>=0&&brightness->valuedouble<=100&&brightness->valuedouble==brightness->valueint;if(valid)candidate.brightness=(uint8_t)brightness->valueint;}
 else valid=valid&&count==5&&!brightness;
 valid=valid&&dsr_light_intent_valid(candidate);
 if(valid)for(const unsigned char *p=(const unsigned char *)id->valuestring;*p;p++)if(!(isalnum(*p)||*p=='-'||*p=='_'))valid=false;
 if(valid){memset(device,0,sizeof(*device));strcpy(device->device_id,id->valuestring);device->generation=DSR_GEN2_PLUS;device->endpoint_kind=DSR_ENDPOINT_LIGHT;device->channel=(uint8_t)channel->valueint;*intent=candidate;}
 cJSON_Delete(root);return valid;
}
char *dsr_runtime_json(uint32_t boot_id)
{
 dsr_runtime_device_t *devices=calloc(DSR_MAX_DEVICES,sizeof(*devices));if(!devices)return NULL;
 size_t count=dsr_workspace_runtime_snapshot(devices,DSR_MAX_DEVICES);
 cJSON *root=cJSON_CreateObject(),*array=cJSON_CreateArray();bool ok=root&&array;
 if(ok)ok=cJSON_AddNumberToObject(root,"boot_id",boot_id)!=NULL;
 bool attached=false;
 if(ok){attached=cJSON_AddItemToObject(root,"devices",array);ok=attached;}
 for(size_t i=0;i<count&&ok;i++){
  const dsr_runtime_device_t *r=&devices[i];const dsr_device_t *d=&r->device;cJSON *o=cJSON_CreateObject();
  if(!o){ok=false;break;}cJSON_AddItemToArray(array,o);
  ok=cJSON_AddStringToObject(o,"device_id",d->device_id)&&cJSON_AddNumberToObject(o,"generation",d->generation)&&cJSON_AddNumberToObject(o,"endpoint_kind",d->endpoint_kind)&&cJSON_AddNumberToObject(o,"channel",d->channel)&&cJSON_AddStringToObject(o,"host",d->host)&&cJSON_AddStringToObject(o,"name",d->name)&&cJSON_AddBoolToObject(o,"online",d->online)&&cJSON_AddBoolToObject(o,"output",d->output)&&cJSON_AddBoolToObject(o,"has_switch",d->has_switch)&&cJSON_AddBoolToObject(o,"has_power",d->has_power)&&cJSON_AddNumberToObject(o,"power_w",d->power_w)&&cJSON_AddBoolToObject(o,"has_voltage",r->telemetry.has_voltage)&&cJSON_AddNumberToObject(o,"voltage_v",r->telemetry.voltage_v)&&cJSON_AddBoolToObject(o,"has_current",r->telemetry.has_current)&&cJSON_AddNumberToObject(o,"current_a",r->telemetry.current_a)&&cJSON_AddBoolToObject(o,"has_energy",r->telemetry.has_energy)&&cJSON_AddNumberToObject(o,"energy_wh",r->telemetry.energy_wh)&&cJSON_AddBoolToObject(o,"pending",r->pending)&&cJSON_AddNumberToObject(o,"error",r->error)&&cJSON_AddBoolToObject(o,"has_sample_time",r->has_sample_time)&&cJSON_AddNumberToObject(o,"sample_age_ms",r->sample_age_ms)&&cJSON_AddNumberToObject(o,"operation_id",r->operation_id);
  if(ok&&d->endpoint_kind==DSR_ENDPOINT_COVER){const dsr_cover_status_t *c=&r->telemetry.cover;
   ok=cJSON_AddNumberToObject(o,"cover_state",c->state)&&cJSON_AddBoolToObject(o,"has_position",(c->flags&DSR_COVER_POSITION)!=0)&&cJSON_AddNumberToObject(o,"position",c->current_pos)&&cJSON_AddBoolToObject(o,"has_target",(c->flags&DSR_COVER_TARGET)!=0)&&cJSON_AddNumberToObject(o,"target",c->target_pos)&&cJSON_AddBoolToObject(o,"can_move",dsr_cover_can_move(c,(dsr_cover_intent_t){.command=DSR_COVER_OPEN}))&&cJSON_AddBoolToObject(o,"can_position",dsr_cover_can_move(c,(dsr_cover_intent_t){.command=DSR_COVER_GOTO}))&&cJSON_AddNumberToObject(o,"cover_errors",c->errors)&&cJSON_AddBoolToObject(o,"can_stop",r->can_stop)&&cJSON_AddBoolToObject(o,"stop_pending",r->stop_pending);
  }
  if(ok&&d->endpoint_kind==DSR_ENDPOINT_LIGHT){const dsr_light_status_t *l=&r->telemetry.light;
   ok=cJSON_AddBoolToObject(o,"has_brightness",(l->flags&DSR_LIGHT_BRIGHTNESS)!=0)&&cJSON_AddNumberToObject(o,"brightness",l->brightness)&&cJSON_AddBoolToObject(o,"can_light",dsr_light_can_set(l,(dsr_light_intent_t){.command=DSR_LIGHT_ON}))&&cJSON_AddBoolToObject(o,"transitioning",(l->flags&DSR_LIGHT_TRANSITION)!=0)&&cJSON_AddBoolToObject(o,"has_brightness_target",(l->flags&DSR_LIGHT_TARGET)!=0)&&cJSON_AddNumberToObject(o,"brightness_target",l->target)&&cJSON_AddNumberToObject(o,"light_errors",l->errors)&&cJSON_AddNumberToObject(o,"light_conditions",l->conditions);
  }
 }
 char *result=ok?cJSON_PrintUnformatted(root):NULL;
 if(!attached){cJSON_Delete(array);}cJSON_Delete(root);free(devices);return result;
}
