/* Actual backend + atomics. FreeRTOS and ESP shutdown are a host MODEL. */
#define _POSIX_C_SOURCE 200809L
#include "dsr_restart.h"
#include "dsr_restart_internal.h"
#include "dsr_activity.h"
#include "dsr_activity_internal.h"
#include "freertos/task.h"
#include "hal/wdt_hal.h"
#include <pthread.h>
#include <setjmp.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static atomic_uint checks;
static unsigned creates,notifies,notifications,waits,delays,restarts,deletes;
static bool boot,create_fail,notify_fail;
static int task_marker;static TaskFunction_t task;
static unsigned guard_step;
static jmp_buf scheduler;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
bool dsr_boot_ready(void){return boot;}
BaseType_t xTaskCreate(TaskFunction_t fn,const char *name,uint32_t stack,void *arg,UBaseType_t priority,TaskHandle_t *out)
{creates++;CHECK(fn&&!strcmp(name,"dsr_restart")&&stack==4096&&!arg&&priority==1&&out);CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE);if(create_fail)return pdFALSE;task=fn;*out=&task_marker;return pdPASS;}
BaseType_t xTaskNotifyGive(TaskHandle_t handle)
{notifies++;CHECK(handle==&task_marker&&dsr_activity_restart_pending()&&!dsr_activity_begin()&&!restarts);if(notify_fail)return pdFALSE;notifications++;return pdPASS;}
uint32_t ulTaskNotifyTake(BaseType_t clear,TickType_t timeout)
{waits++;CHECK(clear==pdTRUE&&timeout==portMAX_DELAY);if(!notifications)longjmp(scheduler,1);unsigned n=notifications;notifications=0;return n;}
void vTaskDelay(TickType_t ticks){CHECK(ticks==500&&!restarts);delays++;}
uint32_t rtc_clk_slow_freq_get_hz(void){CHECK(guard_step==1);return 32768;}
void wdt_hal_init(wdt_hal_context_t *g,wdt_inst_t i,uint32_t p,bool intr){CHECK(!guard_step&&i==WDT_RWDT&&!p&&!intr);g->marker=42;guard_step++;}
void wdt_hal_write_protect_disable(wdt_hal_context_t *g){CHECK(g->marker==42&&guard_step++==1);}
void wdt_hal_config_stage(wdt_hal_context_t *g,wdt_stage_t stage,uint32_t ticks,wdt_stage_action_t action){CHECK(g->marker==42&&ticks==3*32768u);CHECK((guard_step==2&&stage==WDT_STAGE0&&action==WDT_STAGE_ACTION_RESET_SYSTEM)||(guard_step==3&&stage==WDT_STAGE1&&action==WDT_STAGE_ACTION_RESET_RTC));guard_step++;}
void wdt_hal_set_flashboot_en(wdt_hal_context_t *g,bool enabled){CHECK(g->marker==42&&enabled&&guard_step++==4);}
void wdt_hal_enable(wdt_hal_context_t *g){CHECK(g->marker==42&&guard_step++==5);}
void wdt_hal_feed(wdt_hal_context_t *g){CHECK(g->marker==42&&guard_step++==6);}
void wdt_hal_write_protect_enable(wdt_hal_context_t *g){CHECK(g->marker==42&&guard_step++==7);}
void esp_restart(void){CHECK(boot&&dsr_activity_restart_pending()&&!dsr_activity_begin()&&!restarts&&delays&&guard_step==8);CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE);restarts++;}
void vTaskDelete(TaskHandle_t handle){CHECK(!handle&&restarts==1&&!deletes);deletes++;}
static void dispatch(void){CHECK(task);if(!setjmp(scheduler))task(NULL);}
static pthread_barrier_t barrier;
static dsr_restart_result_t requests[8];
static void *requester(void *id){int r=pthread_barrier_wait(&barrier);if(r!=0&&r!=PTHREAD_BARRIER_SERIAL_THREAD)abort();requests[*(unsigned *)id]=dsr_restart_request();return NULL;}
int main(int argc,char **argv)
{
 CHECK(argc==2);boot=true;
 if(!strcmp(argv[1],"before-start")){
  CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE&&!notifies&&!restarts);
 }else if(!strcmp(argv[1],"create-fail")){
  create_fail=true;CHECK(dsr_restart_start()==ESP_ERR_NO_MEM);CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE);
  create_fail=false;CHECK(dsr_restart_start()==ESP_ERR_INVALID_STATE&&creates==1);CHECK(dsr_activity_begin());dsr_activity_end();
 }else{
  CHECK(dsr_restart_start()==ESP_OK&&creates==1&&task);CHECK(dsr_restart_start()==ESP_ERR_INVALID_STATE&&creates==1);
  if(!strcmp(argv[1],"boot")){
   boot=false;CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE&&!notifies);CHECK(dsr_activity_begin());dsr_activity_end();
  }else if(!strncmp(argv[1],"maintenance-",12)){
   uint64_t reservation,generation;CHECK(dsr_activity_maintenance_reserve(&reservation,&generation));CHECK(dsr_activity_maintenance_commit_begin(reservation,generation));
   CHECK(dsr_restart_request_maintenance(reservation+1,generation)==DSR_RESTART_BUSY&&!notifies);
   bool failed=!strcmp(argv[1],"maintenance-notify-fail");notify_fail=failed;
   dsr_restart_result_t result=dsr_restart_request_maintenance(reservation,generation);
   if(failed){CHECK(result==DSR_RESTART_UNAVAILABLE&&!notifications&&!dsr_activity_begin()&&!dsr_activity_stop_begin());notify_fail=false;CHECK(dsr_restart_request_maintenance(reservation,generation)==DSR_RESTART_ACCEPTED);}
   else CHECK(result==DSR_RESTART_ACCEPTED&&!restarts&&notifications==1);
   dispatch();CHECK(restarts==1&&deletes==1&&!dsr_activity_begin());dsr_activity_restart_cancel();CHECK(!dsr_activity_begin());
  }else if(!strcmp(argv[1],"confirmed-busy")){
   CHECK(dsr_activity_begin()&&dsr_activity_begin());
   CHECK(dsr_restart_request_confirmed()==DSR_RESTART_ACCEPTED&&!restarts);
   CHECK(!dsr_activity_begin()&&dsr_restart_request_confirmed()==DSR_RESTART_BUSY);
   dispatch();CHECK(restarts==1&&deletes==1);dsr_activity_end();dsr_activity_end();CHECK(!dsr_activity_begin());
  }else if(!strcmp(argv[1],"confirmed-notify-fail")){
   CHECK(dsr_activity_begin());notify_fail=true;CHECK(dsr_restart_request_confirmed()==DSR_RESTART_UNAVAILABLE&&!notifications&&!dsr_activity_restart_pending());
   CHECK(dsr_activity_begin());dsr_activity_end();CHECK(dsr_restart_request()==DSR_RESTART_BUSY);dsr_activity_end();
   notify_fail=false;CHECK(dsr_restart_request_confirmed()==DSR_RESTART_ACCEPTED);dispatch();CHECK(restarts==1);
  }else if(!strcmp(argv[1],"confirmed-boot-lost")){
   CHECK(dsr_activity_begin());CHECK(dsr_restart_request_confirmed()==DSR_RESTART_ACCEPTED);boot=false;dispatch();CHECK(!restarts&&!dsr_activity_restart_pending());
   CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE);dsr_activity_end();boot=true;CHECK(dsr_activity_begin());dsr_activity_end();
  }else if(!strcmp(argv[1],"busy")){
   CHECK(dsr_activity_begin());CHECK(dsr_restart_request()==DSR_RESTART_BUSY&&!notifies&&!dsr_activity_restart_pending());dsr_activity_end();
   CHECK(dsr_restart_request()==DSR_RESTART_ACCEPTED&&!restarts);dispatch();CHECK(restarts==1&&deletes==1);
  }else if(!strcmp(argv[1],"notify-fail")){
   notify_fail=true;CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE&&!notifications&&!dsr_activity_restart_pending());CHECK(dsr_activity_begin());dsr_activity_end();
   notify_fail=false;CHECK(dsr_restart_request()==DSR_RESTART_ACCEPTED);dispatch();CHECK(restarts==1&&notifies==2);
  }else if(!strcmp(argv[1],"boot-lost")){
   CHECK(dsr_restart_request()==DSR_RESTART_ACCEPTED);boot=false;dispatch();CHECK(!restarts&&!dsr_activity_restart_pending());CHECK(dsr_activity_begin());dsr_activity_end();
   boot=true;CHECK(dsr_restart_request()==DSR_RESTART_ACCEPTED);dispatch();CHECK(restarts==1&&delays==2);
  }else if(!strcmp(argv[1],"spurious")){
   notifications=1;dispatch();CHECK(!restarts&&!dsr_activity_restart_pending());CHECK(dsr_activity_begin());dsr_activity_end();
  }else if(!strcmp(argv[1],"once")){
   CHECK(dsr_restart_request()==DSR_RESTART_ACCEPTED&&!restarts&&notifications==1);CHECK(dsr_restart_request()==DSR_RESTART_BUSY&&notifies==1);
   dispatch();CHECK(restarts==1&&deletes==1);CHECK(dsr_restart_request()==DSR_RESTART_UNAVAILABLE&&!dsr_activity_begin());dsr_activity_restart_cancel();CHECK(!dsr_activity_begin());
  }else if(!strcmp(argv[1],"race")){
   pthread_t threads[8];unsigned ids[8];CHECK(!pthread_barrier_init(&barrier,NULL,8));
   for(unsigned i=0;i<8;i++){ids[i]=i;CHECK(!pthread_create(&threads[i],NULL,requester,&ids[i]));}
   for(unsigned i=0;i<8;i++)CHECK(!pthread_join(threads[i],NULL));
   unsigned accepted=0,busy=0;for(unsigned i=0;i<8;i++){accepted+=requests[i]==DSR_RESTART_ACCEPTED;busy+=requests[i]==DSR_RESTART_BUSY;}
   CHECK(accepted==1&&busy==7&&notifies==1&&!restarts);dispatch();CHECK(restarts==1&&deletes==1);CHECK(!pthread_barrier_destroy(&barrier));
  }else CHECK(false);
 }
 printf("PASS: %u actual restart backend checks (%s); FreeRTOS/shutdown MODEL\n",checks,argv[1]);return 0;
}
