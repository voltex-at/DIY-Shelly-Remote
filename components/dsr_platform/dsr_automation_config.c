#include "dsr_automation_config.h"
#include "dsr_config.h"
#include <string.h>
static const cJSON *get(const cJSON *v,const char *k){return cJSON_GetObjectItemCaseSensitive(v,k);}
static unsigned index_of(const dsr_auto_rules_t *r,uint32_t id)
{for(unsigned i=0;i<r->key_count;i++)if(r->keys[i].layout_id==id)return i;return DSR_AUTO_PARTICIPANTS;}
static uint32_t hash(uint32_t h,uint32_t v)
{for(unsigned i=0;i<4;i++){h^=(v>>(8*i))&255u;h*=16777619u;}return h;}
bool dsr_auto_rules_parse(const cJSON *root,dsr_auto_rules_t *out,char error[96])
{
 if(!out)return false;
 memset(out,0,sizeof(*out));if(!dsr_config_validate(root,error))return false;
 const cJSON *v,*id;
 cJSON_ArrayForEach(v,get(root,"automations")){
  const char *lists[]={"sensors","targets"};
  for(unsigned role=0;role<2;role++)cJSON_ArrayForEach(id,get(v,lists[role])){
   uint32_t value=(uint32_t)id->valuedouble;unsigned at=index_of(out,value);
   if(at==DSR_AUTO_PARTICIPANTS){
    if(out->key_count==DSR_AUTO_PARTICIPANTS)return false;
    at=out->key_count++;out->keys[at].layout_id=value;out->keys[at].kind=role?DSR_AUTO_TRV:DSR_AUTO_CONTACT;
   }
  }
 }
 /* Stable canonical order: editing device list order cannot re-enroll rules. */
 for(unsigned i=1;i<out->key_count;i++)for(unsigned j=i;j&&out->keys[j].layout_id<out->keys[j-1].layout_id;j--){
  dsr_auto_key_t k=out->keys[j];out->keys[j]=out->keys[j-1];out->keys[j-1]=k;
 }
 cJSON_ArrayForEach(v,get(root,"automations")){
  dsr_auto_rule_t *r=&out->items[out->count++];r->id=(uint32_t)get(v,"id")->valuedouble;
  r->target_half=get(v,"target_half")?(uint8_t)get(v,"target_half")->valueint:16;r->enabled=cJSON_IsTrue(get(v,"enabled"));
  cJSON_ArrayForEach(id,get(v,"sensors"))r->sensors|=(uint16_t)(1u<<index_of(out,(uint32_t)id->valuedouble));
  cJSON_ArrayForEach(id,get(v,"targets"))r->targets|=(uint16_t)(1u<<index_of(out,(uint32_t)id->valuedouble));
  r->signature=hash(hash(hash(hash(2166136261u,r->id),r->sensors),r->targets),r->target_half);
  if(!r->signature)r->signature=1;
 }
 for(unsigned i=1;i<out->count;i++)for(unsigned j=i;j&&out->items[j].id<out->items[j-1].id;j--){
  dsr_auto_rule_t r=out->items[j];out->items[j]=out->items[j-1];out->items[j-1]=r;
 }
 return true;
}
