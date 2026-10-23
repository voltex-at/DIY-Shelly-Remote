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
#define DSR_SC_SPACE_SM 4
#define DSR_SC_MARGIN 12
#define DSR_SC_RADIUS 14
#define DSR_SC_TOUCH_MIN 44
#define DSR_SC_STATUS_H 44
#define DSR_SC_NAV_TOP 428
extern const lv_font_t dsr_roboto_11, dsr_roboto_12, dsr_roboto_14;
extern const lv_font_t dsr_roboto_16, dsr_roboto_18, dsr_roboto_28;
extern const lv_font_t dsr_roboto_22;

typedef enum { DSR_ICON_DASHBOARD, DSR_ICON_HOME, DSR_ICON_ENERGY, DSR_ICON_SETTINGS,
    DSR_ICON_PLUS, DSR_ICON_POWER, DSR_ICON_BACK, DSR_ICON_EDIT, DSR_ICON_INFO,
    DSR_ICON_PLUG, DSR_ICON_LIGHT, DSR_ICON_SOFA, DSR_ICON_BED, DSR_ICON_KITCHEN,
    DSR_ICON_BATH, DSR_ICON_WIFI, DSR_ICON_GRID, DSR_ICON_CHEVRON,
    DSR_ICON_UP, DSR_ICON_DOWN, DSR_ICON_CONTACT, DSR_ICON_COVER, DSR_ICON_BATTERY, DSR_ICON_SCENES,
    /* Selectable device icons (0.7.51) */
    DSR_ICON_OUTLET, DSR_ICON_FAN, DSR_ICON_THERMOSTAT, DSR_ICON_TV, DSR_ICON_BOILER, DSR_ICON_GARAGE,
    DSR_ICON_DOOR, DSR_ICON_FLOOR_LAMP, DSR_ICON_WALL_LAMP, DSR_ICON_EV, DSR_ICON_KETTLE,
    DSR_ICON_COFFEE, DSR_ICON_LAUNDRY, DSR_ICON_AC, DSR_ICON_SPEAKER, DSR_ICON_ROUTER, DSR_ICON_WINDOW_CLOSED, DSR_ICON_WINDOW_OPEN, DSR_ICON_DOOR_CLOSED, DSR_ICON_DOOR_OPEN, DSR_ICON_VALVE, DSR_ICON_BLUETOOTH, DSR_ICON_REFRESH,
    /* Cyber-only semantic roles; existing persistent IDs remain unchanged. */
    DSR_ICON_CPU, DSR_ICON_GIT_BRANCH, DSR_ICON_PALETTE, DSR_ICON_DISPLAY, DSR_ICON_SHIELD,
    DSR_ICON_NEXT, DSR_ICON_STOP, DSR_ICON_SAVE, DSR_ICON_TRASH, DSR_ICON_WARNING, DSR_ICON_ACTIVITY, DSR_ICON_MEMORY, DSR_ICON_POWER_SUPPLY, DSR_ICON_WINDOW_TILTED, DSR_ICON_WINDOW_NO_DATA, DSR_ICON_DOOR_NO_DATA,
    DSR_ICON_ILLUMINANCE, DSR_ICON_SIGNAL, DSR_ICON_CLOCK,
    DSR_ICON_ECO, DSR_ICON_LOCATION, DSR_ICON_TRV_VALVE,
    DSR_ICON_VOLTAGE, DSR_ICON_CURRENT, DSR_ICON_WATTAGE, DSR_ICON_PSRAM,
    DSR_ICON_GRAPHICS, DSR_ICON_FIRMWARE, DSR_ICON_HARDWARE, DSR_ICON_DIAGNOSTICS,
    DSR_ICON_RESTART, DSR_ICON_ONLINE, DSR_ICON_OFFLINE,
    DSR_ICON_TOUCH, DSR_ICON_USB, DSR_ICON_DOCUMENT,
    DSR_ICON_COVER_OPEN, DSR_ICON_COVER_CLOSE, DSR_ICON_COUNT } dsr_ui_icon_t;
lv_obj_t *dsr_sc_panel(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius);
lv_obj_t *dsr_sc_text(lv_obj_t *parent,const char *text,int x,int y,int w,const lv_font_t *font,uint32_t color);
lv_obj_t *dsr_sc_button(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius,lv_event_cb_t cb,void *context);
lv_obj_t *dsr_sc_icon(lv_obj_t *parent,dsr_ui_icon_t kind,int x,int y,int size,uint32_t color);
/* Reuse the same icon assets in dense cards without another LVGL object. */
void dsr_sc_draw_icon(lv_obj_t *owner,lv_layer_t *layer,dsr_ui_icon_t kind,const lv_area_t *area,uint32_t color);
/* Align inside the parent's actual content box, including border/padding.
 * Do not use (parent_size-icon_size)/2 as a child's relative position. */
