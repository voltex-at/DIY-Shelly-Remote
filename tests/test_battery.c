/* Real ADC adapter and actual worker routing; synthetic SDK responses only. */
#include "dsr_platform.h"
#include "dsr_native_ota.h"
#include "../components/dsr_platform/dsr_ap_private.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
static unsigned checks,mode,reads,adc_live,cal_live,adc_deletes,cal_deletes;
static bool worker_context,fail_queue;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static int adc_token,cal_token;
esp_err_t adc_oneshot_io_to_channel(int gpio,adc_unit_t *unit,adc_channel_t *channel)
{CHECK(gpio==8);*unit=mode==1?ADC_UNIT_2:ADC_UNIT_1;*channel=7;return mode==2?ESP_FAIL:ESP_OK;}
esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *c,adc_oneshot_unit_handle_t *h)
{CHECK(c->unit_id==ADC_UNIT_1&&c->ulp_mode==0&&!adc_live);if(mode==3)return ESP_ERR_NO_MEM;adc_live++;*h=&adc_token;return ESP_OK;}
esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t h,adc_channel_t channel,const adc_oneshot_chan_cfg_t *c)
{CHECK(h==&adc_token&&channel==7&&c->atten==12&&c->bitwidth==12);return mode==4?ESP_FAIL:ESP_OK;}
esp_err_t adc_cali_create_scheme_curve_fitting(const adc_cali_curve_fitting_config_t *c,adc_cali_handle_t *h)
{CHECK(c->unit_id==0&&c->chan==7&&c->atten==12&&c->bitwidth==12&&!cal_live);if(mode==5)return ESP_ERR_NOT_SUPPORTED;cal_live++;*h=&cal_token;return ESP_OK;}
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t h,adc_channel_t channel,int *raw)
{
 CHECK(h==&adc_token&&channel==7);if(worker_context)CHECK(!dsr_activity_restart_reserve());reads++;
 static const int values[]={3000,2005,1999,2002,2001,2000,1998,2003,0,4095};
 CHECK(reads<=10);*raw=mode==7?4095:mode==8?0:mode==9?-1:values[reads-1];
 return mode==6&&reads==4?ESP_FAIL:ESP_OK;
}
esp_err_t adc_cali_raw_to_voltage(adc_cali_handle_t h,int raw,int *mv)
{CHECK(h==&cal_token&&raw==2001);*mv=mode==11?4000:mode==12?0:mode==13?-1:2025;return mode==10?ESP_FAIL:ESP_OK;}
esp_err_t adc_cali_delete_scheme_curve_fitting(adc_cali_handle_t h)
{CHECK(h==&cal_token&&cal_live==1);cal_live--;cal_deletes++;return mode==14?ESP_FAIL:ESP_OK;}
esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t h)
{CHECK(h==&adc_token&&adc_live==1);adc_live--;adc_deletes++;return mode==15?ESP_FAIL:ESP_OK;}
static void reset(unsigned m){CHECK(!adc_live&&!cal_live);mode=m;reads=adc_deletes=cal_deletes=0;}

