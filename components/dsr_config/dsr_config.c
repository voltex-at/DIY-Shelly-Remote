#include "dsr_config.h"
#include "dsr_core.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool fail(char error[96],const char *message){snprintf(error,96,"%s",message);return false;}
static const cJSON *get(const cJSON *o,const char *k){return cJSON_GetObjectItemCaseSensitive(o,k);}
bool dsr_config_json_depth(const char *json)
{
 unsigned depth=0;bool string=false,escape=false;
 for(const unsigned char *p=(const unsigned char *)json;*p;p++){
  if(string){if(escape)escape=false;else if(*p=='\\')escape=true;else if(*p=='"')string=false;}
  else if(*p=='"')string=true;else if(*p=='{'||*p=='['){if(++depth>16)return false;}
  else if(*p=='}'||*p==']'){if(!depth)return false;depth--;}
 }
 return depth==0&&!string;
}
static bool number(const cJSON *o,const char *k,unsigned low,unsigned high)
{const cJSON *n=get(o,k);return cJSON_IsNumber(n)&&isfinite(n->valuedouble)&&n->valuedouble>=low&&n->valuedouble<=high&&floor(n->valuedouble)==n->valuedouble;}
static bool text(const cJSON *o,const char *k,size_t maximum,bool empty)
{
 const cJSON *v=get(o,k);if(!cJSON_IsString(v)||(!empty&&!v->valuestring[0])||strlen(v->valuestring)>maximum)return false;
 for(const unsigned char *p=(const unsigned char *)v->valuestring;*p;p++)if(*p<32||*p==127)return false;
 return true;
}
static bool choice(const cJSON *o,const char *k,const char *choices)
{const cJSON *v=get(o,k);if(!cJSON_IsString(v)||strlen(v->valuestring)>31||strchr(v->valuestring,'|'))return false;char wanted[48];snprintf(wanted,sizeof(wanted),"|%s|",v->valuestring);return strstr(choices,wanted)!=NULL;}
static bool keys(const cJSON *o,const char *allowed)
{const cJSON *v;cJSON_ArrayForEach(v,o){char key[80];if(!v->string||strlen(v->string)>70||strchr(v->string,'|'))return false;snprintf(key,sizeof(key),"|%s|",v->string);if(!strstr(allowed,key))return false;}return true;}
static bool unique_keys(const cJSON *o,unsigned depth)
{
 if(depth>16)return false;
 const cJSON *a;
 cJSON_ArrayForEach(a,o){if(cJSON_IsObject(o)){if(!a->string)return false;for(const cJSON *b=a->next;b;b=b->next)if(b->string&&!strcmp(a->string,b->string))return false;}
  if((cJSON_IsObject(a)||cJSON_IsArray(a))&&!unique_keys(a,depth+1))return false;}
 return true;
}
static bool collection(const cJSON *root,const char *key,int max)
{
 const cJSON *a=get(root,key);if(!cJSON_IsArray(a)||cJSON_GetArraySize(a)>max)return false;
 const cJSON *v;cJSON_ArrayForEach(v,a){if(!cJSON_IsObject(v)||!number(v,"id",1,1000000)||!text(v,"name",47,!strcmp(key,"devices")&&number(root,"schema",2,2)))return false;
  for(const cJSON *b=v->next;b;b=b->next)if(number(b,"id",1,1000000)&&get(b,"id")->valueint==get(v,"id")->valueint)return false;}
 return true;
}
static bool reference(const cJSON *root,const char *key,const cJSON *id,bool zero)
{
 if(!cJSON_IsNumber(id)||id->valuedouble!=floor(id->valuedouble))return false;
 if(zero&&id->valueint==0)return true;
 const cJSON *v;cJSON_ArrayForEach(v,get(root,key))if(get(v,"id")->valueint==id->valueint)return true;
 return false;
}
/* Exactly three skins. "midnight" (retired in 0.7.68-dev) is accepted only
 * from a stored document so it can be migrated; it is never valid output. */
