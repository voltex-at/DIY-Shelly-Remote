/* Genuine C11 stage/activity races on host pthreads; no SDK or device proof. */
#include "dsr_ota_stage.h"
#include "dsr_activity_internal.h"
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
static atomic_uint inside,admitted,refused,restarts;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static void *writer(void *arg)
{
 (void)arg;
 for(unsigned i=0;i<20000;i++){
  dsr_ota_stage_t *stage=dsr_ota_stage_acquire();
  if(!stage){atomic_fetch_add(&refused,1);sched_yield();continue;}
  CHECK(atomic_fetch_add(&inside,1)==0);atomic_fetch_add(&admitted,1);
  CHECK(dsr_ota_stage_status()==DSR_OTA_STAGE_HELD&&!dsr_activity_restart_reserve());
  sched_yield();CHECK(atomic_fetch_sub(&inside,1)==1);dsr_ota_stage_release(stage);
 }
 return NULL;
}
static void *restart(void *arg)
{
 (void)arg;
 for(unsigned i=0;i<20000;i++){
  if(dsr_activity_restart_reserve()){
   CHECK(atomic_load(&inside)==0);atomic_fetch_add(&restarts,1);sched_yield();dsr_activity_restart_cancel();
  }else sched_yield();
 }
 return NULL;
}
int main(void)
{
 pthread_t threads[3];CHECK(!pthread_create(&threads[0],NULL,writer,NULL));CHECK(!pthread_create(&threads[1],NULL,writer,NULL));CHECK(!pthread_create(&threads[2],NULL,restart,NULL));
 for(unsigned i=0;i<3;i++)CHECK(!pthread_join(threads[i],NULL));
 CHECK(atomic_load(&admitted)>0&&atomic_load(&inside)==0&&dsr_ota_stage_status()==DSR_OTA_STAGE_IDLE);
 CHECK(dsr_activity_restart_reserve());CHECK(!dsr_ota_stage_acquire());dsr_activity_restart_cancel();
 dsr_ota_stage_t *stage=dsr_ota_stage_acquire();CHECK(stage);dsr_ota_stage_commit(stage);
 CHECK(dsr_ota_stage_status()==DSR_OTA_STAGE_STAGED&&!dsr_ota_stage_acquire());
 dsr_ota_stage_release(stage);dsr_ota_stage_release(NULL);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: actual OTA stage/activity pthread race: %u admits, %u busy, %u restart reservations; terminal latch with no leaked lease; host scheduling only\n",atomic_load(&admitted),atomic_load(&refused),atomic_load(&restarts));return 0;
}
