/* Genuine production fetch/verifier; SDK transport is a host fixture, not TLS. */
#include "dsr_ota_fetch.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
static unsigned checks,allocations,fail_allocation,owned;
static void *pointers[256];
static void assert_parse_phase(void);
#ifdef DSR_FETCH_HOST_TEST_TRUST
static void trust_lookup_control(void);
#endif
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
void *__real_malloc(size_t n);void *__real_calloc(size_t n,size_t size);void __real_free(void *p);
static void *track(void *p){if(p){unsigned i;for(i=0;i<256&&pointers[i];i++);CHECK(i<256);pointers[i]=p;owned++;}return p;}
void *__wrap_malloc(size_t n){allocations++;if(allocations>1)assert_parse_phase();return fail_allocation&&allocations==fail_allocation?NULL:track(__real_malloc(n));}
void *__wrap_calloc(size_t n,size_t size){allocations++;if(allocations>1)assert_parse_phase();return fail_allocation&&allocations==fail_allocation?NULL:track(__real_calloc(n,size));}
void __wrap_free(void *p){if(p){unsigned i;for(i=0;i<256&&pointers[i]!=p;i++);CHECK(i<256&&owned);pointers[i]=NULL;owned--;}__real_free(p);}
#ifdef DSR_FETCH_HOST_TEST_TRUST
const uint8_t *dsr_ota_trusted_key(const char *id)
{static const uint8_t key[32]={0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a};trust_lookup_control();return !strcmp(id,"rfc8032-host-only")?key:NULL;}
#endif
typedef struct {const char *key,*value;} header_t;
typedef struct {header_t headers[80];unsigned count;int status;int64_t declared;bool chunked,complete;const unsigned char *body;size_t length,offset;esp_http_client_config_t config;char url[2049];unsigned reads,closed,cleaned,sets;} hop_t;
static hop_t hops[2];static unsigned initialized,opens,header_fetches,close_count,cleanup_count,read_count;
static unsigned fail_init,fail_set,fail_open,fail_headers,fail_read,fail_close,fail_cleanup;
static unsigned headers_again;
static bool headers_partial,headers_cancel,headers_waiting;
static size_t read_chunk;
static bool overread,cancel_flag,complete_early,cancel_after_init,cancel_on_open;
static unsigned cancel_calls,cancel_after,cancel_on_read;
static int64_t monotonic,operation_delay;
#ifdef DSR_FETCH_HOST_TEST_TRUST
static unsigned trust_control;
static void trust_lookup_control(void)
{assert_parse_phase();if(trust_control==1)cancel_flag=true;else if(trust_control==2)monotonic+=31000000;}
#endif
static time_t wall;
static unsigned char fixture[3][2049];static size_t fixture_length[3];
static const char initial[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/manifest.json";
static const char cdn[]="https://release-assets.githubusercontent.com/123/asset?token=fixture%2Fopaque";
static void assert_parse_phase(void){CHECK(close_count&&close_count==cleanup_count);for(unsigned i=0;i<initialized;i++)CHECK(hops[i].closed&&hops[i].cleaned);}
esp_err_t esp_crt_bundle_attach(void *config){(void)config;return ESP_OK;}
time_t __wrap_time(time_t *out){if(out)*out=wall;return wall;}
int64_t esp_timer_get_time(void){return monotonic;}
static void delay(void){monotonic+=operation_delay;}
static hop_t *hop(esp_http_client_handle_t client){hop_t *h=client;CHECK(h==&hops[0]||h==&hops[1]);CHECK(!h->cleaned);return h;}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c)
{
 initialized++;CHECK(initialized<=2);if(initialized==fail_init)return NULL;
 hop_t *h=&hops[initialized-1];CHECK(c->url&&strlen(c->url)<=2048);
 CHECK(c->method==HTTP_METHOD_GET&&c->transport_type==HTTP_TRANSPORT_OVER_SSL);
 CHECK(c->crt_bundle_attach==esp_crt_bundle_attach&&!c->skip_cert_common_name_check&&!c->common_name&&!c->cert_pem);
 CHECK(c->disable_auto_redirect&&c->max_authorization_retries==-1&&c->timeout_ms==5000&&c->buffer_size==1024&&c->buffer_size_tx==4096);
 CHECK(!c->username&&!c->password&&c->user_agent&&!strcmp(c->user_agent,"DIY-Shelly-Remote-manifest/1")&&c->event_handler&&c->user_data);
 CHECK(!c->client_cert_pem&&!c->client_key_pem&&!c->client_key_password&&!c->client_key_password_len&&!c->auth_type&&!c->use_global_ca_store&&!c->keep_alive_enable);
 if(initialized==2)CHECK(hops[0].closed&&hops[0].cleaned);
 h->config=*c;strcpy(h->url,c->url);delay();if(cancel_after_init)cancel_flag=true;return h;
}
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c,const char *key,const char *value)
{hop_t *h=hop(c);h->sets++;CHECK((!strcmp(key,"Accept-Encoding")&&!strcmp(value,"identity"))||(!strcmp(key,"Accept")&&!strcmp(value,"application/json")));return h->sets==fail_set?ESP_FAIL:ESP_OK;}
esp_err_t esp_http_client_open(esp_http_client_handle_t c,int write_length)
{hop_t *h=hop(c);CHECK(!write_length);opens++;delay();if(cancel_on_open){cancel_flag=true;esp_http_client_event_t e={.event_id=HTTP_EVENT_ON_CONNECTED,.client=c,.user_data=h->config.user_data};(void)h->config.event_handler(&e);}return opens==fail_open?ESP_FAIL:ESP_OK;}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c)
{hop_t *h=hop(c);header_fetches++;delay();if(header_fetches==fail_headers)return -1;
 unsigned first=headers_partial&&header_fetches>1?1u:0u,last=h->count;
 headers_waiting=headers_again>0;
 if(headers_waiting){headers_again--;monotonic+=5000000;if(headers_cancel)cancel_flag=true;last=headers_partial&&header_fetches==1?1u:0u;}
 for(unsigned i=first;i<last;i++){esp_http_client_event_t e={.event_id=HTTP_EVENT_ON_HEADER,.client=c,.user_data=h->config.user_data,.header_key=(char *)h->headers[i].key,.header_value=(char *)h->headers[i].value};(void)h->config.event_handler(&e);}
 return headers_waiting?-ESP_ERR_HTTP_EAGAIN:h->chunked?0:h->declared;}
