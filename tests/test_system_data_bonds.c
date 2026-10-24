#include "dsr_backup_bonds_sdk.h"
#include "dsr_backup_bonds.h"
#include "dsr_maintenance_private.h"
#include "nvs.h"
#include "host/ble_store.h"
#include "ble_hs_resolv_priv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"sdk-bonds:%d %s\n",__LINE__,#x);exit(1);}}while(0)
static struct {char key[16];uint8_t bytes[128];size_t size;} store[104];
static unsigned count,opens,writes;static bool snapshot_allowed,boot_allowed;
static struct nvs_iterator {unsigned at;} iterator;
bool dsr_maintenance_bonds_snapshot_allowed(void){return snapshot_allowed;}
bool dsr_maintenance_boot_writes_allowed(void){return boot_allowed;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *out){(void)mode;CHECK(!strcmp(name,"nimble_bond"));opens++;*out=1;return ESP_OK;}
void nvs_close(nvs_handle_t h){CHECK(h==1);}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *n){CHECK(h==1);for(unsigned i=0;i<count;i++)if(!strcmp(key,store[i].key)){if(!out){*n=store[i].size;return ESP_OK;}if(*n<store[i].size)return ESP_ERR_INVALID_SIZE;*n=store[i].size;memcpy(out,store[i].bytes,*n);return ESP_OK;}return ESP_ERR_NVS_NOT_FOUND;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t n){CHECK(h==1&&n<=128&&strlen(key)<16);unsigned i;for(i=0;i<count;i++)if(!strcmp(key,store[i].key))break;CHECK(i<104);if(i==count)count++;strcpy(store[i].key,key);memcpy(store[i].bytes,data,n);store[i].size=n;writes++;return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){CHECK(h==1);return ESP_OK;}
esp_err_t nvs_erase_all(nvs_handle_t h){CHECK(h==1);memset(store,0,sizeof(store));count=0;writes++;return ESP_OK;}
esp_err_t nvs_entry_find(const char *partition,const char *ns,nvs_type_t type,nvs_iterator_t *out){CHECK(!strcmp(partition,"nvs")&&!strcmp(ns,"nimble_bond")&&type==NVS_TYPE_ANY);*out=count?&iterator:NULL;iterator.at=0;return count?ESP_OK:ESP_ERR_NVS_NOT_FOUND;}
esp_err_t nvs_entry_next(nvs_iterator_t *it){if(++(*it)->at>=count){*it=NULL;return ESP_ERR_NVS_NOT_FOUND;}return ESP_OK;}
void nvs_entry_info(nvs_iterator_t it,nvs_entry_info_t *out){memset(out,0,sizeof(*out));strcpy(out->namespace_name,"nimble_bond");strcpy(out->key,store[it->at].key);out->type=NVS_TYPE_BLOB;}
void nvs_release_iterator(nvs_iterator_t it){(void)it;}
int main(void)
{
 /* Poison padding: archive must depend on fields only. */
 struct ble_store_value_sec sdk;memset(&sdk,0xa5,sizeof(sdk));sdk.peer_addr=(ble_addr_t){1,{1,2,3,4,5,6}};
 sdk.bond_count=5;sdk.key_size=16;sdk.ediv=0x1234;sdk.rand_num=UINT64_C(0xfedcba9876543210);
 memset(sdk.ltk,0,16);memset(sdk.irk,0x19,16);memset(sdk.csrk,0x27,16);
 sdk.ltk_present=1;sdk.irk_present=1;sdk.csrk_present=0;sdk.sign_counter=0x98563210;sdk.authenticated=1;sdk.sc=1;
 dsr_backup_bond_t b;CHECK(dsr_backup_bond_from_sdk("our_sec_16",&sdk,sizeof(sdk),&b)==DSR_OK);
 CHECK(b.family==DSR_BACKUP_BOND_OUR_SEC&&b.index==16&&b.value.sec.flags==27&&b.value.sec.bond_count==5&&b.value.sec.rand_num==sdk.rand_num);
 union {struct ble_store_value_sec sec;struct ble_store_value_cccd cccd;struct ble_hs_dev_records peer;uint8_t bytes[128];} raw;
 char key[16];size_t size=0;memset(&raw,0xcc,sizeof(raw));CHECK(dsr_backup_bond_to_sdk(&b,key,&raw,sizeof(raw),&size)==DSR_OK);
 CHECK(!strcmp(key,"our_sec_16")&&size==sizeof(sdk));CHECK(raw.sec.ltk_present&&raw.sec.irk_present&&!raw.sec.csrk_present&&raw.sec.authenticated&&raw.sec.sc);
 CHECK(raw.sec.rand_num==sdk.rand_num&&!memcmp(raw.sec.csrk,sdk.csrk,16));
 CHECK(dsr_backup_bond_from_sdk("our_sec_1",&sdk,sizeof(sdk)-1,&b)==DSR_UNSUPPORTED);
 const char *invalid[]={"our_sec_0","our_sec_01","our_sec_17","cccd_sec_9","our_sec_1x","ead_sec_1","unknown_1","../our_sec_1"};
 for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++)CHECK(dsr_backup_bond_from_sdk(invalid[i],&sdk,sizeof(sdk),&b)==DSR_UNSUPPORTED);
 struct ble_hs_dev_records peer;memset(&peer,0,sizeof(peer));peer.rec_used=true;peer.rand_addr_type=1;
 peer.peer_sec.peer_addr=(ble_addr_t){1,{7,8,9,10,11,12}};peer.peer_sec.irk_present=1;peer.peer_sec.irk[15]=0x93;peer.identity_addr[2]=0x62;
 CHECK(dsr_backup_bond_from_sdk("p_dev_rec_3",&peer,sizeof(peer),&b)==DSR_OK);CHECK(b.value.peer_record.irk[15]==0x93&&b.value.peer_record.identity[2]==0x62);
 CHECK(dsr_backup_bond_to_sdk(&b,key,&raw,sizeof(raw),&size)==DSR_OK);CHECK(raw.peer.rec_used&&raw.peer.peer_sec.irk_present&&raw.peer.peer_sec.irk[15]==0x93);
 const char *keys[]={"peer_sec_1","cccd_sec_2","csfc_sec_3","local_irk_4","rpa_rec_5"};
 const size_t sizes[]={sizeof(struct ble_store_value_sec),sizeof(struct ble_store_value_cccd),sizeof(struct ble_store_value_csfc),sizeof(struct ble_store_value_local_irk),sizeof(struct ble_store_value_rpa_rec)};
 for(unsigned i=0;i<5;i++){
  memset(&raw,0,sizeof(raw));if(i==1)raw.cccd.chr_val_handle=1;
  CHECK(dsr_backup_bond_from_sdk(keys[i],&raw,sizes[i],&b)==DSR_OK);CHECK(dsr_backup_bond_to_sdk(&b,key,&raw,sizeof(raw),&size)==DSR_OK);CHECK(!strcmp(key,keys[i])&&size==sizes[i]);
 }
 cJSON *manifest=cJSON_CreateObject();CHECK(manifest);
 CHECK(dsr_backup_bonds_capture(manifest)==DSR_BUSY&&opens==0);snapshot_allowed=true;
 for(unsigned i=1;i<=16;i++){snprintf(key,sizeof(key),"our_sec_%u",i);CHECK(nvs_set_blob(1,key,&sdk,sizeof(sdk))==ESP_OK);}
 unsigned before=writes;CHECK(dsr_backup_bonds_capture(manifest)==DSR_OK&&writes==before);
 CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(manifest,"bonds"),"records"))==16);
 CHECK(dsr_backup_bonds_apply(manifest)==DSR_BUSY&&writes==before);boot_allowed=true;
 CHECK(dsr_backup_bonds_apply(manifest)==DSR_OK&&count==16);
 before=writes;CHECK(nvs_set_blob(1,"unknown_1",&sdk,sizeof(sdk))==ESP_OK);
 CHECK(dsr_backup_bonds_capture(manifest)==DSR_UNSUPPORTED&&writes==before+1);
 /* Unknown snapshot must not replace the last valid one or erase any store. */
 CHECK(dsr_backup_bonds_apply(manifest)==DSR_OK&&count==16);
 cJSON *records=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(manifest,"bonds"),"records");
 CHECK(cJSON_AddItemToArray(records,cJSON_Parse("{\"family\":\"ead_sec\",\"index\":1,\"data\":\"00\"}")));
 before=writes;CHECK(dsr_backup_bonds_apply(manifest)==DSR_UNSUPPORTED&&writes==before);
 cJSON_Delete(manifest);
 printf("PASS: %u SDK 5.5.4 typed/store checks; actual SDK headers, modeled NVS/owner\n",checks);return 0;
}
