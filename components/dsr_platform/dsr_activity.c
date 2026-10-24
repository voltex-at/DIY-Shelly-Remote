#include "dsr_activity_internal.h"
#include <stdatomic.h>
#include <stdint.h>

/* One CAS word makes writer admission vs restart reservation indivisible.
 * Flags never overlap the count; rejected requests never close admission. */
#define PENDING UINT32_C(0x80000000)
#define DISPATCHED UINT32_C(0x40000000)
#define FAULT UINT32_C(0x20000000)
#define CONFIRMED UINT32_C(0x10000000)
#define MAINTENANCE UINT32_C(0x08000000)
#define COMMIT UINT32_C(0x04000000)
#define COUNT UINT32_C(0x03ffffff)
static _Atomic uint32_t activity_state;
static uint64_t data_generation=1,next_reservation,active_reservation;
static bool generation_exhausted;
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
static portMUX_TYPE dataset_lock=portMUX_INITIALIZER_UNLOCKED;
static void lock(void){portENTER_CRITICAL(&dataset_lock);}
static void unlock(void){portEXIT_CRITICAL(&dataset_lock);}
#else
/* Host scheduling model only. Target uses IDF's interrupt-safe port critical
 * section; no task spins behind a preempted lower-priority owner on ESP32. */
static atomic_flag dataset_lock=ATOMIC_FLAG_INIT;
static void lock(void){while(atomic_flag_test_and_set_explicit(&dataset_lock,memory_order_acquire)){};}
static void unlock(void){atomic_flag_clear_explicit(&dataset_lock,memory_order_release);}
#endif
/* IDF's PSRAM workaround may route C11 atomics through its atomic32 wrappers.
 * Correct single-word atomicity is required, not an unproved lock-free ABI.
 * No application lock or I/O is held across these short atomic operations. */
_Static_assert(sizeof(_Atomic uint32_t)==4,"single-word restart admission state");
bool dsr_activity_begin(void)
{
 uint32_t old=atomic_load(&activity_state);
 do{
  if((old&~COUNT)||(old&COUNT)==COUNT)return false;
 }while(!atomic_compare_exchange_weak(&activity_state,&old,old+1));
 return true;
}
bool dsr_activity_stop_begin(void)
{
 uint32_t old=atomic_load(&activity_state);
 do{
  if((old&~(COUNT|MAINTENANCE))||(old&COUNT)==COUNT)return false;
 }while(!atomic_compare_exchange_weak(&activity_state,&old,(old&~MAINTENANCE)+1));
 return true;
}
void dsr_activity_end(void)
{
 uint32_t old=atomic_load(&activity_state);
 for(;;){
  /* Internal imbalance is fail-closed, never unsigned underflow or reopening
   * a pending restart. It does not silently reset another operation's count. */
  uint32_t next=(old&COUNT)?old-1:old|FAULT;
  if(atomic_compare_exchange_weak(&activity_state,&old,next))return;
 }
}
bool dsr_activity_restart_pending(void)
{return (atomic_load(&activity_state)&~COUNT)!=0;}
bool dsr_activity_restart_reserve(void)
{uint32_t expected=0;return atomic_compare_exchange_strong(&activity_state,&expected,PENDING);}
bool dsr_activity_restart_confirmed_reserve(void)
{
 uint32_t old=atomic_load(&activity_state);
 do{if(old&(PENDING|DISPATCHED|MAINTENANCE|COMMIT))return false;}
 while(!atomic_compare_exchange_weak(&activity_state,&old,old|PENDING|CONFIRMED));
 return true;
}
void dsr_activity_restart_cancel(void)
{
 uint32_t old=atomic_load(&activity_state);
 for(;;){
  if((old&(PENDING|DISPATCHED))!=PENDING)return;
  if(!(old&CONFIRMED)&&old!=PENDING)return;
  uint32_t next=old&~(PENDING|CONFIRMED);
  if(atomic_compare_exchange_weak(&activity_state,&old,next))return;
 }
}
bool dsr_activity_restart_dispatch(void)
{
 uint32_t old=atomic_load(&activity_state);
 do{
  if(old!=PENDING&&((old&(PENDING|CONFIRMED|DISPATCHED))!=(PENDING|CONFIRMED)))return false;
 }while(!atomic_compare_exchange_weak(&activity_state,&old,(old&~(PENDING|CONFIRMED))|DISPATCHED));
 return true;
}
void dsr_activity_data_changed(void)
{lock();if(data_generation==UINT64_MAX)generation_exhausted=true;else data_generation++;unlock();}
uint64_t dsr_activity_data_generation(void)
{lock();uint64_t g=generation_exhausted?0:data_generation;unlock();return g;}
bool dsr_activity_maintenance_reserved(void)
{return (atomic_load(&activity_state)&MAINTENANCE)!=0;}
bool dsr_activity_maintenance_committed(void)
{return (atomic_load(&activity_state)&COMMIT)!=0;}
bool dsr_activity_maintenance_reserve(uint64_t *reservation,uint64_t *generation)
{
 if(!reservation||!generation)return false;
 *reservation=*generation=0;lock();uint32_t expected=0;bool ok=false;
 if(!active_reservation&&!generation_exhausted&&next_reservation!=UINT64_MAX&&atomic_compare_exchange_strong(&activity_state,&expected,MAINTENANCE)){
  active_reservation=++next_reservation;*reservation=active_reservation;*generation=data_generation;ok=true;
 }
 unlock();return ok;
}
void dsr_activity_maintenance_release(uint64_t reservation)
{
 lock();if(reservation&&active_reservation==reservation&&!(atomic_load(&activity_state)&COMMIT)){active_reservation=0;uint32_t expected=MAINTENANCE;(void)atomic_compare_exchange_strong(&activity_state,&expected,0);}unlock();
}
bool dsr_activity_maintenance_valid(uint64_t reservation,uint64_t generation)
{
 lock();bool ok=reservation&&active_reservation==reservation&&!generation_exhausted&&generation==data_generation&&atomic_load(&activity_state)==MAINTENANCE;unlock();return ok;
}
static bool commit_exchange(uint64_t reservation,uint64_t generation,uint32_t from,uint32_t to)
{
 lock();uint32_t expected=from;bool ok=reservation&&active_reservation==reservation&&!generation_exhausted&&generation==data_generation&&atomic_compare_exchange_strong(&activity_state,&expected,to);unlock();return ok;
}
bool dsr_activity_maintenance_commit_begin(uint64_t reservation,uint64_t generation)
{return commit_exchange(reservation,generation,MAINTENANCE,COMMIT);}
bool dsr_activity_maintenance_commit_valid(uint64_t reservation,uint64_t generation)
{lock();bool ok=reservation&&active_reservation==reservation&&!generation_exhausted&&generation==data_generation&&atomic_load(&activity_state)==COMMIT;unlock();return ok;}
bool dsr_activity_maintenance_commit_abort(uint64_t reservation,uint64_t generation)
{return commit_exchange(reservation,generation,COMMIT,MAINTENANCE);}
bool dsr_activity_maintenance_restart_reserve(uint64_t reservation,uint64_t generation)
{return commit_exchange(reservation,generation,COMMIT,COMMIT|PENDING|CONFIRMED);}
