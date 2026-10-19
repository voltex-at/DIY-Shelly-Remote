#ifndef DSR_TOUCH_FILTER_H
#define DSR_TOUCH_FILTER_H
#include "dsr_touch_mailbox.h"
#define DSR_TOUCH_RELEASE_MS 30u
#define DSR_TOUCH_FAULT_MS 100u
typedef struct {
 dsr_touch_sample_t held;uint32_t last_valid,release_at;bool releasing,muted;
} dsr_touch_filter_t;
_Static_assert(sizeof(dsr_touch_filter_t)<=20,"bounded contact filter");
/* Press immediately. Confirm release with 30 ms of valid no-contact reports.
 * An I2C/coordinate fault is unknown contact, never a synthetic click. After
 * 100 ms without a valid report, cancel and require a clean release to rearm. */
static inline bool dsr_touch_filter(dsr_touch_filter_t *f,dsr_touch_sample_t raw,
 bool valid,uint32_t now,dsr_touch_sample_t *out)
{
 if(raw.cancelled){f->muted=true;f->releasing=false;f->held.pressed=false;*out=f->held;out->cancelled=true;return true;}
 if(!valid){
  f->releasing=false;
  if(f->held.pressed&&(uint32_t)(now-f->last_valid)>=DSR_TOUCH_FAULT_MS){
   f->held.pressed=false;f->muted=true;*out=f->held;out->cancelled=true;return true;
  }
  *out=f->held;return !f->muted;
 }
 f->last_valid=now;
 if(f->muted){
  if(raw.pressed){f->releasing=false;return false;}
  if(!f->releasing){f->releasing=true;f->release_at=now;}
  if((uint32_t)(now-f->release_at)<DSR_TOUCH_RELEASE_MS)return false;
  f->muted=f->releasing=false;*out=f->held;return true;
 }
 if(f->held.pressed&&!raw.pressed){
  if(!f->releasing){f->releasing=true;f->release_at=now;}
  if((uint32_t)(now-f->release_at)<DSR_TOUCH_RELEASE_MS){*out=f->held;return true;}
  raw.x=f->held.x;raw.y=f->held.y;
 }
 f->releasing=false;f->held=raw;*out=raw;return true;
}
#endif
