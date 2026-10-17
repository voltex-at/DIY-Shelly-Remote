#include "dsr_ota_release.h"
#include "dsr_ota_discovery.h"
#include "cJSON.h"
#include <stdbool.h>
#include <string.h>

#define SAFE_INTEGER UINT64_C(9007199254740991)
typedef struct {const unsigned char *p,*end;unsigned nodes;} scan_t;
static bool space(unsigned char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
static void whitespace(scan_t *s){while(s->p<s->end&&space(*s->p))s->p++;}
static bool take(scan_t *s,unsigned char c)
{if(s->p==s->end||*s->p!=c)return false;s->p++;return true;}
static int hex(unsigned char c)
{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static bool u16(scan_t *s,unsigned *value)
{
 if((size_t)(s->end-s->p)<4)return false;
 unsigned n=0;
 for(unsigned i=0;i<4;i++){int v=hex(*s->p++);if(v<0)return false;n=n*16+(unsigned)v;}
 *value=n;return true;
}
static bool utf8(scan_t *s,unsigned char lead)
{
 unsigned left,value,minimum;
 if(lead>=0xc2&&lead<=0xdf){left=1;value=lead&31;minimum=0x80;}
 else if(lead>=0xe0&&lead<=0xef){left=2;value=lead&15;minimum=0x800;}
 else if(lead>=0xf0&&lead<=0xf4){left=3;value=lead&7;minimum=0x10000;}
 else return false;
 if((size_t)(s->end-s->p)<left)return false;
 while(left--){unsigned char c=*s->p++;if((c&0xc0)!=0x80)return false;value=value*64+(c&63);}
 return value>=minimum&&value<=0x10ffff&&!(value>=0xd800&&value<=0xdfff);
}
static bool string(scan_t *s)
{
 if(!take(s,'"'))return false;
 while(s->p<s->end){
  unsigned char c=*s->p++;
  if(c=='"')return true;
  if(c<32)return false;
  if(c>=128){if(!utf8(s,c))return false;continue;}
  if(c!='\\')continue;
  if(s->p==s->end)return false;
  c=*s->p++;
  if(c=='"'||c=='\\'||c=='/'||c=='b'||c=='f'||c=='n'||c=='r'||c=='t')continue;
  if(c!='u')return false;
  unsigned value;if(!u16(s,&value)||!value)return false;
  if(value>=0xdc00&&value<=0xdfff)return false;
  if(value>=0xd800&&value<=0xdbff){
   if(!take(s,'\\')||!take(s,'u')||!u16(s,&value)||value<0xdc00||value>0xdfff)return false;
  }
 }
 return false;
}
static bool number(scan_t *s)
{
 if(s->p==s->end||*s->p<'0'||*s->p>'9')return false;
 if(*s->p=='0'){s->p++;return s->p==s->end||*s->p<'0'||*s->p>'9';}
 uint64_t n=0;
 while(s->p<s->end&&*s->p>='0'&&*s->p<='9'){
  unsigned digit=(unsigned)(*s->p++-'0');
  if(n>SAFE_INTEGER/10||(n==SAFE_INTEGER/10&&digit>SAFE_INTEGER%10))return false;
  n=n*10+digit;
 }
 return true;
}
static bool literal(scan_t *s,const char *word)
{size_t n=strlen(word);if((size_t)(s->end-s->p)<n||memcmp(s->p,word,n))return false;s->p+=n;return true;}
static bool value(scan_t *s,unsigned depth)
{
 whitespace(s);if(s->p==s->end||++s->nodes>DSR_OTA_RELEASE_MAX_NODES)return false;
 unsigned char c=*s->p;
 if(c=='"')return string(s);
 if(c=='{'||c=='['){
  if(depth>=DSR_OTA_RELEASE_MAX_DEPTH)return false;
  bool object=c=='{';unsigned char end=object?'}':']';s->p++;whitespace(s);
  if(take(s,end))return true;
  for(;;){
   if(object){if(!string(s))return false;whitespace(s);if(!take(s,':'))return false;}
   if(!value(s,depth+1))return false;
   whitespace(s);if(take(s,end))return true;
   if(!take(s,','))return false;
   whitespace(s);
  }
 }
 if(c=='t')return literal(s,"true");
 if(c=='f')return literal(s,"false");
 if(c=='n')return literal(s,"null");
 return number(s);
}
static bool grammar(const void *input,size_t length)
{scan_t s={input,(const unsigned char *)input+length,0};if(!value(&s,0))return false;whitespace(&s);return s.p==s.end;}
static bool unique(const cJSON *node,unsigned depth)
{
 if(depth>DSR_OTA_RELEASE_MAX_DEPTH)return false;
 bool object=cJSON_IsObject(node);
 for(const cJSON *a=node->child;a;a=a->next){
  if(object){
   if(!a->string)return false;
   for(const cJSON *b=a->next;b;b=b->next)if(b->string&&!strcmp(a->string,b->string))return false;
  }
  if(!unique(a,depth+1))return false;
 }
 return true;
}
static size_t bounded_length(const char *s,size_t max)
{if(!s)return max+1;size_t n=0;while(n<=max&&s[n])n++;return n;}
static bool segment(const char *s,size_t max)
{
 size_t n=bounded_length(s,max);if(!n||n>max||!strcmp(s,".")||strstr(s,".."))return false;
 for(size_t i=0;i<n;i++){unsigned char c=(unsigned char)s[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))return false;}
 return true;
}
static bool api_url(const char *url,const cJSON *id)
{
 static const char prefix[]="https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/";
 if(!url||bounded_length(url,128)>128||strncmp(url,prefix,sizeof(prefix)-1)||!cJSON_IsNumber(id)||id->valuedouble<1)return false;
 const char *p=url+sizeof(prefix)-1;if(*p<'1'||*p>'9')return false;
 uint64_t n=0;
 for(;*p;p++){
  if(*p<'0'||*p>'9')return false;
  unsigned digit=(unsigned)(*p-'0');
  if(n>SAFE_INTEGER/10||(n==SAFE_INTEGER/10&&digit>SAFE_INTEGER%10))return false;
  n=n*10+digit;
 }
 return (double)n==id->valuedouble;
}
static bool download_url(const char *url,const dsr_ota_release_policy_t *p)
{
 static const char prefix[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/";
 if(!url||bounded_length(url,256)>256||strncmp(url,prefix,sizeof(prefix)-1))return false;
 const char *path=url+sizeof(prefix)-1;size_t tag=strlen(p->expected_tag);
 /* Check remaining length first: never index beyond a short decoded string. */
 if(strlen(path)!=tag+1+strlen(p->manifest_asset)||strncmp(path,p->expected_tag,tag)||path[tag]!='/')return false;
 return !strcmp(path+tag+1,p->manifest_asset);
}
static const cJSON *field(const cJSON *node,const char *key)
{return cJSON_GetObjectItemCaseSensitive(node,key);}
static const char *text(const cJSON *node)
{return cJSON_IsString(node)?node->valuestring:NULL;}
dsr_ota_release_result_t dsr_ota_release_policy_check(const dsr_ota_release_policy_t *policy)
{
 if(!policy||!segment(policy->expected_tag,64)||!segment(policy->manifest_asset,96))return DSR_RELEASE_ARGUMENT;
 return policy->selected_channel==DSR_OTA_STABLE?DSR_RELEASE_OK:DSR_RELEASE_POLICY;
}
static dsr_ota_release_result_t candidate_root(const cJSON *root,
 const dsr_ota_release_policy_t *policy,dsr_ota_release_candidate_t *out)
{
 dsr_ota_release_result_t result=DSR_RELEASE_POLICY;
 const cJSON *draft=field(root,"draft"),*prerelease=field(root,"prerelease"),*assets=field(root,"assets");
 const char *tag=text(field(root,"tag_name")),*url=text(field(root,"url"));
 const cJSON *id=field(root,"id");
 if(!cJSON_IsBool(draft)||!cJSON_IsBool(prerelease)||!cJSON_IsArray(assets)||!tag||!url||!cJSON_IsNumber(id))return DSR_RELEASE_INVALID;

 if(!cJSON_IsFalse(draft)||(policy->selected_channel==DSR_OTA_DEV?!cJSON_IsTrue(prerelease):!cJSON_IsFalse(prerelease))||strcmp(tag,policy->expected_tag)||!api_url(url,id))return result;
 const cJSON *selected=NULL;unsigned count=0,matches=0;
 result=DSR_RELEASE_INVALID;
 for(const cJSON *asset=assets->child;asset;asset=asset->next){
  if(++count>DSR_OTA_RELEASE_MAX_ASSETS||!cJSON_IsObject(asset))return result;
  const char *name=text(field(asset,"name"));if(!name)return result;
  if(!strcmp(name,policy->manifest_asset)){selected=asset;matches++;}
 }
 if(matches!=1){result=matches?DSR_RELEASE_AMBIGUOUS:DSR_RELEASE_NOT_FOUND;return result;}
 const char *state=text(field(selected,"state")),*download=text(field(selected,"browser_download_url"));
 const cJSON *size=field(selected,"size");
 if(!state||!download||!cJSON_IsNumber(size))return result;
 result=DSR_RELEASE_POLICY;
 if(strcmp(state,"uploaded")||size->valuedouble<1||size->valuedouble>DSR_OTA_MANIFEST_MAX||!download_url(download,policy))return result;
 strcpy(out->tag,tag);strcpy(out->manifest_url,download);out->manifest_size=(uint32_t)size->valuedouble;return DSR_RELEASE_OK;
}
dsr_ota_release_result_t dsr_ota_release_candidate_parse(const void *input,size_t length,
 const dsr_ota_release_policy_t *policy,dsr_ota_release_candidate_t *out)
{
 if(!out)return DSR_RELEASE_ARGUMENT;
 memset(out,0,sizeof(*out));
 if(!input||!length||length>DSR_OTA_RELEASE_MAX)return DSR_RELEASE_ARGUMENT;
 dsr_ota_release_result_t checked=dsr_ota_release_policy_check(policy);
 if(checked!=DSR_RELEASE_OK)return checked;
 if(!grammar(input,length))return DSR_RELEASE_INVALID;
 const char *end=NULL;cJSON *root=cJSON_ParseWithLengthOpts(input,length,&end,false);
 if(!root)return DSR_RELEASE_INVALID;
 dsr_ota_release_result_t result=DSR_RELEASE_INVALID;
 if(!cJSON_IsObject(root)||!end||!unique(root,0))goto done;
 for(const char *p=end;p<(const char *)input+length;p++)if(!space((unsigned char)*p))goto done;
 result=candidate_root(root,policy,out);
done:
 cJSON_Delete(root);return result;
}

/* Same strict grammar, decoded-key uniqueness and source identity as the
 * explicit-tag API above. The new native catalogue is a separate policy. */
static void plain_notes(const cJSON *body,char out[DSR_OTA_NATIVE_NOTES_MAX+1])
{
 size_t used=0;
 const unsigned char *p=(const unsigned char *)(cJSON_IsString(body)?body->valuestring:"");
 while(*p){
  unsigned char c=*p;size_t n=c<0x80?1:c<0xe0?2:c<0xf0?3:4;
  if((c<32&&c!='\n'&&c!='\t')||c==127){p++;continue;}
  if(used+n>DSR_OTA_NATIVE_NOTES_MAX)break;
  if(c=='\t'){out[used++]=' ';p++;continue;}
  memcpy(out+used,p,n);used+=n;p+=n;
 }
 out[used]=0;
}
dsr_ota_release_result_t dsr_ota_discovery_parse(const void *input,size_t length,
 dsr_ota_channel_t channel,dsr_ota_discovery_t *out,unsigned *entries)
{
 if(entries)*entries=0;
 if(!out)return DSR_RELEASE_ARGUMENT;
 memset(out,0,sizeof(*out));
 if(!input||!length||length>DSR_OTA_RELEASE_MAX||(channel!=DSR_OTA_STABLE&&channel!=DSR_OTA_DEV))return DSR_RELEASE_ARGUMENT;
 if(!grammar(input,length))return DSR_RELEASE_INVALID;
 const char *end=NULL;cJSON *root=cJSON_ParseWithLengthOpts(input,length,&end,false);
 if(!root)return DSR_RELEASE_INVALID;
 dsr_ota_release_result_t result=DSR_RELEASE_INVALID;
 if(!end||!unique(root,0))goto done;
 for(const char *p=end;p<(const char *)input+length;p++)if(!space((unsigned char)*p))goto done;
 const cJSON *selected=NULL;
 if(channel==DSR_OTA_STABLE){if(!cJSON_IsObject(root))goto done;selected=root;}
 else{
  if(!cJSON_IsArray(root)||cJSON_GetArraySize(root)>5)goto done;
  if(entries)*entries=(unsigned)cJSON_GetArraySize(root);
  const cJSON *entry;
  cJSON_ArrayForEach(entry,root){
   if(!cJSON_IsObject(entry)||!cJSON_IsBool(field(entry,"draft"))||!cJSON_IsBool(field(entry,"prerelease")))goto done;
   if(!selected&&cJSON_IsFalse(field(entry,"draft"))&&cJSON_IsTrue(field(entry,"prerelease")))selected=entry;
  }
 }
 if(!selected){result=DSR_RELEASE_NOT_FOUND;goto done;}
 const char *tag=text(field(selected,"tag_name"));
 if(!segment(tag,64)){result=DSR_RELEASE_POLICY;goto done;}
 dsr_ota_release_policy_t policy={.expected_tag=tag,.manifest_asset=DSR_OTA_NATIVE_MANIFEST_ASSET,.selected_channel=channel};
 result=candidate_root(selected,&policy,&out->candidate);
 if(result==DSR_RELEASE_OK){const cJSON *body=field(selected,"body");if(body&&!cJSON_IsNull(body)&&!cJSON_IsString(body))result=DSR_RELEASE_INVALID;else plain_notes(body,out->notes);}
done:
 cJSON_Delete(root);if(result!=DSR_RELEASE_OK)memset(out,0,sizeof(*out));return result;
}
