#include "dsr_ota_power_internal.h"
#include "dsr_activity.h"
#include "esp_timer.h"
#include <stddef.h>

enum {IDLE,QUEUED,RUNNING,DONE,HELD,ABANDONED};
static struct {
 SemaphoreHandle_t lock,done,slot;
 dsr_ota_power_result_t result;
 unsigned char phase;
} power;
_Static_assert(sizeof(power)<=32,"bounded private OTA worker handoff");

esp_err_t dsr_ota_power_init(SemaphoreHandle_t slot)
{
 if(!slot||power.lock)return ESP_ERR_INVALID_STATE;
 SemaphoreHandle_t lock=xSemaphoreCreateMutex(),done=xSemaphoreCreateBinary();
 if(!lock||!done){if(lock)vSemaphoreDelete(lock);if(done)vSemaphoreDelete(done);return ESP_ERR_NO_MEM;}
 power.lock=lock;power.done=done;power.slot=slot;power.phase=IDLE;power.result=DSR_OTA_POWER_BUSY;return ESP_OK;
}
void dsr_ota_power_deinit(void)
{
 /* Only before a worker task has started, or in a stopped host fixture. */
 if(power.phase!=IDLE)return;
 if(power.lock)vSemaphoreDelete(power.lock);
 if(power.done)vSemaphoreDelete(power.done);
 power.lock=power.done=power.slot=NULL;
}
static void release_locked(void)
{
 power.phase=IDLE;power.result=DSR_OTA_POWER_BUSY;
 dsr_activity_end();xSemaphoreGive(power.slot);
}
dsr_result_t dsr_ota_power_acquire(dsr_ota_power_result_t *out)
{
 if(!out)return DSR_INVALID;
 *out=DSR_OTA_POWER_BUSY;
 if(!power.lock||xSemaphoreTake(power.slot,0)!=pdTRUE)return DSR_BUSY;
 if(!dsr_activity_begin()){xSemaphoreGive(power.slot);return DSR_BUSY;}
 xSemaphoreTake(power.lock,portMAX_DELAY);
 if(power.phase!=IDLE){xSemaphoreGive(power.lock);dsr_activity_end();return DSR_BUSY;} /* inconsistent slot: fail closed */
 (void)xSemaphoreTake(power.done,0);power.result=DSR_OTA_POWER_BUSY;power.phase=QUEUED;
 int64_t began=esp_timer_get_time();
 xSemaphoreGive(power.lock);
 bool completed=xSemaphoreTake(power.done,pdMS_TO_TICKS(1000))==pdTRUE;
 xSemaphoreTake(power.lock,portMAX_DELAY);
 int64_t now=esp_timer_get_time();
 completed=completed&&now>=began&&now-began<INT64_C(1000000);
 dsr_result_t result=DSR_BUSY;
 if(completed&&power.phase==DONE){*out=power.result;power.phase=HELD;result=DSR_OK;}
 else if(power.phase==RUNNING)power.phase=ABANDONED; /* worker owns eventual cleanup */
 else if(power.phase==QUEUED||power.phase==DONE){(void)xSemaphoreTake(power.done,0);release_locked();}
 xSemaphoreGive(power.lock);return result;
}
void dsr_ota_power_release(void)
{
 if(!power.lock)return;
 xSemaphoreTake(power.lock,portMAX_DELAY);
 if(power.phase==HELD)release_locked();
 xSemaphoreGive(power.lock);
}
bool dsr_ota_power_poll(void)
{
 if(!power.lock)return false;
 xSemaphoreTake(power.lock,portMAX_DELAY);
 if(power.phase!=QUEUED){xSemaphoreGive(power.lock);return false;}
 power.phase=RUNNING;xSemaphoreGive(power.lock);
 dsr_ota_power_result_t result=dsr_ota_power_read(); /* no mutex across ADC/NVS */
 xSemaphoreTake(power.lock,portMAX_DELAY);
 if(power.phase==ABANDONED)release_locked();
 else{power.result=result;power.phase=DONE;xSemaphoreGive(power.done);}
 xSemaphoreGive(power.lock);return true;
}
