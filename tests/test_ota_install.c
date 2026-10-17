/* Reuse the independent real Ed25519/SHA/HTTP fixture, not fake verified bytes.
 * Only SDK partition/Flash/boot calls below are MODEL memory providers. */
#define main dsr_download_fixture_unused_main
#include "test_ota_download.c"
#undef main
#include "dsr_ota_install.h"
#include "dsr_ota_stage.h"
#include "dsr_activity_internal.h"
#include "dsr_ota_fetch_private.h"
#include "esp_ota_ops.h"

static esp_partition_t parts[3];
static int running_index,next_index;
static bool boot_ready=true,sdk_live,sdk_fail_begin,sdk_fail_end,sdk_fail_boot,sdk_fail_abort;
static unsigned sdk_begins,sdk_writes,sdk_ends,sdk_aborts,sdk_boots,sdk_fail_write;
static size_t sdk_written;
static unsigned char *sdk_bytes;
static int cancel_phase=-1;
static uint32_t last_progress;
static uint32_t expected_total;
static unsigned phase_count[5];
bool dsr_boot_ready(void){return boot_ready;}
const esp_partition_t *esp_partition_find_first(unsigned type,unsigned subtype,const char *label)
{
 if(type==ESP_PARTITION_TYPE_APP&&subtype==ESP_PARTITION_SUBTYPE_APP_OTA_0&&!strcmp(label,"ota_0"))return &parts[0];
 if(type==ESP_PARTITION_TYPE_APP&&subtype==ESP_PARTITION_SUBTYPE_APP_OTA_1&&!strcmp(label,"ota_1"))return &parts[1];
 CHECK(type==ESP_PARTITION_TYPE_DATA&&subtype==ESP_PARTITION_SUBTYPE_DATA_SPIFFS&&!strcmp(label,"dsrdata"));return &parts[2];
}
const esp_partition_t *esp_ota_get_running_partition(void){return running_index<0?NULL:&parts[running_index];}
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *from){CHECK(!from);return next_index<0?NULL:&parts[next_index];}
static void exclusive(void)
{CHECK(dsr_ota_stage_status()==DSR_OTA_STAGE_HELD&&!dsr_ota_stage_acquire());CHECK(!dsr_activity_restart_reserve());}
esp_err_t esp_ota_begin(const esp_partition_t *slot,size_t size,esp_ota_handle_t *handle)
{
 exclusive();CHECK(slot==&parts[next_index]&&next_index!=running_index&&size==OTA_WITH_SEQUENTIAL_WRITES&&!sdk_live&&!sdk_begins);
 CHECK(hops[0].closed&&hops[0].cleaned&&hops[initialized-1].offset>=512);sdk_begins++;*handle=37;
 if(sdk_fail_begin)return ESP_FAIL;
 sdk_live=true;return ESP_OK;
}
esp_err_t esp_ota_write(esp_ota_handle_t handle,const void *bytes,size_t size)
{
 exclusive();CHECK(sdk_live&&handle==37&&size&&sdk_written+size<=image_length);sdk_writes++;
 if(sdk_writes==sdk_fail_write)return ESP_FAIL;
 memcpy(sdk_bytes+sdk_written,bytes,size);sdk_written+=size;return ESP_OK;
}
esp_err_t esp_ota_end(esp_ota_handle_t handle)
{
 exclusive();all_closed();CHECK(handle==37&&sdk_live&&sdk_written==image_length&&!memcmp(sdk_bytes,image,image_length)&&!sdk_ends&&!sdk_aborts);
 sdk_ends++;sdk_live=false;return sdk_fail_end?ESP_FAIL:ESP_OK;
}
esp_err_t esp_ota_abort(esp_ota_handle_t handle)
{exclusive();CHECK(handle==37&&sdk_live&&!sdk_ends&&!sdk_aborts);sdk_aborts++;sdk_live=false;return sdk_fail_abort?ESP_FAIL:ESP_OK;}
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *slot)
{exclusive();all_closed();CHECK(slot==&parts[next_index]&&sdk_ends==1&&!sdk_live&&!sdk_aborts&&!sdk_boots);sdk_boots++;return sdk_fail_boot?ESP_FAIL:ESP_OK;}
static void report_progress(void *ctx,dsr_ota_install_phase_t phase,uint32_t received,uint32_t total)
{
 CHECK(ctx==&cancel_flag&&phase>=DSR_INSTALL_MANIFEST&&phase<=DSR_INSTALL_STAGED&&received<=total&&total==expected_total);
 CHECK(received>=last_progress);last_progress=received;phase_count[phase]++;
 if(phase==DSR_INSTALL_STAGED){CHECK(dsr_ota_stage_status()==DSR_OTA_STAGE_STAGED&&sdk_boots==1);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
 else exclusive();
 if(phase==DSR_INSTALL_MANIFEST)CHECK(!app_requests&&!sdk_begins);
 if(phase==DSR_INSTALL_VERIFYING)CHECK(sdk_live&&sdk_written==image_length&&!memcmp(sdk_bytes,image,image_length));
 if(phase==DSR_INSTALL_SELECTING_BOOT)CHECK(!sdk_live&&sdk_ends==1&&!sdk_boots);
 if((int)phase==cancel_phase)cancel_flag=true;
}
static void fixture(void)
{
 CHECK(dsr_ota_stage_status()==DSR_OTA_STAGE_IDLE);reset();
 parts[0]=(esp_partition_t){.type=ESP_PARTITION_TYPE_APP,.subtype=ESP_PARTITION_SUBTYPE_APP_OTA_0,.address=0x20000,.size=0x600000,.label="ota_0"};
 parts[1]=(esp_partition_t){.type=ESP_PARTITION_TYPE_APP,.subtype=ESP_PARTITION_SUBTYPE_APP_OTA_1,.address=0x620000,.size=0x600000,.label="ota_1"};
 parts[2]=(esp_partition_t){.type=ESP_PARTITION_TYPE_DATA,.subtype=ESP_PARTITION_SUBTYPE_DATA_SPIFFS,.address=0xc20000,.size=0x3e0000,.label="dsrdata"};
 running_index=0;next_index=1;boot_ready=true;sdk_live=sdk_fail_begin=sdk_fail_end=sdk_fail_boot=sdk_fail_abort=false;
 sdk_begins=sdk_writes=sdk_ends=sdk_aborts=sdk_boots=sdk_fail_write=0;sdk_written=last_progress=0;cancel_phase=-1;
 memset(sdk_bytes,0,image_length);memset(phase_count,0,sizeof(phase_count));
}
static dsr_ota_verified_t consent(void)
{
 dsr_ota_verified_t v={.size=(uint32_t)image_length,.channel=DSR_OTA_STABLE};strcpy(v.version,"1.2.4");strcpy(v.url,app_url);
 unsigned n=0;CHECK(EVP_Digest(image,image_length,v.sha256,&n,EVP_sha256(),NULL)==1&&n==32);return v;
}
static dsr_ota_policy_t policy={"1.2.3",DSR_OTA_STABLE,0x600000};
static dsr_ota_install_control_t control={cancelled,report_progress,&cancel_flag};
static dsr_ota_install_report_t details;
static dsr_ota_install_result_t invoke(const dsr_ota_verified_t *expected,bool power)
{expected_total=expected?expected->size:0;return dsr_ota_install(initial,&policy,expected,&control,power,&details);}
static void clean_install(void)
{
 CHECK(!owned&&!dsr_test_sha_live()&&!sdk_live&&dsr_ota_stage_status()==DSR_OTA_STAGE_IDLE);
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 for(unsigned i=0;i<initialized;i++)if(hops[i].config.url)CHECK(hops[i].closed&&hops[i].cleaned);
 CHECK(!sdk_boots||details.boot_selection_attempted);
}
static void preflight(void)
{
 dsr_ota_verified_t v=consent();
 size_t inactive_size=99;fixture();CHECK(dsr_ota_install_ready(&inactive_size)==DSR_INSTALL_OK&&inactive_size==0x600000&&!initialized);CHECK(dsr_ota_install_ready(NULL)==DSR_INSTALL_ARGUMENT);clean_install();
 fixture();CHECK(invoke(&v,false)==DSR_INSTALL_POWER&&!initialized&&!sdk_begins);clean_install();
 fixture();boot_ready=false;CHECK(dsr_ota_install_ready(&inactive_size)==DSR_INSTALL_BOOT&&inactive_size==0);CHECK(invoke(&v,true)==DSR_INSTALL_BOOT&&!initialized);clean_install();
 for(unsigned i=0;i<11;i++){
  fixture();switch(i){case 0:parts[0].address++;break;case 1:parts[0].size--;break;case 2:parts[1].address++;break;case 3:parts[1].size--;break;case 4:parts[2].address++;break;case 5:parts[2].size--;break;case 6:parts[1].label[0]='X';break;case 7:running_index=-1;break;case 8:running_index=2;break;case 9:next_index=0;break;case 10:next_index=-1;break;}
  CHECK(dsr_ota_install_ready(&inactive_size)==DSR_INSTALL_SLOT&&inactive_size==0);CHECK(invoke(&v,true)==DSR_INSTALL_SLOT&&!initialized&&!sdk_begins);clean_install();
 }
 fixture();policy.inactive_slot_size--;CHECK(invoke(&v,true)==DSR_INSTALL_SLOT&&!initialized);policy.inactive_slot_size++;clean_install();
 fixture();dsr_ota_stage_t *other=dsr_ota_stage_acquire();CHECK(other);CHECK(dsr_ota_install_ready(&inactive_size)==DSR_INSTALL_BUSY&&inactive_size==0);CHECK(invoke(&v,true)==DSR_INSTALL_BUSY&&!initialized);CHECK(!dsr_activity_restart_reserve());dsr_ota_stage_release(other);clean_install();
 fixture();CHECK(dsr_activity_restart_reserve());CHECK(invoke(&v,true)==DSR_INSTALL_BUSY&&!initialized);dsr_activity_restart_cancel();clean_install();
 fixture();cancel_flag=true;CHECK(invoke(&v,true)==DSR_INSTALL_CANCELLED&&!initialized);clean_install();
 fixture();CHECK(dsr_ota_install(NULL,&policy,&v,&control,true,&details)==DSR_INSTALL_ARGUMENT&&!initialized);CHECK(invoke(NULL,true)==DSR_INSTALL_ARGUMENT);clean_install();
 fixture();v.size=511;CHECK(invoke(&v,true)==DSR_INSTALL_ARGUMENT&&!initialized);clean_install();
}
static void failures(void)
{
 dsr_ota_verified_t v=consent();
 fixture();cancel_phase=DSR_INSTALL_MANIFEST;CHECK(invoke(&v,true)==DSR_INSTALL_CANCELLED&&!initialized);clean_install();
 fixture();wall=0;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&details.fetch_result==DSR_FETCH_CLOCK&&!initialized&&!sdk_begins);clean_install();
#ifndef DSR_DOWNLOAD_HOST_TEST_TRUST
 fixture();CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&details.manifest_result==DSR_MANIFEST_UNTRUSTED_KEY&&!app_requests&&!sdk_begins&&!sdk_writes&&!sdk_ends&&!sdk_boots);clean_install();
