/* Actual extracted panel functions + genuine SHA/activity. HTTP, lock, random,
 * socket and NVS providers are MODEL; no TLS/device/physical reboot proof. */
#include "dsr_panel_pairing.h"
#include "dsr_activity_internal.h"
#include "dsr_platform.h"
#include <openssl/evp.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
typedef void *SemaphoreHandle_t;
typedef struct {int method;size_t content_len;} httpd_req_t;
typedef unsigned nvs_handle_t;
#define HTTP_GET 0
#define HTTP_POST 1
#define HTTPD_SOCK_ERR_TIMEOUT -3
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define pdTRUE 1
#define pdFALSE 0
#define portMAX_DELAY UINT32_MAX
static int lock_marker;static SemaphoreHandle_t lock=&lock_marker;
static bool boot_ready=true,panel_ready=true,lock_busy,held;
static char pairing[17];static int64_t pair_until,ota_until;static unsigned attempts;
static unsigned char token_hash[32];static bool has_token,revocation_unconfirmed;
static int64_t now=1000000;
static unsigned fault,opens,erases,commits,gets,sets,closes,cookies,random_counter;
static bool durable_present;static unsigned operation;
static unsigned char durable_hash[32];
static const char old_token[]="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
static char cookie[160],saved_cookie[160],status[40],answer[256];
static const char *payload,*host="shelly-remote.local",*origin="https://shelly-remote.local",*site="same-origin",*request_header="1";
static uint32_t peer_ip=0xc0a80102;
static bool dsr_boot_ready(void){return boot_ready;}
static int64_t esp_timer_get_time(void){return now;}
static int xSemaphoreTake(SemaphoreHandle_t h,unsigned timeout){CHECK(h==lock&&(timeout==0||timeout==portMAX_DELAY));if(lock_busy)return pdFALSE;CHECK(!held);held=true;return pdTRUE;}
static int xSemaphoreGive(SemaphoreHandle_t h){CHECK(h==lock&&held);held=false;return pdTRUE;}
static void esp_fill_random(void *out,size_t n){unsigned char *p=out;for(size_t i=0;i<n;i++)p[i]=(unsigned char)++random_counter;}
static int mbedtls_sha256(const unsigned char *data,size_t n,unsigned char out[32],int is224){unsigned length;CHECK(!is224&&EVP_Digest(data,n,out,&length,EVP_sha256(),NULL)==1&&length==32);return 0;}
static void admission_open(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void nvs_gate(void){if(operation)CHECK(held&&!dsr_activity_restart_reserve());if(operation==1)CHECK(!has_token&&revocation_unconfirmed&&!pairing[0]&&!pair_until&&!ota_until&&!attempts&&!memcmp(token_hash,(unsigned char[32]){0},32));}
static esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *out)
{CHECK(!strcmp(name,"dsr_panel")&&(mode==NVS_READONLY||mode==NVS_READWRITE));nvs_gate();opens++;if(fault==1)return ESP_FAIL;*out=7;return ESP_OK;}
static esp_err_t nvs_erase_key(nvs_handle_t h,const char *key)
{CHECK(h==7&&!strcmp(key,"token_hash"));nvs_gate();erases++;if(fault==2)return ESP_FAIL;if(!durable_present)return ESP_ERR_NVS_NOT_FOUND;if(fault!=6)durable_present=false;return fault==3?ESP_FAIL:ESP_OK;}
static esp_err_t nvs_commit(nvs_handle_t h){CHECK(h==7);nvs_gate();commits++;return fault==4?ESP_FAIL:ESP_OK;}
static esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *n)
{CHECK(h==7&&!strcmp(key,"token_hash")&&n);nvs_gate();gets++;if(fault==5)return ESP_FAIL;if(!durable_present)return ESP_ERR_NVS_NOT_FOUND;if(!out){*n=32;return ESP_OK;}if(*n<32)return ESP_ERR_INVALID_SIZE;memcpy(out,durable_hash,32);*n=32;return ESP_OK;}
static esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t n)
{CHECK(h==7&&!strcmp(key,"token_hash")&&n==32&&data);nvs_gate();sets++;memcpy(durable_hash,data,32);durable_present=true;return fault==7?ESP_FAIL:ESP_OK;}
static void nvs_close(nvs_handle_t h){CHECK(h==7);nvs_gate();closes++;}
static int httpd_req_get_hdr_value_str(httpd_req_t *r,const char *key,char *out,size_t n)
{CHECK(r);const char *v=!strcmp(key,"Cookie")?cookie:!strcmp(key,"Host")?host:!strcmp(key,"Origin")?origin:!strcmp(key,"Sec-Fetch-Site")?site:!strcmp(key,"X-DSR-Request")?request_header:NULL;if(!v||strlen(v)>=n)return ESP_FAIL;strcpy(out,v);return ESP_OK;}
static int httpd_req_recv(httpd_req_t *r,char *out,size_t n){CHECK(r&&payload&&n==strlen(payload));memcpy(out,payload,n);return (int)n;}
static esp_err_t httpd_resp_set_hdr(httpd_req_t *r,const char *key,const char *value){CHECK(r&&!strcmp(key,"Set-Cookie"));snprintf(saved_cookie,sizeof(saved_cookie),"%s",value);cookies++;return ESP_OK;}
static esp_err_t json(httpd_req_t *r,const char *s,const char *text){CHECK(r);snprintf(status,sizeof(status),"%s",s);snprintf(answer,sizeof(answer),"%s",text);return ESP_OK;}
static int httpd_req_to_sockfd(httpd_req_t *r){CHECK(r);return 17;}
int getpeername(int fd,struct sockaddr *out,socklen_t *n){CHECK(fd==17&&*n>=sizeof(struct sockaddr_in));struct sockaddr_in peer={.sin_family=AF_INET,.sin_addr.s_addr=htonl(peer_ip)};memcpy(out,&peer,sizeof(peer));*n=sizeof(peer);return 0;}
void dsr_wifi_get_info(dsr_wifi_info_t *out){memset(out,0,sizeof(*out));strcpy(out->ip,"192.168.1.42");}
#include "actual_panel.inc"
/* Actual startup NVS read statement, but the lifecycle invocation is a MODEL. */
static void model_boot_reload(void){has_token=false;revocation_unconfirmed=false;memset(token_hash,0,32);memset(pairing,0,17);pair_until=0;
#include "actual_boot_read.inc"
}
static void reset(void)
{admission_open();CHECK(!held);boot_ready=panel_ready=true;lock=&lock_marker;lock_busy=false;fault=opens=erases=commits=gets=sets=closes=cookies=0;has_token=durable_present=true;revocation_unconfirmed=false;memcpy(pairing,"0123456789abcdef",17);pair_until=now+120000000;attempts=0;mbedtls_sha256((const unsigned char *)old_token,64,token_hash,0);memcpy(durable_hash,token_hash,32);snprintf(cookie,sizeof(cookie),"dsr_session=%s",old_token);saved_cookie[0]=0;}
static dsr_panel_pairing_snapshot_t snapshot(void){unsigned before=opens+erases+commits+gets+sets+closes;dsr_panel_pairing_snapshot_t s;memset(&s,0xa5,sizeof(s));CHECK(sizeof(s)==2*sizeof(bool)&&dsr_panel_pairing_snapshot(&s));CHECK(opens+erases+commits+gets+sets+closes==before);return s;}
static esp_err_t revoke(void){operation=true;esp_err_t r=dsr_panel_pairing_revoke();operation=false;CHECK(!held);admission_open();return r;}
int main(void)
{
 httpd_req_t request={.method=HTTP_POST,.content_len=16};
 reset();CHECK(authorized(&request)&&local_request(&request));CHECK(snapshot().paired&&!snapshot().revocation_unconfirmed);
 CHECK(!dsr_panel_pairing_snapshot(NULL));
 for(unsigned unavailable=0;unavailable<4;unavailable++){
  reset();if(unavailable==0)boot_ready=false;else if(unavailable==1)panel_ready=false;else if(unavailable==2)lock=NULL;else lock_busy=true;
  dsr_panel_pairing_snapshot_t s;memset(&s,0xa5,sizeof(s));CHECK(!dsr_panel_pairing_snapshot(&s)&&!s.paired&&!s.revocation_unconfirmed&&!opens);CHECK(dsr_panel_pairing_revoke()!=ESP_OK&&!opens&&has_token);lock=&lock_marker;lock_busy=false;
 }
 for(unsigned failure=1;failure<=6;failure++){
  reset();fault=failure;CHECK(revoke()!=ESP_OK);CHECK(!has_token&&!pair_until&&!pairing[0]&&!memcmp(token_hash,(unsigned char[32]){0},32));CHECK(!authorized(&request));CHECK(!snapshot().paired&&snapshot().revocation_unconfirmed);CHECK(closes==(failure==1?0u:1u));
  payload="0123456789abcdef";CHECK(pair(&request)==ESP_OK&&!strcmp(status,"401 Unauthorized")&&!cookies);
  bool persisted_absence=!durable_present;fault=0;model_boot_reload();CHECK(authorized(&request)==!persisted_absence);
 }
 reset();CHECK(revoke()==ESP_OK&&!durable_present&&!authorized(&request)&&!snapshot().paired&&!snapshot().revocation_unconfirmed);CHECK(erases==1&&commits==1&&gets>=1&&closes==1);model_boot_reload();CHECK(!authorized(&request));
 CHECK(revoke()==ESP_OK&&!snapshot().paired&&!snapshot().revocation_unconfirmed);
 reset();fault=2;CHECK(revoke()!=ESP_OK&&snapshot().revocation_unconfirmed);fault=0;CHECK(revoke()==ESP_OK&&!snapshot().revocation_unconfirmed&&!snapshot().paired&&!durable_present);
 reset();has_token=durable_present=false;memset(token_hash,0,sizeof(token_hash));CHECK(revoke()==ESP_OK&&erases==1&&commits==1&&gets==1&&!snapshot().paired&&!snapshot().revocation_unconfirmed);
 reset();CHECK(dsr_activity_restart_reserve());CHECK(dsr_panel_pairing_revoke()!=ESP_OK&&!opens&&has_token);dsr_activity_restart_cancel();admission_open();
 for(unsigned pair_fault=1;pair_fault<=3;pair_fault++){
  reset();fault=2;CHECK(revoke()!=ESP_OK);char fresh[17];dsr_panel_pair_begin(fresh);payload=fresh;fault=pair_fault==1?1:pair_fault==2?7:4;operation=2;CHECK(pair(&request)==ESP_OK&&!strcmp(status,"503 Service Unavailable"));operation=0;CHECK(!has_token&&!cookies&&!authorized(&request)&&snapshot().revocation_unconfirmed);admission_open();
 }
 reset();fault=2;CHECK(revoke()!=ESP_OK&&!authorized(&request));fault=0;char fresh[17];dsr_panel_pair_begin(fresh);CHECK(strlen(fresh)==16&&strcmp(fresh,"0123456789abcdef"));payload=fresh;operation=2;CHECK(pair(&request)==ESP_OK&&!strcmp(status,"200 OK"));operation=0;CHECK(has_token&&durable_present&&cookies==1&&!snapshot().revocation_unconfirmed);CHECK(!authorized(&request));snprintf(cookie,sizeof(cookie),"%s",saved_cookie);CHECK(authorized(&request));model_boot_reload();CHECK(authorized(&request));admission_open();
 reset();peer_ip=0x08080808;CHECK(!local_request(&request));peer_ip=0xc0a80102;host="evil.invalid";CHECK(!local_request(&request));host="shelly-remote.local";origin="https://evil.invalid";CHECK(!local_request(&request));origin="https://shelly-remote.local";site="cross-site";CHECK(!local_request(&request));site="same-origin";request_header=NULL;CHECK(!local_request(&request));request_header="1";CHECK(local_request(&request));
 printf("PASS: %u actual panel revoke/snapshot/pair/cookie/local-request checks; genuine SHA/admission; NVS/HTTP/socket/boot MODEL, no device/TLS/reboot proof\n",checks);return 0;
}