/* Only existing queue/worker lifetime is used, no battery task or timer. */
typedef struct {size_t size;bool occupied;unsigned char data[sizeof(dsr_event_t)+sizeof(dsr_job_t)];} queue_t;
static queue_t queues[2];static unsigned queue_count;static bool semaphore;
static void (*worker)(void *);static jmp_buf stop;
QueueHandle_t xQueueCreate(unsigned count,unsigned size)
{CHECK(count==1&&queue_count<2&&size<=sizeof(queues[0].data));queue_t *q=&queues[queue_count++];q->size=size;return q;}
int xQueueSend(QueueHandle_t handle,const void *data,TickType_t wait)
{(void)wait;queue_t *q=handle;CHECK(!q->occupied&&!dsr_activity_restart_reserve());if(q==&queues[0]&&fail_queue)return pdFALSE;memcpy(q->data,data,q->size);q->occupied=true;return pdTRUE;}
int xQueueReceive(QueueHandle_t handle,void *data,TickType_t wait)
{queue_t *q=handle;if(!q->occupied){if(wait)longjmp(stop,1);return pdFALSE;}memcpy(data,q->data,q->size);q->occupied=false;return pdTRUE;}
void vQueueDelete(QueueHandle_t q){(void)q;}
SemaphoreHandle_t xSemaphoreCreateBinary(void){return &semaphore;}
int xSemaphoreTake(SemaphoreHandle_t h,TickType_t wait){(void)wait;CHECK(h==&semaphore);if(!semaphore)return pdFALSE;semaphore=false;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t h){CHECK(h==&semaphore);semaphore=true;return pdTRUE;}
void vSemaphoreDelete(SemaphoreHandle_t h){(void)h;}
int xTaskCreate(void (*entry)(void *),const char *name,unsigned bytes,void *arg,unsigned priority,TaskHandle_t *out)
{CHECK(!strcmp(name,"dsr_network")&&bytes==10240&&!arg&&priority==4&&!out);worker=entry;return pdPASS;}
static dsr_settings_t display_stored={.ssid="new-network",.password="latest-private-password",.wifi_security=DSR_WIFI_WPA2,.brightness=70,.idle_seconds=60};
static unsigned display_load_error,display_save_error,display_saves;
esp_err_t dsr_settings_load(dsr_settings_t *out){CHECK(!dsr_activity_restart_reserve());if(display_load_error)return ESP_FAIL;*out=display_stored;return ESP_OK;}
esp_err_t dsr_settings_save(const dsr_settings_t *in){CHECK(!dsr_activity_restart_reserve());display_saves++;CHECK(!strcmp(in->ssid,"new-network")&&!strcmp(in->password,"latest-private-password")&&in->wifi_security==DSR_WIFI_WPA2);if(display_save_error)return ESP_FAIL;display_stored=*in;return ESP_OK;}
static bool worker_wifi;
bool dsr_wifi_connected(void){return worker_wifi;}
static bool temporary_ap;
static unsigned ap_executed,ap_prepared,wifi_prepared;
bool dsr_wifi_temporary_active(void){return temporary_ap;}
void dsr_ap_execute(const dsr_job_t *job,dsr_event_t *event){CHECK(((job->kind>=DSR_JOB_AP_SCAN&&job->kind<=DSR_JOB_AP_VERIFY)||job->kind==DSR_JOB_LAN_STA_VERIFY)&&!dsr_activity_restart_reserve());ap_executed++;event->result=DSR_OK;}
void dsr_ap_prepare(void){ap_prepared++;}
static bool ota_admit;
static unsigned ota_executed,ota_initialized;
esp_err_t dsr_native_ota_init(void){ota_initialized++;return ESP_OK;}
bool dsr_native_ota_admitted(uint32_t id,dsr_job_kind_t kind){CHECK(id&&(kind==DSR_JOB_OTA_CHECK||kind==DSR_JOB_OTA_INSTALL));return ota_admit;}
void dsr_native_ota_execute(const dsr_job_t *job,dsr_event_t *event){CHECK(ota_admit&&(job->kind==DSR_JOB_OTA_CHECK||job->kind==DSR_JOB_OTA_INSTALL)&&!dsr_activity_restart_reserve());ota_executed++;event->result=DSR_OK;}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){return c&&(!bound||!strcmp(c->device.device_id,"maintenance"));}
void dsr_wifi_prepare_connect(void){wifi_prepared++;}
dsr_result_t dsr_wifi_scan(dsr_wifi_scan_t *s){(void)s;abort();}
dsr_result_t dsr_wifi_connect_saved(const dsr_settings_t *s,dsr_wifi_status_t *d){(void)s;(void)d;abort();}
static unsigned bind_probes;static dsr_result_t probe_result=DSR_OK;
dsr_result_t dsr_http_probe_telemetry(const dsr_connection_t *c,dsr_device_t *d,dsr_telemetry_t *t){CHECK(worker_wifi&&!dsr_activity_restart_reserve());bind_probes++;*d=c->device;d->online=d->has_switch=probe_result==DSR_OK;*t=(dsr_telemetry_t){.has_voltage=true,.voltage_v=230.0};return probe_result;}
esp_err_t dsr_connection_layout_id(const dsr_connection_t *c,uint32_t *id){(void)c;(void)id;abort();}
static unsigned publishes,removes;
esp_err_t dsr_connection_publish(const dsr_connection_t *c){CHECK(c->layout_id==42&&!dsr_activity_restart_reserve());publishes++;return ESP_OK;}
esp_err_t dsr_connection_remove(const dsr_connection_t *c){CHECK(!dsr_activity_restart_reserve());removes++;return c->layout_id==42?ESP_OK:ESP_FAIL;}
static bool mapping_allowed=true,private_failure;static dsr_connection_t private_record;
esp_err_t dsr_connection_save(const dsr_connection_t *c){CHECK(c->layout_id==42&&!dsr_activity_restart_reserve());publishes++;if(private_failure)return ESP_FAIL;private_record=*c;return ESP_OK;}
bool dsr_workspace_mapping_matches(const dsr_connection_t *c){return mapping_allowed&&c->layout_id==42;}
dsr_result_t dsr_http_refresh(dsr_connection_t *c){(void)c;abort();}
dsr_result_t dsr_http_set_output(dsr_connection_t *c,bool on){(void)c;(void)on;abort();}
static void admission_open(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static unsigned history_services,history_clocks;
void dsr_history_service(void){CHECK(worker_context&&!dsr_activity_restart_reserve());history_services++;}
void dsr_history_clock_execute(void){CHECK(worker_context&&!dsr_activity_restart_reserve());history_clocks++;}
int main(void)
{
 CHECK(dsr_battery_read(NULL)==ESP_ERR_INVALID_ARG);
 for(unsigned n=0;n<16;n++){
  reset(n);dsr_battery_sample_t sample;memset(&sample,0xa5,sizeof(sample));esp_err_t error=dsr_battery_read(&sample);
  CHECK(!adc_live&&!cal_live);
  CHECK(adc_deletes==(n>=4||n==0?1u:0u));
  if(n==0){CHECK(error==ESP_OK&&sample.has_raw&&sample.has_voltage&&sample.raw==2001&&sample.spread==4095&&sample.adc_mv==2025&&sample.bat_mv==4050&&reads==10);}
  else if(n==5){CHECK(error==ESP_OK&&sample.has_raw&&!sample.has_voltage&&sample.calibration_error==ESP_ERR_NOT_SUPPORTED&&sample.bat_mv==0);}
  else CHECK(error!=ESP_OK&&!sample.has_voltage);
 }
 for(unsigned n=0;n<100;n++){reset(0);dsr_battery_sample_t s;CHECK(dsr_battery_read(&s)==ESP_OK&&!adc_live&&!cal_live);}
 CHECK(dsr_worker_start()==ESP_OK&&worker&&queue_count==2);
 for(unsigned n=0;n<3;n++){
  reset(n==1?5:n==2?6:0);dsr_job_t job={.id=n+1,.kind=DSR_JOB_BATTERY_READ};
  CHECK(dsr_worker_submit(&job)==DSR_OK);CHECK(!dsr_activity_restart_reserve());CHECK(dsr_worker_submit(&job)==DSR_BUSY);
  worker_context=true;if(!setjmp(stop))worker(NULL);worker_context=false;CHECK(!dsr_activity_restart_reserve());
  dsr_event_t event;CHECK(dsr_worker_receive(&event)&&event.id==n+1);
  CHECK(event.result==(n==2?DSR_IO:DSR_OK)&&event.battery.has_voltage==(n==0));
  CHECK(!dsr_worker_receive(&event)&&!adc_live&&!cal_live);admission_open();
 }
 /* Display jobs remain functional offline and preserve the newest Wi-Fi
  * record even if the submitted UI snapshot contains obsolete credentials. */
 for(unsigned n=0;n<4;n++){
  display_load_error=n==1;display_save_error=n==2;
  unsigned before=display_saves;
  dsr_job_t job={.id=100+n,.kind=DSR_JOB_DISPLAY_SAVE,.settings={.ssid="old",.password="obsolete",.brightness=42,.idle_seconds=n==3?0:47}};
  CHECK(dsr_worker_submit(&job)==DSR_OK);CHECK(!dsr_activity_restart_reserve());CHECK(dsr_worker_submit(&job)==DSR_BUSY);
  if(!setjmp(stop))worker(NULL);
  CHECK(!dsr_activity_restart_reserve());
  dsr_event_t event;CHECK(dsr_worker_receive(&event)&&event.id==100+n);
  CHECK(event.result==((n==1||n==2)?DSR_IO:DSR_OK));
  CHECK(display_saves==before+(n==1?0:1));
  CHECK(event.brightness==42&&event.idle_seconds==job.settings.idle_seconds);
  CHECK(!strcmp(display_stored.ssid,"new-network")&&!strcmp(display_stored.password,"latest-private-password"));
  CHECK(!memcmp(&event.device,&(dsr_device_t){0},sizeof(event.device)));admission_open();
 }
 for(unsigned n=0;n<3;n++){
  dsr_job_t job={.id=150+n,.kind=n?DSR_JOB_UNBIND:DSR_JOB_LAYOUT_PUBLISH};strcpy(job.connection.device.device_id,"maintenance");job.connection.layout_id=n==2?0:42;
  CHECK(dsr_worker_submit(&job)==DSR_OK);if(!setjmp(stop))worker(NULL);
  dsr_event_t event;CHECK(dsr_worker_receive(&event)&&event.id==job.id&&event.layout_id==job.connection.layout_id&&event.result==(n==2?DSR_IO:DSR_OK));admission_open();
 }
 CHECK(publishes==1&&removes==2); /* maintenance offline: zero HTTP calls */
 /* Actual BIND router: probe never writes, including a rebind with a new
  * locator/credential. A separate exact mapping guard precedes full save. */
 private_record=(dsr_connection_t){.device={.device_id="maintenance",.host="192.168.1.10"},.credentials={.password="previous-private"},.layout_id=42};
 dsr_connection_t prior=private_record;unsigned before_private=publishes;worker_wifi=true;
 dsr_job_t bind={.id=153,.kind=DSR_JOB_BIND,.connection=prior};strcpy(bind.connection.device.host,"192.168.1.20");strcpy(bind.connection.credentials.password,"replacement-private");
 CHECK(dsr_worker_submit(&bind)==DSR_OK);if(!setjmp(stop))worker(NULL);dsr_event_t checked;CHECK(dsr_worker_receive(&checked)&&checked.result==DSR_OK&&checked.id==bind.id&&bind_probes==1);
 CHECK(publishes==before_private&&!memcmp(&prior,&private_record,sizeof(prior)));admission_open();
 for(unsigned phase=0;phase<3;phase++){
  mapping_allowed=phase!=0;private_failure=phase==1;dsr_job_t publish=bind;publish.id=154+phase;publish.kind=DSR_JOB_LAYOUT_PUBLISH;
  CHECK(dsr_worker_submit(&publish)==DSR_OK);if(!setjmp(stop))worker(NULL);dsr_event_t committed;CHECK(dsr_worker_receive(&committed)&&committed.id==publish.id&&committed.result==(phase==0?DSR_CONFLICT:phase==1?DSR_IO:DSR_OK));
  if(phase<2)CHECK(!memcmp(&prior,&private_record,sizeof(prior)));else CHECK(!strcmp(private_record.device.host,"192.168.1.20")&&!strcmp(private_record.credentials.password,"replacement-private"));
  CHECK(bind_probes==1&&removes==2);admission_open();
 }
 CHECK(publishes==before_private+2);probe_result=DSR_AUTH_REQUIRED;bind.id=157;CHECK(dsr_worker_submit(&bind)==DSR_OK);if(!setjmp(stop))worker(NULL);CHECK(dsr_worker_receive(&checked)&&checked.result==DSR_AUTH_REQUIRED&&publishes==before_private+2&&bind_probes==2);admission_open();worker_wifi=false;private_failure=false;
 CHECK(ota_initialized==1);
 for(unsigned kind=DSR_JOB_OTA_CHECK;kind<=DSR_JOB_OTA_INSTALL;kind++){
  dsr_job_t ota={.id=160+kind,.kind=kind};ota_admit=false;CHECK(dsr_worker_submit(&ota)==DSR_INVALID);
  ota_admit=true;CHECK(dsr_worker_submit(&ota)==DSR_OK);if(!setjmp(stop))worker(NULL);
  dsr_event_t e;CHECK(dsr_worker_receive(&e)&&e.id==ota.id&&e.result==DSR_OK);admission_open();
 }
 CHECK(ota_executed==2);ota_admit=false;
 /* Actual sole-worker router/validator: AP work is admitted offline and while
  * the station is temporarily on an AP, without falling through to relay SET. */
 for(unsigned kind=DSR_JOB_AP_SCAN;kind<=DSR_JOB_AP_VERIFY;kind++){
  dsr_job_t ap={.id=170+kind,.kind=kind,.ap_confirmed=true,.settings={.ssid="Home",.password="private-home-psk",.wifi_security=DSR_WIFI_WPA2}};
  if(kind==DSR_JOB_AP_INSPECT)ap.ap=(dsr_wifi_ap_t){.ssid="ShellyPlus1-112233445566",.security=DSR_WIFI_OPEN,.bssid={1,2,3,4,5,6}};
  else ap.ap_identity=(dsr_ap_identity_t){.ssid="ShellyPlus1-112233445566",.id="shellyplus1-112233445566",.mac="112233445566",.generation=2};
  strcpy(ap.connection.device.host,"192.168.1.10");temporary_ap=true;CHECK(dsr_worker_submit(&ap)==DSR_OK);if(!setjmp(stop))worker(NULL);
  dsr_event_t e;CHECK(dsr_worker_receive(&e)&&e.id==ap.id&&e.result==DSR_OK);admission_open();
 }
 CHECK(ap_executed==5&&ap_prepared==2&&wifi_prepared==1);
 dsr_job_t ap={.kind=DSR_JOB_AP_INSPECT,.ap={.ssid="ShellyPlus1-112233445566",.security=DSR_WIFI_OPEN}};
 CHECK(dsr_worker_submit(&ap)==DSR_INVALID);ap.ap_confirmed=true;ap.ap.security=DSR_WIFI_WPA2;CHECK(dsr_worker_submit(&ap)==DSR_INVALID);
 dsr_job_t blocked={.kind=DSR_JOB_BATTERY_READ};CHECK(dsr_worker_submit(&blocked)==DSR_BUSY);temporary_ap=false;
 dsr_job_t sta={.id=198,.kind=DSR_JOB_LAN_STA_VERIFY,.settings={.ssid="Home",.password="private-home-psk",.wifi_security=DSR_WIFI_WPA2},.ap_identity={.id="shellyplus1-112233445566",.mac="112233445566",.generation=2}};strcpy(sta.connection.device.host,"192.168.1.10");
 CHECK(dsr_worker_submit(&sta)==DSR_OK);if(!setjmp(stop))worker(NULL);dsr_event_t verified;CHECK(dsr_worker_receive(&verified)&&verified.id==198&&verified.result==DSR_OK&&ap_executed==6&&ap_prepared==2&&wifi_prepared==1);admission_open();
 sta.ap_identity.auth_required=true;CHECK(dsr_worker_submit(&sta)==DSR_INVALID);sta.ap_identity.auth_required=false;sta.ap_identity.mac[0]=0;CHECK(dsr_worker_submit(&sta)==DSR_INVALID);
 /* History has no payload growth, binding, HTTP or radio handoff. The sole
  * worker's existing lease remains held through event acknowledgement. */
 for(unsigned kind=DSR_JOB_HISTORY_SERVICE;kind<=DSR_JOB_HISTORY_CLOCK;kind++){
  dsr_job_t job={.id=300+kind,.kind=kind};CHECK(dsr_worker_submit(&job)==DSR_OK);CHECK(!dsr_activity_restart_reserve());worker_context=true;if(!setjmp(stop))worker(NULL);worker_context=false;CHECK(!dsr_activity_restart_reserve());dsr_event_t event;CHECK(dsr_worker_receive(&event)&&event.id==job.id&&event.result==DSR_OK);admission_open();
 }
 CHECK(history_services==1&&history_clocks==1);
 CHECK(dsr_activity_restart_reserve());dsr_job_t history={.kind=DSR_JOB_HISTORY_SERVICE};CHECK(dsr_worker_submit(&history)==DSR_BUSY);dsr_activity_restart_cancel();temporary_ap=true;CHECK(dsr_worker_submit(&history)==DSR_BUSY);temporary_ap=false;
 dsr_job_t invalid={.kind=DSR_JOB_DISPLAY_SAVE};
 CHECK(dsr_worker_submit(&invalid)==DSR_INVALID);invalid.settings.brightness=101;
 CHECK(dsr_worker_submit(&invalid)==DSR_INVALID);invalid.settings.brightness=70;invalid.settings.idle_seconds=3601;
 CHECK(dsr_worker_submit(&invalid)==DSR_INVALID);
 admission_open();
 dsr_job_t admitted={.id=200,.kind=DSR_JOB_BATTERY_READ};
 fail_queue=true;CHECK(dsr_worker_submit(&admitted)==DSR_BUSY&&!queues[0].occupied&&semaphore);admission_open();fail_queue=false;
 CHECK(dsr_activity_restart_reserve());CHECK(dsr_worker_submit(&admitted)==DSR_BUSY&&!queues[0].occupied&&semaphore);dsr_activity_restart_cancel();admission_open();
 reset(0);CHECK(dsr_worker_submit(&admitted)==DSR_OK&&!dsr_activity_restart_reserve());worker_context=true;if(!setjmp(stop))worker(NULL);worker_context=false;
 CHECK(!dsr_activity_restart_reserve());dsr_event_t final_event;CHECK(dsr_worker_receive(&final_event)&&final_event.id==200);admission_open();
 printf("PASS: %u battery/worker checks; actual ADC lifecycle/filter/calibration, failure cleanup, 100 cycles, offline/no-binding worker route; synthetic SDK, not physical voltage\n",checks);return 0;
}
