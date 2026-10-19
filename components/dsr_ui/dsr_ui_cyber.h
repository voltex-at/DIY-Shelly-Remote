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
uint32_t dsr_ui_cyber_color(uint32_t token);
lv_theme_t *dsr_ui_cyber_theme(lv_theme_t *base);
void dsr_ui_cyber_panel(lv_obj_t *panel,bool primary);
void dsr_ui_cyber_glow(lv_obj_t *obj,bool on);
void dsr_ui_cyber_background(lv_obj_t *root,bool active);
void dsr_ui_cyber_device(lv_obj_t *card,bool on,bool warning);
void dsr_ui_cyber_power(lv_obj_t *button,bool on,bool enabled);
void dsr_ui_cyber_inset(lv_obj_t *obj);
void dsr_ui_cyber_warning(lv_obj_t *obj,bool danger);
void dsr_ui_cyber_cover(lv_obj_t *well,int percent,bool known);
void dsr_ui_cyber_navigation(lv_obj_t *bar,unsigned current);
void dsr_ui_cyber_button(lv_obj_t *button);
#endif
