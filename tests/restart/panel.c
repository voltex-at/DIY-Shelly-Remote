/* Extracted actual pair/restart handlers. HTTP body/locks/NVS are MODEL;
 * genuine activity admission + SHA256. Outer authentication is NOT executed. */
#include "dsr_activity.h"
#include "dsr_activity_internal.h"
#include "dsr_restart.h"
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,opens,sets,commits,closes,cookies,requests;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
typedef struct {unsigned unused;} httpd_req_t;
typedef int nvs_handle_t;
#define NVS_READWRITE 1
#define portMAX_DELAY UINT32_MAX
static int lock;
static int64_t now=1000000,pair_until;
static char pairing[17],token_hash[32],status[40],answer[200];
static unsigned attempts,nvs_fault;
static bool has_token,body_fail;
static const char *body_text="0123456789abcdef";
static dsr_restart_result_t restart_result;
static char *body(httpd_req_t *r,size_t limit){CHECK(r&&limit==16);if(body_fail)return NULL;size_t n=strlen(body_text);char *p=malloc(n+1);CHECK(p);memcpy(p,body_text,n+1);return p;}
static bool equal(const void *a,const void *b,size_t n){return !memcmp(a,b,n);}
static void random_hex(char *p,unsigned n){memset(p,'a',n*2);p[n*2]=0;}
static int mbedtls_sha256(const unsigned char *p,size_t n,unsigned char out[32],int is224)
{unsigned length=0;CHECK(!is224&&EVP_Digest(p,n,out,&length,EVP_sha256(),NULL)==1&&length==32);return 0;}
static int xSemaphoreTake(int l,unsigned timeout){CHECK(l==lock&&timeout==portMAX_DELAY);return 1;}
static int xSemaphoreGive(int l){CHECK(l==lock);return 1;}
static int64_t esp_timer_get_time(void){return now;}
static esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *out){CHECK(!strcmp(name,"dsr_panel")&&mode==NVS_READWRITE&&!dsr_activity_restart_reserve());opens++;*out=7;return nvs_fault==1?ESP_FAIL:ESP_OK;}
static esp_err_t nvs_set_blob(nvs_handle_t handle,const char *key,const void *p,size_t n){CHECK(handle==7&&!strcmp(key,"token_hash")&&p&&n==32&&!dsr_activity_restart_reserve());sets++;return nvs_fault==2?ESP_FAIL:ESP_OK;}
static esp_err_t nvs_commit(nvs_handle_t handle){CHECK(handle==7&&!dsr_activity_restart_reserve());commits++;return nvs_fault==3?ESP_FAIL:ESP_OK;}
static void nvs_close(nvs_handle_t handle){CHECK(handle==7&&!dsr_activity_restart_reserve());closes++;}
static esp_err_t httpd_resp_set_hdr(httpd_req_t *r,const char *key,const char *value){CHECK(r&&!strcmp(key,"Set-Cookie")&&strstr(value,"Secure; HttpOnly; SameSite=Strict"));cookies++;return ESP_OK;}
static esp_err_t json(httpd_req_t *r,const char *s,const char *text){CHECK(r);snprintf(status,sizeof(status),"%s",s);snprintf(answer,sizeof(answer),"%s",text);return ESP_OK;}
dsr_restart_result_t dsr_restart_request(void){requests++;return restart_result;}
#include "pair_endpoint.inc"
#include "restart_endpoint.inc"
static void admission_open(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void reset(void){admission_open();opens=sets=commits=closes=cookies=requests=attempts=nvs_fault=0;has_token=body_fail=false;body_text="0123456789abcdef";memcpy(pairing,body_text,17);memset(token_hash,0,32);pair_until=now+120000000;}
int main(void)
{
 httpd_req_t request={0};
 reset();CHECK(pair(&request)==ESP_OK&&!strcmp(status,"200 OK")&&opens==1&&sets==1&&commits==1&&closes==1&&cookies==1&&has_token&&!pair_until&&!pairing[0]);admission_open();
 for(unsigned fault=1;fault<=3;fault++){reset();nvs_fault=fault;CHECK(pair(&request)==ESP_OK&&!strcmp(status,"503 Service Unavailable")&&!has_token&&!cookies&&pair_until>now);admission_open();}
 reset();pair_until=now;CHECK(pair(&request)==ESP_OK&&!strcmp(status,"401 Unauthorized")&&!opens);admission_open();
 reset();body_text="invalid";CHECK(pair(&request)==ESP_OK&&!strcmp(status,"401 Unauthorized")&&!opens);admission_open();
 reset();attempts=5;CHECK(pair(&request)==ESP_OK&&!strcmp(status,"401 Unauthorized")&&!opens);admission_open();
 reset();body_fail=true;CHECK(pair(&request)==ESP_OK&&!strcmp(status,"400 Bad Request")&&!opens);admission_open();
 reset();CHECK(dsr_activity_restart_reserve());CHECK(pair(&request)==ESP_OK&&!strcmp(status,"409 Conflict")&&!opens&&!sets&&!commits&&!has_token&&!cookies);dsr_activity_restart_cancel();admission_open();
 const char *statuses[]={"200 OK","409 Conflict","503 Service Unavailable"};
 for(unsigned i=0;i<3;i++){reset();restart_result=(dsr_restart_result_t)i;CHECK(restart(&request)==ESP_OK&&!strcmp(status,statuses[i])&&requests==1);admission_open();}
 printf("PASS: %u extracted pairing/restart handler checks; actual atomic admission/SHA, HTTP/NVS MODEL; no auth/device-write proof\n",checks);return 0;
}
