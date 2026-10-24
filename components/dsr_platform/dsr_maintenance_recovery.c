#include "dsr_maintenance_recovery.h"
#include "dsr_maintenance_recovery_io.h"
#include "dsr_maintenance_private.h"
#include "dsr_maintenance_store.h"
#include "dsr_maintenance_stage.h"
#include "dsr_maintenance_io.h"
#include "dsr_backup_sources.h"
#include "dsr_backup_sources_model.h"
#include "dsr_backup_bonds.h"
#include "dsr_boot.h"
#include "dsr_maintenance.h"
#include "dsr_ota_power.h"
#include "mbedtls/sha256.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
static uint8_t *image_alloc(size_t n){return heap_caps_calloc(1,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
#else
static uint8_t *image_alloc(size_t n){return calloc(1,n);}
#endif
static atomic_bool recovering,recovery_failed;
static void put(uint8_t *p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++){p[i]=(uint8_t)v;v>>=8;}}
static int digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static bool descriptor(const cJSON *root,unsigned i,char id[33],uint8_t sha[32])
{
 const cJSON *p=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(root,"photos"),(int)i),*name=cJSON_GetObjectItemCaseSensitive(p,"id"),*s=cJSON_GetObjectItemCaseSensitive(p,"sha256");
 if(!cJSON_IsString(name)||strlen(name->valuestring)!=32||!cJSON_IsString(s)||strlen(s->valuestring)!=64)return false;
 for(unsigned j=0;j<32;j++){int a=digit(s->valuestring[2*j]),b=digit(s->valuestring[2*j+1]);if(a<0||b<0)return false;sha[j]=(uint8_t)(a*16+b);}
 memcpy(id,name->valuestring,33);return !memcmp(id,s->valuestring,32);
}
static bool digest(const uint8_t *p,size_t n,const uint8_t wanted[32])
{uint8_t sha[32];bool ok=!mbedtls_sha256(p,n,sha,0)&&!memcmp(sha,wanted,32);dsr_backup_wipe(sha,sizeof(sha));return ok;}
static dsr_result_t cleanup(const dsr_maint_record_t *r)
{
 if(r->op!=DSR_MAINT_IMPORT_COMMIT)return DSR_OK;
 for(unsigned i=0;i<=r->photos;i++){char name[32];if(!dsr_maint_record_name(r->operation_id,i,name))return DSR_INVALID;dsr_result_t result=dsr_maint_io_remove(name);if(result!=DSR_OK)return result;}return DSR_OK;
}
/* Reconstruct the exact authenticated candidate in bounded PSRAM. Applied and
 * already unlinked photo records can come only from a hash-verified final file.
 * All remaining stages must authenticate, even if a final file also exists. */
static dsr_result_t image_load(const dsr_maint_record_t *r,dsr_backup_image_t **out)
{
 *out=NULL;uint8_t *raw=image_alloc(r->candidate_bytes);if(!raw)return DSR_FULL;
 dsr_result_t result;size_t n=0;cJSON *root=NULL;dsr_backup_sources_data_t *data=NULL;
 memcpy(raw,"DSRSNP1",8);put(raw+8,1,2);put(raw+10,24,2);put(raw+12,r->metadata_bytes,4);put(raw+16,r->photos,2);put(raw+20,r->candidate_bytes,4);
 result=dsr_maint_stage_read(r,0,raw+24,r->metadata_bytes,&n);if(result!=DSR_OK)goto done;
 if(n!=r->metadata_bytes||!digest(raw+24,n,r->manifest_sha256)){result=DSR_BAD_RESPONSE;goto done;}
 root=cJSON_ParseWithLengthOpts((const char *)raw+24,n,NULL,false);if(!root){result=DSR_INVALID;goto done;}
 result=dsr_backup_manifest_validate(root,r->device);if(result!=DSR_OK)goto done;
 if(r->config_revision<=(uint32_t)cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"config"),"revision")->valuedouble){result=DSR_INVALID;goto done;}
 if((unsigned)cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(root,"photos"))!=r->photos){result=DSR_INVALID;goto done;}
 size_t at=24+r->metadata_bytes;
 for(unsigned i=0;i<r->photos;i++){
  char id[33],name[32];uint8_t sha[32];bool exists;
  if(!descriptor(root,i,id,sha)||!dsr_maint_record_name(r->operation_id,i+1,name)){result=DSR_INVALID;goto done;}
  memcpy(raw+at,sha,16);memcpy(raw+at+16,sha,32);put(raw+at+48,DSR_BACKUP_PHOTO_BYTES,4);
  result=dsr_maint_io_exists(name,&exists);if(result!=DSR_OK)goto done;
  if(exists){result=dsr_maint_stage_read(r,i+1,raw+at+56,DSR_BACKUP_PHOTO_BYTES,&n);if(result==DSR_OK&&n!=DSR_BACKUP_PHOTO_BYTES)result=DSR_INVALID;}
  else result=(r->applied_photos&(1u<<i))?dsr_maint_recovery_photo_read(id,raw+at+56):DSR_IO;
  if(result!=DSR_OK)goto done;
  if(!digest(raw+at+56,DSR_BACKUP_PHOTO_BYTES,sha)){result=DSR_BAD_RESPONSE;goto done;}at+=56+DSR_BACKUP_PHOTO_BYTES;
 }
 if(at!=r->candidate_bytes||!digest(raw,r->candidate_bytes,r->candidate_sha256)){result=DSR_BAD_RESPONSE;goto done;}
 /* Ownership is unconditional, including decoder failure. */
 result=dsr_backup_image_open(raw,r->candidate_bytes,r->device,out);raw=NULL;if(result!=DSR_OK)goto done;
 data=dsr_maint_io_alloc(sizeof(*data));if(!data){result=DSR_FULL;goto done;}
 result=dsr_backup_sources_decode(dsr_backup_image_manifest(*out),r->device,data);
 if(result==DSR_OK)result=dsr_backup_bonds_preflight(dsr_backup_image_manifest(*out));
 if(result==DSR_OK&&(data->count!=r->connections||data->enrollment.rules.count!=r->rules||
  (unsigned)cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"bonds"),"records"))!=r->bonds))result=DSR_INVALID;
