#include "dsr_backup_sources_model.h"
#include "dsr_backup_json.h"
#include "dsr_backup_format.h"
#include "dsr_config.h"
#include "dsr_automation_model.h"
#include "dsr_automation_config.h"
#include "dsr_room_photo_ids.h"
#include <stdlib.h>
#include <string.h>
#define KEYS(object,fields) dsr_backup_json_keys(object,fields,sizeof(fields)/sizeof(fields[0]))
static const cJSON *get(const cJSON *o,const char *key){return cJSON_GetObjectItemCaseSensitive(o,key);}
static bool number(const cJSON *o,const char *key,uint32_t max,uint32_t *out){return dsr_backup_json_number(o,key,max,out);}
static bool string(const cJSON *o,const char *key,char *out,size_t size,bool hex){return dsr_backup_json_string(o,key,out,size,hex);}
bool dsr_backup_photo_builtin(const char *id)
{
#define ROOM(stem,label,digest) digest,
 static const char *const ids[]={DSR_ROOM_PHOTO_ENTRIES(ROOM)};
#undef ROOM
 for(unsigned i=0;id&&i<sizeof(ids)/sizeof(ids[0]);i++)if(!strcmp(ids[i],id))return true;
 return false;
}
static bool settings_decode(const cJSON *o,dsr_settings_t *s)
{
 static const char *const fields[]={"ssid","password","brightness","wifi_security","idle_seconds"};uint32_t b,w,t;
 if(!KEYS(o,fields)||!string(o,"ssid",s->ssid,sizeof(s->ssid),true)||!string(o,"password",s->password,sizeof(s->password),true)||!number(o,"brightness",100,&b)||!number(o,"wifi_security",DSR_WIFI_UNSUPPORTED-1,&w)||!number(o,"idle_seconds",3600,&t))return false;
 s->brightness=(uint8_t)b;s->wifi_security=(uint8_t)w;s->idle_seconds=(uint16_t)t;
 return !s->ssid[0]||dsr_wifi_settings_valid(s);
}
static bool battery_decode(const cJSON *o,dsr_battery_profile_t *p)
{
 static const char *const fields[]={"kind","nominal_mv","capacity_mah","empty_mv","mid_mv","full_mv","gain_permille","trend_enabled"};uint32_t v;
 if(!KEYS(o,fields)||!number(o,"kind",2,&v))return false;
 p->kind=(uint8_t)v;
#define FIELD(name) if(!number(o,#name,UINT16_MAX,&v))return false; p->name=(uint16_t)v
 FIELD(nominal_mv);FIELD(capacity_mah);FIELD(empty_mv);FIELD(mid_mv);FIELD(full_mv);FIELD(gain_permille);
#undef FIELD
 if(!cJSON_IsBool(get(o,"trend_enabled")))return false;
 p->trend_enabled=cJSON_IsTrue(get(o,"trend_enabled"));return dsr_battery_profile_valid(p);
}
static const cJSON *public_device(const cJSON *config,uint32_t id)
{const cJSON *d;cJSON_ArrayForEach(d,get(config,"devices"))if((uint32_t)get(d,"id")->valuedouble==id)return d;return NULL;}
static const dsr_connection_t *binding(const dsr_connection_t *connections,size_t count,uint32_t id)
{for(size_t i=0;i<count;i++)if(connections[i].layout_id==id)return &connections[i];return NULL;}
static bool connection_decode(const cJSON *o,dsr_connection_t *c,const cJSON *config)
{
 static const char *const fields[]={"layout_id","host","name","device_id","generation","channel","endpoint","username","password"};uint32_t v;
 if(!KEYS(o,fields)||!number(o,"layout_id",1000000,&c->layout_id)||!c->layout_id||!string(o,"host",c->device.host,sizeof(c->device.host),false)||!string(o,"name",c->device.name,sizeof(c->device.name),true)||!string(o,"device_id",c->device.device_id,sizeof(c->device.device_id),true)||!string(o,"username",c->credentials.username,sizeof(c->credentials.username),true)||!string(o,"password",c->credentials.password,sizeof(c->credentials.password),true))return false;
 if(!number(o,"generation",2,&v))return false;
 c->device.generation=(dsr_generation_t)v;
 if(!number(o,"channel",DSR_MAX_CHANNEL,&v))return false;
 c->device.channel=(uint8_t)v;
 if(!number(o,"endpoint",DSR_ENDPOINT_BLU,&v))return false;
 c->device.endpoint_kind=(uint8_t)v;
 const cJSON *d=public_device(config,c->layout_id);
 return dsr_connection_valid(c,true)&&d&&!strcmp(get(d,"host")->valuestring,c->device.host)&&(uint32_t)get(d,"channel")->valuedouble==c->device.channel;
}
static bool photo_references(const cJSON *config,const cJSON *photos,uint16_t *used)
{
 for(const cJSON *v=config;v;v=v->next){
  if(v->string&&!strcmp(v->string,"photo")&&cJSON_IsString(v)&&v->valuestring[0]&&!dsr_backup_photo_builtin(v->valuestring)){
   bool found=false;unsigned i=0;const cJSON *photo;cJSON_ArrayForEach(photo,photos){if(!strcmp(get(photo,"id")->valuestring,v->valuestring)){*used|=(uint16_t)(1u<<i);found=true;}i++;}if(!found)return false;
  }
  if(v->child&&!photo_references(v->child,photos,used))return false;
 }
 return true;
}
static bool actual_key(const dsr_connection_t *c,const dsr_auto_key_t *key)
{return c&&c->device.endpoint_kind==DSR_ENDPOINT_BLU&&c->device.generation==DSR_GEN2_PLUS&&c->device.generation==key->generation&&c->device.channel==key->channel&&!strcmp(c->device.device_id,key->device_id)&&!strcmp(c->device.host,key->host);}
static bool key_decode(const cJSON *o,dsr_auto_key_t *k)
{
 static const char *const fields[]={"layout_id","device_id","host","generation","channel","kind"};uint32_t v;
 if(!KEYS(o,fields)||!number(o,"layout_id",1000000,&k->layout_id)||!k->layout_id||!string(o,"device_id",k->device_id,sizeof(k->device_id),true)||!string(o,"host",k->host,sizeof(k->host),false)||!number(o,"generation",2,&v)||v!=2)return false;
 k->generation=2;if(!number(o,"channel",15,&v))return false;k->channel=(uint8_t)v;
 if(!number(o,"kind",DSR_AUTO_TRV,&v)||v<DSR_AUTO_CONTACT)return false;
 k->kind=(uint8_t)v;return true;
}
static bool enrollment_decode(const cJSON *o,const cJSON *config,dsr_backup_sources_data_t *data)
{
 dsr_auto_record_t *r=&data->enrollment;char error[96];if(!dsr_auto_rules_parse(config,&r->rules,error))return false;r->generation=1;
 uint16_t enabled=0;unsigned rules=0;
 for(unsigned i=0;i<r->rules.count;i++)if(r->rules.items[i].enabled){enabled|=(uint16_t)(r->rules.items[i].sensors|r->rules.items[i].targets);rules++;}
 const cJSON *approved=get(o,"rules"),*keys=get(o,"keys");if((unsigned)cJSON_GetArraySize(approved)!=rules)return false;
 uint16_t seen_rules=0,seen_keys=0;const cJSON *v;
 static const char *const rule_fields[]={"id","signature"};
 cJSON_ArrayForEach(v,approved){uint32_t id,sig;if(!KEYS(v,rule_fields)||!number(v,"id",1000000,&id)||!number(v,"signature",UINT32_MAX,&sig))return false;
  unsigned i=0;for(;i<r->rules.count;i++)if(r->rules.items[i].id==id)break;
  if(i==r->rules.count||!r->rules.items[i].enabled||r->rules.items[i].signature!=sig||(seen_rules&(1u<<i)))return false;
  seen_rules|=(uint16_t)(1u<<i);
 }
 cJSON_ArrayForEach(v,keys){dsr_auto_key_t k={0};if(!key_decode(v,&k))return false;unsigned i=0;for(;i<r->rules.key_count;i++)if(r->rules.keys[i].layout_id==k.layout_id)break;
  if(i==r->rules.key_count||!(enabled&(1u<<i))||(seen_keys&(1u<<i))||r->rules.keys[i].kind!=k.kind||!actual_key(binding(data->connections,data->count,k.layout_id),&k))return false;
  r->rules.keys[i]=k;seen_keys|=(uint16_t)(1u<<i);
 }
 return seen_keys==enabled&&dsr_auto_record_valid(r);
}
static dsr_result_t decode(const cJSON *root,const uint8_t device_id[32],dsr_backup_sources_data_t *out)
{
 dsr_result_t result=dsr_backup_manifest_validate(root,device_id);if(result!=DSR_OK)return result;
 const cJSON *config=get(root,"config");if(get(config,"schema")->valueint!=DSR_CONFIG_SCHEMA)return DSR_UNSUPPORTED;
 if(!settings_decode(get(root,"settings"),&out->settings)||!battery_decode(get(root,"battery"),&out->battery))return DSR_INVALID;
 out->style=(uint8_t)get(root,"style")->valueint;
 unsigned expected_style=!strcmp(get(config,"theme")->valuestring,"light")?2:!strcmp(get(config,"theme")->valuestring,"cyber_blue")?3:0;
 if(out->style!=expected_style)return DSR_INVALID;
 const cJSON *v;cJSON_ArrayForEach(v,get(root,"connections")){
  dsr_connection_t *c=&out->connections[out->count];if(!connection_decode(v,c,config))return DSR_INVALID;
  for(size_t i=0;i<out->count;i++)if(out->connections[i].layout_id==c->layout_id||(out->connections[i].device.generation==c->device.generation&&out->connections[i].device.channel==c->device.channel&&!strcmp(out->connections[i].device.device_id,c->device.device_id)))return DSR_INVALID;
  out->count++;
 }
 uint16_t used=0;const cJSON *photos=get(root,"photos");if(!photo_references(config->child,photos,&used)||used!=((1u<<(unsigned)cJSON_GetArraySize(photos))-1u))return DSR_INVALID;
 if(!enrollment_decode(get(root,"enrollment"),config,out))return DSR_INVALID;
 return DSR_OK;
}
dsr_result_t dsr_backup_sources_decode(const cJSON *root,const uint8_t device_id[32],dsr_backup_sources_data_t *out)
{
 if(!out)return DSR_INVALID;
 memset(out,0,sizeof(*out));dsr_result_t result=decode(root,device_id,out);if(result!=DSR_OK)dsr_backup_wipe(out,sizeof(*out));return result;
}
static cJSON *settings_encode(const dsr_settings_t *s)
{
 cJSON *o=cJSON_CreateObject();if(!o)return NULL;
 bool ok=dsr_backup_json_add_string(o,"ssid",s->ssid,sizeof(s->ssid),true)&&dsr_backup_json_add_string(o,"password",s->password,sizeof(s->password),true)&&cJSON_AddNumberToObject(o,"brightness",s->brightness)&&cJSON_AddNumberToObject(o,"wifi_security",s->wifi_security)&&cJSON_AddNumberToObject(o,"idle_seconds",s->idle_seconds);
 if(!ok){cJSON_Delete(o);return NULL;}return o;
}
static cJSON *battery_encode(const dsr_battery_profile_t *p)
{
 cJSON *o=cJSON_CreateObject();if(!o)return NULL;
 bool ok=cJSON_AddNumberToObject(o,"kind",p->kind)&&cJSON_AddNumberToObject(o,"nominal_mv",p->nominal_mv)&&cJSON_AddNumberToObject(o,"capacity_mah",p->capacity_mah)&&cJSON_AddNumberToObject(o,"empty_mv",p->empty_mv)&&cJSON_AddNumberToObject(o,"mid_mv",p->mid_mv)&&cJSON_AddNumberToObject(o,"full_mv",p->full_mv)&&cJSON_AddNumberToObject(o,"gain_permille",p->gain_permille)&&cJSON_AddBoolToObject(o,"trend_enabled",p->trend_enabled);
 if(!ok){cJSON_Delete(o);return NULL;}return o;
}
static cJSON *connection_encode(const dsr_connection_t *c)
{
 cJSON *o=cJSON_CreateObject();if(!o)return NULL;
 bool ok=cJSON_AddNumberToObject(o,"layout_id",c->layout_id)&&dsr_backup_json_add_string(o,"host",c->device.host,sizeof(c->device.host),false)&&dsr_backup_json_add_string(o,"name",c->device.name,sizeof(c->device.name),true)&&dsr_backup_json_add_string(o,"device_id",c->device.device_id,sizeof(c->device.device_id),true)&&cJSON_AddNumberToObject(o,"generation",c->device.generation)&&cJSON_AddNumberToObject(o,"channel",c->device.channel)&&cJSON_AddNumberToObject(o,"endpoint",c->device.endpoint_kind)&&dsr_backup_json_add_string(o,"username",c->credentials.username,sizeof(c->credentials.username),true)&&dsr_backup_json_add_string(o,"password",c->credentials.password,sizeof(c->credentials.password),true);
 if(!ok){cJSON_Delete(o);return NULL;}return o;
}
static cJSON *key_encode(const dsr_auto_key_t *k)
{
 cJSON *o=cJSON_CreateObject();if(!o)return NULL;
 bool ok=cJSON_AddNumberToObject(o,"layout_id",k->layout_id)&&dsr_backup_json_add_string(o,"device_id",k->device_id,sizeof(k->device_id),true)&&dsr_backup_json_add_string(o,"host",k->host,sizeof(k->host),false)&&cJSON_AddNumberToObject(o,"generation",k->generation)&&cJSON_AddNumberToObject(o,"channel",k->channel)&&cJSON_AddNumberToObject(o,"kind",k->kind);
 if(!ok){cJSON_Delete(o);return NULL;}return o;
}
dsr_result_t dsr_backup_sources_encode(cJSON *root,const dsr_backup_sources_data_t *data)
{
 if(!root||!data||data->count>DSR_MAX_DEVICES||!dsr_auto_record_valid(&data->enrollment))return DSR_INVALID;
 if(!dsr_backup_json_replace(root,"settings",settings_encode(&data->settings))||!dsr_backup_json_replace(root,"battery",battery_encode(&data->battery))||!dsr_backup_json_replace(root,"style",cJSON_CreateNumber(data->style)))return DSR_IO;
 cJSON *connections=cJSON_CreateArray();if(!connections)return DSR_IO;
 for(size_t i=0;i<data->count;i++){cJSON *row=connection_encode(&data->connections[i]);if(!row||!cJSON_AddItemToArray(connections,row)){cJSON_Delete(row);cJSON_Delete(connections);return DSR_IO;}}
 if(!dsr_backup_json_replace(root,"connections",connections))return DSR_IO;
 cJSON *enroll=cJSON_CreateObject();if(!enroll)return DSR_IO;
 cJSON *keys=cJSON_AddArrayToObject(enroll,"keys"),*rules=cJSON_AddArrayToObject(enroll,"rules");
 if(!keys||!rules||!cJSON_AddNumberToObject(enroll,"version",1)){cJSON_Delete(enroll);return DSR_IO;}
 uint16_t mask=0;
 for(unsigned i=0;i<data->enrollment.rules.count;i++)if(data->enrollment.rules.items[i].enabled){const dsr_auto_rule_t *r=&data->enrollment.rules.items[i];mask|=(uint16_t)(r->sensors|r->targets);
  cJSON *row=cJSON_CreateObject();if(!row||!cJSON_AddNumberToObject(row,"id",r->id)||!cJSON_AddNumberToObject(row,"signature",r->signature)||!cJSON_AddItemToArray(rules,row)){cJSON_Delete(row);cJSON_Delete(enroll);return DSR_IO;}
 }
 for(unsigned i=0;i<data->enrollment.rules.key_count;i++)if(mask&(1u<<i)){cJSON *row=key_encode(&data->enrollment.rules.keys[i]);if(!row||!cJSON_AddItemToArray(keys,row)){cJSON_Delete(row);cJSON_Delete(enroll);return DSR_IO;}}
 return dsr_backup_json_replace(root,"enrollment",enroll)?DSR_OK:DSR_IO;
}
dsr_result_t dsr_backup_enrollment_prepare(const cJSON *config,const dsr_auto_record_t *trusted,const dsr_connection_t *connections,size_t count,dsr_auto_record_t *out)
{
 if(!out||!trusted||!dsr_auto_record_valid(trusted)||count>DSR_MAX_DEVICES)return DSR_INVALID;
 memset(out,0,sizeof(*out));char error[96];if(!dsr_auto_rules_parse(config,&out->rules,error))return DSR_INVALID;out->generation=1;
 for(unsigned i=0;i<trusted->rules.count;i++)if(trusted->cycles[i].active)return DSR_BUSY;
 for(unsigned i=0;i<trusted->rules.key_count;i++){
  unsigned phase=trusted->targets[i].phase;
  if(phase==DSR_AUTO_PREPARED||phase==DSR_AUTO_VERIFY||phase==DSR_AUTO_RESTORE_PREPARED||phase==DSR_AUTO_RESTORE_VERIFY||phase==DSR_AUTO_UNCERTAIN)return DSR_BUSY;
 }
 uint16_t mask=0;
 for(unsigned i=0;i<out->rules.count;i++)if(out->rules.items[i].enabled){const dsr_auto_rule_t *r=&out->rules.items[i];unsigned old=0;
  for(;old<trusted->rules.count;old++)if(trusted->rules.items[old].id==r->id)break;
  if(old==trusted->rules.count||!trusted->rules.items[old].enabled||!dsr_auto_rule_same(&out->rules,r,&trusted->rules,&trusted->rules.items[old]))return DSR_CONFLICT;
  mask|=(uint16_t)(r->sensors|r->targets);
 }
 for(unsigned i=0;i<out->rules.key_count;i++)if(mask&(1u<<i)){dsr_auto_key_t *key=&out->rules.keys[i];unsigned old=0;
  for(;old<trusted->rules.key_count;old++)if(trusted->rules.keys[old].layout_id==key->layout_id)break;
  if(old==trusted->rules.key_count||trusted->rules.keys[old].kind!=key->kind||!actual_key(binding(connections,count,key->layout_id),&trusted->rules.keys[old]))return DSR_IDENTITY_MISMATCH;
  *key=trusted->rules.keys[old];
 }
 return dsr_auto_record_valid(out)?DSR_OK:DSR_INVALID;
}
