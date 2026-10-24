#include "dsr_maintenance_stage.h"
#include "dsr_maintenance_io.h"
#include "dsr_maintenance_store.h"
#include "dsr_maintenance.h"
#include "dsr_maintenance_private.h"
#include "dsr_activity_internal.h"
#include "dsr_backup_bonds_model.h"
#include "image.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,files,handles,random_serial,peak_fs,max_chunk,allocated;
static bool short_write,bond_supported=true,quiet=true,remove_fail,preempt_on_barrier;
static int fail_allocation=-1,remove_fail_at=-1;
static dsr_maint_limits_t limits={.total=0x3e0000,.used=65536,.nvs_available=1800,.psram_free=1024*1024,.psram_largest=1024*1024,.internal_free=65536,.internal_largest=32768};
static struct {char name[32];uint8_t *bytes;size_t size;} disk[17];
static struct {void *ptr;size_t size;} memory[16];
struct dsr_maint_file {unsigned at;size_t position;bool writing;};
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"stage:%d %s\n",__LINE__,#x);exit(1);}}while(0)
static bool test_boot_ready=true;
bool dsr_boot_ready(void){return test_boot_ready;}
bool dsr_workspace_maintenance_quiet(void){return true;}
bool dsr_backup_sources_quiet(void){return quiet;}
bool dsr_ble_maintenance_quiet(void){return true;}
bool dsr_ota_maintenance_quiet(void){return true;}
bool dsr_ble_backup_store_owner(void){return false;}
dsr_result_t dsr_ble_backup_capture(const dsr_maint_lease_t *l,cJSON *root)
{CHECK(dsr_maintenance_lease_valid(l));if(preempt_on_barrier){preempt_on_barrier=false;CHECK(dsr_activity_stop_begin());dsr_activity_end();}cJSON *o=cJSON_AddObjectToObject(root,"bonds");CHECK(o&&cJSON_AddNumberToObject(o,"version",1)&&cJSON_AddArrayToObject(o,"records"));return DSR_OK;}
dsr_result_t dsr_backup_device_id(uint8_t out[32]){memset(out,0,32);out[0]=1;return DSR_OK;}
dsr_result_t dsr_backup_bonds_preflight(const cJSON *root)
{return bond_supported?dsr_backup_bonds_validate(cJSON_GetObjectItemCaseSensitive(root,"bonds")):DSR_UNSUPPORTED;}
dsr_result_t dsr_maint_io_limits(dsr_maint_limits_t *out)
{*out=limits;for(unsigned i=0;i<17;i++)out->used+=disk[i].size;return DSR_OK;}
dsr_result_t dsr_maint_io_random(uint8_t *out,size_t n)
{CHECK(n<=64);for(size_t i=0;i<n;i++)out[i]=(uint8_t)++random_serial;return DSR_OK;}
dsr_result_t dsr_maint_io_next_revision(uint32_t imported,uint32_t *out){*out=imported+1000;return DSR_OK;}
void *dsr_maint_io_alloc(size_t n)
{
 if(fail_allocation==0){fail_allocation=-1;return NULL;}if(fail_allocation>0)fail_allocation--;
 unsigned i;for(i=0;i<16;i++)if(!memory[i].ptr)break;CHECK(i<16);void *p=calloc(1,n);CHECK(p);memory[i].ptr=p;memory[i].size=n;allocated++;return p;
}
void dsr_maint_io_free(void *p,size_t n)
{if(!p)return;unsigned i;for(i=0;i<16;i++)if(memory[i].ptr==p)break;CHECK(i<16&&memory[i].size==n&&allocated);memset(p,0,n);free(p);memory[i].ptr=NULL;memory[i].size=0;allocated--;}
static int find(const char *name)
{CHECK(strlen(name)==27&&name[0]=='m'&&!strchr(name,'/'));for(unsigned i=0;i<17;i++)if(disk[i].bytes&&!strcmp(name,disk[i].name))return (int)i;return -1;}
dsr_result_t dsr_maint_io_exists(const char *name,bool *exists){*exists=find(name)>=0;return DSR_OK;}
dsr_result_t dsr_maint_io_open_write(const char *name,dsr_maint_file_t **out)
{
 if(find(name)>=0)return DSR_CONFLICT;
 unsigned i;for(i=0;i<17;i++)if(!disk[i].bytes)break;CHECK(i<17);disk[i].bytes=malloc(115344);CHECK(disk[i].bytes);strcpy(disk[i].name,name);disk[i].size=0;files++;
 *out=calloc(1,sizeof(**out));CHECK(*out);(*out)->at=i;(*out)->writing=true;handles++;return DSR_OK;
}
dsr_result_t dsr_maint_io_open_read(const char *name,dsr_maint_file_t **out,size_t *size)
{int i=find(name);if(i<0)return DSR_IO;*out=calloc(1,sizeof(**out));CHECK(*out);(*out)->at=(unsigned)i;*size=disk[i].size;handles++;return DSR_OK;}
dsr_result_t dsr_maint_io_write(dsr_maint_file_t *f,const uint8_t *p,size_t n)
{
 CHECK(f&&f->writing&&n>0&&n<=4096);if(n>max_chunk)max_chunk=(unsigned)n;size_t copy=short_write?n/2:n;CHECK(f->position+copy<=115344);memcpy(disk[f->at].bytes+f->position,p,copy);f->position+=copy;disk[f->at].size=f->position;
 dsr_maint_limits_t l;dsr_maint_io_limits(&l);if(l.used>peak_fs)peak_fs=(unsigned)l.used;CHECK(l.used<=l.total*3/4);
 if(short_write){short_write=false;return DSR_IO;}return DSR_OK;
}
dsr_result_t dsr_maint_io_read(dsr_maint_file_t *f,uint8_t *p,size_t n,size_t *got)
{CHECK(f&&!f->writing&&n>0&&n<=4096);if(n>max_chunk)max_chunk=(unsigned)n;size_t left=disk[f->at].size-f->position;if(n>left)n=left;if(n>3071)n=3071;memcpy(p,disk[f->at].bytes+f->position,n);f->position+=n;*got=n;return DSR_OK;}
dsr_result_t dsr_maint_io_close(dsr_maint_file_t *f){CHECK(f&&handles);handles--;free(f);return DSR_OK;}
dsr_result_t dsr_maint_io_remove(const char *name)
{if(remove_fail||(remove_fail_at>=0&&remove_fail_at--==0)){remove_fail=false;remove_fail_at=-1;return DSR_IO;}int i=find(name);if(i>=0){free(disk[i].bytes);memset(disk+i,0,sizeof(disk[i]));CHECK(files);files--;}return DSR_OK;}
int main(int argc,char **argv)
{
 CHECK(argc==2||argc==3);dsr_backup_image_t *small=test_backup_image(argv[1],0,16),*full=NULL;CHECK(small);dsr_maint_stage_t *stage=NULL;dsr_maint_record_t record,loaded;bool found,pending;
 if(argc==3){
  CHECK(!strcmp(argv[2],"commit-uncertain"));CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);dsr_maint_lease_t lease;CHECK(dsr_maintenance_acquire(&lease));test_nvs_fail_after=1;
  CHECK(dsr_maint_stage_commit(stage,&lease)==DSR_IO);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&pending&&loaded.phase==DSR_MAINT_COMMIT);dsr_maintenance_release(&lease);
  CHECK(!dsr_activity_begin()&&!dsr_activity_stop_begin());CHECK(dsr_maint_stage_cancel(stage)==DSR_CONFLICT&&files==1);dsr_maint_stage_free(stage);CHECK(files==1&&!allocated&&!handles&&test_active_writes==0);
  dsr_backup_image_free(small);for(unsigned i=0;i<17;i++)free(disk[i].bytes);test_nvs_reset();printf("PASS: %u ambiguous commit checks; failed ACK preserves durable intent and closed admission, NVS/IO MODEL\n",checks);return 0;
 }
 full=test_backup_image(argv[1],16,16);CHECK(full);
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);dsr_maint_archive_info_t archive;CHECK(dsr_maint_stage_archive_info(stage,&archive)&&!archive.created&&!strcmp(archive.firmware,"0.7.70-dev"));CHECK(dsr_maint_stage_cancel(stage)==DSR_OK);dsr_maint_stage_free(stage);stage=NULL;
 unsigned writes=test_nvs_writes;limits.used+=16*115200;CHECK(dsr_maint_stage_prepare(full,&stage)==DSR_FULL&&!stage&&!files&&test_nvs_writes==writes);limits.used=65536;
 limits.nvs_available=1;CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_FULL&&!stage&&test_nvs_writes==writes);limits.nvs_available=1800;
 limits.psram_largest=98304;CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_FULL&&!stage&&test_nvs_writes==writes);limits.psram_largest=1024*1024;
 fail_allocation=2;CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_FULL&&!stage&&!allocated&&test_nvs_writes==writes);
 bond_supported=false;CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_UNSUPPORTED&&!stage&&!files&&test_nvs_writes==writes);bond_supported=true;
 quiet=false;CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_BUSY&&!stage);quiet=true;
 short_write=true;CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_IO&&!stage&&!files&&!handles&&!allocated);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&!pending);
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK&&stage&&files==1);CHECK(dsr_maint_stage_record(stage,&record));uint8_t first_key[32];memcpy(first_key,record.key,32);
 disk[0].bytes[128]^=1;dsr_maint_lease_t lease;CHECK(dsr_maintenance_acquire(&lease));CHECK(dsr_maint_stage_commit(stage,&lease)==DSR_BAD_RESPONSE);dsr_maintenance_release(&lease);CHECK(dsr_maint_stage_cancel(stage)==DSR_OK&&!files);dsr_maint_stage_free(stage);stage=NULL;
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);CHECK(dsr_maint_stage_record(stage,&record)&&memcmp(first_key,record.key,32));dsr_maintenance_mutated();CHECK(dsr_maintenance_acquire(&lease));CHECK(dsr_maint_stage_commit(stage,&lease)==DSR_CONFLICT);dsr_maintenance_release(&lease);CHECK(dsr_maint_stage_cancel(stage)==DSR_OK);dsr_maint_stage_free(stage);stage=NULL;
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);CHECK(dsr_maint_stage_record(stage,&record));dsr_maint_stage_free(stage);stage=NULL;
 CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&pending&&loaded.phase==DSR_MAINT_PREPARING);CHECK(dsr_maint_stage_cleanup_preparing(&loaded)==DSR_OK&&!files);
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);remove_fail=true;CHECK(dsr_maint_stage_cancel(stage)==DSR_IO&&files==1);dsr_maint_stage_free(stage);stage=NULL;
 CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&pending);CHECK(dsr_maint_stage_cleanup_preparing(&loaded)==DSR_OK&&!files);
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);CHECK(dsr_maintenance_acquire(&lease));preempt_on_barrier=true;CHECK(dsr_maint_stage_commit(stage,&lease)==DSR_CONFLICT);
 dsr_maintenance_release(&lease);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&pending&&loaded.phase==DSR_MAINT_PREPARING);CHECK(dsr_maint_stage_cancel(stage)==DSR_OK);dsr_maint_stage_free(stage);stage=NULL;
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);CHECK(dsr_maintenance_acquire(&lease));test_nvs_fail_after=0;CHECK(dsr_maint_stage_commit(stage,&lease)==DSR_IO&&dsr_maintenance_lease_valid(&lease));
 dsr_maintenance_release(&lease);CHECK(dsr_maint_stage_cancel(stage)==DSR_OK);dsr_maint_stage_free(stage);stage=NULL;
 CHECK(dsr_maint_stage_prepare(small,&stage)==DSR_OK);CHECK(dsr_maintenance_acquire(&lease));CHECK(dsr_maint_stage_barrier(stage,&lease)==DSR_OK);dsr_maintenance_release(&lease);
 dsr_maintenance_mutated();CHECK(dsr_maintenance_acquire(&lease));CHECK(dsr_maint_stage_commit_verified(stage,&lease)==DSR_CONFLICT);dsr_maintenance_release(&lease);CHECK(dsr_maint_stage_cancel(stage)==DSR_OK);dsr_maint_stage_free(stage);stage=NULL;
 CHECK(dsr_maint_stage_prepare(full,&stage)==DSR_OK&&stage&&files==17);CHECK(dsr_maint_stage_record(stage,&record)&&record.generation==dsr_maintenance_generation()&&record.photos==16&&record.bonds==16);
 CHECK(max_chunk<=4096&&peak_fs<=limits.total*3/4&&test_active_writes==0&&!handles);CHECK(dsr_maintenance_acquire(&lease));CHECK(dsr_maint_stage_commit(stage,&lease)==DSR_OK);
 CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&pending&&loaded.phase==DSR_MAINT_COMMIT);CHECK(!dsr_activity_stop_begin()&&!dsr_activity_begin());dsr_maintenance_release(&lease);CHECK(!dsr_activity_begin());
 CHECK(dsr_maint_stage_cancel(stage)==DSR_CONFLICT&&files==17);dsr_maint_stage_free(stage);CHECK(files==17&&!allocated&&!handles&&test_active_writes==0);
 dsr_backup_image_free(small);dsr_backup_image_free(full);for(unsigned i=0;i<17;i++)free(disk[i].bytes);test_nvs_reset();
 printf("PASS: %u stage checks; actual codecs/crypto/journal/admission, modeled IO/owners, no OTA or active-data writes\n",checks);return 0;
}
