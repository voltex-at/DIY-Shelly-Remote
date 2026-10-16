#include "dsr_ota_manifest.h"
#include "dsr_ota_trust.h"
#include "dsr_core.h"
#include "monocypher-ed25519.h"
#include "cJSON.h"
#include <stdbool.h>
#include <string.h>

static bool space(unsigned char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
/* A deliberately strict, flat ASCII JSON profile. No escapes (including
 * escaped aliases/NUL), arrays, literals, exponent/fraction or leading-zero
 * numbers. cJSON still handles actual JSON grammar and full consumption. */
static bool lexical(const unsigned char *s,size_t n)
{
 bool quoted=false;unsigned depth=0;
 for(size_t i=0;i<n;i++){
  unsigned char c=s[i];
  if(c=='"'){quoted=!quoted;continue;}
  if(c=='\\'||c>126||(!quoted&&!space(c)&&c<32)||!c)return false;
  if(quoted){if(c<32)return false;continue;}
  if(c=='{'){if(depth++)return false;continue;}
  if(c=='}'){if(depth!=1)return false;depth=0;continue;}
  if(space(c)||c==':'||c==',')continue;
  if(c<'0'||c>'9')return false;
  size_t start=i;while(i+1<n&&s[i+1]>='0'&&s[i+1]<='9')i++;
  if(i-start>=10||(s[start]=='0'&&i!=start))return false;
  if(i+1<n&&!space(s[i+1])&&s[i+1]!=','&&s[i+1]!='}')return false;
 }
 return !quoted&&!depth;
}
static int hex(unsigned char c)
{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static int b64(unsigned char c)
{return c>='A'&&c<='Z'?c-'A':c>='a'&&c<='z'?c-'a'+26:c>='0'&&c<='9'?c-'0'+52:c=='+'?62:c=='/'?63:-1;}
static bool signature_decode(const char *s,uint8_t out[64])
{
 if(strlen(s)!=88||s[86]!='='||s[87]!='=')return false;
 unsigned bits=0,value=0,written=0;
 for(unsigned i=0;i<86;i++){
  int v=b64((unsigned char)s[i]);if(v<0)return false;
  value=(value<<6)|(unsigned)v;bits+=6;
  if(bits>=8){bits-=8;if(written>=64)return false;out[written++]=(uint8_t)(value>>bits);}
 }
 return written==64&&bits==4&&(value&15)==0;
}
static bool token(const char *s,size_t maximum)
{
 size_t n=strlen(s);if(!n||n>maximum)return false;
 for(size_t i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='_'))return false;
 return true;
}
static bool release_url(const char *s)
{
 const char *prefix="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/";
 if(strncmp(s,prefix,strlen(prefix)))return false;
 const char *path=s+strlen(prefix),*slash=strchr(path,'/');
 if(!slash||slash==path||!slash[1]||strchr(slash+1,'/')||strstr(path,".."))return false;
 for(const char *p=path;*p;p++)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'||*p=='.'||*p=='/'))return false;
 return true;
}
static size_t signed_bytes(uint8_t out[1024],const char *const strings[7],uint32_t size,const uint8_t hash[32])
{
 static const char domain[]="DSR-OTA-MANIFEST";
 size_t n=sizeof(domain);memcpy(out,domain,n);out[n++]=2;
 for(unsigned i=0;i<7;i++){
  size_t len=strlen(strings[i]);out[n++]=(uint8_t)(len>>8);out[n++]=(uint8_t)len;
  memcpy(out+n,strings[i],len);n+=len;
 }
 for(int shift=56;shift>=0;shift-=8)out[n++]=(uint8_t)((uint64_t)size>>shift);
 memcpy(out+n,hash,32);return n+32;
}

dsr_manifest_result_t dsr_ota_manifest_verify(const void *input,size_t length,
 const dsr_ota_policy_t *policy,dsr_ota_verified_t *out)
{
 if(!out)return DSR_MANIFEST_INVALID;
 memset(out,0,sizeof(*out));
 if(!input||!length||length>DSR_OTA_MANIFEST_MAX||!policy||!policy->current_version||
    (policy->selected_channel!=DSR_OTA_STABLE&&policy->selected_channel!=DSR_OTA_DEV)||!policy->inactive_slot_size||policy->inactive_slot_size>0x600000)return DSR_MANIFEST_INVALID;
 uint32_t installed[3];if(!dsr_version_parse(policy->current_version,installed)||policy->current_version[0]=='v')return DSR_MANIFEST_POLICY;
 const unsigned char *bytes=input;if(!lexical(bytes,length))return DSR_MANIFEST_INVALID;
 /* cJSON does not distinguish parse failure from allocation failure. Both
  * fail closed, with zero output. Nesting was rejected before allocation. */
 const char *end=NULL;cJSON *root=cJSON_ParseWithLengthOpts(input,length,&end,false);
 if(!root)return DSR_MANIFEST_INVALID;
 dsr_manifest_result_t result=DSR_MANIFEST_INVALID;
 static const char *const keys[]={"schema_version","hardware_id","partition_layout","version","channel","url","size","sha256","key_id","signature_algorithm","signature"};
 const cJSON *fields[11]={0};unsigned seen=0;
 if(!cJSON_IsObject(root)||!end)goto done;
 for(const char *p=end;p<(const char *)input+length;p++)if(!space((unsigned char)*p))goto done;
 for(const cJSON *f=root->child;f;f=f->next){
  unsigned i;for(i=0;i<11;i++)if(f->string&&!strcmp(f->string,keys[i]))break;
  if(i==11||(seen&(1u<<i)))goto done;
  if(i==0||i==6){if(!cJSON_IsNumber(f))goto done;}else if(!cJSON_IsString(f)||!f->valuestring)goto done;
  seen|=1u<<i;fields[i]=f;
 }
 if(seen!=2047||fields[0]->valuedouble!=2||fields[6]->valuedouble<1||fields[6]->valuedouble>0x600000)goto done;
 const char *hardware=fields[1]->valuestring,*layout=fields[2]->valuestring,*version=fields[3]->valuestring,*channel=fields[4]->valuestring;
 const char *url=fields[5]->valuestring,*sha=fields[7]->valuestring,*id=fields[8]->valuestring,*algorithm=fields[9]->valuestring;
 if(strlen(hardware)>64||strlen(layout)>64||strlen(version)>31||strlen(url)>256||!token(id,64)||strcmp(algorithm,"ed25519"))goto done;
 dsr_ota_verified_t verified={0};verified.size=(uint32_t)fields[6]->valuedouble;
 if(strlen(sha)!=64||!signature_decode(fields[10]->valuestring,(uint8_t[64]){0}))goto done;
 for(unsigned i=0;i<32;i++){int hi=hex((unsigned char)sha[i*2]),lo=hex((unsigned char)sha[i*2+1]);if(hi<0||lo<0)goto done;verified.sha256[i]=(uint8_t)((hi<<4)|lo);}
 result=DSR_MANIFEST_POLICY;
 if(strcmp(hardware,DSR_OTA_HARDWARE_ID)||strcmp(layout,DSR_OTA_PARTITION_LAYOUT)||version[0]=='v'||
    strcmp(channel,policy->selected_channel==DSR_OTA_DEV?"dev":"stable")||!release_url(url))goto done;
 dsr_update_metadata_t meta={hardware,version,url,sha,verified.size};
 if(dsr_update_metadata_check(&meta,DSR_OTA_HARDWARE_ID,policy->current_version,policy->inactive_slot_size)!=DSR_OK)goto done;
 const uint8_t *key=dsr_ota_trusted_key(id);result=DSR_MANIFEST_UNTRUSTED_KEY;if(!key)goto done;
 uint8_t signature[64],message[1024];signature_decode(fields[10]->valuestring,signature);
 const char *strings[7]={hardware,layout,version,channel,url,id,algorithm};size_t n=signed_bytes(message,strings,verified.size,verified.sha256);
 result=DSR_MANIFEST_BAD_SIGNATURE;if(crypto_ed25519_check(signature,key,message,n))goto done;
 strcpy(verified.version,version);strcpy(verified.url,url);verified.channel=policy->selected_channel;*out=verified;result=DSR_MANIFEST_OK;
done:
 cJSON_Delete(root);return result;
}