static bool theme(const cJSON *r,bool stored)
{return choice(r,"theme",stored?"|graphite||midnight||light||cyber_blue|":"|graphite||light||cyber_blue|");}
bool dsr_config_retire_theme(cJSON *root)
{
 const cJSON *value=get(root,"theme");
 if(!cJSON_IsString(value)||strcmp(value->valuestring,"midnight"))return false;
 return cJSON_ReplaceItemInObjectCaseSensitive(root,"theme",cJSON_CreateString("graphite"));
}
static bool validate_v1(const cJSON *r,char error[96])
{
 if(!cJSON_IsObject(r)||!unique_keys(r,0))return fail(error,"Invalid object or duplicate keys");
 if(!keys(r,"|schema||revision||language||theme||accent||active_dashboard||rooms||devices||scenes||dashboards|"))return fail(error,"Unknown configuration field");
 if(!number(r,"schema",1,1)||!number(r,"revision",1,UINT32_MAX-1))return fail(error,"Unsupported schema/revision");
 if(!choice(r,"language","|pl||en||de|")||!theme(r,true)||!number(r,"accent",0,0xffffff))return fail(error,"Invalid appearance");
 if(!collection(r,"rooms",8)||!collection(r,"devices",16)||!collection(r,"scenes",8)||!collection(r,"dashboards",DSR_CONFIG_MAX_DASHBOARDS))return fail(error,"Invalid collection, duplicate ID or limit");
 if(!reference(r,"dashboards",get(r,"active_dashboard"),false))return fail(error,"Select an existing dashboard");
 const cJSON *v;
 cJSON_ArrayForEach(v,get(r,"rooms")){
  if(!keys(v,"|id||name||icon||photo|")||!number(v,"icon",0,3)||!text(v,"photo",32,true))return fail(error,"Invalid room");
  const char *photo=get(v,"photo")->valuestring;if(photo[0]){if(strlen(photo)!=32)return fail(error,"Invalid photo ID");for(unsigned i=0;i<32;i++)if(!((photo[i]>='a'&&photo[i]<='f')||(photo[i]>='0'&&photo[i]<='9')))return fail(error,"Invalid photo ID");}
 }
 cJSON_ArrayForEach(v,get(r,"devices")){
  const cJSON *host=get(v,"host");
  if(!keys(v,"|id||name||host||channel||kind||room|")||!text(v,"host",15,true)||(host->valuestring[0]&&!dsr_ipv4_valid(host->valuestring))||!number(v,"channel",0,15)||
   !choice(v,"kind","|light||relay||plug||cover||contact||sensor|")||!reference(r,"rooms",get(v,"room"),true))return fail(error,"Invalid device address/type/room");
 }
 cJSON_ArrayForEach(v,get(r,"scenes")){
  const cJSON *actions=get(v,"actions"),*a;if(!keys(v,"|id||name||actions|")||!cJSON_IsArray(actions)||cJSON_GetArraySize(actions)>8)return fail(error,"Scene limit: 8 explicit actions");
  cJSON_ArrayForEach(a,actions)if(!keys(a,"|device||command|")||!reference(r,"devices",get(a,"device"),false)||!choice(a,"command","|on||off||open||close||stop|"))return fail(error,"Invalid scene action");
 }
 cJSON_ArrayForEach(v,get(r,"dashboards")){
  const cJSON *tiles=get(v,"tiles"),*t;if(!keys(v,"|id||name||tiles|")||!cJSON_IsArray(tiles)||cJSON_GetArraySize(tiles)>DSR_CONFIG_MAX_TILES)return fail(error,"Dashboard tile limit");
  cJSON_ArrayForEach(t,tiles){
   if(!keys(t,"|id||type||ref||span||height||fields|")||!number(t,"id",1,1000000)||!choice(t,"type","|device||room||scene|")||!number(t,"span",1,2)||!number(t,"height",1,2)||!number(t,"fields",0,255))return fail(error,"Invalid tile");
   const char *type=get(t,"type")->valuestring;
   if(strcmp(type,"room")&&get(t,"span")->valueint!=2)return fail(error,"Device and scene cards must use full width");
   if(!reference(r,!strcmp(type,"device")?"devices":!strcmp(type,"room")?"rooms":"scenes",get(t,"ref"),false))return fail(error,"Tile references missing object");
   for(const cJSON *b=t->next;b;b=b->next)if(number(b,"id",1,1000000)&&get(b,"id")->valueint==get(t,"id")->valueint)return fail(error,"Duplicate tile ID");
  }
 }
 error[0]=0;return true;
}
/* Presentation metadata is optional: old definitions keep their exact meaning.
 * IDs, never paths/URLs, select bounded local photo assets. */
