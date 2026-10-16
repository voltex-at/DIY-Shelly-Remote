/* Original pinned IDF HTTP parser + production fetch/verifier.
 * Header assembly, SDK transport and body cache below are MODELS, not IDF
 * esp_http_client/TLS. No sockets, certificate or hardware claims. */
#include "dsr_ota_fetch.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "http_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static unsigned checks,cases;
static bool signature_failure;
static const char initial[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/manifest.json";
static unsigned char manifest[2049];static size_t manifest_length;
typedef struct {
 http_parser parser;http_parser_settings settings;esp_http_client_config_t config;
 const char *wire;size_t length,offset,fragment;
 char key[256],value[9000];size_t keys,values;
 unsigned char cache[4096];size_t cached,read;
 bool value_phase,headers,complete,error,closed,cleaned,eof,coalesced;
 int64_t declared;
 unsigned status,header_callbacks,body_callbacks;
} client_t;
static client_t client;
static void dispatch(client_t *s)
{
 if(!s->keys)return;
 s->key[s->keys]=0;s->value[s->values]=0;
 esp_http_client_event_t e={.event_id=HTTP_EVENT_ON_HEADER,.client=s,
 .user_data=s->config.user_data,.header_key=s->key,.header_value=s->value};
 /* Deliberately match IDF's ignored header event return, not parser returns. */
 (void)s->config.event_handler(&e);s->header_callbacks++;
 memset(s->key,0xdd,sizeof(s->key));memset(s->value,0xdd,sizeof(s->value));
 s->keys=s->values=0;s->value_phase=false;
}
static int field(http_parser *p,const char *data,size_t n)
{client_t *s=p->data;if(s->value_phase)dispatch(s);if(n>=sizeof(s->key)-s->keys)return -1;memcpy(s->key+s->keys,data,n);s->keys+=n;return 0;}
static int value(http_parser *p,const char *data,size_t n)
{client_t *s=p->data;s->value_phase=true;if(n>=sizeof(s->value)-s->values)return -1;memcpy(s->value+s->values,data,n);s->values+=n;return 0;}
static int headers(http_parser *p)
{client_t *s=p->data;dispatch(s);s->headers=true;s->status=p->status_code;s->declared=(p->flags&F_CONTENTLENGTH)?(int64_t)p->content_length:-1;return 0;}
static int body(http_parser *p,const char *data,size_t n)
{client_t *s=p->data;if(n>sizeof(s->cache)-s->cached)return -1;memcpy(s->cache+s->cached,data,n);s->cached+=n;s->body_callbacks++;return 0;}
static int complete(http_parser *p)
{client_t *s=p->data;dispatch(s);s->complete=true;return 0;}
static void pump(client_t *s)
{
 if(s->error||s->complete||s->eof)return;
 size_t n=s->length-s->offset;if(n>s->fragment)n=s->fragment;
 size_t parsed=http_parser_execute(&s->parser,&s->settings,n?s->wire+s->offset:NULL,n);
 s->offset+=parsed;if(!n)s->eof=true;
 if(HTTP_PARSER_ERRNO(&s->parser)!=HPE_OK||parsed!=n)s->error=true;
}
esp_err_t esp_crt_bundle_attach(void *config){(void)config;return ESP_OK;}
time_t __wrap_time(time_t *out){time_t t=1767225600;if(out)*out=t;return t;}
int64_t esp_timer_get_time(void){return 0;}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c)
{
 CHECK(!client.closed&&!client.cleaned);client.config=*c;
 http_parser_init(&client.parser,HTTP_RESPONSE);client.parser.data=&client;
 http_parser_settings_init(&client.settings);client.settings.on_header_field=field;
 client.settings.on_header_value=value;client.settings.on_headers_complete=headers;
 client.settings.on_body=body;client.settings.on_message_complete=complete;
 return &client;
}
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c,const char *k,const char *v)
{CHECK(c==&client&&k&&v);return ESP_OK;}
esp_err_t esp_http_client_open(esp_http_client_handle_t c,int length)
{CHECK(c==&client&&!length);return ESP_OK;}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c)
{client_t *s=c;while(!s->headers&&!s->error&&!s->eof)pump(s);s->coalesced=s->cached>0;return s->error?-1:(s->parser.flags&F_CHUNKED)?0:s->declared<0?0:s->declared;}
int esp_http_client_get_status_code(esp_http_client_handle_t c){return (int)((client_t *)c)->status;}
int64_t esp_http_client_get_content_length(esp_http_client_handle_t c){return ((client_t *)c)->declared;}
bool esp_http_client_is_chunked_response(esp_http_client_handle_t c){return (((client_t *)c)->parser.flags&F_CHUNKED)!=0;}
int esp_http_client_read(esp_http_client_handle_t c,char *out,int capacity)
{client_t *s=c;CHECK(capacity==1);while(s->cached==s->read&&!s->error&&!s->complete&&!s->eof)pump(s);if(s->error)return -1;if(s->read<s->cached){*out=(char)s->cache[s->read++];return 1;}return 0;}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c)
{client_t *s=c;return s->complete&&!s->error;}
esp_err_t esp_http_client_close(esp_http_client_handle_t c)
{client_t *s=c;CHECK(!s->closed);s->closed=true;return ESP_OK;}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c)
{client_t *s=c;CHECK(s->closed&&!s->cleaned);s->cleaned=true;return ESP_OK;}
#ifdef DSR_FETCH_HOST_TEST_TRUST
const uint8_t *dsr_ota_trusted_key(const char *id)
{static const uint8_t key[32]={0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a};CHECK(client.closed&&client.cleaned);return !strcmp(id,"rfc8032-host-only")?key:NULL;}
#endif
static void run(const char *wire,size_t length,size_t fragment,bool accepted)
{
 memset(&client,0,sizeof(client));client.wire=wire;client.length=length;client.fragment=fragment;
 dsr_ota_policy_t policy={"1.2.3",DSR_OTA_STABLE,0x600000};dsr_ota_verified_t out;
 dsr_manifest_result_t why;memset(&out,0xa5,sizeof(out));
 dsr_ota_fetch_result_t result=dsr_ota_fetch_manifest(initial,&policy,NULL,&out,&why);
#ifdef DSR_FETCH_HOST_TEST_TRUST
 if(accepted){CHECK(result==DSR_FETCH_OK&&why==DSR_MANIFEST_OK);CHECK(out.size==1234&&!strcmp(out.version,"1.2.4"));}
#else
 if(accepted)CHECK(result==DSR_FETCH_MANIFEST&&why==DSR_MANIFEST_UNTRUSTED_KEY);
#endif
 if(!accepted){
  CHECK(result!=DSR_FETCH_OK);
  if(signature_failure){
#ifdef DSR_FETCH_HOST_TEST_TRUST
   CHECK(result==DSR_FETCH_MANIFEST&&why==DSR_MANIFEST_BAD_SIGNATURE);
#else
   CHECK(result==DSR_FETCH_MANIFEST&&why==DSR_MANIFEST_UNTRUSTED_KEY);
#endif
  }else CHECK(why==DSR_MANIFEST_INVALID);
 }
 if(result!=DSR_FETCH_OK){const unsigned char *bytes=(const unsigned char *)&out;for(size_t i=0;i<sizeof(out);i++)CHECK(!bytes[i]);}
 CHECK(client.closed&&client.cleaned);cases++;
}
static char wire[8192];
static size_t cl(const char *extra,int adjustment,size_t body_length)
{int n=snprintf(wire,sizeof(wire),"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n%s\r\n",(long)manifest_length+adjustment,extra);CHECK(n>0&&(size_t)n+body_length<sizeof(wire));memcpy(wire+n,manifest,body_length);return (size_t)n+body_length;}
static size_t chunked(const char *extra,const char *trailer,bool ending)
{
 int n=snprintf(wire,sizeof(wire),"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n%s\r\n",extra);CHECK(n>0);size_t used=(size_t)n;
 for(size_t pos=0;pos<manifest_length;){size_t take=manifest_length-pos;if(take>17)take=17;n=snprintf(wire+used,sizeof(wire)-used,"%zx;foo=bar\r\n",take);CHECK(n>0);used+=(size_t)n;memcpy(wire+used,manifest+pos,take);used+=take;memcpy(wire+used,"\r\n",2);used+=2;pos+=take;}
 if(ending){n=snprintf(wire+used,sizeof(wire)-used,"0\r\n%s\r\n",trailer);CHECK(n>0);used+=(size_t)n;}return used;
}
int main(int argc,char **argv)
{
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);manifest_length=fread(manifest,1,sizeof(manifest),f);CHECK(manifest_length&&manifest_length<=2048&&feof(f));fclose(f);
 CHECK(http_parser_version()==0x020700);
 size_t n=cl("",0,manifest_length);
 for(size_t fragment=1;fragment<=n;fragment++)run(wire,n,fragment,true);
 run(wire,n,n,true);CHECK(client.coalesced&&client.complete&&client.read==manifest_length);
 for(size_t cut=0;cut<n;cut++)run(wire,cut,1,false);
 /* Alter exact received signed metadata, retaining syntactically valid JSON. */
 char *hash=strstr(wire,"\"sha256\":\"");CHECK(hash);hash+=10;char saved=*hash;
 *hash=saved=='0'?'1':'0';signature_failure=true;run(wire,n,1,false);run(wire,n,n,false);
 signature_failure=false;*hash=saved;
 n=chunked("","X-Trailer: harmless\r\n",true);
 for(size_t fragment=1;fragment<=n;fragment++)run(wire,n,fragment,true);
 /* Truncation before the final chunk cannot reach the verifier. Trailer
  * truncation is tested separately below: chunk payload alone is insufficient. */
 n=chunked("","",true);for(size_t cut=0;cut<n;cut++)run(wire,cut,7,false);
 /* Invalid HTTP/framing must fail before verifier, for both trust providers. */
 const char *extras[]={"Content-Length: 1\r\n","Transfer-Encoding: chunked\r\n","Content-Encoding: gzip\r\n","Location: https://release-assets.githubusercontent.com/x\r\n"};
 for(size_t i=0;i<sizeof(extras)/sizeof(extras[0]);i++){n=cl(extras[i],0,manifest_length);run(wire,n,1,false);run(wire,n,n,false);}
 n=chunked("Transfer-Encoding: chunked\r\n","",true);run(wire,n,1,false);run(wire,n,n,false);
 n=chunked("Content-Length: 1\r\n","",true);run(wire,n,1,false);run(wire,n,n,false);
 n=cl("",1,manifest_length);run(wire,n,1,false);run(wire,n,n,false);
 n=cl("",0,manifest_length-1);run(wire,n,1,false);run(wire,n,n,false);
 n=chunked("","",false);run(wire,n,1,false);run(wire,n,n,false);
 n=chunked("","Content-Length: 1\r\n",true);run(wire,n,1,false);run(wire,n,n,false);
 const char *bad[]={"HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nQ\r\n", "HTTP/1.1 200 OK\r\nX-Bad\r\n\r\n", "HTTP/1.1 200 OK\r\n\r\n{}"};
 for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);i++){run(bad[i],strlen(bad[i]),1,false);run(bad[i],strlen(bad[i]),strlen(bad[i]),false);}
 printf("PASS: %u raw HTTP cases / %u checks; original IDF parser + real fetch/Ed25519; SDK assembly/cache/transport MODEL; trust=%s; no TLS/network/hardware proof\n",cases,checks,
#ifdef DSR_FETCH_HOST_TEST_TRUST
 "HOST ONLY"
#else
 "PRODUCTION EMPTY"
#endif
 );return 0;
}
