#include "dsr_discovery.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const cJSON *get(const cJSON *o,const char *key){return cJSON_GetObjectItemCaseSensitive(o,key);}
static bool text(const cJSON *v,size_t maximum)
{if(!cJSON_IsString(v)||!v->valuestring[0]||strlen(v->valuestring)>=maximum)return false;for(const unsigned char *p=(const unsigned char *)v->valuestring;*p;p++)if(*p<32)return false;return true;}
static bool unique_keys(const cJSON *o)
{for(const cJSON *a=o->child;a;a=a->next)for(const cJSON *b=a->next;b;b=b->next)if(!strcmp(a->string,b->string))return false;return true;}
static bool integral(const cJSON *v,int lo,int hi)
{return cJSON_IsNumber(v)&&isfinite(v->valuedouble)&&v->valuedouble>=lo&&v->valuedouble<=hi&&floor(v->valuedouble)==v->valuedouble;}
dsr_result_t dsr_shelly_parse_identity(const cJSON *r,dsr_lan_device_t *out)
{
    if(!out||!cJSON_IsObject(r)||!unique_keys(r))return DSR_BAD_RESPONSE;
    const cJSON *gen=get(r,"gen"),*mac=get(r,"mac"),*id=get(r,"id"),*model=get(r,"model"),*name=get(r,"name");
    if(!text(mac,13)||strlen(mac->valuestring)!=12)return DSR_BAD_RESPONSE;
    dsr_lan_device_t next={0};bool nonzero=false,not_broadcast=false;
    for(unsigned i=0;i<12;i++){unsigned char c=(unsigned char)mac->valuestring[i];if(!isxdigit(c))return DSR_BAD_RESPONSE;next.mac[i]=(char)toupper(c);nonzero|=c!='0';not_broadcast|=toupper(c)!='F';}
    if(!nonzero||!not_broadcast)return DSR_BAD_RESPONSE;
    if(!gen||integral(gen,1,1)){
        const cJSON *type=get(r,"type"),*auth=get(r,"auth");
        if(!text(type,sizeof(next.model))||strncmp(type->valuestring,"SH",2)||!cJSON_IsBool(auth))return DSR_UNSUPPORTED;
        next.generation=1;next.auth_required=cJSON_IsTrue(auth);strcpy(next.id,next.mac);strcpy(next.model,type->valuestring);
    }else{
        const cJSON *auth=get(r,"auth_en");
        if(!integral(gen,2,4)||!text(id,sizeof(next.id))||strncmp(id->valuestring,"shelly",6)||!text(model,sizeof(next.model))||!cJSON_IsBool(auth))return DSR_UNSUPPORTED;
        for(const unsigned char *p=(const unsigned char *)id->valuestring;*p;p++)if(!(isalnum(*p)||*p=='-'||*p=='_'))return DSR_BAD_RESPONSE;
        next.generation=(uint8_t)gen->valueint;next.auth_required=cJSON_IsTrue(auth);strcpy(next.id,id->valuestring);strcpy(next.model,model->valuestring);
    }
    const cJSON *profile=get(r,"profile");
    if(profile){if(!text(profile,24))return DSR_BAD_RESPONSE;next.profile=!strcmp(profile->valuestring,"switch")?1:!strcmp(profile->valuestring,"cover")?2:!strcmp(profile->valuestring,"light")?3:!strcmp(profile->valuestring,"rgb")?4:255;}
    const char *friendly=dsr_device_model_name(next.id);
    if(!cJSON_IsString(name)||!name->valuestring[0]||!dsr_text_copy(next.name,sizeof(next.name),name->valuestring))dsr_text_copy(next.name,sizeof(next.name),friendly?friendly:next.id);
    *out=next;return DSR_OK;
}
dsr_result_t dsr_lan_parse_identity(const cJSON *r,const char *host,dsr_lan_device_t *out)
{if(!out||!dsr_ipv4_valid(host))return DSR_BAD_RESPONSE;dsr_result_t result=dsr_shelly_parse_identity(r,out);if(result==DSR_OK)strcpy(out->host,host);return result;}
static bool ht_unique(const cJSON *r)
{
 if(cJSON_IsObject(r))for(const cJSON *a=r->child;a;a=a->next){if(!a->string)return false;for(const cJSON *b=a->next;b;b=b->next)if(!b->string||!strcmp(a->string,b->string))return false;}
 for(const cJSON *v=r?r->child:NULL;v;v=v->next)if(!ht_unique(v))return false;
 return true;
}
static bool ht_number(const cJSON *v,double lo,double hi,bool *has,double *value)
{
 *has=false;*value=0;if(!v||cJSON_IsNull(v))return true;
 if(!cJSON_IsNumber(v)||!isfinite(v->valuedouble)||v->valuedouble<lo||v->valuedouble>hi)return false;
 *has=true;*value=v->valuedouble;return true;
}
static bool ht_errors(const cJSON *o,bool *has)
{
 const cJSON *errors=get(o,"errors");*has=false;if(!errors)return true;
 if(!cJSON_IsArray(errors)||cJSON_GetArraySize(errors)>16)return false;
 for(const cJSON *v=errors->child;v;v=v->next){if(!cJSON_IsString(v)||!v->valuestring[0]||strlen(v->valuestring)>64)return false;*has=true;}
 return true;
}
dsr_result_t dsr_ht_parse(const cJSON *r,dsr_telemetry_t *out)
{
 if(!out||!cJSON_IsObject(r)||!ht_unique(r))return DSR_BAD_RESPONSE;
 const cJSON *t=get(r,"temperature:0"),*h=get(r,"humidity:0"),*p=get(r,"devicepower:0");
 if(!cJSON_IsObject(t)||!cJSON_IsObject(h)||!cJSON_IsObject(p))return DSR_UNSUPPORTED;
 if(!integral(get(t,"id"),0,0)||!integral(get(h,"id"),0,0)||!integral(get(p,"id"),0,0))return DSR_BAD_RESPONSE;
 dsr_telemetry_t sample={0};bool te=false,he=false,pe=false,reported=false;
 /* Defensive adapter bounds. The API does not publish a numeric tC range;
  * the model's rated ambient 0..40 C is not a wire-validation range. */
 if(!ht_number(get(t,"tC"),-273.15,1000,&sample.has_temperature,&sample.temperature_c)||!ht_errors(t,&reported))te=true;
 te|=reported;reported=false;
 if(!ht_number(get(h,"rh"),0,100,&sample.has_humidity,&sample.humidity_pct)||!ht_errors(h,&reported))he=true;
 he|=reported;reported=false;
 if(!ht_errors(p,&reported))pe=true;
 pe|=reported;
 const cJSON *fahrenheit=get(t,"tF");bool has_f;double f;
 if(!ht_number(fahrenheit,-459.67,1832,&has_f,&f))te=true;
 const cJSON *battery=get(p,"battery"),*external=get(p,"external");
 /* A malformed optional power section invalidates power only. It must not
  * suppress independently valid Celsius or humidity from this same bundle. */
 if(battery&&!cJSON_IsNull(battery)){
  bool has_v;double v;
  if(!cJSON_IsObject(battery)||!ht_number(get(battery,"percent"),0,100,&sample.has_battery,&sample.battery_pct)||!ht_number(get(battery,"V"),0,20,&has_v,&v))pe=true;
 }
 if(external&&!cJSON_IsNull(external)){
  if(!cJSON_IsObject(external))pe=true;
  else {const cJSON *present=get(external,"present");
   if(present&&!cJSON_IsNull(present)){if(!cJSON_IsBool(present))pe=true;else {sample.ht.flags|=DSR_HT_EXTERNAL_KNOWN;if(cJSON_IsTrue(present))sample.ht.flags|=DSR_HT_EXTERNAL_PRESENT;}}
  }
 }
 sample.ht.flags|=DSR_HT_VALID;
 if(te){sample.ht.errors|=DSR_HT_TEMPERATURE_ERROR;sample.has_temperature=false;sample.temperature_c=0;}
 if(he){sample.ht.errors|=DSR_HT_HUMIDITY_ERROR;sample.has_humidity=false;sample.humidity_pct=0;}
 if(pe){sample.ht.errors|=DSR_HT_POWER_ERROR;sample.has_battery=false;sample.battery_pct=0;sample.ht.flags&=(uint8_t)~(DSR_HT_EXTERNAL_KNOWN|DSR_HT_EXTERNAL_PRESENT);}
 *out=sample;return DSR_OK;
}
/* ---- Shelly BLU through a Gen2+ gateway (BTHome components) ----
 * BTHomeDevice cid 200..215 maps to endpoint channel 0..15. Values are what the
 * gateway decoded; the remote never sees the BLE frames or encryption keys. */
