#include "dsr_automation_store.h"
#include "dsr_automation_model.h"
#include "dsr_activity.h"
#include "nvs.h"
#include <stdlib.h>
#include <string.h>

/* Fixed canonical little-endian layout, never a raw compiler structure. */
enum {HEADER=20,KEY_BYTES=88,RULE_BYTES=16,CYCLE_BYTES=8,TARGET_BYTES=8,
 WIRE=HEADER+DSR_AUTO_PARTICIPANTS*KEY_BYTES+DSR_AUTO_RULES*(RULE_BYTES+CYCLE_BYTES)+DSR_AUTO_PARTICIPANTS*TARGET_BYTES+4};
_Static_assert(WIRE<=DSR_AUTO_RECORD_MAX,"journal wire budget");
static const char *const slots[]={"a","b"};
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint16_t get16(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t get32(const uint8_t *p){uint32_t v=0;for(unsigned i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
static uint32_t crc(const uint8_t *p,size_t size)
{
 uint32_t v=UINT32_MAX;
 for(size_t i=0;i<size;i++){v^=p[i];for(unsigned b=0;b<8;b++)v=(v>>1)^(0xedb88320u&(0u-(v&1)));}
 return ~v;
}
static void encode(const dsr_auto_record_t *r,uint32_t seq,uint8_t *b)
{
 memset(b,0,WIRE);memcpy(b,"DSRA",4);put16(b+4,1);put16(b+6,WIRE);put32(b+8,seq);put32(b+12,r->generation);
 b[16]=r->rules.count;b[17]=r->rules.key_count;size_t p=HEADER;
 for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++,p+=KEY_BYTES){
  const dsr_auto_key_t *k=&r->rules.keys[i];put32(b+p,k->layout_id);memcpy(b+p+4,k->device_id,64);memcpy(b+p+68,k->host,16);
  b[p+84]=k->generation;b[p+85]=k->channel;b[p+86]=k->kind;
 }
 for(unsigned i=0;i<DSR_AUTO_RULES;i++,p+=RULE_BYTES){
  const dsr_auto_rule_t *v=&r->rules.items[i];put32(b+p,v->id);put32(b+p+4,v->signature);
  put16(b+p+8,v->sensors);put16(b+p+10,v->targets);b[p+12]=v->target_half;b[p+13]=v->enabled;
 }
 for(unsigned i=0;i<DSR_AUTO_RULES;i++,p+=CYCLE_BYTES){put32(b+p,r->cycles[i].cycle);b[p+4]=r->cycles[i].active;}
 for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++,p+=TARGET_BYTES){
  const dsr_auto_target_t *t=&r->targets[i];put32(b+p,t->manual_intent);
  b[p+4]=t->baseline;b[p+5]=t->desired;b[p+6]=t->phase;b[p+7]=t->override;
 }
 put32(b+WIRE-4,crc(b,WIRE-4));
}
static esp_err_t header(const uint8_t *b)
{
 if(memcmp(b,"DSRA",4)||get16(b+6)!=WIRE||!get32(b+8)||!get32(b+12)||b[18]||b[19])return ESP_ERR_INVALID_SIZE;
 if(get16(b+4)!=1)return ESP_ERR_NOT_SUPPORTED;
 if(get32(b+WIRE-4)!=crc(b,WIRE-4))return ESP_ERR_INVALID_SIZE;
 return ESP_OK;
}
static bool decode(const uint8_t *b,dsr_auto_record_t *r)
{
 memset(r,0,sizeof(*r));r->generation=get32(b+12);r->rules.count=b[16];r->rules.key_count=b[17];size_t p=HEADER;
 for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++,p+=KEY_BYTES){
  dsr_auto_key_t *k=&r->rules.keys[i];k->layout_id=get32(b+p);memcpy(k->device_id,b+p+4,64);memcpy(k->host,b+p+68,16);
  k->generation=b[p+84];k->channel=b[p+85];k->kind=b[p+86];if(b[p+87])return false;
 }
 for(unsigned i=0;i<DSR_AUTO_RULES;i++,p+=RULE_BYTES){
  dsr_auto_rule_t *v=&r->rules.items[i];v->id=get32(b+p);v->signature=get32(b+p+4);
  v->sensors=get16(b+p+8);v->targets=get16(b+p+10);v->target_half=b[p+12];v->enabled=b[p+13];if(b[p+14]||b[p+15])return false;
 }
 for(unsigned i=0;i<DSR_AUTO_RULES;i++,p+=CYCLE_BYTES){
  r->cycles[i].cycle=get32(b+p);r->cycles[i].active=b[p+4];if(b[p+5]||b[p+6]||b[p+7])return false;
 }
 for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++,p+=TARGET_BYTES){
  dsr_auto_target_t *t=&r->targets[i];t->manual_intent=get32(b+p);
  t->baseline=b[p+4];t->desired=b[p+5];t->phase=b[p+6];t->override=b[p+7];
 }
 return dsr_auto_record_valid(r);
}
static esp_err_t latest(uint8_t *pair,unsigned *chosen,bool *found)
{
 *found=false;*chosen=0;nvs_handle_t h;esp_err_t e=nvs_open("dsr_auto",NVS_READONLY,&h);
 if(e==ESP_ERR_NVS_NOT_FOUND)return ESP_OK;
 if(e!=ESP_OK)return e;
 dsr_auto_record_t *decoded=malloc(sizeof(*decoded));
 if(!decoded){nvs_close(h);return ESP_ERR_NO_MEM;}
 bool present[2]={false,false};
 for(unsigned i=0;i<2;i++){
  size_t size=0;e=nvs_get_blob(h,slots[i],NULL,&size);
  if(e==ESP_ERR_NVS_NOT_FOUND){e=ESP_OK;continue;}
  if(e!=ESP_OK)break;
  /* A corrupt header cannot be trusted to prove this was the older slot. */
  if(size!=WIRE){e=ESP_ERR_INVALID_SIZE;break;}
  e=nvs_get_blob(h,slots[i],pair+i*WIRE,&size);if(e!=ESP_OK)break;
  if(size!=WIRE){e=ESP_ERR_INVALID_SIZE;break;}
  e=header(pair+i*WIRE);if(e!=ESP_OK)break;
  if(!decode(pair+i*WIRE,decoded)){e=ESP_ERR_INVALID_ARG;break;}present[i]=true;
 }
 nvs_close(h);free(decoded);if(e!=ESP_OK)return e;
 if(present[0]&&present[1]){
  uint32_t a=get32(pair+8),b=get32(pair+WIRE+8);
  if(a==b||(a>b?a-b:b-a)!=1)return ESP_ERR_INVALID_STATE;
  *chosen=b>a?1u:0u;
 }else if(present[1])*chosen=1;
 *found=present[0]||present[1];return ESP_OK;
}
esp_err_t dsr_auto_store_load(dsr_auto_record_t *out,bool *found)
{
 if(!out||!found)return ESP_ERR_INVALID_ARG;
 *found=false;memset(out,0,sizeof(*out));uint8_t *pair=malloc(2*WIRE);if(!pair)return ESP_ERR_NO_MEM;
 unsigned chosen;bool exists;esp_err_t e=latest(pair,&chosen,&exists);
 if(e==ESP_OK&&exists){if(!decode(pair+chosen*WIRE,out)){memset(out,0,sizeof(*out));e=ESP_ERR_INVALID_ARG;}else *found=true;}
 free(pair);return e;
}
esp_err_t dsr_auto_store_save(const dsr_auto_record_t *record)
{
 if(!dsr_auto_record_valid(record))return ESP_ERR_INVALID_ARG;
 uint8_t *bytes=malloc(3*WIRE);if(!bytes)return ESP_ERR_NO_MEM;
 uint8_t *next=bytes+2*WIRE;unsigned chosen;bool exists;esp_err_t e=latest(bytes,&chosen,&exists);
 if(e!=ESP_OK){free(bytes);return e;}
 uint32_t seq=exists?get32(bytes+chosen*WIRE+8):0;
 if(seq==UINT32_MAX){free(bytes);return ESP_ERR_INVALID_STATE;}
 if(exists&&record->generation<=get32(bytes+chosen*WIRE+12)){
  encode(record,seq,next);e=memcmp(next,bytes+chosen*WIRE,WIRE)?ESP_ERR_INVALID_STATE:ESP_OK;free(bytes);return e;
 }
 unsigned destination=exists?1u-chosen:0;encode(record,seq+1,next);nvs_handle_t h;
 e=nvs_open("dsr_auto",NVS_READWRITE,&h);
 if(e==ESP_OK){dsr_activity_persistent_write();e=nvs_set_blob(h,slots[destination],next,WIRE);if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);}
 if(e==ESP_OK){
  e=nvs_open("dsr_auto",NVS_READONLY,&h);
  if(e==ESP_OK){size_t size=WIRE;e=nvs_get_blob(h,slots[destination],bytes,&size);nvs_close(h);
   if(e==ESP_OK&&(size!=WIRE||memcmp(bytes,next,WIRE)))e=ESP_FAIL;}
 }
 free(bytes);return e;
}
#if defined(DSR_BOARD_ES3C35P) && (defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA))
#include "dsr_backup_storage.h"
#include "dsr_maintenance_private.h"
esp_err_t dsr_auto_store_restore(const dsr_auto_record_t *record)
{
 if(!dsr_maintenance_boot_writes_allowed())return ESP_ERR_INVALID_STATE;
 if(!dsr_auto_record_valid(record)||record->generation!=1)return ESP_ERR_INVALID_ARG;
 for(unsigned i=0;i<DSR_AUTO_RULES;i++)if(record->cycles[i].active||record->cycles[i].cycle)return ESP_ERR_INVALID_ARG;
 const dsr_auto_target_t zero={0};for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++)if(memcmp(&record->targets[i],&zero,sizeof(zero)))return ESP_ERR_INVALID_ARG;
 nvs_handle_t h;esp_err_t e=nvs_open("dsr_auto",NVS_READWRITE,&h);if(e!=ESP_OK)return e;
 e=nvs_erase_all(h);if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);
 /* Interrupted replacement is retried before any ordinary boot reader. */
 return e==ESP_OK?dsr_auto_store_save(record):e;
}
#endif
