/* Actual actor and sole worker, deterministic guard/HTTP/NVS providers. */
#define main dsr_legacy_actor_worker_main
#define dsr_http_trv dsr_legacy_typed_trv
#include "test_actions_worker.c"
#undef dsr_http_trv
#undef main
#include "dsr_automation.h"
static bool manual_durable=true,automatic_allowed=true;
bool dsr_automation_manual_ready(uint32_t id,uint32_t intent){CHECK(id==42&&intent);automatic_allowed=false;return manual_durable;}
dsr_result_t dsr_automation_manual_result(uint32_t id,uint32_t intent){CHECK(id==42&&intent);return manual_durable?DSR_OK:DSR_BUSY;}
bool dsr_automation_auto_allowed(uint32_t rule,uint32_t cycle,uint32_t generation,uint32_t id,uint8_t half)
{CHECK(rule==7&&cycle==1&&generation==5&&id==42&&half==16);return automatic_allowed;}
bool dsr_automation_job_owned(uint32_t id){(void)id;return false;}
dsr_result_t dsr_automation_worker_allowed(uint32_t id,const dsr_connection_t *c){(void)id;(void)c;return DSR_OK;}
bool dsr_automation_binding_matches(uint32_t id,const dsr_connection_t *c){return id==42&&c->layout_id==42;}
dsr_result_t dsr_http_trv(dsr_connection_t *c,dsr_trv_intent_t intent){
 CHECK(guard&&write_hook&&intent.command==DSR_TRV_SET_TARGET);dsr_result_t r=guard();if(r!=DSR_OK)return r;
 CHECK(intent.half_c==43);write_hook();posts++;c->device.online=true;return DSR_OK;
}
int main(void){
 CHECK(dsr_actions_init()==ESP_OK&&dsr_worker_start()==ESP_OK);typed_config=true;
 a=(dsr_connection_t){.layout_id=42,.device={.host="192.0.2.42",.device_id="trv-a",.generation=2,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true},
  .telemetry={.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET}}};
 CHECK(dsr_actions_publish_begin(7,true));dsr_actions_publish_at(&a,true,false,DSR_OK,true);dsr_actions_publish_end();
 uint32_t id=42,automatic_run;uint8_t half=16;
 CHECK(dsr_actions_native_reserve(42));CHECK(dsr_actions_auto_request(7,1,5,&id,&half,1,&automatic_run)!=DSR_OK);dsr_actions_native_release(42);
 CHECK(dsr_actions_auto_request(7,1,5,&id,&half,1,&automatic_run)==DSR_OK);
 CHECK(dsr_actions_target()==42);dsr_job_t automatic={.id=100,.connection=a};
 CHECK(dsr_actions_prepare_job(&automatic,DSR_OK,true)&&automatic.kind==DSR_JOB_TRV&&automatic.trv.half_c==16);
 CHECK(dsr_worker_submit(&automatic)==DSR_OK);manual_durable=false;
 dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);char json[256];
 snprintf(json,sizeof(json),"{\"id\":4,\"revision\":7,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"0000000000000abc\"}",s.boot_id,s.admission_generation);
 uint32_t manual_run;CHECK(dsr_actions_request(json,1,0,&manual_run)==DSR_OK);CHECK(manual_run!=automatic_run);
 CHECK(!automatic_allowed);dsr_job_t manual={.id=101,.connection=a};CHECK(!dsr_actions_prepare_job(&manual,DSR_OK,true));
 if(!setjmp(idle))worker(NULL);
 dsr_event_t e;CHECK(dsr_worker_receive(&e)&&e.id==100&&e.result!=DSR_OK&&posts==0);dsr_actions_complete(e.id,e.result);
 dsr_result_t result;CHECK(dsr_actions_auto_result(automatic_run,&result)&&result!=DSR_OK);
 manual_durable=true;CHECK(dsr_actions_prepare_job(&manual,DSR_OK,true)&&manual.trv.half_c==43);
 CHECK(dsr_worker_submit(&manual)==DSR_OK);if(!setjmp(idle))worker(NULL);
 CHECK(dsr_worker_receive(&e)&&e.id==101&&e.result==DSR_OK&&posts==1);dsr_actions_complete(e.id,e.result);
 dsr_actions_snapshot(&s);CHECK(!s.active&&s.run_id==manual_run&&s.steps[0].outcome==DSR_ACTION_SUCCESS);
 dsr_actions_complete(100,DSR_OK);dsr_actions_snapshot(&s);CHECK(s.run_id==manual_run&&s.steps[0].result==DSR_OK);
 dsr_action_log_page_t page;CHECK(dsr_actions_log(0,0,32,&page));bool found=false;
 for(unsigned i=0;i<page.count;i++)if(page.entries[i].kind==4&&page.entries[i].source==3)found=true;
 CHECK(found);printf("automation actor: %u checks passed; manual waits for durable override, auto ACK remains owned\n",checks);return 0;
}
