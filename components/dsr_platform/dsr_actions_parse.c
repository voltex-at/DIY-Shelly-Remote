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
 const cJSON *id=NULL,*revision=NULL,*boot=NULL,*generation=NULL,*nonce=NULL,*on=NULL,*command=NULL,*value=NULL,*rgb=NULL,*v;bool ok=true;
 cJSON_ArrayForEach(v,r){
  const cJSON **slot=!strcmp(v->string,"id")?&id:!strcmp(v->string,"revision")?&revision:!strcmp(v->string,"boot_id")?&boot:!strcmp(v->string,"admission_generation")?&generation:!strcmp(v->string,"nonce")?&nonce:!kind&&!strcmp(v->string,"on")?&on:!kind&&!strcmp(v->string,"command")?&command:!kind&&!strcmp(v->string,"value")?&value:!kind&&!strcmp(v->string,"rgb")?&rgb:NULL;
  if(!slot||*slot){ok=false;break;}*slot=v;
 }
 ok=ok&&integer(id,1,1000000)&&integer(revision,1,UINT32_MAX-1)&&integer(boot,1,UINT32_MAX)&&integer(generation,0,UINT32_MAX-1)&&cJSON_IsString(nonce)&&strlen(nonce->valuestring)==16;
 dsr_group_intent_t intent={0};
 if(ok&&!kind){
  if(on) {ok=cJSON_IsBool(on)&&!command&&!value&&!rgb;intent.command=cJSON_IsTrue(on)?DSR_ACTION_ON:DSR_ACTION_OFF;}
  else if(cJSON_IsString(command)){
   const char *names[]={"on","off","open","close","stop","position","brightness","rgb","target"};
   for(unsigned i=0;i<9;i++)if(!strcmp(command->valuestring,names[i]))intent.command=(uint8_t)(i+1);
   ok=intent.command!=0;
   if(intent.command==DSR_ACTION_RGB){ok=ok&&!value&&cJSON_IsArray(rgb)&&cJSON_GetArraySize(rgb)==3;for(unsigned i=0;ok&&i<3;i++){const cJSON *c=cJSON_GetArrayItem(rgb,(int)i);ok=integer(c,0,255);if(ok)intent.value[i]=(uint8_t)c->valueint;}}
   else if(intent.command==DSR_ACTION_TARGET||intent.command==DSR_ACTION_POSITION||intent.command==DSR_ACTION_BRIGHTNESS){ok=ok&&!rgb&&integer(value,intent.command==DSR_ACTION_TARGET?DSR_TRV_MIN_HALF:0,intent.command==DSR_ACTION_TARGET?DSR_TRV_MAX_HALF:100);if(ok)intent.value[0]=(uint8_t)value->valueint;}
   else ok=ok&&!value&&!rgb;
  }else ok=false;
 }
 if(ok)for(unsigned i=0;i<16;i++){char c=nonce->valuestring[i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f'))){ok=false;break;}}
 if(ok){out->object_id=(uint32_t)id->valuedouble;out->revision=(uint32_t)revision->valuedouble;out->boot_id=(uint32_t)boot->valuedouble;out->admission_generation=(uint32_t)generation->valuedouble;memcpy(out->nonce,nonce->valuestring,17);out->kind=kind;out->on=cJSON_IsTrue(on);out->intent=intent;}
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
  if(!request->kind){plan.layout_ids[plan.count]=(uint32_t)v->valueint;plan.commands[plan.count]=request->intent.command?request->intent.command:request->on?DSR_ACTION_ON:DSR_ACTION_OFF;memcpy(plan.values[plan.count],request->intent.value,3);}
  else{
   plan.layout_ids[plan.count]=(uint32_t)cJSON_GetObjectItemCaseSensitive(v,"device")->valueint;
   const char *command=cJSON_GetObjectItemCaseSensitive(v,"command")->valuestring;
   plan.commands[plan.count]=!strcmp(command,"on")?DSR_ACTION_ON:!strcmp(command,"off")?DSR_ACTION_OFF:!strcmp(command,"open")?DSR_ACTION_OPEN:!strcmp(command,"close")?DSR_ACTION_CLOSE:!strcmp(command,"stop")?DSR_ACTION_STOP:DSR_ACTION_TARGET;
   if(plan.commands[plan.count]==DSR_ACTION_TARGET)plan.values[plan.count][0]=(uint8_t)cJSON_GetObjectItemCaseSensitive(v,"value")->valueint;
  }
  plan.count++;
 }
 bool found=object!=NULL;cJSON_Delete(r);if(!found||!plan.count)return DSR_INVALID;*out=plan;return DSR_OK;
}
