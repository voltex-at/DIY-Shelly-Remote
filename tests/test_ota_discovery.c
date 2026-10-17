/* Python-authored cases exercise the actual native catalogue parser. Pure
 * untrusted metadata only: no network, publisher trust or installation proof. */
#include "dsr_ota_discovery.h"
#include "cJSON.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,calls,fail_at,live;static bool fault;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static void *allocate(size_t size){unsigned call=calls++;if(fault&&call==fail_at)return NULL;void *p=malloc(size);if(p)live++;return p;}
static void release(void *p){if(p){CHECK(live);live--;}free(p);}
static uint32_t integer(FILE *f){uint8_t b[4];CHECK(fread(b,1,4,f)==4);return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static bool zero(const dsr_ota_discovery_t *d){const uint8_t *p=(const uint8_t *)d;for(size_t i=0;i<sizeof(*d);i++)if(p[i])return false;return true;}
int main(int argc,char **argv)
{
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);unsigned count=integer(f);
 cJSON_Hooks hooks={allocate,release};cJSON_InitHooks(&hooks);unsigned oom=0;
 for(unsigned i=0;i<count;i++){
  int expected=(int32_t)integer(f);unsigned channel=integer(f),length=integer(f),tag_size=integer(f),notes_size=integer(f);CHECK(length<=20000&&tag_size<65&&notes_size<=320);
  uint8_t *raw=malloc(length?length:1);CHECK(raw&&fread(raw,1,length,f)==length);char tag[65],notes[321];CHECK(fread(tag,1,tag_size,f)==tag_size&&fread(notes,1,notes_size,f)==notes_size);tag[tag_size]=notes[notes_size]=0;
  dsr_ota_discovery_t out;memset(&out,0xa5,sizeof(out));calls=0;unsigned entries=99;
  dsr_ota_release_result_t result=dsr_ota_discovery_parse(raw,length,(dsr_ota_channel_t)channel,&out,&entries);CHECK(entries<=5);unsigned total=calls;
  if((expected<0&&result==DSR_RELEASE_OK)||(expected>=0&&result!=(dsr_ota_release_result_t)expected)){fprintf(stderr,"case%u expected%d got%d\n",i,expected,result);abort();}
  CHECK(expected<0?result!=DSR_RELEASE_OK:result==(dsr_ota_release_result_t)expected);
  if(result==DSR_RELEASE_OK){char url[400];snprintf(url,sizeof(url),"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/%s/%s",tag,DSR_OTA_NATIVE_MANIFEST_ASSET);CHECK(!strcmp(out.candidate.tag,tag)&&!strcmp(out.candidate.manifest_url,url)&&out.candidate.manifest_size==700&&!strcmp(out.notes,notes));}
  else {CHECK(zero(&out));}
  CHECK(!live);
  if(i<2){CHECK(result==DSR_RELEASE_OK&&total>10);for(unsigned at=0;at<total;at++){fault=true;fail_at=at;calls=0;memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_discovery_parse(raw,length,(dsr_ota_channel_t)channel,&out,NULL)==DSR_RELEASE_INVALID&&zero(&out)&&!live);oom++;}fault=false;}
  free(raw);
 }
 CHECK(fgetc(f)==EOF);fclose(f);dsr_ota_discovery_t out;memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_discovery_parse(NULL,1,DSR_OTA_STABLE,&out,NULL)==DSR_RELEASE_ARGUMENT&&zero(&out));CHECK(dsr_ota_discovery_parse("{}",2,DSR_OTA_STABLE,NULL,NULL)==DSR_RELEASE_ARGUMENT);CHECK(!live);cJSON_InitHooks(NULL);
 printf("PASS: %u actual discovery parser checks; %u explicit channel/selection/notes negatives, %u real allocation faults; no I/O/trust/Flash\n",checks,count,oom);return 0;
}
