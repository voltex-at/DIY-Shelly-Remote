/* Real ADC adapter and actual worker routing; synthetic SDK responses only. */
#include "dsr_platform.h"
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
 CHECK(h==&adc_token&&channel==7);reads++;
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
{(void)wait;queue_t *q=handle;CHECK(!q->occupied);memcpy(q->data,data,q->size);q->occupied=true;return pdTRUE;}
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
esp_err_t dsr_settings_load(dsr_settings_t *out){if(display_load_error)return ESP_FAIL;*out=display_stored;return ESP_OK;}
esp_err_t dsr_settings_save(const dsr_settings_t *in){display_saves++;CHECK(!strcmp(in->ssid,"new-network")&&!strcmp(in->password,"latest-private-password")&&in->wifi_security==DSR_WIFI_WPA2);if(display_save_error)return ESP_FAIL;display_stored=*in;return ESP_OK;}
bool dsr_wifi_connected(void){return false;}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){(void)c;(void)bound;return false;}
bool dsr_wifi_settings_valid(const dsr_settings_t *s){(void)s;return false;}
void dsr_wifi_prepare_connect(void){abort();}
dsr_result_t dsr_wifi_scan(dsr_wifi_scan_t *s){(void)s;abort();}
dsr_result_t dsr_wifi_connect_saved(const dsr_settings_t *s,dsr_wifi_status_t *d){(void)s;(void)d;abort();}
dsr_result_t dsr_http_probe_telemetry(const dsr_connection_t *c,dsr_device_t *d,dsr_telemetry_t *t){(void)c;(void)d;(void)t;abort();}
esp_err_t dsr_connection_save(const dsr_connection_t *c){(void)c;abort();}
dsr_result_t dsr_http_refresh(dsr_connection_t *c){(void)c;abort();}
dsr_result_t dsr_http_set_output(dsr_connection_t *c,bool on){(void)c;(void)on;abort();}
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
  CHECK(dsr_worker_submit(&job)==DSR_OK);CHECK(dsr_worker_submit(&job)==DSR_BUSY);
  if(!setjmp(stop))worker(NULL);
  dsr_event_t event;CHECK(dsr_worker_receive(&event)&&event.id==n+1);
  CHECK(event.result==(n==2?DSR_IO:DSR_OK)&&event.battery.has_voltage==(n==0));
  CHECK(!dsr_worker_receive(&event)&&!adc_live&&!cal_live);
 }
 /* Display jobs remain functional offline and preserve the newest Wi-Fi
  * record even if the submitted UI snapshot contains obsolete credentials. */
 for(unsigned n=0;n<4;n++){
  display_load_error=n==1;display_save_error=n==2;
  unsigned before=display_saves;
  dsr_job_t job={.id=100+n,.kind=DSR_JOB_DISPLAY_SAVE,.settings={.ssid="old",.password="obsolete",.brightness=42,.idle_seconds=n==3?0:47}};
  CHECK(dsr_worker_submit(&job)==DSR_OK);CHECK(dsr_worker_submit(&job)==DSR_BUSY);
  if(!setjmp(stop))worker(NULL);
  dsr_event_t event;CHECK(dsr_worker_receive(&event)&&event.id==100+n);
  CHECK(event.result==((n==1||n==2)?DSR_IO:DSR_OK));
  CHECK(display_saves==before+(n==1?0:1));
  CHECK(event.brightness==42&&event.idle_seconds==job.settings.idle_seconds);
  CHECK(!strcmp(display_stored.ssid,"new-network")&&!strcmp(display_stored.password,"latest-private-password"));
  CHECK(!memcmp(&event.device,&(dsr_device_t){0},sizeof(event.device)));
 }
 dsr_job_t invalid={.kind=DSR_JOB_DISPLAY_SAVE};
 CHECK(dsr_worker_submit(&invalid)==DSR_INVALID);invalid.settings.brightness=101;
 CHECK(dsr_worker_submit(&invalid)==DSR_INVALID);invalid.settings.brightness=70;invalid.settings.idle_seconds=3601;
 CHECK(dsr_worker_submit(&invalid)==DSR_INVALID);
 printf("PASS: %u battery/worker checks; actual ADC lifecycle/filter/calibration, failure cleanup, 100 cycles, offline/no-binding worker route; synthetic SDK, not physical voltage\n",checks);return 0;
}
