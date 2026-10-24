#ifndef DSR_MAINTENANCE_PRIVATE_H
#define DSR_MAINTENANCE_PRIVATE_H
#include "dsr_maintenance_model.h"
#include "cJSON.h"
/* Private owner attestations, never exposed as an HTTP/LCD bypass. */
bool dsr_maintenance_lease_valid(const dsr_maint_lease_t *);
bool dsr_maintenance_bonds_snapshot_allowed(void);
bool dsr_maintenance_boot_writes_allowed(void);
/* Validated committed startup owner only, before normal services. */
void dsr_maintenance_boot_authority(bool enabled);
dsr_result_t dsr_maintenance_bonds_capture(const dsr_maint_lease_t *,cJSON *manifest);
bool dsr_workspace_maintenance_quiet(void);
bool dsr_ble_maintenance_quiet(void);
bool dsr_ota_maintenance_quiet(void);
bool dsr_ble_backup_store_owner(void);
/* Acknowledged stop/deinit only; reset must not deserialize obsolete bonds. */
dsr_result_t dsr_ble_maintenance_stop(const dsr_maint_lease_t *);
dsr_result_t dsr_ble_backup_capture(const dsr_maint_lease_t *,cJSON *manifest);
/* Existing config owner, no new network/worker task. Argument ownership moves
 * only on success; callback executes once after queued admission ends. */
bool dsr_workspace_maintenance_submit(void (*run)(void *),void *owned_argument);
#endif
