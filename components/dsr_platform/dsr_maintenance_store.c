#include "dsr_maintenance_store.h"
#include "nvs.h"
#include <string.h>
static const char *const keys[]={"a","b"};
typedef struct {dsr_maint_record_t record[2];bool present[2],pending;unsigned current;} pair_t;
static void wipe(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
static bool same(const dsr_maint_record_t *a,const dsr_maint_record_t *b)
{
 return a->generation==b->generation&&a->op==b->op&&a->config_revision==b->config_revision&&a->candidate_bytes==b->candidate_bytes&&a->metadata_bytes==b->metadata_bytes&&a->photos==b->photos&&a->connections==b->connections&&a->rules==b->rules&&a->bonds==b->bonds&&
  !memcmp(a->operation_id,b->operation_id,16)&&!memcmp(a->device,b->device,32)&&!memcmp(a->candidate_sha256,b->candidate_sha256,32)&&!memcmp(a->manifest_sha256,b->manifest_sha256,32);
}
static bool transition(const dsr_maint_record_t *a,const dsr_maint_record_t *b)
{
 if(!same(a,b)||!dsr_maint_record_valid(b))return false;
 if(b->phase==DSR_MAINT_RECEIPT){
  if(a->phase==DSR_MAINT_RECEIPT)return a->cancelled==b->cancelled&&a->applied_photos==b->applied_photos&&a->applied_parts==b->applied_parts;
  return b->cancelled?a->phase==DSR_MAINT_PREPARING:
   a->phase==DSR_MAINT_APPLY&&a->applied_photos==b->applied_photos&&a->applied_parts==b->applied_parts;
 }
 if(memcmp(a->key,b->key,32)||memcmp(a->nonce_prefix,b->nonce_prefix,8)||a->cancelled||b->cancelled)return false;
 if((a->applied_photos&~b->applied_photos)||(a->applied_parts&~b->applied_parts))return false;
 if(a->phase==DSR_MAINT_PREPARING)return b->phase==DSR_MAINT_PREPARING||b->phase==DSR_MAINT_COMMIT;
 if(a->phase==DSR_MAINT_COMMIT)return (b->phase==DSR_MAINT_APPLY||(a->op==DSR_MAINT_RESET&&b->phase==DSR_MAINT_COMMIT))&&!b->applied_photos&&!b->applied_parts;
 if(a->phase==DSR_MAINT_APPLY)return b->phase==DSR_MAINT_APPLY||b->phase==DSR_MAINT_JOURNAL_FAULT;
 return a->phase==DSR_MAINT_JOURNAL_FAULT&&(b->phase==DSR_MAINT_JOURNAL_FAULT||b->phase==DSR_MAINT_APPLY)&&a->applied_photos==b->applied_photos&&a->applied_parts==b->applied_parts;
}
static dsr_result_t read_pair(pair_t *p)
{
 memset(p,0,sizeof(*p));nvs_handle_t h;esp_err_t e=nvs_open("dsr_maint",NVS_READONLY,&h);
 if(e==ESP_ERR_NVS_NOT_FOUND)return DSR_OK;
 if(e!=ESP_OK){p->pending=true;return DSR_IO;}
 uint8_t pending=255;esp_err_t latch=nvs_get_u8(h,"pending",&pending);dsr_result_t r=DSR_OK;uint8_t wire[DSR_MAINT_JOURNAL_BYTES];
 for(unsigned i=0;i<2;i++){
  size_t n=sizeof(wire);e=nvs_get_blob(h,keys[i],wire,&n);if(e==ESP_ERR_NVS_NOT_FOUND)continue;
  p->present[i]=true;if(e!=ESP_OK){r=DSR_IO;break;}
  r=dsr_maint_record_decode(wire,n,&p->record[i]);if(r!=DSR_OK)break;
 }
 nvs_close(h);wipe(wire,sizeof(wire));
 if(latch==ESP_ERR_NVS_NOT_FOUND&&!p->present[0]&&!p->present[1]&&r==DSR_OK)return DSR_OK;
 p->pending=true; /* On every ambiguity, ordinary boot stays closed. */
 if(r!=DSR_OK)return r;
 if(latch!=ESP_OK)return latch==ESP_ERR_NVS_NOT_FOUND?DSR_INVALID:DSR_IO;
 if(pending>1||!p->present[0]||!p->present[1])return DSR_INVALID;
 const dsr_maint_record_t *a=&p->record[0],*b=&p->record[1];
 if(!(a->sequence&1)||(b->sequence&1)||a->sequence==b->sequence)return DSR_INVALID;
 p->current=a->sequence>b->sequence?0:1;const dsr_maint_record_t *newest=&p->record[p->current],*older=&p->record[1-p->current];
 if(newest->sequence-older->sequence!=1||!transition(older,newest))return DSR_INVALID;
 if(!pending&&(a->phase!=DSR_MAINT_RECEIPT||b->phase!=DSR_MAINT_RECEIPT))return DSR_INVALID;
 p->pending=pending!=0;return DSR_OK;
}
dsr_result_t dsr_maint_store_load(dsr_maint_record_t *out,bool *found,bool *pending)
{
 if(!out||!found||!pending)return DSR_INVALID;
 memset(out,0,sizeof(*out));*found=false;*pending=true;pair_t p;dsr_result_t r=read_pair(&p);*pending=p.pending;
 if(r==DSR_OK&&p.present[p.current]){*out=p.record[p.current];*found=true;}
 wipe(&p,sizeof(p));return r;
}
static dsr_result_t put_slot(nvs_handle_t h,unsigned slot,const dsr_maint_record_t *record)
{
 uint8_t wire[DSR_MAINT_JOURNAL_BYTES],check[DSR_MAINT_JOURNAL_BYTES];if(!dsr_maint_record_encode(record,wire))return DSR_INVALID;
 esp_err_t e=nvs_set_blob(h,keys[slot],wire,sizeof(wire));if(e==ESP_OK)e=nvs_commit(h);
 if(e==ESP_OK){size_t n=sizeof(check);e=nvs_get_blob(h,keys[slot],check,&n);if(e==ESP_OK&&(n!=sizeof(check)||memcmp(check,wire,sizeof(wire))))e=ESP_FAIL;}
 wipe(wire,sizeof(wire));wipe(check,sizeof(check));return e==ESP_OK?DSR_OK:DSR_IO;
}
static dsr_result_t latch(nvs_handle_t h,uint8_t value)
{
 esp_err_t e=nvs_set_u8(h,"pending",value);if(e==ESP_OK)e=nvs_commit(h);uint8_t check=255;
 if(e==ESP_OK)e=nvs_get_u8(h,"pending",&check);
 return e==ESP_OK&&check==value?DSR_OK:DSR_IO;
}
dsr_result_t dsr_maint_store_begin(dsr_maint_record_t *record)
{
 if(!record)return DSR_INVALID;
 pair_t p;dsr_result_t r=read_pair(&p);bool busy=p.pending;wipe(&p,sizeof(p));if(r!=DSR_OK)return r;if(busy)return DSR_BUSY;
 dsr_maint_record_t next=*record;next.sequence=1;
 if(next.phase!=DSR_MAINT_PREPARING||!dsr_maint_record_valid(&next)){wipe(&next,sizeof(next));return DSR_INVALID;}
 nvs_handle_t h;esp_err_t e=nvs_open("dsr_maint",NVS_READWRITE,&h);if(e!=ESP_OK){wipe(&next,sizeof(next));return DSR_IO;}
 r=latch(h,1);if(r==DSR_OK)r=put_slot(h,0,&next);if(r==DSR_OK){next.sequence=2;r=put_slot(h,1,&next);}nvs_close(h);
 if(r==DSR_OK)*record=next;
 wipe(&next,sizeof(next));return r;
}
dsr_result_t dsr_maint_store_write(dsr_maint_record_t *record)
{
 if(!record)return DSR_INVALID;
 pair_t p;dsr_result_t r=read_pair(&p);if(r!=DSR_OK){wipe(&p,sizeof(p));return r;}
 const dsr_maint_record_t *current=&p.record[p.current];
 if(!p.pending||!p.present[p.current]||!same(current,record)||current->sequence!=record->sequence){wipe(&p,sizeof(p));return DSR_CONFLICT;}
 if(current->sequence==UINT64_MAX){wipe(&p,sizeof(p));return DSR_FULL;}
 if(!transition(current,record)){wipe(&p,sizeof(p));return DSR_INVALID;}
 dsr_maint_record_t next=*record;next.sequence++;unsigned destination=1-p.current;wipe(&p,sizeof(p));
 nvs_handle_t h;esp_err_t e=nvs_open("dsr_maint",NVS_READWRITE,&h);if(e!=ESP_OK){wipe(&next,sizeof(next));return DSR_IO;}
 r=put_slot(h,destination,&next);nvs_close(h);if(r==DSR_OK)*record=next;wipe(&next,sizeof(next));return r;
}
dsr_result_t dsr_maint_store_receipt(dsr_maint_record_t *record,bool cancelled)
{
 if(!record)return DSR_INVALID;
 dsr_maint_record_t next;bool found,pending;dsr_result_t r=dsr_maint_store_load(&next,&found,&pending);
 if(r!=DSR_OK)goto done;
 if(!found||!same(&next,record)){r=DSR_CONFLICT;goto done;}
 if(next.phase==DSR_MAINT_RECEIPT){if(next.cancelled!=cancelled){r=DSR_INVALID;goto done;}if(!pending){*record=next;goto done;}}
 else if((cancelled&&next.phase!=DSR_MAINT_PREPARING)||(!cancelled&&next.phase!=DSR_MAINT_APPLY)){r=DSR_INVALID;goto done;}
 next.phase=DSR_MAINT_RECEIPT;next.cancelled=cancelled?1:0;wipe(next.key,sizeof(next.key));wipe(next.nonce_prefix,sizeof(next.nonce_prefix));
 if(!dsr_maint_record_valid(&next)){r=DSR_INVALID;goto done;}
 /* Two receipts replace both key-bearing copies before pending is cleared. */
 r=dsr_maint_store_write(&next);if(r==DSR_OK)r=dsr_maint_store_write(&next);if(r!=DSR_OK)goto done;
 *record=next;
done:
 wipe(&next,sizeof(next));return r;
}
dsr_result_t dsr_maint_store_finish(dsr_maint_record_t *record,bool cancelled)
{
 dsr_result_t r=dsr_maint_store_receipt(record,cancelled);if(r!=DSR_OK)return r;
 nvs_handle_t h;if(nvs_open("dsr_maint",NVS_READWRITE,&h)!=ESP_OK)return DSR_IO;
 r=latch(h,0);nvs_close(h);return r;
}
dsr_result_t dsr_maint_store_recovery_reset(dsr_maint_record_t *record)
{
 if(!record||record->op!=DSR_MAINT_RESET||record->phase!=DSR_MAINT_COMMIT||record->applied_parts||record->applied_photos)return DSR_INVALID;
 dsr_maint_record_t next=*record;next.sequence=1;if(!dsr_maint_record_valid(&next)){wipe(&next,sizeof(next));return DSR_INVALID;}
 nvs_handle_t h;if(nvs_open("dsr_maint",NVS_READWRITE,&h)!=ESP_OK){wipe(&next,sizeof(next));return DSR_IO;}
 dsr_result_t r=latch(h,1);if(r==DSR_OK)r=put_slot(h,0,&next);if(r==DSR_OK){next.sequence=2;r=put_slot(h,1,&next);}nvs_close(h);
 if(r==DSR_OK)*record=next;
 wipe(&next,sizeof(next));return r;
}
