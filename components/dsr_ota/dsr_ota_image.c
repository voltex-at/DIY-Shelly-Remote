#include "dsr_ota_image.h"
#include "mbedtls/sha256.h"
#include <stdlib.h>
#include <string.h>

/* Offsets from pinned ESP-IDF v5.5.4 esp_app_format.h / esp_app_desc.h:
 * image header 24 B, first segment header 8 B, app descriptor 256 B.
 * Byte reads avoid alignment/strict-aliasing assumptions about BIN input.
 * This is NOT the full IDF segment/checksum/revision/secure-boot validator. */
static const unsigned char marker[]=DSR_OTA_IMAGE_BOARD_MARKER;
struct dsr_ota_image {
 mbedtls_sha256_context hash;
 unsigned char prefix[DSR_OTA_IMAGE_PREFIX_SIZE],expected[32];
 unsigned char fallback[sizeof(marker)];
 size_t received,expected_size,matched;
 dsr_image_result_t result;
 bool hash_live,header_ready,marker_found,finished;
};
_Static_assert(sizeof(struct dsr_ota_image)<=1024,"bounded OTA image heap context");
static uint32_t le32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void release_hash(dsr_ota_image_t *s)
{if(s->hash_live){mbedtls_sha256_free(&s->hash);s->hash_live=false;}}
static dsr_image_result_t fail(dsr_ota_image_t *s,dsr_image_result_t result)
{release_hash(s);s->header_ready=false;s->finished=true;s->result=result;return result;}
static bool header_valid(const dsr_ota_image_t *s)
{
 const unsigned char *p=s->prefix;
 static const char project[]="diy_shelly_remote";
 uint32_t first_length=le32(p+28);
 return p[0]==0xe9 && p[1]>=1 && p[1]<=16 && p[12]==9 && p[13]==0
  && first_length>=256 && first_length<=s->expected_size-32
  && le32(p+32)==0xabcd5432u
  && !memcmp(p+80,project,sizeof(project));
}
dsr_image_result_t dsr_ota_image_reset(dsr_ota_image_t *s,const dsr_ota_verified_t *v)
{
 if(!s)return DSR_IMAGE_ARGUMENT;
 release_hash(s);memset(s,0,sizeof(*s));
 if(!v||v->size<DSR_OTA_IMAGE_PREFIX_SIZE||v->size>DSR_OTA_IMAGE_MAX_SIZE)
  return fail(s,DSR_IMAGE_ARGUMENT);
 s->expected_size=v->size;memcpy(s->expected,v->sha256,32);
 /* KMP carry survives arbitrarily short chunks, including the first 512 B. */
 for(size_t i=1,j=0;i<sizeof(marker);i++){
  while(j&&marker[i]!=marker[j])j=s->fallback[j-1];
  if(marker[i]==marker[j])j++;
  s->fallback[i]=(unsigned char)j;
 }
 mbedtls_sha256_init(&s->hash);s->hash_live=true;
 if(mbedtls_sha256_starts(&s->hash,0))return fail(s,DSR_IMAGE_CRYPTO);
 return DSR_IMAGE_OK;
}
dsr_image_result_t dsr_ota_image_create(const dsr_ota_verified_t *v,dsr_ota_image_t **out)
{
 if(!out)return DSR_IMAGE_ARGUMENT;
 *out=NULL;
 if(!v||v->size<DSR_OTA_IMAGE_PREFIX_SIZE||v->size>DSR_OTA_IMAGE_MAX_SIZE)
  return DSR_IMAGE_ARGUMENT;
 dsr_ota_image_t *s=calloc(1,sizeof(*s));
 if(!s)return DSR_IMAGE_NOMEM;
 dsr_image_result_t r=dsr_ota_image_reset(s,v);
 if(r!=DSR_IMAGE_OK){dsr_ota_image_destroy(s);return r;}
 *out=s;return DSR_IMAGE_OK;
}
dsr_image_result_t dsr_ota_image_update(dsr_ota_image_t *s,const void *data,size_t length)
{
 if(!s)return DSR_IMAGE_ARGUMENT;
 if(s->result!=DSR_IMAGE_OK)return s->result;
 if(s->finished)return fail(s,DSR_IMAGE_STATE);
 if(!data&&length)return fail(s,DSR_IMAGE_ARGUMENT);
 if(length>s->expected_size-s->received)return fail(s,DSR_IMAGE_EXTRA);
 if(!length)return DSR_IMAGE_OK;
 const unsigned char *p=data;
 size_t copy=s->received<sizeof(s->prefix)?sizeof(s->prefix)-s->received:0;
 if(copy>length)copy=length;
 if(copy)memcpy(s->prefix+s->received,p,copy);
 if(mbedtls_sha256_update(&s->hash,p,length))return fail(s,DSR_IMAGE_CRYPTO);
 for(size_t i=0;!s->marker_found&&i<length;i++){
  while(s->matched&&p[i]!=marker[s->matched])s->matched=s->fallback[s->matched-1];
  if(p[i]==marker[s->matched])s->matched++;
  if(s->matched==sizeof(marker))s->marker_found=true;
 }
 s->received+=length;
 if(!s->header_ready&&s->received>=sizeof(s->prefix)){
  if(!header_valid(s))return fail(s,DSR_IMAGE_HEADER);
  s->header_ready=true;
 }
 return DSR_IMAGE_OK;
}
dsr_image_result_t dsr_ota_image_finish(dsr_ota_image_t *s)
{
 if(!s)return DSR_IMAGE_ARGUMENT;
 if(s->finished)return s->result;
 if(s->received!=s->expected_size)return fail(s,DSR_IMAGE_TRUNCATED);
 if(!s->header_ready)return fail(s,DSR_IMAGE_HEADER);
 if(!s->marker_found)return fail(s,DSR_IMAGE_MARKER);
 unsigned char digest[32];
 if(mbedtls_sha256_finish(&s->hash,digest))return fail(s,DSR_IMAGE_CRYPTO);
 release_hash(s);
 unsigned difference=0;
 for(size_t i=0;i<sizeof(digest);i++)difference|=digest[i]^s->expected[i];
 if(difference)return fail(s,DSR_IMAGE_HASH);
 s->finished=true;return DSR_IMAGE_OK;
}
bool dsr_ota_image_header_ready(const dsr_ota_image_t *s)
{return s&&s->result==DSR_IMAGE_OK&&s->header_ready;}
size_t dsr_ota_image_received(const dsr_ota_image_t *s){return s?s->received:0;}
size_t dsr_ota_image_context_size(void){return sizeof(dsr_ota_image_t);}
void dsr_ota_image_destroy(dsr_ota_image_t *s)
{if(s){release_hash(s);memset(s,0,sizeof(*s));free(s);}}
