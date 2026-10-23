#include "dsr_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
bool old_validate(const cJSON *,char error[96]);
char *old_store_load(const char *,bool *,char error[96]);
bool old_store_save(const char *,const char *,char error[96]);
static unsigned attempts,fail_at,live;
static void *allocated[4096];
static void *allocate(size_t n){attempts++;if(attempts==fail_at)return NULL;void *p=malloc(n);if(p){unsigned i=0;while(i<4096&&allocated[i])i++;CHECK(i<4096);allocated[i]=p;live++;}return p;}
static void release(void *p){if(!p)return;unsigned i=0;while(i<4096&&allocated[i]!=p)i++;CHECK(i<4096&&live);allocated[i]=NULL;live--;free(p);}
static size_t limit=SIZE_MAX,written;
size_t __real_fwrite(const void *,size_t,size_t,FILE *);
size_t __wrap_fwrite(const void *p,size_t s,size_t n,FILE *f){size_t bytes=s*n;if(bytes>limit-written)bytes=limit-written;size_t done=__real_fwrite(p,1,bytes,f);written+=done;return s?done/s:0;}
static void put(const char *dir,unsigned slot,const char *json){char p[256];snprintf(p,sizeof(p),"%s/config%u.dat",dir,slot);FILE *f=fopen(p,"wb");CHECK(f);uint32_t h[]={0x44535231,(uint32_t)strlen(json),dsr_config_crc32(json,strlen(json)),dsr_config_revision(json)};CHECK(fwrite(h,sizeof(h),1,f)==1&&fwrite(json,1,strlen(json),f)==strlen(json));CHECK(!fclose(f));}
static void remove_slot(const char *dir,unsigned slot){char p[256];snprintf(p,sizeof(p),"%s/config%u.dat",dir,slot);unlink(p);}
int main(int argc,char **argv)
{
 CHECK(argc==2);char error[96];char *clean=dsr_config_default();CHECK(clean);cJSON *r=cJSON_Parse(clean);CHECK(dsr_config_validate(r,error)&&dsr_config_revision(clean)==1);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(r,"schema"),1);cJSON_DeleteItemFromObjectCaseSensitive(r,"groups");char *legacy=cJSON_PrintUnformatted(r);CHECK(legacy&&old_validate(r,error)&&dsr_config_validate_stored(r,error)&&!dsr_config_validate(r,error));
 bool was_old=false;char *up=dsr_config_upgrade(legacy,&was_old,error);CHECK(up&&was_old&&dsr_config_revision(up)==1);cJSON *u=cJSON_Parse(up);CHECK(dsr_config_validate(u,error)&&!old_validate(u,error));CHECK(cJSON_IsArray(cJSON_GetObjectItemCaseSensitive(u,"groups"))&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(u,"groups"))==0);
 /* The schema1 path is exactly known-fields validation, not parser2 guessing. */
 cJSON_AddArrayToObject(r,"groups");CHECK(!old_validate(r,error)&&!dsr_config_validate_stored(r,error));char *wrong=cJSON_PrintUnformatted(r);CHECK(!dsr_config_upgrade(wrong,NULL,error));free(wrong);cJSON_DeleteItemFromObjectCaseSensitive(r,"groups");
 for(unsigned n=0;n<100;n++){char key[48];snprintf(key,sizeof(key),"unknown%u",n);cJSON_AddNumberToObject(r,key,n);CHECK(!old_validate(r,error)&&!dsr_config_validate_stored(r,error));cJSON_DeleteItemFromObjectCaseSensitive(r,key);}
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(u,"devices"),cJSON_Parse("{\"id\":1,\"name\":\"Switch\",\"host\":\"192.0.2.1\",\"channel\":0,\"kind\":\"relay\",\"room\":0}"));cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(u,"groups"),cJSON_Parse("{\"id\":1,\"name\":\"Group\",\"members\":[1]}"));CHECK(dsr_config_validate(u,error));cJSON *members=cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(u,"groups"),0),"members");cJSON_AddItemToArray(members,cJSON_CreateNumber(1));CHECK(!dsr_config_validate(u,error));cJSON_DeleteItemFromArray(members,1);cJSON_ReplaceItemInArray(members,0,cJSON_CreateNumber(2));CHECK(!dsr_config_validate(u,error));cJSON_ReplaceItemInArray(members,0,cJSON_CreateNumber(1));cJSON *device=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(u,"devices"),0);cJSON_ReplaceItemInObjectCaseSensitive(device,"kind",cJSON_CreateString("cover"));CHECK(dsr_config_validate(u,error));cJSON_ReplaceItemInObjectCaseSensitive(device,"kind",cJSON_CreateString("relay"));CHECK(dsr_config_validate(u,error));
 /* Optional device "icon" (0.7.51): absent or integer 0..20; anything else is refused. */
 {cJSON *dev=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(u,"devices"),0);CHECK(dev);
  const double good[]={0,1,11,DSR_CONFIG_DEVICE_ICONS},bad[]={-1,DSR_CONFIG_DEVICE_ICONS+1,2.5,1e9};
  for(unsigned i=0;i<4;i++){cJSON_DeleteItemFromObjectCaseSensitive(dev,"icon");cJSON_AddNumberToObject(dev,"icon",good[i]);CHECK(dsr_config_validate(u,error));}
  for(unsigned i=0;i<4;i++){cJSON_DeleteItemFromObjectCaseSensitive(dev,"icon");cJSON_AddNumberToObject(dev,"icon",bad[i]);CHECK(!dsr_config_validate(u,error));}
  cJSON_DeleteItemFromObjectCaseSensitive(dev,"icon");cJSON_AddStringToObject(dev,"icon","tv");CHECK(!dsr_config_validate(u,error));
  cJSON_DeleteItemFromObjectCaseSensitive(dev,"icon");cJSON_AddNullToObject(dev,"icon");CHECK(!dsr_config_validate(u,error));
  cJSON_DeleteItemFromObjectCaseSensitive(dev,"icon");CHECK(dsr_config_validate(u,error));}
 /* Every genuine cJSON allocation failure leaves no partial upgrade/leak. */
 cJSON_Hooks hooks={allocate,release};cJSON_InitHooks(&hooks);attempts=0;char *copy=dsr_config_upgrade(legacy,NULL,error);CHECK(copy);unsigned total=attempts;cJSON_free(copy);CHECK(!live);for(unsigned n=1;n<=total;n++){attempts=0;fail_at=n;CHECK(!dsr_config_upgrade(legacy,NULL,error)&&!live);}fail_at=0;cJSON_InitHooks(NULL);
 /* Mixed1/2 and every byte interruption of first explicit upgraded Save. */
 put(argv[1],0,legacy);bool found;char *loaded=dsr_config_store_load(argv[1],&found,error);CHECK(loaded&&found&&!strcmp(loaded,legacy));free(loaded);char *candidate=cJSON_PrintUnformatted(u);char *next=dsr_config_replace(up,candidate,1,error);CHECK(next&&dsr_config_revision(next)==2);CHECK(!dsr_config_replace(up,candidate,2,error));
 size_t size=16+strlen(next);for(size_t cut=0;cut<size;cut++){remove_slot(argv[1],1);written=0;limit=cut;CHECK(!dsr_config_store_save(argv[1],next,error));limit=SIZE_MAX;loaded=dsr_config_store_load(argv[1],&found,error);CHECK(loaded&&found&&!strcmp(loaded,legacy));free(loaded);}
 CHECK(dsr_config_store_save(argv[1],next,error));loaded=dsr_config_store_load(argv[1],&found,error);CHECK(loaded&&found&&!strcmp(loaded,next));free(loaded);loaded=old_store_load(argv[1],&found,error);CHECK(!loaded&&!found&&strstr(error,"Newer"));CHECK(!old_store_save(argv[1],legacy,error));
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(u,"schema"),3);char *future=cJSON_PrintUnformatted(u);put(argv[1],1,future);loaded=dsr_config_store_load(argv[1],&found,error);CHECK(!loaded&&!found&&strstr(error,"Newer"));CHECK(!dsr_config_store_save(argv[1],next,error)&&!dsr_config_upgrade(future,NULL,error));
 cJSON_Delete(r);cJSON_Delete(u);free(clean);free(legacy);free(up);free(candidate);free(next);free(future);printf("PASS: %u actual schema2 checks; frozen schema1 strict path/old-firmware refusal, %u cJSON OOM and %zu byte powercuts, mixed1/2/future3 preservation; host files only\n",checks,total,size);return 0;
}
