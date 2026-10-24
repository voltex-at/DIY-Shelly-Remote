#ifndef DSR_MAINTENANCE_GRANT_H
#define DSR_MAINTENANCE_GRANT_H
#include "dsr_maintenance_types.h"
#include "dsr_ota_power.h"
enum {DSR_GRANT_EMPTY,DSR_GRANT_WAIT,DSR_GRANT_READY,DSR_GRANT_RUNNING,DSR_GRANT_DONE};
enum {DSR_MAINT_CONSENT_MS=120000,DSR_MAINT_POWER_MS=1000};
/* Pure model. Controller serializes access; no pointers into HTTP/LVGL data. */
typedef struct {
 dsr_maint_request_t request;uint64_t generation;uint32_t started;
 dsr_result_t result;uint8_t state;bool usb_required;
} dsr_maint_grant_t;
dsr_result_t dsr_maint_grant_begin(dsr_maint_grant_t *,const dsr_maint_request_t *,uint64_t generation,uint32_t now);
bool dsr_maint_grant_live(dsr_maint_grant_t *,uint64_t generation,uint32_t now);
bool dsr_maint_grant_confirm(dsr_maint_grant_t *,const uint8_t id[16],uint64_t generation,uint32_t now,bool usb,dsr_ota_power_result_t);
bool dsr_maint_grant_take(dsr_maint_grant_t *,const dsr_maint_request_t *,uint64_t generation,uint32_t now);
bool dsr_maint_grant_cancel(dsr_maint_grant_t *,const uint8_t id[16]);
void dsr_maint_grant_invalidate_session(dsr_maint_grant_t *,const uint8_t session[32]);
void dsr_maint_grant_finish(dsr_maint_grant_t *,dsr_result_t);
#endif