lv_obj_t *dsr_sc_icon_centered(lv_obj_t *parent,dsr_ui_icon_t kind,int size,uint32_t color);
/* Non-interactive 28 px status module, shared by Cyber device pages. */
lv_obj_t *dsr_sc_badge(lv_obj_t *parent,dsr_ui_icon_t icon,const char *text,int x,int y,int width,uint32_t color);
void dsr_sc_icon_color(lv_obj_t *icon,uint32_t color);
/* Decorative/action accent. Explicit data, warning and disabled tints remain
 * caller-controlled; the low-level A8 recoloring API still accepts any RGB. */
uint32_t dsr_sc_icon_accent(void);
void dsr_sc_clock_tick(void);
/* Selectable device icons, 1-based as stored in the workspace "icon" field. */
#define DSR_SC_DEVICE_ICONS 20 /* == DSR_CONFIG_DEVICE_ICONS */
extern const dsr_ui_icon_t dsr_sc_device_icons[DSR_SC_DEVICE_ICONS];
void dsr_sc_scrollbar(lv_obj_t *scrollable); /* visible position indicator on the right */
/* Exactly three skins with their stable stored ids. Id 1 belonged to the
 * retired "midnight" skin (0.7.68-dev): it is never selectable or rendered and
 * any legacy 1 (style hint or old caller) is applied as graphite. */
enum {DSR_SC_STYLE_GRAPHITE=0,DSR_SC_STYLE_LIGHT=2,DSR_SC_STYLE_CYBER_BLUE=3};
unsigned dsr_sc_style_from_name(const char *name);
void dsr_sc_appearance(uint32_t accent,unsigned style,unsigned language);
unsigned dsr_sc_style(void);
const char *dsr_sc_user(const char *text);
const char *dsr_sc_name(const char *name);
const char *dsr_sc_tr(const char *text);
void dsr_sc_label_set_text(lv_obj_t *label,const char *text);
void dsr_sc_label_set_text_fmt(lv_obj_t *label,const char *format,...);
lv_color_t dsr_sc_color(uint32_t color);
lv_color_t dsr_sc_text_color(uint32_t color);
void dsr_sc_background(lv_obj_t *root);
unsigned dsr_sc_language(void);
void dsr_sc_header(lv_obj_t *root,const char *title,bool demo,bool wifi,lv_event_cb_t back,void *context);
void dsr_sc_header_back(lv_obj_t *root,lv_event_cb_t back,void *context);
void dsr_sc_border_color(lv_obj_t *obj,lv_color_t color,lv_style_selector_t selector);
void dsr_sc_border_width(lv_obj_t *obj,int32_t width,lv_style_selector_t selector);
const lv_font_t *dsr_sc_font(const lv_font_t *font);
void dsr_mobile_actions_shell(lv_obj_t *root,uint8_t kind);
void dsr_sc_blu_header(lv_obj_t *root,const char *title,bool demo,bool wifi,lv_event_cb_t back,void *context,lv_event_cb_t help);
/* Returns the label; its gold panel fits the translated text. */
lv_obj_t *dsr_sc_warning(lv_obj_t *parent,const char *text,int x,int y,int width,const lv_font_t *font);
void dsr_sc_navigation(lv_obj_t *root,unsigned current,unsigned enabled,lv_event_cb_t callback);
lv_obj_t *dsr_sc_chip(lv_obj_t *parent,const char *text,int x,int y,int max_width,uint32_t color);
/* Shelly Smart Control look (owner references 2026-10-18). */
#define DSR_SC_GLOW 0x2f7bff
/* Blue halo for an active control or card; off removes it. */
void dsr_sc_glow(lv_obj_t *obj,bool on);
/* White round button inside a dark ring; on = blue ring + halo. */
lv_obj_t *dsr_sc_power_button(lv_obj_t *parent,int x,int y,int size,bool on,bool enabled,dsr_ui_icon_t icon,uint32_t icon_color,lv_event_cb_t cb,void *context);
/* Dark pill with values separated by thin dividers ("0 W | 231.6 V"). Returns its width. */
int dsr_sc_segments(lv_obj_t *parent,const char *const *texts,unsigned count,int x,int y,int max_width,uint32_t color);
/* Arc drawn as colour segments interpolated through stops (0xRRGGBB). Angles: LVGL degrees. */
void dsr_sc_segments_update(lv_obj_t *pill,const char *const *texts,unsigned count,int max_width);
void dsr_sc_gradient_arc(lv_obj_t *parent,int cx,int cy,int radius,int width,int start,int span,const uint32_t *stops,unsigned stop_count,unsigned segments,bool enabled);
/* Built-in room photos (owner artwork, flash): 100x100 RGB565, content-addressed
 * like uploaded photos so the config keeps one 32-hex photo field. */
#define DSR_ROOM_PHOTO_SIZE 100
typedef struct {const char *id,*label;const uint8_t *pixels;} dsr_room_photo_t;
unsigned dsr_room_photo_count(void);
const dsr_room_photo_t *dsr_room_photo(unsigned index);
const dsr_room_photo_t *dsr_room_photo_find(const char *id);
/* Uploaded (320x180, PSRAM) or built-in (100x100, flash) photo as an LVGL source. */
bool dsr_ui_photo_source(const char *id,lv_image_dsc_t *out);
#endif
