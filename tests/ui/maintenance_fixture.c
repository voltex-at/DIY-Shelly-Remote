/* LCD-only model; production grant/power/commit/recovery have C cases. */
#include "dsr_maintenance_api.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>
static dsr_maint_status_t state;static unsigned confirmations,resets,cancels;static bool usb;
void test_maintenance_set(unsigned op,unsigned phase,dsr_result_t result,unsigned id)
{state=(dsr_maint_status_t){.op=(uint8_t)op,.phase=(uint8_t)phase,.result=result,.revision=id,.can_confirm=phase==DSR_MAINT_CONFIRM,.photos=16,.connections=16,.rules=8};state.request_id[0]=(uint8_t)id;}
void test_maintenance_usb(void){state.usb_required=true;}
void test_maintenance_archive(uint32_t created,const char *firmware){state.archive.created=created;snprintf(state.archive.firmware,sizeof(state.archive.firmware),"%s",firmware);state.revision++;}
unsigned test_maintenance_confirms(void){return confirmations;}
unsigned test_maintenance_resets(void){return resets;}
bool test_maintenance_last_usb(void){return usb;}
void dsr_maintenance_status(dsr_maint_status_t *out){*out=state;}
bool dsr_maintenance_sensitive(void){return state.phase==DSR_MAINT_CONFIRM||state.phase==DSR_MAINT_WAITING||state.phase==DSR_MAINT_COMMITTED;}
dsr_result_t dsr_maintenance_local_reset_request(void){test_maintenance_set(DSR_MAINT_RESET,DSR_MAINT_CONFIRM,DSR_OK,(unsigned)state.request_id[0]+1);return DSR_OK;}
dsr_result_t dsr_maintenance_confirm(const uint8_t id[16],bool stable_usb)
{if(!state.can_confirm||memcmp(id,state.request_id,16))return DSR_CONFLICT;confirmations++;if(state.op==DSR_MAINT_RESET)resets++;usb=stable_usb;state.can_confirm=false;state.phase=DSR_MAINT_WAITING;return DSR_OK;}
bool dsr_maintenance_cancel(const uint8_t id[16]){if(memcmp(id,state.request_id,16)||state.phase==DSR_MAINT_COMMITTED)return false;state.phase=DSR_MAINT_FAULT;state.result=DSR_CONFLICT;state.can_confirm=false;cancels++;return true;}
dsr_result_t dsr_maintenance_restart(void){return DSR_BUSY;}
