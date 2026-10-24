#ifndef DSR_UI_CYBER_H
#define DSR_UI_CYBER_H
#include "lvgl.h"
#define DSR_CYBER_BG 0x0a1324
#define DSR_CYBER_CARD 0x0e223a
#define DSR_CYBER_ELEVATED 0x173452
#define DSR_CYBER_BLUE 0x007bff
#define DSR_CYBER_CYAN 0x00e5ff
#define DSR_CYBER_TEXT 0xffffff
#define DSR_CYBER_MUTED 0xa9b7cc
#define DSR_CYBER_BORDER 0x315a83
#define DSR_CYBER_DANGER 0xff3b30
#define DSR_CYBER_BORDER_WIDTH 2
/* One contour standard; semantic warnings/errors keep their own color. */
void dsr_ui_cyber_contour(lv_obj_t *obj);
/* Same panel contour in an existing object's draw layer; no extra widget. */
void dsr_ui_cyber_draw_contour(lv_layer_t *layer,const lv_area_t *area,uint32_t color,lv_opa_t opacity);
uint32_t dsr_ui_cyber_color(uint32_t token);
lv_theme_t *dsr_ui_cyber_theme(lv_theme_t *base);
void dsr_ui_cyber_panel(lv_obj_t *panel,bool primary);
/* Double contour for the compact header control, without diagonal inlays. */
void dsr_ui_cyber_header_back(lv_obj_t *button);
void dsr_ui_cyber_glow(lv_obj_t *obj,bool on);
void dsr_ui_cyber_background(lv_obj_t *root,bool active);
void dsr_ui_cyber_device(lv_obj_t *card,bool on,bool warning);
/* Same chamfered tile contour without status strip (selector, room tiles). */
void dsr_ui_cyber_frame(lv_obj_t *obj,bool inner);
void dsr_ui_cyber_power(lv_obj_t *button,bool on,bool enabled);
void dsr_ui_cyber_inset(lv_obj_t *obj);
void dsr_ui_cyber_warning(lv_obj_t *obj,bool danger);
void dsr_ui_cyber_cover(lv_obj_t *well,int percent,bool known);
typedef struct dsr_cyber_shutter dsr_cyber_shutter_t;
/* Confirmed samples tween; a finger preview is immediate, with an actual-position marker. */
dsr_cyber_shutter_t *dsr_ui_cyber_cover_transition(lv_obj_t *well,int from,int percent,bool known,bool moving);
/* Update an existing drawing; never recreate the screen for a telemetry poll. */
void dsr_ui_cyber_cover_update(dsr_cyber_shutter_t *shutter,int percent,bool known,bool moving);
void dsr_ui_cyber_cover_preview(dsr_cyber_shutter_t *shutter,int target,int confirmed);
void dsr_ui_cyber_cover_restore(dsr_cyber_shutter_t *shutter,int confirmed);
int dsr_ui_cyber_cover_target(const dsr_cyber_shutter_t *shutter,int screen_y);
void dsr_ui_cyber_navigation(lv_obj_t *bar,unsigned current);
void dsr_ui_cyber_button(lv_obj_t *button);
#endif
