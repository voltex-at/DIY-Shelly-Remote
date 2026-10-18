#include "dsr_ble.h"
#include "dsr_ble_shared.h"
#include "dsr_activity.h"
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
static atomic_bool host_on;
static void (*rpc_operation)(bool host_ready);
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
bool dsr_ble_host_healthy(void){return atomic_load(&synced)&&!atomic_load(&host_failed);}
static bool prepare_host(void)
{
 if(!initialized){
  if(nimble_port_init()!=ESP_OK){memory_log("init_failed");return false;}
  initialized=true;atomic_store(&host_on,true);ble_hs_cfg.reset_cb=reset;ble_hs_cfg.sync_cb=sync_host;
  nimble_port_freertos_init(host_task);
 }
 uint32_t waiting=now_ms();
 while(!atomic_load(&synced)&&!atomic_load(&host_failed)&&!atomic_load(&cancelled)&&now_ms()-waiting<4000)vTaskDelay(pdMS_TO_TICKS(20));
 return !atomic_load(&cancelled)&&dsr_ble_host_healthy();
}
static void scan_once(uint32_t id)
{
 memory_log("before");
 if(atomic_load(&cancelled)){finish(id,DSR_BLE_CANCELLED);return;}
 if(!prepare_host()){finish(id,atomic_load(&cancelled)?DSR_BLE_CANCELLED:DSR_BLE_UNAVAILABLE);return;}
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
/* 0.7.52: release host and controller after DSR_BLE_IDLE_RELEASE_MS without a
 * scan or BLE RPC. Until then they stayed on until restart: Wi-Fi kept sharing
 * the radio (coexistence) and the controller kept its internal RAM. The next
 * request initializes them again (prepare_host). */
bool dsr_ble_host_active(void){return atomic_load(&host_on);}
static void release_host(void)
{
 if(!initialized)return;
 xSemaphoreTake(mutex,portMAX_DELAY);bool idle=!busy;xSemaphoreGive(mutex);if(!idle)return;
 if(nimble_port_stop()!=0){memory_log("stop_failed");return;}
 (void)nimble_port_deinit();initialized=false;atomic_store(&host_on,false);atomic_store(&synced,false);atomic_store(&host_failed,false);memory_log("released");
}
static void scanner_task(void *arg)
{(void)arg;uint32_t id;for(;;)if(xQueueReceive(queue,&id,initialized?pdMS_TO_TICKS(DSR_BLE_IDLE_RELEASE_MS):portMAX_DELAY)!=pdTRUE)release_host();else{xSemaphoreTake(mutex,portMAX_DELAY);void (*operation)(bool)=rpc_operation;xSemaphoreGive(mutex);if(operation)operation(prepare_host());else scan_once(id);xSemaphoreTake(mutex,portMAX_DELAY);busy=false;rpc_operation=NULL;state.serial++;xSemaphoreGive(mutex);memory_log("idle");}}
bool dsr_ble_manager_rpc(void (*operation)(bool))
{
 if(!mutex||!queue||!operation)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);if(busy){xSemaphoreGive(mutex);return false;}busy=true;rpc_operation=operation;atomic_store(&cancelled,false);xSemaphoreGive(mutex);
 uint32_t token=0;if(xQueueSend(queue,&token,0)==pdTRUE)return true;
 xSemaphoreTake(mutex,portMAX_DELAY);busy=false;rpc_operation=NULL;xSemaphoreGive(mutex);return false;
}
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
 xSemaphoreTake(mutex,portMAX_DELAY);*out=state;out->busy=busy&&!rpc_operation;xSemaphoreGive(mutex);
 out->snapshot_ms=now_ms();
 if(out->phase==DSR_BLE_SCANNING||out->phase==DSR_BLE_STARTING){out->elapsed_ms=now_ms()-out->started_ms;if(out->elapsed_ms>DSR_BLE_SCAN_MS)out->elapsed_ms=DSR_BLE_SCAN_MS;}
}

static bool handoff_available,handoff_active;
static dsr_ble_handoff_t handoff;
static dsr_ble_device_t handoff_candidate;
static uint32_t handoff_requested;
static void expire_handoff(uint32_t now)
{
 /* A caller can capture now just before a competing request gets the mutex.
  * A slightly older clock is not a 49-day elapsed time. */
 if(handoff==DSR_BLE_HANDOFF_REQUESTED&&(int32_t)(now-handoff_requested)>=10000)handoff=DSR_BLE_HANDOFF_EXPIRED;
}
void dsr_ble_handoff_publish(bool available,bool active,uint32_t now)
{
 if(!mutex)return;
 xSemaphoreTake(mutex,portMAX_DELAY);expire_handoff(now);handoff_available=available;handoff_active=active;
 if(active)handoff=DSR_BLE_HANDOFF_ACTIVE;
 else if(!available)handoff=DSR_BLE_HANDOFF_UNAVAILABLE;
 else if(handoff!=DSR_BLE_HANDOFF_REQUESTED&&handoff!=DSR_BLE_HANDOFF_EXPIRED)handoff=DSR_BLE_HANDOFF_READY;
 xSemaphoreGive(mutex);
}
bool dsr_ble_handoff_request(uint32_t serial,unsigned index)
{
 if(!mutex||!dsr_activity_begin())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);uint32_t now=now_ms();expire_handoff(now);
 bool accepted=handoff_available&&!handoff_active&&!busy&&handoff!=DSR_BLE_HANDOFF_REQUESTED&&serial==state.serial&&index<state.count;
 if(accepted){const dsr_ble_device_t *d=&state.devices[index];accepted=d->kind==DSR_BLE_SHELLY_RPC&&(d->evidence&DSR_BLE_RPC)&&now-d->seen_ms<=30000;
  if(accepted){handoff_candidate=*d;handoff_requested=now;handoff=DSR_BLE_HANDOFF_REQUESTED;}}
 xSemaphoreGive(mutex);dsr_activity_end();return accepted;
}
bool dsr_ble_handoff_take(dsr_ble_device_t *candidate,uint32_t now)
{
 if(!mutex||!candidate||!dsr_activity_begin())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);expire_handoff(now);
 bool accepted=handoff_available&&!handoff_active&&!busy&&handoff==DSR_BLE_HANDOFF_REQUESTED;
 if(accepted){*candidate=handoff_candidate;handoff_active=true;handoff=DSR_BLE_HANDOFF_ACTIVE;}
 xSemaphoreGive(mutex);dsr_activity_end();return accepted;
}
dsr_ble_handoff_t dsr_ble_handoff_status(uint32_t now)
{
 if(!mutex)return DSR_BLE_HANDOFF_UNAVAILABLE;
 xSemaphoreTake(mutex,portMAX_DELAY);expire_handoff(now);dsr_ble_handoff_t result=handoff;xSemaphoreGive(mutex);return result;
}
