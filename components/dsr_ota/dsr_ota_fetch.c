#include "dsr_ota_fetch.h"
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
 static const char prefix[]="https://release-assets.githubusercontent.com/";
 size_t n=bounded_length(url,DSR_OTA_FETCH_URL_MAX);
 if(n>DSR_OTA_FETCH_URL_MAX||n<=sizeof(prefix)-1||strncmp(url,prefix,sizeof(prefix)-1))return false;
 const char *p=url+sizeof(prefix)-1,*path=p;
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
dsr_ota_fetch_result_t dsr_ota_fetch_manifest(const char *url,const dsr_ota_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,dsr_manifest_result_t *why)
{
 if(why)*why=DSR_MANIFEST_INVALID;
 if(!out)return DSR_FETCH_ARGUMENT;
 memset(out,0,sizeof(*out));
 if(!policy||!policy->current_version||bounded_length(policy->current_version,31)>31
    ||(policy->selected_channel!=DSR_OTA_STABLE&&policy->selected_channel!=DSR_OTA_DEV)
    ||!policy->inactive_slot_size||policy->inactive_slot_size>0x600000)return DSR_FETCH_ARGUMENT;
 if(!initial_url(url))return DSR_FETCH_URL;
 if(time(NULL)<(time_t)1735689600)return DSR_FETCH_CLOCK;
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
  if(!stopped(s)&&esp_http_client_set_header(client,"Accept","application/json")!=ESP_OK)s->result=DSR_FETCH_IO;
  if(!stopped(s)&&esp_http_client_open(client,0)!=ESP_OK&&s->result==DSR_FETCH_OK)s->result=DSR_FETCH_IO;
  if(!stopped(s)){
   int64_t length=esp_http_client_fetch_headers(client);
   if(!stopped(s)){
    int status=esp_http_client_get_status_code(client);
    if(length<0)s->result=DSR_FETCH_IO;
    else if(status==301||status==302||status==303||status==307||status==308){
     if(hop||!s->location_seen||!asset_url(s->location))s->result=DSR_FETCH_REDIRECT;
     else redirect=true;
    }else if(status!=200)s->result=DSR_FETCH_STATUS;
    else if(s->location_seen||(!s->length_seen&&!s->transfer_seen))s->result=DSR_FETCH_HEADERS;
    else if(s->length_seen&&(!s->declared||s->declared>DSR_OTA_MANIFEST_MAX))s->result=DSR_FETCH_SIZE;
    else if((s->length_seen&&(length!=(int64_t)s->declared||esp_http_client_get_content_length(client)!=length||esp_http_client_is_chunked_response(client)))
      ||(s->transfer_seen&&(length!=0||!esp_http_client_is_chunked_response(client))))s->result=DSR_FETCH_HEADERS;
    else{
     /* Read even when complete is already true: fetch_headers may cache body.
      * One byte per call bounds positive-progress body waits between polls. */
     while(!stopped(s)){
      int n=esp_http_client_read(client,(char *)s->body+s->used,1);
      if(stopped(s))break;
      if(n<0){s->result=DSR_FETCH_IO;break;}
      if(n>1){s->result=DSR_FETCH_SIZE;break;}
      if(!n){
       if(!esp_http_client_is_complete_data_received(client)||(s->length_seen&&s->used!=s->declared))s->result=DSR_FETCH_INCOMPLETE;
       else if(!s->used)s->result=DSR_FETCH_SIZE;
       break;
      }
      if(++s->used>DSR_OTA_MANIFEST_MAX||(s->length_seen&&s->used>s->declared)){s->result=DSR_FETCH_SIZE;break;}
     }
    }
   }
  }
  esp_err_t closed=esp_http_client_close(client),cleaned=esp_http_client_cleanup(client);
  if((closed!=ESP_OK||cleaned!=ESP_OK)&&s->result==DSR_FETCH_OK)s->result=DSR_FETCH_IO;
  if(stopped(s))break;
  if(redirect){strcpy(s->url,s->location);continue;}
  dsr_manifest_result_t result=dsr_ota_manifest_verify(s->body,s->used,policy,out);
  if(why)*why=result;
  if(result!=DSR_MANIFEST_OK)s->result=DSR_FETCH_MANIFEST;
  else (void)stopped(s);
  break;
 }
 dsr_ota_fetch_result_t result=s->result;
 if(result!=DSR_FETCH_OK)memset(out,0,sizeof(*out));
 free(s);return result;
}
