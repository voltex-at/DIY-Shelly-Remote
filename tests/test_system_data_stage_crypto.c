#include "dsr_maintenance_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"stage-crypto:%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void)
{
 dsr_maint_record_t r={.sequence=2,.generation=7,.candidate_bytes=24+500+16*(56+115200),.metadata_bytes=500,.op=DSR_MAINT_IMPORT_COMMIT,.phase=DSR_MAINT_PREPARING,.photos=16,.connections=16,.rules=8,.bonds=16};
 r.config_revision=901;r.operation_id[0]=1;r.device[1]=2;r.candidate_sha256[2]=3;r.manifest_sha256[3]=4;r.key[4]=5;r.nonce_prefix[5]=6;
 uint8_t *plain=malloc(115200),*result=malloc(115200),*zeros=calloc(1,115200),*sealed=malloc(115200+144),*other=malloc(115200+144);CHECK(plain&&result&&zeros&&sealed&&other);
 for(unsigned i=0;i<115200;i++)plain[i]=(uint8_t)i;
 size_t written=99;CHECK(dsr_maint_crypto_seal(&r,1,plain,115200,sealed,115200+144)==DSR_OK);CHECK(memcmp(sealed+144,plain,115200));
 CHECK(dsr_maint_crypto_open(&r,1,sealed,115200+144,result,115200,&written)==DSR_OK&&written==115200&&!memcmp(plain,result,115200));
 CHECK(dsr_maint_crypto_seal(&r,2,plain,115200,other,115200+144)==DSR_OK&&memcmp(sealed,other,115200+144));
 const unsigned offsets[]={0,12,28,60,92,93,96,100,112,128,144,115200+143};
 for(unsigned i=0;i<sizeof(offsets)/sizeof(*offsets);i++){
  sealed[offsets[i]]^=1;memset(result,0xcc,115200);written=123;
  CHECK(dsr_maint_crypto_open(&r,1,sealed,115200+144,result,115200,&written)!=DSR_OK&&written==0&&!memcmp(result,zeros,115200));sealed[offsets[i]]^=1;
 }
 r.key[0]^=1;CHECK(dsr_maint_crypto_open(&r,1,sealed,115200+144,result,115200,&written)==DSR_BAD_RESPONSE&&written==0&&!memcmp(result,zeros,115200));r.key[0]^=1;
 CHECK(dsr_maint_crypto_open(&r,2,sealed,115200+144,result,115200,&written)!=DSR_OK&&written==0);
 CHECK(dsr_maint_crypto_open(&r,1,sealed,115200+143,result,115200,&written)!=DSR_OK&&written==0);
 CHECK(dsr_maint_crypto_seal(&r,0,plain,500,sealed,115200+144)==DSR_OK);CHECK(dsr_maint_crypto_open(&r,0,sealed,644,result,115200,&written)==DSR_OK&&written==500&&!memcmp(plain,result,500));
 CHECK(dsr_maint_crypto_seal(&r,17,plain,500,sealed,115200+144)==DSR_INVALID);CHECK(dsr_maint_crypto_seal(&r,0,plain,499,sealed,115200+144)==DSR_INVALID);
 free(plain);free(result);free(zeros);free(sealed);free(other);printf("PASS: %u authenticated record checks; genuine OpenSSL GCM, SDK backend remains target evidence\n",checks);return 0;
}
