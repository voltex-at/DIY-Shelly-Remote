/* Host-only cross-implementation fixtures. Never part of an IDF component. */
#include "dsr_ota_manifest.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static unsigned checks,allocation_count,fail_at;
static bool allocation_failure;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
#ifdef DSR_OTA_HOST_TEST_TRUST
/* RFC 8032 test 1 public key. Its matching seed lives only in the host tool. */
const uint8_t *dsr_ota_trusted_key(const char *id)
{
 static const uint8_t key[32]={0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a};
 return !strcmp(id,"rfc8032-host-only")||!strcmp(id,"rfc8032-host-alias")?key:NULL;
}
#endif
static void *allocate(size_t size)
{unsigned n=allocation_count++;return allocation_failure&&n==fail_at?NULL:malloc(size);}
static uint32_t integer(FILE *file)
{uint8_t b[4];CHECK(fread(b,1,4,file)==4);return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];}
static bool zero(const dsr_ota_verified_t *out)
{const unsigned char *bytes=(const unsigned char *)out;for(size_t i=0;i<sizeof(*out);i++)if(bytes[i])return false;return true;}
int main(int argc,char **argv)
{
 CHECK(argc==2);FILE *file=fopen(argv[1],"rb");CHECK(file);unsigned cases=integer(file);
 unsigned char *valid=NULL;size_t valid_length=0;
 for(unsigned i=0;i<cases;i++){
  unsigned expected=integer(file),channel=integer(file),slot=integer(file),current_length=integer(file),length=integer(file);
  CHECK(current_length<64&&length<8192);char current[64];CHECK(fread(current,1,current_length,file)==current_length);current[current_length]=0;
  unsigned char *json=malloc(length+1);CHECK(json);CHECK(fread(json,1,length,file)==length);json[length]=0;
  dsr_ota_policy_t policy={current,(dsr_ota_channel_t)channel,slot};dsr_ota_verified_t out;memset(&out,0xa5,sizeof(out));
#ifndef DSR_OTA_HOST_TEST_TRUST
  if(expected==DSR_MANIFEST_OK||expected==DSR_MANIFEST_BAD_SIGNATURE)expected=DSR_MANIFEST_UNTRUSTED_KEY;
#endif
  dsr_manifest_result_t result=dsr_ota_manifest_verify(json,length,&policy,&out);
  if(result!=expected){fprintf(stderr,"case %u expected=%u actual=%u JSON=%.*s\n",i,expected,(unsigned)result,(int)length,json);abort();}
  CHECK(result==expected);
  if(result==DSR_MANIFEST_OK){CHECK(out.size==1234&&!strcmp(out.version,"1.2.4")&&out.channel==(dsr_ota_channel_t)channel);for(unsigned j=0;j<32;j++)CHECK(out.sha256[j]==j);CHECK(!strncmp(out.url,"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/",61));}
  else CHECK(zero(&out));
  if(i==0){valid=json;valid_length=length;}else free(json);
 }
 CHECK(fgetc(file)==EOF);fclose(file);CHECK(valid);
 dsr_ota_policy_t policy={"1.2.3",DSR_OTA_STABLE,0x600000};dsr_ota_verified_t out;
 memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_manifest_verify(NULL,valid_length,&policy,&out)==DSR_MANIFEST_INVALID&&zero(&out));
 memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_manifest_verify(valid,valid_length,NULL,&out)==DSR_MANIFEST_INVALID&&zero(&out));
 CHECK(dsr_ota_manifest_verify(valid,valid_length,&policy,NULL)==DSR_MANIFEST_INVALID);
 memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_manifest_verify(valid,0,&policy,&out)==DSR_MANIFEST_INVALID&&zero(&out));
 cJSON_Hooks hooks={allocate,free};cJSON_InitHooks(&hooks);allocation_failure=false;allocation_count=0;
 (void)dsr_ota_manifest_verify(valid,valid_length,&policy,&out);unsigned allocations=allocation_count;CHECK(allocations>10);
 for(unsigned i=0;i<allocations;i++){
  allocation_failure=true;fail_at=i;allocation_count=0;memset(&out,0xa5,sizeof(out));
  CHECK(dsr_ota_manifest_verify(valid,valid_length,&policy,&out)==DSR_MANIFEST_INVALID&&zero(&out));
 }
 cJSON_InitHooks(NULL);free(valid);
 printf("PASS: %u manifest checks; %u independent Python Ed25519 cases; %u allocation failures; trust=%s; no network/Flash\n",checks,cases,allocations,
#ifdef DSR_OTA_HOST_TEST_TRUST
 "HOST FIXTURE ONLY"
#else
 "PRODUCTION EMPTY"
#endif
 );return 0;
}
