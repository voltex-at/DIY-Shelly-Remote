/* Independent Python stdlib JSON corpus drives the actual production parser. */
#include "dsr_ota_release.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
static unsigned checks,calls,fail_at,live;static bool fault;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static void *allocate(size_t n){unsigned at=calls++;if(fault&&at==fail_at)return NULL;void *p=malloc(n);if(p)live++;return p;}
static void release(void *p){if(p){CHECK(live);live--;}free(p);}
static uint32_t integer(FILE *f){uint8_t b[4];CHECK(fread(b,1,4,f)==4);return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static bool zero(const dsr_ota_release_candidate_t *out){const unsigned char *p=(const unsigned char *)out;for(size_t i=0;i<sizeof(*out);i++)if(p[i])return false;return true;}
int main(int argc,char **argv)
{
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);unsigned count=integer(f);unsigned char *baseline=NULL;size_t baseline_length=0;
 cJSON_Hooks hooks={allocate,release};cJSON_InitHooks(&hooks);
 for(unsigned i=0;i<count;i++){
  int expected=(int32_t)integer(f);uint32_t expected_size=integer(f),tag_length=integer(f),asset_length=integer(f),channel=integer(f),length=integer(f);
  CHECK(tag_length<128&&asset_length<128&&length<=20000);char tag[128],asset[128];CHECK(fread(tag,1,tag_length,f)==tag_length&&fread(asset,1,asset_length,f)==asset_length);tag[tag_length]=asset[asset_length]=0;
  /* Exact-sized allocation intentionally supplies NO trailing NUL. ASan CI
   * can therefore detect a parser reading beyond the explicit input length. */
  unsigned char *json=malloc(length?length:1);CHECK(json);CHECK(fread(json,1,length,f)==length);
  dsr_ota_release_policy_t policy={tag,asset,(dsr_ota_channel_t)channel};dsr_ota_release_candidate_t out;memset(&out,0xa5,sizeof(out));
  dsr_ota_release_result_t result=dsr_ota_release_candidate_parse(json,length,&policy,&out);
  if(expected>=0&&result!=(dsr_ota_release_result_t)expected){fprintf(stderr,"case %u expected=%d got=%d JSON=%.*s\n",i,expected,result,(int)length,json);abort();}
  if(expected<0)CHECK(result!=DSR_RELEASE_OK);else CHECK(result==(dsr_ota_release_result_t)expected);
  if(result==DSR_RELEASE_OK){char url[512];snprintf(url,sizeof(url),"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/%s/%s",tag,asset);CHECK(!strcmp(out.tag,tag)&&!strcmp(out.manifest_url,url)&&out.manifest_size==expected_size);}
  else{CHECK(zero(&out));}
  CHECK(!live);
  if(i==0){baseline=json;baseline_length=length;}else free(json);
 }
 CHECK(fgetc(f)==EOF);fclose(f);CHECK(baseline);
 dsr_ota_release_policy_t policy={"v1.2.4","manifest.json",DSR_OTA_STABLE};dsr_ota_release_candidate_t out;
 calls=0;CHECK(dsr_ota_release_candidate_parse(baseline,baseline_length,&policy,&out)==DSR_RELEASE_OK);unsigned total=calls;CHECK(total>10);
 for(unsigned i=0;i<total;i++){fault=true;fail_at=i;calls=0;memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_release_candidate_parse(baseline,baseline_length,&policy,&out)==DSR_RELEASE_INVALID&&zero(&out)&&!live);}
 fault=false;cJSON_InitHooks(NULL);
 memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_release_candidate_parse(NULL,baseline_length,&policy,&out)==DSR_RELEASE_ARGUMENT&&zero(&out));
 memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_release_candidate_parse(baseline,baseline_length,NULL,&out)==DSR_RELEASE_ARGUMENT&&zero(&out));
 CHECK(dsr_ota_release_candidate_parse(baseline,baseline_length,&policy,NULL)==DSR_RELEASE_ARGUMENT);
 free(baseline);printf("PASS: %u release-candidate checks; %u independent JSON/oracle cases incl seeded mutations; %u real cJSON allocation failures; no I/O/trust/Flash\n",checks,count,total);return 0;
}
