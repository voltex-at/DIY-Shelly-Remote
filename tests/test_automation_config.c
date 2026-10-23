#include "dsr_config.h"
#include "dsr_automation_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;static char error[96];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s (%s)\n",__func__,__LINE__,#x,error);exit(1);}}while(0)
static cJSON *get(cJSON *r,const char *k){return cJSON_GetObjectItemCaseSensitive(r,k);}
static cJSON *base(void){
 char *s=dsr_config_default();CHECK(s);cJSON *r=cJSON_Parse(s);free(s);CHECK(r);
 cJSON_SetNumberValue(get(r,"revision"),7);
 cJSON_AddItemToArray(get(r,"devices"),cJSON_Parse("{\"id\":1,\"name\":\"Window\",\"host\":\"192.0.2.1\",\"channel\":1,\"kind\":\"contact\",\"room\":0}"));
 cJSON_AddItemToArray(get(r,"devices"),cJSON_Parse("{\"id\":2,\"name\":\"TRV\",\"host\":\"192.0.2.1\",\"channel\":2,\"kind\":\"sensor\",\"room\":0}"));
 cJSON_AddItemToArray(get(r,"scenes"),cJSON_Parse("{\"id\":1,\"name\":\"Warm\",\"actions\":[{\"device\":2,\"command\":\"target\",\"value\":44}]}"));return r;
}
static cJSON *rule(void){return cJSON_Parse("{\"id\":1,\"name\":\"Ventilation\",\"kind\":\"contact_heating\",\"enabled\":false,\"sensors\":[1],\"targets\":[2],\"target_half\":16}");}
int main(void){
 CHECK(DSR_CONFIG_SCHEMA==3);cJSON *r=base();CHECK(dsr_config_validate(r,error));
 cJSON_DeleteItemFromObjectCaseSensitive(r,"automations");cJSON_SetNumberValue(get(r,"schema"),2);CHECK(dsr_config_validate_stored(r,error));
 cJSON_AddArrayToObject(r,"automations");CHECK(!dsr_config_validate_stored(r,error));cJSON_DeleteItemFromObjectCaseSensitive(r,"automations");
 cJSON *legacy_action=cJSON_GetArrayItem(get(cJSON_GetArrayItem(get(r,"scenes"),0),"actions"),0);
 cJSON_SetNumberValue(get(legacy_action,"value"),7);CHECK(!dsr_config_validate_stored(r,error));cJSON_SetNumberValue(get(legacy_action,"value"),44);
 char *legacy=cJSON_PrintUnformatted(r);bool old=false;char *up=dsr_config_upgrade(legacy,&old,error);CHECK(up&&old&&dsr_config_revision(up)==7);
 cJSON *current=cJSON_Parse(up);CHECK(dsr_config_validate(current,error)&&get(current,"schema")->valueint==3);
 CHECK(cJSON_IsArray(get(current,"automations"))&&cJSON_GetArraySize(get(current,"automations"))==0);
 CHECK(get(cJSON_GetArrayItem(get(cJSON_GetArrayItem(get(current,"scenes"),0),"actions"),0),"value")->valueint==44);
 char *again=dsr_config_upgrade(up,&old,error);CHECK(again&&!old&&!strcmp(again,up));free(again);free(up);free(legacy);cJSON_Delete(current);
 /* Legacy1 remains independently strict and gets BOTH missing collections. */
 cJSON_DeleteItemFromObjectCaseSensitive(r,"groups");cJSON_SetNumberValue(get(r,"schema"),1);
 cJSON_DeleteItemFromArray(get(r,"scenes"),0);CHECK(dsr_config_validate_stored(r,error));legacy=cJSON_PrintUnformatted(r);
 up=dsr_config_upgrade(legacy,&old,error);CHECK(up&&old&&dsr_config_revision(up)==7);current=cJSON_Parse(up);
 CHECK(get(current,"groups")&&get(current,"automations")&&dsr_config_validate(current,error));free(up);free(legacy);cJSON_Delete(r);cJSON_Delete(current);
 r=base();cJSON *a=get(r,"automations"),*v=rule();cJSON_AddItemToArray(a,v);CHECK(dsr_config_validate(r,error));
 dsr_auto_rules_t rules;CHECK(dsr_auto_rules_parse(r,&rules,error)&&rules.count==1&&rules.key_count==2);
 CHECK(rules.items[0].target_half==16&&!rules.items[0].enabled&&!rules.enroll);
 CHECK(rules.keys[0].layout_id==1&&rules.keys[0].kind==DSR_AUTO_CONTACT&&rules.keys[1].kind==DSR_AUTO_TRV&&!rules.keys[1].device_id[0]);
 CHECK(!dsr_config_remove_device(r,1,error)&&strstr(error,"automation"));CHECK(!dsr_config_remove_device(r,2,error));
 const double bad[]={7,61,16.5,-1,1e20};for(unsigned i=0;i<5;i++){cJSON_SetNumberValue(get(v,"target_half"),bad[i]);CHECK(!dsr_config_validate(r,error));}cJSON_SetNumberValue(get(v,"target_half"),16);
 cJSON_DeleteItemFromObjectCaseSensitive(v,"target_half");CHECK(dsr_config_validate(r,error)&&dsr_auto_rules_parse(r,&rules,error)&&rules.items[0].target_half==16);cJSON_AddNumberToObject(v,"target_half",16);
 cJSON_AddNumberToObject(v,"unknown",1);CHECK(!dsr_config_validate(r,error));cJSON_DeleteItemFromObjectCaseSensitive(v,"unknown");
 cJSON_AddBoolToObject(v,"enabled",true);CHECK(!dsr_config_validate(r,error));cJSON_DeleteItemFromObjectCaseSensitive(v,"enabled");cJSON_ReplaceItemInObjectCaseSensitive(v,"enabled",cJSON_CreateTrue());
 cJSON *second=rule();cJSON_AddItemToArray(a,second);CHECK(!dsr_config_validate(r,error));cJSON_SetNumberValue(get(second,"id"),2);CHECK(!dsr_config_validate(r,error));cJSON_DeleteItemFromArray(a,1);
 cJSON_AddItemToArray(get(v,"sensors"),cJSON_CreateNumber(1));CHECK(!dsr_config_validate(r,error));cJSON_DeleteItemFromArray(get(v,"sensors"),1);
 cJSON_ReplaceItemInArray(get(v,"targets"),0,cJSON_CreateNumber(1));CHECK(!dsr_config_validate(r,error));cJSON_ReplaceItemInArray(get(v,"targets"),0,cJSON_CreateNumber(99));CHECK(!dsr_config_validate(r,error));
 cJSON_ReplaceItemInArray(get(v,"targets"),0,cJSON_CreateNumber(2));cJSON_DeleteItemFromArray(get(v,"sensors"),0);CHECK(!dsr_config_validate(r,error));cJSON_AddItemToArray(get(v,"sensors"),cJSON_CreateNumber(1));
 CHECK(dsr_config_validate(r,error));cJSON_DeleteItemFromArray(a,0);cJSON *deleted=dsr_config_remove_device(r,1,error);CHECK(deleted&&dsr_config_validate(deleted,error));cJSON_Delete(deleted);cJSON_Delete(r);
 char *huge=malloc(DSR_CONFIG_MAX_JSON+2);CHECK(huge);memset(huge,' ',DSR_CONFIG_MAX_JSON+1);huge[DSR_CONFIG_MAX_JSON+1]=0;CHECK(!dsr_config_upgrade(huge,NULL,error));free(huge);
 printf("automation config: %u checks passed\n",checks);return 0;
}
