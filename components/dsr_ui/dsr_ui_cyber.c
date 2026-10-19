/* Optional skin for the existing appearance system, pinned to LVGL 9.3.
 * Original brand bitmap and icon atlas remain in Flash and are reused.
 * Static shared styles initialize only on first use; no per-frame allocation,
 * image copies, blur, timers or animations. No device/network code lives here. */
#include "dsr_ui_cyber.h"
#include "dsr_ui_style.h"
#include "src/themes/lv_theme_private.h" /* LVGL 9.3's concrete theme storage */

uint32_t dsr_ui_cyber_color(uint32_t c)
{
 static const uint32_t map[][2]={
  {DSR_SC_BG,DSR_CYBER_BG},{DSR_SC_PANEL,DSR_CYBER_CARD},{DSR_SC_CARD,DSR_CYBER_ELEVATED},
  {DSR_SC_EDGE,DSR_CYBER_BORDER},{DSR_SC_TEXT,DSR_CYBER_TEXT},{DSR_SC_MUTED,DSR_CYBER_MUTED},
  {DSR_SC_BLUE,DSR_CYBER_BLUE},{DSR_SC_NAV,0x0b1727},{DSR_SC_OFF,0x8199b3},{DSR_SC_GLOW,DSR_CYBER_CYAN},
  {0xf0f4fb,DSR_CYBER_TEXT},{0x98aac0,DSR_CYBER_MUTED},{0x45494d,DSR_CYBER_ELEVATED},
  {0x555b60,DSR_CYBER_BORDER},{0x3c4246,DSR_CYBER_ELEVATED},{0x1a2733,DSR_CYBER_CARD},
  {0x344f71,0x17456c},{0x30373d,0x182638},{0x232b33,DSR_CYBER_BG},{0x9db5d1,DSR_CYBER_MUTED},
  {0xa8bbd3,DSR_CYBER_MUTED},{0x41474b,DSR_CYBER_ELEVATED},{0x292d30,0x0b1626},
  {0x252a2e,DSR_CYBER_CARD},{0x244357,0x123d60},{0x29343b,0x0a1729},
  {0x303943,DSR_CYBER_ELEVATED},{0x25292b,DSR_CYBER_BG},{0x262a2d,DSR_CYBER_CARD},
  {0x414548,DSR_CYBER_ELEVATED},{0x60646b,DSR_CYBER_BORDER},{0xe8ecef,0xd7ebff},
  {0x818991,0x8199b3},{0xe8edf2,DSR_CYBER_TEXT},{0x2b2f33,DSR_CYBER_CARD},
  {0x23272a,0x091525},{0x3a3f44,DSR_CYBER_BORDER},{0x17191c,DSR_CYBER_BG},
  {0x1d2124,0x0b1727},{0x3d4246,DSR_CYBER_BORDER},{0xeceff1,0xc4e5ff},
  {0x5a6066,DSR_CYBER_BORDER},{0x8a9096,0x8199b3},{0x1d3f73,0x0d3557},
  {0x287ec7,0x075da5},{0x2379c8,0x004b93},
  /* Destructive actions keep a readable dark red fill, bright red edge. */
  {0xb42332,0x691a2b},{0x8f1d2b,0x8c2035},{0xe14d5b,DSR_CYBER_DANGER}};
 for(unsigned i=0;i<sizeof(map)/sizeof(map[0]);i++)if(c==map[i][0])return map[i][1];
 return c; /* Measured data, RGB swatches and semantic warnings stay intact. */
}

typedef struct {
 lv_theme_t theme;
 lv_style_transition_dsc_t no_transition;
 lv_style_t text,card,primary,pressed,disabled,focus,muted,track,knob,scroll,decor,action,glow,flat,backdrop;
 bool initialized;
} cyber_t;
#ifdef ESP_PLATFORM
#include "esp_attr.h"
EXT_RAM_BSS_ATTR
#endif
static cyber_t hud;

