#ifndef DSR_MAINTENANCE_PRIVATE_H
#define DSR_MAINTENANCE_PRIVATE_H
#include "dsr_maintenance_model.h"
#include "cJSON.h"
/* Private owner attestations, never exposed as an HTTP/LCD bypass. */
bool dsr_maintenance_lease_valid(const dsr_maint_lease_t *);
bool dsr_maintenance_bonds_snapshot_allowed(void);
bool dsr_maintenance_boot_writes_allowed(void);
dsr_result_t dsr_maintenance_bonds_capture(const dsr_maint_lease_t *,cJSON *manifest);
#endif
