/* Extracted shipped handoff handler; HTTP/clock/workspace are MODEL.
 * Outer local/boot/auth gates are covered by the exact-source invariance gate. */
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HTTP_GET 0
#define HTTP_POST 1
#define HTTP_PUT 2
#define HTTP_DELETE 3
typedef struct {const char *uri;int method;size_t content_len;} httpd_req_t;
static unsigned checks,requests,snapshots;
static bool connected=true,accepted=true;
static uint32_t received_ms;
static int64_t now_us=123456789000LL;
static dsr_ap_handoff_t handoff=DSR_AP_HANDOFF_READY;
static char status[40],answer[200];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static int64_t esp_timer_get_time(void){return now_us;}
bool dsr_wifi_connected(void){return connected;}
bool dsr_workspace_request_ap_open(uint32_t now){requests++;received_ms=now;return accepted;}
dsr_ap_handoff_t dsr_workspace_ap_handoff(uint32_t now){snapshots++;received_ms=now;return handoff;}
static esp_err_t json(httpd_req_t *r,const char *code,const char *text)
{CHECK(r);snprintf(status,sizeof(status),"%s",code);snprintf(answer,sizeof(answer),"%s",text);return ESP_OK;}
#include "ap_handoff_endpoint.inc"
int main(void)
{
 httpd_req_t r={.uri="/api/v1/ap",.method=HTTP_GET};
 for(unsigned state=0;state<=4;state++){
  handoff=(dsr_ap_handoff_t)state;unsigned before=requests;
  CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"200 OK")&&requests==before&&received_ms==123456789);
  char value[32];snprintf(value,sizeof(value),"\"handoff\":%u",state);CHECK(strstr(answer,value)&&strstr(answer,"\"physical_confirmation_required\":true"));
 }
 CHECK(snapshots==5);r.uri="/api/v1/ap/open";r.method=HTTP_POST;
 CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"202 Accepted")&&requests==1&&strstr(answer,"\"queued\":true")&&!strstr(answer,"password"));
 accepted=false;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"409 Conflict")&&requests==2);
 connected=false;accepted=true;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"409 Conflict")&&requests==2);
 connected=true;r.content_len=1;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"400 Bad Request")&&requests==2);
 r.content_len=SIZE_MAX;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"400 Bad Request")&&requests==2);r.content_len=0;
 r.method=HTTP_GET;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed")&&requests==2);
 r.uri="/api/v1/ap";r.method=HTTP_POST;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed")&&requests==2);
 r.uri="/api/v1/ap/open/extra";CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed")&&requests==2);
 r.uri="/api/v1/ap/open";r.method=HTTP_PUT;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed")&&requests==2);
 r.method=HTTP_DELETE;CHECK(ap_handoff_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed")&&requests==2);
 r.method=HTTP_POST;now_us=((int64_t)UINT32_MAX+2000)*1000;CHECK(ap_handoff_api(&r)==ESP_OK&&received_ms==1999&&requests==3);
 printf("PASS: %u extracted actual AP handoff handler checks; no credential body, strict route/method, home gate, no replay; HTTP/workspace MODEL\n",checks);return 0;
}
