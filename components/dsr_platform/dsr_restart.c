#include "dsr_restart.h"
#include "dsr_boot.h"
#include "dsr_activity_internal.h"
#include "esp_system.h"
#include "soc/rtc.h"
#include "hal/wdt_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <stddef.h>

#define RESTART_STACK_BYTES 4096u
_Static_assert(RESTART_STACK_BYTES==4096,"bounded restart task stack configuration");
enum {DISPATCH_NEW,DISPATCH_STARTING,DISPATCH_READY,DISPATCH_UNAVAILABLE};
typedef struct {atomic_int phase;TaskHandle_t task;} restart_dispatch_t;
static restart_dispatch_t restart_dispatcher;
_Static_assert(sizeof(restart_dispatch_t)<=16,"bounded restart-owned static state");
static void shutdown_guard(void)
{
 /* IDF 5.5.4 runs synchronous shutdown handlers (including Wi-Fi stop) before
  * its own no-OS reset watchdog is armed. Protect that earlier interval too.
  * This hardware clock needs no worker, LVGL tick or network callback. */
 wdt_hal_context_t guard;
 wdt_hal_init(&guard,WDT_RWDT,0,false);
 uint32_t ticks=rtc_clk_slow_freq_get_hz()*3u;
 wdt_hal_write_protect_disable(&guard);
 wdt_hal_config_stage(&guard,WDT_STAGE0,ticks,WDT_STAGE_ACTION_RESET_SYSTEM);
 wdt_hal_config_stage(&guard,WDT_STAGE1,ticks,WDT_STAGE_ACTION_RESET_RTC);
 wdt_hal_set_flashboot_en(&guard,true);
 wdt_hal_enable(&guard);wdt_hal_feed(&guard);
 wdt_hal_write_protect_enable(&guard);
}
static void dispatch_task(void *argument)
{
 (void)argument;
 for(;;){
  if(!ulTaskNotifyTake(pdTRUE,portMAX_DELAY))continue;
  vTaskDelay(pdMS_TO_TICKS(500));
  if(!dsr_boot_ready()){dsr_activity_restart_cancel();continue;}
  if(!dsr_activity_restart_dispatch())continue;
  /* Close request admission before the nonreturning SDK call. No application
   * mutex/critical section is held across shutdown handlers. */
  atomic_store(&restart_dispatcher.phase,DISPATCH_UNAVAILABLE);
  shutdown_guard();
  esp_restart();
  /* Host test stubs can return; never retry or reopen activity admission. */
  vTaskDelete(NULL);
  return;
 }
}
esp_err_t dsr_restart_start(void)
{
 int expected=DISPATCH_NEW;
 if(!atomic_compare_exchange_strong(&restart_dispatcher.phase,&expected,DISPATCH_STARTING))return ESP_ERR_INVALID_STATE;
 if(xTaskCreate(dispatch_task,"dsr_restart",RESTART_STACK_BYTES,NULL,1,&restart_dispatcher.task)!=pdPASS){
  restart_dispatcher.task=NULL;atomic_store(&restart_dispatcher.phase,DISPATCH_UNAVAILABLE);return ESP_ERR_NO_MEM;
 }
 atomic_store(&restart_dispatcher.phase,DISPATCH_READY);
 return ESP_OK;
}
static dsr_restart_result_t request(bool confirmed)
{
 if(!dsr_boot_ready()||atomic_load(&restart_dispatcher.phase)!=DISPATCH_READY)return DSR_RESTART_UNAVAILABLE;
 if(!(confirmed?dsr_activity_restart_confirmed_reserve():dsr_activity_restart_reserve()))return DSR_RESTART_BUSY;
 if(xTaskNotifyGive(restart_dispatcher.task)!=pdPASS){
  dsr_activity_restart_cancel();return DSR_RESTART_UNAVAILABLE;
 }
 return DSR_RESTART_ACCEPTED;
}
dsr_restart_result_t dsr_restart_request(void){return request(false);}
dsr_restart_result_t dsr_restart_request_confirmed(void){return request(true);}
