#include "dsr_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}} while(0)
static void invalid(cJSON *r,const char *key,cJSON *value)
{char error[96];cJSON *copy=cJSON_Duplicate(r,true);cJSON_ReplaceItemInObjectCaseSensitive(copy,key,value);CHECK(!dsr_config_validate(copy,error));cJSON_Delete(copy);}
int main(int argc,char **argv)
{
 CHECK(argc==2);char error[96]={0};char *initial=dsr_config_default();CHECK(initial);cJSON *r=cJSON_Parse(initial);CHECK(dsr_config_validate(r,error));CHECK(dsr_config_revision(initial)==1);
 invalid(r,"schema",cJSON_CreateNumber(2));invalid(r,"revision",cJSON_CreateNumber(0));invalid(r,"revision",cJSON_CreateNumber(1.5));invalid(r,"language",cJSON_CreateString("xx"));invalid(r,"accent",cJSON_CreateNumber(0x1000000));invalid(r,"active_dashboard",cJSON_CreateNumber(99));
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
 cJSON_Delete(r);free(initial);free(next);free(candidate);printf("PASS: %u configuration checks; CAS, schema, references, partial-write recovery and dual corruption preservation\n",checks);return 0;
}