int esp_http_client_get_status_code(esp_http_client_handle_t c){return headers_waiting?-1:hop(c)->status;}
int64_t esp_http_client_get_content_length(esp_http_client_handle_t c){hop_t *h=hop(c);return h->chunked?-1:h->declared;}
bool esp_http_client_is_chunked_response(esp_http_client_handle_t c){return hop(c)->chunked;}
int esp_http_client_read(esp_http_client_handle_t c,char *out,int capacity)
{hop_t *h=hop(c);CHECK(capacity>0&&capacity<=2049);h->reads++;read_count++;delay();if(read_count==cancel_on_read)cancel_flag=true;if(read_count==fail_read)return -1;if(overread)return capacity+1;size_t n=h->length-h->offset;if(n>(size_t)capacity)n=(size_t)capacity;if(n>read_chunk)n=read_chunk;memcpy(out,h->body+h->offset,n);h->offset+=n;return (int)n;}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c){hop_t *h=hop(c);return complete_early||(h->complete&&h->offset==h->length);}
esp_err_t esp_http_client_close(esp_http_client_handle_t c){hop_t *h=hop(c);h->closed++;CHECK(h->closed==1);close_count++;return close_count==fail_close?ESP_FAIL:ESP_OK;}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c){hop_t *h=hop(c);h->cleaned++;CHECK(h->closed==1&&h->cleaned==1);cleanup_count++;return cleanup_count==fail_cleanup?ESP_FAIL:ESP_OK;}
static bool cancelled(void *context){CHECK(context==&cancel_flag);cancel_calls++;return cancel_flag||(cancel_after&&cancel_calls>=cancel_after);}
static void add(unsigned index,const char *key,const char *value){hop_t *h=&hops[index];CHECK(h->count<80);h->headers[h->count++]=(header_t){key,value};}
static char lengths[2][32];
static void content(unsigned index,const unsigned char *data,size_t length)
{hop_t *h=&hops[index];h->body=data;h->length=length;h->declared=(int64_t)length;snprintf(lengths[index],sizeof(lengths[index]),"%lu",(unsigned long)length);h->count=0;add(index,"Content-Length",lengths[index]);}
static void reset(void)
{CHECK(!owned);memset(hops,0,sizeof(hops));for(unsigned i=0;i<2;i++){hops[i].status=200;hops[i].complete=true;content(i,fixture[0],fixture_length[0]);}initialized=opens=header_fetches=close_count=cleanup_count=read_count=0;fail_init=fail_set=fail_open=fail_headers=fail_read=fail_close=fail_cleanup=0;headers_again=0;headers_partial=headers_cancel=headers_waiting=false;read_chunk=2049;overread=cancel_flag=complete_early=cancel_after_init=cancel_on_open=false;cancel_calls=cancel_after=cancel_on_read=0;monotonic=operation_delay=0;wall=1767225600;allocations=fail_allocation=0;}
static bool zero(const dsr_ota_verified_t *out){const unsigned char *b=(const unsigned char *)out;for(size_t i=0;i<sizeof(*out);i++)if(b[i])return false;return true;}
static dsr_ota_fetch_result_t run(const char *url,int expectation,dsr_manifest_result_t why_expected)
{
 dsr_ota_policy_t policy={"1.2.3",DSR_OTA_STABLE,0x600000};dsr_ota_fetch_control_t control={cancelled,&cancel_flag};dsr_ota_verified_t out;memset(&out,0xa5,sizeof(out));dsr_manifest_result_t why=(dsr_manifest_result_t)99;
 dsr_ota_fetch_result_t result=dsr_ota_fetch_manifest(url,&policy,&control,&out,&why);
#ifndef DSR_FETCH_HOST_TEST_TRUST
 if(expectation==DSR_FETCH_OK){expectation=DSR_FETCH_MANIFEST;why_expected=DSR_MANIFEST_UNTRUSTED_KEY;}
 if(why_expected==DSR_MANIFEST_BAD_SIGNATURE)why_expected=DSR_MANIFEST_UNTRUSTED_KEY;
#endif
 if(expectation==-2)CHECK(result!=DSR_FETCH_OK);
 else if(expectation<0)CHECK(result!=DSR_FETCH_OK&&result!=DSR_FETCH_MANIFEST);
 else if(result!=(dsr_ota_fetch_result_t)expectation){fprintf(stderr,"fetch result expected=%d actual=%d why=%d\n",expectation,result,why);abort();}
 CHECK(why==why_expected);
 if(result==DSR_FETCH_OK)CHECK(out.size==1234&&!strcmp(out.version,"1.2.4")&&!strcmp(out.url,"https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin"));else CHECK(zero(&out));
 CHECK(!owned);CHECK(close_count==cleanup_count);for(unsigned i=0;i<initialized&&i<2;i++)if(hops[i].config.url)CHECK(hops[i].closed==1&&hops[i].cleaned==1);
 return result;
}
static uint32_t integer(FILE *f){uint8_t b[4];CHECK(fread(b,1,4,f)==4);return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
int main(int argc,char **argv)
{
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);for(unsigned i=0;i<3;i++){fixture_length[i]=integer(f);CHECK(fixture_length[i]<=2048&&fread(fixture[i],1,fixture_length[i],f)==fixture_length[i]);}fclose(f);
 cJSON_Hooks hooks={__wrap_malloc,__wrap_free};cJSON_InitHooks(&hooks);
 reset();run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(initialized==1&&hops[0].offset==fixture_length[0]);unsigned allocations_success=allocations;
 reset();headers_again=2;run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(initialized==1&&opens==1&&header_fetches==3);
 reset();headers_again=2;headers_partial=true;run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(initialized==1&&opens==1&&header_fetches==3);
 reset();headers_again=10;run(initial,DSR_FETCH_TIMEOUT,DSR_MANIFEST_INVALID);CHECK(initialized==1&&opens==1&&header_fetches==6&&!read_count);
 reset();headers_again=2;headers_cancel=true;run(initial,DSR_FETCH_CANCELLED,DSR_MANIFEST_INVALID);CHECK(initialized==1&&opens==1&&header_fetches==1&&!read_count);
 for(size_t n=1;n<=64;n++){reset();read_chunk=n;run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(hops[0].offset==fixture_length[0]);}
 reset();content(0,fixture[1],fixture_length[1]);run(initial,DSR_FETCH_MANIFEST,DSR_MANIFEST_BAD_SIGNATURE);
 reset();content(0,fixture[2],fixture_length[2]);run(initial,DSR_FETCH_MANIFEST,DSR_MANIFEST_UNTRUSTED_KEY);
 const char *bad_urls[]={"http://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/x","https://github.com.evil/voltex-at/DIY-Shelly-Remote/releases/download/v1/x","https://github.com/other/repo/releases/download/v1/x","https://user@github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/x","https://github.com:443/voltex-at/DIY-Shelly-Remote/releases/download/v1/x","https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/x?query","https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/x#fragment","https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/../x","https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/%78","https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/"};
 for(unsigned i=0;i<sizeof(bad_urls)/sizeof(bad_urls[0]);i++){reset();run(bad_urls[i],DSR_FETCH_URL,DSR_MANIFEST_INVALID);CHECK(!initialized);}
 char url_limit[258];strcpy(url_limit,initial);size_t initial_length=strlen(url_limit);memset(url_limit+initial_length,'a',256-initial_length);url_limit[256]=0;
 reset();run(url_limit,DSR_FETCH_OK,DSR_MANIFEST_OK);url_limit[256]='a';url_limit[257]=0;reset();run(url_limit,DSR_FETCH_URL,DSR_MANIFEST_INVALID);CHECK(!initialized);
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(initialized==2&&!strcmp(hops[1].url,cdn));
 char cdn_limit[2050];strcpy(cdn_limit,cdn);size_t cdn_length=strlen(cdn_limit);memset(cdn_limit+cdn_length,'a',2048-cdn_length);cdn_limit[2048]=0;
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn_limit);run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(initialized==2);
 cdn_limit[2048]='a';cdn_limit[2049]=0;reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn_limit);run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);CHECK(initialized==1);
 const int redirected_statuses[]={301,303,307,308};
 for(unsigned i=0;i<4;i++){reset();hops[0].status=redirected_statuses[i];hops[0].count=0;add(0,"Location",cdn);run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);}
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);fail_init=2;run(initial,DSR_FETCH_NOMEM,DSR_MANIFEST_INVALID);
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);fail_open=2;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);fail_headers=2;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);fail_close=2;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);fail_cleanup=2;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 /* GitHub also redirects to objects.githubusercontent.com (observed on dev-v0.7.38). */
 static const char objects_cdn[]="https://objects.githubusercontent.com/github-production-release-asset-2e65be/1419964327/276c096a?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Signature=ab12";
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",objects_cdn);run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(initialized==2);
 const char *bad_objects[]={"http://objects.githubusercontent.com/path","https://objects.githubusercontent.com.evil/path","https://objects-githubusercontent.com/path","https://objects.githubusercontent.co/path","https://user@objects.githubusercontent.com/path","https://objects.githubusercontent.com:443/path","https://objects.githubusercontent.com/","https://objects.githubusercontent.com/a/../b","https://evil.com/objects.githubusercontent.com/path"};
 for(unsigned i=0;i<sizeof(bad_objects)/sizeof(bad_objects[0]);i++){reset();hops[0].status=302;hops[0].count=0;add(0,"Location",bad_objects[i]);run(initial,-1,DSR_MANIFEST_INVALID);CHECK(initialized==1);}
 const char *bad_redirects[]={"http://release-assets.githubusercontent.com/path","https://release-assets.githubusercontent.com.evil/path","https://user@release-assets.githubusercontent.com/path","https://release-assets.githubusercontent.com:443/path","https://release-assets.githubusercontent.com/path#fragment","https://release-assets.githubusercontent.com/path?bad=%0d","https://release-assets.githubusercontent.com/path?bad=%00","https://release-assets.githubusercontent.com/path?bad=%7f","https://release-assets.githubusercontent.com/path?bad=%q0","https://release-assets.githubusercontent.com/path?bad=%","/relative/path","https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1/x"};
 for(unsigned i=0;i<sizeof(bad_redirects)/sizeof(bad_redirects[0]);i++){reset();hops[0].status=302;hops[0].count=0;add(0,"Location",bad_redirects[i]);run(initial,-1,DSR_MANIFEST_INVALID);CHECK(initialized==1);}
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);hops[1].status=302;hops[1].count=0;add(1,"Location",cdn);run(initial,DSR_FETCH_REDIRECT,DSR_MANIFEST_INVALID);CHECK(initialized==2);
 reset();hops[0].status=302;hops[0].count=0;run(initial,-1,DSR_MANIFEST_INVALID);
 reset();hops[0].status=302;hops[0].count=0;add(0,"Location",cdn);add(0,"location",cdn);run(initial,-1,DSR_MANIFEST_INVALID);
 const int statuses[]={204,206,301,303,307,308,400,401,404,500};
 for(unsigned i=0;i<sizeof(statuses)/sizeof(statuses[0]);i++){reset();hops[0].status=statuses[i];run(initial,-1,DSR_MANIFEST_INVALID);}
 const char *invalid_lengths[]={"0","-1","+1","01","1.0","1e3"," 123","123 ","18446744073709551616","2049"};
 for(unsigned i=0;i<sizeof(invalid_lengths)/sizeof(invalid_lengths[0]);i++){reset();hops[0].headers[0].value=invalid_lengths[i];run(initial,-1,DSR_MANIFEST_INVALID);}
 reset();hops[0].count=0;run(initial,-1,DSR_MANIFEST_INVALID);
 reset();add(0,"content-length",lengths[0]);run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();add(0,"Transfer-Encoding","chunked");hops[0].chunked=true;run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();hops[0].count=0;hops[0].chunked=true;add(0,"Transfer-Encoding","chunked");run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 reset();hops[0].count=0;add(0,"Transfer-Encoding","chunked");run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();hops[0].chunked=true;run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();hops[0].declared--;run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();hops[0].count=0;hops[0].chunked=true;add(0,"Transfer-Encoding","chunked");add(0,"transfer-encoding","chunked");run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 const char *encodings[]={"gzip","chunked, gzip","identity","chunked,chunked"};
 for(unsigned i=0;i<sizeof(encodings)/sizeof(encodings[0]);i++){reset();hops[0].count=0;hops[0].chunked=true;add(0,"Transfer-Encoding",encodings[i]);run(initial,-1,DSR_MANIFEST_INVALID);}
 reset();add(0,"Content-Encoding","identity");run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 reset();add(0,"Content-Encoding","gzip");run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();add(0,"Content-Encoding","identity");add(0,"content-encoding","identity");run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();add(0,"Location",cdn);run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();hops[0].headers[0].key="cOnTeNt-LeNgTh";run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 char ows[40];reset();snprintf(ows,sizeof(ows)," \t%s \t",lengths[0]);hops[0].headers[0].value=ows;run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 reset();hops[0].count=0;hops[0].chunked=true;add(0,"tRaNsFeR-EnCoDiNg"," \tChUnKeD \t");run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 char long_key[130],long_value[8194];memset(long_key,'x',129);long_key[129]=0;memset(long_value,'a',8193);long_value[8193]=0;
 reset();add(0,long_key,"x");run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();add(0,"X",long_value);run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();for(unsigned i=0;i<64;i++)add(0,"X","x");run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();for(unsigned i=0;i<63;i++)add(0,"X","x");run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 long_value[8192]=0;reset();add(0,"X",long_value);add(0,"Y",long_value);run(initial,DSR_FETCH_HEADERS,DSR_MANIFEST_INVALID);
 reset();add(0,"X",long_value);run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 long_key[128]=0;reset();add(0,long_key,"x");run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 unsigned char padded[2049];memcpy(padded,fixture[0],fixture_length[0]);memset(padded+fixture_length[0],' ',sizeof(padded)-fixture_length[0]);
 reset();content(0,padded,2048);run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);
 reset();content(0,padded,2049);run(initial,DSR_FETCH_SIZE,DSR_MANIFEST_INVALID);
 reset();content(0,padded,2049);hops[0].count=0;hops[0].chunked=true;add(0,"Transfer-Encoding","chunked");run(initial,DSR_FETCH_SIZE,DSR_MANIFEST_INVALID);
 reset();hops[0].length--;run(initial,-1,DSR_MANIFEST_INVALID);
 reset();hops[0].declared--;snprintf(lengths[0],32,"%lld",(long long)hops[0].declared);run(initial,-1,DSR_MANIFEST_INVALID);
 reset();hops[0].complete=false;run(initial,DSR_FETCH_INCOMPLETE,DSR_MANIFEST_INVALID);
 reset();complete_early=true;read_chunk=1;run(initial,DSR_FETCH_OK,DSR_MANIFEST_OK);CHECK(hops[0].offset==fixture_length[0]);
 reset();overread=true;run(initial,-1,DSR_MANIFEST_INVALID);
 for(unsigned n=1;n<=2;n++){reset();fail_set=n;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);CHECK(!opens);}
 reset();fail_init=1;run(initial,DSR_FETCH_NOMEM,DSR_MANIFEST_INVALID);
 reset();fail_open=1;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();fail_headers=1;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();fail_read=1;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();fail_close=1;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();fail_cleanup=1;run(initial,DSR_FETCH_IO,DSR_MANIFEST_INVALID);
 reset();cancel_flag=true;run(initial,DSR_FETCH_CANCELLED,DSR_MANIFEST_INVALID);CHECK(!initialized);
 reset();cancel_on_read=2;run(initial,DSR_FETCH_CANCELLED,DSR_MANIFEST_INVALID);
 reset();operation_delay=31000000;run(initial,DSR_FETCH_TIMEOUT,DSR_MANIFEST_INVALID);CHECK(!opens);
 reset();cancel_after_init=true;run(initial,DSR_FETCH_CANCELLED,DSR_MANIFEST_INVALID);CHECK(!opens);
 reset();cancel_on_open=true;fail_open=1;run(initial,DSR_FETCH_CANCELLED,DSR_MANIFEST_INVALID);
