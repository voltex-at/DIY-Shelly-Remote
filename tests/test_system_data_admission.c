#include "dsr_maintenance.h"
#include "dsr_maintenance_private.h"
#include "dsr_activity_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;static bool ready=true,workspace=true,heating=true,ble=true,ota=true;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"admission:%d %s\n",__LINE__,#x);exit(1);}}while(0)
bool dsr_boot_ready(void){return ready;}
bool dsr_workspace_maintenance_quiet(void){return workspace;}
bool dsr_backup_sources_quiet(void){return heating;}
bool dsr_ble_maintenance_quiet(void){return ble;}
bool dsr_ota_maintenance_quiet(void){return ota;}
bool dsr_ble_backup_store_owner(void){return false;}
dsr_result_t dsr_ble_backup_capture(const dsr_maint_lease_t *l,cJSON *root){(void)l;(void)root;return DSR_BUSY;}
int main(void)
{
 dsr_maint_lease_t lease={0},next={0};CHECK(dsr_activity_begin()&&dsr_activity_begin());CHECK(!dsr_maintenance_acquire(&lease));CHECK(!dsr_activity_restart_pending());
 dsr_activity_end();CHECK(!dsr_maintenance_acquire(&lease));dsr_activity_end();
 bool *owners[]={&ready,&workspace,&heating,&ble,&ota};
 for(unsigned i=0;i<5;i++){*owners[i]=false;CHECK(!dsr_maintenance_acquire(&lease));CHECK(dsr_activity_begin());dsr_activity_end();*owners[i]=true;}
 CHECK(dsr_maintenance_acquire(&lease)&&lease.reservation&&lease.generation);CHECK(dsr_maintenance_lease_valid(&lease));CHECK(!dsr_activity_begin());
 CHECK(!dsr_activity_restart_reserve()&&!dsr_activity_restart_confirmed_reserve());CHECK(!dsr_maintenance_acquire(&next));
 uint64_t gen=dsr_maintenance_generation();dsr_maintenance_mutated();CHECK(dsr_maintenance_generation()==gen+1&&!dsr_maintenance_lease_valid(&lease));
 dsr_maintenance_release(&lease);CHECK(dsr_maintenance_acquire(&next));CHECK(next.reservation!=lease.reservation);dsr_maintenance_release(&lease);CHECK(dsr_maintenance_lease_valid(&next));
 CHECK(dsr_activity_stop_begin());CHECK(!dsr_maintenance_lease_valid(&next));CHECK(!dsr_maintenance_acquire(&lease));dsr_activity_end();CHECK(!dsr_maintenance_acquire(&lease));
 dsr_maintenance_release(&next);CHECK(dsr_maintenance_acquire(&lease));dsr_maintenance_release(&lease);dsr_maintenance_release(&lease);
 CHECK(dsr_activity_begin());dsr_activity_end();CHECK(dsr_activity_restart_reserve());CHECK(!dsr_maintenance_acquire(&lease));dsr_activity_restart_cancel();
 CHECK(dsr_activity_begin());dsr_activity_end();CHECK(dsr_activity_restart_confirmed_reserve());CHECK(!dsr_maintenance_acquire(&lease));dsr_activity_restart_cancel();
 dsr_maint_status_t status;memset(&status,0xff,sizeof(status));dsr_maintenance_snapshot(&status);CHECK(status.phase==DSR_MAINT_IDLE&&!status.can_confirm);
 CHECK(!dsr_maintenance_boot_writes_allowed()&&!dsr_maintenance_bonds_snapshot_allowed());
 printf("PASS: %u maintenance admission checks; real atomic gate, modeled owner attestations\n",checks);return 0;
}
