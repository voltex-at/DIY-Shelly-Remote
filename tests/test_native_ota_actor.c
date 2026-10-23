/* Actual actor only. Chain/installer/clock/worker admission are explicit MODEL
 * providers; genuine signature/image/Flash ordering is tested separately. */
#include "dsr_native_ota.h"
#include "dsr_ota_stage.h"
#include "esp_app_desc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
struct test_semaphore {bool held;};
static struct test_semaphore mutex;
static bool no_memory,boot=true,connected=true,temporary;
static int64_t now=1000000;
static esp_app_desc_t description={.version="0.7.26-native-ota-dev"};
static dsr_ota_stage_state_t stage=DSR_OTA_STAGE_IDLE;
static dsr_ota_install_result_t readiness=DSR_INSTALL_OK,install_result=DSR_INSTALL_OK;
static dsr_ota_fetch_result_t clock_result=DSR_FETCH_OK,catalogue_result=DSR_FETCH_OK,manifest_result=DSR_FETCH_OK;
static dsr_manifest_result_t manifest_why=DSR_MANIFEST_OK;
static dsr_ota_release_result_t catalogue_why=DSR_RELEASE_OK;
static unsigned clock_calls,catalogue_calls,manifest_calls,install_calls;
static unsigned battery_reads;
static bool cancel_battery;
static esp_err_t battery_error,profile_error;
static dsr_battery_sample_t battery_sample;
static dsr_battery_profile_t battery_profile;
esp_err_t dsr_battery_read(dsr_battery_sample_t *out)
{CHECK(!mutex.held);battery_reads++;*out=battery_sample;if(cancel_battery)dsr_native_ota_cancel();return battery_error;}
esp_err_t dsr_battery_profile_load(dsr_battery_profile_t *out)
{CHECK(!mutex.held);*out=battery_profile;return profile_error;}
static unsigned cancel_at; /* 1 clock, 2 catalogue, 3 manifest, 4 install boot */
static bool boot_attempted,probe_running;
static int catalogue_status=200;
static int installed_http,installed_io;static esp_err_t installed_sdk;
static uint32_t next_id=100,current_id;
static dsr_job_kind_t current_kind;
static dsr_ota_channel_t expected_channel=DSR_OTA_STABLE;
static const char url[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/DIY-Shelly-Remote-es3c35p-manifest.json";
static const char app_url[]="https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/v1.2.4/app.bin";

SemaphoreHandle_t xSemaphoreCreateMutex(void){return no_memory?NULL:&mutex;}
int xSemaphoreTake(SemaphoreHandle_t handle,TickType_t wait)
{CHECK(handle==&mutex&&wait==portMAX_DELAY&&!mutex.held);mutex.held=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t handle)
{CHECK(handle==&mutex&&mutex.held);mutex.held=false;return pdTRUE;}
const esp_app_desc_t *esp_app_get_description(void){return &description;}
int64_t esp_timer_get_time(void){return now;}
bool dsr_boot_ready(void){return boot;}
bool dsr_wifi_connected(void){return connected;}
bool dsr_wifi_temporary_active(void){return temporary;}
dsr_ota_stage_state_t dsr_ota_stage_status(void){return stage;}
dsr_ota_install_result_t dsr_ota_install_ready(size_t *size)
{CHECK(size&&!mutex.held);*size=readiness==DSR_INSTALL_OK?0x600000:0;return readiness;}
static dsr_native_ota_snapshot_t snapshot(void)
{dsr_native_ota_snapshot_t view;memset(&view,0xa5,sizeof(view));dsr_native_ota_snapshot(&view);return view;}
static void provider(void)
{
 CHECK(!mutex.held);
 if(!probe_running)return;
 dsr_native_ota_snapshot_t view=snapshot();
 CHECK(view.state==(current_kind==DSR_JOB_OTA_CHECK?DSR_NATIVE_CHECKING:DSR_NATIVE_INSTALLING));
 CHECK(dsr_native_ota_prepare(current_id+1,DSR_JOB_OTA_CHECK,DSR_OTA_DEV,false)==DSR_BUSY);
 CHECK(!dsr_native_ota_admitted(current_id,current_kind));
 dsr_native_ota_reject(current_id);CHECK(snapshot().state==view.state);
 dsr_job_t duplicate={.id=current_id,.kind=current_kind};dsr_event_t event={0};
 dsr_native_ota_execute(&duplicate,&event);CHECK(event.result==DSR_INVALID);
}
static void policy(const dsr_ota_policy_t *p)
{CHECK(p&&!strcmp(p->current_version,"0.7.26")&&p->selected_channel==expected_channel&&p->inactive_slot_size==0x600000);}
dsr_ota_fetch_result_t dsr_native_ota_clock(const dsr_ota_fetch_control_t *control)
{provider();CHECK(control&&control->cancelled);clock_calls++;if(cancel_at==1)dsr_native_ota_cancel();return clock_result;}
dsr_ota_fetch_result_t dsr_ota_discovery_fetch(dsr_ota_channel_t channel,const dsr_ota_fetch_control_t *control,dsr_ota_discovery_t *out,dsr_ota_release_result_t *why,int *http)
{
 provider();CHECK(channel==expected_channel&&control&&out&&why&&http);catalogue_calls++;
 memset(out,0,sizeof(*out));strcpy(out->candidate.tag,"v1.2.4");strcpy(out->candidate.manifest_url,url);out->candidate.manifest_size=700;strcpy(out->notes,"Untrusted <b>plain</b> notes");
 *why=catalogue_why;*http=catalogue_status;if(cancel_at==2)dsr_native_ota_cancel();return catalogue_result;
}
dsr_ota_fetch_result_t dsr_ota_fetch_manifest(const char *source,const dsr_ota_policy_t *p,const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,dsr_manifest_result_t *why)
{
 provider();CHECK(source&&!strcmp(source,url)&&control&&out&&why);policy(p);manifest_calls++;
 memset(out,0,sizeof(*out));strcpy(out->version,"1.2.4");strcpy(out->url,app_url);memset(out->sha256,0xa5,sizeof(out->sha256));out->size=8192;out->channel=expected_channel;
 *why=manifest_why;if(cancel_at==3)dsr_native_ota_cancel();return manifest_result;
}
dsr_ota_install_result_t dsr_ota_install(const char *source,const dsr_ota_policy_t *p,const dsr_ota_verified_t *consent,const dsr_ota_install_control_t *control,bool power,dsr_ota_install_report_t *report)
{
 provider();CHECK(source&&!strcmp(source,url)&&consent&&control&&control->cancelled&&control->progress&&power&&report);policy(p);install_calls++;
 CHECK(!strcmp(consent->version,"1.2.4")&&!strcmp(consent->url,app_url)&&consent->size==8192&&consent->channel==expected_channel);
 for(unsigned i=0;i<32;i++)CHECK(consent->sha256[i]==0xa5);
 *report=(dsr_ota_install_report_t){.fetch_result=DSR_FETCH_OK,.manifest_result=DSR_MANIFEST_OK,.image_result=DSR_IMAGE_OK,.bytes_written=8192,.boot_selection_attempted=boot_attempted,.boot_selected=install_result==DSR_INSTALL_OK};
 report->http_status=installed_http;report->io_result=installed_io;report->sdk_error=installed_sdk;
 control->progress(control->context,DSR_INSTALL_DOWNLOADING,4096,8192);CHECK(snapshot().received==4096);
 if(cancel_at==4)dsr_native_ota_cancel();
 if(install_result==DSR_INSTALL_OK)stage=DSR_OTA_STAGE_STAGED;else if(boot_attempted)stage=DSR_OTA_STAGE_UNCERTAIN;
 return install_result;
}
static void defaults(void)
{
 no_memory=false;boot=connected=true;temporary=false;readiness=install_result=DSR_INSTALL_OK;
 clock_result=catalogue_result=manifest_result=DSR_FETCH_OK;manifest_why=DSR_MANIFEST_OK;catalogue_why=DSR_RELEASE_OK;
 cancel_at=0;boot_attempted=false;probe_running=true;catalogue_status=200;
 installed_http=installed_io=installed_sdk=0;
 clock_calls=catalogue_calls=manifest_calls=install_calls=0;
 battery_reads=0;cancel_battery=false;battery_error=profile_error=ESP_OK;
 dsr_battery_profile_defaults(&battery_profile);battery_profile.kind=DSR_BATTERY_LIPO_1S;
 battery_sample=(dsr_battery_sample_t){.has_voltage=true,.adc_mv=2000,.bat_mv=4000};
}
static dsr_result_t prepare(dsr_job_kind_t kind,dsr_ota_channel_t channel,bool power)
{current_id=++next_id;current_kind=kind;expected_channel=channel;return dsr_native_ota_prepare(current_id,kind,channel,power);}
static dsr_event_t execute(void)
{dsr_job_t job={.id=current_id,.kind=current_kind};dsr_event_t event={.id=current_id};dsr_native_ota_execute(&job,&event);return event;}
static void available(dsr_ota_channel_t channel)
{
 defaults();CHECK(prepare(DSR_JOB_OTA_CHECK,channel,false)==DSR_OK);
 CHECK(dsr_native_ota_admitted(current_id,current_kind));CHECK(execute().result==DSR_OK);
 dsr_native_ota_snapshot_t view=snapshot();CHECK(view.state==DSR_NATIVE_AVAILABLE&&view.operation_id==current_id&&view.channel==channel&&view.total==8192&&!strcmp(view.version,"1.2.4")&&!strcmp(view.notes,"Untrusted <b>plain</b> notes"));
 CHECK(clock_calls==1&&catalogue_calls==1&&manifest_calls==1&&!install_calls);
}
static void lifecycle(void)
{
 defaults();dsr_native_ota_snapshot_t view=snapshot();CHECK(view.state==DSR_NATIVE_IDLE&&!strcmp(view.installed,description.version));
 CHECK(dsr_native_ota_prepare(0,DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_INVALID);
 CHECK(dsr_native_ota_prepare(1,DSR_JOB_PROBE,DSR_OTA_STABLE,false)==DSR_INVALID);
 CHECK(dsr_native_ota_prepare(1,DSR_JOB_OTA_CHECK,(dsr_ota_channel_t)7,false)==DSR_INVALID);
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_STABLE,true)==DSR_INVALID&&!install_calls);
 CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);
 CHECK(!dsr_native_ota_admitted(current_id+1,current_kind)&&!dsr_native_ota_admitted(current_id,DSR_JOB_OTA_INSTALL));
 dsr_native_ota_reject(current_id+1);CHECK(dsr_native_ota_admitted(current_id,current_kind));
 dsr_job_t stale={.id=current_id+1,.kind=current_kind};dsr_event_t event={0};dsr_native_ota_execute(&stale,&event);CHECK(event.result==DSR_INVALID&&!clock_calls);
 dsr_native_ota_reject(current_id);CHECK(snapshot().state==DSR_NATIVE_FAILED&&snapshot().error==DSR_NATIVE_ERROR_BUSY);
 CHECK(!dsr_native_ota_admitted(current_id,current_kind));CHECK(execute().result==DSR_INVALID&&!clock_calls);
 defaults();CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);dsr_native_ota_cancel();CHECK(execute().result!=DSR_OK&&snapshot().state==DSR_NATIVE_CANCELLED&&!clock_calls&&!catalogue_calls&&!manifest_calls&&!install_calls);
 for(unsigned at=1;at<=3;at++){defaults();cancel_at=at;CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);CHECK(execute().result!=DSR_OK&&snapshot().state==DSR_NATIVE_CANCELLED&&!install_calls);}
 available(DSR_OTA_STABLE);uint32_t checked=current_id;
 CHECK(dsr_native_ota_prepare(++next_id,DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_INVALID);
 CHECK(dsr_native_ota_prepare(++next_id,DSR_JOB_OTA_INSTALL,DSR_OTA_STABLE,false)==DSR_INVALID&&snapshot().operation_id==checked&&!install_calls);
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_STABLE,true)==DSR_OK);dsr_native_ota_cancel();
 CHECK(execute().result!=DSR_OK&&snapshot().state==DSR_NATIVE_CANCELLED&&!install_calls);
 available(DSR_OTA_STABLE);
 dsr_native_ota_cancel();CHECK(snapshot().state==DSR_NATIVE_CANCELLED&&snapshot().total==0&&!snapshot().version[0]);
 available(DSR_OTA_DEV);now+=119999999;CHECK(snapshot().state==DSR_NATIVE_AVAILABLE);now++;
 CHECK(snapshot().state==DSR_NATIVE_FAILED&&snapshot().error==DSR_NATIVE_ERROR_EXPIRED&&snapshot().total==0&&!snapshot().version[0]);
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_INVALID&&!install_calls);
 available(DSR_OTA_DEV);now+=120000000;
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_INVALID&&snapshot().error==DSR_NATIVE_ERROR_EXPIRED&&!install_calls);
 available(DSR_OTA_STABLE);now--;CHECK(snapshot().state==DSR_NATIVE_FAILED&&snapshot().error==DSR_NATIVE_ERROR_EXPIRED);now++;
 for(unsigned gate=0;gate<4;gate++){
  defaults();CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);
  if(gate==0)boot=false;else if(gate==1)connected=false;else if(gate==2)temporary=true;else readiness=DSR_INSTALL_BUSY;
  CHECK(execute().result!=DSR_OK&&snapshot().state==DSR_NATIVE_FAILED&&!clock_calls&&!catalogue_calls&&!manifest_calls&&!install_calls);
 }
 defaults();clock_result=DSR_FETCH_CLOCK;CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);CHECK(execute().result!=DSR_OK&&snapshot().error==DSR_NATIVE_ERROR_CLOCK&&!catalogue_calls);
 defaults();catalogue_result=DSR_FETCH_STATUS;catalogue_status=404;CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);CHECK(execute().result!=DSR_OK&&snapshot().error==DSR_NATIVE_ERROR_NO_RELEASE&&!manifest_calls);
 defaults();catalogue_result=DSR_FETCH_RELEASE;catalogue_why=DSR_RELEASE_NOT_FOUND;CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_DEV,false)==DSR_OK);CHECK(execute().result!=DSR_OK&&snapshot().error==DSR_NATIVE_ERROR_NO_RELEASE&&!manifest_calls);
 defaults();manifest_result=DSR_FETCH_MANIFEST;manifest_why=DSR_MANIFEST_UNTRUSTED_KEY;CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);CHECK(execute().result!=DSR_OK&&snapshot().error==DSR_NATIVE_ERROR_MANIFEST&&snapshot().manifest_result==DSR_MANIFEST_UNTRUSTED_KEY&&!install_calls);
 for(unsigned failure=0;failure<2;failure++){
  available(DSR_OTA_STABLE);install_result=failure?DSR_INSTALL_CONSENT_CHANGED:DSR_INSTALL_CANCELLED;
  if(!failure)cancel_at=4;
  CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_STABLE,true)==DSR_OK);CHECK(execute().result!=DSR_OK&&install_calls==1);
  CHECK(snapshot().state==(failure?DSR_NATIVE_FAILED:DSR_NATIVE_CANCELLED));
  CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_STABLE,true)==DSR_INVALID&&install_calls==1);
 }
 available(DSR_OTA_DEV);install_result=DSR_INSTALL_FETCH;installed_http=200;installed_io=-0x7007;
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_OK);CHECK(execute().result==DSR_IO);
 view=snapshot();CHECK(view.state==DSR_NATIVE_FAILED&&view.install_result==DSR_INSTALL_FETCH&&view.http_status==200&&view.io_result==-0x7007&&view.sdk_error==0);
 CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_DEV,false)==DSR_OK&&snapshot().io_result==0&&snapshot().sdk_error==0&&snapshot().http_status==0);CHECK(execute().result==DSR_OK);
 install_result=DSR_INSTALL_SDK;installed_sdk=ESP_ERR_NO_MEM;
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_OK);CHECK(execute().result==DSR_IO);CHECK(snapshot().sdk_error==ESP_ERR_NO_MEM&&snapshot().install_result==DSR_INSTALL_SDK);
 available(DSR_OTA_STABLE);stage=DSR_OTA_STAGE_HELD;CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_STABLE,true)==DSR_BUSY&&!install_calls);stage=DSR_OTA_STAGE_UNCERTAIN;CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_BUSY&&!install_calls);stage=DSR_OTA_STAGE_IDLE;dsr_native_ota_cancel();
}
static void battery_gate(void)
{
 /* Real percentage estimator; model ADC/NVS. 3490 mV is 19%, 3500 is 20%. */
 for(unsigned scenario=0;scenario<8;scenario++){
  available(DSR_OTA_DEV);CHECK(!battery_reads);install_result=DSR_INSTALL_FETCH;
  battery_sample.adc_mv=scenario==0?1745:scenario==1?1750:1650;
  if(scenario==3)battery_error=ESP_FAIL;
  if(scenario==4)profile_error=ESP_FAIL;
  if(scenario==5)battery_profile.kind=DSR_BATTERY_NONE;
  if(scenario==6){battery_sample.has_voltage=false;battery_sample.calibration_error=ESP_FAIL;}
  if(scenario==7){battery_sample.has_percent=true;battery_sample.percent=101;}
  CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_OK);
  CHECK(execute().result==DSR_IO&&battery_reads==1);
  if(scenario<3&&scenario!=1)CHECK(snapshot().state==DSR_NATIVE_FAILED&&snapshot().error==DSR_NATIVE_ERROR_BATTERY_LOW&&!install_calls&&!snapshot().boot_selection_attempted);
  else CHECK(install_calls==1); /* unknown: still requires the USB consent */
 }
 available(DSR_OTA_DEV);battery_sample.adc_mv=1745;
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,false)==DSR_INVALID&&!install_calls&&!battery_reads);
 available(DSR_OTA_DEV);cancel_battery=true;
 CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_OK);
 CHECK(execute().result==DSR_IO&&battery_reads==1&&!install_calls);
 CHECK(snapshot().state==DSR_NATIVE_CANCELLED&&snapshot().error==DSR_NATIVE_ERROR_NONE);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);unsigned mode=(unsigned)strtoul(argv[1],NULL,10);
 if(mode==4)strcpy(description.version,"not-a-version");
 if(mode==5)no_memory=true;
 CHECK(dsr_native_ota_init()==(mode==5?ESP_ERR_NO_MEM:ESP_OK));
 if(mode==5){CHECK(snapshot().state==DSR_NATIVE_FAILED&&snapshot().error==DSR_NATIVE_ERROR_MEMORY);CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_UNSUPPORTED);}
 else if(mode==4){CHECK(snapshot().state==DSR_NATIVE_FAILED&&snapshot().error==DSR_NATIVE_ERROR_LOCAL_VERSION);CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_INVALID);}
 else{
  CHECK(dsr_native_ota_init()==ESP_ERR_INVALID_STATE);
  if(!mode)lifecycle();
  else if(mode==7)battery_gate();
  else{
   bool success=mode==1||mode==6;
   available(DSR_OTA_DEV);boot_attempted=true;install_result=success?DSR_INSTALL_OK:DSR_INSTALL_SDK;if(mode==3||mode==6)cancel_at=4;
   CHECK(prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_OK);uint32_t installed_id=current_id;
   CHECK(dsr_native_ota_prepare(installed_id+1,DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_BUSY);
   CHECK(execute().result==(success?DSR_OK:DSR_IO));dsr_native_ota_snapshot_t final=snapshot();
   CHECK(install_calls==1&&final.boot_selection_attempted&&stage==(success?DSR_OTA_STAGE_STAGED:DSR_OTA_STAGE_UNCERTAIN));
   CHECK(final.state==(success?DSR_NATIVE_STAGED:DSR_NATIVE_FAILED));
   dsr_native_ota_cancel();dsr_native_ota_reject(installed_id);CHECK(execute().result==DSR_INVALID&&install_calls==1);
   CHECK(prepare(DSR_JOB_OTA_CHECK,DSR_OTA_DEV,false)==DSR_BUSY&&prepare(DSR_JOB_OTA_INSTALL,DSR_OTA_DEV,true)==DSR_BUSY&&install_calls==1);
  }
 }
 CHECK(!mutex.held);printf("PASS: %u actual native OTA actor checks; mode%u, id/ownership/cancel/TTL/channel/power/one-shot/terminal stage; chain/installer/clock/worker MODEL\n",checks,mode);return 0;
}
