#ifndef DSR_TOUCH_LVGL_H
#define DSR_TOUCH_LVGL_H
#include "dsr_touch_mailbox.h"
#include "lvgl.h"
typedef struct {uint16_t x,y;bool dragged;} dsr_touch_lvgl_t;
static inline void dsr_touch_lvgl_sample(lv_indev_t *input,lv_indev_data_t *data,
 dsr_touch_lvgl_t *state,dsr_touch_sample_t sample,bool more)
{
 if(sample.cancelled)lv_indev_wait_release(input);
 state->dragged=sample.dragged;
 if(sample.pressed){state->x=sample.x;state->y=sample.y;}
 data->point=(lv_point_t){state->x,state->y};
 data->state=sample.pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;
 data->continue_reading=more;
}
/* LVGL sends input events before widget callbacks. A drag is never a tap,
 * including a horizontal swipe on a vertical list or a buffered out-and-back.
 * RELEASED still reaches native sliders/arcs so their drag commits work. */
static inline void dsr_touch_lvgl_guard(lv_event_t *event)
{
 dsr_touch_lvgl_t *state=lv_event_get_user_data(event);
 lv_event_code_t code=lv_event_get_code(event);
 if(state->dragged&&(code==LV_EVENT_CLICKED||code==LV_EVENT_SHORT_CLICKED))
  lv_indev_stop_processing(lv_event_get_target(event));
}
#endif