done:
 dsr_backup_json_discard(root);dsr_maint_io_free(data,data?sizeof(*data):0);if(raw){dsr_backup_wipe(raw,r->candidate_bytes);free(raw);}
 if(result!=DSR_OK){dsr_backup_image_free(*out);*out=NULL;}return result;
}
static void report(const dsr_maint_record_t *r,dsr_maint_progress_fn fn,dsr_result_t result,bool complete)
{
 if(!fn)return;
 unsigned total=r->op==DSR_MAINT_RESET?10:r->photos+5,done=0;
 for(unsigned i=0;i<16;i++)if(r->applied_photos&(1u<<i))done++;
 for(unsigned i=0;i<10;i++)if(r->applied_parts&(1u<<i))done++;
 dsr_maint_status_t s={.op=r->op,.phase=complete?DSR_MAINT_COMPLETE:result==DSR_OK?DSR_MAINT_APPLYING:DSR_MAINT_FAULT,.result=result,.progress=(uint8_t)(done*100/total),.photos=r->photos,.connections=r->connections,.rules=r->rules};fn(&s);
}
static dsr_result_t part_done(dsr_maint_record_t *r,unsigned bit)
{if(r->applied_parts&(1u<<bit))return DSR_OK;r->applied_parts|=(uint16_t)(1u<<bit);return dsr_maint_store_write(r);}
static dsr_result_t import_apply(dsr_maint_record_t *r,dsr_maint_progress_fn progress)
{
 dsr_backup_image_t *image=NULL;dsr_result_t result=image_load(r,&image);if(result!=DSR_OK)return result;
 uint8_t *verify=dsr_maint_io_alloc(DSR_BACKUP_PHOTO_BYTES);if(!verify){dsr_backup_image_free(image);return DSR_FULL;}
 if(r->phase!=DSR_MAINT_APPLY){r->phase=DSR_MAINT_APPLY;result=dsr_maint_store_write(r);if(result!=DSR_OK)goto done;}
 dsr_maintenance_boot_authority(true);
 for(unsigned i=0;i<r->photos;i++){
  const dsr_backup_photo_t *p=dsr_backup_image_photo(image,i);if(!p){result=DSR_INVALID;goto done;}
  bool good=dsr_maint_recovery_photo_read(p->id,verify)==DSR_OK&&digest(verify,DSR_BACKUP_PHOTO_BYTES,p->sha256);
  if(!good){result=dsr_maint_recovery_photo_write(p->id,p->pixels);if(result!=DSR_OK)goto done;result=dsr_maint_recovery_photo_read(p->id,verify);if(result!=DSR_OK)goto done;if(!digest(verify,DSR_BACKUP_PHOTO_BYTES,p->sha256)){result=DSR_BAD_RESPONSE;goto done;}}
  if(!(r->applied_photos&(1u<<i))){r->applied_photos|=(uint16_t)(1u<<i);result=dsr_maint_store_write(r);if(result!=DSR_OK)goto done;}
  char name[32];if(!dsr_maint_record_name(r->operation_id,i+1,name)){result=DSR_INVALID;goto done;}result=dsr_maint_io_remove(name);if(result!=DSR_OK)goto done;report(r,progress,DSR_OK,false);
 }
 const cJSON *root=dsr_backup_image_manifest(image),*config=cJSON_GetObjectItemCaseSensitive(root,"config");
 /* Idempotent source adapters always read back. Reapply on retry also checks
  * the final stores if a cut happened after a durable progress bit. */
 result=dsr_backup_sources_apply(root);if(result==DSR_OK)result=part_done(r,0);if(result!=DSR_OK)goto done;report(r,progress,DSR_OK,false);
 result=dsr_backup_bonds_apply(root);if(result==DSR_OK)result=part_done(r,1);if(result!=DSR_OK)goto done;report(r,progress,DSR_OK,false);
 for(unsigned i=0;i<2;i++){result=dsr_maint_recovery_checkpoint(config,r->config_revision,i);if(result==DSR_OK)result=part_done(r,2+i);if(result!=DSR_OK)goto done;report(r,progress,DSR_OK,false);}
 result=dsr_maint_recovery_unpair();if(result==DSR_OK)result=part_done(r,4);
done:
 dsr_maintenance_boot_authority(false);dsr_maint_io_free(verify,DSR_BACKUP_PHOTO_BYTES);dsr_backup_image_free(image);return result;
}
dsr_result_t dsr_maintenance_recover(dsr_maint_progress_fn progress)
{
 bool expected=false;if(dsr_boot_ready()||!atomic_compare_exchange_strong(&recovering,&expected,true))return DSR_BUSY;
 dsr_maint_record_t r={0};bool found,pending;dsr_result_t result=dsr_maint_store_load(&r,&found,&pending);if(result!=DSR_OK||!pending)goto done;
 if(!found){result=DSR_INVALID;goto done;}uint8_t device[32];result=dsr_backup_device_id(device);if(result!=DSR_OK)goto done;
 if(memcmp(device,r.device,32)){result=DSR_IDENTITY_MISMATCH;goto done;}
 if(r.op==DSR_MAINT_IMPORT_COMMIT){result=dsr_maint_recovery_mount();if(result!=DSR_OK)goto done;}
 if(r.phase==DSR_MAINT_PREPARING){result=cleanup(&r);if(result==DSR_OK)result=dsr_maint_store_finish(&r,true);goto done;}
 if(r.phase!=DSR_MAINT_RECEIPT){
  report(&r,progress,DSR_OK,false);
  if(r.op==DSR_MAINT_IMPORT_COMMIT)result=import_apply(&r,progress);
  else{
   if(r.phase!=DSR_MAINT_APPLY){r.phase=DSR_MAINT_APPLY;result=dsr_maint_store_write(&r);if(result!=DSR_OK)goto done;}
   dsr_maintenance_boot_authority(true);
   for(unsigned i=0;i<10;i++){
    /* Even completed parts are rechecked/re-erased before receipts. No normal
     * services have started and reset contains no external device command. */
    result=dsr_maint_recovery_reset_part(i);if(result==DSR_OK)result=part_done(&r,i);if(result!=DSR_OK)break;report(&r,progress,DSR_OK,false);
   }
   dsr_maintenance_boot_authority(false);
  }
  if(result!=DSR_OK)goto done;
  result=dsr_maint_store_receipt(&r,false);if(result!=DSR_OK)goto done;
 }
 result=cleanup(&r);if(result==DSR_OK)result=dsr_maint_store_finish(&r,r.cancelled!=0);
done:
 dsr_maintenance_boot_authority(false);if(pending)report(&r,progress,result,result==DSR_OK);dsr_backup_wipe(&r,sizeof(r));atomic_store(&recovery_failed,result!=DSR_OK);atomic_store(&recovering,false);return result;
}
dsr_result_t dsr_maintenance_recovery_reset(bool usb)
{
 bool expected=false;if(dsr_boot_ready()||!atomic_load(&recovery_failed)||!atomic_compare_exchange_strong(&recovering,&expected,true))return DSR_BUSY;
 dsr_ota_power_result_t power=dsr_ota_power_read();dsr_result_t result=DSR_BUSY;
 if(power==DSR_OTA_POWER_LOW){result=DSR_OFFLINE;goto done;}
 if(power==DSR_OTA_POWER_UNKNOWN&&!usb){result=DSR_INVALID;goto done;}
 if(power!=DSR_OTA_POWER_READY&&power!=DSR_OTA_POWER_UNKNOWN)goto done;
 dsr_maint_record_t r={.sequence=1,.generation=dsr_maintenance_generation(),.op=DSR_MAINT_RESET,.phase=DSR_MAINT_COMMIT};
 result=dsr_backup_device_id(r.device);if(result==DSR_OK)result=dsr_maint_io_identifier(r.operation_id);if(result==DSR_OK)result=dsr_maint_store_recovery_reset(&r);
 dsr_backup_wipe(&r,sizeof(r));
done:
 atomic_store(&recovering,false);return result;
}
