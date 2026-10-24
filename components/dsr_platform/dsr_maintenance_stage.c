#include "dsr_maintenance_stage.h"
#include "dsr_maintenance_crypto.h"
#include "dsr_maintenance_io.h"
#include "dsr_maintenance_store.h"
#include "dsr_maintenance.h"
#include "dsr_maintenance_private.h"
#include "dsr_activity_internal.h"
#include "dsr_backup_sources.h"
#include "dsr_backup_sources_model.h"
#include "dsr_backup_bonds.h"
#include "mbedtls/sha256.h"
#include <string.h>
enum {PLAIN=115200,SEALED=115200+DSR_MAINT_RECORD_HEADER};
struct dsr_maint_stage {dsr_maint_record_t record;cJSON *manifest;bool verified;};
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t *p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++){p[i]=(uint8_t)v;v>>=8;}}
static size_t length(const dsr_maint_record_t *r,unsigned index)
{return dsr_maint_record_valid(r)&&r->op==DSR_MAINT_IMPORT_COMMIT&&index<=r->photos?(index?PLAIN:r->metadata_bytes):0;}
static bool identity(const dsr_maint_record_t *a,const dsr_maint_record_t *b)
{return a->generation==b->generation&&a->op==b->op&&!memcmp(a->operation_id,b->operation_id,16)&&!memcmp(a->device,b->device,32)&&!memcmp(a->candidate_sha256,b->candidate_sha256,32);}
static dsr_result_t write_file(const dsr_maint_record_t *r,unsigned index,const uint8_t *bytes,size_t size)
{
 char name[32];if(!dsr_maint_record_name(r->operation_id,index,name))return DSR_INVALID;
 dsr_maint_file_t *f=NULL;dsr_result_t result=dsr_maint_io_open_write(name,&f);if(result!=DSR_OK)return result;
 for(size_t at=0;at<size;){size_t n=size-at;if(n>DSR_MAINT_IO_CHUNK)n=DSR_MAINT_IO_CHUNK;result=dsr_maint_io_write(f,bytes+at,n);if(result!=DSR_OK)break;at+=n;}
 dsr_result_t closed=dsr_maint_io_close(f);return result==DSR_OK?closed:result;
}
static dsr_result_t read_record(const dsr_maint_record_t *r,unsigned index,uint8_t *cipher,uint8_t *plain,size_t capacity,size_t *written)
{
 *written=0;size_t n=length(r,index);if(!n||capacity<n)return n?DSR_FULL:DSR_INVALID;
 char name[32];if(!dsr_maint_record_name(r->operation_id,index,name))return DSR_INVALID;
 dsr_maint_file_t *f=NULL;size_t size=0;dsr_result_t result=dsr_maint_io_open_read(name,&f,&size);if(result!=DSR_OK)return result;
 if(size!=n+DSR_MAINT_RECORD_HEADER)result=DSR_INVALID;
 for(size_t at=0;result==DSR_OK&&at<size;){
  size_t chunk=size-at;if(chunk>DSR_MAINT_IO_CHUNK)chunk=DSR_MAINT_IO_CHUNK;size_t got=0;result=dsr_maint_io_read(f,cipher+at,chunk,&got);
  if(result==DSR_OK&&(!got||got>chunk))result=DSR_IO;
  at+=got;
 }
 dsr_result_t closed=dsr_maint_io_close(f);if(result==DSR_OK)result=closed;
 return result==DSR_OK?dsr_maint_crypto_open(r,index,cipher,size,plain,capacity,written):result;
}
dsr_result_t dsr_maint_stage_read(const dsr_maint_record_t *r,unsigned index,uint8_t *out,size_t capacity,size_t *written)
{
 if(written)*written=0;
 if(!out||!written||capacity>PLAIN)return DSR_INVALID;
 dsr_backup_wipe(out,capacity);
 uint8_t *cipher=dsr_maint_io_alloc(SEALED);if(!cipher)return DSR_FULL;
 dsr_result_t result=read_record(r,index,cipher,out,capacity,written);if(result!=DSR_OK)dsr_backup_wipe(out,capacity);dsr_maint_io_free(cipher,SEALED);return result;
}
static int digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static bool photo_sha(const cJSON *manifest,unsigned index,uint8_t out[32])
{
 const cJSON *p=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(manifest,"photos"),(int)index),*s=cJSON_GetObjectItemCaseSensitive(p,"sha256");
 if(!cJSON_IsString(s)||strlen(s->valuestring)!=64)return false;
 for(unsigned i=0;i<32;i++){int a=digit(s->valuestring[2*i]),b=digit(s->valuestring[2*i+1]);if(a<0||b<0)return false;out[i]=(uint8_t)(a*16+b);}return true;
}
static dsr_result_t verify(dsr_maint_stage_t *s,const dsr_maint_lease_t *lease,uint8_t *cipher,uint8_t *plain)
{
 uint8_t header[56]={0},digest[32],wanted[32];mbedtls_sha256_context sha;mbedtls_sha256_init(&sha);dsr_result_t result=DSR_IO;
 if(mbedtls_sha256_starts(&sha,0))goto done;
 memcpy(header,"DSRSNP1",8);put(header+8,1,2);put(header+10,24,2);put(header+12,s->record.metadata_bytes,4);put(header+16,s->record.photos,2);put(header+20,s->record.candidate_bytes,4);
 if(mbedtls_sha256_update(&sha,header,24))goto done;
 for(unsigned i=0;i<=s->record.photos;i++){
  if(!dsr_maintenance_lease_valid(lease)){result=DSR_BUSY;goto done;}
  size_t n=0;result=read_record(&s->record,i,cipher,plain,PLAIN,&n);if(result!=DSR_OK)goto done;
  if(mbedtls_sha256(plain,n,digest,0)){result=DSR_IO;goto done;}
  if(!i)memcpy(wanted,s->record.manifest_sha256,32);else if(!photo_sha(s->manifest,i-1,wanted)){result=DSR_INVALID;goto done;}
  if(memcmp(digest,wanted,32)){result=DSR_BAD_RESPONSE;goto done;}
  if(i){memset(header,0,sizeof(header));memcpy(header,wanted,16);memcpy(header+16,wanted,32);put(header+48,PLAIN,4);if(mbedtls_sha256_update(&sha,header,sizeof(header))){result=DSR_IO;goto done;}}
  if(mbedtls_sha256_update(&sha,plain,n)){result=DSR_IO;goto done;}
 }
 result=mbedtls_sha256_finish(&sha,digest)?DSR_IO:memcmp(digest,s->record.candidate_sha256,32)?DSR_BAD_RESPONSE:DSR_OK;
 if(result==DSR_OK&&!dsr_maintenance_lease_valid(lease))result=DSR_BUSY;
done:
 mbedtls_sha256_free(&sha);dsr_backup_wipe(digest,sizeof(digest));dsr_backup_wipe(wanted,sizeof(wanted));return result;
}
static bool nonzero(const uint8_t *p,size_t n){uint8_t v=0;for(size_t i=0;i<n;i++)v|=p[i];return v!=0;}
static dsr_result_t fresh(dsr_maint_record_t *r)
{
 for(unsigned attempt=0;attempt<3;attempt++){
  dsr_result_t result=dsr_maint_io_random(r->operation_id,16);if(result==DSR_OK)result=dsr_maint_io_random(r->key,32);if(result==DSR_OK)result=dsr_maint_io_random(r->nonce_prefix,8);if(result!=DSR_OK)return result;
  if(!nonzero(r->operation_id,16)||!nonzero(r->key,32)||!nonzero(r->nonce_prefix,8))continue;
  bool conflict=false;for(unsigned i=0;i<=r->photos;i++){char name[32];bool exists;if(!dsr_maint_record_name(r->operation_id,i,name))return DSR_INVALID;result=dsr_maint_io_exists(name,&exists);if(result!=DSR_OK)return result;conflict|=exists;}
  if(!conflict)return DSR_OK;
 }return DSR_CONFLICT;
}
dsr_result_t dsr_maint_stage_cleanup_preparing(const dsr_maint_record_t *expected)
{
 if(!expected)return DSR_INVALID;
 dsr_maint_record_t r;bool found,pending;dsr_result_t result=dsr_maint_store_load(&r,&found,&pending);
 if(result!=DSR_OK)goto done;
 if(!found||!identity(&r,expected)||r.phase!=DSR_MAINT_PREPARING||!pending){result=DSR_CONFLICT;goto done;}
 for(unsigned i=0;i<=r.photos;i++){
  char name[32];if(!dsr_maint_record_name(r.operation_id,i,name)){result=DSR_INVALID;goto done;}
  result=dsr_maint_io_remove(name);if(result!=DSR_OK)goto done;
 }
 result=dsr_maint_store_finish(&r,true);
done:
 dsr_backup_wipe(&r,sizeof(r));return result;
}
dsr_result_t dsr_maint_stage_cancel(dsr_maint_stage_t *s)
{return s?dsr_maint_stage_cleanup_preparing(&s->record):DSR_INVALID;}
void dsr_maint_stage_free(dsr_maint_stage_t *s)
{if(s){dsr_backup_json_discard(s->manifest);dsr_maint_io_free(s,sizeof(*s));}}
bool dsr_maint_stage_record(const dsr_maint_stage_t *s,dsr_maint_record_t *out)
{if(!out)return false;memset(out,0,sizeof(*out));if(!s)return false;*out=s->record;return true;}
bool dsr_maint_stage_archive_info(const dsr_maint_stage_t *s,dsr_maint_archive_info_t *out)
{
 if(!out)return false;
 memset(out,0,sizeof(*out));if(!s||!s->manifest)return false;
 const cJSON *fw=cJSON_GetObjectItemCaseSensitive(s->manifest,"firmware"),*created=cJSON_GetObjectItemCaseSensitive(s->manifest,"created");
 if(!cJSON_IsString(fw)||!fw->valuestring[0]||strlen(fw->valuestring)>=sizeof(out->firmware))return false;
 memcpy(out->firmware,fw->valuestring,strlen(fw->valuestring)+1);if(cJSON_IsNumber(created))out->created=(uint32_t)created->valuedouble;return true;
}
dsr_result_t dsr_maint_stage_prepare(const dsr_backup_image_t *image,dsr_maint_stage_t **out)
{
 if(!out)return DSR_INVALID;
 *out=NULL;if(!image)return DSR_INVALID;
 dsr_maint_lease_t lease;if(!dsr_maintenance_acquire(&lease))return DSR_BUSY;
 dsr_result_t result=DSR_FULL;dsr_maint_stage_t *s=NULL;dsr_backup_sources_data_t *data=NULL;uint8_t *cipher=NULL,*plain=NULL;bool begun=false;
 size_t bytes=0;const uint8_t *raw=dsr_backup_image_bytes(image,&bytes);const cJSON *manifest=dsr_backup_image_manifest(image);dsr_maint_limits_t limits;
 if(!raw||bytes<24||!manifest){result=DSR_INVALID;goto done;}
 uint32_t metadata=get32(raw+12);unsigned photos=(unsigned)cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(manifest,"photos")),bonds=(unsigned)cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(manifest,"bonds"),"records"));
 result=dsr_maint_io_limits(&limits);if(result!=DSR_OK)goto done;result=dsr_maint_preflight(&limits,metadata,photos,bonds);if(result!=DSR_OK)goto done;
 /* Allocate the actual blocks, not just a favorable total-free snapshot. */
 s=dsr_maint_io_alloc(sizeof(*s));cipher=dsr_maint_io_alloc(SEALED);plain=dsr_maint_io_alloc(PLAIN);data=dsr_maint_io_alloc(sizeof(*data));if(!s||!cipher||!plain||!data){result=DSR_FULL;goto done;}
 result=dsr_backup_device_id(s->record.device);if(result!=DSR_OK)goto done;
 result=dsr_backup_sources_decode(manifest,s->record.device,data);if(result!=DSR_OK)goto done;result=dsr_backup_bonds_preflight(manifest);if(result!=DSR_OK)goto done;
 s->manifest=cJSON_Duplicate(manifest,true);if(!s->manifest){result=DSR_FULL;goto done;}
 s->record.sequence=1;s->record.generation=lease.generation;s->record.op=DSR_MAINT_IMPORT_COMMIT;s->record.phase=DSR_MAINT_PREPARING;s->record.photos=(uint8_t)photos;s->record.bonds=(uint8_t)bonds;
 s->record.connections=(uint8_t)data->count;s->record.rules=data->enrollment.rules.count;s->record.metadata_bytes=metadata;s->record.candidate_bytes=(uint32_t)bytes;
 result=dsr_maint_io_next_revision((uint32_t)cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(manifest,"config"),"revision")->valuedouble,&s->record.config_revision);if(result!=DSR_OK)goto done;
 if(mbedtls_sha256(raw,bytes,s->record.candidate_sha256,0)||mbedtls_sha256(raw+24,metadata,s->record.manifest_sha256,0)){result=DSR_IO;goto done;}
 result=fresh(&s->record);if(result!=DSR_OK)goto done;if(!dsr_maintenance_lease_valid(&lease)){result=DSR_CONFLICT;goto done;}
 result=dsr_maint_store_begin(&s->record);if(result!=DSR_OK)goto done;begun=true;
 for(unsigned i=0;i<=photos;i++){
  if(!dsr_maintenance_lease_valid(&lease)){result=DSR_BUSY;goto done;}
  const dsr_backup_photo_t *p=i?dsr_backup_image_photo(image,i-1):NULL;const uint8_t *input=i?(p?p->pixels:NULL):raw+24;size_t n=i?PLAIN:metadata;
  result=dsr_maint_crypto_seal(&s->record,i,input,n,cipher,SEALED);if(result!=DSR_OK)goto done;result=write_file(&s->record,i,cipher,n+DSR_MAINT_RECORD_HEADER);if(result!=DSR_OK)goto done;
 }
 result=verify(s,&lease,cipher,plain);if(result==DSR_OK){*out=s;s=NULL;}
