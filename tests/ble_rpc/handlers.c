/* Shipped handler + genuine JSON parser; HTTP/scanner are providers. */
#include "dsr_ble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HTTP_GET 0
#define HTTP_POST 1
#define HTTP_PUT 2
typedef struct {const char *uri;int method;const char *payload;} httpd_req_t;
static unsigned checks,requests;
static bool connected=true,accepted=true;
static uint32_t received_serial;static unsigned received_index;
static char status[40],answer[200];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static bool dsr_wifi_connected(void){return connected;}
bool dsr_ble_handoff_request(uint32_t serial,unsigned index){requests++;received_serial=serial;received_index=index;return accepted;}
static char *body(httpd_req_t *r,size_t maximum){if(!r->payload||strlen(r->payload)>maximum)return NULL;char *copy=malloc(strlen(r->payload)+1);if(copy)strcpy(copy,r->payload);return copy;}
static esp_err_t json(httpd_req_t *r,const char *code,const char *text){CHECK(r);snprintf(status,sizeof(status),"%s",code);snprintf(answer,sizeof(answer),"%s",text);return ESP_OK;}
#include "ble_open_endpoint.inc"
int main(void)
{
 httpd_req_t r={.uri="/api/v1/ble/open",.method=HTTP_POST,.payload="{\"serial\":4294967295,\"index\":15}"};
 CHECK(ble_open_api(&r)==ESP_OK&&!strcmp(status,"202 Accepted")&&requests==1&&received_serial==UINT32_MAX&&received_index==15&&strstr(answer,"\"physical_confirmation_required\":true"));
 accepted=false;CHECK(ble_open_api(&r)==ESP_OK&&!strcmp(status,"409 Conflict")&&requests==2);accepted=true;connected=false;CHECK(ble_open_api(&r)==ESP_OK&&!strcmp(status,"409 Conflict")&&requests==2);connected=true;
 const char *bad[]={NULL,"","{}","{\"serial\":1,\"index\":0,\"password\":\"forbidden\"}","{\"serial\":1,\"index\":0,\"mac\":\"AABBCCDDEEFF\"}","{\"serial\":1,\"index\":0}garbage","{\"serial\":1,\"index\":16}","{\"serial\":1,\"index\":-1}","{\"serial\":1,\"serial\":2,\"index\":0}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){r.payload=bad[i];CHECK(ble_open_api(&r)==ESP_OK&&!strcmp(status,"400 Bad Request")&&requests==2);}
 r.method=HTTP_GET;CHECK(ble_open_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed")&&requests==2);r.method=HTTP_PUT;CHECK(ble_open_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed"));r.method=HTTP_POST;r.uri="/api/v1/ble/open/extra";CHECK(ble_open_api(&r)==ESP_OK&&!strcmp(status,"405 Method Not Allowed"));
 printf("PASS: %u actual BLE handoff handler checks; genuine parser, HTTP/scanner MODEL; no MAC/PSK accepted\n",checks);return 0;
}
