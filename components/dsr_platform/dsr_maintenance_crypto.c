#include "dsr_maintenance_crypto.h"
#include "mbedtls/gcm.h"
#include <string.h>
static void wipe(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
static void put(uint8_t *p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++){p[i]=(uint8_t)v;v>>=8;}}
static size_t payload(const dsr_maint_record_t *r,unsigned index)
{
 if(!dsr_maint_record_valid(r)||r->op!=DSR_MAINT_IMPORT_COMMIT||r->phase==DSR_MAINT_RECEIPT||index>r->photos)return 0;
 uint8_t key=0,nonce=0;for(unsigned i=0;i<32;i++)key|=r->key[i];for(unsigned i=0;i<8;i++)nonce|=r->nonce_prefix[i];
 return key&&nonce?(index?115200:r->metadata_bytes):0;
}
static void header(const dsr_maint_record_t *r,unsigned index,size_t n,uint8_t out[128])
{
 memset(out,0,128);memcpy(out,"DSRREC1",8);put(out+8,1,2);put(out+10,DSR_MAINT_RECORD_HEADER,2);memcpy(out+12,r->operation_id,16);memcpy(out+28,r->device,32);memcpy(out+60,r->manifest_sha256,32);
 out[92]=index?2:1;out[93]=(uint8_t)index;put(out+96,(uint32_t)n,4);memcpy(out+100,r->nonce_prefix,8);put(out+108,index,4);
}
dsr_result_t dsr_maint_crypto_seal(const dsr_maint_record_t *r,unsigned index,const uint8_t *plain,size_t size,uint8_t *sealed,size_t capacity)
{
 size_t n=payload(r,index);if(!n||!plain||!sealed||size!=n)return DSR_INVALID;
 if(capacity<n+DSR_MAINT_RECORD_HEADER)return DSR_FULL;
 header(r,index,n,sealed);mbedtls_gcm_context ctx;mbedtls_gcm_init(&ctx);
 int e=mbedtls_gcm_setkey(&ctx,MBEDTLS_CIPHER_ID_AES,r->key,256);
 if(!e)e=mbedtls_gcm_crypt_and_tag(&ctx,MBEDTLS_GCM_ENCRYPT,n,sealed+100,12,sealed,128,plain,sealed+DSR_MAINT_RECORD_HEADER,16,sealed+128);
 mbedtls_gcm_free(&ctx);if(e){wipe(sealed,n+DSR_MAINT_RECORD_HEADER);return DSR_IO;}return DSR_OK;
}
dsr_result_t dsr_maint_crypto_open(const dsr_maint_record_t *r,unsigned index,const uint8_t *sealed,size_t size,uint8_t *plain,size_t capacity,size_t *written)
{
 if(written)*written=0;
 if(!written||!plain||capacity>115200)return DSR_INVALID;
 wipe(plain,capacity);size_t n=payload(r,index);if(!n||!sealed||size!=n+DSR_MAINT_RECORD_HEADER)return DSR_INVALID;
 if(capacity<n)return DSR_FULL;
 uint8_t expected[128];header(r,index,n,expected);bool match=!memcmp(expected,sealed,sizeof(expected));wipe(expected,sizeof(expected));if(!match)return DSR_INVALID;
 mbedtls_gcm_context ctx;mbedtls_gcm_init(&ctx);int e=mbedtls_gcm_setkey(&ctx,MBEDTLS_CIPHER_ID_AES,r->key,256);
 if(!e)e=mbedtls_gcm_auth_decrypt(&ctx,n,sealed+100,12,sealed,128,sealed+128,16,sealed+DSR_MAINT_RECORD_HEADER,plain);
 mbedtls_gcm_free(&ctx);if(e){wipe(plain,capacity);return DSR_BAD_RESPONSE;}*written=n;return DSR_OK;
}
