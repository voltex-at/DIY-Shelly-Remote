#ifndef DSR_UI_STYLE_H
#define DSR_UI_STYLE_H
#include "lvgl.h"

/* Shared tokens, sampled by visual comparison to the manufacturer's published
 * app screenshots. Original implementation and original geometric icons. */
#define DSR_SC_BG 0x121a23
#define DSR_SC_PANEL 0x20272e
#define DSR_SC_CARD 0x45494f
#define DSR_SC_EDGE 0x5a6169
#define DSR_SC_TEXT 0xf5f6f7
#define DSR_SC_MUTED 0xc3c8ce
#define DSR_SC_BLUE 0x4c9bf5
#define DSR_SC_NAV 0x30373f
#define DSR_SC_OFF 0x81878e
extern const lv_font_t dsr_roboto_11, dsr_roboto_12, dsr_roboto_14;
extern const lv_font_t dsr_roboto_16, dsr_roboto_18, dsr_roboto_28;

typedef enum { DSR_ICON_DASHBOARD, DSR_ICON_HOME, DSR_ICON_ENERGY, DSR_ICON_SETTINGS,
    DSR_ICON_PLUS, DSR_ICON_POWER, DSR_ICON_BACK, DSR_ICON_EDIT, DSR_ICON_INFO,
    DSR_ICON_PLUG, DSR_ICON_LIGHT, DSR_ICON_SOFA, DSR_ICON_BED, DSR_ICON_KITCHEN,
    DSR_ICON_BATH, DSR_ICON_WIFI, DSR_ICON_GRID, DSR_ICON_CHEVRON,
    DSR_ICON_UP, DSR_ICON_DOWN, DSR_ICON_CONTACT, DSR_ICON_COVER, DSR_ICON_BATTERY, DSR_ICON_SCENES } dsr_ui_icon_t;
lv_obj_t *dsr_sc_panel(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius);
lv_obj_t *dsr_sc_text(lv_obj_t *parent,const char *text,int x,int y,int w,const lv_font_t *font,uint32_t color);
lv_obj_t *dsr_sc_button(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius,lv_event_cb_t cb,void *context);
lv_obj_t *dsr_sc_icon(lv_obj_t *parent,dsr_ui_icon_t kind,int x,int y,int size,uint32_t color);
/* Align inside the parent's actual content box, including border/padding.
 * Do not use (parent_size-icon_size)/2 as a child's relative position. */
lv_obj_t *dsr_sc_icon_centered(lv_obj_t *parent,dsr_ui_icon_t kind,int size,uint32_t color);
void dsr_sc_icon_color(lv_obj_t *icon,uint32_t color);
void dsr_sc_appearance(uint32_t accent,bool midnight,unsigned language);
const char *dsr_sc_tr(const char *text);
lv_color_t dsr_sc_color(uint32_t color);
#endif
