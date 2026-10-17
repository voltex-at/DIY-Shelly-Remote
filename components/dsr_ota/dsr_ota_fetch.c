#include "dsr_ota_fetch.h"
#include "dsr_ota_fetch_private.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
 char url[DSR_OTA_FETCH_URL_MAX+1],location[DSR_OTA_FETCH_URL_MAX+1];
 unsigned char body[DSR_OTA_MANIFEST_MAX+1];
 esp_http_client_config_t config;
 const dsr_ota_fetch_control_t *control;
 int64_t started;
 uint64_t declared;
 size_t used,headers,count;
 dsr_ota_fetch_result_t result;
 bool length_seen,transfer_seen,encoding_seen,location_seen;
} fetch_t;
_Static_assert(sizeof(fetch_t)<=8192,"bounded manifest-owned workspace");
static size_t bounded_length(const char *s,size_t max)
{size_t n=0;if(!s)return max+1;while(n<=max&&s[n])n++;return n;}
static bool alnum(char c)
{return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9');}
static int digit(char c)
{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static bool initial_url(const char *url)
{
 static const char prefix[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/";
 size_t n=bounded_length(url,256);if(n>256||n<=sizeof(prefix)-1||strncmp(url,prefix,sizeof(prefix)-1))return false;
 const char *p=url+sizeof(prefix)-1,*slash=strchr(p,'/');
 if(!slash||slash==p||!slash[1]||strchr(slash+1,'/')||strstr(p,".."))return false;
 for(;*p;p++)if(!alnum(*p)&&*p!='-'&&*p!='_'&&*p!='.'&&*p!='/')return false;
 return true;
}
static bool asset_url(const char *url)
{
 /* GitHub serves release assets from either CDN host (observed 2026-10-17:
  * objects.githubusercontent.com). The host is not a trust anchor: the
  * signed manifest pins size and SHA-256 of every byte that is installed. */
 static const char *const prefixes[]={"https://release-assets.githubusercontent.com/","https://objects.githubusercontent.com/"};
 size_t n=bounded_length(url,DSR_OTA_FETCH_URL_MAX),prefix=0;
 if(n>DSR_OTA_FETCH_URL_MAX)return false;
 for(unsigned i=0;i<2&&!prefix;i++){size_t k=strlen(prefixes[i]);if(n>k&&!strncmp(url,prefixes[i],k))prefix=k;}
 if(!prefix)return false;
 const char *p=url+prefix,*path=p;
 for(;*p&&*p!='?';p++){
  if(!alnum(*p)&&*p!='-'&&*p!='_'&&*p!='.'&&*p!='/')return false;
  if((*p=='.'&&p[1]=='.')||(*p=='/'&&(p==path||p[1]=='/')))return false;
 }
 if(p==path||p[-1]=='/')return false;
 if(*p=='?'){
  if(!*++p)return false;
  for(;*p;p++){
   unsigned char c=(unsigned char)*p;
   if(c<33||c>126||c=='#'||c=='\\'||c=='"'||c=='<'||c=='>')return false;
   if(c=='%'){
    if(!p[1]||!p[2])return false;
    int hi=digit(p[1]),lo=digit(p[2]);if(hi<0||lo<0)return false;
    int decoded=hi*16+lo;if(decoded<32||decoded==127||decoded=='\\')return false;
    p+=2;
   }
  }
 }
 return true;
}
static bool metadata_url(const char *url)
{
 static const char prefix[]="https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/tags/";
 size_t n=bounded_length(url,sizeof(prefix)-1+64);
 if(n>sizeof(prefix)-1+64||n<=sizeof(prefix)-1||strncmp(url,prefix,sizeof(prefix)-1))return false;
 const char *p=url+sizeof(prefix)-1;
 if(!strcmp(p,".")||strstr(p,".."))return false;
 for(;*p;p++)if(!alnum(*p)&&*p!='-'&&*p!='_'&&*p!='.')return false;
 return true;
}
static const char stable_discovery[]="https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/latest";
/* DEV walks one release per page so each response stays far below the
 * 16 KiB body limit however many releases and assets the repository has. */
static const char *const dev_discovery[DSR_OTA_DEV_PAGES]={
 "https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases?per_page=1&page=1",
 "https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases?per_page=1&page=2",
 "https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases?per_page=1&page=3",
 "https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases?per_page=1&page=4",
 "https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases?per_page=1&page=5"};
_Static_assert(DSR_OTA_DEV_PAGES==5,"one URL per DEV page");
static bool discovery_url(const char *url)
{
 if(!url)return false;
 if(!strcmp(url,stable_discovery))return true;
 for(unsigned i=0;i<DSR_OTA_DEV_PAGES;i++)if(!strcmp(url,dev_discovery[i]))return true;
 return false;
}
bool dsr_ota_fetch_policy_valid(const dsr_ota_policy_t *policy)
{
 return policy&&policy->current_version&&bounded_length(policy->current_version,31)<=31
  &&(policy->selected_channel==DSR_OTA_STABLE||policy->selected_channel==DSR_OTA_DEV)
  &&policy->inactive_slot_size&&policy->inactive_slot_size<=0x600000;
}
static bool same(const char *s,size_t n,const char *word)
{
 if(strlen(word)!=n)return false;
 for(size_t i=0;i<n;i++){char c=s[i];if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');if(c!=word[i])return false;}
 return true;
}
static bool stopped(fetch_t *s)
{
 if(s->result!=DSR_FETCH_OK)return true;
 if(s->control&&s->control->cancelled&&s->control->cancelled(s->control->context))s->result=DSR_FETCH_CANCELLED;
 int64_t now=esp_timer_get_time();
 if(s->result==DSR_FETCH_OK&&(now<s->started||now-s->started>=DSR_OTA_FETCH_BUDGET_US))s->result=DSR_FETCH_TIMEOUT;
 return s->result!=DSR_FETCH_OK;
}
static esp_err_t on_event(esp_http_client_event_t *event)
{
 fetch_t *s=event->user_data;if(!s)return ESP_FAIL;
 if(stopped(s))return ESP_FAIL;
 if(event->event_id==HTTP_EVENT_ERROR){s->result=DSR_FETCH_IO;return ESP_FAIL;}
 if(event->event_id!=HTTP_EVENT_ON_HEADER)return ESP_OK;
 size_t key=bounded_length(event->header_key,128),n=bounded_length(event->header_value,8192);
 if(!key||key>128||n>8192||++s->count>64||key+n>16384-s->headers)goto bad;
 s->headers+=key+n;
 const char *value=event->header_value;
 while(n&&(*value==' '||*value=='\t')){value++;n--;}
 while(n&&(value[n-1]==' '||value[n-1]=='\t'))n--;
 if(same(event->header_key,key,"content-length")){
  if(s->length_seen||s->transfer_seen||!n||n>20||(n>1&&value[0]=='0'))goto bad;
  uint64_t length=0;
  for(size_t i=0;i<n;i++){
   if(value[i]<'0'||value[i]>'9'||length>(UINT64_MAX-(unsigned)(value[i]-'0'))/10)goto bad;
   length=length*10+(unsigned)(value[i]-'0');
  }
  s->declared=length;s->length_seen=true;
 }else if(same(event->header_key,key,"transfer-encoding")){
  if(s->transfer_seen||s->length_seen||!same(value,n,"chunked"))goto bad;
  s->transfer_seen=true;
 }else if(same(event->header_key,key,"content-encoding")){
  if(s->encoding_seen||!same(value,n,"identity"))goto bad;
  s->encoding_seen=true;
 }else if(same(event->header_key,key,"location")){
  if(s->location_seen||!n||n>DSR_OTA_FETCH_URL_MAX)goto bad;
  memcpy(s->location,value,n);s->location[n]=0;s->location_seen=true;
 }
 return ESP_OK;
bad:
 /* IDF ignores this callback's result at header dispatch; latch the failure. */
 s->result=DSR_FETCH_HEADERS;return ESP_FAIL;
}
static dsr_ota_fetch_result_t fetch(const char *url,const dsr_ota_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,dsr_manifest_result_t *why,
 dsr_ota_image_t *image,dsr_image_result_t *image_why,
 unsigned char *metadata,size_t *metadata_length,size_t expected_manifest_size,
 const dsr_ota_stream_sink_t *sink,bool discovery,int *http_status)
{
 bool release=metadata!=NULL;
 if(why)*why=DSR_MANIFEST_INVALID;
 if(release){if(!metadata_length)return DSR_FETCH_ARGUMENT;*metadata_length=0;}
 else{
 if(!out)return DSR_FETCH_ARGUMENT;
 if(!image)memset(out,0,sizeof(*out));
 if(!dsr_ota_fetch_policy_valid(policy)||expected_manifest_size>DSR_OTA_MANIFEST_MAX)return DSR_FETCH_ARGUMENT;
 }
 if(release?(discovery?!discovery_url(url):!metadata_url(url)):!initial_url(url))return DSR_FETCH_URL;
 if(time(NULL)<(time_t)1735689600)return DSR_FETCH_CLOCK;
 size_t limit=release?16384u:image?out->size:expected_manifest_size?expected_manifest_size:DSR_OTA_MANIFEST_MAX;
 fetch_t *s=calloc(1,sizeof(*s));if(!s)return DSR_FETCH_NOMEM;
 s->control=control;s->started=esp_timer_get_time();strcpy(s->url,url);
 for(unsigned hop=0;hop<2;hop++){
  if(stopped(s))break;
  s->used=s->headers=s->count=0;s->declared=0;
  s->length_seen=s->transfer_seen=s->encoding_seen=s->location_seen=false;s->location[0]=0;
  memset(&s->config,0,sizeof(s->config));
  s->config.url=s->url;s->config.method=HTTP_METHOD_GET;
  s->config.transport_type=HTTP_TRANSPORT_OVER_SSL;s->config.crt_bundle_attach=esp_crt_bundle_attach;
  s->config.skip_cert_common_name_check=false;s->config.common_name=NULL;
  s->config.disable_auto_redirect=true;s->config.max_authorization_retries=-1;
  s->config.timeout_ms=DSR_OTA_FETCH_IO_TIMEOUT_MS;s->config.buffer_size=1024;s->config.buffer_size_tx=4096;
  s->config.user_agent="DIY-Shelly-Remote-manifest/1";s->config.event_handler=on_event;s->config.user_data=s;
  esp_http_client_handle_t client=esp_http_client_init(&s->config);
  if(!client){s->result=DSR_FETCH_NOMEM;break;}
  bool redirect=false;
  if(!stopped(s)&&esp_http_client_set_header(client,"Accept-Encoding","identity")!=ESP_OK)s->result=DSR_FETCH_IO;
  if(!stopped(s)&&esp_http_client_set_header(client,"Accept",release?"application/vnd.github+json":image?"application/octet-stream":"application/json")!=ESP_OK)s->result=DSR_FETCH_IO;
  if(release&&!stopped(s)&&esp_http_client_set_header(client,"X-GitHub-Api-Version","2022-11-28")!=ESP_OK)s->result=DSR_FETCH_IO;
  if(!stopped(s)&&esp_http_client_open(client,0)!=ESP_OK&&s->result==DSR_FETCH_OK)s->result=DSR_FETCH_IO;
  if(!stopped(s)){
   int64_t length=esp_http_client_fetch_headers(client);
   if(http_status)*http_status=esp_http_client_get_status_code(client);
   if(!stopped(s)){
    int status=esp_http_client_get_status_code(client);
    if(length<0)s->result=DSR_FETCH_IO;
    else if(status==301||status==302||status==303||status==307||status==308){
     if(release||hop||!s->location_seen||!asset_url(s->location))s->result=DSR_FETCH_REDIRECT;
     else redirect=true;
    }else if(status!=200)s->result=DSR_FETCH_STATUS;
    else if(s->location_seen||(!s->length_seen&&!s->transfer_seen))s->result=DSR_FETCH_HEADERS;
    else if(s->length_seen&&(!s->declared||s->declared>limit||((image||expected_manifest_size)&&s->declared!=limit)))s->result=DSR_FETCH_SIZE;
    else if((s->length_seen&&(length!=(int64_t)s->declared||esp_http_client_get_content_length(client)!=length||esp_http_client_is_chunked_response(client)))
      ||(s->transfer_seen&&(length!=0||!esp_http_client_is_chunked_response(client))))s->result=DSR_FETCH_HEADERS;
    else{
     /* Drain cached bytes even if headers already completed the response.
      * App uses the same heap buffer repeatedly; no whole-image allocation.
      * SDK read may wait for its requested bytes despite positive progress,
      * hence the budget is explicitly cooperative, not a hard deadline. */
     while(!stopped(s)){
      size_t wanted=(image||release)?limit-s->used+1:1;
      if(wanted>1024)wanted=1024;
      char *buffer=release?(char *)metadata+s->used:(char *)s->body+(image?0:s->used);
      int n=esp_http_client_read(client,buffer,(int)wanted);
      if(stopped(s))break;
      if(n<0){s->result=DSR_FETCH_IO;break;}
      if((size_t)n>wanted){s->result=DSR_FETCH_SIZE;break;}
      if(!n){
       if(!esp_http_client_is_complete_data_received(client)||(s->length_seen&&s->used!=s->declared))s->result=DSR_FETCH_INCOMPLETE;
       else if(!s->used)s->result=DSR_FETCH_SIZE;
       break;
      }
      if((size_t)n>limit-s->used){s->result=DSR_FETCH_SIZE;break;}
      if(image){
       dsr_image_result_t result=dsr_ota_image_update(image,buffer,(size_t)n);
       if(image_why)*image_why=result;
       if(result!=DSR_IMAGE_OK){s->result=DSR_FETCH_IMAGE;break;}
       if(sink&&!sink->write(sink->context,buffer,(size_t)n,dsr_ota_image_header_ready(image))){s->result=DSR_FETCH_IO;break;}
      }
      s->used+=(size_t)n;
      if(s->length_seen&&s->used>s->declared){s->result=DSR_FETCH_SIZE;break;}
     }
    }
   }
  }
  esp_err_t closed=esp_http_client_close(client),cleaned=esp_http_client_cleanup(client);
  if((closed!=ESP_OK||cleaned!=ESP_OK)&&s->result==DSR_FETCH_OK)s->result=DSR_FETCH_IO;
  if(stopped(s))break;
  if(redirect){strcpy(s->url,s->location);continue;}
  /* SDK-visible chunk trailers arrive after the initial header checks.
   * IDF does not dispatch its final pending trailer at message-complete;
   * this check covers delivered events, not all raw trailer bytes. */
  if(s->location_seen){s->result=DSR_FETCH_HEADERS;break;}
  if(release){*metadata_length=s->used;}
  else if(!image&&expected_manifest_size&&s->used!=expected_manifest_size){s->result=DSR_FETCH_INCOMPLETE;}
  else if(image){
   dsr_image_result_t result=dsr_ota_image_finish(image);
   if(image_why)*image_why=result;
   if(result!=DSR_IMAGE_OK)s->result=DSR_FETCH_IMAGE;
   else (void)stopped(s);
  }else{
   dsr_manifest_result_t result=dsr_ota_manifest_verify(s->body,s->used,policy,out);
   if(why)*why=result;
   if(result!=DSR_MANIFEST_OK)s->result=DSR_FETCH_MANIFEST;
   else (void)stopped(s);
  }
  break;
 }
 dsr_ota_fetch_result_t result=s->result;
 if(result!=DSR_FETCH_OK){if(release)*metadata_length=0;else memset(out,0,sizeof(*out));}
 free(s);return result;
}
dsr_ota_fetch_result_t dsr_ota_fetch_manifest(const char *url,const dsr_ota_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,dsr_manifest_result_t *why)
{return fetch(url,policy,control,out,why,NULL,NULL,NULL,NULL,0,NULL,false,NULL);}

dsr_ota_fetch_result_t dsr_ota_fetch_metadata_body(const char *url,
 const dsr_ota_fetch_control_t *control,unsigned char *body,size_t *length)
{
 if(length)*length=0;
 if(!body||!length)return DSR_FETCH_ARGUMENT;
 return fetch(url,NULL,control,NULL,NULL,NULL,NULL,body,length,0,NULL,false,NULL);
}
dsr_ota_fetch_result_t dsr_ota_fetch_discovery_page(dsr_ota_channel_t channel,unsigned page,
 const dsr_ota_fetch_control_t *control,unsigned char *body,size_t *length,int *http_status)
{
 if(length)*length=0;
 if(http_status)*http_status=0;
 if(!body||!length||(channel!=DSR_OTA_STABLE&&channel!=DSR_OTA_DEV))return DSR_FETCH_ARGUMENT;
 if(page<1||page>(channel==DSR_OTA_STABLE?1u:DSR_OTA_DEV_PAGES))return DSR_FETCH_ARGUMENT;
 return fetch(channel==DSR_OTA_STABLE?stable_discovery:dev_discovery[page-1],NULL,control,
  NULL,NULL,NULL,NULL,body,length,0,NULL,true,http_status);
}

typedef struct {dsr_ota_verified_t verified;dsr_ota_image_t *image;} app_chain_t;
_Static_assert(sizeof(app_chain_t)<=512,"bounded app-chain workspace");
static bool same_consent(const dsr_ota_verified_t *a,const dsr_ota_verified_t *b)
{
 return memchr(a->version,0,sizeof(a->version))&&memchr(a->url,0,sizeof(a->url))&&
  a->size==b->size&&a->channel==b->channel&&!strcmp(a->version,b->version)&&
  !strcmp(a->url,b->url)&&!memcmp(a->sha256,b->sha256,sizeof(a->sha256));
}
static dsr_ota_fetch_result_t app_chain_run(const char *url,const dsr_ota_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,dsr_manifest_result_t *why,
 dsr_image_result_t *image_why,size_t expected_manifest_size,
 const dsr_ota_verified_t *consent,const dsr_ota_stream_sink_t *sink,bool *consent_changed)
{
 if(consent_changed)*consent_changed=false;
 if(why)*why=DSR_MANIFEST_INVALID;
 if(image_why)*image_why=DSR_IMAGE_STATE;
 if(!out)return DSR_FETCH_ARGUMENT;
 memset(out,0,sizeof(*out));
 app_chain_t *chain=calloc(1,sizeof(*chain));if(!chain)return DSR_FETCH_NOMEM;
 dsr_ota_fetch_result_t result=fetch(url,policy,control,&chain->verified,why,NULL,NULL,NULL,NULL,expected_manifest_size,NULL,false,NULL);
 if(result==DSR_FETCH_OK&&consent&&!same_consent(consent,&chain->verified)){
  if(consent_changed)*consent_changed=true;
  result=DSR_FETCH_ARGUMENT;
 }
 if(result==DSR_FETCH_OK){
  dsr_image_result_t created=dsr_ota_image_create(&chain->verified,&chain->image);
  if(image_why)*image_why=created;
  if(created!=DSR_IMAGE_OK)result=DSR_FETCH_IMAGE;
  else result=fetch(chain->verified.url,policy,control,&chain->verified,NULL,chain->image,image_why,NULL,NULL,0,sink,false,NULL);
 }
 dsr_ota_image_destroy(chain->image);
 if(result==DSR_FETCH_OK)*out=chain->verified;
 free(chain);return result;
}
dsr_ota_fetch_result_t dsr_ota_fetch_app_sized(const char *url,const dsr_ota_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,dsr_manifest_result_t *why,
 dsr_image_result_t *image_why,size_t expected_manifest_size)
{return app_chain_run(url,policy,control,out,why,image_why,expected_manifest_size,NULL,NULL,NULL);}
dsr_ota_fetch_result_t dsr_ota_fetch_stream_app(const char *url,const dsr_ota_policy_t *policy,
 const dsr_ota_verified_t *consent,const dsr_ota_fetch_control_t *control,
 const dsr_ota_stream_sink_t *sink,dsr_ota_verified_t *out,dsr_manifest_result_t *why,
 dsr_image_result_t *image_why,bool *consent_changed)
{
 if(consent_changed)*consent_changed=false;
 if(!consent||!sink||!sink->write){if(out)memset(out,0,sizeof(*out));if(why)*why=DSR_MANIFEST_INVALID;if(image_why)*image_why=DSR_IMAGE_STATE;return DSR_FETCH_ARGUMENT;}
 return app_chain_run(url,policy,control,out,why,image_why,0,consent,sink,consent_changed);
}
dsr_ota_fetch_result_t dsr_ota_fetch_app(const char *url,const dsr_ota_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,dsr_manifest_result_t *why,
 dsr_image_result_t *image_why)
{return dsr_ota_fetch_app_sized(url,policy,control,out,why,image_why,0);}
