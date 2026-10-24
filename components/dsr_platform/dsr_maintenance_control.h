#ifndef DSR_MAINTENANCE_CONTROL_H
#define DSR_MAINTENANCE_CONTROL_H
#include "dsr_maintenance_api.h"
#include "dsr_maintenance_stage.h"
/* Trusted transport only. Exact operation/session/id/digest; consumed once. */
bool dsr_maintenance_take(const dsr_maint_request_t *);
void dsr_maintenance_finished(const uint8_t request_id[16],dsr_result_t);
void dsr_maintenance_session_changed(const uint8_t old_session[32]);
bool dsr_maintenance_session_matches(const uint8_t session[32]);
/* Ownership moves on success; upload must already be consumed by this session. */
dsr_result_t dsr_maintenance_candidate(dsr_maint_stage_t *,const uint8_t session[32]);
/* Existing config owner: expiry/cancel garbage collection, never LVGL IO. */
void dsr_maintenance_poll(void);
#endif
