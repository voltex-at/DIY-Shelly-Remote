/* Genuine release parser + fetch. SDK/TLS/cache are explicitly a MODEL.
 * No network, manifest trust, app download, or Flash is proved here. */
#include "dsr_ota_release_fetch.h"
#include "dsr_ota_fetch_private.h"
#include "dsr_ota_discovery.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static unsigned checks,allocations,fail_allocation,owned,parse_calls,postparse;
static void *pointers[256];
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);void __real_free(void *);
static void *track(void *p){if(p){unsigned i;for(i=0;i<256&&pointers[i];i++);CHECK(i<256);pointers[i]=p;owned++;}return p;}
void *__wrap_malloc(size_t n){allocations++;return allocations==fail_allocation?NULL:track(__real_malloc(n));}
void *__wrap_calloc(size_t n,size_t s){allocations++;return allocations==fail_allocation?NULL:track(__real_calloc(n,s));}
void __wrap_free(void *p){if(p){unsigned i;for(i=0;i<256&&pointers[i]!=p;i++);CHECK(i<256&&owned);pointers[i]=NULL;owned--;}__real_free(p);}
typedef struct {const char *key,*value;} header_t;
static struct {header_t headers[80];unsigned count,sets,reads,closed,cleaned;int status;int64_t declared;bool chunked,complete;const unsigned char *body;size_t length,offset;esp_http_client_config_t config;} client;
static unsigned initialized,opened,closed,cleaned,fail_init,fail_set,fail_open,fail_headers,fail_read,fail_close,fail_cleanup;
static size_t chunk;static bool cancel_flag,cancel_init,cancel_read,cancel_cleanup,overread,early_complete,late_location;
static int64_t monotonic,delay;static time_t wall;
static const char valid[]="{\"id\":123,\"url\":\"https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/123\",\"tag_name\":\"v1.2.4\",\"draft\":false,\"prerelease\":false,\"assets\":[{\"name\":\"manifest.json\",\"state\":\"uploaded\",\"size\":700,\"browser_download_url\":\"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/manifest.json\"}]}";
static char length_text[32];static unsigned char padded[16386];
static dsr_ota_release_policy_t policy={"v1.2.4","manifest.json",DSR_OTA_STABLE};
static const char *discovery_expected;
static unsigned discovery_parse_calls;
static bool cancelled(void *p){CHECK(p==&cancel_flag);return cancel_flag;}
static dsr_ota_fetch_control_t control={cancelled,&cancel_flag};
esp_err_t esp_crt_bundle_attach(void *p){(void)p;return ESP_OK;}
time_t __wrap_time(time_t *p){if(p)*p=wall;return wall;}
int64_t esp_timer_get_time(void){return monotonic;}
static void handle(esp_http_client_handle_t c){CHECK(c==&client&&!client.cleaned);}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c)
{
 initialized++;CHECK(initialized==1);if(fail_init)return NULL;
 char expected_url[160];snprintf(expected_url,sizeof(expected_url),"https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/tags/%s",policy.expected_tag);
 CHECK(!strcmp(c->url,discovery_expected?discovery_expected:expected_url));
 CHECK(c->method==HTTP_METHOD_GET&&c->transport_type==HTTP_TRANSPORT_OVER_SSL);
 CHECK(c->crt_bundle_attach==esp_crt_bundle_attach&&!c->skip_cert_common_name_check&&!c->common_name&&!c->cert_pem);
 CHECK(c->disable_auto_redirect&&c->max_authorization_retries==-1&&c->timeout_ms==5000&&c->buffer_size==1024&&c->buffer_size_tx==4096);
 CHECK(!c->username&&!c->password&&c->user_agent&&!strcmp(c->user_agent,"DIY-Shelly-Remote-manifest/1")&&c->event_handler&&c->user_data);
 CHECK(!c->client_cert_pem&&!c->client_key_pem&&!c->client_key_password&&!c->client_key_password_len&&!c->auth_type&&!c->use_global_ca_store&&!c->keep_alive_enable);
 client.config=*c;monotonic+=delay;if(cancel_init)cancel_flag=true;return &client;
}
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c,const char *k,const char *v)
{handle(c);client.sets++;CHECK((!strcmp(k,"Accept-Encoding")&&!strcmp(v,"identity"))||(!strcmp(k,"Accept")&&!strcmp(v,"application/vnd.github+json"))||(!strcmp(k,"X-GitHub-Api-Version")&&!strcmp(v,"2022-11-28")));return client.sets==fail_set?ESP_FAIL:ESP_OK;}
esp_err_t esp_http_client_open(esp_http_client_handle_t c,int n){handle(c);CHECK(!n&&client.sets==3);opened++;monotonic+=delay;return fail_open?ESP_FAIL:ESP_OK;}
static void event(const char *k,const char *v){esp_http_client_event_t e={.event_id=HTTP_EVENT_ON_HEADER,.client=&client,.user_data=client.config.user_data,.header_key=(char *)k,.header_value=(char *)v};(void)client.config.event_handler(&e);}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c){handle(c);monotonic+=delay;if(fail_headers)return -1;for(unsigned i=0;i<client.count;i++)event(client.headers[i].key,client.headers[i].value);return client.chunked?0:client.declared;}
int esp_http_client_get_status_code(esp_http_client_handle_t c){handle(c);return client.status;}
int64_t esp_http_client_get_content_length(esp_http_client_handle_t c){handle(c);return client.chunked?-1:client.declared;}
bool esp_http_client_is_chunked_response(esp_http_client_handle_t c){handle(c);return client.chunked;}
int esp_http_client_read(esp_http_client_handle_t c,char *p,int cap)
{handle(c);CHECK(cap>0&&cap<=1024);client.reads++;monotonic+=delay;if(cancel_read)cancel_flag=true;if(client.reads==fail_read)return -1;if(overread)return cap+1;size_t n=client.length-client.offset;if(n>(size_t)cap)n=(size_t)cap;if(n>chunk)n=chunk;memcpy(p,client.body+client.offset,n);client.offset+=n;if(!n&&late_location)event("Location","https://example.invalid/late");return (int)n;}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c){handle(c);return early_complete||(client.complete&&client.offset==client.length);}
esp_err_t esp_http_client_close(esp_http_client_handle_t c){handle(c);CHECK(!client.closed);client.closed++;closed++;return fail_close?ESP_FAIL:ESP_OK;}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c){handle(c);CHECK(client.closed&&!client.cleaned);client.cleaned++;cleaned++;if(cancel_cleanup)cancel_flag=true;return fail_cleanup?ESP_FAIL:ESP_OK;}
dsr_ota_release_result_t __real_dsr_ota_release_candidate_parse(const void *,size_t,const dsr_ota_release_policy_t *,dsr_ota_release_candidate_t *);
dsr_ota_release_result_t __wrap_dsr_ota_release_candidate_parse(const void *j,size_t n,const dsr_ota_release_policy_t *p,dsr_ota_release_candidate_t *out)
{
 parse_calls++;CHECK(closed==1&&cleaned==1&&owned==1); /* Only metadata storage remains. */
 dsr_ota_release_result_t r=__real_dsr_ota_release_candidate_parse(j,n,p,out);
 if(postparse==1)cancel_flag=true;
 if(postparse==2)monotonic+=31000000;
 if(postparse==3)monotonic=-1;
 return r;
}
dsr_ota_release_result_t __real_dsr_ota_discovery_parse(const void *,size_t,dsr_ota_channel_t,dsr_ota_discovery_t *);
dsr_ota_release_result_t __wrap_dsr_ota_discovery_parse(const void *j,size_t n,dsr_ota_channel_t channel,dsr_ota_discovery_t *out)
{
 discovery_parse_calls++;CHECK(closed==1&&cleaned==1&&owned==1);
 dsr_ota_release_result_t r=__real_dsr_ota_discovery_parse(j,n,channel,out);
 if(postparse==1)cancel_flag=true;
 if(postparse==2)monotonic+=31000000;
 if(postparse==3)monotonic=-1;
 return r;
}
static void add(const char *k,const char *v){CHECK(client.count<80);client.headers[client.count++]=(header_t){k,v};}
static void content(const void *p,size_t n){client.body=p;client.length=n;client.declared=(int64_t)n;client.count=0;snprintf(length_text,sizeof(length_text),"%zu",n);add("Content-Length",length_text);}
static void reset(void)
{CHECK(!owned);memset(&client,0,sizeof(client));client.status=200;client.complete=true;content(valid,strlen(valid));initialized=opened=closed=cleaned=0;fail_init=fail_set=fail_open=fail_headers=fail_read=fail_close=fail_cleanup=0;allocations=fail_allocation=parse_calls=postparse=0;chunk=1024;cancel_flag=cancel_init=cancel_read=cancel_cleanup=overread=early_complete=late_location=false;monotonic=delay=0;wall=1767225600;discovery_expected=NULL;policy=(dsr_ota_release_policy_t){"v1.2.4","manifest.json",DSR_OTA_STABLE};}
static void chunked(void){client.count=0;client.chunked=true;add("Transfer-Encoding","chunked");}
static bool zero(const void *p,size_t n){const unsigned char *s=p;for(size_t i=0;i<n;i++)if(s[i])return false;return true;}
static dsr_ota_fetch_result_t run(int expected,int detail)
{
 dsr_ota_release_candidate_t out;memset(&out,0xa5,sizeof(out));dsr_ota_release_result_t why=(dsr_ota_release_result_t)99;
 dsr_ota_fetch_result_t r=dsr_ota_fetch_release(&policy,&control,&out,&why);
 if(expected<0)CHECK(r!=DSR_FETCH_OK);else if((int)r!=expected){fprintf(stderr,"expected=%d got=%d why=%d parse=%u\n",expected,r,why,parse_calls);abort();}
 if(detail>=0)CHECK((int)why==detail);
 if(r==DSR_FETCH_OK){char expected_url[257];snprintf(expected_url,sizeof(expected_url),"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/%s/%s",policy.expected_tag,policy.manifest_asset);CHECK(!strcmp(out.tag,policy.expected_tag)&&out.manifest_size==700&&!strcmp(out.manifest_url,expected_url));}else CHECK(zero(&out,sizeof(out)));
 CHECK(!owned&&closed==cleaned);if(client.config.url)CHECK(closed==1&&cleaned==1);return r;
}
static void test_discovery(void)
{
 static const char stable[]="https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/latest";
 static const char dev[]="https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases?per_page=5";
 unsigned char *body=__real_malloc(16385);CHECK(body);size_t length=99;int status=99;
 reset();CHECK(dsr_ota_fetch_metadata_body(stable,&control,body,&length)==DSR_FETCH_URL&&length==0&&!initialized);
 reset();CHECK(dsr_ota_fetch_discovery_body((dsr_ota_channel_t)99,&control,body,&length,&status)==DSR_FETCH_ARGUMENT&&length==0&&status==0&&!initialized);
 reset();discovery_expected=stable;CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_OK&&status==200&&length==strlen(valid)&&!memcmp(body,valid,length)&&!parse_calls&&!owned);
 reset();discovery_expected=dev;content("[]",2);chunked();chunk=1;CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_DEV,&control,body,&length,&status)==DSR_FETCH_OK&&status==200&&length==2&&!memcmp(body,"[]",2)&&!parse_calls&&!owned);
 const int statuses[]={404,403,429};
 for(unsigned i=0;i<3;i++){reset();discovery_expected=stable;client.status=statuses[i];CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_STATUS&&status==statuses[i]&&length==0&&!client.reads&&!parse_calls&&!owned);}
 reset();discovery_expected=stable;client.status=302;add("Location",stable);CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_REDIRECT&&status==302&&length==0&&initialized==1&&!parse_calls&&!owned);
 reset();discovery_expected=stable;fail_open=1;CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_IO&&status==0&&length==0&&!owned);
 reset();discovery_expected=stable;content(padded,16384);CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_OK&&status==200&&length==16384&&!owned);
 reset();discovery_expected=stable;content(padded,16385);chunked();CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_SIZE&&status==200&&length==0&&!owned);
 reset();discovery_expected=stable;cancel_cleanup=true;CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_CANCELLED&&status==200&&length==0&&!owned);
 reset();discovery_expected=stable;wall=0;CHECK(dsr_ota_fetch_discovery_body(DSR_OTA_STABLE,&control,body,&length,&status)==DSR_FETCH_CLOCK&&status==0&&length==0&&!initialized&&!owned);
 __real_free(body);
}
static void test_native_discovery(void)
{
 static const char stable[]="https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/latest";
 static const char dev[]="https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases?per_page=5";
 static const char native[]="{\"id\":123,\"url\":\"https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/123\",\"tag_name\":\"v1.2.4\",\"draft\":false,\"prerelease\":false,\"assets\":[{\"name\":\"" DSR_OTA_NATIVE_MANIFEST_ASSET "\",\"state\":\"uploaded\",\"size\":700,\"browser_download_url\":\"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/" DSR_OTA_NATIVE_MANIFEST_ASSET "\"}],\"body\":\"plain <b>untrusted</b>\"}";
 dsr_ota_discovery_t out;dsr_ota_release_result_t why;int status;
 reset();discovery_expected=stable;content(native,strlen(native));
 CHECK(dsr_ota_discovery_fetch(DSR_OTA_STABLE,&control,&out,&why,&status)==DSR_FETCH_OK&&why==DSR_RELEASE_OK&&status==200&&!owned&&!strcmp(out.notes,"plain <b>untrusted</b>"));unsigned total=allocations;
 for(size_t n=1;n<=1024;n*=2){reset();discovery_expected=stable;content(native,strlen(native));chunked();chunk=n;CHECK(dsr_ota_discovery_fetch(DSR_OTA_STABLE,&control,&out,&why,&status)==DSR_FETCH_OK&&!owned&&client.offset==strlen(native));}
 for(unsigned n=1;n<=total;n++){reset();discovery_expected=stable;content(native,strlen(native));fail_allocation=n;memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_discovery_fetch(DSR_OTA_STABLE,&control,&out,&why,&status)!=DSR_FETCH_OK&&zero(&out,sizeof(out))&&!owned);}
 for(unsigned n=1;n<=3;n++){reset();discovery_expected=stable;content(native,strlen(native));postparse=n;CHECK(dsr_ota_discovery_fetch(DSR_OTA_STABLE,&control,&out,&why,&status)==(n==1?DSR_FETCH_CANCELLED:DSR_FETCH_TIMEOUT)&&why==DSR_RELEASE_OK&&zero(&out,sizeof(out))&&!owned);}
 reset();discovery_expected=dev;content("[]",2);CHECK(dsr_ota_discovery_fetch(DSR_OTA_DEV,&control,&out,&why,&status)==DSR_FETCH_RELEASE&&why==DSR_RELEASE_NOT_FOUND&&status==200&&zero(&out,sizeof(out))&&!owned);
 reset();discovery_expected=stable;client.status=404;CHECK(dsr_ota_discovery_fetch(DSR_OTA_STABLE,&control,&out,&why,&status)==DSR_FETCH_STATUS&&status==404&&zero(&out,sizeof(out))&&!owned);
 reset();wall=0;CHECK(dsr_ota_discovery_fetch(DSR_OTA_STABLE,&control,&out,&why,&status)==DSR_FETCH_CLOCK&&status==0&&!initialized&&zero(&out,sizeof(out))&&!owned);
 reset();cancel_flag=true;CHECK(dsr_ota_discovery_fetch(DSR_OTA_STABLE,&control,&out,&why,&status)==DSR_FETCH_CANCELLED&&!initialized&&!owned);
 reset();CHECK(dsr_ota_discovery_fetch((dsr_ota_channel_t)99,&control,&out,&why,&status)==DSR_FETCH_ARGUMENT&&status==0&&!initialized&&zero(&out,sizeof(out))&&!owned);
 CHECK(discovery_parse_calls>10);
}
int main(void)
{
 cJSON_Hooks hooks={__wrap_malloc,__wrap_free};cJSON_InitHooks(&hooks);
 reset();run(DSR_FETCH_OK,DSR_RELEASE_OK);CHECK(parse_calls==1&&client.offset==strlen(valid));unsigned successful_allocations=allocations;
 for(size_t n=1;n<=1024;n++){reset();chunk=n;early_complete=true;run(DSR_FETCH_OK,DSR_RELEASE_OK);CHECK(client.offset==strlen(valid)&&client.reads>=2);}
 for(size_t n=1;n<=1024;n*=2){reset();chunked();chunk=n;run(DSR_FETCH_OK,DSR_RELEASE_OK);}
 memcpy(padded,valid,strlen(valid));memset(padded+strlen(valid),' ',sizeof(padded)-strlen(valid));
 reset();content(padded,16384);run(DSR_FETCH_OK,DSR_RELEASE_OK);
 reset();content(padded,16384);chunked();run(DSR_FETCH_OK,DSR_RELEASE_OK);
 reset();content(padded,16385);run(DSR_FETCH_SIZE,DSR_RELEASE_INVALID);CHECK(!parse_calls&&!client.reads);
 reset();content(padded,16385);chunked();run(DSR_FETCH_SIZE,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 reset();content("{}",2);run(DSR_FETCH_RELEASE,DSR_RELEASE_INVALID);CHECK(parse_calls==1);
 reset();content(valid,strlen(valid)-1);run(DSR_FETCH_RELEASE,DSR_RELEASE_INVALID);
 /* A well-formed HTTP body is still subject to the genuine candidate policy. */
 unsigned char changed[sizeof(valid)];memcpy(changed,valid,sizeof(valid));
 char *tag=strstr((char *)changed,"v1.2.4");CHECK(tag);tag[5]='5';
 reset();content(changed,strlen((char *)changed));run(DSR_FETCH_RELEASE,DSR_RELEASE_POLICY);
 memcpy(changed,valid,sizeof(valid));char *name=strstr((char *)changed,"manifest.json");CHECK(name);name[0]='x';
 reset();content(changed,strlen((char *)changed));run(DSR_FETCH_RELEASE,DSR_RELEASE_NOT_FOUND);
 for(unsigned n=1;n<=successful_allocations;n++){reset();fail_allocation=n;run(-1,-1);CHECK(!owned);}
 reset();fail_init=1;run(DSR_FETCH_NOMEM,DSR_RELEASE_INVALID);CHECK(!opened&&!closed);
 for(unsigned n=1;n<=3;n++){reset();fail_set=n;run(DSR_FETCH_IO,DSR_RELEASE_INVALID);CHECK(!opened&&!parse_calls);}
 reset();fail_open=1;run(DSR_FETCH_IO,DSR_RELEASE_INVALID);
 reset();fail_headers=1;run(DSR_FETCH_IO,DSR_RELEASE_INVALID);
 reset();fail_read=1;run(DSR_FETCH_IO,DSR_RELEASE_INVALID);
 reset();fail_close=1;run(DSR_FETCH_IO,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 reset();fail_cleanup=1;run(DSR_FETCH_IO,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 reset();overread=true;run(-1,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 reset();client.complete=false;run(DSR_FETCH_INCOMPLETE,DSR_RELEASE_INVALID);
 reset();client.length--;run(DSR_FETCH_INCOMPLETE,DSR_RELEASE_INVALID);
 reset();client.length++;client.body=padded;run(-1,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 const int statuses[]={204,206,300,301,302,303,304,305,306,307,308,400,401,403,404,429,500};
 for(unsigned i=0;i<sizeof(statuses)/sizeof(statuses[0]);i++){reset();client.status=statuses[i];add("Location","https://api.github.com/elsewhere");run(-1,DSR_RELEASE_INVALID);CHECK(initialized==1&&!parse_calls&&!client.reads);}
 const char *bad[]={"0","01","-1","+1","1.0","1e3","18446744073709551616","16385"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){reset();client.headers[0].value=bad[i];run(-1,DSR_RELEASE_INVALID);CHECK(!parse_calls);}
 reset();client.count=0;run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();add("content-length",length_text);run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();add("Transfer-Encoding","chunked");client.chunked=true;run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();client.declared--;run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();chunked();client.chunked=false;run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();client.chunked=true;run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();chunked();add("transfer-encoding","chunked");run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();add("Content-Encoding","gzip");run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();add("Content-Encoding","identity");run(DSR_FETCH_OK,DSR_RELEASE_OK);
 reset();add("Content-Encoding","identity");add("content-encoding","identity");run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();add("Location","https://example.invalid/");run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);
 reset();chunked();late_location=true;run(DSR_FETCH_HEADERS,DSR_RELEASE_INVALID);CHECK(!parse_calls); /* SDK-visible event, not raw trailer proof. */
 reset();wall=0;run(DSR_FETCH_CLOCK,DSR_RELEASE_INVALID);CHECK(!initialized&&!allocations);
 reset();wall=(time_t)-1;run(DSR_FETCH_CLOCK,DSR_RELEASE_INVALID);CHECK(!initialized&&!allocations);
 reset();wall=1735689600;run(DSR_FETCH_OK,DSR_RELEASE_OK);
 reset();cancel_flag=true;run(DSR_FETCH_CANCELLED,DSR_RELEASE_INVALID);CHECK(!initialized&&!allocations);
 reset();cancel_init=true;run(DSR_FETCH_CANCELLED,DSR_RELEASE_INVALID);CHECK(!opened);
 reset();delay=31000000;run(DSR_FETCH_TIMEOUT,DSR_RELEASE_INVALID);CHECK(!opened);
 reset();delay=10000000;run(DSR_FETCH_TIMEOUT,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 reset();cancel_read=true;fail_read=1;run(DSR_FETCH_CANCELLED,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 reset();cancel_cleanup=true;run(DSR_FETCH_CANCELLED,DSR_RELEASE_INVALID);CHECK(!parse_calls);
 reset();postparse=1;run(DSR_FETCH_CANCELLED,DSR_RELEASE_OK);CHECK(parse_calls==1);
 reset();postparse=2;run(DSR_FETCH_TIMEOUT,DSR_RELEASE_OK);CHECK(parse_calls==1);
 reset();postparse=3;run(DSR_FETCH_TIMEOUT,DSR_RELEASE_OK);CHECK(parse_calls==1);
 reset();policy.selected_channel=DSR_OTA_DEV;run(DSR_FETCH_ARGUMENT,DSR_RELEASE_POLICY);CHECK(!initialized);
 reset();policy.expected_tag="..";run(DSR_FETCH_ARGUMENT,DSR_RELEASE_ARGUMENT);CHECK(!initialized);
 reset();policy.manifest_asset="bad/name";run(DSR_FETCH_ARGUMENT,DSR_RELEASE_ARGUMENT);CHECK(!initialized);
 const char *bad_tags[]={"/","?","%","@","https://evil.invalid/tag",NULL};
 for(unsigned i=0;i<sizeof(bad_tags)/sizeof(bad_tags[0]);i++){reset();policy.expected_tag=bad_tags[i];run(DSR_FETCH_ARGUMENT,DSR_RELEASE_ARGUMENT);CHECK(!initialized&&!allocations);}
 reset();policy.selected_channel=(dsr_ota_channel_t)99;run(DSR_FETCH_ARGUMENT,DSR_RELEASE_POLICY);CHECK(!initialized&&!allocations);
 char tag64[65],asset96[97],boundary[1024];memset(tag64,'t',64);tag64[64]=0;memset(asset96,'a',96);asset96[96]=0;
 int boundary_len=snprintf(boundary,sizeof(boundary),"{\"id\":123,\"url\":\"https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/123\",\"tag_name\":\"%s\",\"draft\":false,\"prerelease\":false,\"assets\":[{\"name\":\"%s\",\"state\":\"uploaded\",\"size\":700,\"browser_download_url\":\"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/%s/%s\"}]}",tag64,asset96,tag64,asset96);
 CHECK(boundary_len>0&&(size_t)boundary_len<sizeof(boundary));
 reset();policy.expected_tag=tag64;policy.manifest_asset=asset96;content(boundary,(size_t)boundary_len);run(DSR_FETCH_OK,DSR_RELEASE_OK);
 char tag65[66];memset(tag65,'t',65);tag65[65]=0;reset();policy.expected_tag=tag65;run(DSR_FETCH_ARGUMENT,DSR_RELEASE_ARGUMENT);CHECK(!initialized&&!allocations);
 char asset97[98];memset(asset97,'a',97);asset97[97]=0;reset();policy.manifest_asset=asset97;run(DSR_FETCH_ARGUMENT,DSR_RELEASE_ARGUMENT);CHECK(!initialized&&!allocations);
 reset();dsr_ota_release_candidate_t out;memset(&out,0xa5,sizeof(out));dsr_ota_release_result_t why=DSR_RELEASE_OK;
 CHECK(dsr_ota_fetch_release(NULL,&control,&out,&why)==DSR_FETCH_ARGUMENT&&why==DSR_RELEASE_ARGUMENT&&zero(&out,sizeof(out))&&!initialized&&!allocations);
 reset();CHECK(dsr_ota_fetch_release(&policy,&control,NULL,&why)==DSR_FETCH_ARGUMENT&&why==DSR_RELEASE_INVALID&&!initialized&&!allocations);
 reset();CHECK(dsr_ota_fetch_release(&policy,NULL,&out,NULL)==DSR_FETCH_OK&&!owned&&parse_calls==1);
 test_discovery();
 test_native_discovery();
 printf("PASS: %u release-fetch/discovery checks; genuine parser; %u allocation faults; transport MODEL, no HTTPS/Flash/trust proof\n",checks,successful_allocations);
 return 0;
}
