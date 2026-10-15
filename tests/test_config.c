#include "dsr_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}} while(0)
static void invalid(cJSON *r,const char *key,cJSON *value)
{char error[96];cJSON *copy=cJSON_Duplicate(r,true);cJSON_ReplaceItemInObjectCaseSensitive(copy,key,value);CHECK(!dsr_config_validate(copy,error));cJSON_Delete(copy);}
static void photo_gc(cJSON *source,const char *base)
{
 char dir[512],file[600],id[33],error[96];snprintf(dir,sizeof(dir),"%s/photos",base);CHECK(!mkdir(dir,0700));
 cJSON *doc=cJSON_Duplicate(source,true),*room=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(doc,"rooms"),0);
 for(unsigned i=1;i<=18;i++){snprintf(id,sizeof(id),"%032x",i);snprintf(file,sizeof(file),"%s/p%s.rgb",dir,id);FILE *f=fopen(file,"wb");CHECK(f);CHECK(fputc(1,f)!=EOF);CHECK(!fclose(f));}
 for(unsigned i=1;i<=2;i++){snprintf(id,sizeof(id),"%032x",i);cJSON_ReplaceItemInObjectCaseSensitive(room,"photo",cJSON_CreateString(id));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(doc,"revision"),i);char *json=cJSON_PrintUnformatted(doc);CHECK(dsr_config_store_save(dir,json,error));free(json);}
 snprintf(file,sizeof(file),"%s/unknown.rgb",dir);FILE *f=fopen(file,"wb");CHECK(f);fclose(f);
 unsigned removed=99;CHECK(dsr_config_store_collect_photos(dir,&removed,error));CHECK(removed==16);
 for(unsigned i=1;i<=18;i++){snprintf(file,sizeof(file),"%s/p%032x.rgb",dir,i);CHECK((access(file,F_OK)==0)==(i<=2));}
 snprintf(file,sizeof(file),"%s/unknown.rgb",dir);CHECK(!access(file,F_OK));
 /* A damaged fallback must fail closed, even with a valid newest checkpoint. */
 snprintf(file,sizeof(file),"%s/config0.dat",dir);f=fopen(file,"wb");CHECK(f);fclose(f);
 CHECK(!dsr_config_store_collect_photos(dir,&removed,error));CHECK(removed==0);
 snprintf(file,sizeof(file),"%s/p%032x.rgb",dir,1);CHECK(!access(file,F_OK));
 /* Two subsequent durable revisions retire both old references. Repeated
  * replacement/reboot cycles must not consume the finite cache forever. */
 cJSON_ReplaceItemInObjectCaseSensitive(room,"photo",cJSON_CreateString(""));
 for(unsigned i=3;i<=4;i++){cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(doc,"revision"),i);char *json=cJSON_PrintUnformatted(doc);CHECK(dsr_config_store_save(dir,json,error));free(json);}
 CHECK(dsr_config_store_collect_photos(dir,&removed,error));CHECK(removed==2);
 for(unsigned i=20;i<52;i++){snprintf(file,sizeof(file),"%s/p%032x.rgb",dir,i);f=fopen(file,"wb");CHECK(f);fclose(f);CHECK(dsr_config_store_collect_photos(dir,&removed,error));CHECK(removed==1);CHECK(access(file,F_OK)!=0);}
 cJSON_Delete(doc);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);char error[96]={0};char *initial=dsr_config_default();CHECK(initial);cJSON *r=cJSON_Parse(initial);CHECK(dsr_config_validate(r,error));CHECK(dsr_config_revision(initial)==1);
 invalid(r,"schema",cJSON_CreateNumber(2));invalid(r,"revision",cJSON_CreateNumber(0));invalid(r,"revision",cJSON_CreateNumber(1.5));invalid(r,"language",cJSON_CreateString("xx"));invalid(r,"accent",cJSON_CreateNumber(0x1000000));invalid(r,"active_dashboard",cJSON_CreateNumber(99));
 invalid(r,"language",cJSON_CreateString("pl||en"));
 cJSON_AddStringToObject(r,"password","must-not-be-stored");CHECK(!dsr_config_validate(r,error));cJSON_DeleteItemFromObjectCaseSensitive(r,"password");
 cJSON *rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms"),*devices=cJSON_GetObjectItemCaseSensitive(r,"devices"),*dash=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(r,"dashboards"),0),*tiles=cJSON_GetObjectItemCaseSensitive(dash,"tiles");
 cJSON *room=cJSON_Parse("{\"id\":1,\"name\":\"Salon\",\"icon\":0,\"photo\":\"\"}");cJSON_AddItemToArray(rooms,room);
 cJSON *device=cJSON_Parse("{\"id\":1,\"name\":\"Światło\",\"host\":\"192.168.1.77\",\"channel\":0,\"kind\":\"light\",\"room\":1}");cJSON_AddItemToArray(devices,device);
 cJSON *tile=cJSON_Parse("{\"id\":1,\"type\":\"device\",\"ref\":1,\"span\":2,\"height\":1,\"fields\":3}");cJSON_AddItemToArray(tiles,tile);CHECK(dsr_config_validate(r,error));
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(tile,"span"),1);CHECK(!dsr_config_validate(r,error));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(tile,"span"),2);
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(device,"room"),2);CHECK(!dsr_config_validate(r,error));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(device,"room"),1);
 cJSON_AddItemToArray(devices,cJSON_Duplicate(device,true));CHECK(!dsr_config_validate(r,error));cJSON_DeleteItemFromArray(devices,1);
 cJSON_ReplaceItemInObjectCaseSensitive(room,"photo",cJSON_CreateString("../config0.dat"));CHECK(!dsr_config_validate(r,error));cJSON_ReplaceItemInObjectCaseSensitive(room,"photo",cJSON_CreateString(""));
 CHECK(dsr_config_validate(r,error));char *candidate=cJSON_PrintUnformatted(r);char *next=dsr_config_replace(initial,candidate,1,error);CHECK(next);CHECK(dsr_config_revision(next)==2);
 CHECK(!dsr_config_replace(next,candidate,1,error));CHECK(strstr(error,"conflict"));CHECK(!dsr_config_replace(next,candidate,2,error));
 CHECK(!dsr_config_replace(initial,"{\"schema\":1,\"schema\":1}",1,error));CHECK(!dsr_config_json_depth("[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]"));CHECK(!dsr_config_replace(initial,"{} trailing",1,error));
 CHECK(dsr_config_crc32("123456789",9)==0xcbf43926);
 bool found=false;char *loaded=dsr_config_store_load(argv[1],&found,error);CHECK(!loaded&&!found&&!error[0]);
 CHECK(dsr_config_store_save(argv[1],initial,error));loaded=dsr_config_store_load(argv[1],&found,error);CHECK(found&&!strcmp(loaded,initial));free(loaded);
 CHECK(dsr_config_store_save(argv[1],next,error));loaded=dsr_config_store_load(argv[1],&found,error);CHECK(found&&!strcmp(loaded,next));free(loaded);
 char path[512];snprintf(path,sizeof(path),"%s/config1.dat",argv[1]);FILE *f=fopen(path,"wb");CHECK(f);CHECK(fwrite("power-cut",1,9,f)==9);CHECK(!fclose(f));
 loaded=dsr_config_store_load(argv[1],&found,error);CHECK(found&&!strcmp(loaded,initial));free(loaded);
 CHECK(dsr_config_store_save(argv[1],next,error));loaded=dsr_config_store_load(argv[1],&found,error);CHECK(found&&dsr_config_revision(loaded)==2);free(loaded);
 snprintf(path,sizeof(path),"%s/config0.dat",argv[1]);f=fopen(path,"wb");CHECK(f);fclose(f);snprintf(path,sizeof(path),"%s/config1.dat",argv[1]);f=fopen(path,"wb");CHECK(f);fclose(f);
 loaded=dsr_config_store_load(argv[1],&found,error);CHECK(!loaded&&!found&&error[0]);
 CHECK(dsr_config_store_save(argv[1],initial,error));
 photo_gc(r,argv[1]);
 cJSON_ReplaceItemInObjectCaseSensitive(r,"schema",cJSON_CreateNumber(2));char *future=cJSON_PrintUnformatted(r);uint32_t header[]={0x44535231,(uint32_t)strlen(future),dsr_config_crc32(future,strlen(future)),1};
 snprintf(path,sizeof(path),"%s/config1.dat",argv[1]);f=fopen(path,"wb");CHECK(f);CHECK(fwrite(header,sizeof(header),1,f)==1);CHECK(fwrite(future,1,strlen(future),f)==strlen(future));CHECK(!fclose(f));free(future);
 loaded=dsr_config_store_load(argv[1],&found,error);CHECK(!loaded&&!found&&strstr(error,"Newer"));CHECK(!dsr_config_store_save(argv[1],initial,error));
 unsigned removed=99;CHECK(!dsr_config_store_collect_photos(argv[1],&removed,error));CHECK(removed==0);
 cJSON_Delete(r);free(initial);free(next);free(candidate);printf("PASS: %u configuration checks; CAS, schema, references, partial-write recovery and dual corruption preservation\n",checks);return 0;
}
