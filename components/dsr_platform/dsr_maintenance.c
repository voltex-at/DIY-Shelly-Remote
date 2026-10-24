#include "dsr_maintenance.h"
#include "dsr_maintenance_private.h"
#include "dsr_activity_internal.h"
#include "dsr_backup_sources.h"
#include "dsr_boot.h"
#include <string.h>
bool dsr_maintenance_acquire(dsr_maint_lease_t *lease)
{
 if(!lease)return false;
 memset(lease,0,sizeof(*lease));
 if(!dsr_boot_ready()||!dsr_activity_maintenance_reserve(&lease->reservation,&lease->generation))return false;
 if(!dsr_workspace_maintenance_quiet()||!dsr_backup_sources_quiet()||!dsr_ble_maintenance_quiet()||!dsr_ota_maintenance_quiet()||!dsr_maintenance_lease_valid(lease)){
  dsr_maintenance_release(lease);memset(lease,0,sizeof(*lease));return false;
 }return true;
}
void dsr_maintenance_release(const dsr_maint_lease_t *lease)
{if(lease)dsr_activity_maintenance_release(lease->reservation);}
bool dsr_maintenance_lease_valid(const dsr_maint_lease_t *lease)
{return lease&&dsr_activity_maintenance_valid(lease->reservation,lease->generation);}
dsr_maint_generation_t dsr_maintenance_generation(void){return dsr_activity_data_generation();}
bool dsr_maintenance_generation_matches(dsr_maint_generation_t gen){return gen&&gen==dsr_maintenance_generation();}
void dsr_maintenance_mutated(void){dsr_activity_data_changed();}
bool dsr_maintenance_bonds_snapshot_allowed(void)
{return dsr_activity_maintenance_reserved()&&dsr_ble_backup_store_owner();}
dsr_result_t dsr_maintenance_bonds_capture(const dsr_maint_lease_t *lease,cJSON *root)
{return dsr_maintenance_lease_valid(lease)?dsr_ble_backup_capture(lease,root):DSR_BUSY;}
/* Task6 connects a validated committed boot intent. No ordinary caller is
 * authorized while that recovery controller is not installed. */
bool dsr_maintenance_boot_writes_allowed(void){return false;}
void dsr_maintenance_snapshot(dsr_maint_status_t *out)
{
 if(!out)return;
 memset(out,0,sizeof(*out));out->phase=dsr_activity_maintenance_committed()?DSR_MAINT_COMMITTED:dsr_activity_maintenance_reserved()?DSR_MAINT_CAPTURING:DSR_MAINT_IDLE;
}