static bool action_appearance(const cJSON *r,const cJSON *o)
{
 if(get(o,"room")&&!reference(r,"rooms",get(o,"room"),true))return false;
 if(!get(o,"photo"))return true;
 if(!text(o,"photo",32,true))return false;
 const char *p=get(o,"photo")->valuestring;if(!*p)return true;
 if(strlen(p)!=32)return false;
 for(unsigned i=0;i<32;i++)if(!((p[i]>='a'&&p[i]<='f')||(p[i]>='0'&&p[i]<='9')))return false;
 return true;
}
static bool validate_v2(const cJSON *r,char error[96],bool stored)
{
 if(!cJSON_IsObject(r)||!unique_keys(r,0))return fail(error,"Invalid object or duplicate keys");
 if(!keys(r,"|schema||revision||language||theme||accent||active_dashboard||rooms||devices||scenes||dashboards||groups|"))return fail(error,"Unknown configuration field");
 if(!number(r,"schema",DSR_CONFIG_SCHEMA,DSR_CONFIG_SCHEMA)||!number(r,"revision",1,UINT32_MAX-1))return fail(error,"Unsupported schema/revision");
 if(!choice(r,"language","|pl||en||de|")||!theme(r,stored)||!number(r,"accent",0,0xffffff))return fail(error,"Invalid appearance");
 if(!collection(r,"rooms",8)||!collection(r,"devices",16)||!collection(r,"scenes",8)||!collection(r,"groups",8)||!collection(r,"dashboards",DSR_CONFIG_MAX_DASHBOARDS))return fail(error,"Invalid collection, duplicate ID or limit");
 if(!reference(r,"dashboards",get(r,"active_dashboard"),false))return fail(error,"Select an existing dashboard");
 const cJSON *v;
 cJSON_ArrayForEach(v,get(r,"rooms")){
  if(!keys(v,"|id||name||icon||photo|")||!number(v,"icon",0,3)||!text(v,"photo",32,true))return fail(error,"Invalid room");
  const char *photo=get(v,"photo")->valuestring;if(photo[0]){if(strlen(photo)!=32)return fail(error,"Invalid photo ID");for(unsigned i=0;i<32;i++)if(!((photo[i]>='a'&&photo[i]<='f')||(photo[i]>='0'&&photo[i]<='9')))return fail(error,"Invalid photo ID");}
 }
 cJSON_ArrayForEach(v,get(r,"devices")){
  const cJSON *host=get(v,"host");
  if(!keys(v,"|id||name||host||channel||kind||room||icon|")||(get(v,"icon")&&!number(v,"icon",0,DSR_CONFIG_DEVICE_ICONS))||!text(v,"host",15,true)||(host->valuestring[0]&&!dsr_ipv4_valid(host->valuestring))||!number(v,"channel",0,15)||
   !choice(v,"kind","|light||relay||plug||cover||contact||sensor|")||!reference(r,"rooms",get(v,"room"),true))return fail(error,"Invalid device address/type/room");
 }
 cJSON_ArrayForEach(v,get(r,"scenes")){
  const cJSON *actions=get(v,"actions"),*a;if(!keys(v,"|id||name||actions||room||photo|")||!action_appearance(r,v)||!cJSON_IsArray(actions)||cJSON_GetArraySize(actions)>8)return fail(error,"Scene limit: 8 explicit actions");
  /* Target values share the actor's half-degree representation (8..60).
   * Stored display kind cannot grant TRV capability; the executor checks the
   * actual private binding again before any write. Schema1 remains frozen. */
  cJSON_ArrayForEach(a,actions){
   if(!keys(a,"|device||command||value|")||!reference(r,"devices",get(a,"device"),false)||!choice(a,"command","|on||off||open||close||stop||target|"))return fail(error,"Invalid scene action");
   if(!strcmp(get(a,"command")->valuestring,"target")?!number(a,"value",DSR_TRV_MIN_HALF,DSR_TRV_MAX_HALF):get(a,"value")!=NULL)return fail(error,"Invalid scene value");
  }
 }
 /* Display kinds do not identify actuators (a BLU TRV is visually a sensor).
  * Member identity, homogeneous control and capability are checked against
  * private bindings by the executor before I/O, never granted by this schema. */
 cJSON_ArrayForEach(v,get(r,"groups")){
  const cJSON *members=get(v,"members"),*member;
  if(!keys(v,"|id||name||members||room||photo|")||!action_appearance(r,v)||!cJSON_IsArray(members)||cJSON_GetArraySize(members)>16)return fail(error,"Group limit: 16 unique members");
  cJSON_ArrayForEach(member,members){
   if(!reference(r,"devices",member,false))return fail(error,"Group references missing device");
   for(const cJSON *other=member->next;other;other=other->next)if(cJSON_IsNumber(other)&&other->valuedouble==member->valuedouble)return fail(error,"Duplicate group member");
  }
 }
 cJSON_ArrayForEach(v,get(r,"dashboards")){
  const cJSON *tiles=get(v,"tiles"),*t;if(!keys(v,"|id||name||tiles|")||!cJSON_IsArray(tiles)||cJSON_GetArraySize(tiles)>DSR_CONFIG_MAX_TILES)return fail(error,"Dashboard tile limit");
  cJSON_ArrayForEach(t,tiles){
   if(!keys(t,"|id||type||ref||span||height||fields|")||!number(t,"id",1,1000000)||!choice(t,"type","|device||room||scene||group|")||!number(t,"span",1,2)||!number(t,"height",1,2)||!number(t,"fields",0,255))return fail(error,"Invalid tile");
   const char *type=get(t,"type")->valuestring;
   if(strcmp(type,"room")&&get(t,"span")->valueint!=2)return fail(error,"Device and scene cards must use full width");
   if(!reference(r,!strcmp(type,"device")?"devices":!strcmp(type,"room")?"rooms":!strcmp(type,"group")?"groups":"scenes",get(t,"ref"),false))return fail(error,"Tile references missing object");
   for(const cJSON *b=t->next;b;b=b->next)if(number(b,"id",1,1000000)&&get(b,"id")->valueint==get(t,"id")->valueint)return fail(error,"Duplicate tile ID");
  }
 }
 error[0]=0;return true;
}
bool dsr_config_validate(const cJSON *root,char error[96])
{return validate_v2(root,error,false);}
cJSON *dsr_config_remove_device(const cJSON *root,uint32_t id,char error[96])
{
 if(!id||!dsr_config_validate(root,error)){fail(error,"Invalid device deletion source");return NULL;}
 const cJSON *devices=get(root,"devices");int at=0;
 while(at<cJSON_GetArraySize(devices)&&(uint32_t)get(cJSON_GetArrayItem(devices,at),"id")->valueint!=id)at++;
 if(at==cJSON_GetArraySize(devices)){fail(error,"Device no longer exists");return NULL;}
 cJSON *next=cJSON_Duplicate(root,true);if(!next){fail(error,"Not enough memory");return NULL;}
 cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(next,"devices"),at);
 cJSON *v;
 cJSON_ArrayForEach(v,get(next,"dashboards")){
  cJSON *a=cJSON_GetObjectItemCaseSensitive(v,"tiles");
  for(int i=cJSON_GetArraySize(a)-1;i>=0;i--){const cJSON *t=cJSON_GetArrayItem(a,i);
   if(!strcmp(get(t,"type")->valuestring,"device")&&(uint32_t)get(t,"ref")->valueint==id)cJSON_DeleteItemFromArray(a,i);
  }
 }
 cJSON_ArrayForEach(v,get(next,"scenes")){
  cJSON *a=cJSON_GetObjectItemCaseSensitive(v,"actions");
  for(int i=cJSON_GetArraySize(a)-1;i>=0;i--)if((uint32_t)get(cJSON_GetArrayItem(a,i),"device")->valueint==id)cJSON_DeleteItemFromArray(a,i);
 }
 cJSON_ArrayForEach(v,get(next,"groups")){
  cJSON *a=cJSON_GetObjectItemCaseSensitive(v,"members");
  for(int i=cJSON_GetArraySize(a)-1;i>=0;i--)if((uint32_t)cJSON_GetArrayItem(a,i)->valueint==id)cJSON_DeleteItemFromArray(a,i);
 }
 if(!dsr_config_validate(next,error)){cJSON_Delete(next);return NULL;}
 return next;
}
bool dsr_config_validate_stored(const cJSON *root,char error[96])
{
 if(cJSON_IsObject(root)&&number(root,"schema",1,1))return validate_v1(root,error);
 return validate_v2(root,error,true);
}
/* Explicit validated upgrade. No filesystem/NVS/worker or revision mutation.
 * The caller publishes canonical2 in RAM and writes only on explicit Save
 * (a schema-2 boot document is refreshed by the existing checkpoint save).
 * The retired midnight skin becomes graphite here, idempotently; every other
 * setting, room, scene, binding id and photo reference is kept byte-exact. */
