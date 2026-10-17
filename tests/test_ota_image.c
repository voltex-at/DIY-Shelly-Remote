#include "dsr_ota_image.h"
#include "mbedtls/sha256.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static bool calloc_failure;
void *__real_calloc(size_t count,size_t size);
void *__wrap_calloc(size_t count,size_t size)
{return calloc_failure?NULL:__real_calloc(count,size);}
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static const unsigned char marker[]="DSR-OTA:lcdwiki-es3c35p-n16r8:config-api-1";
#define IMAGE_SIZE 1024u
static void make_image(unsigned char *image,size_t marker_at)
{
 memset(image,0x5a,IMAGE_SIZE);memset(image,0,512);
 image[0]=0xe9;image[1]=1;image[12]=9;image[29]=1; /* LE first-segment length 256. */
 image[32]=0x32;image[33]=0x54;image[34]=0xcd;image[35]=0xab;
 memcpy(image+48,"1.2.4",6);memcpy(image+80,"diy_shelly_remote",sizeof("diy_shelly_remote"));
 if(marker_at+sizeof(marker)<=IMAGE_SIZE)memcpy(image+marker_at,marker,sizeof(marker));
}
static dsr_ota_verified_t metadata(const unsigned char *image,size_t length)
{
 dsr_ota_verified_t v={.size=(uint32_t)length};unsigned out=0;
 CHECK(EVP_Digest(image,length,v.sha256,&out,EVP_sha256(),NULL)==1&&out==32);return v;
}
static void run_chunks(const unsigned char *image,size_t length,size_t chunk)
{
 dsr_ota_verified_t v=metadata(image,length);dsr_ota_image_t *s=NULL;
 CHECK(dsr_ota_image_create(&v,&s)==DSR_IMAGE_OK&&s);
 for(size_t offset=0;offset<length;){size_t n=length-offset;if(n>chunk)n=chunk;CHECK(dsr_ota_image_update(s,image+offset,n)==DSR_IMAGE_OK);offset+=n;CHECK(dsr_ota_image_received(s)==offset);CHECK(dsr_ota_image_header_ready(s)==(offset>=512));}
 CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_OK);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_OK);
 dsr_ota_image_destroy(s);CHECK(!dsr_test_sha_live());
}
/* Faithful scanner-only extraction of old dsr_panel.c. Earlier partial prefix
 * reads are never scanned. Tail retains only the last chunk, losing carry when
 * reads are shorter than the marker. No HTTP, hash or Flash is simulated. */