static void surface(lv_style_t *s,uint32_t bg,uint32_t border)
{
 lv_style_init(s);lv_style_set_bg_color(s,lv_color_hex(bg));lv_style_set_bg_opa(s,LV_OPA_COVER);
 lv_style_set_text_color(s,lv_color_hex(DSR_CYBER_TEXT));lv_style_set_border_color(s,lv_color_hex(border));
 lv_style_set_border_width(s,1);lv_style_set_radius(s,8);lv_style_set_shadow_width(s,0);
 lv_style_set_bg_grad_dir(s,LV_GRAD_DIR_NONE);lv_style_set_transition(s,&hud.no_transition);
 lv_style_set_color_filter_opa(s,LV_OPA_TRANSP);
}
static void init(void)
{
 if(hud.initialized)return;
 hud.initialized=true;
 /* 9.3 dereferences a present transition property: use an empty descriptor,
  * never NULL, when overriding the parent theme's animated states. */
 static const lv_style_prop_t none[]={0};
 lv_style_transition_dsc_init(&hud.no_transition,none,lv_anim_path_linear,0,0,NULL);
 lv_style_init(&hud.text);lv_style_set_text_color(&hud.text,lv_color_hex(DSR_CYBER_TEXT));
 lv_style_set_text_font(&hud.text,&dsr_roboto_14);
 surface(&hud.card,DSR_CYBER_ELEVATED,DSR_CYBER_BORDER);
 surface(&hud.primary,0x0065ba,DSR_CYBER_CYAN);
 surface(&hud.pressed,0x17456c,DSR_CYBER_CYAN);
 surface(&hud.disabled,0x182638,DSR_CYBER_BORDER);
 lv_style_set_text_color(&hud.disabled,lv_color_hex(0x8199b3));
 lv_style_init(&hud.focus);lv_style_set_border_color(&hud.focus,lv_color_hex(DSR_CYBER_CYAN));
 lv_style_set_outline_width(&hud.focus,1);lv_style_set_outline_color(&hud.focus,lv_color_hex(DSR_CYBER_CYAN));
 lv_style_init(&hud.muted);lv_style_set_text_color(&hud.muted,lv_color_hex(DSR_CYBER_MUTED));
 lv_style_init(&hud.track);lv_style_set_bg_color(&hud.track,lv_color_hex(DSR_CYBER_BORDER));
 lv_style_set_arc_color(&hud.track,lv_color_hex(DSR_CYBER_BORDER));
 lv_style_init(&hud.knob);lv_style_set_bg_color(&hud.knob,lv_color_hex(DSR_CYBER_CYAN));
 lv_style_set_arc_color(&hud.knob,lv_color_hex(DSR_CYBER_CYAN));lv_style_set_shadow_width(&hud.knob,0);
 lv_style_init(&hud.scroll);lv_style_set_bg_color(&hud.scroll,lv_color_hex(DSR_CYBER_CYAN));
 /* Decor does not change padding, radius or the caller's semantic fill. */
 lv_style_init(&hud.decor);lv_style_set_border_width(&hud.decor,1);
 lv_style_set_border_color(&hud.decor,lv_color_hex(DSR_CYBER_BORDER));
 lv_style_set_bg_grad_color(&hud.decor,lv_color_hex(0x091525));lv_style_set_bg_grad_dir(&hud.decor,LV_GRAD_DIR_VER);
 lv_style_init(&hud.action);lv_style_set_border_width(&hud.action,1);
 lv_style_set_bg_color(&hud.action,lv_color_hex(0x0065ba));
 lv_style_set_border_color(&hud.action,lv_color_hex(DSR_CYBER_CYAN));
 lv_style_set_bg_grad_color(&hud.action,lv_color_hex(0x004b93));lv_style_set_bg_grad_dir(&hud.action,LV_GRAD_DIR_VER);
 lv_style_init(&hud.glow);lv_style_set_outline_width(&hud.glow,1);lv_style_set_outline_pad(&hud.glow,1);
 lv_style_set_outline_color(&hud.glow,lv_color_hex(DSR_CYBER_CYAN));lv_style_set_outline_opa(&hud.glow,LV_OPA_60);
 lv_style_set_shadow_width(&hud.glow,0);
 lv_style_init(&hud.flat);lv_style_set_bg_color(&hud.flat,lv_color_hex(DSR_CYBER_ELEVATED));
 lv_style_set_bg_opa(&hud.flat,LV_OPA_COVER);lv_style_set_border_width(&hud.flat,0);lv_style_set_radius(&hud.flat,0);
 lv_style_init(&hud.backdrop);lv_style_set_bg_color(&hud.backdrop,lv_color_hex(DSR_CYBER_BG));
 lv_style_set_bg_opa(&hud.backdrop,LV_OPA_80);lv_style_set_border_width(&hud.backdrop,0);lv_style_set_radius(&hud.backdrop,0);
}
static void states(lv_obj_t *o,lv_part_t part)
{
 lv_obj_add_style(o,&hud.pressed,part|LV_STATE_PRESSED);
 lv_obj_add_style(o,&hud.primary,part|LV_STATE_CHECKED);
 lv_obj_add_style(o,&hud.disabled,part|LV_STATE_DISABLED);
 lv_obj_add_style(o,&hud.focus,part|LV_STATE_FOCUSED);
}
static void apply(lv_theme_t *theme,lv_obj_t *o)
{
 (void)theme;
 lv_obj_add_style(o,&hud.text,0);
 if(lv_obj_check_type(o,&lv_label_class)||lv_obj_check_type(o,&lv_image_class))return;
 lv_obj_add_style(o,&hud.scroll,LV_PART_SCROLLBAR);
 if(lv_obj_check_type(o,&lv_msgbox_backdrop_class)){lv_obj_add_style(o,&hud.backdrop,0);return;}
 if(lv_obj_check_type(o,&lv_msgbox_header_class)||lv_obj_check_type(o,&lv_msgbox_content_class)||lv_obj_check_type(o,&lv_msgbox_footer_class)){
  lv_obj_add_style(o,&hud.flat,0);return;
 }
 if(lv_obj_check_type(o,&lv_msgbox_header_button_class)||lv_obj_check_type(o,&lv_msgbox_footer_button_class)){
  lv_obj_add_style(o,&hud.primary,0);states(o,LV_PART_MAIN);return;
 }
 if(lv_obj_check_type(o,&lv_checkbox_class)){
  lv_obj_add_style(o,&hud.card,LV_PART_INDICATOR);states(o,LV_PART_INDICATOR);return;
 }
 if(lv_obj_check_type(o,&lv_slider_class)||lv_obj_check_type(o,&lv_bar_class)||lv_obj_check_type(o,&lv_switch_class)){
  lv_obj_add_style(o,&hud.track,0);lv_obj_add_style(o,&hud.knob,LV_PART_INDICATOR);
  lv_obj_add_style(o,&hud.knob,LV_PART_KNOB);return;
 }
 if(lv_obj_check_type(o,&lv_arc_class)||lv_obj_check_type(o,&lv_spinner_class)){
  lv_obj_add_style(o,&hud.track,0);lv_obj_add_style(o,&hud.knob,LV_PART_INDICATOR);return;
 }
 if(lv_obj_check_type(o,&lv_keyboard_class)||lv_obj_check_type(o,&lv_buttonmatrix_class)){
  lv_obj_add_style(o,&hud.card,0);lv_obj_add_style(o,&hud.card,LV_PART_ITEMS);states(o,LV_PART_ITEMS);return;
 }
 /* Labels and transparent structural children inherit text. Default-widget
  * surfaces include dropdown lists and msgbox header/content/footer classes. */
 if(lv_obj_check_type(o,&lv_obj_class)||lv_obj_check_type(o,&lv_dropdown_class)||
    lv_obj_check_type(o,&lv_dropdownlist_class)||lv_obj_check_type(o,&lv_textarea_class)||
    lv_obj_check_type(o,&lv_list_class)||lv_obj_check_type(o,&lv_list_button_class)||
    lv_obj_check_type(o,&lv_list_text_class)||lv_obj_check_type(o,&lv_msgbox_class)||
    lv_obj_check_type(o,&lv_msgbox_header_class)||lv_obj_check_type(o,&lv_msgbox_content_class)||
    lv_obj_check_type(o,&lv_msgbox_footer_class)||lv_obj_check_type(o,&lv_button_class)||
    lv_obj_check_type(o,&lv_msgbox_header_button_class)||lv_obj_check_type(o,&lv_msgbox_footer_button_class)||
    lv_obj_check_type(o,&lv_roller_class)){
  lv_obj_add_style(o,&hud.card,0);states(o,LV_PART_MAIN);
 }
 if(lv_obj_check_type(o,&lv_dropdownlist_class)||lv_obj_check_type(o,&lv_roller_class)){
  lv_obj_add_style(o,&hud.card,LV_PART_SELECTED);states(o,LV_PART_SELECTED);
 }
 if(lv_obj_check_type(o,&lv_textarea_class)){
  lv_obj_add_style(o,&hud.muted,LV_PART_TEXTAREA_PLACEHOLDER);
  lv_obj_add_style(o,&hud.knob,LV_PART_CURSOR);
 }
}
lv_theme_t *dsr_ui_cyber_theme(lv_theme_t *base)
{
 init();hud.theme=*base;lv_theme_set_parent(&hud.theme,base);lv_theme_set_apply_cb(&hud.theme,apply);
 return &hud.theme;
}
static void line(lv_layer_t *layer,int x1,int y1,int x2,int y2,uint32_t color)
{
 lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=lv_color_hex(color);d.width=1;
 d.p1=(lv_point_precise_t){x1,y1};d.p2=(lv_point_precise_t){x2,y2};lv_draw_line(layer,&d);
}
static void panel_draw(lv_event_t *e)
{
 lv_obj_t *o=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(o,&a);lv_layer_t *layer=lv_event_get_layer(e);
 /* Three metallic diagonal strokes, inside the rounded border. */
 for(int i=0;i<3;i++)line(layer,a.x2-44+i*7,a.y1+5,a.x2-22+i*7,a.y1+27,0x234463);
 line(layer,a.x1+12,a.y2-2,a.x1+38,a.y2-2,DSR_CYBER_CYAN);
}
void dsr_ui_cyber_panel(lv_obj_t *o,bool primary)
{init();lv_obj_remove_style(o,&hud.action,0);lv_obj_remove_style(o,&hud.decor,0);lv_obj_remove_event_cb(o,panel_draw);lv_obj_add_style(o,primary?&hud.action:&hud.decor,0);lv_obj_add_event_cb(o,panel_draw,LV_EVENT_DRAW_MAIN_END,NULL);}
void dsr_ui_cyber_glow(lv_obj_t *o,bool on)
{init();if(on)lv_obj_add_style(o,&hud.glow,0);else lv_obj_remove_style(o,&hud.glow,0);}
static void background_draw(lv_event_t *e)
{
 lv_obj_t *o=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(o,&a);lv_layer_t *layer=lv_event_get_layer(e);
 line(layer,a.x1,a.y1+105,a.x1+100,a.y1+5,DSR_CYBER_BORDER);
 line(layer,a.x2-68,a.y2-5,a.x2,a.y2-73,DSR_CYBER_CYAN);
}
void dsr_ui_cyber_background(lv_obj_t *o,bool active)
{lv_obj_remove_event_cb(o,background_draw);if(active)lv_obj_add_event_cb(o,background_draw,LV_EVENT_DRAW_MAIN_END,NULL);}
