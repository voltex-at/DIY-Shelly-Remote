#include "dsr_maintenance_control.h"
#include "dsr_maintenance_private.h"
#include "dsr_activity_internal.h"
#include "dsr_maintenance_store.h"
#include "dsr_ota_power.h"
#include "dsr_restart_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,writes,restarts,power_reads,bond_captures;static uint32_t now=100;
static bool held,queue_busy,power_timeout,mutate_barrier,restart_unavailable,stage_valid;static dsr_ota_power_result_t power=DSR_OTA_POWER_READY;
static void (*job)(void*);static void *arg;static dsr_maint_record_t stored;static bool pending;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"control:%d %s\n",__LINE__,#x);exit(1);}}while(0)
int64_t esp_timer_get_time(void){return (int64_t)now*1000;}
bool dsr_boot_ready(void){return true;}
bool dsr_workspace_maintenance_quiet(void){return true;}
bool dsr_backup_sources_quiet(void){return true;}
bool dsr_ble_maintenance_quiet(void){return true;}
bool dsr_ota_maintenance_quiet(void){return true;}
bool dsr_ble_backup_store_owner(void){return false;}
dsr_result_t dsr_ble_backup_capture(const dsr_maint_lease_t *l,cJSON *root){(void)root;CHECK(dsr_maintenance_lease_valid(l));bond_captures++;if(mutate_barrier)dsr_activity_data_changed();return DSR_UNSUPPORTED;}
dsr_result_t dsr_ble_maintenance_stop(const dsr_maint_lease_t *l){CHECK(dsr_maintenance_lease_valid(l));if(mutate_barrier)dsr_activity_data_changed();return DSR_OK;}
bool dsr_workspace_maintenance_submit(void (*fn)(void*),void *p){if(queue_busy||job)return false;job=fn;arg=p;return true;}
static void run(void){CHECK(job!=NULL);void (*fn)(void*)=job;job=NULL;fn(arg);arg=NULL;}
void dsr_backup_json_discard(cJSON *p){cJSON_Delete(p);}
dsr_result_t dsr_ota_power_acquire(dsr_ota_power_result_t *out){power_reads++;if(power_timeout){*out=DSR_OTA_POWER_BUSY;return DSR_BUSY;}CHECK(dsr_activity_begin());held=true;*out=power;return DSR_OK;}
void dsr_ota_power_release(void){CHECK(held);held=false;dsr_activity_end();}
dsr_result_t dsr_backup_device_id(uint8_t out[32]){memset(out,3,32);return DSR_OK;}
dsr_result_t dsr_maint_io_identifier(uint8_t out[16]){memset(out,4,16);return DSR_OK;}
dsr_result_t dsr_maint_io_random(uint8_t *out,size_t n){static uint8_t serial=5;memset(out,++serial,n);return DSR_OK;}
dsr_result_t dsr_maint_store_begin(dsr_maint_record_t *r){CHECK(!held);writes++;stored=*r;stored.sequence=r->sequence=2;pending=true;return DSR_OK;}
dsr_result_t dsr_maint_store_write(dsr_maint_record_t *r){CHECK(!held);writes++;stored=*r;stored.sequence=++r->sequence;return DSR_OK;}
dsr_result_t dsr_maint_store_load(dsr_maint_record_t *r,bool *found,bool *p){*r=stored;*found=pending;*p=pending;return DSR_OK;}
dsr_result_t dsr_maint_store_finish(dsr_maint_record_t *r,bool cancel){(void)r;(void)cancel;return DSR_OK;}
dsr_restart_result_t dsr_restart_request_maintenance(uint64_t r,uint64_t g){CHECK(dsr_activity_maintenance_commit_valid(r,g));CHECK(stored.phase==DSR_MAINT_COMMIT);if(restart_unavailable)return DSR_RESTART_UNAVAILABLE;restarts++;return DSR_RESTART_ACCEPTED;}
dsr_result_t dsr_maint_stage_barrier(dsr_maint_stage_t *s,const dsr_maint_lease_t *l){(void)s;(void)l;return DSR_OK;}
dsr_result_t dsr_maint_stage_commit_verified(dsr_maint_stage_t *s,const dsr_maint_lease_t *l){(void)s;(void)l;return DSR_BUSY;}
bool dsr_maint_stage_record(const dsr_maint_stage_t *s,dsr_maint_record_t *r){(void)s;memset(r,0,sizeof(*r));r->generation=dsr_activity_data_generation();r->candidate_sha256[0]=1;return stage_valid;}
bool dsr_maint_stage_archive_info(const dsr_maint_stage_t *s,dsr_maint_archive_info_t *out){(void)s;*out=(dsr_maint_archive_info_t){.created=1792800000,.firmware="0.7.70-dev"};return stage_valid;}
dsr_result_t dsr_maint_stage_cancel(dsr_maint_stage_t *s){(void)s;return DSR_OK;}
void dsr_maint_stage_free(dsr_maint_stage_t *s){(void)s;}
int main(void)
{
 dsr_maint_request_t r={.op=DSR_MAINT_EXPORT,.nonce={1},.session={2}},bad;dsr_maint_status_t st;
 CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(!writes&&!dsr_maintenance_take(&r));
 CHECK(dsr_maintenance_confirm((uint8_t[16]){3},false)==DSR_CONFLICT);
 CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);CHECK(!job&&!power_reads&&!writes);
 bad=r;bad.op=DSR_MAINT_IMPORT_UPLOAD;CHECK(!dsr_maintenance_take(&bad));bad=r;bad.session[1]=3;CHECK(!dsr_maintenance_take(&bad));
 CHECK(dsr_maintenance_take(&r));CHECK(!dsr_maintenance_take(&r));dsr_maintenance_finished(r.nonce,DSR_OK);
 r.nonce[0]++;r.op=DSR_MAINT_IMPORT_COMMIT;r.candidate_sha256[0]=1;CHECK(dsr_maintenance_request(&r)==DSR_INVALID);
 r.op=DSR_MAINT_IMPORT_UPLOAD;r.candidate_sha256[0]=0;CHECK(dsr_maintenance_request(&r)==DSR_OK);
 CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);dsr_maintenance_session_changed(r.session);CHECK(!dsr_maintenance_take(&r));
 r.nonce[0]++;CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);dsr_activity_data_changed();CHECK(!dsr_maintenance_take(&r));
 r.nonce[0]++;CHECK(dsr_maintenance_request(&r)==DSR_OK);now+=120000;dsr_maintenance_status(&st);CHECK(!st.can_confirm);CHECK(!dsr_maintenance_take(&r));
 r.nonce[0]++;CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);CHECK(dsr_maintenance_take(&r));stage_valid=true;
 CHECK(dsr_maintenance_candidate((dsr_maint_stage_t*)1,&r)==DSR_OK);dsr_maintenance_status(&st);CHECK(st.op==DSR_MAINT_IMPORT_COMMIT&&st.can_confirm&&memcmp(st.request_id,r.nonce,16));
 CHECK(st.archive.created==1792800000&&!strcmp(st.archive.firmware,"0.7.70-dev"));
 CHECK(dsr_maintenance_cancel(r.nonce));dsr_maintenance_status(&st);CHECK(!st.can_confirm);dsr_maintenance_poll();stage_valid=false;
 r.nonce[0]++;CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);CHECK(dsr_maintenance_take(&r));stage_valid=true;
 CHECK(dsr_maintenance_cancel(r.nonce));CHECK(dsr_maintenance_candidate((dsr_maint_stage_t*)1,&r)==DSR_CONFLICT);
 dsr_maint_request_t old_upload=r;r.nonce[0]++;CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);CHECK(dsr_maintenance_take(&r));
 CHECK(dsr_maintenance_candidate((dsr_maint_stage_t*)1,&old_upload)==DSR_CONFLICT);CHECK(dsr_maintenance_cancel(r.nonce));stage_valid=false;
 r.nonce[0]++;r.op=DSR_MAINT_RESET;memset(r.session,0,32);CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(!writes);
 queue_busy=true;CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_BUSY);queue_busy=false;
 power=DSR_OTA_POWER_LOW;CHECK(dsr_maintenance_confirm(r.nonce,true)==DSR_OK);CHECK(!writes);run();CHECK(!writes&&!restarts);dsr_maintenance_status(&st);CHECK(st.result==DSR_OFFLINE&&st.can_confirm);
 power_timeout=true;CHECK(dsr_maintenance_confirm(r.nonce,true)==DSR_OK);run();dsr_maintenance_status(&st);CHECK(st.result==DSR_BUSY&&!st.usb_required&&!writes);power_timeout=false;
 power=DSR_OTA_POWER_UNKNOWN;CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);run();dsr_maintenance_status(&st);CHECK(st.usb_required&&!writes);
 power_timeout=true;CHECK(dsr_maintenance_confirm(r.nonce,true)==DSR_OK);run();dsr_maintenance_status(&st);CHECK(st.result==DSR_BUSY&&!st.usb_required&&!writes);power_timeout=false;
 power=DSR_OTA_POWER_READY;mutate_barrier=true;CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);run();CHECK(!writes&&!restarts);mutate_barrier=false;
 r.nonce[0]++;CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);CHECK(dsr_maintenance_cancel(r.nonce));run();CHECK(!writes);
 r.nonce[0]++;CHECK(dsr_maintenance_request(&r)==DSR_OK);CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_OK);restart_unavailable=true;run();CHECK(writes==2&&restarts==0&&!bond_captures);CHECK(!held&&!dsr_activity_begin());restart_unavailable=false;CHECK(dsr_maintenance_restart()==DSR_OK&&restarts==1);
 dsr_maintenance_status(&st);CHECK(st.phase==DSR_MAINT_COMMITTED&&!st.can_confirm);CHECK(!dsr_maintenance_cancel(r.nonce));CHECK(dsr_maintenance_confirm(r.nonce,false)==DSR_CONFLICT);
 printf("PASS: %u physical control checks; real grants/admission, modeled power/queue/flash/restart\n",checks);return 0;
}
