#include "dsr_history.h"
#include "dsr_config.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool integer(const cJSON *v,unsigned min,unsigned max)
{return cJSON_IsNumber(v)&&isfinite(v->valuedouble)&&floor(v->valuedouble)==v->valuedouble&&v->valuedouble>=min&&v->valuedouble<=max;}
bool dsr_history_parse(const char *json,dsr_history_key_t *key,unsigned *hours)
{
 if(key)memset(key,0,sizeof(*key));
 if(hours)*hours=0;
 if(!json||!key||!hours||strlen(json)>256||!dsr_config_json_depth(json)||strstr(json,"\\u0000"))return false;
 const char *end=NULL;cJSON *r=cJSON_ParseWithOpts(json,&end,true);if(!cJSON_IsObject(r)){cJSON_Delete(r);return false;}
 const cJSON *id=NULL,*generation=NULL,*channel=NULL,*range=NULL,*kind=NULL;
 bool ok=true;const cJSON *v;cJSON_ArrayForEach(v,r){
  const cJSON **destination=!strcmp(v->string,"device_id")?&id:!strcmp(v->string,"generation")?&generation:!strcmp(v->string,"channel")?&channel:!strcmp(v->string,"hours")?&range:!strcmp(v->string,"kind")?&kind:NULL;
  if(!destination||*destination){ok=false;break;}*destination=v;
 }
 ok=ok&&cJSON_IsString(id)&&id->valuestring[0]&&strlen(id->valuestring)<64&&integer(generation,1,2)&&integer(channel,0,15)&&integer(range,1,24)&&(range->valueint==1||range->valueint==24)&&(!kind||integer(kind,0,0));
 if(ok)for(const unsigned char *p=(const unsigned char *)id->valuestring;*p;p++)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_')){ok=false;break;}
 if(ok){strcpy(key->device_id,id->valuestring);key->generation=(uint8_t)generation->valueint;key->channel=(uint8_t)channel->valueint;key->kind=0;*hours=(unsigned)range->valueint;}
 cJSON_Delete(r);return ok;
}
#define JSON_CAP 49152u
_Static_assert(DSR_HISTORY_POINTS*160u+2048u<=JSON_CAP,"bounded history JSON capacity");
typedef struct {char *text;size_t used;bool ok;} writer_t;
static void append(writer_t *w,const char *format,...)
{
 if(!w->ok)return;
 va_list args;va_start(args,format);int n=vsnprintf(w->text+w->used,JSON_CAP-w->used,format,args);va_end(args);
 if(n<0||(size_t)n>=JSON_CAP-w->used){w->ok=false;return;}w->used+=(size_t)n;
}
static void string(writer_t *w,const char *s)
{
 append(w,"\"");for(const unsigned char *p=(const unsigned char *)s;*p;p++){
  if(*p=='"'||*p=='\\')append(w,"\\%c",*p);else if(*p<32)append(w,"\\u%04x",*p);else append(w,"%c",*p);
 }append(w,"\"");
}
static void value(writer_t *w,bool present,double number)
{if(present)append(w,"%.17g",number);else append(w,"null");}
char *dsr_history_json(const dsr_history_key_t *key,unsigned hours)
{
 dsr_history_series_t *series=dsr_history_query(key,hours);if(!series)return NULL;
 char *out=heap_caps_calloc(1,JSON_CAP,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!out){dsr_history_series_free(series);return NULL;}
 const dsr_history_status_t *s=&series->status;writer_t w={.text=out,.ok=true};
 append(&w,"{\"schema\":1,\"state\":%u,\"clock_valid\":%s,\"durable\":%s,\"clock_pending\":%s,\"clock_busy\":%s,\"clock_result\":%u,\"samples_without_time\":%lu,\"timezone\":\"UTC\",\"step\":300,\"retention\":86400,\"from\":%lu,\"to\":%lu,\"device_id\":",
  (unsigned)s->state,s->clock_valid?"true":"false",s->durable?"true":"false",s->clock_pending?"true":"false",s->clock_busy?"true":"false",(unsigned)s->clock_result,(unsigned long)s->samples_without_time,(unsigned long)series->from_utc,(unsigned long)series->to_utc);
 string(&w,series->key.device_id);append(&w,",\"generation\":%u,\"channel\":%u,\"kind\":%u,\"layout_id\":%lu,\"columns\":[\"utc\",\"w\",\"v\",\"a\",\"wh\",\"flags\",\"w_count\",\"v_count\",\"a_count\"],\"points\":[",series->key.generation,series->key.channel,series->key.kind,(unsigned long)series->key.layout_id);
 for(unsigned i=0;i<series->count;i++){
  const dsr_history_point_t *p=&series->points[i];append(&w,"%s[%lu,",i?",":"",(unsigned long)p->utc);
  value(&w,p->mask&DSR_HISTORY_W,p->power);append(&w,",");value(&w,p->mask&DSR_HISTORY_V,p->voltage);append(&w,",");value(&w,p->mask&DSR_HISTORY_A,p->current);append(&w,",");value(&w,p->mask&DSR_HISTORY_WH,p->energy_wh);
  append(&w,",%u,%u,%u,%u]",p->flags,p->count[0],p->count[1],p->count[2]);
 }append(&w,"]}");dsr_history_series_free(series);if(!w.ok){free(out);return NULL;}return out;
}
