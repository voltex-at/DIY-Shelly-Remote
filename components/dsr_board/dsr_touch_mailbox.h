#ifndef DSR_TOUCH_MAILBOX_H
#define DSR_TOUCH_MAILBOX_H
#include <stdbool.h>
#include <stdint.h>
/* Protected by the board's short critical section. Keep a complete short tap
 * when rendering spans both edges. Movement is coalesced, never queued at
 * 100 Hz. Overflow discards the gesture until release, rather than leaving a
 * stuck press or replaying a partial command. No LVGL calls on the I2C task. */
typedef struct {bool pressed;uint16_t x,y;bool cancelled;} dsr_touch_sample_t;
typedef struct {dsr_touch_sample_t latest,edges[8];uint32_t captured[8];uint8_t head,count;bool muted;} dsr_touch_mailbox_t;
static inline void dsr_touch_push(dsr_touch_mailbox_t *m,dsr_touch_sample_t s,uint32_t now)
{
 if(!s.pressed){s.x=m->latest.x;s.y=m->latest.y;}
 if(m->latest.cancelled){m->muted=s.pressed;return;}
 if(m->muted){if(!s.pressed)m->muted=false;return;}
 if(s.pressed!=m->latest.pressed){
  if(m->count==8){m->head=m->count=0;m->muted=s.pressed;m->latest.pressed=false;m->latest.cancelled=true;return;}
  m->edges[(m->head+m->count)%8]=s;m->captured[(m->head+m->count)%8]=now;m->count++;
 }
 m->latest=s;
}
static inline dsr_touch_sample_t dsr_touch_pop(dsr_touch_mailbox_t *m,uint32_t now)
{
 /* A tap made during boot or a long paused UI must not activate a later
  * screen. Expiry cancels LVGL's pressed target as well as both old edges. */
 if(m->count&&(uint32_t)(now-m->captured[m->head])>500){m->head=m->count=0;m->muted=m->latest.pressed;m->latest.pressed=false;m->latest.cancelled=true;}
 if(!m->count){dsr_touch_sample_t s=m->latest;m->latest.cancelled=false;return s;}
 dsr_touch_sample_t s=m->edges[m->head];m->head=(m->head+1)%8;m->count--;return s;
}
#endif