static bool old_scan(const unsigned char *image,size_t length,size_t first_chunk,size_t later_chunk)
{
 unsigned char tail[80]={0},probe[IMAGE_SIZE+80];size_t tail_size=0,first_size=0;bool header=false,found=false;
 for(size_t offset=0;offset<length;){
  size_t n=offset?later_chunk:first_chunk;if(n>length-offset)n=length-offset;
  const unsigned char *chunk=image+offset;offset+=n;
  if(!header){size_t take=n;if(take>512-first_size)take=512-first_size;first_size+=take;if(first_size<512)continue;header=true;}
  memcpy(probe,tail,tail_size);memcpy(probe+tail_size,chunk,n);
  for(size_t i=0;i+sizeof(marker)<=tail_size+n;i++)if(!memcmp(probe+i,marker,sizeof(marker)))found=true;
  tail_size=n<sizeof(tail)?n:sizeof(tail);memcpy(tail,chunk+n-tail_size,tail_size);
 }return found;
}
static void terminal(const unsigned char *image,const dsr_ota_verified_t *v,dsr_image_result_t expected)
{
 dsr_ota_image_t *s=NULL;CHECK(dsr_ota_image_create(v,&s)==DSR_IMAGE_OK);
 dsr_image_result_t result=dsr_ota_image_update(s,image,v->size);
 if(result==DSR_IMAGE_OK)result=dsr_ota_image_finish(s);
 CHECK(result==expected);CHECK(dsr_ota_image_finish(s)==expected);
 if(expected!=DSR_IMAGE_OK){CHECK(dsr_ota_image_update(s,image,1)==expected);CHECK(!dsr_ota_image_header_ready(s));}
 dsr_ota_image_destroy(s);CHECK(!dsr_test_sha_live());
}
int main(int argc,char **argv)
{
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);unsigned char python_hash[32];CHECK(fread(python_hash,1,32,f)==32);CHECK(fgetc(f)==EOF);fclose(f);
 unsigned char image[IMAGE_SIZE],changed[IMAGE_SIZE];make_image(image,600);
 dsr_ota_verified_t v=metadata(image,sizeof(image));CHECK(!memcmp(v.sha256,python_hash,32));
 CHECK(!old_scan(image,sizeof(image),512,1));CHECK(old_scan(image,sizeof(image),IMAGE_SIZE,IMAGE_SIZE));
 make_image(changed,100);CHECK(!old_scan(changed,sizeof(changed),128,384));CHECK(old_scan(changed,sizeof(changed),512,512));
 /* Every two-chunk split, including 511/512 and every marker-byte boundary. */
 for(size_t split=1;split<sizeof(image);split++){
  dsr_ota_image_t *s=NULL;CHECK(dsr_ota_image_create(&v,&s)==DSR_IMAGE_OK);
  CHECK(dsr_ota_image_update(s,image,split)==DSR_IMAGE_OK);CHECK(dsr_ota_image_header_ready(s)==(split>=512));
  CHECK(dsr_ota_image_update(s,image+split,sizeof(image)-split)==DSR_IMAGE_OK);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_OK);dsr_ota_image_destroy(s);
 }
 const size_t chunks[]={1,2,7,16,41,42,43,44,127,128,255,256,511,512,513,1024};
 for(unsigned i=0;i<sizeof(chunks)/sizeof(chunks[0]);i++)run_chunks(image,sizeof(image),chunks[i]);
 /* Every placement after the validated project name, streamed one byte at a time.
 * This includes markers entirely within the early prefix and crossing 512. */
 for(size_t at=98;at+sizeof(marker)<=sizeof(image);at++){make_image(changed,at);run_chunks(changed,sizeof(changed),1);}
 make_image(changed,288);run_chunks(changed,512,1);
 /* Truncation at every byte: a complete marker is insufficient without size. */
 for(size_t length=0;length<sizeof(image);length++){
  dsr_ota_image_t *s=NULL;CHECK(dsr_ota_image_create(&v,&s)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,image,length)==DSR_IMAGE_OK);
  CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_TRUNCATED);CHECK(dsr_ota_image_update(s,image,1)==DSR_IMAGE_TRUNCATED);dsr_ota_image_destroy(s);
 }
 const size_t header_offsets[]={0,1,12,13,29,32,33,34,35,80,97};
 for(unsigned i=0;i<sizeof(header_offsets)/sizeof(header_offsets[0]);i++){memcpy(changed,image,sizeof(image));changed[header_offsets[i]]=header_offsets[i]==1?0:0xff;dsr_ota_verified_t bad=metadata(changed,sizeof(changed));terminal(changed,&bad,DSR_IMAGE_HEADER);}
 memcpy(changed,image,sizeof(image));changed[1]=17;dsr_ota_verified_t bad=metadata(changed,sizeof(changed));terminal(changed,&bad,DSR_IMAGE_HEADER);
 memcpy(changed,image,sizeof(image));changed[28]=255;changed[29]=0;bad=metadata(changed,sizeof(changed));terminal(changed,&bad,DSR_IMAGE_HEADER);
 memcpy(changed,image,sizeof(image));changed[28]=0xe1;changed[29]=3;bad=metadata(changed,sizeof(changed));terminal(changed,&bad,DSR_IMAGE_HEADER); /* 993 > size-32. */
 memcpy(changed,image,sizeof(image));changed[28]=0xe0;changed[29]=3;changed[1]=16;run_chunks(changed,sizeof(changed),1); /* Inclusive size-32 and segment-count limits. */
 memcpy(changed,image,sizeof(image));changed[642]=1;bad=metadata(changed,sizeof(changed));terminal(changed,&bad,DSR_IMAGE_MARKER);
 make_image(changed,IMAGE_SIZE);bad=metadata(changed,sizeof(changed));terminal(changed,&bad,DSR_IMAGE_MARKER);
 memcpy(changed,image,sizeof(image));changed[900]^=1;terminal(changed,&v,DSR_IMAGE_HASH);
 bad=v;bad.sha256[31]^=1;terminal(image,&bad,DSR_IMAGE_HASH);
 dsr_ota_image_t *s=NULL;CHECK(dsr_ota_image_create(&v,&s)==DSR_IMAGE_OK);
 CHECK(dsr_ota_image_update(s,NULL,0)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,image,IMAGE_SIZE+1)==DSR_IMAGE_EXTRA);CHECK(!dsr_ota_image_received(s));CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_EXTRA);
 CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,image,512)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,image,513)==DSR_IMAGE_EXTRA);CHECK(dsr_ota_image_received(s)==512);
 CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,NULL,1)==DSR_IMAGE_ARGUMENT);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_ARGUMENT);
 CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,image,sizeof(image))==DSR_IMAGE_OK);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,NULL,0)==DSR_IMAGE_STATE);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_STATE);
 CHECK(dsr_ota_image_reset(s,NULL)==DSR_IMAGE_ARGUMENT);CHECK(!dsr_ota_image_received(s)&&!dsr_ota_image_header_ready(s));CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_ARGUMENT);
 CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);make_image(changed,IMAGE_SIZE);bad=metadata(changed,sizeof(changed));CHECK(dsr_ota_image_reset(s,&bad)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,changed,sizeof(changed))==DSR_IMAGE_OK);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_MARKER);dsr_ota_image_destroy(s);
 /* Neither caller metadata edits nor abandoned prefix/marker carry may alter
  * the next image. Reset invalidates all previous progress and acceptance. */
 bad=v;CHECK(dsr_ota_image_create(&bad,&s)==DSR_IMAGE_OK);bad.size=0;memset(bad.sha256,0,32);
 CHECK(dsr_ota_image_update(s,image,sizeof(image))==DSR_IMAGE_OK);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_OK);
 CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,image,200)==DSR_IMAGE_OK);
 CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);CHECK(!dsr_ota_image_received(s)&&!dsr_ota_image_header_ready(s));
 CHECK(dsr_ota_image_update(s,image,sizeof(image))==DSR_IMAGE_OK);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_OK);
 CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);CHECK(dsr_ota_image_update(s,image,621)==DSR_IMAGE_OK);
 make_image(changed,IMAGE_SIZE);bad=metadata(changed,sizeof(changed));CHECK(dsr_ota_image_reset(s,&bad)==DSR_IMAGE_OK);
 CHECK(!dsr_ota_image_received(s)&&!dsr_ota_image_header_ready(s));CHECK(dsr_ota_image_update(s,changed,sizeof(changed))==DSR_IMAGE_OK);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_MARKER);dsr_ota_image_destroy(s);
 /* Exercise the actual six-MiB bound with real data and SHA, rather than
  * treating successful context allocation as validation of a large image. */
 size_t maximum=0x600000;unsigned char *large=malloc(maximum);CHECK(large);memset(large,0x5a,maximum);memcpy(large,image,sizeof(image));
 bad=metadata(large,maximum);CHECK(dsr_ota_image_create(&bad,&s)==DSR_IMAGE_OK);
 unsigned char one=0;CHECK(dsr_ota_image_update(s,&one,SIZE_MAX)==DSR_IMAGE_EXTRA);CHECK(!dsr_ota_image_received(s));
 CHECK(dsr_ota_image_reset(s,&bad)==DSR_IMAGE_OK);
 for(size_t offset=0;offset<maximum;offset+=4096)CHECK(dsr_ota_image_update(s,large+offset,4096)==DSR_IMAGE_OK);
 CHECK(dsr_ota_image_received(s)==maximum&&dsr_ota_image_finish(s)==DSR_IMAGE_OK);dsr_ota_image_destroy(s);free(large);
 for(unsigned failure=1;failure<=4;failure++){
  dsr_test_sha_fault(failure);s=(dsr_ota_image_t *)(uintptr_t)1;
  dsr_image_result_t result=dsr_ota_image_create(&v,&s);
  if(failure<=2)CHECK(result==DSR_IMAGE_CRYPTO&&!s);
  else{CHECK(result==DSR_IMAGE_OK&&s);result=dsr_ota_image_update(s,image,sizeof(image));if(result==DSR_IMAGE_OK)result=dsr_ota_image_finish(s);CHECK(result==DSR_IMAGE_CRYPTO);CHECK(dsr_ota_image_finish(s)==DSR_IMAGE_CRYPTO);CHECK(dsr_ota_image_update(s,image,1)==DSR_IMAGE_CRYPTO);dsr_ota_image_destroy(s);}
  CHECK(!dsr_test_sha_live());
 }
 dsr_test_sha_fault(0);CHECK(dsr_ota_image_create(&v,&s)==DSR_IMAGE_OK);dsr_test_sha_fault(1);CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_CRYPTO);CHECK(!dsr_test_sha_live());CHECK(dsr_ota_image_update(s,image,1)==DSR_IMAGE_CRYPTO);dsr_test_sha_fault(0);CHECK(dsr_ota_image_reset(s,&v)==DSR_IMAGE_OK);dsr_ota_image_destroy(s);
 calloc_failure=true;s=(dsr_ota_image_t *)(uintptr_t)1;CHECK(dsr_ota_image_create(&v,&s)==DSR_IMAGE_NOMEM&&!s);calloc_failure=false;
 for(unsigned i=0;i<3;i++){bad=v;bad.size=i==0?0:i==1?511:0x600001;s=(dsr_ota_image_t *)(uintptr_t)1;CHECK(dsr_ota_image_create(&bad,&s)==DSR_IMAGE_ARGUMENT&&!s);}
 CHECK(dsr_ota_image_create(NULL,&s)==DSR_IMAGE_ARGUMENT&&!s);CHECK(dsr_ota_image_create(&v,NULL)==DSR_IMAGE_ARGUMENT);
 CHECK(dsr_ota_image_update(NULL,image,1)==DSR_IMAGE_ARGUMENT);CHECK(dsr_ota_image_finish(NULL)==DSR_IMAGE_ARGUMENT);CHECK(dsr_ota_image_reset(NULL,&v)==DSR_IMAGE_ARGUMENT);CHECK(!dsr_ota_image_header_ready(NULL)&&!dsr_ota_image_received(NULL));dsr_ota_image_destroy(NULL);
 CHECK(dsr_ota_image_context_size()<=1024);CHECK(!dsr_test_sha_live());
 printf("PASS: %u streaming image checks; real OpenSSL SHA256 + Python oracle, all splits/marker positions/1-byte chunks, old scanner counterexamples, sticky failures/reset/OOM/crypto faults; no Flash/network\n",checks);return 0;
}