#else
 for(unsigned i=0;i<5;i++){
  fixture();dsr_ota_verified_t changed_consent=v;switch(i){case 0:changed_consent.version[4]='5';break;case 1:changed_consent.url[sizeof(app_url)-6]='X';break;case 2:changed_consent.size--;break;case 3:changed_consent.sha256[0]^=1;break;case 4:changed_consent.channel=DSR_OTA_DEV;break;}
  CHECK(invoke(&changed_consent,true)==DSR_INSTALL_CONSENT_CHANGED&&details.manifest_result==DSR_MANIFEST_OK&&!app_requests&&!sdk_begins);clean_install();
 }
 for(unsigned i=1;i<5;i++){fixture();content(0,true,manifests[i],manifest_lengths[i]);CHECK(invoke(&v,true)!=DSR_INSTALL_OK&&!app_requests&&!sdk_begins);clean_install();}
 fixture();cancel_phase=DSR_INSTALL_SELECTING_BOOT;CHECK(invoke(&v,true)==DSR_INSTALL_CANCELLED&&sdk_ends==1&&!sdk_aborts&&!sdk_boots);unsigned allocation_count=allocations;clean_install();
 for(unsigned i=1;i<=allocation_count;i++){fixture();fail_allocation=i;CHECK(invoke(&v,true)!=DSR_INSTALL_OK&&!sdk_boots);clean_install();}
 /* Every possible first-header split with the actual genuine stream/helper.
  * Cancel after full validation permits another isolated staging transaction. */
 for(unsigned split=1;split<=512;split++){fixture();app_chunk=split;cancel_phase=DSR_INSTALL_VERIFYING;CHECK(invoke(&v,true)==DSR_INSTALL_CANCELLED&&sdk_begins==1&&sdk_aborts==1&&!sdk_ends&&!sdk_boots&&sdk_written==image_length&&!memcmp(sdk_bytes,image,image_length));clean_install();}
 fixture();sdk_fail_begin=true;CHECK(invoke(&v,true)==DSR_INSTALL_SDK&&sdk_begins==1&&!sdk_writes&&!sdk_aborts&&!sdk_ends);clean_install();
 for(unsigned i=1;i<=3;i++){fixture();sdk_fail_write=i;CHECK(invoke(&v,true)==DSR_INSTALL_SDK&&sdk_aborts==1&&!sdk_ends&&!sdk_boots);clean_install();}
 fixture();sdk_fail_end=true;CHECK(invoke(&v,true)==DSR_INSTALL_SDK&&sdk_ends==1&&!sdk_aborts&&!sdk_boots);clean_install();
 fixture();cancel_phase=DSR_INSTALL_DOWNLOADING;sdk_fail_abort=true;CHECK(invoke(&v,true)==DSR_INSTALL_SDK&&sdk_aborts==1&&!sdk_ends&&!sdk_boots);clean_install();
 fixture();memcpy(changed,image,image_length);changed[0]=0;hops[1].body=changed;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&!sdk_begins&&!sdk_writes&&!sdk_aborts);clean_install();
 fixture();memcpy(changed,image,image_length);changed[900]^=1;hops[1].body=changed;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&details.image_result==DSR_IMAGE_HASH&&sdk_aborts==1&&!sdk_ends&&!sdk_boots);clean_install();
 fixture();hops[1].length--;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&sdk_aborts==1&&!sdk_ends&&!sdk_boots);clean_install();
 fixture();hops[1].complete=false;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&sdk_aborts==1&&!sdk_ends);clean_install();
 fixture();fail_read=(unsigned)manifest_lengths[0]+3;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&sdk_aborts==1&&!sdk_ends&&!sdk_boots);clean_install();
 fixture();fail_open=2;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&!sdk_begins&&!sdk_aborts);clean_install();
 fixture();hops[1].status=500;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&!sdk_begins&&!sdk_aborts);clean_install();
 fixture();hops[1].chunked=true;hops[1].headers[0]=(header_t){"Transfer-Encoding","chunked"};hops[1].length++;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&sdk_aborts==1&&!sdk_ends);clean_install();
 fixture();fail_close=2;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&sdk_aborts==1&&!sdk_ends);clean_install();
 fixture();fail_cleanup=2;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&sdk_aborts==1&&!sdk_ends);clean_install();
 fixture();post_finish=1;CHECK(invoke(&v,true)==DSR_INSTALL_CANCELLED&&details.image_result==DSR_IMAGE_OK&&sdk_aborts==1&&!sdk_ends);clean_install();
 fixture();post_finish=2;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&details.fetch_result==DSR_FETCH_TIMEOUT&&sdk_aborts==1&&!sdk_ends);clean_install();
 fixture();cancel_app_read=2;CHECK(invoke(&v,true)==DSR_INSTALL_CANCELLED&&!sdk_ends&&!sdk_boots);clean_install();
 fixture();app_delay=31000000;CHECK(invoke(&v,true)==DSR_INSTALL_FETCH&&!sdk_ends&&!sdk_boots);clean_install();