#ifdef DSR_FETCH_HOST_TEST_TRUST
 /* Valid cryptography is insufficient when cancellation/deadline expires
  * during verification. Only FETCH_OK publishes verified metadata. */
 reset();trust_control=1;run(initial,DSR_FETCH_CANCELLED,DSR_MANIFEST_OK);trust_control=0;
 reset();trust_control=2;run(initial,DSR_FETCH_TIMEOUT,DSR_MANIFEST_OK);trust_control=0;
#endif
 reset();wall=0;run(initial,DSR_FETCH_CLOCK,DSR_MANIFEST_INVALID);CHECK(!initialized);
 reset();wall=(time_t)-1;run(initial,DSR_FETCH_CLOCK,DSR_MANIFEST_INVALID);CHECK(!initialized);
 /* Fail each wrapper-owned allocation, including the real parser. */
 for(unsigned i=1;i<=allocations_success;i++){reset();fail_allocation=i;dsr_ota_fetch_result_t r=run(initial,-2,DSR_MANIFEST_INVALID);(void)r;}
 reset();dsr_ota_verified_t out;dsr_manifest_result_t why;dsr_ota_policy_t policy={"1.2.3",DSR_OTA_STABLE,0x600000};
 memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_fetch_manifest(initial,NULL,NULL,&out,&why)==DSR_FETCH_ARGUMENT&&zero(&out)&&why==DSR_MANIFEST_INVALID);
 CHECK(dsr_ota_fetch_manifest(initial,&policy,NULL,NULL,&why)==DSR_FETCH_ARGUMENT&&why==DSR_MANIFEST_INVALID);
 memset(&out,0xa5,sizeof(out));CHECK(dsr_ota_fetch_manifest(NULL,&policy,NULL,&out,&why)==DSR_FETCH_URL&&zero(&out));
 reset();dsr_ota_fetch_result_t optional=dsr_ota_fetch_manifest(initial,&policy,NULL,&out,NULL);
#ifdef DSR_FETCH_HOST_TEST_TRUST
 CHECK(optional==DSR_FETCH_OK);
#else
 CHECK(optional==DSR_FETCH_MANIFEST&&zero(&out));
#endif
 CHECK(!owned&&close_count==cleanup_count);
 cJSON_InitHooks(NULL);
 printf("PASS: %u genuine bounded HTTPS-fetch checks; %u real allocation faults; TLS CONFIG/HTTP SDK FIXTURE ONLY; real Ed25519/parser, redirects/framing/cache/EOF/bounds/cancellation/clock/OOM; trust=%s; no TLS/network/Flash proof\n",checks,allocations_success,
#ifdef DSR_FETCH_HOST_TEST_TRUST
 "HOST ONLY"
#else
 "PRODUCTION (fixture key not trusted)"
#endif
 );return 0;
}
