#include "dsr_ble.h"
#include "dsr_ble_shared.h"
#ifdef ESP_PLATFORM
#include "dsr_blu.h"
#endif
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
#if defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA)
#include "dsr_maintenance_private.h"
#include "dsr_backup_bonds.h"
#include "dsr_backup_format.h"
#include "dsr_backup_json.h"
#include <stdlib.h>
typedef struct {dsr_maint_lease_t lease;cJSON *root;atomic_uint refs;atomic_bool done;dsr_result_t result;} backup_request_t;
static backup_request_t *backup_request;
static TaskHandle_t manager_handle;
static atomic_bool backup_owner;
static void backup_release(backup_request_t *r)
{if(atomic_fetch_sub(&r->refs,1)==1){dsr_backup_json_discard(r->root);dsr_backup_wipe(r,sizeof(*r));free(r);}}
#endif

static SemaphoreHandle_t mutex;
static QueueHandle_t queue;
static dsr_ble_snapshot_t state;
static atomic_bool cancelled,synced,host_failed,quarantined;
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
bool dsr_ble_host_healthy(void){return atomic_load(&synced)&&!atomic_load(&host_failed)&&!atomic_load(&quarantined);}
void dsr_ble_manager_quarantine(void)
{
 atomic_store(&quarantined,true);
 /* Schedule on the host task; do not destroy a live host or its callback data.
  * Its later sync cannot reopen admission before a complete deinit. */
 ble_hs_sched_reset(BLE_HS_EUNKNOWN);
 memory_log("quarantined");
}
/* dsr_wifi.c: coexistence needs modem sleep before the controller starts. */
void dsr_wifi_power_ble(bool active);
static bool prepare_host(void)
{
 if(!initialized){
  dsr_wifi_power_ble(true);
  if(nimble_port_init()!=ESP_OK){dsr_wifi_power_ble(false);memory_log("init_failed");return false;}
  initialized=true;atomic_store(&host_on,true);ble_hs_cfg.reset_cb=reset;ble_hs_cfg.sync_cb=sync_host;
  #ifdef ESP_PLATFORM
  dsr_blu_host_setup();
#endif
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
static bool release_host(bool reserved)
{
 if(!initialized)return true;
 xSemaphoreTake(mutex,portMAX_DELAY);bool idle=!busy||reserved;xSemaphoreGive(mutex);if(!idle)return false;
 if(nimble_port_stop()!=0){memory_log("stop_failed");return false;}
 if(nimble_port_deinit()!=ESP_OK){atomic_store(&quarantined,true);memory_log("deinit_failed");return false;}
 initialized=false;atomic_store(&host_on,false);atomic_store(&synced,false);atomic_store(&host_failed,false);atomic_store(&quarantined,false);memory_log("released");
 dsr_wifi_power_ble(false);
 return true;
}
static void scanner_task(void *arg)
{
 (void)arg;uint32_t id;
 for(;;){
  if(xQueueReceive(queue,&id,initialized?pdMS_TO_TICKS(DSR_BLE_IDLE_RELEASE_MS):portMAX_DELAY)!=pdTRUE){(void)release_host(false);continue;}
  xSemaphoreTake(mutex,portMAX_DELAY);void (*operation)(bool)=rpc_operation;
#if defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA)
  backup_request_t *backup=backup_request;
#endif
  xSemaphoreGive(mutex);bool activity_owned=true;
#if defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA)
  if(backup){
   activity_owned=false;backup->result=DSR_BUSY;
   if(dsr_maintenance_lease_valid(&backup->lease)&&release_host(true)){
    atomic_store(&backup_owner,true);backup->result=backup->root?dsr_backup_bonds_capture(backup->root):DSR_OK;atomic_store(&backup_owner,false);
    if(!dsr_maintenance_lease_valid(&backup->lease))backup->result=DSR_BUSY;
   }
  }else
#endif
  if(operation)operation(prepare_host());else scan_once(id);
  xSemaphoreTake(mutex,portMAX_DELAY);busy=false;rpc_operation=NULL;state.serial++;
#if defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA)
  backup_request=NULL;
#endif
  xSemaphoreGive(mutex);
  if(activity_owned)dsr_activity_end();
#if defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA)
  if(backup){atomic_store(&backup->done,true);backup_release(backup);}
#endif
  memory_log("idle");
 }
}
bool dsr_ble_manager_rpc(void (*operation)(bool))
{
 if(!mutex||!queue||!operation||!dsr_activity_begin())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);if(busy||atomic_load(&quarantined)){xSemaphoreGive(mutex);dsr_activity_end();return false;}busy=true;rpc_operation=operation;atomic_store(&cancelled,false);xSemaphoreGive(mutex);
 uint32_t token=0;if(xQueueSend(queue,&token,0)==pdTRUE)return true;
 xSemaphoreTake(mutex,portMAX_DELAY);busy=false;rpc_operation=NULL;xSemaphoreGive(mutex);dsr_activity_end();return false;
}
esp_err_t dsr_ble_init(void)
{
 if(mutex)return ESP_OK;
 mutex=xSemaphoreCreateMutex();queue=xQueueCreate(1,sizeof(uint32_t));
 if(!mutex||!queue){if(mutex)vSemaphoreDelete(mutex);if(queue)vQueueDelete(queue);mutex=NULL;queue=NULL;return ESP_ERR_NO_MEM;}
#if defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA)
 void *handle=&manager_handle;
#else
 void *handle=NULL;
#endif
 if(xTaskCreatePinnedToCore(scanner_task,"dsr_ble_scan",4096,NULL,2,handle,1)!=pdPASS){vSemaphoreDelete(mutex);vQueueDelete(queue);mutex=NULL;queue=NULL;return ESP_ERR_NO_MEM;}
 return ESP_OK;
}
bool dsr_ble_scan(void)
{
 if(!mutex||!queue||!dsr_activity_begin())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);
 if(busy||atomic_load(&quarantined)){xSemaphoreGive(mutex);dsr_activity_end();return false;}
 busy=true;dsr_ble_begin(&state,now_ms());uint32_t id=state.generation;atomic_store(&cancelled,false);xSemaphoreGive(mutex);
 if(xQueueSend(queue,&id,0)!=pdTRUE){finish(id,DSR_BLE_UNAVAILABLE);xSemaphoreTake(mutex,portMAX_DELAY);busy=false;xSemaphoreGive(mutex);dsr_activity_end();return false;}
 return true;
}
void dsr_ble_cancel(void){atomic_store(&cancelled,true);}
#if defined(ESP_PLATFORM) || defined(DSR_TEST_SYSTEM_DATA)
bool dsr_ble_maintenance_quiet(void)
{
 if(!mutex||!queue)return false;
 xSemaphoreTake(mutex,portMAX_DELAY);bool quiet=!busy&&!atomic_load(&quarantined);xSemaphoreGive(mutex);return quiet;
}
bool dsr_ble_backup_store_owner(void)
{return xTaskGetCurrentTaskHandle()==manager_handle&&atomic_load(&backup_owner)&&!atomic_load(&host_on)&&!initialized;}
static dsr_result_t maintenance_barrier(const dsr_maint_lease_t *lease,cJSON *manifest)
{
 if((manifest&&!cJSON_IsObject(manifest))||!mutex||!queue||!dsr_maintenance_lease_valid(lease))return DSR_BUSY;
 backup_request_t *r=calloc(1,sizeof(*r));if(!r)return DSR_FULL;
 if(manifest){r->root=cJSON_CreateObject();if(!r->root){free(r);return DSR_FULL;}}
 r->lease=*lease;r->result=DSR_BUSY;atomic_init(&r->refs,2);atomic_init(&r->done,false);
 xSemaphoreTake(mutex,portMAX_DELAY);
 bool accepted=!busy&&!atomic_load(&quarantined)&&dsr_maintenance_lease_valid(lease);
 if(accepted){busy=true;backup_request=r;uint32_t token=0;accepted=xQueueSend(queue,&token,0)==pdTRUE;if(!accepted){busy=false;backup_request=NULL;}}
 xSemaphoreGive(mutex);
 if(!accepted){backup_release(r);backup_release(r);return DSR_BUSY;}
 uint32_t started=now_ms();while(!atomic_load(&r->done)&&dsr_maintenance_lease_valid(lease)&&now_ms()-started<5000)vTaskDelay(pdMS_TO_TICKS(10));
 dsr_result_t result=DSR_BUSY;
 if(atomic_load(&r->done)&&dsr_maintenance_lease_valid(lease)){
  result=r->result;
  if(result==DSR_OK&&manifest){cJSON *bonds=cJSON_DetachItemFromObjectCaseSensitive(r->root,"bonds");if(!bonds||!dsr_backup_json_replace(manifest,"bonds",bonds))result=DSR_FULL;}
 }
 /* On timeout the manager retains its own reference. It can finish or reject
  * the stale lease later without accessing the caller's manifest or stack. */
 backup_release(r);return result;
}
dsr_result_t dsr_ble_maintenance_stop(const dsr_maint_lease_t *lease)
{return maintenance_barrier(lease,NULL);}
dsr_result_t dsr_ble_backup_capture(const dsr_maint_lease_t *lease,cJSON *manifest)
{return manifest?maintenance_barrier(lease,manifest):DSR_INVALID;}
#endif
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
 bool accepted=handoff_available&&!handoff_active&&!busy&&!atomic_load(&quarantined)&&handoff!=DSR_BLE_HANDOFF_REQUESTED&&serial==state.serial&&index<state.count;
 if(accepted){const dsr_ble_device_t *d=&state.devices[index];accepted=d->kind==DSR_BLE_SHELLY_RPC&&(d->evidence&DSR_BLE_RPC)&&now-d->seen_ms<=30000;
  if(accepted){handoff_candidate=*d;handoff_requested=now;handoff=DSR_BLE_HANDOFF_REQUESTED;}}
 xSemaphoreGive(mutex);dsr_activity_end();return accepted;
}
bool dsr_ble_handoff_take(dsr_ble_device_t *candidate,uint32_t now)
{
 if(!mutex||!candidate||!dsr_activity_begin())return false;
 xSemaphoreTake(mutex,portMAX_DELAY);expire_handoff(now);
 bool accepted=handoff_available&&!handoff_active&&!busy&&!atomic_load(&quarantined)&&handoff==DSR_BLE_HANDOFF_REQUESTED;
 if(accepted){*candidate=handoff_candidate;handoff_active=true;handoff=DSR_BLE_HANDOFF_ACTIVE;}
 xSemaphoreGive(mutex);dsr_activity_end();return accepted;
}
dsr_ble_handoff_t dsr_ble_handoff_status(uint32_t now)
{
 if(!mutex)return DSR_BLE_HANDOFF_UNAVAILABLE;
 xSemaphoreTake(mutex,portMAX_DELAY);expire_handoff(now);dsr_ble_handoff_t result=handoff;xSemaphoreGive(mutex);return result;
}
