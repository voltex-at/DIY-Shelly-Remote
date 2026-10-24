#include "dsr_backup_bonds.h"
#include "dsr_backup_bonds_sdk.h"
#include "dsr_backup_format.h"
#include "dsr_backup_json.h"
#include "dsr_maintenance_private.h"
#include "nvs.h"
#include <string.h>

static dsr_result_t capture(cJSON *records)
{
 nvs_iterator_t it=NULL;esp_err_t e=nvs_entry_find("nvs","nimble_bond",NVS_TYPE_ANY,&it);
 if(e==ESP_ERR_NVS_NOT_FOUND)return DSR_OK;
 if(e!=ESP_OK)return DSR_IO;
 nvs_handle_t h;e=nvs_open("nimble_bond",NVS_READONLY,&h);if(e!=ESP_OK){nvs_release_iterator(it);return DSR_IO;}
 dsr_result_t result=DSR_OK;unsigned count=0;uint8_t raw[128]={0};dsr_backup_bond_t bond={0};
 while(e==ESP_OK){
  nvs_entry_info_t info;nvs_entry_info(it,&info);size_t size=0;
  if(++count>104||info.type!=NVS_TYPE_BLOB){result=DSR_UNSUPPORTED;break;}
  e=nvs_get_blob(h,info.key,NULL,&size);if(e!=ESP_OK){result=DSR_IO;break;}
  if(size>sizeof(raw)){result=DSR_UNSUPPORTED;break;}
  e=nvs_get_blob(h,info.key,raw,&size);if(e!=ESP_OK){result=DSR_IO;break;}
  result=dsr_backup_bond_from_sdk(info.key,raw,size,&bond);dsr_backup_wipe(raw,sizeof(raw));
  if(result!=DSR_OK)break;
  cJSON *row=NULL;result=dsr_backup_bond_encode(&bond,&row);dsr_backup_wipe(&bond,sizeof(bond));if(result!=DSR_OK)break;
  if(!cJSON_AddItemToArray(records,row)){dsr_backup_json_discard(row);result=DSR_FULL;break;}
  e=nvs_entry_next(&it);
 }
 if(result==DSR_OK&&e!=ESP_ERR_NVS_NOT_FOUND)result=DSR_IO;
 nvs_release_iterator(it);nvs_close(h);dsr_backup_wipe(raw,sizeof(raw));dsr_backup_wipe(&bond,sizeof(bond));return result;
}
dsr_result_t dsr_backup_bonds_capture(cJSON *manifest)
{
 if(!dsr_maintenance_bonds_snapshot_allowed())return DSR_BUSY;
 if(!cJSON_IsObject(manifest))return DSR_INVALID;
 cJSON *bonds=cJSON_CreateObject();if(!bonds)return DSR_FULL;
 cJSON *records=cJSON_AddArrayToObject(bonds,"records");dsr_result_t r=DSR_FULL;
 if(records&&cJSON_AddNumberToObject(bonds,"version",1))r=capture(records);
 if(r==DSR_OK)r=dsr_backup_bonds_validate(bonds);
 if(r==DSR_OK&&!dsr_maintenance_bonds_snapshot_allowed())r=DSR_BUSY;
 if(r==DSR_OK){if(dsr_backup_json_replace(manifest,"bonds",bonds))return DSR_OK;return DSR_FULL;}
 dsr_backup_json_discard(bonds);return r;
}
dsr_result_t dsr_backup_bonds_preflight(const cJSON *manifest)
{
 const cJSON *bonds=cJSON_GetObjectItemCaseSensitive(manifest,"bonds");dsr_result_t r=dsr_backup_bonds_validate(bonds);if(r!=DSR_OK)return r;
 const cJSON *records=cJSON_GetObjectItemCaseSensitive(bonds,"records"),*row;uint8_t raw[128]={0};char key[16];size_t size=0;
 dsr_backup_bond_t b={0};
 cJSON_ArrayForEach(row,records){r=dsr_backup_bond_decode(row,&b);if(r==DSR_OK)r=dsr_backup_bond_to_sdk(&b,key,raw,sizeof(raw),&size);if(r!=DSR_OK)break;}
 dsr_backup_wipe(raw,sizeof(raw));dsr_backup_wipe(&b,sizeof(b));return r;
}
dsr_result_t dsr_backup_bonds_apply(const cJSON *manifest)
{
 if(!dsr_maintenance_boot_writes_allowed())return DSR_BUSY;
 dsr_result_t r=dsr_backup_bonds_preflight(manifest);if(r!=DSR_OK)return r;
 const cJSON *records=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(manifest,"bonds"),"records"),*row;
 uint8_t raw[128]={0},check[128]={0};char key[16];size_t size=0;dsr_backup_bond_t b={0};
 nvs_handle_t h;esp_err_t e=nvs_open("nimble_bond",NVS_READWRITE,&h);if(e!=ESP_OK){r=DSR_IO;goto done;}
 e=nvs_erase_all(h);if(e==ESP_OK)e=nvs_commit(h);
 cJSON_ArrayForEach(row,records){
  if(e!=ESP_OK)break;
  r=dsr_backup_bond_decode(row,&b);if(r!=DSR_OK)break;
  r=dsr_backup_bond_to_sdk(&b,key,raw,sizeof(raw),&size);if(r!=DSR_OK)break;
  e=nvs_set_blob(h,key,raw,size);
 }
 if(e==ESP_OK&&r==DSR_OK)e=nvs_commit(h);
 cJSON_ArrayForEach(row,records){
  if(e!=ESP_OK||r!=DSR_OK)break;
  r=dsr_backup_bond_decode(row,&b);if(r==DSR_OK)r=dsr_backup_bond_to_sdk(&b,key,raw,sizeof(raw),&size);if(r!=DSR_OK)break;
  size_t read_size=sizeof(check);e=nvs_get_blob(h,key,check,&read_size);
  if(e==ESP_OK&&(read_size!=size||memcmp(raw,check,size)))e=ESP_FAIL;
 }
 nvs_close(h);if(e!=ESP_OK)r=DSR_IO;
done:
 dsr_backup_wipe(raw,sizeof(raw));dsr_backup_wipe(check,sizeof(check));dsr_backup_wipe(&b,sizeof(b));return r;
}
