#include "dsr_ble.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static const uint8_t blu[]={6,0xff,0xa9,0x0b,0x0b,2,0};
static const uint8_t bthome[]={6,0x16,0xd2,0xfc,0x40,1,92};
static const uint8_t rpc[]={17,7,0x5f,0x44,0x49,0x5f,0x43,0x56,0x53,0x5f,0x43,0x50,0x52,0x5f,0x53,0x4f,0x6d,0x5f};
static uint32_t clock_ms,job;
static bool queued,radio,init_failure,submit_failure;
static unsigned init_calls,host_calls,cancel_calls,mode;
static void (*worker)(void *);
static int (*callback)(struct ble_gap_event *,void *);
static void *context;
static jmp_buf idle;
struct test_ble_hs_cfg ble_hs_cfg;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return (void *)1;}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t t){(void)s;(void)t;return 1;}
int xSemaphoreGive(SemaphoreHandle_t s){(void)s;return 1;}
void vSemaphoreDelete(SemaphoreHandle_t s){(void)s;}
QueueHandle_t xQueueCreate(unsigned n,unsigned bytes){CHECK(n==1&&bytes==4);return (void *)2;}
int xQueueSend(QueueHandle_t q,const void *v,TickType_t t){(void)q;(void)t;if(submit_failure)return 0;CHECK(!queued);memcpy(&job,v,4);queued=true;return 1;}
int xQueueReceive(QueueHandle_t q,void *v,TickType_t t){(void)q;(void)t;if(!queued)longjmp(idle,1);memcpy(v,&job,4);queued=false;return 1;}
void vQueueDelete(QueueHandle_t q){(void)q;}
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *n,unsigned stack,void *a,unsigned p,void *h,int core)
{(void)n;(void)a;(void)p;(void)h;CHECK(stack==4096&&core==1);worker=fn;return 1;}
int64_t esp_timer_get_time(void){return (int64_t)clock_ms*1000;}
esp_err_t nimble_port_init(void){init_calls++;return init_failure?ESP_FAIL:ESP_OK;}
void nimble_port_freertos_init(void (*fn)(void *)){(void)fn;host_calls++;ble_hs_cfg.sync_cb();}
void nimble_port_run(void){}
void nimble_port_freertos_deinit(void){}
int ble_hs_util_ensure_addr(int p){(void)p;return 0;}
int ble_hs_id_infer_auto(int p,uint8_t *t){(void)p;*t=0;return 0;}
int ble_gap_disc(uint8_t t,int32_t duration,const struct ble_gap_disc_params *p,int (*cb)(struct ble_gap_event *,void *),void *arg)
{(void)t;CHECK(!radio);CHECK(duration==20000&&p->itvl==160&&p->window==40&&!p->passive&&!p->filter_duplicates);radio=true;callback=cb;context=arg;return 0;}
int ble_gap_disc_active(void){return radio;}
int ble_gap_disc_cancel(void){cancel_calls++;radio=false;struct ble_gap_event e={.type=BLE_GAP_EVENT_DISC_COMPLETE,.disc_complete.reason=1};callback(&e,context);return 0;}
void vTaskDelay(TickType_t ticks)
{
 clock_ms+=ticks;
 if(!radio)return;
 struct ble_gap_event e={.type=BLE_GAP_EVENT_DISC,.disc={.addr={.type=0,.val={1,2,3,4,5,6}},.rssi=-45,.length_data=sizeof(blu),.data=blu}};callback(&e,context);
 if(mode==1)dsr_ble_cancel();
 if(mode==2)ble_hs_cfg.reset_cb(7);
 if(mode==3){radio=false;e.type=BLE_GAP_EVENT_DISC_COMPLETE;e.disc_complete.reason=0;callback(&e,context);CHECK(!dsr_ble_scan());}
}
static void run_worker(void){if(!setjmp(idle))worker(NULL);}
static void parser_tests(void)
{
 dsr_ble_device_t d;CHECK(dsr_ble_parse(blu,sizeof(blu),&d));CHECK(d.kind==DSR_BLE_SHELLY_BLU&&d.model_id==2);CHECK(!strcmp(dsr_ble_model_name(&d),"Shelly BLU DoorWindow"));
 CHECK(dsr_ble_parse(bthome,sizeof(bthome),&d));CHECK(d.kind==DSR_BLE_OTHER&&d.bthome_version==2&&!d.encrypted);
 CHECK(dsr_ble_parse(rpc,sizeof(rpc),&d));CHECK(d.kind==DSR_BLE_SHELLY_RPC&&!d.has_model);
 const uint8_t unrelated[]={3,9,'A','B'};CHECK(dsr_ble_parse(unrelated,sizeof(unrelated),&d));CHECK(!d.evidence);
 CHECK(!dsr_ble_parse(NULL,0,&d));CHECK(!dsr_ble_parse(blu,sizeof(blu),NULL));CHECK(!dsr_ble_parse(blu,32,&d));
 for(size_t n=1;n<sizeof(blu);n++)CHECK(!dsr_ble_parse(blu,n,&d));
 const uint8_t short_uuid[]={2,7,1},bad_mfg[]={5,0xff,0xa9,0x0b,0x0b,2},bad_version[]={4,0x16,0xd2,0xfc,0x20};
 CHECK(!dsr_ble_parse(short_uuid,sizeof(short_uuid),&d));CHECK(!dsr_ble_parse(bad_mfg,sizeof(bad_mfg),&d));CHECK(!dsr_ble_parse(bad_version,sizeof(bad_version),&d));
 uint8_t encrypted[]={12,0x16,0xd2,0xfc,0x41,1,2,3,4,5,6,7,8};CHECK(dsr_ble_parse(encrypted,sizeof(encrypted),&d));CHECK(d.encrypted);
 /* Every possible length byte with bounded buffers; fuzz is deterministic. */
 uint32_t seed=1;for(unsigned i=0;i<2000;i++){uint8_t bytes[31];for(unsigned j=0;j<31;j++){seed=seed*1664525u+1013904223u;bytes[j]=(uint8_t)(seed>>24);}dsr_ble_device_t before=d;bool ok=dsr_ble_parse(bytes,1+i%31,&d);if(ok)CHECK(memchr(d.name,0,sizeof(d.name))!=NULL);else CHECK(!memcmp(&before,&d,sizeof(d)));}
 dsr_ble_snapshot_t s={0};dsr_ble_begin(&s,0xfffffff0u);s.phase=DSR_BLE_SCANNING;uint8_t address[6]={1,2,3,4,5,6};
 CHECK(dsr_ble_ingest(&s,s.generation,address,0,-80,bthome,sizeof(bthome),5));CHECK(s.count==1&&s.devices[0].kind==DSR_BLE_OTHER);
 CHECK(dsr_ble_ingest(&s,s.generation,address,0,-42,blu,sizeof(blu),10));CHECK(s.count==1&&s.devices[0].kind==DSR_BLE_SHELLY_BLU&&s.devices[0].rssi==-42&&s.devices[0].seen_ms==10);
 CHECK(dsr_ble_ingest(&s,s.generation,address,1,-42,blu,sizeof(blu),10));CHECK(s.count==2);
 for(unsigned i=2;i<16;i++){address[0]=(uint8_t)i;CHECK(dsr_ble_ingest(&s,s.generation,address,0,-40,blu,sizeof(blu),15));}CHECK(s.count==16);
 address[0]=42;CHECK(!dsr_ble_ingest(&s,s.generation,address,0,-40,blu,sizeof(blu),15));CHECK(s.dropped==1);
 CHECK(dsr_ble_finish(&s,s.generation,DSR_BLE_CANCELLED,16));CHECK(s.elapsed_ms==32&&!s.busy);CHECK(!dsr_ble_ingest(&s,s.generation,address,0,-40,blu,sizeof(blu),17));
 uint32_t previous=s.generation;dsr_ble_begin(&s,50);s.phase=DSR_BLE_SCANNING;CHECK(!dsr_ble_finish(&s,previous,DSR_BLE_DONE,60));CHECK(!dsr_ble_ingest(&s,previous,address,0,-40,blu,sizeof(blu),60));CHECK(s.count==0);
}
int main(void)
{
 parser_tests();dsr_ble_snapshot_t s;
 dsr_ble_snapshot(&s);CHECK(s.phase==DSR_BLE_UNAVAILABLE);CHECK(!dsr_ble_scan());CHECK(dsr_ble_init()==ESP_OK);
 submit_failure=true;CHECK(!dsr_ble_scan());dsr_ble_snapshot(&s);CHECK(!s.busy&&s.phase==DSR_BLE_UNAVAILABLE);submit_failure=false;
 init_failure=true;CHECK(dsr_ble_scan());CHECK(!dsr_ble_scan());run_worker();dsr_ble_snapshot(&s);CHECK(!s.busy&&s.phase==DSR_BLE_UNAVAILABLE);init_failure=false;
 mode=0;CHECK(dsr_ble_scan());run_worker();dsr_ble_snapshot(&s);CHECK(s.phase==DSR_BLE_DONE&&!s.busy&&!radio&&s.count==1);CHECK(init_calls==2&&host_calls==1);
 unsigned initial_cancel=cancel_calls;
 for(unsigned i=0;i<10;i++){mode=1;CHECK(dsr_ble_scan());run_worker();dsr_ble_snapshot(&s);CHECK(s.phase==DSR_BLE_CANCELLED&&!s.busy&&!radio&&s.count==1);}
 CHECK(init_calls==2&&host_calls==1&&cancel_calls==initial_cancel+10);
 mode=2;CHECK(dsr_ble_scan());run_worker();dsr_ble_snapshot(&s);CHECK(s.phase==DSR_BLE_UNAVAILABLE&&!s.busy&&!radio);ble_hs_cfg.sync_cb();
 mode=3;CHECK(dsr_ble_scan());run_worker();dsr_ble_snapshot(&s);CHECK(s.phase==DSR_BLE_DONE&&!s.busy&&!radio);
 uint32_t previous=s.serial;struct ble_gap_event late={.type=BLE_GAP_EVENT_DISC,.disc={.addr={.type=0,.val={1}},.rssi=-20,.length_data=sizeof(rpc),.data=rpc}};callback(&late,context);dsr_ble_snapshot(&s);CHECK(s.serial==previous);
 CHECK(dsr_ble_scan());late.type=BLE_GAP_EVENT_DISC_COMPLETE;late.disc_complete.reason=0;callback(&late,context);dsr_ble_snapshot(&s);CHECK(s.phase==DSR_BLE_STARTING&&s.count==0&&s.busy);run_worker();
 printf("PASS: %u BLE checks; real parser + scanner adapter, bounded AD, merge/dedupe/cap, 20s timeout, 10 cancel cycles, host reset, retry, late events; host radio fixture only\n",checks);return 0;
}
