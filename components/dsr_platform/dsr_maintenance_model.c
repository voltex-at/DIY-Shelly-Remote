#include "dsr_maintenance_model.h"
#include "dsr_photo_name.h"
#include <stdio.h>
#include <string.h>
static bool zero(const uint8_t *p,size_t n){uint8_t v=0;for(size_t i=0;i<n;i++)v|=p[i];return v==0;}
static void wipe(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
dsr_result_t dsr_maint_preflight(const dsr_maint_limits_t *l,uint32_t metadata,unsigned photos,unsigned bonds)
{
 if(!l||!metadata||metadata>65536||photos>16||bonds>104||!l->total||l->total>0x3e0000||l->used>l->total)return DSR_INVALID;
 /* Data/files + authenticated headers, one destination photo, per-file FS
  * overhead and a separate 64 KiB reserve. No OTA partition is considered. */
 uint64_t stage=(uint64_t)metadata+(uint64_t)photos*115200+(photos+1u)*DSR_MAINT_RECORD_HEADER;
 uint64_t peak=(uint64_t)l->used+stage+115200+(photos+2u)*8192+65536;
 if(peak>(uint64_t)l->total*3/4||l->nvs_available<448u+6u*bonds||l->psram_free<196608||l->psram_largest<65536||l->internal_free<32768||l->internal_largest<8192)return DSR_FULL;
 return DSR_OK;
}
bool dsr_maint_record_valid(const dsr_maint_record_t *r)
{
 if(!r||!r->sequence||!r->generation||zero(r->operation_id,16)||zero(r->device,32)||r->phase<DSR_MAINT_PREPARING||r->phase>DSR_MAINT_JOURNAL_FAULT||r->cancelled>1)return false;
 if(r->cancelled&&r->phase!=DSR_MAINT_RECEIPT)return false;
 if(r->phase==DSR_MAINT_PREPARING||r->phase==DSR_MAINT_COMMIT)if(r->applied_photos||r->applied_parts)return false;
 if(r->op==DSR_MAINT_IMPORT_COMMIT){
  if(!r->metadata_bytes||r->metadata_bytes>65536||r->photos>16||r->connections>16||r->rules>8||r->bonds>104||zero(r->candidate_sha256,32)||zero(r->manifest_sha256,32))return false;
  if(r->candidate_bytes!=24+r->metadata_bytes+r->photos*(56u+115200u)||r->candidate_bytes>2097152)return false;
  unsigned mask=(1u<<r->photos)-1;if((r->applied_photos&~mask)||(r->applied_parts&~DSR_MAINT_IMPORT_PARTS))return false;
  if(r->phase==DSR_MAINT_RECEIPT){
   if(!zero(r->key,32)||!zero(r->nonce_prefix,8))return false;
   return r->cancelled?!(r->applied_photos||r->applied_parts):r->applied_photos==mask&&r->applied_parts==DSR_MAINT_IMPORT_PARTS;
  }
  return r->phase==DSR_MAINT_JOURNAL_FAULT||(!zero(r->key,32)&&!zero(r->nonce_prefix,8));
 }
 if(r->op!=DSR_MAINT_RESET||r->candidate_bytes||r->metadata_bytes||r->photos||r->connections||r->rules||r->bonds||r->applied_photos)return false;
 if(!zero(r->candidate_sha256,32)||!zero(r->manifest_sha256,32)||!zero(r->key,32)||!zero(r->nonce_prefix,8)||(r->applied_parts&~DSR_MAINT_RESET_PARTS))return false;
 return r->phase!=DSR_MAINT_RECEIPT||(r->cancelled?r->applied_parts==0:r->applied_parts==DSR_MAINT_RESET_PARTS);
}
static void put(uint8_t *p,uint64_t v,unsigned n){for(unsigned i=0;i<n;i++){p[i]=(uint8_t)v;v>>=8;}}
static uint64_t get(const uint8_t *p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;i++)v|=(uint64_t)p[i]<<(i*8);return v;}
static uint32_t crc(const uint8_t *p,size_t n)
{uint32_t c=UINT32_MAX;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned b=0;b<8;b++)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}return ~c;}
bool dsr_maint_record_encode(const dsr_maint_record_t *r,uint8_t b[DSR_MAINT_JOURNAL_BYTES])
{
 if(!b)return false;
 memset(b,0,DSR_MAINT_JOURNAL_BYTES);if(!dsr_maint_record_valid(r))return false;
 memcpy(b,"DSRMNT1",8);put(b+8,1,2);put(b+10,DSR_MAINT_JOURNAL_BYTES,2);put(b+12,r->sequence,8);put(b+20,r->generation,8);
 memcpy(b+28,r->operation_id,16);memcpy(b+44,r->device,32);memcpy(b+76,r->candidate_sha256,32);memcpy(b+108,r->manifest_sha256,32);
 memcpy(b+140,r->key,32);memcpy(b+172,r->nonce_prefix,8);put(b+180,r->candidate_bytes,4);put(b+184,r->metadata_bytes,4);put(b+188,r->applied_photos,2);put(b+190,r->applied_parts,2);
 b[192]=r->op;b[193]=r->phase;b[194]=r->photos;b[195]=r->connections;b[196]=r->rules;b[197]=r->bonds;b[198]=r->cancelled;put(b+252,crc(b,252),4);return true;
}
dsr_result_t dsr_maint_record_decode(const uint8_t *b,size_t n,dsr_maint_record_t *out)
{
 if(!out)return DSR_INVALID;
 memset(out,0,sizeof(*out));if(!b||n!=DSR_MAINT_JOURNAL_BYTES||memcmp(b,"DSRMNT1",8)||get(b+10,2)!=DSR_MAINT_JOURNAL_BYTES||get(b+252,4)!=crc(b,252)||!zero(b+199,53))return DSR_INVALID;
 if(get(b+8,2)!=1)return DSR_UNSUPPORTED;
 dsr_maint_record_t r={0};r.sequence=get(b+12,8);r.generation=get(b+20,8);memcpy(r.operation_id,b+28,16);memcpy(r.device,b+44,32);memcpy(r.candidate_sha256,b+76,32);memcpy(r.manifest_sha256,b+108,32);
 memcpy(r.key,b+140,32);memcpy(r.nonce_prefix,b+172,8);r.candidate_bytes=(uint32_t)get(b+180,4);r.metadata_bytes=(uint32_t)get(b+184,4);r.applied_photos=(uint16_t)get(b+188,2);r.applied_parts=(uint16_t)get(b+190,2);
 r.op=b[192];r.phase=b[193];r.photos=b[194];r.connections=b[195];r.rules=b[196];r.bonds=b[197];r.cancelled=b[198];
 bool valid=dsr_maint_record_valid(&r);if(valid)*out=r;wipe(&r,sizeof(r));return valid?DSR_OK:DSR_INVALID;
}
bool dsr_maint_record_name(const uint8_t operation[16],unsigned record,char name[32])
{
 if(!operation||!name||record>16||zero(operation,16))return false;
 static const char digits[]="0123456789abcdef";char id[33],temporary[38];
 for(unsigned i=0;i<16;i++){id[2*i]=digits[operation[i]>>4];id[2*i+1]=digits[operation[i]&15];}id[32]=0;
 if(!dsr_photo_filename(id,temporary))return false;
 temporary[0]='m';snprintf(temporary+23,sizeof(temporary)-23,".s%02x",record);memcpy(name,temporary,28);return true;
}
