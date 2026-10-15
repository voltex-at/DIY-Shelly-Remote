#include "dsr_ble.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include <stdatomic.h>
#include <string.h>

static SemaphoreHandle_t mutex;
static QueueHandle_t queue;
static dsr_ble_snapshot_t state;
static atomic_bool cancelled,synced,host_failed;
static bool initialized,busy;
static uint32_t now_ms(void){return (uint32_t)(esp_timer_get_time()/1000);}
static bool running(void){return state.phase==DSR_BLE_STARTING||state.phase==DSR_BLE_SCANNING;}
static void memory_log(const char *phase)
{ESP_LOGI("dsr_ble","%s internal_free=%u largest=%u psram_free=%u",phase,(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),(unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),(unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));}
static void reset(int reason){(void)reason;atomic_store(&synced,false);atomic_store(&host_failed,true);}
static void sync_host(void)
{if(ble_hs_util_ensure_addr(0)==0){atomic_store(&host_failed,false);atomic_store(&synced,true);}else atomic_store(&host_failed,true);}
static void host_task(void *arg){(void)arg;nimble_port_run();nimble_port_freertos_deinit();}
static void finish(uint32_t id,dsr_ble_phase_t phase)
{xSemaphoreTake(mutex,portMAX_DELAY);dsr_ble_finish(&state,id,phase,now_ms());xSemaphoreGive(mutex);}
static int gap_event(struct ble_gap_event *event,void *arg)
{
 uint32_t id=(uint32_t)(uintptr_t)arg;
 if(event->type==BLE_GAP_EVENT_DISC){
  if(atomic_load(&cancelled))return 0;
  const struct ble_gap_disc_desc *d=&event->disc;
  xSemaphoreTake(mutex,portMAX_DELAY);dsr_ble_ingest(&state,id,d->addr.val,d->addr.type,d->rssi,d->data,d->length_data,now_ms());xSemaphoreGive(mutex);
 }else if(event->type==BLE_GAP_EVENT_DISC_COMPLETE){
  finish(id,atomic_load(&cancelled)?DSR_BLE_CANCELLED:(event->disc_complete.reason==0||event->disc_complete.reason==BLE_HS_ETIMEOUT)?DSR_BLE_DONE:DSR_BLE_UNAVAILABLE);
 }
 return 0;
}
static void scan_once(uint32_t id)
{
 memory_log("before");
 if(atomic_load(&cancelled)){finish(id,DSR_BLE_CANCELLED);return;}
 if(!initialized){
  if(nimble_port_init()!=ESP_OK){finish(id,DSR_BLE_UNAVAILABLE);memory_log("init_failed");return;}
  initialized=true;ble_hs_cfg.reset_cb=reset;ble_hs_cfg.sync_cb=sync_host;
  nimble_port_freertos_init(host_task);
 }
 uint32_t waiting=now_ms();
 while(!atomic_load(&synced)&&!atomic_load(&host_failed)&&!atomic_load(&cancelled)&&now_ms()-waiting<4000)vTaskDelay(pdMS_TO_TICKS(20));
 if(atomic_load(&cancelled)){finish(id,DSR_BLE_CANCELLED);return;}
 if(!atomic_load(&synced)||atomic_load(&host_failed)){finish(id,DSR_BLE_UNAVAILABLE);return;}
 uint8_t own_address;
 if(ble_hs_id_infer_auto(0,&own_address)!=0){finish(id,DSR_BLE_UNAVAILABLE);return;}
 /* Active scan obtains manufacturer/name scan responses. 25% scan duty cycle;
  * bounded 20 s, no connect, no pairing, no service/credential writes. */
 struct ble_gap_disc_params params={.passive=0,.itvl=160,.window=40,.filter_duplicates=0};
 xSemaphoreTake(mutex,portMAX_DELAY);state.phase=DSR_BLE_SCANNING;state.started_ms=now_ms();state.serial++;xSemaphoreGive(mutex);
 if(ble_gap_disc(own_address,DSR_BLE_SCAN_MS,&params,gap_event,(void *)(uintptr_t)id)!=0){finish(id,DSR_BLE_UNAVAILABLE);return;}
 memory_log("scanning");uint32_t started=now_ms();
 for(;;){
  xSemaphoreTake(mutex,portMAX_DELAY);bool active=running();xSemaphoreGive(mutex);
  if(!active)break;
  if(atomic_load(&cancelled)||atomic_load(&host_failed)||now_ms()-started>=DSR_BLE_SCAN_MS){
   /* Seal first: a late advertisement or completion cannot publish new rows. */
   finish(id,atomic_load(&cancelled)?DSR_BLE_CANCELLED:atomic_load(&host_failed)?DSR_BLE_UNAVAILABLE:DSR_BLE_DONE);
   (void)ble_gap_disc_cancel();break;
  }
  vTaskDelay(pdMS_TO_TICKS(20));
 }
 /* Successful completion already stopped discovery. Cancel is also idempotent
  * at the manager level; allow next requests only after the radio is idle. */
 if(ble_gap_disc_active())(void)ble_gap_disc_cancel();
}
static void scanner_task(void *arg)
{(void)arg;uint32_t id;for(;;)if(xQueueReceive(queue,&id,portMAX_DELAY)==pdTRUE){scan_once(id);xSemaphoreTake(mutex,portMAX_DELAY);busy=false;state.serial++;xSemaphoreGive(mutex);memory_log("idle");}}
esp_err_t dsr_ble_init(void)
{
 if(mutex)return ESP_OK;
 mutex=xSemaphoreCreateMutex();queue=xQueueCreate(1,sizeof(uint32_t));
 if(!mutex||!queue){if(mutex)vSemaphoreDelete(mutex);if(queue)vQueueDelete(queue);mutex=NULL;queue=NULL;return ESP_ERR_NO_MEM;}
 if(xTaskCreatePinnedToCore(scanner_task,"dsr_ble_scan",4096,NULL,2,NULL,1)!=pdPASS){vSemaphoreDelete(mutex);vQueueDelete(queue);mutex=NULL;queue=NULL;return ESP_ERR_NO_MEM;}
 return ESP_OK;
}
bool dsr_ble_scan(void)
{
 if(!mutex||!queue)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);
 if(busy){xSemaphoreGive(mutex);return false;}
 busy=true;dsr_ble_begin(&state,now_ms());uint32_t id=state.generation;atomic_store(&cancelled,false);xSemaphoreGive(mutex);
 if(xQueueSend(queue,&id,0)!=pdTRUE){finish(id,DSR_BLE_UNAVAILABLE);xSemaphoreTake(mutex,portMAX_DELAY);busy=false;xSemaphoreGive(mutex);return false;}
 return true;
}
void dsr_ble_cancel(void){atomic_store(&cancelled,true);}
void dsr_ble_snapshot(dsr_ble_snapshot_t *out)
{
 if(!out)return;
 if(!mutex){memset(out,0,sizeof(*out));out->phase=DSR_BLE_UNAVAILABLE;return;}
 xSemaphoreTake(mutex,portMAX_DELAY);*out=state;out->busy=busy;xSemaphoreGive(mutex);
 out->snapshot_ms=now_ms();
 if(out->phase==DSR_BLE_SCANNING||out->phase==DSR_BLE_STARTING){out->elapsed_ms=now_ms()-out->started_ms;if(out->elapsed_ms>DSR_BLE_SCAN_MS)out->elapsed_ms=DSR_BLE_SCAN_MS;}
}
