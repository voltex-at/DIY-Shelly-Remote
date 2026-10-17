#ifndef DSR_UI_STYLE_H
#define DSR_UI_STYLE_H
#include "lvgl.h"

/* Shared graphite design tokens. Material Symbols Rounded (Apache-2.0). */
#define DSR_SC_BG 0x101a22
#define DSR_SC_PANEL 0x20272b
#define DSR_SC_CARD 0x323638
#define DSR_SC_EDGE 0x535a5f
#define DSR_SC_TEXT 0xf5f6f7
#define DSR_SC_MUTED 0xc3c8ce
#define DSR_SC_BLUE 0x3999f6
#define DSR_SC_NAV 0x303638
#define DSR_SC_OFF 0x81878e
#define DSR_SC_GAP 8
#define DSR_SC_MARGIN 12
#define DSR_SC_RADIUS 14
#define DSR_SC_TOUCH_MIN 44
extern const lv_font_t dsr_roboto_11, dsr_roboto_12, dsr_roboto_14;
extern const lv_font_t dsr_roboto_16, dsr_roboto_18, dsr_roboto_28;
extern const lv_font_t dsr_roboto_22;

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
enum {DSR_SC_STYLE_GRAPHITE,DSR_SC_STYLE_MIDNIGHT,DSR_SC_STYLE_LIGHT};
/* style: graphite / midnight / light ("Jasny"); a bool midnight still maps 0/1. */
void dsr_sc_appearance(uint32_t accent,unsigned style,unsigned language);
unsigned dsr_sc_style(void);
const char *dsr_sc_tr(const char *text);
void dsr_sc_label_set_text(lv_obj_t *label,const char *text);
void dsr_sc_label_set_text_fmt(lv_obj_t *label,const char *format,...);
lv_color_t dsr_sc_color(uint32_t color);
lv_color_t dsr_sc_text_color(uint32_t color);
void dsr_sc_background(lv_obj_t *root);
unsigned dsr_sc_language(void);
void dsr_sc_header(lv_obj_t *root,const char *title,bool demo,bool wifi,lv_event_cb_t back,void *context);
void dsr_sc_navigation(lv_obj_t *root,unsigned current,unsigned enabled,lv_event_cb_t callback);
lv_obj_t *dsr_sc_chip(lv_obj_t *parent,const char *text,int x,int y,int max_width,uint32_t color);
#endif
