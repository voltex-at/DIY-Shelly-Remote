#ifndef DSR_UI_INTERNAL_H
#define DSR_UI_INTERNAL_H
#include "lvgl.h"
lv_obj_t *dsr_ui_label(lv_obj_t *parent, const char *text, int x, int y, int width);
lv_obj_t *dsr_ui_button(lv_obj_t *parent, const char *text, int x, int y, int width,
                       lv_event_cb_t callback, void *context);
lv_obj_t *dsr_ui_screen(const char *title, bool navigation);
void dsr_ui_home(lv_event_t *event);
void dsr_network_init(void);
void dsr_network_reset_view(void);
void dsr_network_home(lv_event_t *event);
void dsr_network_wifi(lv_event_t *event);
void dsr_network_device(lv_event_t *event);
#endif