done:
 if(s&&begun){dsr_result_t cleanup=dsr_maint_stage_cancel(s);if(cleanup!=DSR_OK)result=DSR_IO;}
 dsr_maint_io_free(data,data?sizeof(*data):0);dsr_maint_io_free(cipher,cipher?SEALED:0);dsr_maint_io_free(plain,plain?PLAIN:0);dsr_maint_stage_free(s);dsr_maintenance_release(&lease);return result;
}
dsr_result_t dsr_maint_stage_barrier(dsr_maint_stage_t *s,const dsr_maint_lease_t *lease)
{
 if(!s||!lease)return DSR_INVALID;
 s->verified=false;
 if(!dsr_maintenance_lease_valid(lease))return DSR_BUSY;
 if(s->record.generation!=lease->generation)return DSR_CONFLICT;
 uint8_t *cipher=dsr_maint_io_alloc(SEALED),*plain=dsr_maint_io_alloc(PLAIN);cJSON *barrier=NULL;dsr_result_t result=DSR_FULL;
 if(!cipher||!plain)goto done;
 result=verify(s,lease,cipher,plain);if(result!=DSR_OK)goto done;
 /* Stop/deinit acknowledgements end all uncounted SDK store callbacks before
  * the final atomic, non-revocable intent transition. No new host is started. */
 barrier=cJSON_CreateObject();if(!barrier){result=DSR_FULL;goto done;}
 result=dsr_maintenance_bonds_capture(lease,barrier);
 if(result==DSR_OK){s->verified=dsr_maintenance_lease_valid(lease);if(!s->verified)result=DSR_CONFLICT;}
done:
 dsr_backup_json_discard(barrier);dsr_maint_io_free(cipher,cipher?SEALED:0);dsr_maint_io_free(plain,plain?PLAIN:0);return result;
}
dsr_result_t dsr_maint_stage_commit_verified(dsr_maint_stage_t *s,const dsr_maint_lease_t *lease)
{
 if(!s||!lease)return DSR_INVALID;
 if(!s->verified||s->record.generation!=lease->generation||!dsr_maintenance_lease_valid(lease))return DSR_CONFLICT;
 s->verified=false;
 if(!dsr_activity_maintenance_commit_begin(lease->reservation,lease->generation))return DSR_CONFLICT;
 dsr_maint_record_t next=s->record;next.phase=DSR_MAINT_COMMIT;dsr_result_t result=dsr_maint_store_write(&next);
 if(result==DSR_OK)s->record=next;
 else{
  bool found,pending;dsr_maint_record_t current;dsr_result_t read=dsr_maint_store_load(&current,&found,&pending);
  if(read==DSR_OK&&found&&pending&&identity(&current,&s->record)){
   s->record=current;
   if(current.phase==DSR_MAINT_PREPARING)(void)dsr_activity_maintenance_commit_abort(lease->reservation,lease->generation);
  }
  dsr_backup_wipe(&current,sizeof(current));
 }
 dsr_backup_wipe(&next,sizeof(next));return result;
}
dsr_result_t dsr_maint_stage_commit(dsr_maint_stage_t *s,const dsr_maint_lease_t *lease)
{dsr_result_t result=dsr_maint_stage_barrier(s,lease);return result==DSR_OK?dsr_maint_stage_commit_verified(s,lease):result;}