char *dsr_config_upgrade(const char *json,bool *legacy,char error[96])
{
 if(legacy)*legacy=false;
 if(!json||strlen(json)>DSR_CONFIG_MAX_JSON||!dsr_config_json_depth(json)||strstr(json,"\\u0000")){fail(error,"Configuration size/depth/encoding limit");return NULL;}
 const char *end=NULL;cJSON *root=cJSON_ParseWithOpts(json,&end,true);
 if(!root||!dsr_config_validate_stored(root,error)){cJSON_Delete(root);return NULL;}
 const cJSON *stored_theme=get(root,"theme");
 if(!strcmp(stored_theme->valuestring,"midnight")&&!dsr_config_retire_theme(root)){cJSON_Delete(root);fail(error,"Not enough memory for configuration upgrade");return NULL;}
 bool old=number(root,"schema",1,1);
 if(old){
  cJSON *groups=cJSON_CreateArray();
  if(!groups||!cJSON_AddItemToObject(root,"groups",groups)){cJSON_Delete(groups);cJSON_Delete(root);fail(error,"Not enough memory for configuration upgrade");return NULL;}
  cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(root,"schema"),DSR_CONFIG_SCHEMA);
 }
 char *out=cJSON_PrintUnformatted(root);cJSON_Delete(root);
 if(!out){fail(error,"Not enough memory for configuration upgrade");return NULL;}
 if(strlen(out)>DSR_CONFIG_MAX_JSON){cJSON_free(out);fail(error,"Upgraded configuration too large");return NULL;}
 if(legacy)*legacy=old;
 error[0]=0;return out;
}

