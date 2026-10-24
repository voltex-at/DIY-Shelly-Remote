/* Reuse bounded staged-file/owner providers, not a second implementation of
 * codecs, AES, journal or admission. All data are generated synthetic values. */
#define main stage_fixture_unused_main
#include "test_system_data_stage.c"
#undef main
#include "dsr_maintenance_recovery.h"
#include "dsr_maintenance_recovery_io.h"
#include "dsr_ota_power.h"
#include "nvs.h"
static uint8_t final_photo[115200];static bool final_present;static uint32_t checkpoints[2];
static unsigned sources_applied,bonds_applied,unpaired,reset_mask,part_calls;
static int cut_effect=-1;static bool fail_mount;
static dsr_ota_power_result_t recovery_power=DSR_OTA_POWER_READY;
dsr_ota_power_result_t dsr_ota_power_read(void){return recovery_power;}
dsr_result_t dsr_maint_io_identifier(uint8_t out[16]){memset(out,42,16);return DSR_OK;}
static dsr_result_t effect(void){if(cut_effect<0)return DSR_OK;if(cut_effect--==0){cut_effect=-1;return DSR_IO;}return DSR_OK;}
dsr_result_t dsr_maint_recovery_mount(void){return fail_mount?DSR_IO:DSR_OK;}
dsr_result_t dsr_maint_recovery_photo_read(const char id[33],uint8_t out[115200]){CHECK(strlen(id)==32);if(!final_present)return DSR_IO;memcpy(out,final_photo,115200);return DSR_OK;}
dsr_result_t dsr_maint_recovery_photo_write(const char id[33],const uint8_t p[115200]){CHECK(strlen(id)==32&&dsr_maintenance_boot_writes_allowed());memcpy(final_photo,p,115200);final_present=true;return effect();}
dsr_result_t dsr_maint_recovery_checkpoint(const cJSON *config,uint32_t revision,unsigned slot){CHECK(dsr_maintenance_boot_writes_allowed()&&slot<2&&cJSON_IsObject(config)&&revision>900);checkpoints[slot]=revision;return effect();}
dsr_result_t dsr_maint_recovery_unpair(void){CHECK(dsr_maintenance_boot_writes_allowed());unpaired++;return effect();}
dsr_result_t dsr_maint_recovery_reset_part(unsigned part){CHECK(dsr_maintenance_boot_writes_allowed()&&part<10);part_calls++;reset_mask|=1u<<part;return effect();}
dsr_result_t dsr_backup_sources_apply(const cJSON *root){CHECK(dsr_maintenance_boot_writes_allowed()&&cJSON_IsObject(root));sources_applied++;return effect();}
dsr_result_t dsr_backup_bonds_apply(const cJSON *root){CHECK(dsr_maintenance_boot_writes_allowed()&&cJSON_IsObject(root));bonds_applied++;return effect();}
static void progress(const dsr_maint_status_t *s){CHECK(s&&s->phase>=DSR_MAINT_APPLYING&&!s->can_confirm);}
int main(int argc,char **argv)
{
 CHECK(argc==3);unsigned scenario=(unsigned)strtoul(argv[2],NULL,10);CHECK(scenario<61);dsr_maint_record_t record,loaded;bool found,pending;
 if(scenario==60){
  test_boot_ready=false;CHECK(dsr_maintenance_recovery_reset(true)==DSR_BUSY);
  nvs_handle_t h;CHECK(nvs_open("dsr_maint",NVS_READWRITE,&h)==ESP_OK);CHECK(nvs_set_u8(h,"pending",1)==ESP_OK);nvs_close(h);CHECK(dsr_maintenance_recover(progress)==DSR_INVALID);
  unsigned before=test_nvs_writes;recovery_power=DSR_OTA_POWER_LOW;CHECK(dsr_maintenance_recovery_reset(true)==DSR_OFFLINE&&test_nvs_writes==before);
  recovery_power=DSR_OTA_POWER_UNKNOWN;CHECK(dsr_maintenance_recovery_reset(false)==DSR_INVALID&&test_nvs_writes==before);
  CHECK(dsr_maintenance_recovery_reset(true)==DSR_OK);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&pending&&loaded.op==DSR_MAINT_RESET&&loaded.phase==DSR_MAINT_COMMIT);
  CHECK(dsr_maintenance_recover(progress)==DSR_OK&&reset_mask==1023);
 }else if(scenario>=50){
  record=(dsr_maint_record_t){.sequence=1,.generation=1,.op=DSR_MAINT_RESET,.phase=DSR_MAINT_PREPARING};record.operation_id[0]=1;CHECK(dsr_backup_device_id(record.device)==DSR_OK);CHECK(dsr_maint_store_begin(&record)==DSR_OK);record.phase=DSR_MAINT_COMMIT;CHECK(dsr_maint_store_write(&record)==DSR_OK);
  test_boot_ready=false;cut_effect=(int)(scenario-50);CHECK(dsr_maintenance_recover(progress)==DSR_IO);CHECK(!dsr_maintenance_boot_writes_allowed());CHECK(dsr_maintenance_recover(progress)==DSR_OK);CHECK(reset_mask==1023&&part_calls>=10);
 }else{
  dsr_backup_image_t *image=test_backup_image(argv[1],1,16);CHECK(image);dsr_maint_stage_t *stage=NULL;CHECK(dsr_maint_stage_prepare(image,&stage)==DSR_OK);CHECK(dsr_maint_stage_record(stage,&record));
  if(scenario==44){test_boot_ready=false;CHECK(dsr_maintenance_recover(progress)==DSR_OK&&!files&&!sources_applied&&!reset_mask);goto done_image;}
  dsr_maint_lease_t lease;CHECK(dsr_maintenance_acquire(&lease));CHECK(dsr_maint_stage_commit(stage,&lease)==DSR_OK);dsr_maintenance_release(&lease);test_boot_ready=false;
  if(scenario==42){disk[0].bytes[128]^=1;CHECK(dsr_maintenance_recover(progress)==DSR_BAD_RESPONSE&&!sources_applied&&!final_present&&!checkpoints[0]);goto done_image;}
  if(scenario==43){fail_mount=true;CHECK(dsr_maintenance_recover(progress)==DSR_IO&&!sources_applied);fail_mount=false;}
  if(scenario==45||scenario==46){cut_effect=1;CHECK(dsr_maintenance_recover(progress)==DSR_IO&&final_present);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&loaded.applied_photos==1);if(scenario==45)final_present=false;else final_photo[1]^=1;CHECK(dsr_maintenance_recover(progress)!=DSR_OK);goto done_image;}
  if(scenario==47){char name[32];CHECK(dsr_maint_record_name(record.operation_id,1,name));CHECK(dsr_maint_io_remove(name)==DSR_OK);CHECK(dsr_maintenance_recover(progress)==DSR_IO&&!final_present&&!sources_applied);goto done_image;}
  if(scenario==48)remove_fail_at=1; /* receipt durable, metadata cleanup fails */
  if(scenario==49){bond_supported=false;CHECK(dsr_maintenance_recover(progress)==DSR_UNSUPPORTED&&!sources_applied&&!final_present);goto done_image;}
  if(scenario<6)cut_effect=(int)scenario;else if(scenario<42)test_nvs_fail_after=(int)(scenario-6);
  dsr_result_t first=dsr_maintenance_recover(progress);CHECK(!dsr_maintenance_boot_writes_allowed());cut_effect=-1;test_nvs_fail_after=-1;
  CHECK(first==DSR_OK||first==DSR_IO);CHECK(dsr_maintenance_recover(progress)==DSR_OK);
  CHECK(final_present&&sources_applied&&bonds_applied&&unpaired&&checkpoints[0]>900&&checkpoints[0]==checkpoints[1]&&!files);
done_image:
  dsr_maint_stage_free(stage);dsr_backup_image_free(image);
 }
 CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK);CHECK((scenario==42||(scenario>=45&&scenario<=47)||scenario==49)?pending:!pending);CHECK(!dsr_maintenance_boot_writes_allowed());
 for(unsigned i=0;i<17;i++)free(disk[i].bytes);
 test_nvs_reset();
 printf("PASS: %u recovery checks scenario%u; actual encrypted staging/journal/recovery, destination IO MODEL\n",checks,scenario);return 0;
}
