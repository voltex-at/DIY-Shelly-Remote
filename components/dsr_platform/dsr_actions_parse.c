#include "dsr_actions_private.h"
#include "dsr_config.h"
#include <math.h>
#include <string.h>
static bool integer(const cJSON *v,uint32_t min,uint32_t max)
{return cJSON_IsNumber(v)&&isfinite(v->valuedouble)&&floor(v->valuedouble)==v->valuedouble&&v->valuedouble>=min&&v->valuedouble<=max;}
bool dsr_actions_request_parse(const char *json,uint8_t kind,dsr_actions_request_t *out)
{
 if(out)memset(out,0,sizeof(*out));
 if(!out||!json||kind>1||strlen(json)>256||!dsr_config_json_depth(json)||strstr(json,"\\u0000"))return false;
 const char *end=NULL;cJSON *r=cJSON_ParseWithOpts(json,&end,true);if(!cJSON_IsObject(r)){cJSON_Delete(r);return false;}
 const cJSON *id=NULL,*revision=NULL,*boot=NULL,*generation=NULL,*nonce=NULL,*on=NULL,*v;bool ok=true;
 cJSON_ArrayForEach(v,r){
  const cJSON **slot=!strcmp(v->string,"id")?&id:!strcmp(v->string,"revision")?&revision:!strcmp(v->string,"boot_id")?&boot:!strcmp(v->string,"admission_generation")?&generation:!strcmp(v->string,"nonce")?&nonce:!kind&&!strcmp(v->string,"on")?&on:NULL;
  if(!slot||*slot){ok=false;break;}*slot=v;
 }
 ok=ok&&integer(id,1,1000000)&&integer(revision,1,UINT32_MAX-1)&&integer(boot,1,UINT32_MAX)&&integer(generation,0,UINT32_MAX-1)&&cJSON_IsString(nonce)&&strlen(nonce->valuestring)==16&&(kind||cJSON_IsBool(on));
 if(ok)for(unsigned i=0;i<16;i++){char c=nonce->valuestring[i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f'))){ok=false;break;}}
 if(ok){out->object_id=(uint32_t)id->valuedouble;out->revision=(uint32_t)revision->valuedouble;out->boot_id=(uint32_t)boot->valuedouble;out->admission_generation=(uint32_t)generation->valuedouble;memcpy(out->nonce,nonce->valuestring,17);out->kind=kind;out->on=cJSON_IsTrue(on);}
 cJSON_Delete(r);return ok;
}
dsr_result_t dsr_actions_plan_build(const char *json,const dsr_actions_request_t *request,dsr_actions_plan_t *out)
{
 if(out)memset(out,0,sizeof(*out));
 if(!json||!request||!out||request->kind>1||strlen(json)>DSR_CONFIG_MAX_JSON||!dsr_config_json_depth(json)||strstr(json,"\\u0000"))return DSR_INVALID;
 const char *end=NULL;cJSON *r=cJSON_ParseWithOpts(json,&end,true);char error[96];
 if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);return DSR_INVALID;}
 if((uint32_t)cJSON_GetObjectItemCaseSensitive(r,"revision")->valuedouble!=request->revision){cJSON_Delete(r);return DSR_CONFLICT;}
 const cJSON *object=NULL,*v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(r,request->kind?"scenes":"groups"))if((uint32_t)cJSON_GetObjectItemCaseSensitive(v,"id")->valueint==request->object_id){object=v;break;}
 dsr_actions_plan_t plan={.kind=request->kind,.object_id=request->object_id,.revision=request->revision};
 const cJSON *list=cJSON_GetObjectItemCaseSensitive(object,request->kind?"actions":"members");
 cJSON_ArrayForEach(v,list){
  if(plan.count>=DSR_ACTION_STEPS){cJSON_Delete(r);return DSR_FULL;}
  if(!request->kind){plan.layout_ids[plan.count]=(uint32_t)v->valueint;plan.commands[plan.count]=request->on?DSR_ACTION_ON:DSR_ACTION_OFF;}
  else{
   plan.layout_ids[plan.count]=(uint32_t)cJSON_GetObjectItemCaseSensitive(v,"device")->valueint;
   const char *command=cJSON_GetObjectItemCaseSensitive(v,"command")->valuestring;
   plan.commands[plan.count]=!strcmp(command,"on")?DSR_ACTION_ON:!strcmp(command,"off")?DSR_ACTION_OFF:!strcmp(command,"open")?DSR_ACTION_OPEN:!strcmp(command,"close")?DSR_ACTION_CLOSE:DSR_ACTION_STOP;
  }
  plan.count++;
 }
 bool found=object!=NULL;cJSON_Delete(r);if(!found||!plan.count)return DSR_INVALID;*out=plan;return DSR_OK;
}
