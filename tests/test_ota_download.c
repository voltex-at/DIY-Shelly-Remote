/* Actual manifest-fetch/verifier/app-fetch/image chain. HTTP/TLS SDK is a MODEL;
 * Ed25519 and OpenSSL SHA are genuine. No bytes reach Flash or an installer. */
#include "dsr_ota_fetch.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "mbedtls/sha256.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
static unsigned checks,allocations,fail_allocation,owned;
static void *pointers[128];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
void *__real_malloc(size_t size);void *__real_calloc(size_t n,size_t size);void __real_free(void *p);
static void *track(void *p){if(p){unsigned i;for(i=0;i<128&&pointers[i];i++);CHECK(i<128);pointers[i]=p;owned++;}return p;}
void *__wrap_malloc(size_t n){allocations++;return fail_allocation&&allocations==fail_allocation?NULL:track(__real_malloc(n));}
void *__wrap_calloc(size_t n,size_t size){allocations++;return fail_allocation&&allocations==fail_allocation?NULL:track(__real_calloc(n,size));}
void __wrap_free(void *p){if(p){unsigned i;for(i=0;i<128&&pointers[i]!=p;i++);CHECK(i<128&&owned);pointers[i]=NULL;owned--;}__real_free(p);}
typedef struct {const char *key,*value;} header_t;
typedef struct {bool manifest,complete,chunked,closed,cleaned;int status;int64_t declared;const unsigned char *body;size_t length,offset;header_t headers[8];unsigned count,sets,reads;esp_http_client_config_t config;char url[2049],length_text[32];} hop_t;
static hop_t hops[4];static unsigned initialized,app_requests,read_calls,close_calls,cleanup_calls;
static unsigned fail_init,fail_open,fail_read,fail_close,fail_cleanup,cancel_app_read,trust_fault,post_finish;
static bool cancel_flag,early_complete,oversize_return,app_trailer_location;
static int64_t monotonic,app_delay,app_first_delay;
static size_t app_chunk;
static unsigned again_at,again_left,again_calls;
static time_t wall;
static unsigned char manifests[5][2049],*image,*changed;static size_t manifest_lengths[5],image_length;
static const char initial[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/manifest.json";
static const char app_url[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin";
static const char manifest_cdn[]="https://release-assets.githubusercontent.com/1/manifest?token=fixture";
static const char app_cdn[]="https://release-assets.githubusercontent.com/1/app?token=fixture";
static void all_closed(void){CHECK(close_calls&&close_calls==cleanup_calls);for(unsigned i=0;i<initialized;i++)CHECK(hops[i].closed&&hops[i].cleaned);}
#ifdef DSR_DOWNLOAD_HOST_TEST_TRUST
const uint8_t *dsr_ota_trusted_key(const char *id)
{static const uint8_t key[32]={0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a};all_closed();CHECK(!app_requests);if(trust_fault==1)cancel_flag=true;if(trust_fault==2)monotonic+=31000000;if(trust_fault==3)monotonic+=29000000;return !strcmp(id,"rfc8032-host-only")?key:NULL;}
#endif
int __real_EVP_DigestFinal_ex(EVP_MD_CTX *c,unsigned char *out,unsigned int *length);
int __wrap_EVP_DigestFinal_ex(EVP_MD_CTX *c,unsigned char *out,unsigned int *length)
{int result=__real_EVP_DigestFinal_ex(c,out,length);all_closed();if(post_finish==1)cancel_flag=true;if(post_finish==2)monotonic+=31000000;return result;}
time_t __wrap_time(time_t *out){if(out)*out=wall;return wall;}
int64_t esp_timer_get_time(void){return monotonic;}
esp_err_t esp_crt_bundle_attach(void *config){(void)config;return ESP_OK;}
static hop_t *hop(esp_http_client_handle_t c){hop_t *h=c;CHECK(h>=hops&&h<hops+4&&!h->cleaned);return h;}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c)
{
 initialized++;CHECK(initialized<=4);if(initialized==fail_init)return NULL;hop_t *h=&hops[initialized-1];
 for(unsigned i=0;i+1<initialized;i++)CHECK(hops[i].closed&&hops[i].cleaned);
 CHECK(c->transport_type==HTTP_TRANSPORT_OVER_SSL&&c->method==HTTP_METHOD_GET&&c->crt_bundle_attach==esp_crt_bundle_attach&&!c->skip_cert_common_name_check&&!c->common_name&&c->disable_auto_redirect&&c->max_authorization_retries==-1&&c->timeout_ms==5000);
 CHECK(!c->username&&!c->password&&!c->client_key_pem&&!c->client_cert_pem&&!c->use_global_ca_store);
 CHECK(c->url&&strlen(c->url)<=2048&&c->event_handler&&c->user_data);strcpy(h->url,c->url);h->config=*c;
 if(!h->manifest){app_requests++;CHECK(!strcmp(c->url,app_url)||!strcmp(c->url,app_cdn));}else CHECK(!strcmp(c->url,initial)||!strcmp(c->url,manifest_cdn));return h;
}
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c,const char *key,const char *value)
{hop_t *h=hop(c);h->sets++;CHECK((!strcmp(key,"Accept-Encoding")&&!strcmp(value,"identity"))||(!strcmp(key,"Accept")&&!strcmp(value,h->manifest?"application/json":"application/octet-stream")));return ESP_OK;}
esp_err_t esp_http_client_open(esp_http_client_handle_t c,int length)
{hop_t *h=hop(c);CHECK(!length&&h->sets==2);return initialized==fail_open?ESP_FAIL:ESP_OK;}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c)
{hop_t *h=hop(c);for(unsigned i=0;i<h->count;i++){esp_http_client_event_t e={.event_id=HTTP_EVENT_ON_HEADER,.client=c,.user_data=h->config.user_data,.header_key=(char *)h->headers[i].key,.header_value=(char *)h->headers[i].value};(void)h->config.event_handler(&e);}return h->chunked?0:h->declared;}
int esp_http_client_get_status_code(esp_http_client_handle_t c){return hop(c)->status;}
int64_t esp_http_client_get_content_length(esp_http_client_handle_t c){hop_t *h=hop(c);return h->chunked?-1:h->declared;}
bool esp_http_client_is_chunked_response(esp_http_client_handle_t c){return hop(c)->chunked;}
int esp_http_client_read(esp_http_client_handle_t c,char *out,int capacity)
{hop_t *h=hop(c);CHECK(capacity>0&&capacity<=(h->manifest?1:1024));h->reads++;read_calls++;if(!h->manifest){monotonic+=app_delay;if(h->reads==1)monotonic+=app_first_delay;if(h->reads==cancel_app_read)cancel_flag=true;if(again_at&&h->reads>=again_at&&again_left){again_left--;again_calls++;monotonic+=5000000;return -ESP_ERR_HTTP_EAGAIN;}}if(read_calls==fail_read)return -1;if(oversize_return&&!h->manifest)return capacity+1;size_t n=h->length-h->offset;if(n>(size_t)capacity)n=(size_t)capacity;if(!h->manifest&&n>app_chunk)n=app_chunk;memcpy(out,h->body+h->offset,n);h->offset+=n;if(!n&&!h->manifest&&app_trailer_location){esp_http_client_event_t e={.event_id=HTTP_EVENT_ON_HEADER,.client=c,.user_data=h->config.user_data,.header_key="Location",.header_value="https://release-assets.githubusercontent.com/late"};(void)h->config.event_handler(&e);}return (int)n;}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c){hop_t *h=hop(c);return early_complete||(h->complete&&h->offset==h->length);}
esp_err_t esp_http_client_close(esp_http_client_handle_t c){hop_t *h=hop(c);CHECK(!h->closed);h->closed=true;close_calls++;return close_calls==fail_close?ESP_FAIL:ESP_OK;}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c){hop_t *h=hop(c);CHECK(h->closed);h->cleaned=true;cleanup_calls++;return cleanup_calls==fail_cleanup?ESP_FAIL:ESP_OK;}
static bool cancelled(void *context){CHECK(context==&cancel_flag);return cancel_flag;}
static void content(unsigned index,bool manifest,const unsigned char *data,size_t length)
{hop_t *h=&hops[index];memset(h,0,sizeof(*h));h->manifest=manifest;h->status=200;h->complete=true;h->body=data;h->length=length;h->declared=(int64_t)length;snprintf(h->length_text,sizeof(h->length_text),"%lu",(unsigned long)length);h->headers[0]=(header_t){"Content-Length",h->length_text};h->count=1;}
static void redirect(unsigned index,bool manifest,const char *url)
{content(index,manifest,NULL,0);hops[index].status=302;hops[index].headers[0]=(header_t){"Location",url};hops[index].count=1;}
static void reset(void)
{CHECK(!owned&&!dsr_test_sha_live());memset(hops,0,sizeof(hops));content(0,true,manifests[0],manifest_lengths[0]);content(1,false,image,image_length);initialized=app_requests=read_calls=close_calls=cleanup_calls=0;fail_init=fail_open=fail_read=fail_close=fail_cleanup=cancel_app_read=trust_fault=post_finish=0;again_at=again_left=again_calls=0;cancel_flag=early_complete=oversize_return=app_trailer_location=false;monotonic=app_delay=app_first_delay=0;wall=1767225600;app_chunk=1024;allocations=fail_allocation=0;dsr_test_sha_fault(0);}
static bool zero(const dsr_ota_verified_t *v){const unsigned char *p=(const unsigned char *)v;for(size_t i=0;i<sizeof(*v);i++)if(p[i])return false;return true;}
static dsr_ota_fetch_result_t run(int expected,int manifest_expected,int image_expected)
{
 dsr_ota_policy_t policy={"1.2.3",DSR_OTA_STABLE,0x600000};dsr_ota_fetch_control_t control={cancelled,&cancel_flag};dsr_ota_verified_t out;memset(&out,0xa5,sizeof(out));dsr_manifest_result_t m=(dsr_manifest_result_t)99;dsr_image_result_t im=(dsr_image_result_t)99;
 dsr_ota_fetch_result_t result=dsr_ota_fetch_app(initial,&policy,&control,&out,&m,&im);
#ifndef DSR_DOWNLOAD_HOST_TEST_TRUST
 (void)image_expected;
 /* A production-empty chain cannot progress into any planned app-side fault. */
 if(initialized&&(manifest_expected==DSR_MANIFEST_OK||manifest_expected==DSR_MANIFEST_BAD_SIGNATURE||manifest_expected==DSR_MANIFEST_UNTRUSTED_KEY)){CHECK(result==DSR_FETCH_MANIFEST&&m==DSR_MANIFEST_UNTRUSTED_KEY&&im==DSR_IMAGE_STATE);}
 else if(expected>=0){CHECK(result==(dsr_ota_fetch_result_t)expected);if(manifest_expected>=0)CHECK(m==(dsr_manifest_result_t)manifest_expected);}
 CHECK(!app_requests);
#else
 if(expected<0)CHECK(result!=DSR_FETCH_OK);else if(result!=(dsr_ota_fetch_result_t)expected){fprintf(stderr,"expected=%d got=%d manifest=%d image=%d\n",expected,result,m,im);abort();}
 if(manifest_expected>=0)CHECK(m==(dsr_manifest_result_t)manifest_expected);
 if(image_expected>=0)CHECK(im==(dsr_image_result_t)image_expected);
#endif
 if(result==DSR_FETCH_OK){CHECK(out.size==image_length&&!strcmp(out.version,"1.2.4")&&!strcmp(out.url,app_url));CHECK(app_requests&&hops[initialized-1].offset==image_length);}
 else CHECK(zero(&out));
 CHECK(!owned&&!dsr_test_sha_live());for(unsigned i=0;i<initialized;i++)if(hops[i].config.url)CHECK(hops[i].closed&&hops[i].cleaned);return result;
}
static uint32_t integer(FILE *f){uint8_t b[4];CHECK(fread(b,1,4,f)==4);return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
int main(int argc,char **argv)
{
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);image_length=integer(f);CHECK(image_length>=512&&image_length<=0x600000);image=__real_malloc(image_length+1);changed=__real_malloc(image_length+1);CHECK(image&&changed);CHECK(fread(image,1,image_length,f)==image_length);image[image_length]=0;
 for(unsigned i=0;i<5;i++){manifest_lengths[i]=integer(f);CHECK(manifest_lengths[i]<=2048&&fread(manifests[i],1,manifest_lengths[i],f)==manifest_lengths[i]);}CHECK(fgetc(f)==EOF);fclose(f);cJSON_Hooks hooks={__wrap_malloc,__wrap_free};cJSON_InitHooks(&hooks);
 reset();run(DSR_FETCH_OK,DSR_MANIFEST_OK,DSR_IMAGE_OK);unsigned successful_allocations=allocations;
 const size_t chunks[]={1,7,41,42,43,127,511,512,513,1024};
 for(unsigned i=0;i<sizeof(chunks)/sizeof(chunks[0]);i++){reset();app_chunk=chunks[i];run(DSR_FETCH_OK,DSR_MANIFEST_OK,DSR_IMAGE_OK);}
 /* IDF returns -0x7007 for a recoverable read timeout on this connection. */
#ifdef DSR_DOWNLOAD_HOST_TEST_TRUST
 reset();again_at=3;again_left=2;run(DSR_FETCH_OK,DSR_MANIFEST_OK,DSR_IMAGE_OK);CHECK(again_calls==2&&initialized==2);
 reset();again_at=1;again_left=100;run(DSR_FETCH_TIMEOUT,DSR_MANIFEST_OK,DSR_IMAGE_OK);CHECK(again_calls==6&&initialized==2);
 reset();again_at=1;again_left=2;cancel_app_read=2;run(DSR_FETCH_CANCELLED,DSR_MANIFEST_OK,DSR_IMAGE_OK);CHECK(again_calls==2&&initialized==2);
#else
 reset();again_at=1;again_left=100;run(DSR_FETCH_MANIFEST,DSR_MANIFEST_UNTRUSTED_KEY,DSR_IMAGE_STATE);CHECK(!again_calls&&!app_requests);
#endif
 reset();redirect(0,true,manifest_cdn);content(1,true,manifests[0],manifest_lengths[0]);redirect(2,false,app_cdn);content(3,false,image,image_length);run(DSR_FETCH_OK,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 reset();redirect(1,false,app_cdn);redirect(2,false,app_cdn);run(DSR_FETCH_REDIRECT,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 reset();redirect(1,false,"https://evil.example/app");run(DSR_FETCH_REDIRECT,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 reset();trust_fault=3;app_first_delay=15000000;run(DSR_FETCH_OK,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 reset();hops[1].chunked=true;hops[1].headers[0]=(header_t){"Transfer-Encoding","chunked"};run(DSR_FETCH_OK,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 /* SDK-visible late Location event. This does not model raw trailer assembly:
  * the actual SDK may not dispatch a final pending trailer header. */
 reset();hops[1].chunked=true;hops[1].headers[0]=(header_t){"Transfer-Encoding","chunked"};app_trailer_location=true;run(DSR_FETCH_HEADERS,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 reset();early_complete=true;app_chunk=7;run(DSR_FETCH_OK,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 reset();content(0,true,manifests[1],manifest_lengths[1]);run(DSR_FETCH_MANIFEST,DSR_MANIFEST_BAD_SIGNATURE,DSR_IMAGE_STATE);CHECK(!app_requests);
 reset();content(0,true,manifests[2],manifest_lengths[2]);run(DSR_FETCH_MANIFEST,DSR_MANIFEST_UNTRUSTED_KEY,DSR_IMAGE_STATE);CHECK(!app_requests);
 reset();content(0,true,manifests[3],manifest_lengths[3]);run(DSR_FETCH_IMAGE,DSR_MANIFEST_OK,DSR_IMAGE_ARGUMENT);CHECK(!app_requests);
 reset();content(0,true,manifests[4],manifest_lengths[4]);run(DSR_FETCH_MANIFEST,DSR_MANIFEST_POLICY,DSR_IMAGE_STATE);CHECK(!app_requests);
 reset();hops[0].status=500;run(DSR_FETCH_STATUS,DSR_MANIFEST_INVALID,DSR_IMAGE_STATE);CHECK(!app_requests);
 reset();trust_fault=1;run(DSR_FETCH_CANCELLED,DSR_MANIFEST_OK,DSR_IMAGE_STATE);CHECK(!app_requests);
 reset();trust_fault=2;run(DSR_FETCH_TIMEOUT,DSR_MANIFEST_OK,DSR_IMAGE_STATE);CHECK(!app_requests);
 reset();hops[1].declared--;snprintf(hops[1].length_text,32,"%lld",(long long)hops[1].declared);run(DSR_FETCH_SIZE,DSR_MANIFEST_OK,-1);
 const int app_statuses[]={500,206};
 for(unsigned i=0;i<2;i++){reset();hops[1].status=app_statuses[i];run(DSR_FETCH_STATUS,DSR_MANIFEST_OK,DSR_IMAGE_OK);CHECK(!app_requests||(!hops[1].reads&&dsr_test_sha_calls()==2));}
 reset();hops[1].headers[1]=hops[1].headers[0];hops[1].count=2;run(DSR_FETCH_HEADERS,DSR_MANIFEST_OK,DSR_IMAGE_OK);CHECK(!app_requests||(!hops[1].reads&&dsr_test_sha_calls()==2));
 reset();hops[1].chunked=true;run(DSR_FETCH_HEADERS,DSR_MANIFEST_OK,DSR_IMAGE_OK);CHECK(!app_requests||(!hops[1].reads&&dsr_test_sha_calls()==2));
 reset();hops[1].length--;run(DSR_FETCH_INCOMPLETE,DSR_MANIFEST_OK,-1);
 reset();hops[1].complete=false;run(DSR_FETCH_INCOMPLETE,DSR_MANIFEST_OK,-1);
 reset();hops[1].chunked=true;hops[1].headers[0]=(header_t){"Transfer-Encoding","chunked"};hops[1].length--;run(DSR_FETCH_IMAGE,DSR_MANIFEST_OK,DSR_IMAGE_TRUNCATED);
 reset();hops[1].chunked=true;hops[1].headers[0]=(header_t){"Transfer-Encoding","chunked"};hops[1].length++;run(DSR_FETCH_SIZE,DSR_MANIFEST_OK,-1);
 reset();memcpy(changed,image,image_length);changed[2]^=1;hops[1].body=changed;run(DSR_FETCH_IMAGE,DSR_MANIFEST_OK,DSR_IMAGE_HASH);
 reset();memcpy(changed,image,image_length);unsigned markers=0;static const unsigned char marker[]="DSR-OTA:lcdwiki-es3c35p-n16r8:config-api-1";
 for(size_t i=0;i+sizeof(marker)<=image_length;i++)if(!memcmp(changed+i,marker,sizeof(marker))){changed[i+sizeof(marker)-1]=1;markers++;}
 CHECK(markers);hops[1].body=changed;run(DSR_FETCH_IMAGE,DSR_MANIFEST_OK,DSR_IMAGE_MARKER);
 reset();memcpy(changed,image,image_length);changed[0]=0;hops[1].body=changed;run(DSR_FETCH_IMAGE,DSR_MANIFEST_OK,DSR_IMAGE_HEADER);
 reset();oversize_return=true;run(DSR_FETCH_SIZE,DSR_MANIFEST_OK,-1);
 reset();fail_init=2;run(DSR_FETCH_NOMEM,DSR_MANIFEST_OK,-1);
 reset();fail_open=2;run(DSR_FETCH_IO,DSR_MANIFEST_OK,-1);
 reset();fail_read=(unsigned)manifest_lengths[0]+2;run(DSR_FETCH_IO,DSR_MANIFEST_OK,-1);
 reset();fail_close=2;run(DSR_FETCH_IO,DSR_MANIFEST_OK,-1);
 reset();fail_cleanup=2;run(DSR_FETCH_IO,DSR_MANIFEST_OK,-1);
 reset();cancel_app_read=2;run(DSR_FETCH_CANCELLED,DSR_MANIFEST_OK,-1);
 reset();app_delay=31000000;run(DSR_FETCH_TIMEOUT,DSR_MANIFEST_OK,-1);
 reset();post_finish=1;run(DSR_FETCH_CANCELLED,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 reset();post_finish=2;run(DSR_FETCH_TIMEOUT,DSR_MANIFEST_OK,DSR_IMAGE_OK);
 for(unsigned i=1;i<=3;i++){reset();dsr_test_sha_fault(i);run(DSR_FETCH_IMAGE,DSR_MANIFEST_OK,DSR_IMAGE_CRYPTO);}
 reset();dsr_test_sha_fault((unsigned)((image_length+1023)/1024)+3);run(DSR_FETCH_IMAGE,DSR_MANIFEST_OK,DSR_IMAGE_CRYPTO);
 for(unsigned i=1;i<=successful_allocations;i++){reset();fail_allocation=i;run(-1,-1,-1);}
 reset();cancel_flag=true;run(DSR_FETCH_CANCELLED,DSR_MANIFEST_INVALID,DSR_IMAGE_STATE);CHECK(!initialized);
 reset();wall=0;run(DSR_FETCH_CLOCK,DSR_MANIFEST_INVALID,DSR_IMAGE_STATE);CHECK(!initialized);
 dsr_ota_policy_t policy={"1.2.3",DSR_OTA_STABLE,0x600000};dsr_ota_verified_t out;dsr_manifest_result_t m;dsr_image_result_t im;
 reset();memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_fetch_app(initial,NULL,NULL,&out,&m,&im)==DSR_FETCH_ARGUMENT&&zero(&out)&&m==DSR_MANIFEST_INVALID&&im==DSR_IMAGE_STATE);
 CHECK(dsr_ota_fetch_app(initial,&policy,NULL,NULL,&m,&im)==DSR_FETCH_ARGUMENT&&m==DSR_MANIFEST_INVALID&&im==DSR_IMAGE_STATE);CHECK(!owned);
 cJSON_InitHooks(NULL);__real_free(image);__real_free(changed);
 printf("PASS: %u genuine manifest-to-app checks; %u allocation faults; exact signed app %lu B; HTTP/TLS SDK MODEL; crypto/hash genuine; trust=%s; no Flash/install proof\n",checks,successful_allocations,(unsigned long)image_length,
#ifdef DSR_DOWNLOAD_HOST_TEST_TRUST
 "HOST FIXTURE ONLY"
#else
 "PRODUCTION EMPTY, ZERO APP REQUESTS"
#endif
 );return 0;
}
