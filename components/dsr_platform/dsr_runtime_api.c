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
bool dsr_rgb_parse(const char *json,dsr_device_t *device,dsr_rgb_intent_t *intent)
{
 if(!json||!device||!intent||strlen(json)>256||strstr(json,"\\u0000")||!dsr_config_json_depth(json))return false;
 cJSON *root=cJSON_ParseWithOpts(json,NULL,true);bool valid=cJSON_IsObject(root);unsigned count=0;const cJSON *v;
 cJSON_ArrayForEach(v,root){
  count++;if(!v->string||(strcmp(v->string,"device_id")&&strcmp(v->string,"generation")&&strcmp(v->string,"endpoint_kind")&&strcmp(v->string,"channel")&&strcmp(v->string,"command")&&strcmp(v->string,"brightness")&&strcmp(v->string,"rgb")))valid=false;
  for(const cJSON *b=v->next;b;b=b->next)if(b->string&&v->string&&!strcmp(b->string,v->string))valid=false;
 }
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(root,"device_id"),*generation=cJSON_GetObjectItemCaseSensitive(root,"generation"),*kind=cJSON_GetObjectItemCaseSensitive(root,"endpoint_kind"),*channel=cJSON_GetObjectItemCaseSensitive(root,"channel"),*command=cJSON_GetObjectItemCaseSensitive(root,"command"),*brightness=cJSON_GetObjectItemCaseSensitive(root,"brightness"),*rgb=cJSON_GetObjectItemCaseSensitive(root,"rgb");
 dsr_rgb_intent_t candidate={0};
 if(cJSON_IsString(command))candidate.command=!strcmp(command->valuestring,"on")?DSR_RGB_ON:!strcmp(command->valuestring,"off")?DSR_RGB_OFF:!strcmp(command->valuestring,"brightness")?DSR_RGB_SET_BRIGHTNESS:!strcmp(command->valuestring,"rgb")?DSR_RGB_SET_COLOR:DSR_RGB_NONE;
 valid=valid&&cJSON_IsString(id)&&id->valuestring[0]&&strlen(id->valuestring)<sizeof(device->device_id)&&cJSON_IsNumber(generation)&&generation->valuedouble==DSR_GEN2_PLUS&&cJSON_IsNumber(kind)&&kind->valuedouble==DSR_ENDPOINT_RGB&&cJSON_IsNumber(channel)&&channel->valuedouble==0;
 if(candidate.command==DSR_RGB_SET_BRIGHTNESS){valid=valid&&!rgb&&count==6&&cJSON_IsNumber(brightness)&&brightness->valuedouble>=0&&brightness->valuedouble<=100&&brightness->valuedouble==brightness->valueint;if(valid)candidate.brightness=(uint8_t)brightness->valueint;}
 else if(candidate.command==DSR_RGB_SET_COLOR){
  valid=valid&&count==6&&!brightness&&cJSON_IsArray(rgb)&&cJSON_GetArraySize(rgb)==3;
  unsigned i=0;for(const cJSON *v=rgb?rgb->child:NULL;valid&&v;v=v->next,i++){valid=cJSON_IsNumber(v)&&v->valuedouble>=0&&v->valuedouble<=255&&v->valuedouble==v->valueint;if(valid)candidate.rgb[i]=(uint8_t)v->valueint;}
 }else valid=valid&&count==5&&!brightness&&!rgb;
 valid=valid&&dsr_rgb_intent_valid(candidate);
 if(valid)for(const unsigned char *p=(const unsigned char *)id->valuestring;*p;p++)if(!(isalnum(*p)||*p=='-'||*p=='_'))valid=false;
 if(valid){memset(device,0,sizeof(*device));strcpy(device->device_id,id->valuestring);device->generation=DSR_GEN2_PLUS;device->endpoint_kind=DSR_ENDPOINT_RGB;device->channel=(uint8_t)channel->valueint;*intent=candidate;}
 cJSON_Delete(root);return valid;
}
bool dsr_ht_refresh_parse(const char *json,dsr_device_t *device)
{
 if(!json||!device||strlen(json)>256||strstr(json,"\\u0000")||!dsr_config_json_depth(json))return false;
 cJSON *r=cJSON_ParseWithOpts(json,NULL,true);bool valid=cJSON_IsObject(r);unsigned count=0;
 for(const cJSON *v=r?r->child:NULL;v;v=v->next){count++;if(!v->string||(strcmp(v->string,"device_id")&&strcmp(v->string,"generation")&&strcmp(v->string,"endpoint_kind")&&strcmp(v->string,"channel")))valid=false;for(const cJSON *b=v->next;b;b=b->next)if(b->string&&v->string&&!strcmp(v->string,b->string))valid=false;}
 const cJSON *id=cJSON_GetObjectItemCaseSensitive(r,"device_id"),*gen=cJSON_GetObjectItemCaseSensitive(r,"generation"),*kind=cJSON_GetObjectItemCaseSensitive(r,"endpoint_kind"),*channel=cJSON_GetObjectItemCaseSensitive(r,"channel");
 valid=valid&&count==4&&cJSON_IsString(id)&&id->valuestring[0]&&strlen(id->valuestring)<sizeof(device->device_id)&&cJSON_IsNumber(gen)&&gen->valuedouble==DSR_GEN2_PLUS&&cJSON_IsNumber(kind)&&(kind->valuedouble==DSR_ENDPOINT_HT||kind->valuedouble==DSR_ENDPOINT_BLU)&&cJSON_IsNumber(channel)&&channel->valuedouble==channel->valueint&&channel->valueint>=0&&channel->valueint<=(kind->valuedouble==DSR_ENDPOINT_HT?0:DSR_MAX_CHANNEL);
 if(valid)for(const unsigned char *p=(const unsigned char *)id->valuestring;*p;p++)if(!(isalnum(*p)||*p=='-'||*p=='_'))valid=false;
 if(valid){uint8_t k=(uint8_t)kind->valueint,ch=(uint8_t)channel->valueint;memset(device,0,sizeof(*device));strcpy(device->device_id,id->valuestring);device->generation=DSR_GEN2_PLUS;device->endpoint_kind=k;device->channel=ch;}
 cJSON_Delete(r);return valid;
}
static bool nullable(cJSON *o,const char *key,bool has,double value)
{return has?cJSON_AddNumberToObject(o,key,value)!=NULL:cJSON_AddNullToObject(o,key)!=NULL;}
char *dsr_runtime_json(uint32_t boot_id)
{
 dsr_runtime_device_t *devices=calloc(DSR_MAX_DEVICES,sizeof(*devices));if(!devices)return NULL;
 size_t count=dsr_workspace_runtime_snapshot(devices,DSR_MAX_DEVICES);
 cJSON *root=cJSON_CreateObject(),*array=cJSON_CreateArray();bool ok=root&&array;
 if(ok)ok=cJSON_AddNumberToObject(root,"boot_id",boot_id)!=NULL;
 bool attached=false;
 if(ok){attached=cJSON_AddItemToObject(root,"devices",array);ok=attached;}
 for(size_t i=0;i<count&&ok;i++){
  const dsr_runtime_device_t *r=&devices[i];const dsr_device_t *d=&r->device;bool sensor=DSR_ENDPOINT_IS_SENSOR(d->endpoint_kind);cJSON *o=cJSON_CreateObject();
  if(!o){ok=false;break;}cJSON_AddItemToArray(array,o);
  ok=cJSON_AddStringToObject(o,"device_id",d->device_id)&&cJSON_AddNumberToObject(o,"generation",d->generation)&&cJSON_AddNumberToObject(o,"endpoint_kind",d->endpoint_kind)&&cJSON_AddNumberToObject(o,"channel",d->channel)&&cJSON_AddStringToObject(o,"host",d->host)&&cJSON_AddStringToObject(o,"name",d->name)&&cJSON_AddBoolToObject(o,"online",d->online)&&(sensor?cJSON_AddNullToObject(o,"output"):cJSON_AddBoolToObject(o,"output",d->output))&&cJSON_AddBoolToObject(o,"has_switch",!sensor&&d->has_switch)&&cJSON_AddBoolToObject(o,"has_power",!sensor&&d->has_power)&&nullable(o,"power_w",!sensor,d->power_w)&&cJSON_AddBoolToObject(o,"has_voltage",!sensor&&r->telemetry.has_voltage)&&nullable(o,"voltage_v",!sensor,r->telemetry.voltage_v)&&cJSON_AddBoolToObject(o,"has_current",!sensor&&r->telemetry.has_current)&&nullable(o,"current_a",!sensor,r->telemetry.current_a)&&cJSON_AddBoolToObject(o,"has_energy",!sensor&&r->telemetry.has_energy)&&nullable(o,"energy_wh",!sensor,r->telemetry.energy_wh)&&cJSON_AddBoolToObject(o,"pending",r->pending)&&cJSON_AddNumberToObject(o,"error",r->error)&&cJSON_AddBoolToObject(o,"has_sample_time",r->has_sample_time)&&cJSON_AddNumberToObject(o,"sample_age_ms",r->sample_age_ms)&&cJSON_AddNumberToObject(o,"operation_id",r->operation_id);
  if(ok&&d->endpoint_kind==DSR_ENDPOINT_BLU){const dsr_telemetry_t *t=&r->telemetry;const dsr_blu_status_t *b=&t->blu;
   cJSON *v=cJSON_AddObjectToObject(o,"blu");ok=v!=NULL;
   ok=ok&&cJSON_AddBoolToObject(v,"valid",(b->flags&DSR_BLU_VALID)!=0)
    &&((b->flags&DSR_BLU_HAS_MOTION)?cJSON_AddBoolToObject(v,"motion",(b->flags&DSR_BLU_MOTION)!=0)!=NULL:cJSON_AddNullToObject(v,"motion")!=NULL)
    &&((b->flags&DSR_BLU_HAS_OPEN)?cJSON_AddBoolToObject(v,"open",(b->flags&DSR_BLU_OPEN)!=0)!=NULL:cJSON_AddNullToObject(v,"open")!=NULL)
    &&nullable(v,"illuminance_lux",t->has_illuminance&&!(b->errors&DSR_BLU_TRV),t->illuminance_lux)&&nullable(v,"battery_pct",t->has_blu_battery,t->blu_battery_pct)
    &&nullable(v,"temperature_c",t->has_blu_temperature,t->blu_temperature_c)&&nullable(v,"humidity_pct",(b->flags&DSR_BLU_HAS_HUMIDITY)&&!(b->errors&DSR_BLU_TRV),b->humidity)
    &&nullable(v,"button",(b->flags&DSR_BLU_HAS_BUTTON)!=0,b->button)&&nullable(v,"rssi",(b->flags&DSR_BLU_HAS_RSSI)!=0,b->rssi)
    &&nullable(v,"updated_s",t->blu_updated_s!=0,t->blu_updated_s)&&cJSON_AddNumberToObject(v,"errors",b->errors&DSR_BLU_ERROR_MASK);
   if(ok&&(b->errors&DSR_BLU_TRV)){bool known=(b->errors&DSR_BLU_TRV_TARGET)!=0;cJSON *trv=cJSON_AddObjectToObject(v,"trv");
    ok=trv&&nullable(trv,"target_c",known,t->trv_target_c)&&nullable(trv,"current_c",known,t->trv_current_c)&&nullable(trv,"valve_pct",known,b->trv_pos)&&cJSON_AddBoolToObject(trv,"fault",(b->errors&DSR_BLU_TRV_FAULT)!=0);}
   else if(ok)ok=cJSON_AddNullToObject(v,"trv")!=NULL;
  }
  if(ok&&d->endpoint_kind==DSR_ENDPOINT_HT){const dsr_telemetry_t *t=&r->telemetry;
   ok=cJSON_AddBoolToObject(o,"has_temperature",t->has_temperature)&&nullable(o,"temperature_c",t->has_temperature,t->temperature_c)&&cJSON_AddBoolToObject(o,"has_humidity",t->has_humidity)&&nullable(o,"humidity_pct",t->has_humidity,t->humidity_pct)&&cJSON_AddBoolToObject(o,"has_battery",t->has_battery)&&nullable(o,"battery_pct",t->has_battery,t->battery_pct)&&cJSON_AddNumberToObject(o,"sensor_errors",t->ht.errors)&&cJSON_AddBoolToObject(o,"measurement_time_known",false)&&cJSON_AddBoolToObject(o,"can_refresh",r->can_refresh);
   if(ok)ok=(t->ht.flags&DSR_HT_EXTERNAL_KNOWN)?cJSON_AddBoolToObject(o,"external_present",(t->ht.flags&DSR_HT_EXTERNAL_PRESENT)!=0)!=NULL:cJSON_AddNullToObject(o,"external_present")!=NULL;
  }
  if(ok&&d->endpoint_kind==DSR_ENDPOINT_COVER){const dsr_cover_status_t *c=&r->telemetry.cover;
   ok=cJSON_AddNumberToObject(o,"cover_state",c->state)&&cJSON_AddBoolToObject(o,"has_position",(c->flags&DSR_COVER_POSITION)!=0)&&cJSON_AddNumberToObject(o,"position",c->current_pos)&&cJSON_AddBoolToObject(o,"has_target",(c->flags&DSR_COVER_TARGET)!=0)&&cJSON_AddNumberToObject(o,"target",c->target_pos)&&cJSON_AddBoolToObject(o,"can_move",dsr_cover_can_move(c,(dsr_cover_intent_t){.command=DSR_COVER_OPEN}))&&cJSON_AddBoolToObject(o,"can_position",dsr_cover_can_move(c,(dsr_cover_intent_t){.command=DSR_COVER_GOTO}))&&cJSON_AddNumberToObject(o,"cover_errors",c->errors)&&cJSON_AddBoolToObject(o,"can_stop",r->can_stop)&&cJSON_AddBoolToObject(o,"stop_pending",r->stop_pending);
  }
  if(ok&&d->endpoint_kind==DSR_ENDPOINT_RGB){const dsr_rgb_status_t *c=&r->telemetry.rgb;
   cJSON *color=cJSON_AddArrayToObject(o,"rgb");ok=color&&cJSON_AddItemToArray(color,cJSON_CreateNumber(c->r))&&cJSON_AddItemToArray(color,cJSON_CreateNumber(c->g))&&cJSON_AddItemToArray(color,cJSON_CreateNumber(c->b));
   ok=ok&&cJSON_AddBoolToObject(o,"has_rgb",(c->flags&DSR_RGB_VALID)!=0)&&cJSON_AddNumberToObject(o,"brightness",c->brightness)&&cJSON_AddBoolToObject(o,"can_rgb",dsr_rgb_can_set(c,(dsr_rgb_intent_t){.command=DSR_RGB_ON}))&&cJSON_AddBoolToObject(o,"transitioning",(c->flags&DSR_RGB_TRANSITION)!=0)&&cJSON_AddBoolToObject(o,"rgb_error",(c->flags&DSR_RGB_ERROR)!=0);
  }
  if(ok&&d->endpoint_kind==DSR_ENDPOINT_LIGHT){const dsr_light_status_t *l=&r->telemetry.light;
   ok=cJSON_AddBoolToObject(o,"has_brightness",(l->flags&DSR_LIGHT_BRIGHTNESS)!=0)&&cJSON_AddNumberToObject(o,"brightness",l->brightness)&&cJSON_AddBoolToObject(o,"can_light",dsr_light_can_set(l,(dsr_light_intent_t){.command=DSR_LIGHT_ON}))&&cJSON_AddBoolToObject(o,"transitioning",(l->flags&DSR_LIGHT_TRANSITION)!=0)&&cJSON_AddBoolToObject(o,"has_brightness_target",(l->flags&DSR_LIGHT_TARGET)!=0)&&cJSON_AddNumberToObject(o,"brightness_target",l->target)&&cJSON_AddNumberToObject(o,"light_errors",l->errors)&&cJSON_AddNumberToObject(o,"light_conditions",l->conditions);
  }
 }
 char *result=ok?cJSON_PrintUnformatted(root):NULL;
 if(!attached){cJSON_Delete(array);}cJSON_Delete(root);free(devices);return result;
}
