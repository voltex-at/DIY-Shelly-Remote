/* Actual native pending-intent controller; no LVGL or radio in this case. */
#include "dsr_automation.h"
#include "dsr_actions.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,queued,retained,cleared;static bool durable,io_failure;static uint32_t last_intent;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#x);exit(1);}}while(0)
void *heap_caps_calloc(size_t n,size_t s,unsigned flags){CHECK(flags==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return calloc(n,s);}
bool dsr_automation_manual_ready(uint32_t id,uint32_t intent){CHECK(id==3&&intent);last_intent=intent;return durable;}
dsr_result_t dsr_automation_manual_result(uint32_t id,uint32_t intent){CHECK(id==3&&intent);return io_failure?DSR_IO:DSR_BUSY;}
bool dsr_actions_reserved(const dsr_device_t *d){(void)d;return false;}
bool dsr_actions_native_reserve(uint32_t id){CHECK(id==3);return true;}
void dsr_actions_native_release(uint32_t id){CHECK(id==3);}
void dsr_mobile_trv_request_state(uint32_t id,uint8_t half,uint32_t intent,bool keep){CHECK(id==3&&half>=8&&half<=60&&intent);if(keep)retained++;else cleared++;}
dsr_result_t dsr_fleet_trv_request(dsr_fleet_t *f,unsigned i,dsr_trv_intent_t intent,bool wifi,uint32_t now){(void)now;CHECK(durable&&wifi&&!f->slots[i].in_flight&&intent.command==DSR_TRV_SET_TARGET);queued++;f->slots[i].result=DSR_OK;f->slots[i].queued=true;f->slots[i].trv=intent;return DSR_OK;}
#include "../components/dsr_ui/dsr_ui_network_automation.inc"
int main(void){
 CHECK(!native_trv_waiting());
 dsr_fleet_t f={.count=1};dsr_fleet_slot_t *s=&f.slots[0];s->layout_enabled=true;s->connection=(dsr_connection_t){.layout_id=3,.device={.device_id="TRV-A",.host="192.0.2.1",.generation=2,.endpoint_kind=DSR_ENDPOINT_BLU},.telemetry={.blu={.errors=DSR_BLU_TRV}}};
 CHECK(native_trv_request(&f,0,44)==DSR_OK&&retained==1&&queued==0&&native_trv_busy(3)&&native_trv_waiting());uint32_t first=last_intent;
 native_trv_tick(&f,1000,true);CHECK(queued==0&&native_trv_waiting());durable=true;s->in_flight=true;native_trv_tick(&f,2000,true);CHECK(queued==0&&native_trv_waiting());
 s->in_flight=false;native_trv_tick(&f,3000,true);CHECK(queued==1&&s->trv.half_c==44&&!native_trv_waiting());
 dsr_job_t j={.id=77,.kind=DSR_JOB_TRV,.connection=s->connection,.trv=s->trv};native_trv_started(&j);native_trv_completed(&f,76,DSR_OK);CHECK(cleared==0);native_trv_completed(&f,77,DSR_IO);CHECK(cleared==0&&!native_trv_busy(3));s->queued=false;
 durable=false;io_failure=true;CHECK(native_trv_request(&f,0,45)==DSR_OK&&last_intent!=first);native_trv_tick(&f,4000,true);CHECK(s->result==DSR_IO&&queued==1&&!native_trv_busy(3)&&cleared==0);
 io_failure=false;CHECK(native_trv_request(&f,0,46)==DSR_OK);strcpy(s->connection.device.device_id,"replacement");native_trv_tick(&f,5000,true);CHECK(cleared==1&&queued==1&&!native_trv_busy(3));
 CHECK(native_trv_request(&f,0,47)==DSR_OK);durable=true;native_trv_tick(&f,6000,true);CHECK(queued==2);j=(dsr_job_t){.id=78,.kind=DSR_JOB_TRV,.connection=s->connection,.trv=s->trv};native_trv_started(&j);native_trv_completed(&f,78,DSR_OK);CHECK(cleared==2&&!native_trv_busy(3));
 s->queued=false;s->connection.telemetry.trv_target_c=24;s->connection.telemetry.blu.errors|=DSR_BLU_TRV_TARGET;
 CHECK(native_trv_request(&f,0,48)==DSR_OK);native_trv_tick(&f,7000,true);CHECK(queued==3); /* same-value manual override still durable */
 j=(dsr_job_t){.id=79,.kind=DSR_JOB_TRV,.connection=s->connection,.trv=s->trv};native_trv_started(&j);s->trv_followups=3;
 native_trv_completed(&f,79,DSR_OK);CHECK(cleared==2&&!native_trv_waiting());s->queued=false;native_trv_tick(&f,8000,true);CHECK(cleared==2&&queued==3);
 native_trv_tick(&f,8100,false);CHECK(cleared==2&&native_trv_busy(3)); /* leaving the control page pauses; it does not orphan the draft */
 s->trv_followups=0;native_trv_tick(&f,9000,true);CHECK(cleared==3&&queued==3&&!native_trv_busy(3));
 CHECK(native_trv_request(&f,0,49)==DSR_OK);native_trv_tick(&f,10000,true);j=(dsr_job_t){.id=80,.kind=DSR_JOB_TRV,.connection=s->connection,.trv=s->trv};native_trv_started(&j);native_trv_completed(&f,80,DSR_OFFLINE);s->queued=false;s->result=DSR_OFFLINE;s->connection.device.online=false;
 native_trv_tick(&f,11000,true);CHECK(cleared==3&&queued==4);s->result=DSR_OK;s->connection.device.online=true;native_trv_tick(&f,12000,true);CHECK(cleared==4&&queued==4); /* later confirmed GET ends a failed draft; no write replay */
 printf("automation native controller: %u checks passed; no write before durable override or after rebind\n",checks);free(native_trvs);return 0;
}
