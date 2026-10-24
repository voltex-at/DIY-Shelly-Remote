#ifndef DSR_MAINTENANCE_API_H
#define DSR_MAINTENANCE_API_H
#include "dsr_maintenance_types.h"
/* Request IDs and session bindings are filled by a trusted local handler.
 * IMPORT_COMMIT is produced internally after complete candidate validation. */
dsr_result_t dsr_maintenance_request(const dsr_maint_request_t *);
/* Native physical LCD only. Never expose this call through HTTP or preview. */
dsr_result_t dsr_maintenance_confirm(const uint8_t request_id[16],bool stable_usb);
bool dsr_maintenance_cancel(const uint8_t request_id[16]);
void dsr_maintenance_status(dsr_maint_status_t *);
bool dsr_maintenance_sensitive(void);
#endif
