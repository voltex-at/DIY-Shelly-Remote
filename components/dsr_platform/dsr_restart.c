#include "dsr_restart.h"
#include "dsr_boot.h"
#include "dsr_activity_internal.h"
#include "esp_system.h"
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
dsr_restart_result_t dsr_restart_request(void)
{
 if(!dsr_boot_ready()||atomic_load(&restart_dispatcher.phase)!=DISPATCH_READY)return DSR_RESTART_UNAVAILABLE;
 if(!dsr_activity_restart_reserve())return DSR_RESTART_BUSY;
 if(xTaskNotifyGive(restart_dispatcher.task)!=pdPASS){
  dsr_activity_restart_cancel();return DSR_RESTART_UNAVAILABLE;
 }
 return DSR_RESTART_ACCEPTED;
}
