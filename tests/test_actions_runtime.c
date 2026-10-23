#define _POSIX_C_SOURCE 200809L
#include "dsr_actions.h"
#include "dsr_workspace.h"
#include "dsr_activity.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "esp_heap_caps.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static unsigned checks,live,attempts,fail_at;static void *allocated[8192];
static int lock;static bool fail_heap,unavailable;static size_t max_heap;static uint32_t now=120000;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static const char config[]="{\"schema\":2,\"revision\":7,\"language\":\"pl\",\"theme\":\"graphite\",\"accent\":5020661,\"active_dashboard\":1,\"rooms\":[],\"devices\":[{\"id\":1,\"name\":\"A\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0,\"kind\":\"relay\"},{\"id\":2,\"name\":\"B\",\"host\":\"192.0.2.2\",\"channel\":0,\"room\":0,\"kind\":\"relay\"}],\"groups\":[{\"id\":1,\"name\":\"All\",\"members\":[1,2]}],\"scenes\":[{\"id\":1,\"name\":\"Off\",\"actions\":[{\"device\":1,\"command\":\"off\"}]}],\"dashboards\":[{\"id\":1,\"name\":\"Home\",\"tiles\":[]}]}";
char *dsr_workspace_snapshot(void){return unavailable?NULL:strdup(config);}
void *heap_caps_calloc(size_t n,size_t s,unsigned caps){CHECK(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(n*s>max_heap)max_heap=n*s;return fail_heap?NULL:calloc(n,s);}
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &lock;}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t t){(void)t;CHECK(s==&lock&&!lock);lock=1;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t s){CHECK(s==&lock&&lock);lock=0;return pdTRUE;}
int64_t esp_timer_get_time(void){return (int64_t)now*1000;}
uint32_t esp_random(void){return 12345;}
time_t __wrap_time(time_t *out){if(out)*out=1760000100;return 1760000100;}
static dsr_connection_t connection(unsigned id){dsr_connection_t c={.layout_id=id,.device={.generation=2,.online=true,.has_switch=true}};snprintf(c.device.device_id,64,"shelly-%u",id);snprintf(c.device.host,16,"192.0.2.%u",id);strcpy(c.credentials.password,"never-export-this-secret");return c;}
static void publish(dsr_connection_t *a,dsr_connection_t *b,bool available){CHECK(dsr_actions_publish_begin(7,available));dsr_actions_publish_at(a,true,false,DSR_OK,true);dsr_actions_publish_at(b,true,false,DSR_OK,true);dsr_actions_publish_end();}
static void idle(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void request_text(char text[256],const dsr_actions_snapshot_t *s,unsigned nonce,uint32_t gen){snprintf(text,256,"{\"id\":1,\"revision\":7,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%016x\",\"on\":true}",s->boot_id,gen,nonce);}
static uint32_t start(unsigned nonce){dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);char text[256];request_text(text,&s,nonce,s.admission_generation);uint32_t id=0;CHECK(dsr_actions_request(text,0,0,&id)==DSR_OK&&id);CHECK(!dsr_activity_restart_reserve());return id;}
static void *allocate(size_t n){attempts++;if(attempts==fail_at)return NULL;void *p=malloc(n);if(p){unsigned i=0;while(i<8192&&allocated[i])i++;CHECK(i<8192);allocated[i]=p;live++;}return p;}
static void release(void *p){if(!p)return;unsigned i=0;while(i<8192&&allocated[i]!=p)i++;CHECK(i<8192&&live);allocated[i]=NULL;live--;free(p);}
enum {HTTP_GET,HTTP_POST};
typedef struct {const char *uri,*input;size_t content_len;unsigned method;} httpd_req_t;
static char http_status[48];
static char *body(httpd_req_t *r,size_t limit){return r->input&&r->content_len&&r->content_len<=limit?strdup(r->input):NULL;}
static esp_err_t json(httpd_req_t *r,const char *status,const char *text){(void)r;CHECK(text);cJSON *parsed=cJSON_Parse(text);CHECK(parsed);cJSON_Delete(parsed);snprintf(http_status,sizeof(http_status),"%s",status);return ESP_OK;}
#include "actions_endpoint.inc"
static void endpoint(void)
{
 httpd_req_t req={.uri="/api/v1/actions/status",.method=HTTP_GET};CHECK(actions_api(&req)==ESP_OK&&!strcmp(http_status,"200 OK"));req.method=HTTP_POST;actions_api(&req);CHECK(!strcmp(http_status,"405 Method Not Allowed"));
 req=(httpd_req_t){.uri="/api/v1/groups/run",.input="{}",.content_len=2,.method=HTTP_POST};actions_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));req.content_len=257;actions_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));req.method=HTTP_GET;actions_api(&req);CHECK(!strcmp(http_status,"405 Method Not Allowed"));
 dsr_actions_snapshot_t status;dsr_actions_snapshot(&status);char request[256];request_text(request,&status,77,status.admission_generation);req=(httpd_req_t){.uri="/api/v1/groups/run",.input=request,.content_len=strlen(request),.method=HTTP_POST};actions_api(&req);CHECK(!strcmp(http_status,"202 Accepted"));actions_api(&req);CHECK(!strcmp(http_status,"202 Accepted"));dsr_actions_snapshot(&status);CHECK(dsr_actions_cancel(status.boot_id,status.run_id));CHECK(!dsr_actions_target());idle();
 req=(httpd_req_t){.uri="/api/v1/actions/cancel",.input="{\"boot_id\":999,\"run_id\":1}",.method=HTTP_POST};req.content_len=strlen(req.input);actions_api(&req);CHECK(!strcmp(http_status,"409 Conflict"));req.input="{}";req.content_len=2;actions_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));
 req=(httpd_req_t){.uri="/api/v1/actions/log/query",.input="{\"epoch\":0,\"offset\":0,\"limit\":32}",.method=HTTP_POST};req.content_len=strlen(req.input);actions_api(&req);CHECK(!strcmp(http_status,"200 OK"));req.content_len=0;actions_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));req.uri="/api/v1/actions/log/clear";actions_api(&req);CHECK(!strcmp(http_status,"200 OK"));req.content_len=1;actions_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));
}
int main(void)
{
 dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);CHECK(!s.boot_id&&!s.available);fail_heap=true;CHECK(dsr_actions_init()==ESP_ERR_NO_MEM);fail_heap=false;CHECK(dsr_actions_init()==ESP_OK&&max_heap<=12288);CHECK(dsr_actions_init()==ESP_ERR_INVALID_STATE);dsr_actions_snapshot(&s);CHECK(s.boot_id==12345&&s.admission_generation==0&&!s.available);idle();
 char text[256];uint32_t run=0;request_text(text,&s,1,0);CHECK(dsr_actions_request(text,0,0,&run)==DSR_CONFLICT);dsr_connection_t a=connection(1),b=connection(2);publish(&a,&b,true);run=start(1);CHECK(dsr_actions_reserved(&a.device)&&dsr_actions_reserved(&b.device));
 CHECK(dsr_actions_target()==1);dsr_job_kind_t kind;bool on;CHECK(!dsr_actions_prepare(&a,DSR_AUTH_REQUIRED,true,true,1,&kind,&on));CHECK(dsr_actions_target()==2);CHECK(dsr_actions_prepare(&b,DSR_OK,true,true,2,&kind,&on)&&kind==DSR_JOB_SET_OUTPUT&&on);CHECK(dsr_actions_worker_allowed(2,&b)==DSR_OK);CHECK(dsr_actions_cancel(12345,run));dsr_actions_complete(2,DSR_OK);CHECK(!dsr_actions_active());dsr_actions_snapshot(&s);CHECK(s.steps[0].outcome==DSR_ACTION_FAILED&&s.steps[0].result==DSR_AUTH_REQUIRED&&s.steps[1].outcome==DSR_ACTION_SUCCESS);CHECK(!dsr_actions_reserved(&a.device));idle();
 unavailable=true;uint32_t duplicate=0;CHECK(dsr_actions_request(text,0,0,&duplicate)==DSR_OK&&duplicate==run);unavailable=false;request_text(text,&s,2,0);CHECK(dsr_actions_request(text,0,0,&duplicate)==DSR_CONFLICT);CHECK(!dsr_actions_cancel(999,run));
 run=start(2);CHECK(dsr_actions_prepare(&a,DSR_OK,false,true,3,&kind,&on)&&kind==DSR_JOB_PROBE);CHECK(dsr_actions_worker_allowed(3,&a)==DSR_OK);dsr_actions_complete(3,DSR_OK);CHECK(dsr_actions_prepare(&a,DSR_OK,true,true,4,&kind,&on)&&kind==DSR_JOB_SET_OUTPUT);CHECK(dsr_actions_worker_allowed(4,&a)==DSR_OK);CHECK(dsr_actions_cancel(12345,run));CHECK(dsr_actions_worker_allowed(4,&a)==DSR_BUSY);CHECK(dsr_actions_active());dsr_actions_complete(999,DSR_OK);CHECK(dsr_actions_active());dsr_actions_complete(4,DSR_BUSY);CHECK(!dsr_actions_target());dsr_actions_snapshot(&s);CHECK(s.steps[0].outcome==DSR_ACTION_UNCERTAIN&&s.steps[1].outcome==DSR_ACTION_CANCELLED);idle();
 start(3);CHECK(dsr_actions_prepare(&a,DSR_OK,true,true,5,&kind,&on));dsr_actions_unqueue(5);CHECK(dsr_actions_prepare(&a,DSR_OK,true,true,6,&kind,&on));dsr_connection_t changed=a;strcpy(changed.device.host,"192.0.2.99");CHECK(dsr_actions_worker_allowed(6,&changed)==DSR_IDENTITY_MISMATCH);dsr_actions_complete(6,DSR_IDENTITY_MISMATCH);CHECK(dsr_actions_target()==2);dsr_actions_missing(2);CHECK(!dsr_actions_target());idle();
 run=start(4);CHECK(dsr_actions_prepare(&a,DSR_OK,true,true,7,&kind,&on));dsr_actions_mode_block();CHECK(dsr_actions_active()&&dsr_actions_worker_allowed(7,&a)==DSR_BUSY);dsr_actions_complete(7,DSR_BUSY);CHECK(!dsr_actions_target());idle();publish(&a,&b,true);start(5);now+=180000;CHECK(!dsr_actions_target());idle();
 dsr_action_log_page_t *page=calloc(1,sizeof(*page));CHECK(page&&dsr_actions_log(0,0,32,page)&&page->count>0);uint32_t epoch=page->epoch;unsigned count=page->total;publish(&a,&b,true);CHECK(dsr_actions_log(epoch,0,32,page)&&page->total==count);a.device.output=true;publish(&a,&b,true);CHECK(dsr_actions_log(epoch,0,32,page)&&page->total==count+1&&page->entries[0].kind==2&&page->entries[0].utc==1760000100);
 char *out=dsr_actions_status_json();CHECK(out&&!strstr(out,"192.0.2.")&&!strstr(out,"shelly-")&&!strstr(out,"secret"));free(out);out=dsr_actions_log_json(0,0,32);CHECK(out&&!strstr(out,"192.0.2.")&&!strstr(out,"shelly-")&&!strstr(out,"secret"));free(out);
 cJSON_Hooks hooks={allocate,release};cJSON_InitHooks(&hooks);attempts=0;out=dsr_actions_status_json();CHECK(out);unsigned total=attempts;cJSON_free(out);CHECK(!live);for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;CHECK(!dsr_actions_status_json()&&!live);}fail_at=0;attempts=0;out=dsr_actions_log_json(0,0,32);CHECK(out);unsigned log_total=attempts;cJSON_free(out);CHECK(!live);for(unsigned i=1;i<=log_total;i++){attempts=0;fail_at=i;CHECK(!dsr_actions_log_json(0,0,32)&&!live);}fail_at=0;cJSON_InitHooks(NULL);
 dsr_actions_log_clear();CHECK(!dsr_actions_log(epoch,0,32,page));CHECK(dsr_actions_log(0,0,32,page)&&!page->count&&page->epoch!=epoch);uint32_t boot,id;unsigned offset,limit;CHECK(dsr_actions_cancel_parse("{\"boot_id\":12345,\"run_id\":1}",&boot,&id)&&boot==12345&&id==1);CHECK(!dsr_actions_cancel_parse("{\"boot_id\":1,\"boot_id\":2,\"run_id\":1}",&boot,&id));CHECK(dsr_actions_log_parse("{\"epoch\":0,\"offset\":128,\"limit\":32}",&epoch,&offset,&limit)&&offset==128&&limit==32);CHECK(!dsr_actions_log_parse("{\"epoch\":0,\"offset\":0,\"limit\":33}",&epoch,&offset,&limit));free(page);
 endpoint();
 /* A Cover transport transition is not a confirmed Switch OFF command. The
  * actual wrapper/cache/log is exercised; no replacement log provider. */
 dsr_connection_t cover=b;cover.device.endpoint_kind=DSR_ENDPOINT_COVER;cover.device.has_switch=false;cover.device.output=false;cover.device.online=true;
 publish(&a,&cover,true);dsr_actions_log_clear();cover.device.online=false;publish(&a,&cover,true);
 page=calloc(1,sizeof(*page));CHECK(page&&dsr_actions_log(0,0,32,page)&&page->count==1&&page->entries[0].layout_id==2&&page->entries[0].kind==2);
 fprintf(stderr,"Cover online->offline activity: command=%u outcome=%u result=%u (SwitchOFF=%u success=%u)\n",page->entries[0].command,page->entries[0].outcome,page->entries[0].result,DSR_ACTION_OFF,DSR_ACTION_SUCCESS);
 CHECK(page->entries[0].command!=DSR_ACTION_ON&&page->entries[0].command!=DSR_ACTION_OFF);CHECK(page->entries[0].outcome==DSR_ACTION_FAILED&&page->entries[0].result==DSR_OFFLINE);free(page);
 /* Same physical ID/gen/channel/host, different endpoint kind: no reservation
  * alias and the actual registered worker guard rejects the wrong type. */
 publish(&a,&b,true);start(88);dsr_connection_t alias=a;alias.device.endpoint_kind=DSR_ENDPOINT_COVER;
 CHECK(dsr_actions_reserved(&a.device)&&!dsr_actions_reserved(&alias.device));CHECK(dsr_actions_prepare(&a,DSR_OK,true,true,900,&kind,&on));CHECK(dsr_actions_worker_allowed(900,&alias)==DSR_IDENTITY_MISMATCH);dsr_actions_complete(900,DSR_IDENTITY_MISMATCH);dsr_actions_missing(2);CHECK(!dsr_actions_target());idle();
 /* The actual actor freezes and dispatches typed payloads for every member. */
 const uint8_t families[]={DSR_ENDPOINT_COVER,DSR_ENDPOINT_LIGHT,DSR_ENDPOINT_RGB,DSR_ENDPOINT_BLU};
 const char *payloads[]={"\"command\":\"position\",\"value\":80","\"command\":\"brightness\",\"value\":65","\"command\":\"rgb\",\"rgb\":[10,120,250]","\"command\":\"target\",\"value\":43"};
 for(unsigned type=0;type<4;type++){
  a=connection(1);b=connection(2);a.device.endpoint_kind=b.device.endpoint_kind=families[type];
  if(!type)a.telemetry.cover=b.telemetry.cover=(dsr_cover_status_t){.flags=DSR_COVER_VALID|DSR_COVER_POSITION_CONTROL|DSR_COVER_POSITION,.state=DSR_COVER_STATE_STOPPED};
  if(type==1)a.telemetry.light=b.telemetry.light=(dsr_light_status_t){.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS};
  if(type==2)a.telemetry.rgb=b.telemetry.rgb=(dsr_rgb_status_t){.flags=DSR_RGB_VALID};
  if(type==3)a.telemetry.blu.errors=b.telemetry.blu.errors=DSR_BLU_TRV;
  publish(&a,&b,true);dsr_actions_snapshot(&s);snprintf(text,sizeof(text),"{\"id\":1,\"revision\":7,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%016x\",%s}",s.boot_id,s.admission_generation,100+type,payloads[type]);CHECK(dsr_actions_request(text,0,1,&run)==DSR_OK);
  for(unsigned member=0;member<2;member++){
   dsr_job_t job={.id=1000+type*2+member,.connection=member?b:a};CHECK(dsr_actions_target()==member+1);CHECK(dsr_actions_prepare_job(&job,DSR_OK,true));
   CHECK(job.kind==(type==0?DSR_JOB_COVER:type==1?DSR_JOB_LIGHT:type==2?DSR_JOB_RGB:DSR_JOB_TRV));
   CHECK(type==0?job.cover.command==DSR_COVER_GOTO&&job.cover.position==80:type==1?job.light.command==DSR_LIGHT_SET_BRIGHTNESS&&job.light.brightness==65:type==2?job.rgb.command==DSR_RGB_SET_COLOR&&job.rgb.rgb[0]==10&&job.rgb.rgb[1]==120&&job.rgb.rgb[2]==250:job.trv.command==DSR_TRV_SET_TARGET&&job.trv.half_c==43);
   CHECK(dsr_actions_worker_allowed(job.id,&job.connection)==DSR_OK);dsr_actions_worker_write_attempted(job.id);dsr_actions_complete(job.id,DSR_OK);
  }
  CHECK(!dsr_actions_target());dsr_actions_snapshot(&s);CHECK(s.steps[0].outcome==DSR_ACTION_SUCCESS&&s.steps[1].outcome==DSR_ACTION_SUCCESS);idle();
 }
 /* Reject even a legacy ON request for mixed/private passive membership. */
 a=connection(1);b=connection(2);b.device.endpoint_kind=DSR_ENDPOINT_LIGHT;publish(&a,&b,true);dsr_actions_snapshot(&s);request_text(text,&s,110,s.admission_generation);CHECK(dsr_actions_request(text,0,1,&run)==DSR_UNSUPPORTED&&!dsr_actions_active());
 b.device.endpoint_kind=DSR_ENDPOINT_BLU;b.telemetry.blu.errors=0;publish(&a,&b,true);CHECK(dsr_actions_request(text,0,1,&run)==DSR_UNSUPPORTED&&!dsr_actions_target());idle();
 printf("PASS: %u actual actor/cache/activity/API checks; %u+%u cJSON OOM; PSRAM=%zu; typed groups, common TRV target, mixed/passive rejection, exact ACK/cancel/readback ownership; RTOS/clock/workspace providers MODEL\n",checks,total,log_total,max_heap);return 0;
}
