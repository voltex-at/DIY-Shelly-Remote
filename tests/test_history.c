#define _POSIX_C_SOURCE 200809L
#include "dsr_history_private.h"
#include "dsr_activity.h"
#include "dsr_activity_internal.h"
#include "esp_heap_caps.h"
#include "esp_spiffs.h"
#include "esp_netif_sntp.h"
#include "freertos/semphr.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static bool worker=true,locked,fail_alloc,fail_flush,fail_sync,no_space,unmounted;
static size_t max_alloc,write_limit=SIZE_MAX,written;
static const char *directory;
static time_t clock_utc=1760000100;
static uint64_t monotonic=1000;
static unsigned init_count,wait_count,cancel_at;
static bool clock_after_wait,init_fail;
FILE *__real_fopen(const char *,const char *);
FILE *__wrap_fopen(const char *p,const char *m){CHECK(worker&&!locked);char mapped[256];if(!strncmp(p,"/dsr/",5)){CHECK(!strcmp(p+5,"history0.dat")||!strcmp(p+5,"history1.dat"));snprintf(mapped,sizeof(mapped),"%s/%s",directory,p+5);p=mapped;}return __real_fopen(p,m);}
size_t __real_fwrite(const void *,size_t,size_t,FILE *);
size_t __wrap_fwrite(const void *p,size_t size,size_t n,FILE *f){CHECK(worker&&!locked);size_t bytes=size*n;if(bytes>write_limit-written)bytes=write_limit-written;size_t result=__real_fwrite(p,1,bytes,f);written+=result;return size?result/size:0;}
int __real_fflush(FILE *);
int __wrap_fflush(FILE *f){CHECK(worker&&!locked);if(fail_flush)return EOF;return __real_fflush(f);}
int __real_fsync(int);
int __wrap_fsync(int fd){CHECK(worker&&!locked);if(fail_sync)return -1;return __real_fsync(fd);}
time_t __wrap_time(time_t *out){if(out)*out=clock_utc;return clock_utc;}
void *heap_caps_calloc(size_t n,size_t s,unsigned caps){CHECK(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(n*s>max_alloc)max_alloc=n*s;return fail_alloc?NULL:calloc(n,s);}
struct test_semaphore {int value;};static struct test_semaphore mutex;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &mutex;}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t timeout){(void)timeout;CHECK(s==&mutex&&!locked);locked=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t s){CHECK(s==&mutex&&locked);locked=false;return pdTRUE;}
int64_t esp_timer_get_time(void){return (int64_t)monotonic*1000;}
uint32_t dsr_workspace_revision(void){return 1;}
bool dsr_wifi_connected(void){return true;}
esp_err_t esp_spiffs_info(const char *label,size_t *total,size_t *used){CHECK(worker&&!locked&&!strcmp(label,"dsrdata"));*total=0x3e0000;*used=no_space?*total:0;return unmounted?ESP_FAIL:ESP_OK;}
esp_err_t esp_netif_sntp_init(const esp_sntp_config_t *config){CHECK(worker&&!locked&&!strcmp(config->server,"pool.ntp.org"));init_count++;return init_fail?ESP_FAIL:ESP_OK;}
esp_err_t esp_netif_sntp_sync_wait(TickType_t ticks){CHECK(worker&&!locked&&ticks==pdMS_TO_TICKS(1000));wait_count++;if(clock_after_wait)clock_utc=1760000100;return ESP_FAIL;}
static bool cancel(void *ctx){(void)ctx;return cancel_at&&wait_count>=cancel_at;}
enum {HTTP_GET,HTTP_POST};
typedef struct {const char *uri,*input;size_t content_len;unsigned method;} httpd_req_t;
static char http_status[48];
static char *body(httpd_req_t *r,size_t max){if(!r->input||!r->content_len||r->content_len>max)return NULL;return strdup(r->input);}
static esp_err_t json(httpd_req_t *r,const char *status,const char *text){(void)r;CHECK(text);cJSON *parsed=cJSON_Parse(text);CHECK(parsed);cJSON_Delete(parsed);snprintf(http_status,sizeof(http_status),"%s",status);return ESP_OK;}
#include "history_endpoint.inc"
static dsr_history_key_t key(unsigned n){dsr_history_key_t k={.layout_id=n+1,.generation=2};snprintf(k.device_id,sizeof(k.device_id),"shelly-%u",n);return k;}
static void rmfiles(void){char p[256];for(unsigned i=0;i<2;i++){snprintf(p,sizeof(p),"%s/history%u.dat",directory,i);unlink(p);}}
static void raw(const char *name,const void *p,size_t n){char path[256];snprintf(path,sizeof(path),"%s/%s",directory,name);FILE *f=fopen(path,"wb");CHECK(f);CHECK(fwrite(p,1,n,f)==n);CHECK(!fclose(f));}
static void model(void)
{
 dsr_history_model_t *m=calloc(1,sizeof(*m));dsr_history_series_t *s=calloc(1,sizeof(*s));CHECK(m&&s);
 dsr_history_key_t k=key(0);dsr_device_t d={.online=true,.has_power=true,.power_w=0};dsr_telemetry_t t={.has_voltage=true,.voltage_v=230,.has_current=true,.current_a=0,.has_energy=true,.energy_wh=0};
 dsr_history_model_record(m,&k,&d,&t,1,1760000100,1000);int slot=dsr_history_model_binding(m,&k,true);CHECK(slot>=0);dsr_history_model_query(m,slot,1,1760000100,s);CHECK(s->count==1&&s->points[0].mask==15&&s->points[0].power==0&&s->points[0].energy_wh==0);
 d.power_w=100;dsr_history_model_record(m,&k,&d,&t,1,1760000101,2000);dsr_history_model_query(m,slot,1,1760000101,s);CHECK(s->points[0].count[0]==1);dsr_history_model_record(m,&k,&d,&t,2,1760000102,3000);dsr_history_model_query(m,slot,1,1760000102,s);CHECK(s->points[0].power==50&&s->points[0].count[0]==2);
 t.energy_wh=100;dsr_history_model_record(m,&k,&d,&t,3,1760000103,4000);t.energy_wh=2;dsr_history_model_record(m,&k,&d,&t,4,1760000400,40000);dsr_history_model_query(m,slot,1,1760000400,s);CHECK(s->count==2&&s->points[1].flags==3&&s->points[1].energy_wh==2);
 t=(dsr_telemetry_t){0};d.has_power=false;dsr_history_model_record(m,&k,&d,&t,5,1760000700,41000);dsr_history_model_query(m,slot,1,1760000700,s);CHECK(s->count==2); /* a missing field cannot become a zero point */
 dsr_history_model_record(m,&k,&d,&t,6,0,42000);CHECK(m->without_time==1);dsr_history_model_record(m,&k,&d,&t,7,1760000100,43000);CHECK(m->without_time==2);
 dsr_history_key_t alias=k;alias.layout_id=99;CHECK(dsr_history_model_binding(m,&alias,true)==slot);alias.channel=1;CHECK(dsr_history_model_binding(m,&alias,true)!=slot);alias=k;alias.kind=1;CHECK(dsr_history_model_binding(m,&alias,true)<0);
 memset(m,0,sizeof(*m));d.has_power=true;t.has_energy=true;t.energy_wh=123;for(unsigned n=0;n<16;n++){k=key(n);for(unsigned j=0;j<300;j++){d.power_w=n+j;dsr_history_model_record(m,&k,&d,&t,j,1760000100+j*300u,1000+j*300000u);}}
 k=key(0);slot=dsr_history_model_binding(m,&k,true);dsr_history_model_query(m,slot,24,1760000100+299*300u,s);CHECK(s->count==289);dsr_history_model_query(m,slot,1,1760000100+299*300u,s);CHECK(s->count==13);k=key(16);CHECK(dsr_history_model_binding(m,&k,true)<0);
 unsigned char *snapshot=malloc(DSR_HISTORY_SNAPSHOT_MAX);CHECK(snapshot);CHECK(dsr_history_checkpoint(m,snapshot,DSR_HISTORY_SNAPSHOT_MAX,1)==DSR_HISTORY_SNAPSHOT_MAX);free(snapshot);
 printf("history sizeof point=%zu model=%zu slot=%zu series=%zu checkpoint=%u\n",sizeof(dsr_history_point_t),sizeof(*m),sizeof(dsr_history_slot_t),sizeof(*s),DSR_HISTORY_SNAPSHOT_MAX);free(m);free(s);
}
static void store(void)
{
 dsr_history_model_t *m=calloc(1,sizeof(*m)),*read=calloc(1,sizeof(*m));unsigned char *bytes=calloc(1,DSR_HISTORY_SNAPSHOT_MAX);CHECK(m&&read&&bytes);dsr_history_store_t st={0},loaded;
 CHECK(dsr_history_store_load(&st,m,directory)&&st.loaded&&!st.generation);dsr_history_key_t k=key(0);int slot=dsr_history_model_binding(m,&k,true);dsr_device_t d={.online=true,.has_power=true,.power_w=42};dsr_telemetry_t t={0};dsr_history_model_record(m,&k,&d,&t,1,1760000100,1000);
 size_t n=dsr_history_checkpoint(m,bytes,DSR_HISTORY_SNAPSHOT_MAX,1);CHECK(n==152);CHECK(dsr_history_store_checkpoint(&st,bytes,n,directory));CHECK(dsr_history_store_load(&loaded,read,directory)&&loaded.generation==1&&read->points[slot][(1760000100u/300)%289].power==42);
 d.power_w=84;dsr_history_model_record(m,&k,&d,&t,2,1760000400,301000);n=dsr_history_checkpoint(m,bytes,DSR_HISTORY_SNAPSHOT_MAX,2);CHECK(n==192);
 /* Every byte-level interruption in the other checkpoint retains the old one. */
 for(size_t cut=0;cut<n;cut++){written=0;write_limit=cut;CHECK(!dsr_history_store_checkpoint(&st,bytes,n,directory));write_limit=SIZE_MAX;CHECK(dsr_history_store_load(&loaded,read,directory)&&loaded.generation==1);}
 CHECK(dsr_history_store_checkpoint(&st,bytes,n,directory));CHECK(dsr_history_store_load(&loaded,read,directory)&&loaded.generation==2);
 unsigned char point[40];dsr_history_point_t p={.utc=1760000700,.power=7,.mask=1,.count={1}};dsr_history_encode_point(point,slot,&p);
 /* Torn append tail exposes only complete, CRC-valid records. */
 for(size_t cut=0;cut<40;cut++){dsr_history_store_t copy=st;written=0;write_limit=cut;CHECK(!dsr_history_store_append(&copy,point,40,directory));write_limit=SIZE_MAX;CHECK(dsr_history_store_load(&loaded,read,directory)&&loaded.generation==2&&loaded.repair==(cut!=0));CHECK(dsr_history_store_append(&loaded,point,40,directory));st=loaded;}
 fail_flush=true;CHECK(!dsr_history_store_append(&st,point,40,directory));fail_flush=false;CHECK(dsr_history_store_load(&loaded,read,directory));fail_sync=true;CHECK(!dsr_history_store_append(&loaded,point,40,directory));fail_sync=false;
 /* Unknown schema and endpoint kind are preserved and never opened for write. */
 bytes[4]=2;raw("history0.dat",bytes,n);CHECK(!dsr_history_store_load(&loaded,read,directory)&&loaded.future);CHECK(!dsr_history_store_append(&loaded,point,40,directory));rmfiles();CHECK(dsr_history_store_load(&st,m,directory));k=key(0);slot=dsr_history_model_binding(m,&k,true);n=dsr_history_checkpoint(m,bytes,DSR_HISTORY_SNAPSHOT_MAX,1);CHECK(dsr_history_store_checkpoint(&st,bytes,n,directory));unsigned char frame[80];k.kind=1;dsr_history_encode_key(frame,slot,&k);CHECK(dsr_history_store_append(&st,frame,80,directory));CHECK(!dsr_history_store_load(&loaded,read,directory)&&loaded.future);
 rmfiles();raw("history0.dat","bad",3);raw("history1.dat","bad",3);CHECK(!dsr_history_store_load(&loaded,read,directory)&&!loaded.future);rmfiles();free(bytes);free(read);free(m);
}
static void clock_test(void)
{
 CHECK(dsr_clock_sync(cancel,NULL)==DSR_CLOCK_READY&&!init_count);clock_utc=0;init_fail=true;CHECK(dsr_clock_sync(cancel,NULL)==DSR_CLOCK_UNAVAILABLE&&init_count==1&&!wait_count);init_fail=false;cancel_at=1;CHECK(dsr_clock_sync(cancel,NULL)==DSR_CLOCK_CANCELLED&&init_count==2&&wait_count==1);cancel_at=0;wait_count=0;CHECK(dsr_clock_sync(cancel,NULL)==DSR_CLOCK_UNAVAILABLE&&init_count==2&&wait_count==20);wait_count=0;clock_after_wait=true;CHECK(dsr_clock_sync(cancel,NULL)==DSR_CLOCK_READY&&init_count==2&&wait_count==1);clock_after_wait=false;
}
static void wrapper(void)
{
 worker=false;fail_alloc=true;CHECK(dsr_history_init()==ESP_ERR_NO_MEM);fail_alloc=false;CHECK(dsr_history_init()==ESP_OK);CHECK(dsr_history_service_due(1000));worker=true;dsr_history_service();worker=false;
 dsr_connection_t c={.layout_id=1,.device={.device_id="shelly-real",.generation=2,.online=true,.has_power=true,.power_w=0},.telemetry={.has_voltage=true,.voltage_v=230,.has_current=true,.current_a=0,.has_energy=true,.energy_wh=0}};
 dsr_history_binding(&c,true);dsr_history_record(&c,1);dsr_history_key_t k={.device_id="shelly-real",.generation=2};dsr_history_series_t *s=dsr_history_query(&k,1);CHECK(s&&s->count==1&&s->points[0].mask==15&&!s->status.durable);dsr_history_series_free(s);
 /* A Cover with identical physical ID/gen/channel cannot bind, record, forget
  * or query the Switch slot. Genuine wrapper+model, not a replacement provider. */
 dsr_connection_t cover=c;cover.device.endpoint_kind=DSR_ENDPOINT_COVER;cover.layout_id=2;cover.device.power_w=777;cover.telemetry.energy_wh=999;
 dsr_history_binding(&cover,true);dsr_history_binding(&cover,false);dsr_history_record(&cover,2);dsr_history_forget(&cover.device);
 dsr_history_key_t ck=k;ck.kind=DSR_ENDPOINT_COVER;CHECK(!dsr_history_query(&ck,1)&&!dsr_history_json(&ck,1));
 s=dsr_history_query(&k,1);CHECK(s&&s->key.layout_id==1&&s->count==1&&s->points[0].power==0&&s->points[0].energy_wh==0&&s->points[0].count[0]==1);dsr_history_series_free(s);
 char *json=dsr_history_json(&k,1);CHECK(json);cJSON *r=cJSON_Parse(json);CHECK(r&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"points"))==1&&!strcmp(cJSON_GetObjectItemCaseSensitive(r,"timezone")->valuestring,"UTC"));cJSON_Delete(r);free(json);
 clock_utc+=300;monotonic+=301000;CHECK(dsr_history_service_due((uint32_t)monotonic));worker=true;dsr_history_service();worker=false;s=dsr_history_query(&k,24);CHECK(s&&s->status.durable);dsr_history_series_free(s);
 dsr_history_key_t parsed;unsigned hours;const char *good="{\"device_id\":\"shelly-real\",\"generation\":2,\"channel\":0,\"hours\":24}";CHECK(dsr_history_parse(good,&parsed,&hours)&&hours==24&&!strcmp(parsed.device_id,k.device_id));const char *bad[]={"{}","{\"device_id\":\"a\",\"device_id\":\"b\",\"generation\":2,\"channel\":0,\"hours\":1}","{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"hours\":7}","{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"hours\":1,\"kind\":1}","{\"device_id\":\"a\\u0000b\",\"generation\":2,\"channel\":0,\"hours\":1}","{\"device_id\":\"../a\",\"generation\":2,\"channel\":0,\"hours\":1}","{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"hours\":1,\"host\":\"x\"}"};for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)CHECK(!dsr_history_parse(bad[i],&parsed,&hours));
 httpd_req_t req={.uri="/api/v1/history/query",.method=HTTP_POST,.input=good,.content_len=strlen(good)};CHECK(history_api(&req)==ESP_OK&&!strcmp(http_status,"200 OK"));req.input="{}";req.content_len=2;history_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));req.input=good;req.content_len=257;history_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));req.method=HTTP_GET;history_api(&req);CHECK(!strcmp(http_status,"405 Method Not Allowed"));req=(httpd_req_t){.uri="/api/v1/history/clock",.method=HTTP_POST};history_api(&req);CHECK(!strcmp(http_status,"202 Accepted"));history_api(&req);CHECK(!strcmp(http_status,"409 Conflict"));req.uri="/api/v1/history/clock/cancel";history_api(&req);CHECK(!strcmp(http_status,"202 Accepted"));req.content_len=1;history_api(&req);CHECK(!strcmp(http_status,"400 Bad Request"));
 fail_alloc=true;CHECK(!dsr_history_query(&k,1)&&!dsr_history_json(&k,1));fail_alloc=false;dsr_history_forget(&c.device);CHECK(!dsr_history_query(&k,1));CHECK(dsr_activity_restart_reserve());CHECK(!dsr_history_request_clock());dsr_activity_restart_cancel();CHECK(dsr_history_request_clock()&&!dsr_history_request_clock());dsr_history_cancel_clock();CHECK(!dsr_history_clock_pending());worker=true;dsr_history_clock_execute();worker=false;CHECK(max_alloc<=DSR_HISTORY_MEMORY_MAX&&!locked);
}
static void unavailable(unsigned mode)
{
 rmfiles();if(mode==2){unsigned char header[32]={0x44,0x53,0x52,0x48,2};raw("history0.dat",header,sizeof(header));}else if(mode==3){raw("history0.dat","bad",3);raw("history1.dat","bad",3);}else unmounted=true;
 size_t before=written;worker=false;CHECK(dsr_history_init()==ESP_OK);worker=true;dsr_history_service();worker=false;
 dsr_connection_t c={.layout_id=1,.device={.device_id="shelly-ram",.generation=2,.online=true,.has_power=true,.power_w=0}};dsr_history_record(&c,1);dsr_history_key_t k={.device_id="shelly-ram",.generation=2};dsr_history_series_t *s=dsr_history_query(&k,1);CHECK(s&&s->count==1&&!s->status.durable&&s->status.state==(mode==2?DSR_HISTORY_FUTURE:DSR_HISTORY_IO));dsr_history_series_free(s);monotonic+=600000;CHECK(!dsr_history_service_due((uint32_t)monotonic));worker=true;dsr_history_service();worker=false;CHECK(written==before); /* unavailable/future/corrupt storage stays untouched; RAM controls still work */
}
static void missing_bucket(void)
{
 rmfiles();worker=false;CHECK(dsr_history_init()==ESP_OK);worker=true;dsr_history_service();worker=false;
 const uint32_t start=1760000100;clock_utc=start;
 dsr_connection_t c={.layout_id=1,.device={.device_id="shelly-gap",.generation=2,.online=true,.has_power=true,.power_w=10}};
 dsr_history_key_t k={.device_id="shelly-gap",.generation=2,.layout_id=1};
 dsr_history_record(&c,1);clock_utc=start+300;monotonic+=300000;CHECK(dsr_history_service_due((uint32_t)monotonic));worker=true;dsr_history_service();worker=false; /* Existing valid checkpoint must precede the missing bucket. */
 c.device.has_power=false;dsr_history_record(&c,2);clock_utc=start+600;monotonic+=300000;c.device.has_power=true;c.device.power_w=20;dsr_history_record(&c,3);clock_utc=start+900;monotonic+=300000;CHECK(dsr_history_service_due((uint32_t)monotonic));worker=true;dsr_history_service();worker=false;
 dsr_history_series_t *series=dsr_history_query(&k,1);CHECK(series&&series->count==2&&series->status.durable);CHECK(series->points[1].utc-series->points[0].utc==600);dsr_history_series_free(series);
 /* Genuine store reload models reboot; no fabricated query/replay provider. */
 dsr_history_model_t *loaded=calloc(1,sizeof(*loaded));series=calloc(1,sizeof(*series));CHECK(loaded&&series);dsr_history_store_t store;worker=true;CHECK(dsr_history_store_load(&store,loaded,directory));int slot=dsr_history_model_binding(loaded,&k,true);CHECK(slot>=0);dsr_history_model_query(loaded,(unsigned)slot,1,(uint32_t)clock_utc,series);
 fprintf(stderr,"Missing-bucket reboot: valid%u repair%u\n",series->count,(unsigned)store.repair);CHECK(series->count==2&&!store.repair);CHECK(series->points[0].utc==start&&series->points[1].utc==start+600&&series->points[0].power==10&&series->points[1].power==20);
 unsigned char bytes[40];CHECK(dsr_history_encode_point(bytes,0,&(dsr_history_point_t){.utc=start+300})==0);free(series);free(loaded);
}
static void boot_clock_offset(void)
{
 rmfiles();monotonic=120000;worker=false;CHECK(dsr_history_init()==ESP_OK);
 /* LVGL starts after 120 s of board init; all scheduling uses ESP time. */
 CHECK(!dsr_history_service_due(0));CHECK(dsr_history_service_due((uint32_t)monotonic));worker=true;dsr_history_service();worker=false;
 dsr_history_status_t status;dsr_history_status(&status);CHECK(status.state==DSR_HISTORY_READY);
 uint32_t start=(uint32_t)clock_utc;dsr_connection_t c={.layout_id=1,.device={.device_id="shelly-clock",.generation=2,.online=true,.has_power=true,.power_w=42}};
 dsr_history_key_t k={.device_id="shelly-clock",.generation=2};dsr_history_record(&c,1);clock_utc+=300;monotonic+=300000;
 CHECK(dsr_history_service_due((uint32_t)monotonic));no_space=true;worker=true;dsr_history_service();worker=false;dsr_history_status(&status);CHECK(status.state==DSR_HISTORY_IO);
 monotonic+=4999;CHECK(!dsr_history_service_due((uint32_t)monotonic));monotonic++;
 CHECK(dsr_history_service_due((uint32_t)monotonic));CHECK(!dsr_history_service_due((uint32_t)monotonic-120000));
 no_space=false;worker=true;dsr_history_service();worker=false;dsr_history_status(&status);CHECK(status.state==DSR_HISTORY_READY&&status.durable);
 dsr_history_series_t *series=dsr_history_query(&k,1);CHECK(series&&series->count==1&&series->points[0].utc==start&&series->points[0].power==42);dsr_history_series_free(series);
}
int main(int argc,char **argv){CHECK(argc==3);directory=argv[1];unsigned mode=(unsigned)atoi(argv[2]);if(mode==5)boot_clock_offset();else if(mode==4)missing_bucket();else if(mode)unavailable(mode);else{model();store();clock_test();wrapper();}printf("Actual history: %u checks PASS (SDK/clock/allocator models; real bounded journal files)\n",checks);return 0;}