char *dsr_config_default(void)
{
 const char *value="{\"schema\":2,\"revision\":1,\"language\":\"en\",\"theme\":\"graphite\",\"accent\":5020661,\"active_dashboard\":1,\"rooms\":[],\"devices\":[],\"scenes\":[],\"groups\":[],\"dashboards\":[{\"id\":1,\"name\":\"Mój pulpit\",\"tiles\":[] }]}";
 char *copy=malloc(strlen(value)+1);if(copy)memcpy(copy,value,strlen(value)+1);return copy;
}
uint32_t dsr_config_revision(const char *json)
{cJSON *r=cJSON_Parse(json);uint32_t v=0;if(r&&number(r,"revision",1,UINT32_MAX-1))v=(uint32_t)get(r,"revision")->valuedouble;cJSON_Delete(r);return v;}
char *dsr_config_replace(const char *current,const char *candidate,uint32_t expected,char error[96])
{
 if(!current||!candidate||strlen(candidate)>DSR_CONFIG_MAX_JSON||!dsr_config_json_depth(candidate)){fail(error,"Configuration size/depth limit");return NULL;}
 uint32_t revision=dsr_config_revision(current);
 if(!revision||revision!=expected){fail(error,"Revision conflict; reload before editing");return NULL;}
 const char *end=NULL;cJSON *r=cJSON_ParseWithOpts(candidate,&end,true);
 if(!r){fail(error,"Malformed JSON");return NULL;}
 /* A page opened before the update may still hold the retired theme. */
 if(cJSON_IsObject(r))(void)dsr_config_retire_theme(r);
 if(!dsr_config_validate(r,error)){cJSON_Delete(r);return NULL;}
 if((uint32_t)get(r,"revision")->valuedouble!=expected){cJSON_Delete(r);fail(error,"Revision conflict; reload before editing");return NULL;}
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(r,"revision"),revision+1);
 char *json=cJSON_PrintUnformatted(r);cJSON_Delete(r);
 if(json&&strlen(json)>DSR_CONFIG_MAX_JSON){cJSON_free(json);json=NULL;fail(error,"Configuration too large");}
 return json;
}
