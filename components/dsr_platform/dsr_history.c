#include "dsr_history_private.h"
#include "dsr_workspace.h"
#include "dsr_activity.h"
#include "esp_heap_caps.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
 dsr_history_model_t model;
 dsr_history_store_t store; /* Worker-owned, never accessed by UI/HTTP. */
 dsr_history_state_t state;
 uint32_t retry_ms;
 bool readable,persisted,storage_ready;
 dsr_clock_result_t clock_result;
} history_t;
_Static_assert(sizeof(history_t)<=DSR_HISTORY_MEMORY_MAX,"history PSRAM context budget");
static history_t *history;
static SemaphoreHandle_t lock;
enum {CLOCK_IDLE,CLOCK_PENDING,CLOCK_RUNNING};
static atomic_uint clock_state;
static atomic_bool clock_cancel;
static uint32_t now_ms(void){return (uint32_t)(esp_timer_get_time()/1000);}
static uint32_t utc_now(void){time_t t=time(NULL);return t>=(time_t)1735689600&&(uint64_t)t<=UINT32_MAX-86400u?(uint32_t)t:0;}
static bool take(void){return lock&&xSemaphoreTake(lock,portMAX_DELAY)==pdTRUE;}
static void give(void){xSemaphoreGive(lock);}
static void *memory(size_t size){return heap_caps_calloc(1,size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
esp_err_t dsr_history_init(void)
{
 if(history)return ESP_ERR_INVALID_STATE;
 history_t *next=memory(sizeof(*next));if(!next)return ESP_ERR_NO_MEM;
 SemaphoreHandle_t mutex=xSemaphoreCreateMutex();if(!mutex){free(next);return ESP_ERR_NO_MEM;}
 next->state=DSR_HISTORY_LOADING;next->clock_result=DSR_CLOCK_UNAVAILABLE;next->retry_ms=now_ms();history=next;lock=mutex;return ESP_OK;
}
static void status_locked(dsr_history_status_t *out)
{
 *out=(dsr_history_status_t){.state=history?history->state:DSR_HISTORY_UNAVAILABLE,.now_utc=utc_now(),.step_seconds=DSR_HISTORY_STEP,.retention_seconds=86400,
  .clock_pending=atomic_load(&clock_state)==CLOCK_PENDING,.clock_busy=atomic_load(&clock_state)==CLOCK_RUNNING,.clock_result=history?history->clock_result:DSR_CLOCK_UNAVAILABLE};
 out->clock_valid=out->now_utc!=0;
 if(!history||!history->readable)return;
 out->samples_without_time=history->model.without_time;out->durable=history->persisted&&history->state==DSR_HISTORY_READY;
 for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++){
  if(history->model.slots[i].current_utc&&!history->model.slots[i].sealed)out->durable=false;
  for(unsigned n=0;n<DSR_HISTORY_POINTS;n++)if(history->model.dirty[i][n])out->durable=false;
 }
}
void dsr_history_status(dsr_history_status_t *out)
{if(!out)return;if(!take()){*out=(dsr_history_status_t){.state=DSR_HISTORY_UNAVAILABLE};return;}status_locked(out);give();}
static dsr_history_key_t key(const dsr_connection_t *c)
{
 dsr_history_key_t k={.layout_id=c->layout_id,.generation=(uint8_t)c->device.generation,.channel=c->device.channel,.kind=c->device.endpoint_kind};
 memcpy(k.device_id,c->device.device_id,sizeof(k.device_id));return k;
}
void dsr_history_binding(const dsr_connection_t *connection,bool enabled)
{
 if(!connection||connection->device.endpoint_kind!=DSR_ENDPOINT_SWITCH||!connection->layout_id||!take())return;
 if(history->readable){dsr_history_key_t k=key(connection);(void)dsr_history_model_binding(&history->model,&k,enabled);}give();
}
void dsr_history_forget(const dsr_device_t *device)
{
 if(!device||device->endpoint_kind!=DSR_ENDPOINT_SWITCH||!take())return;
 dsr_connection_t c={.device=*device};dsr_history_key_t k=key(&c);
 if(history->readable)for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++)if(history->model.slots[i].used&&dsr_history_key_equal(&history->model.slots[i].key,&k))history->model.slots[i].active=false;
 give();
}
void dsr_history_record(const dsr_connection_t *connection,uint32_t request_id)
{
 if(!connection||connection->device.endpoint_kind!=DSR_ENDPOINT_SWITCH||!take())return;
 if(history->readable){dsr_history_key_t k=key(connection);dsr_history_model_record(&history->model,&k,&connection->device,&connection->telemetry,request_id,utc_now(),(uint64_t)(esp_timer_get_time()/1000));}give();
}
dsr_history_series_t *dsr_history_query(const dsr_history_key_t *expected,unsigned hours)
{
 if(!expected||expected->kind!=DSR_ENDPOINT_SWITCH||(hours!=1&&hours!=24))return NULL;
 dsr_history_series_t *out=memory(sizeof(*out));if(!out)return NULL;
 if(!take()){out->status.state=DSR_HISTORY_UNAVAILABLE;return out;}
 status_locked(&out->status);out->key=*expected;
 if(history->readable)for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++){
  const dsr_history_slot_t *s=&history->model.slots[i];
  if(s->used&&s->active&&dsr_history_key_equal(&s->key,expected)&&(!expected->layout_id||expected->layout_id==s->key.layout_id)){
   if(out->status.clock_valid)dsr_history_model_query(&history->model,i,hours,out->status.now_utc,out);
   out->key=s->key;give();return out;
  }
 }
 give();free(out);return NULL;
}
void dsr_history_series_free(dsr_history_series_t *series){free(series);}
bool dsr_history_service_due(uint32_t now)
{
 if(!take())return false;
 bool due=false,loading=false;
 if((int32_t)(now-history->retry_ms)>=0){
  if(history->state==DSR_HISTORY_LOADING)loading=true;
  else if(history->readable&&history->storage_ready&&history->state!=DSR_HISTORY_FUTURE){
   dsr_history_model_seal(&history->model,utc_now());
   for(unsigned i=0;i<DSR_HISTORY_SLOTS&&!due;i++)for(unsigned n=0;n<DSR_HISTORY_POINTS;n++)if(history->model.dirty[i][n]){due=true;break;}
  }
 }
 give();return due||loading;
}
static bool space(size_t length)
{size_t total=0,used=0;return esp_spiffs_info("dsrdata",&total,&used)==ESP_OK&&used<=total&&length<=total-used&&used+length+65536u<=total*3u/4u;}
static bool checkpoint(void)
{
 unsigned char *bytes=memory(DSR_HISTORY_SNAPSHOT_MAX);if(!bytes)return false;
 size_t length=0;if(take()){length=dsr_history_checkpoint(&history->model,bytes,DSR_HISTORY_SNAPSHOT_MAX,history->store.generation+1u);give();}
 bool ok=length&&space(length)&&dsr_history_store_checkpoint(&history->store,bytes,length,"/dsr");free(bytes);return ok;
}
void dsr_history_service(void)
{
 if(!take())return;
 bool loading=history->state==DSR_HISTORY_LOADING;if(!loading&&(!history->readable||!history->storage_ready)){give();return;}give();
 if(loading){
  /* ENOENT on an unmounted filesystem must not look like a fresh journal. */
  size_t total=0,used=0;
  bool ok=dsr_workspace_revision()!=0&&esp_spiffs_info("dsrdata",&total,&used)==ESP_OK&&total&&used<=total&&dsr_history_store_load(&history->store,&history->model,"/dsr");
  if(take()){history->readable=true;history->storage_ready=ok;history->persisted=ok&&history->store.generation!=0;history->state=ok?DSR_HISTORY_READY:history->store.future?DSR_HISTORY_FUTURE:DSR_HISTORY_IO;history->retry_ms=now_ms()+1000;give();}return;
 }
 bool ok=true;
 for(unsigned batch=0;batch<16&&ok;batch++){
  /* Rotate before selecting a dirty record: the immutable checkpoint may
   * observe a newer identity in a reused slot while file I/O is in flight. */
  if(!history->store.generation||history->store.length+120>DSR_HISTORY_FILE_MAX){ok=checkpoint();if(!ok)break;}
  dsr_history_key_t k={0};dsr_history_point_t p={0};unsigned slot=0,at=0;uint32_t version=0;
  if(!take())return;
  for(unsigned i=0;i<DSR_HISTORY_SLOTS&&!version;i++)for(unsigned n=0;n<DSR_HISTORY_POINTS;n++)if(history->model.dirty[i][n]){slot=i;at=n;version=history->model.dirty[i][n];k=history->model.slots[i].key;p=history->model.points[i][n];break;}
  give();if(!version)break;
  /* Defensive barrier even if a future producer mistakenly marks an empty
   * bucket dirty. Consume only its exact version; preserve the UTC gap. */
  if(!p.mask){
   if(take()){if(dsr_history_key_equal(&history->model.slots[slot].key,&k)&&history->model.dirty[slot][at]==version)history->model.dirty[slot][at]=0;give();}
   continue;
  }
  unsigned char bytes[120];size_t length=0;
  if(!history->store.known[slot]||!dsr_history_key_equal(&history->store.keys[slot],&k)||history->store.keys[slot].layout_id!=k.layout_id)length=dsr_history_encode_key(bytes,slot,&k);
  size_t point_length=dsr_history_encode_point(bytes+length,slot,&p);
  if(!point_length){ok=false;break;} /* Never append a key-only success. */
  length+=point_length;
  ok=space(length)&&dsr_history_store_append(&history->store,bytes,length,"/dsr");
  if(ok){
   history->store.keys[slot]=k;history->store.known[slot]=true;
   if(take()){if(dsr_history_key_equal(&history->model.slots[slot].key,&k)&&history->model.dirty[slot][at]==version)history->model.dirty[slot][at]=0;give();}
  }
 }
 if(take()){history->state=ok?DSR_HISTORY_READY:DSR_HISTORY_IO;history->persisted=ok;history->retry_ms=now_ms()+(ok?1000:5000);give();}
}
bool dsr_history_request_clock(void)
{
 if(!history||!dsr_wifi_connected()||!dsr_activity_begin())return false;
 unsigned expected=CLOCK_IDLE;bool accepted=atomic_compare_exchange_strong(&clock_state,&expected,CLOCK_PENDING);
 if(accepted)atomic_store(&clock_cancel,false);
 dsr_activity_end();return accepted;
}
bool dsr_history_clock_pending(void){return atomic_load(&clock_state)==CLOCK_PENDING;}
static bool cancelled(void *context){(void)context;return atomic_load(&clock_cancel);}
void dsr_history_clock_execute(void)
{
 unsigned expected=CLOCK_PENDING;if(!atomic_compare_exchange_strong(&clock_state,&expected,CLOCK_RUNNING))return;
 dsr_clock_result_t result=dsr_clock_sync(cancelled,NULL);
 if(take()){history->clock_result=result;give();}atomic_store(&clock_state,CLOCK_IDLE);
}
void dsr_history_cancel_clock(void){atomic_store(&clock_cancel,true);unsigned expected=CLOCK_PENDING;(void)atomic_compare_exchange_strong(&clock_state,&expected,CLOCK_IDLE);}
