#include "dsr_activity_internal.h"
#include <stdatomic.h>
#include <stdint.h>

/* One CAS word makes writer admission vs restart reservation indivisible.
 * Flags never overlap the count; rejected requests never close admission. */
#define PENDING UINT32_C(0x80000000)
#define DISPATCHED UINT32_C(0x40000000)
#define FAULT UINT32_C(0x20000000)
#define CONFIRMED UINT32_C(0x10000000)
#define COUNT UINT32_C(0x0fffffff)
static _Atomic uint32_t activity_state;
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
 do{if(old&(PENDING|DISPATCHED))return false;}
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
