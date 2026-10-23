/* Real handoff/activity; pthread-backed FreeRTOS semaphores and ADC provider. */
#define _POSIX_C_SOURCE 200809L
#include "dsr_ota_power_internal.h"
#include "dsr_activity_internal.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

static _Atomic unsigned checks;
#define CHECK(x) do{atomic_fetch_add(&checks,1);if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
typedef struct {pthread_mutex_t lock;pthread_cond_t changed;bool mutex;unsigned count;} sem_t;
static unsigned creates,fail_create,live,reads;
static sem_t *ready,*broker_lock;
static pthread_t worker,client;
static bool thread_live,read_entered,release_read;
static pthread_mutex_t gate=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t progress=PTHREAD_COND_INITIALIZER;
/* 1 timeout while QUEUED; 2 during RUNNING; 3 DONE races the timeout. */
static unsigned timeout_mode;
static _Atomic int64_t clock_us=1000000;
int64_t esp_timer_get_time(void){return atomic_load(&clock_us);}
static dsr_ota_power_result_t sample=DSR_OTA_POWER_READY;
static struct timespec deadline(void){struct timespec t;CHECK(!clock_gettime(CLOCK_REALTIME,&t));t.tv_sec+=5;return t;}
static sem_t *create(bool mutex)
{
 if(++creates==fail_create)return NULL;
 sem_t *s=calloc(1,sizeof(*s));CHECK(s);CHECK(!pthread_mutex_init(&s->lock,NULL)&&!pthread_cond_init(&s->changed,NULL));s->mutex=mutex;live++;return s;
}
SemaphoreHandle_t xSemaphoreCreateMutex(void){broker_lock=create(true);return broker_lock;}
SemaphoreHandle_t xSemaphoreCreateBinary(void){return create(false);}
void vSemaphoreDelete(SemaphoreHandle_t h){sem_t *s=h;CHECK(s&&live);CHECK(!pthread_mutex_destroy(&s->lock)&&!pthread_cond_destroy(&s->changed));free(s);live--;}
static void *run(void *unused){(void)unused;CHECK(dsr_ota_power_poll());return NULL;}
static void join(void){if(thread_live){CHECK(!pthread_join(worker,NULL));thread_live=false;}}
int xSemaphoreTake(SemaphoreHandle_t h,TickType_t wait)
{
 sem_t *s=h;CHECK(s);
 if(s->mutex){CHECK(!pthread_mutex_lock(&s->lock));return pdTRUE;}
 if(wait==1000){
  CHECK(!thread_live);ready=s;
  if(timeout_mode==1)return pdFALSE;
  read_entered=false;release_read=timeout_mode!=2;
  CHECK(!pthread_create(&worker,NULL,run,NULL));thread_live=true;
  if(timeout_mode==2){pthread_mutex_lock(&gate);struct timespec d=deadline();while(!read_entered)CHECK(!pthread_cond_timedwait(&progress,&gate,&d));pthread_mutex_unlock(&gate);return pdFALSE;}
 }
 CHECK(!pthread_mutex_lock(&s->lock));struct timespec d=deadline();
 while(!s->count&&wait){int e=pthread_cond_timedwait(&s->changed,&s->lock,&d);CHECK(e==0||e==ETIMEDOUT);if(e==ETIMEDOUT)break;}
 bool ok=s->count!=0;
 if(wait==1000&&timeout_mode==3)ok=false; /* leave the late signal for cleanup */
 if(wait==1000&&timeout_mode==4)atomic_fetch_add(&clock_us,1000000); /* signalled, client scheduled too late */
 if(ok)s->count--;
 CHECK(!pthread_mutex_unlock(&s->lock));return ok?pdTRUE:pdFALSE;
}
int xSemaphoreGive(SemaphoreHandle_t h)
{
 sem_t *s=h;CHECK(s);
 if(s->mutex){CHECK(!pthread_mutex_unlock(&s->lock));return pdTRUE;}
 CHECK(!pthread_mutex_lock(&s->lock));CHECK(!s->count);s->count=1;CHECK(!pthread_cond_broadcast(&s->changed)&&!pthread_mutex_unlock(&s->lock));return pdTRUE;
}
dsr_ota_power_result_t dsr_ota_power_read(void)
{
 CHECK(!pthread_equal(pthread_self(),client));
 /* No handoff mutex may span ADC/profile I/O. */
 CHECK(!pthread_mutex_trylock(&broker_lock->lock));CHECK(!pthread_mutex_unlock(&broker_lock->lock));
 pthread_mutex_lock(&gate);reads++;read_entered=true;pthread_cond_broadcast(&progress);
 struct timespec d=deadline();while(!release_read)CHECK(!pthread_cond_timedwait(&progress,&gate,&d));
 dsr_ota_power_result_t result=sample;pthread_mutex_unlock(&gate);return result;
}
static void idle(SemaphoreHandle_t slot)
{CHECK(xSemaphoreTake(slot,0)==pdTRUE);CHECK(xSemaphoreGive(slot)==pdTRUE);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
int main(void)
{
 client=pthread_self();dsr_ota_power_result_t out;
 CHECK(dsr_ota_power_acquire(&out)==DSR_BUSY&&out==DSR_OTA_POWER_BUSY);CHECK(dsr_ota_power_acquire(NULL)==DSR_INVALID);
 SemaphoreHandle_t slot=xSemaphoreCreateBinary();xSemaphoreGive(slot);
 for(unsigned fault=1;fault<=2;fault++){fail_create=creates+fault;CHECK(dsr_ota_power_init(slot)==ESP_ERR_NO_MEM&&live==1);idle(slot);}
 fail_create=0;CHECK(dsr_ota_power_init(slot)==ESP_OK&&live==3);CHECK(dsr_ota_power_init(slot)==ESP_ERR_INVALID_STATE);
 CHECK(dsr_activity_restart_reserve());CHECK(dsr_ota_power_acquire(&out)==DSR_BUSY);dsr_activity_restart_cancel();idle(slot);
 CHECK(xSemaphoreTake(slot,0)==pdTRUE);CHECK(dsr_ota_power_acquire(&out)==DSR_BUSY);xSemaphoreGive(slot);
 for(unsigned result=DSR_OTA_POWER_READY;result<=DSR_OTA_POWER_LOW;result++){
  sample=(dsr_ota_power_result_t)result;unsigned before=reads;
  CHECK(dsr_ota_power_acquire(&out)==DSR_OK&&out==sample);join();CHECK(reads==before+1);
  CHECK(xSemaphoreTake(slot,0)==pdFALSE&&!dsr_activity_restart_reserve());
  CHECK(dsr_ota_power_acquire(&out)==DSR_BUSY&&out==DSR_OTA_POWER_BUSY);
  dsr_ota_power_release();dsr_ota_power_release();idle(slot);
 }
 for(unsigned fault=1;fault<=4;fault++){
  timeout_mode=fault;sample=DSR_OTA_POWER_LOW;unsigned before=reads;
  CHECK(dsr_ota_power_acquire(&out)==DSR_BUSY&&out==DSR_OTA_POWER_BUSY);
  if(fault==1){CHECK(!dsr_ota_power_poll()&&reads==before);}
  if(fault==2){
   CHECK(dsr_ota_power_acquire(&out)==DSR_BUSY&&!dsr_activity_restart_reserve());
   pthread_mutex_lock(&gate);release_read=true;pthread_cond_broadcast(&progress);pthread_mutex_unlock(&gate);
  }
  join();idle(slot);timeout_mode=0;sample=DSR_OTA_POWER_UNKNOWN;
  CHECK(dsr_ota_power_acquire(&out)==DSR_OK&&out==DSR_OTA_POWER_UNKNOWN);join();
  dsr_ota_power_release();idle(slot); /* a late LOW/signal cannot be reused */
 }
 CHECK(ready);dsr_ota_power_deinit();CHECK(live==1);vSemaphoreDelete(slot);CHECK(!live);
 printf("PASS: %u OTA power handoff checks; real phases/activity, worker/RTOS models, no ADC/Flash hardware\n",atomic_load(&checks));return 0;
}
