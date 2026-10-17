#include "dsr_actions.h"
#include "dsr_config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static bool number(const cJSON *v,uint32_t min,uint32_t max)
{return cJSON_IsNumber(v)&&isfinite(v->valuedouble)&&floor(v->valuedouble)==v->valuedouble&&v->valuedouble>=min&&v->valuedouble<=max;}
static cJSON *parse(const char *json,const char *const *keys,size_t count)
{
 if(!json||strlen(json)>256||!dsr_config_json_depth(json)||strstr(json,"\\u0000"))return NULL;
 cJSON *r=cJSON_ParseWithOpts(json,NULL,true);bool valid=cJSON_IsObject(r);size_t n=0;const cJSON *v;
 cJSON_ArrayForEach(v,r){bool known=false;for(size_t i=0;i<count;i++)if(v->string&&!strcmp(v->string,keys[i]))known=true;if(!known)valid=false;for(const cJSON *b=v->next;b;b=b->next)if(b->string&&v->string&&!strcmp(b->string,v->string))valid=false;n++;}
 if(!valid||n!=count){cJSON_Delete(r);return NULL;}return r;
}
bool dsr_actions_cancel_parse(const char *json,uint32_t *boot,uint32_t *run)
{
 const char *keys[]={"boot_id","run_id"};cJSON *r=parse(json,keys,2);const cJSON *b=cJSON_GetObjectItemCaseSensitive(r,"boot_id"),*id=cJSON_GetObjectItemCaseSensitive(r,"run_id");bool ok=boot&&run&&number(b,1,UINT32_MAX)&&number(id,1,UINT32_MAX);if(ok){*boot=(uint32_t)b->valuedouble;*run=(uint32_t)id->valuedouble;}cJSON_Delete(r);return ok;
}
bool dsr_actions_log_parse(const char *json,uint32_t *epoch,unsigned *offset,unsigned *limit)
{
 const char *keys[]={"epoch","offset","limit"};cJSON *r=parse(json,keys,3);const cJSON *e=cJSON_GetObjectItemCaseSensitive(r,"epoch"),*o=cJSON_GetObjectItemCaseSensitive(r,"offset"),*l=cJSON_GetObjectItemCaseSensitive(r,"limit");bool ok=epoch&&offset&&limit&&number(e,0,UINT32_MAX)&&number(o,0,DSR_ACTION_LOG_COUNT)&&number(l,1,DSR_ACTION_LOG_PAGE);if(ok){*epoch=(uint32_t)e->valuedouble;*offset=(unsigned)o->valueint;*limit=(unsigned)l->valueint;}cJSON_Delete(r);return ok;
}
char *dsr_actions_status_json(void)
{
 dsr_actions_snapshot_t snapshot;dsr_actions_snapshot(&snapshot);cJSON *r=cJSON_CreateObject(),*steps=NULL;bool ok=r&&
 cJSON_AddNumberToObject(r,"boot_id",snapshot.boot_id)&&cJSON_AddNumberToObject(r,"admission_generation",snapshot.admission_generation)&&cJSON_AddNumberToObject(r,"revision",snapshot.revision)&&cJSON_AddNumberToObject(r,"run_id",snapshot.run_id)&&cJSON_AddNumberToObject(r,"object_id",snapshot.object_id)&&cJSON_AddStringToObject(r,"nonce",snapshot.nonce)&&cJSON_AddNumberToObject(r,"kind",snapshot.kind)&&cJSON_AddBoolToObject(r,"available",snapshot.available)&&cJSON_AddBoolToObject(r,"active",snapshot.active)&&cJSON_AddBoolToObject(r,"cancelled",snapshot.cancelled)&&cJSON_AddNumberToObject(r,"current",snapshot.current);
 if(ok){steps=cJSON_AddArrayToObject(r,"steps");ok=steps!=NULL;}
 for(unsigned i=0;i<snapshot.count&&ok;i++){const dsr_action_result_t *s=&snapshot.steps[i];cJSON *v=cJSON_CreateObject();if(!v){ok=false;break;}ok=cJSON_AddItemToArray(steps,v);if(!ok){cJSON_Delete(v);break;}ok=cJSON_AddNumberToObject(v,"layout_id",s->layout_id)&&cJSON_AddNumberToObject(v,"command",s->command)&&cJSON_AddNumberToObject(v,"outcome",s->outcome)&&cJSON_AddNumberToObject(v,"result",s->result);}
 char *out=ok?cJSON_PrintUnformatted(r):NULL;cJSON_Delete(r);return out;
}
char *dsr_actions_log_json(uint32_t epoch,unsigned offset,unsigned limit)
{
 dsr_action_log_page_t *page=calloc(1,sizeof(*page));if(!page)return NULL;if(!dsr_actions_log(epoch,offset,limit,page)){free(page);return NULL;}
 cJSON *r=cJSON_CreateObject(),*array=NULL;bool ok=r&&cJSON_AddNumberToObject(r,"epoch",page->epoch)&&cJSON_AddNumberToObject(r,"newest",page->newest)&&cJSON_AddNumberToObject(r,"total",page->total)&&cJSON_AddStringToObject(r,"timezone","UTC");if(ok){array=cJSON_AddArrayToObject(r,"entries");ok=array!=NULL;}
 for(unsigned i=0;i<page->count&&ok;i++){dsr_action_log_entry_t *e=&page->entries[i];cJSON *v=cJSON_CreateObject();if(!v){ok=false;break;}ok=cJSON_AddItemToArray(array,v);if(!ok){cJSON_Delete(v);break;}
  ok=cJSON_AddNumberToObject(v,"sequence",e->sequence)&&cJSON_AddNumberToObject(v,"utc",e->utc)&&cJSON_AddNumberToObject(v,"monotonic_ms",e->monotonic_ms)&&cJSON_AddNumberToObject(v,"run_id",e->run_id)&&cJSON_AddNumberToObject(v,"object_id",e->object_id)&&cJSON_AddNumberToObject(v,"layout_id",e->layout_id)&&cJSON_AddNumberToObject(v,"source",e->source)&&cJSON_AddNumberToObject(v,"kind",e->kind)&&cJSON_AddNumberToObject(v,"command",e->command)&&cJSON_AddNumberToObject(v,"outcome",e->outcome)&&cJSON_AddNumberToObject(v,"result",e->result);
 }
 char *out=ok?cJSON_PrintUnformatted(r):NULL;cJSON_Delete(r);free(page);return out;
}
