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
    snprintf(next.name,sizeof(next.name),"%s",text(name,sizeof(next.name))?name->valuestring:next.model);*out=next;return DSR_OK;
}
dsr_result_t dsr_lan_parse_identity(const cJSON *r,const char *host,dsr_lan_device_t *out)
{if(!out||!dsr_ipv4_valid(host))return DSR_BAD_RESPONSE;dsr_result_t result=dsr_shelly_parse_identity(r,out);if(result==DSR_OK)strcpy(out->host,host);return result;}
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
                if(rgb){
                    const cJSON *color=get(v,"rgb");if(strcmp(v->string,"rgb:0")||index!=0||!cJSON_IsBool(get(v,"output"))||!integral(get(v,"brightness"),0,100)||!cJSON_IsArray(color)||cJSON_GetArraySize(color)!=3)return DSR_BAD_RESPONSE;
                    for(const cJSON *c=color->child;c;c=c->next)if(!integral(c,0,255))return DSR_BAD_RESPONSE;
                    caps.rgbs|=1;
                }else if(cover){if(!text(get(v,"state"),32))return DSR_BAD_RESPONSE;caps.covers|=(uint16_t)(1u<<index);}
                else{if(!cJSON_IsBool(get(v,"output"))||(light&&!integral(get(v,"brightness"),0,100)))return DSR_BAD_RESPONSE;*(sw?&caps.switches:&caps.lights)|=(uint16_t)(1u<<index);}
            }else if(!strncmp(v->string,"temperature:",12)||!strncmp(v->string,"humidity:",9)||!strncmp(v->string,"devicepower:",12))caps.sensors=cJSON_IsObject(v)||caps.sensors;
        }
    }else return DSR_UNSUPPORTED;
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
