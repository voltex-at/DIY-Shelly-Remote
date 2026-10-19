#ifndef DSR_TOUCH_MAILBOX_H
#define DSR_TOUCH_MAILBOX_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Protected by the board's short critical section. Keep press, coalesced
 * movement and release in order even across a full repaint. A sticky movement
 * flag also survives an out-and-back swipe. Overflow/expiry cancel the whole
 * gesture. No allocation or LVGL calls on the I2C task. */
#define DSR_TOUCH_DRAG_LIMIT 10u
typedef struct {bool pressed;uint16_t x,y;bool cancelled,dragged;} dsr_touch_sample_t;
typedef struct {
 dsr_touch_sample_t latest,edges[8];uint32_t captured[8];
 uint16_t origin_x,origin_y;uint8_t head,count;bool muted,motion_tail;
} dsr_touch_mailbox_t;
_Static_assert(sizeof(dsr_touch_mailbox_t)<=112,"bounded touch mailbox");
static inline void dsr_touch_cancel(dsr_touch_mailbox_t *m,bool held)
{m->head=m->count=0;m->motion_tail=false;m->muted=held;m->latest.pressed=false;m->latest.cancelled=true;}
static inline void dsr_touch_push(dsr_touch_mailbox_t *m,dsr_touch_sample_t s,uint32_t now)
{
 if(!s.pressed){s.x=m->latest.x;s.y=m->latest.y;}
 if(s.cancelled){dsr_touch_cancel(m,s.pressed);return;}
 if(m->latest.cancelled){m->muted=s.pressed;return;}
 if(m->muted){if(!s.pressed)m->muted=false;return;}
 bool edge=s.pressed!=m->latest.pressed;
 if(edge&&s.pressed){m->origin_x=s.x;m->origin_y=s.y;s.dragged=false;}
 else{
  int dx=(int)s.x-m->origin_x,dy=(int)s.y-m->origin_y;
  s.dragged=m->latest.dragged||(s.pressed&&(dx>=(int)DSR_TOUCH_DRAG_LIMIT||dx<=-(int)DSR_TOUCH_DRAG_LIMIT||dy>=(int)DSR_TOUCH_DRAG_LIMIT||dy<=-(int)DSR_TOUCH_DRAG_LIMIT));
 }
 bool motion=s.pressed&&!edge&&(s.x!=m->latest.x||s.y!=m->latest.y||s.dragged!=m->latest.dragged);
 if(edge||motion){
  unsigned at=(m->head+m->count)%8;
  if(motion&&m->motion_tail)at=(at+7)%8;
  else{
   if(m->count==8){dsr_touch_cancel(m,s.pressed);return;}
   m->count++;
  }
  m->edges[at]=s;m->captured[at]=now;m->motion_tail=motion;
 }
 m->latest=s;
}
static inline dsr_touch_sample_t dsr_touch_pop_at(dsr_touch_mailbox_t *m,uint32_t now,uint32_t *captured)
{
 /* A tap made during boot or a long paused UI must not activate a later
  * screen. Expiry cancels LVGL's pressed target as well as both old edges. */
 if(m->count&&(uint32_t)(now-m->captured[m->head])>500)dsr_touch_cancel(m,m->latest.pressed);
 if(captured)*captured=m->count?m->captured[m->head]:now;
 if(!m->count){dsr_touch_sample_t s=m->latest;m->latest.cancelled=false;return s;}
 dsr_touch_sample_t s=m->edges[m->head];m->head=(m->head+1)%8;m->count--;if(!m->count)m->motion_tail=false;return s;
}
static inline dsr_touch_sample_t dsr_touch_pop(dsr_touch_mailbox_t *m,uint32_t now)
{return dsr_touch_pop_at(m,now,NULL);}
#endif
