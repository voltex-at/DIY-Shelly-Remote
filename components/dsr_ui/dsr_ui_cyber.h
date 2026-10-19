#ifndef DSR_UI_CYBER_H
#define DSR_UI_CYBER_H
#include "lvgl.h"
#define DSR_CYBER_BG 0x070d17
#define DSR_CYBER_CARD 0x101d30
#define DSR_CYBER_ELEVATED 0x15263b
#define DSR_CYBER_BLUE 0x008cff
#define DSR_CYBER_CYAN 0x00cfff
#define DSR_CYBER_TEXT 0xf2f7ff
#define DSR_CYBER_MUTED 0xa5bcd5
#define DSR_CYBER_BORDER 0x315a83
#define DSR_CYBER_DANGER 0xff455c
uint32_t dsr_ui_cyber_color(uint32_t token);
lv_theme_t *dsr_ui_cyber_theme(lv_theme_t *base);
void dsr_ui_cyber_panel(lv_obj_t *panel,bool primary);
void dsr_ui_cyber_glow(lv_obj_t *obj,bool on);
void dsr_ui_cyber_background(lv_obj_t *root,bool active);
#endif
