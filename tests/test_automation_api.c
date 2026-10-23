/* Actual serializer and outer route gates; HTTP/auth/clock providers are MODEL. */
#include "dsr_automation.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *dsr_automation_status_json(void);
static bool failed_read;static unsigned checks,copies;static bool local=true,ready=true,paired;static uint32_t revision=7;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#x);exit(1);}}while(0)
typedef struct {const char *uri;int method;size_t content_len;} httpd_req_t;
enum {HTTP_GET,HTTP_POST,HTTP_PUT,HTTP_DELETE};
static char response[6000],status[40];
static int json(httpd_req_t *r,const char *s,const char *body){CHECK(r);snprintf(status,sizeof(status),"%s",s);snprintf(response,sizeof(response),"%s",body);return 0;}
static bool local_request(httpd_req_t *r){CHECK(r);return local;}
static bool authorized(httpd_req_t *r){CHECK(r);return paired;}
bool dsr_boot_ready(void){return ready;}
static int pair(httpd_req_t *r){(void)r;CHECK(false);return 0;}
uint32_t dsr_workspace_revision(void){return revision;}
void dsr_automation_snapshot(dsr_auto_snapshot_t *out){copies++;memset(out,0,sizeof(*out));out->revision=7;out->count=1;out->rules[0]=(dsr_auto_rule_status_t){.rule_id=3,.state=DSR_AUTO_MANUAL,.cycle=2,.missing_id=1};out->target_count=1;out->targets[0].layout_id=2;out->targets[0].baseline=40;out->targets[0].requested=16;out->targets[0].read_half=44;out->targets[0].overridden=true;if(failed_read){out->rules[0].state=DSR_AUTO_ERROR;out->rules[0].result=DSR_UNSUPPORTED;out->targets[0].result=DSR_UNSUPPORTED;}}
size_t dsr_workspace_runtime_snapshot(dsr_runtime_device_t *out,size_t cap){CHECK(cap==DSR_MAX_DEVICES);memset(out,0,4*sizeof(*out));for(unsigned i=0;i<4;i++){out[i].layout_id=i+1;out[i].device.generation=DSR_GEN2_PLUS;out[i].device.endpoint_kind=DSR_ENDPOINT_BLU;strcpy(out[i].device.device_id,"PRIVATE_ID");strcpy(out[i].device.host,"192.0.2.99");out[i].telemetry.blu.flags=DSR_BLU_VALID;}out[0].telemetry.blu.flags|=DSR_BLU_HAS_OPEN;out[1].telemetry.blu.errors=DSR_BLU_TRV;out[2].telemetry.blu.flags|=DSR_BLU_HAS_MOTION;out[3].device.device_id[0]=0;out[3].telemetry.blu.flags|=DSR_BLU_HAS_OPEN;return 4;}
#include "automation_endpoint.inc"
#include "automation_route.inc"
int main(void){
 httpd_req_t req={.uri="/api/v1/automations/status",.method=HTTP_GET};
 api(&req);CHECK(!strcmp(status,"401 Unauthorized")&&copies==0);
 paired=true;local=false;api(&req);CHECK(!strcmp(status,"403 Forbidden")&&copies==0);
 local=true;ready=false;api(&req);CHECK(!strcmp(status,"503 Service Unavailable")&&copies==0);ready=true;
 req.method=HTTP_POST;api(&req);CHECK(!strcmp(status,"405 Method Not Allowed")&&copies==0);req.method=HTTP_GET;
 api(&req);CHECK(!strcmp(status,"200 OK")&&copies==1);CHECK(!strstr(response,"PRIVATE_ID")&&!strstr(response,"192.0.2.99")&&!strstr(response,"generation")&&!strstr(response,"journal"));
 cJSON *root=cJSON_Parse(response);CHECK(root);CHECK(cJSON_GetObjectItemCaseSensitive(root,"revision")->valueint==7);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(root,"devices"))==2);CHECK(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"rules")->child,"state")->valueint==DSR_AUTO_MANUAL);CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"targets")->child,"overridden")));cJSON_Delete(root);
 failed_read=true;api(&req);root=cJSON_Parse(response);CHECK(root);CHECK(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root,"targets")->child,"result")->valueint==DSR_UNSUPPORTED);cJSON_Delete(root);
 revision=8;api(&req);CHECK(!strcmp(status,"503 Service Unavailable"));
 req.uri="/api/v1/automations/execute";api(&req);CHECK(!strcmp(status,"404 Not Found"));
 printf("automation API: %u checks; actual serialization/auth route order, HTTP MODEL\n",checks);return 0;
}
