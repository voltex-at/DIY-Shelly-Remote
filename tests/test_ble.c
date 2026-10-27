#include "dsr_ble.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "../components/dsr_platform/dsr_ble_shared.h"
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
static bool queued,radio,init_failure,submit_failure,idle_release,timed_out,stop_failure;
static unsigned stop_calls,deinit_calls;
static unsigned init_calls,host_calls,cancel_calls,mode;
static void (*worker)(void *);
static int (*callback)(struct ble_gap_event *,void *);
static void *context;
static jmp_buf idle;
#ifdef DSR_TEST_SYSTEM_DATA
#include "dsr_maintenance_private.h"
#include "dsr_backup_bonds.h"
static bool on_manager,owner_pump=true,preempt;
static unsigned captures;static dsr_result_t capture_result=DSR_OK;
TaskHandle_t xTaskGetCurrentTaskHandle(void){return on_manager?(void *)&worker:(void *)&checks;}
bool dsr_maintenance_lease_valid(const dsr_maint_lease_t *l){return l&&dsr_activity_maintenance_valid(l->reservation,l->generation);}
dsr_result_t dsr_backup_bonds_capture(cJSON *root)
{CHECK(dsr_ble_backup_store_owner()&&!dsr_ble_host_active());captures++;if(capture_result!=DSR_OK)return capture_result;cJSON *bonds=cJSON_AddObjectToObject(root,"bonds");CHECK(bonds&&cJSON_AddNumberToObject(bonds,"version",1)&&cJSON_AddArrayToObject(bonds,"records"));return DSR_OK;}
static void run_worker(void);
#endif
struct test_ble_hs_cfg ble_hs_cfg;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return (void *)1;}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t t){(void)s;(void)t;return 1;}
int xSemaphoreGive(SemaphoreHandle_t s){(void)s;return 1;}
void vSemaphoreDelete(SemaphoreHandle_t s){(void)s;}
QueueHandle_t xQueueCreate(unsigned n,unsigned bytes){CHECK(n==1&&bytes==4);return (void *)2;}
int xQueueSend(QueueHandle_t q,const void *v,TickType_t t){(void)q;(void)t;if(submit_failure)return 0;CHECK(!queued);memcpy(&job,v,4);queued=true;return 1;}
int xQueueReceive(QueueHandle_t q,void *v,TickType_t t)
{
 (void)q;
 /* Idle wait: bounded only while the host is up (0.7.52 release timer). */
 if(!queued){if(idle_release&&t!=portMAX_DELAY&&!timed_out){CHECK(t==pdMS_TO_TICKS(DSR_BLE_IDLE_RELEASE_MS));timed_out=true;return 0;}longjmp(idle,1);}
 timed_out=false;memcpy(v,&job,4);queued=false;return 1;
}
void vQueueDelete(QueueHandle_t q){(void)q;}
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *n,unsigned stack,void *a,unsigned p,void *h,int core)
{(void)n;(void)a;(void)p;CHECK(stack==4096&&core==1);worker=fn;if(h)*(void **)h=&worker;return 1;}
int64_t esp_timer_get_time(void){return (int64_t)clock_ms*1000;}
static bool wifi_ble_power;
void dsr_wifi_power_ble(bool active){wifi_ble_power=active;}
esp_err_t nimble_port_init(void){CHECK(wifi_ble_power);init_calls++;return init_failure?ESP_FAIL:ESP_OK;}
void nimble_port_freertos_init(void (*fn)(void *)){(void)fn;host_calls++;ble_hs_cfg.sync_cb();}
void nimble_port_run(void){}
void nimble_port_freertos_deinit(void){}
int nimble_port_stop(void){stop_calls++;return stop_failure?1:0;}
esp_err_t nimble_port_deinit(void){deinit_calls++;return ESP_OK;}
int ble_hs_util_ensure_addr(int p){(void)p;return 0;}
int ble_hs_id_infer_auto(int p,uint8_t *t){(void)p;*t=0;return 0;}
int ble_gap_disc(uint8_t t,int32_t duration,const struct ble_gap_disc_params *p,int (*cb)(struct ble_gap_event *,void *),void *arg)
{(void)t;CHECK(!radio);CHECK(duration==20000&&p->itvl==160&&p->window==40&&!p->passive&&!p->filter_duplicates);radio=true;callback=cb;context=arg;return 0;}
int ble_gap_disc_active(void){return radio;}
int ble_gap_disc_cancel(void){cancel_calls++;radio=false;struct ble_gap_event e={.type=BLE_GAP_EVENT_DISC_COMPLETE,.disc_complete.reason=1};callback(&e,context);return 0;}
void vTaskDelay(TickType_t ticks)
{
 clock_ms+=ticks;
#ifdef DSR_TEST_SYSTEM_DATA
 if(!on_manager&&preempt){CHECK(dsr_activity_stop_begin());dsr_activity_end();preempt=false;}
 if(!on_manager&&owner_pump&&queued)run_worker();
#endif
 if(!radio)return;
 struct ble_gap_event e={.type=BLE_GAP_EVENT_DISC,.disc={.addr={.type=0,.val={1,2,3,4,5,6}},.rssi=-45,.length_data=sizeof(blu),.data=blu}};if(mode==4){e.disc.data=rpc;e.disc.length_data=sizeof(rpc);}callback(&e,context);
 if(mode==1)dsr_ble_cancel();
 if(mode==2)ble_hs_cfg.reset_cb(7);
 if(mode==3||mode==4){radio=false;e.type=BLE_GAP_EVENT_DISC_COMPLETE;e.disc_complete.reason=0;callback(&e,context);CHECK(!dsr_ble_scan());}
}
static void run_worker(void){
#ifdef DSR_TEST_SYSTEM_DATA
 on_manager=true;
#endif
 if(!setjmp(idle))worker(NULL);
#ifdef DSR_TEST_SYSTEM_DATA
 on_manager=false;
#endif
}
static unsigned rpc_calls;
static void quarantine_operation(bool ready)
{CHECK(ready);dsr_ble_manager_quarantine();CHECK(!dsr_ble_host_healthy()&&!dsr_ble_scan());}
void ble_hs_sched_reset(int reason){CHECK(reason==BLE_HS_EUNKNOWN);ble_hs_cfg.reset_cb(reason);ble_hs_cfg.sync_cb();}
static void rpc_operation(bool ready)
{CHECK(ready&&dsr_ble_host_healthy());CHECK(!dsr_ble_scan());dsr_ble_snapshot_t snapshot;dsr_ble_snapshot(&snapshot);CHECK(!snapshot.busy&&snapshot.phase!=DSR_BLE_SCANNING);rpc_calls++;}
static void parser_tests(void)
{
 dsr_ble_device_t d;CHECK(dsr_ble_parse(blu,sizeof(blu),&d));CHECK(d.kind==DSR_BLE_SHELLY_BLU&&d.model_id==2);CHECK(!strcmp(dsr_ble_model_name(&d),"Shelly BLU DoorWindow"));
 CHECK(dsr_ble_parse(bthome,sizeof(bthome),&d));CHECK(d.kind==DSR_BLE_OTHER&&d.bthome_version==2&&!d.encrypted);
 CHECK(dsr_ble_parse(rpc,sizeof(rpc),&d));CHECK(d.kind==DSR_BLE_SHELLY_RPC&&!d.has_model);
 /* Gen2+ Wi-Fi Shelly: 0x0BA9 flags (RPC bit) + MAC + model, no 128-bit UUID. */
 const uint8_t shelly_rpc[]={16,0xff,0xa9,0x0b,0x01,0x05,0x00,0x0a,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x0b,0x34,0x12};
 CHECK(dsr_ble_parse(shelly_rpc,sizeof(shelly_rpc),&d));CHECK(d.kind==DSR_BLE_SHELLY_RPC&&(d.evidence&DSR_BLE_RPC_FLAG)&&d.has_flags&&d.flags==5&&d.has_mac&&d.mac[0]==0xAA&&d.mac[5]==0xFF&&d.has_model&&d.model_id==0x1234&&!dsr_ble_model_name(&d)[0]);
 const uint8_t shelly_no_rpc[]={6,0xff,0xa9,0x0b,0x01,0x01,0x00};
 CHECK(dsr_ble_parse(shelly_no_rpc,sizeof(shelly_no_rpc),&d));CHECK(d.kind==DSR_BLE_OTHER&&(d.evidence&DSR_BLE_MFG)&&d.has_flags&&!(d.evidence&DSR_BLE_RPC_FLAG));
 const uint8_t named[]={17,9,'S','h','e','l','l','y','P','l','u','s','1','-','A','B','C','D'};
 CHECK(dsr_ble_parse(named,sizeof(named),&d));CHECK(d.kind==DSR_BLE_SHELLY_RPC&&(d.evidence&DSR_BLE_NAME));
 const uint8_t blu_name[]={10,9,'S','B','B','T','-','0','0','2','C'};
 CHECK(dsr_ble_parse(blu_name,sizeof(blu_name),&d));CHECK(d.kind==DSR_BLE_SHELLY_BLU);
 {/* Name in the advertisement, flags+MAC in the scan response: one row. */
  dsr_ble_snapshot_t m={0};dsr_ble_begin(&m,1);m.phase=DSR_BLE_SCANNING;uint8_t a[6]={9,8,7,6,5,4};
  CHECK(dsr_ble_ingest(&m,m.generation,a,0,-70,named,sizeof(named),2)&&dsr_ble_ingest(&m,m.generation,a,0,-68,shelly_rpc,sizeof(shelly_rpc),3));
  CHECK(m.count==1&&m.devices[0].kind==DSR_BLE_SHELLY_RPC&&!strcmp(m.devices[0].name,"ShellyPlus1-ABCD")&&m.devices[0].has_mac&&m.devices[0].rssi==-68);
 }
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
#ifdef DSR_TEST_SYSTEM_DATA
static void owner_tests(void)
{
 CHECK(dsr_ble_init()==ESP_OK&&dsr_ble_maintenance_quiet());cJSON *root=cJSON_CreateObject();CHECK(root);dsr_maint_lease_t l;
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));capture_result=DSR_UNSUPPORTED;
 CHECK(dsr_ble_maintenance_stop(&l)==DSR_OK&&!captures&&!init_calls);
 CHECK(dsr_ble_backup_capture(&l,root)==DSR_UNSUPPORTED&&captures==1);capture_result=DSR_OK;captures=0;dsr_activity_maintenance_release(l.reservation);
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));CHECK(!dsr_ble_scan()&&!dsr_ble_manager_rpc(rpc_operation));
 CHECK(dsr_ble_backup_capture(&l,root)==DSR_OK&&captures==1&&init_calls==0&&host_calls==0);dsr_activity_maintenance_release(l.reservation);
 CHECK(dsr_ble_manager_rpc(rpc_operation));run_worker();CHECK(dsr_ble_host_active());unsigned started=init_calls;
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));CHECK(dsr_ble_backup_capture(&l,root)==DSR_OK&&captures==2&&!dsr_ble_host_active()&&stop_calls==1&&deinit_calls==1&&init_calls==started);dsr_activity_maintenance_release(l.reservation);
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));submit_failure=true;CHECK(dsr_ble_backup_capture(&l,root)==DSR_BUSY&&dsr_ble_maintenance_quiet());submit_failure=false;dsr_activity_maintenance_release(l.reservation);
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));owner_pump=false;CHECK(dsr_ble_backup_capture(&l,root)==DSR_BUSY&&queued);dsr_activity_maintenance_release(l.reservation);run_worker();CHECK(captures==2&&dsr_ble_maintenance_quiet());owner_pump=true;
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));preempt=true;CHECK(dsr_ble_backup_capture(&l,root)==DSR_BUSY&&captures==2);dsr_activity_maintenance_release(l.reservation);
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));capture_result=DSR_IO;CHECK(dsr_ble_backup_capture(&l,root)==DSR_IO);dsr_activity_maintenance_release(l.reservation);capture_result=DSR_OK;
 CHECK(dsr_ble_manager_rpc(rpc_operation));run_worker();CHECK(dsr_ble_host_active());
 CHECK(dsr_activity_maintenance_reserve(&l.reservation,&l.generation));stop_failure=true;unsigned before=captures;CHECK(dsr_ble_backup_capture(&l,root)==DSR_BUSY&&captures==before&&dsr_ble_host_active());stop_failure=false;dsr_activity_maintenance_release(l.reservation);
 CHECK(dsr_activity_begin());dsr_activity_end();CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();cJSON_Delete(root);
 printf("PASS: %u existing BLE manager owner checks; actual queue/lifetime code, modeled RTOS/store/radio\n",checks);
}
#endif
int main(int argc,char **argv)
{
#ifdef DSR_TEST_SYSTEM_DATA
 if(argc==2&&!strcmp(argv[1],"--maintenance-owner")){owner_tests();return 0;}
#else
 (void)argc;(void)argv;
#endif
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
 CHECK(dsr_ble_manager_rpc(rpc_operation));CHECK(!dsr_ble_manager_rpc(rpc_operation)&&!dsr_ble_scan());run_worker();CHECK(rpc_calls==1);CHECK(dsr_ble_scan());mode=3;run_worker();
 mode=4;CHECK(dsr_ble_scan());run_worker();dsr_ble_snapshot(&s);CHECK(s.count==1&&s.devices[0].kind==DSR_BLE_SHELLY_RPC);
 dsr_ble_handoff_publish(true,false,clock_ms);CHECK(dsr_ble_handoff_status(clock_ms)==DSR_BLE_HANDOFF_READY);
 CHECK(!dsr_ble_handoff_request(s.serial+1,0)&&!dsr_ble_handoff_request(s.serial,1));
 CHECK(dsr_ble_handoff_request(s.serial,0));CHECK(!dsr_ble_handoff_request(s.serial,0));
 dsr_ble_handoff_publish(true,false,clock_ms-1);CHECK(dsr_ble_handoff_status(clock_ms-1)==DSR_BLE_HANDOFF_REQUESTED);
 dsr_ble_device_t handoff;CHECK(dsr_activity_restart_reserve());CHECK(!dsr_ble_handoff_take(&handoff,clock_ms));dsr_activity_restart_cancel();
 CHECK(dsr_ble_handoff_take(&handoff,clock_ms)&&handoff.kind==DSR_BLE_SHELLY_RPC&&!memcmp(handoff.address,s.devices[0].address,6));CHECK(!dsr_ble_handoff_take(&handoff,clock_ms));
 dsr_ble_handoff_publish(false,true,clock_ms);CHECK(dsr_ble_handoff_status(clock_ms)==DSR_BLE_HANDOFF_ACTIVE);
 dsr_ble_handoff_publish(true,false,clock_ms);CHECK(dsr_ble_handoff_request(s.serial,0));clock_ms+=10000;CHECK(!dsr_ble_handoff_take(&handoff,clock_ms)&&dsr_ble_handoff_status(clock_ms)==DSR_BLE_HANDOFF_EXPIRED);
 dsr_ble_handoff_publish(true,false,clock_ms);CHECK(dsr_ble_handoff_status(clock_ms)==DSR_BLE_HANDOFF_EXPIRED);CHECK(dsr_ble_handoff_request(s.serial,0));dsr_ble_handoff_publish(false,false,clock_ms);CHECK(!dsr_ble_handoff_take(&handoff,clock_ms));
 clock_ms+=30001;dsr_ble_handoff_publish(true,false,clock_ms);CHECK(!dsr_ble_handoff_request(s.serial,0));
 /* 0.7.52: an idle host is released (radio + controller RAM), then re-created by
  * the next job; a failed stop keeps it; never released while a job is queued. */
 CHECK(dsr_ble_host_active());unsigned inits=init_calls;idle_release=true;
 stop_failure=true;mode=3;CHECK(dsr_ble_scan());run_worker();CHECK(stop_calls==1&&!deinit_calls&&dsr_ble_host_active());stop_failure=false;
 CHECK(wifi_ble_power); /* a failed stop keeps modem sleep for the live host */
 mode=3;CHECK(dsr_ble_scan());run_worker();CHECK(stop_calls==2&&deinit_calls==1&&!dsr_ble_host_active()&&!dsr_ble_host_healthy()&&init_calls==inits);
 CHECK(!wifi_ble_power);
 mode=3;CHECK(dsr_ble_scan());run_worker();CHECK(init_calls==inits+1&&deinit_calls==2&&!dsr_ble_host_active()&&!wifi_ble_power);
 CHECK(dsr_ble_manager_rpc(rpc_operation));run_worker();CHECK(rpc_calls==2&&init_calls==inits+2&&deinit_calls==3);
 dsr_ble_snapshot(&s);CHECK(!s.busy);idle_release=false;
 /* A missing disconnect must block only the old BLE host, not restart or
  * other activity. A late sync is not proof that its old callbacks drained. */
 CHECK(dsr_ble_manager_rpc(quarantine_operation));run_worker();CHECK(!dsr_ble_host_healthy());
 CHECK(!dsr_ble_scan()&&!dsr_ble_manager_rpc(rpc_operation));CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 CHECK(dsr_activity_begin());dsr_activity_end(); /* Wi-Fi/OTA admission remains open. */
 idle_release=true;run_worker();CHECK(!dsr_ble_host_active());
 mode=3;CHECK(dsr_ble_scan());run_worker();CHECK(!dsr_ble_host_active());
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: %u BLE checks; idle host release/re-init; real parser + scanner adapter, bounded AD, merge/dedupe/cap, 20s timeout, 10 cancel cycles, host reset, retry, late events; host radio fixture only\n",checks);return 0;
}