dsr_result_t dsr_blu_parse_address(const cJSON *config,unsigned cid,char mac[13])
{
 mac[0]=0;
 if(!cJSON_IsObject(config)||!ht_unique(config)||!integral(get(config,"id"),(int)cid,(int)cid))return DSR_BAD_RESPONSE;
 const cJSON *addr=get(config,"addr");if(!text(addr,18)||strlen(addr->valuestring)!=17)return DSR_BAD_RESPONSE;
 const char *a=addr->valuestring;char out[13];unsigned n=0;bool nonzero=false;
 for(unsigned i=0;i<17;i++){
  if(i%3==2){if(a[i]!=':')return DSR_BAD_RESPONSE;continue;}
  if(!isxdigit((unsigned char)a[i]))return DSR_BAD_RESPONSE;
  out[n++]=(char)tolower((unsigned char)a[i]);nonzero|=a[i]!='0';
 }
 out[n]=0;if(!nonzero)return DSR_BAD_RESPONSE;
 memcpy(mac,out,sizeof(out));return DSR_OK;
}
dsr_result_t dsr_blu_parse_known(const cJSON *r,unsigned cid,dsr_blu_object_t *objects,unsigned capacity,unsigned *count)
{
 *count=0;
 if(!cJSON_IsObject(r)||!ht_unique(r)||!integral(get(r,"id"),(int)cid,(int)cid))return DSR_BAD_RESPONSE;
 const cJSON *list=get(r,"objects");if(!cJSON_IsArray(list)||cJSON_GetArraySize(list)>32)return DSR_BAD_RESPONSE;
 const cJSON *v;cJSON_ArrayForEach(v,list){
  if(!cJSON_IsObject(v)||!integral(get(v,"obj_id"),0,255)||!integral(get(v,"idx"),0,255))return DSR_BAD_RESPONSE;
  const cJSON *component=get(v,"component");
  if(!component||cJSON_IsNull(component))continue; /* known to the gateway, no sensor component */
  if(!text(component,24)||strncmp(component->valuestring,"bthomesensor:",13))return DSR_BAD_RESPONSE;
  char *end;long id=strtol(component->valuestring+13,&end,10);
  if(!component->valuestring[13]||*end||id<0||id>65535)return DSR_BAD_RESPONSE;
  uint8_t obj=(uint8_t)get(v,"obj_id")->valueint;bool seen=false;for(unsigned i=0;i<*count;i++)seen|=objects[i].obj_id==obj;
  if(seen)continue; /* e.g. BLU TRV reports two temperatures: keep the first (idx 0) */
  if(*count<capacity)objects[(*count)++]=(dsr_blu_object_t){.sensor=(uint16_t)id,.obj_id=obj};
 }
 return DSR_OK;
}
dsr_result_t dsr_blu_parse_device(const cJSON *r,unsigned cid,dsr_telemetry_t *out)
{
 memset(out,0,sizeof(*out));
 if(!cJSON_IsObject(r)||!ht_unique(r)||!integral(get(r,"id"),(int)cid,(int)cid))return DSR_BAD_RESPONSE;
 bool has;double value;
 if(!ht_number(get(r,"battery"),0,100,&out->has_blu_battery,&out->blu_battery_pct))return DSR_BAD_RESPONSE;
 if(!ht_number(get(r,"rssi"),-127,20,&has,&value))return DSR_BAD_RESPONSE;
 if(has){out->blu.rssi=(int8_t)value;out->blu.flags|=DSR_BLU_HAS_RSSI;}
 /* Documented as last_update_ts; devices answer last_updated_ts. */
 const cJSON *ts=get(r,"last_updated_ts");if(!ts)ts=get(r,"last_update_ts");
 if(!ht_number(ts,0,4294967295.0,&has,&value))return DSR_BAD_RESPONSE;
 if(has)out->blu_updated_s=(uint32_t)value;
 const cJSON *errors=get(r,"errors");
 if(errors){
  if(!cJSON_IsArray(errors)||cJSON_GetArraySize(errors)>8)return DSR_BAD_RESPONSE;
  const char *known[]={"key_missing_or_bad","decrypt_failed","parse_failed","unencrypted_data"};
  const cJSON *e;cJSON_ArrayForEach(e,errors){if(!text(e,64))return DSR_BAD_RESPONSE;for(unsigned i=0;i<4;i++)if(!strcmp(e->valuestring,known[i]))out->blu.errors|=(uint8_t)(1u<<i);}
 }
 out->blu.flags|=DSR_BLU_VALID;return DSR_OK;
}
dsr_result_t dsr_blu_apply_sensor(const cJSON *r,unsigned sensor,uint8_t obj_id,dsr_telemetry_t *out)
{
 if(!cJSON_IsObject(r)||!ht_unique(r)||!integral(get(r,"id"),(int)sensor,(int)sensor))return DSR_BAD_RESPONSE;
 const cJSON *v=get(r,"value");if(!v||cJSON_IsNull(v))return DSR_OK; /* nothing received yet */
 bool has;double value;
 switch(obj_id){
 case 0x21: /* motion */
  if(!cJSON_IsBool(v)){return DSR_BAD_RESPONSE;}out->blu.flags|=DSR_BLU_HAS_MOTION|(cJSON_IsTrue(v)?DSR_BLU_MOTION:0);break;
 case 0x1A: case 0x2D: /* door, window: true = open */
  if(!cJSON_IsBool(v)){return DSR_BAD_RESPONSE;}out->blu.flags|=DSR_BLU_HAS_OPEN|(cJSON_IsTrue(v)?DSR_BLU_OPEN:0);break;
 case 0x05: /* illuminance, lux */
  if(!ht_number(v,0,200000,&has,&value)){return DSR_BAD_RESPONSE;}out->has_illuminance=has;out->illuminance_lux=value;break;
 case 0x01: /* battery, % (BTHomeDevice battery may be absent) */
  if(!ht_number(v,0,100,&has,&value)){return DSR_BAD_RESPONSE;}if(has){out->has_blu_battery=true;out->blu_battery_pct=value;}break;
 case 0x02: case 0x45: /* temperature, C */
  if(!ht_number(v,-60,120,&has,&value)){return DSR_BAD_RESPONSE;}out->has_blu_temperature=has;out->blu_temperature_c=(float)value;break;
 case 0x03: case 0x2E: /* humidity, % */
  if(!ht_number(v,0,100,&has,&value)){return DSR_BAD_RESPONSE;}if(has){out->blu.flags|=DSR_BLU_HAS_HUMIDITY;out->blu.humidity=(uint8_t)(value+0.5);}break;
 case 0x3A: /* button event code */
  if(!ht_number(v,0,255,&has,&value)){return DSR_BAD_RESPONSE;}if(has){out->blu.flags|=DSR_BLU_HAS_BUTTON;out->blu.button=(uint8_t)value;}break;
 default:break; /* rotation, packet id, ...: not shown */
 }
 return DSR_OK;
}
bool dsr_blu_trv_linked(const cJSON *r,unsigned id,unsigned cid)
{
 if(!cJSON_IsObject(r)||!ht_unique(r)||!integral(get(r,"id"),(int)id,(int)id))return false;
 const cJSON *trv=get(r,"trv");char expected[24];snprintf(expected,sizeof(expected),"bthomedevice:%u",cid);
 return text(trv,23)&&!strcmp(trv->valuestring,expected);
}
dsr_result_t dsr_blu_parse_trv(const cJSON *r,unsigned id,dsr_telemetry_t *out)
{
 if(!cJSON_IsObject(r)||!ht_unique(r)||!integral(get(r,"id"),(int)id,(int)id))return DSR_BAD_RESPONSE;
 bool has_target,has_current,has_pos;double target,current,pos;
 if(!ht_number(get(r,"target_C"),-50,100,&has_target,&target)||!ht_number(get(r,"current_C"),-50,100,&has_current,&current)||!ht_number(get(r,"pos"),0,100,&has_pos,&pos))return DSR_BAD_RESPONSE;
 const cJSON *errors=get(r,"errors");
 if(errors&&(!cJSON_IsArray(errors)||cJSON_GetArraySize(errors)>8))return DSR_BAD_RESPONSE;
 const cJSON *e;if(errors)cJSON_ArrayForEach(e,errors)if(!text(e,64))return DSR_BAD_RESPONSE;
 out->blu.errors|=DSR_BLU_TRV;if(errors&&cJSON_GetArraySize(errors))out->blu.errors|=DSR_BLU_TRV_FAULT;
 /* TRV has no lux and no humidity: their storage carries the TRV values. */
 out->has_illuminance=false;out->blu.flags&=(uint8_t)~DSR_BLU_HAS_HUMIDITY;
 if(has_target&&has_current){out->trv_target_c=(float)target;out->trv_current_c=(float)current;out->blu.trv_pos=has_pos?(uint8_t)(pos+0.5):0;out->blu.errors|=DSR_BLU_TRV_TARGET;}
 return DSR_OK;
}
bool dsr_blu_sensor_shown(uint8_t obj_id)
{return obj_id==0x21||obj_id==0x1A||obj_id==0x2D||obj_id==0x05||obj_id==0x01||obj_id==0x02||obj_id==0x45||obj_id==0x03||obj_id==0x2E||obj_id==0x3A;}
const char *dsr_blu_model_name(uint8_t model)
{
 /* Only ids confirmed on a real gateway (2026-10-17); others stay numeric. */
 switch(model){case 2:return "BLU Door/Window";case 8:return "BLU TRV";default:return NULL;}
}
dsr_result_t dsr_lan_parse_components(const cJSON *r,unsigned offset,uint16_t *blu,dsr_blu_labels_t *labels,unsigned *next,unsigned *total)
{
 if(!blu||!next||!total||!cJSON_IsObject(r)||!unique_keys(r))return DSR_BAD_RESPONSE;
 const cJSON *list=get(r,"components"),*at=get(r,"offset"),*all=get(r,"total");
 if(!cJSON_IsArray(list)||cJSON_GetArraySize(list)>100||!integral(at,(int)offset,(int)offset)||!integral(all,0,1000))return DSR_BAD_RESPONSE;
 unsigned n=0;uint16_t found=0,labelled=0;const cJSON *v;
 /* Items are written in place (bounded frame); valid bits only after the page checks out. */
 cJSON_ArrayForEach(v,list){
  n++;const cJSON *key=get(v,"key");if(!cJSON_IsObject(v)||!text(key,40))return DSR_BAD_RESPONSE;
  if(!strncmp(key->valuestring,"bthomedevice:",13)){
   char *end;long cid=strtol(key->valuestring+13,&end,10);if(!key->valuestring[13]||*end||cid<0)return DSR_BAD_RESPONSE;
   if(cid>=(long)DSR_BLU_FIRST_CID&&cid<(long)DSR_BLU_FIRST_CID+16){
    unsigned i=(unsigned)(cid-(long)DSR_BLU_FIRST_CID);found|=(uint16_t)(1u<<i);
    /* Labels are best effort: odd metadata never hides the channel. */
    const cJSON *config=get(v,"config"),*attrs=get(v,"attrs");char mac[13];dsr_blu_label_t scratch,*l=labels?&labels->items[i]:&scratch;memset(l,0,sizeof(*l));
    if(cJSON_IsObject(config)&&dsr_blu_parse_address(config,(unsigned)cid,mac)==DSR_OK){
     for(unsigned b=0;b<2;b++){unsigned hi=(unsigned)(isdigit((unsigned char)mac[8+b*2])?mac[8+b*2]-'0':mac[8+b*2]-'a'+10),lo=(unsigned)(isdigit((unsigned char)mac[9+b*2])?mac[9+b*2]-'0':mac[9+b*2]-'a'+10);l->mac_tail[b]=(uint8_t)(hi<<4|lo);}
     labelled|=(uint16_t)(1u<<i);
    }
    const cJSON *name=cJSON_IsObject(config)?get(config,"name"):NULL;
    if(cJSON_IsString(name)&&name->valuestring){
     /* Printable UTF-8 only, cut on a character boundary. */
     size_t len=0;bool ok=true;for(const unsigned char *c=(const unsigned char *)name->valuestring;*c;c++)if(*c<0x20||*c==0x7f)ok=false;
     if(ok){const char *t=name->valuestring;while(t[len]&&len<sizeof(l->name)-1)len++;while(len&&t[len]&&((unsigned char)t[len]&0xC0)==0x80)len--;memcpy(l->name,t,len);l->name[len]=0;}
    }
    const cJSON *model=cJSON_IsObject(attrs)?get(attrs,"model_id"):NULL;if(integral(model,1,255))l->model=(uint8_t)model->valueint;
   }
  }
 }
 if(offset+n>(unsigned)all->valueint||(!n&&offset<(unsigned)all->valueint))return DSR_BAD_RESPONSE; /* overrun or no progress */
 if(labels)labels->valid|=labelled;
 *blu|=found;*next=offset+n;*total=(unsigned)all->valueint;return DSR_OK;
}
dsr_result_t dsr_lan_parse_capabilities(const cJSON *r,unsigned generation,dsr_lan_capabilities_t *out)
{
    if(!out||!cJSON_IsObject(r)||!unique_keys(r))return DSR_BAD_RESPONSE;
    dsr_lan_capabilities_t caps={0};
    if(generation==1){
        const char *keys[]={"relays","rollers","lights"};uint16_t *masks[]={&caps.switches,&caps.covers,&caps.lights};
        for(unsigned k=0;k<3;k++){const cJSON *a=get(r,keys[k]);if(!a)continue;if(!cJSON_IsArray(a)||cJSON_GetArraySize(a)>16)return DSR_BAD_RESPONSE;
            unsigned i=0;for(const cJSON *v=a->child;v;v=v->next,i++){
                if(!cJSON_IsObject(v)||!unique_keys(v))return DSR_BAD_RESPONSE;
                if(k==1){if(!text(get(v,"state"),32))return DSR_BAD_RESPONSE;}else if(!cJSON_IsBool(get(v,"ison")))return DSR_BAD_RESPONSE;
                *masks[k]|=(uint16_t)(1u<<i);
            }
        }
        caps.sensors=get(r,"tmp")||get(r,"hum")||get(r,"bat")||get(r,"sensor");
    }else if(generation>=2&&generation<=4){
        for(const cJSON *v=r->child;v;v=v->next){const char *colon=strchr(v->string,':');if(!colon)continue;
            bool sw=!strncmp(v->string,"switch:",7),cover=!strncmp(v->string,"cover:",6),light=!strncmp(v->string,"light:",6),rgb=!strncmp(v->string,"rgb:",4);
            if(sw||cover||light||rgb){char *end;long index=strtol(colon+1,&end,10);if(!colon[1]||*end||index<0||index>15||!cJSON_IsObject(v)||!unique_keys(v)||!integral(get(v,"id"),(int)index,(int)index))return DSR_BAD_RESPONSE;
                char canonical[16];snprintf(canonical,sizeof(canonical),"%.*s:%ld",(int)(colon-v->string),v->string,index);if(strcmp(canonical,v->string))return DSR_BAD_RESPONSE;
                if(rgb){
                    const cJSON *color=get(v,"rgb");if(strcmp(v->string,"rgb:0")||index!=0||!cJSON_IsBool(get(v,"output"))||!integral(get(v,"brightness"),0,100)||!cJSON_IsArray(color)||cJSON_GetArraySize(color)!=3)return DSR_BAD_RESPONSE;
                    for(const cJSON *c=color->child;c;c=c->next)if(!integral(c,0,255))return DSR_BAD_RESPONSE;
                    caps.rgbs|=1;
                }else if(cover){if(!text(get(v,"state"),32))return DSR_BAD_RESPONSE;caps.covers|=(uint16_t)(1u<<index);}
                else{if(!cJSON_IsBool(get(v,"output"))||(light&&!integral(get(v,"brightness"),0,100)))return DSR_BAD_RESPONSE;*(sw?&caps.switches:&caps.lights)|=(uint16_t)(1u<<index);}
            }else if(!strncmp(v->string,"bthomedevice:",13)){char *end;long cid=strtol(v->string+13,&end,10);
                if(!v->string[13]||*end||!cJSON_IsObject(v)||!integral(get(v,"id"),(int)cid,(int)cid))return DSR_BAD_RESPONSE;
                if(cid>=(long)DSR_BLU_FIRST_CID&&cid<(long)DSR_BLU_FIRST_CID+16)caps.blu|=(uint16_t)(1u<<(cid-(long)DSR_BLU_FIRST_CID));
            }else if(!strncmp(v->string,"temperature:",12)||!strncmp(v->string,"humidity:",9)||!strncmp(v->string,"devicepower:",12))caps.sensors=cJSON_IsObject(v)||caps.sensors;
        }
    }else return DSR_UNSUPPORTED;
    if(generation==3&&get(r,"temperature:0")&&get(r,"humidity:0")&&get(r,"devicepower:0")){
        dsr_telemetry_t sample;dsr_result_t result=dsr_ht_parse(r,&sample);if(result!=DSR_OK)return result;caps.ht=true;
    }
    *out=caps;return DSR_OK;
}
dsr_result_t dsr_lan_upsert(dsr_lan_snapshot_t *scan,const dsr_lan_device_t *d)
{
    if(!scan||scan->count>DSR_LAN_MAX_DEVICES||!d||!d->id[0]||!d->mac[0])return DSR_INVALID;
    for(unsigned i=0;i<scan->count;i++)if(!strcmp(scan->devices[i].id,d->id)||!strcmp(scan->devices[i].mac,d->mac)){
        if(strcmp(scan->devices[i].id,d->id)||strcmp(scan->devices[i].mac,d->mac))return DSR_IDENTITY_MISMATCH;
        scan->devices[i]=*d;return DSR_OK; /* DHCP: stable identity, new location. */
    }
    if(scan->count>=DSR_LAN_MAX_DEVICES)return DSR_FULL;
    scan->devices[scan->count++]=*d;return DSR_OK;
}
static uint32_t ipv4(const char *s)
{unsigned a,b,c,d;char extra,canonical[16];if(!s||sscanf(s,"%u.%u.%u.%u%c",&a,&b,&c,&d,&extra)!=4||a>255||b>255||c>255||d>255)return 0;snprintf(canonical,sizeof(canonical),"%u.%u.%u.%u",a,b,c,d);if(strcmp(canonical,s))return 0;return a<<24|b<<16|c<<8|d;}
bool dsr_lan_plan_add(dsr_lan_plan_t *plan,const char *host,const char *local,const char *mask)
{
    if(!plan||plan->count>=DSR_LAN_MAX_HOSTS)return false;
    uint32_t ip=ipv4(host),own=ipv4(local),netmask=ipv4(mask),bits=~netmask;
    if(!ip||!own||!netmask||(bits&(bits+1))||ip==own||(ip&netmask)!=(own&netmask)||(ip&bits)==0||(ip&bits)==bits)return false;
    unsigned first=ip>>24;if(first==0||first==127||first>=224)return false;
    for(unsigned i=0;i<plan->count;i++)if(!strcmp(plan->hosts[i],host))return false;
    strcpy(plan->hosts[plan->count++],host);return true;
}