#endif
}
int main(int argc,char **argv)
{
 CHECK(argc==3);FILE *f=fopen(argv[1],"rb");CHECK(f);image_length=integer(f);CHECK(image_length>=512&&image_length<=0x600000);
 image=__real_malloc(image_length+1);changed=__real_malloc(image_length+1);sdk_bytes=__real_malloc(image_length);CHECK(image&&changed&&sdk_bytes);CHECK(fread(image,1,image_length,f)==image_length);image[image_length]=0;
 for(unsigned i=0;i<5;i++){manifest_lengths[i]=integer(f);CHECK(manifest_lengths[i]<=2048&&fread(manifests[i],1,manifest_lengths[i],f)==manifest_lengths[i]);}CHECK(fgetc(f)==EOF);fclose(f);cJSON_Hooks hooks={__wrap_malloc,__wrap_free};cJSON_InitHooks(&hooks);
 unsigned mode=(unsigned)strtoul(argv[2],NULL,10);
 if(!mode){preflight();failures();}
#ifdef DSR_DOWNLOAD_HOST_TEST_TRUST
 else{
  fixture();if(mode==1)app_chunk=1;else if(mode==2)app_chunk=511;else if(mode==3)app_chunk=512;
  else if(mode==4){running_index=1;next_index=0;}else if(mode==5){hops[1].chunked=true;hops[1].headers[0]=(header_t){"Transfer-Encoding","chunked"};}
  else if(mode==6){redirect(1,false,app_cdn);content(2,false,image,image_length);}
  bool uncertain=mode==7;sdk_fail_boot=uncertain;
  dsr_ota_verified_t v=consent();CHECK(invoke(&v,true)==(uncertain?DSR_INSTALL_SDK:DSR_INSTALL_OK)&&details.boot_selected==!uncertain&&details.boot_selection_attempted&&sdk_begins==1&&sdk_ends==1&&sdk_boots==1&&!sdk_aborts&&!sdk_live);
  CHECK(sdk_written==image_length&&!memcmp(sdk_bytes,image,image_length)&&!owned&&!dsr_test_sha_live());CHECK(dsr_ota_stage_status()==(uncertain?DSR_OTA_STAGE_UNCERTAIN:DSR_OTA_STAGE_STAGED)&&!dsr_ota_stage_acquire());CHECK(phase_count[DSR_INSTALL_STAGED]==(uncertain?0u:1u));
  size_t inactive_size=99;CHECK(dsr_ota_install_ready(&inactive_size)==DSR_INSTALL_BUSY&&inactive_size==0);
  CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();unsigned requests=initialized;CHECK(invoke(&v,true)==DSR_INSTALL_BUSY&&initialized==requests&&sdk_boots==1);
 }
#endif
 __real_free(image);__real_free(changed);__real_free(sdk_bytes);
 printf("PASS: %u actual native installer checks; mode%u, real manifest/signature/SHA/stream, shared stage/lease/latch; partition/Flash/boot MODEL, no physical install proof\n",checks,mode);return 0;
}
