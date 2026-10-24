#include "dsr_backup_sources.h"
#include "dsr_backup_sources_model.h"
#include "dsr_backup_storage.h"
#include "dsr_backup_bonds_model.h"
#include "dsr_backup_json.h"
#include "dsr_maintenance_private.h"
#include "dsr_automation_store.h"
#include "dsr_automation_model.h"
#include "dsr_automation_config.h"
#include "dsr_workspace.h"
#include "dsr_config.h"
#include "dsr_style_hint.h"
#include "storage.h"
#include "mbedtls/sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;static bool lease_allowed,quiet=true,lose_lease;
static const uint8_t device[32]={1};static cJSON *public_config;
static uint8_t *pixels[16];static char ids[16][33];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"source-io:%d %s\n",__LINE__,#x);exit(1);}}while(0)
dsr_result_t dsr_backup_device_id(uint8_t out[32]){memcpy(out,device,32);return DSR_OK;}
bool dsr_maintenance_lease_valid(const dsr_maint_lease_t *lease){return lease_allowed&&lease&&lease->reservation==1&&lease->generation==7;}
dsr_snapshot_result_t dsr_workspace_snapshot_checked(char **out){*out=cJSON_PrintUnformatted(public_config);return *out?DSR_SNAPSHOT_OK:DSR_SNAPSHOT_NO_MEMORY;}
const void *dsr_photo_pixels(const char *id){for(unsigned i=0;i<16;i++)if(!strcmp(ids[i],id))return pixels[i];return NULL;}
dsr_result_t dsr_maintenance_bonds_capture(const dsr_maint_lease_t *lease,cJSON *root)
{CHECK(dsr_maintenance_lease_valid(lease));if(lose_lease)lease_allowed=false;return dsr_backup_json_replace(root,"bonds",cJSON_Parse("{\"version\":1,\"records\":[]}"))?DSR_OK:DSR_FULL;}
dsr_result_t dsr_automation_backup_record(dsr_auto_record_t *out)
{if(!quiet)return DSR_BUSY;if(!out)return DSR_OK;bool found=false;CHECK(dsr_auto_store_load(out,&found)==ESP_OK);if(!found){memset(out,0,sizeof(*out));out->generation=1;}return DSR_OK;}
static cJSON *read_fixture(const char *path)
{char data[2048];FILE *f=fopen(path,"rb");CHECK(f);size_t n=fread(data,1,sizeof(data)-1,f);CHECK(feof(f)&&!fclose(f));data[n]=0;cJSON *r=cJSON_Parse(data);CHECK(r);return r;}
int main(int argc,char **argv)
{
 CHECK(argc==2);cJSON *fixture=read_fixture(argv[1]);public_config=cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(fixture,"config"),true);CHECK(public_config);cJSON_Delete(fixture);
 dsr_maint_lease_t lease={1,7};dsr_backup_image_t *image=NULL;unsigned before=test_nvs_calls;
 CHECK(dsr_backup_sources_capture(&lease,&image)==DSR_BUSY&&!image&&test_nvs_calls==before);
 dsr_settings_t settings={.ssid="fixture-ssid",.password="synthetic-secret",.brightness=81,.wifi_security=DSR_WIFI_WPA2,.idle_seconds=123};settings.ssid[0]=(char)0xff;
 test_boot_allowed=true;CHECK(dsr_settings_restore(&settings)==ESP_OK);dsr_battery_profile_t battery;dsr_battery_profile_defaults(&battery);battery.capacity_mah=1500;battery.gain_permille=1020;
 CHECK(dsr_battery_profile_restore(&battery)==ESP_OK);CHECK(dsr_style_hint_restore(3)==ESP_OK);
 CHECK(cJSON_ReplaceItemInObjectCaseSensitive(public_config,"theme",cJSON_CreateString("cyber_blue")));
 dsr_connection_t *bindings=calloc(16,sizeof(*bindings));CHECK(bindings);
 for(unsigned i=0;i<16;i++){
  dsr_connection_t *b=&bindings[i];b->layout_id=i+1;b->device.generation=DSR_GEN2_PLUS;b->device.channel=(uint8_t)i;
  strcpy(b->device.host,"192.0.2.4");snprintf(b->device.name,48,"Fixture %u",i);snprintf(b->device.device_id,64,"synthetic-%u",i);snprintf(b->credentials.username,33,"user%u",i);snprintf(b->credentials.password,65,"synthetic-password-%u",i);
  cJSON *d=cJSON_CreateObject();CHECK(d);CHECK(cJSON_AddNumberToObject(d,"id",i+1)&&cJSON_AddStringToObject(d,"name",b->device.name)&&cJSON_AddStringToObject(d,"host",b->device.host)&&cJSON_AddNumberToObject(d,"channel",i)&&cJSON_AddNumberToObject(d,"room",0)&&cJSON_AddStringToObject(d,"kind","relay")&&cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(public_config,"devices"),d));
  pixels[i]=malloc(DSR_BACKUP_PHOTO_BYTES);CHECK(pixels[i]);memset(pixels[i],(int)i,DSR_BACKUP_PHOTO_BYTES);uint8_t sha[32];CHECK(!mbedtls_sha256(pixels[i],DSR_BACKUP_PHOTO_BYTES,sha,0));for(unsigned j=0;j<16;j++)snprintf(ids[i]+j*2,3,"%02x",sha[j]);
  cJSON *p=cJSON_CreateObject();CHECK(p&&cJSON_AddNumberToObject(p,"id",i%8+1)&&cJSON_AddStringToObject(p,"name","Photo")&&cJSON_AddStringToObject(p,"photo",ids[i]));
  if(i<8)CHECK(cJSON_AddNumberToObject(p,"icon",0));else CHECK(cJSON_AddArrayToObject(p,"actions"));
  CHECK(cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(public_config,i<8?"rooms":"scenes"),p));
 }
 char config_error[96];if(!dsr_config_validate(public_config,config_error)){fprintf(stderr,"fixture validation: %s\n",config_error);return 1;}
 CHECK(dsr_connections_restore(bindings,16)==ESP_OK);test_boot_allowed=false;lease_allowed=true;quiet=false;
 CHECK(dsr_backup_sources_capture(&lease,&image)==DSR_BUSY&&!image);quiet=true;
 unsigned writes=test_nvs_writes;CHECK(dsr_backup_sources_capture(&lease,&image)==DSR_OK&&image&&test_nvs_writes==writes);
 const cJSON *manifest=dsr_backup_image_manifest(image);CHECK(!cJSON_GetObjectItemCaseSensitive(manifest,"tls")&&!cJSON_GetObjectItemCaseSensitive(manifest,"panel_token"));
 CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(manifest,"connections"))==16&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(manifest,"photos"))==16);
 for(unsigned i=0;i<16;i++){const dsr_backup_photo_t *p=dsr_backup_image_photo(image,i);CHECK(p&&!strcmp(p->id,ids[i])&&!memcmp(p->pixels,pixels[i],DSR_BACKUP_PHOTO_BYTES));}
 CHECK(dsr_backup_sources_apply(manifest)==DSR_BUSY&&test_nvs_writes==writes);
 test_nvs_reset();test_boot_allowed=true;CHECK(dsr_backup_sources_apply(manifest)==DSR_OK);
 dsr_settings_t restored;CHECK(dsr_settings_load(&restored)==ESP_OK&&!strcmp(restored.ssid,settings.ssid)&&!strcmp(restored.password,settings.password)&&restored.brightness==81&&restored.idle_seconds==123);
 dsr_battery_profile_t check;CHECK(dsr_battery_profile_load(&check)==ESP_OK&&dsr_battery_profile_equal(&check,&battery));CHECK(dsr_style_hint_get()==3);
 dsr_connection_t *restored_bindings=calloc(16,sizeof(*restored_bindings));CHECK(restored_bindings);size_t count=0;CHECK(dsr_connections_load(restored_bindings,16,&count)==ESP_OK&&count==16);
 for(unsigned i=0;i<16;i++)CHECK(!memcmp(bindings+i,restored_bindings+i,sizeof(*bindings)));
 dsr_auto_record_t record;bool found=false;CHECK(dsr_auto_store_load(&record,&found)==ESP_OK&&found&&record.generation==1&&record.rules.count==0);
 cJSON *bad=cJSON_Duplicate(manifest,true);CHECK(bad);cJSON *conn=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(bad,"connections"),0);CHECK(cJSON_ReplaceItemInObjectCaseSensitive(conn,"channel",cJSON_CreateNumber(15)));writes=test_nvs_writes;CHECK(dsr_backup_sources_apply(bad)==DSR_INVALID&&test_nvs_writes==writes);dsr_backup_json_discard(bad);
 dsr_backup_image_free(image);image=NULL;
 /* Enabled enrollment survives, but operational history never does. */
 cJSON *first=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(public_config,"devices"),0),*second=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(public_config,"devices"),1);
 CHECK(cJSON_ReplaceItemInObjectCaseSensitive(first,"kind",cJSON_CreateString("contact"))&&cJSON_ReplaceItemInObjectCaseSensitive(second,"kind",cJSON_CreateString("sensor")));
 CHECK(cJSON_ReplaceItemInObjectCaseSensitive(public_config,"automations",cJSON_Parse("[{\"id\":1,\"name\":\"Heating\",\"kind\":\"contact_heating\",\"enabled\":true,\"sensors\":[1],\"targets\":[2],\"target_half\":16}]")));
 bindings[0].device.endpoint_kind=bindings[1].device.endpoint_kind=DSR_ENDPOINT_BLU;CHECK(dsr_connections_restore(bindings,16)==ESP_OK);
 dsr_auto_rules_t rules;CHECK(dsr_auto_rules_parse(public_config,&rules,config_error));
 for(unsigned i=0;i<2;i++){dsr_auto_key_t *k=&rules.keys[i];const dsr_connection_t *b=&bindings[k->layout_id-1];strcpy(k->device_id,b->device.device_id);strcpy(k->host,b->device.host);k->generation=2;k->channel=b->device.channel;}
 rules.enroll=true;dsr_auto_model_t *model=calloc(1,sizeof(*model));CHECK(model);dsr_auto_init(model);CHECK(dsr_auto_configure(model,&rules)==DSR_OK);CHECK(dsr_auto_store_restore(&model->record)==ESP_OK);
 test_boot_allowed=false;CHECK(dsr_backup_sources_capture(&lease,&image)==DSR_OK&&image);
 test_boot_allowed=true;CHECK(dsr_backup_sources_apply(dsr_backup_image_manifest(image))==DSR_OK);CHECK(dsr_auto_store_load(&record,&found)==ESP_OK&&found&&record.generation==1&&record.rules.items[0].enabled);
 dsr_auto_init(model);CHECK(dsr_auto_recover(model,&record));rules.enroll=false;CHECK(dsr_auto_configure(model,&rules)==DSR_OK);CHECK(!model->ready&&model->ready_rules==0);
 dsr_backup_image_free(image);image=NULL;record.cycles[0].cycle=1;record.cycles[0].active=DSR_AUTO_CYCLE_OPEN;record.generation++;
 CHECK(dsr_auto_store_save(&record)==ESP_OK);test_boot_allowed=false;CHECK(dsr_backup_sources_capture(&lease,&image)==DSR_BUSY&&!image);
 record.cycles[0]=(dsr_auto_cycle_t){0};record.generation++;strcpy(record.rules.keys[0].device_id,"changed-identity");CHECK(dsr_auto_store_save(&record)==ESP_OK);
 CHECK(dsr_backup_sources_capture(&lease,&image)==DSR_IDENTITY_MISMATCH&&!image);free(model);
 /* Cancellation after a source copy cannot return a mixed-generation image. */
 CHECK(dsr_backup_sources_quiet());test_boot_allowed=true;record.rules.count=record.rules.key_count=0;memset(record.rules.items,0,sizeof(record.rules.items));memset(record.rules.keys,0,sizeof(record.rules.keys));record.generation++;
 CHECK(dsr_auto_store_save(&record)==ESP_OK);CHECK(cJSON_ReplaceItemInObjectCaseSensitive(public_config,"automations",cJSON_CreateArray()));
 test_boot_allowed=false;lose_lease=true;CHECK(dsr_backup_sources_capture(&lease,&image)==DSR_BUSY&&!image);
 for(unsigned i=0;i<16;i++)free(pixels[i]);
 free(bindings);free(restored_bindings);cJSON_Delete(public_config);test_nvs_reset();
 printf("PASS: %u composed source checks; real store codecs, modeled NVS/photos/owners, zero radio commands\n",checks);return 0;
}
