/* Optional skin for the existing appearance system, pinned to LVGL 9.3.
 * Original brand bitmap and icon atlas remain in Flash and are reused.
 * Static shared styles initialize only on first use; no per-frame allocation,
 * image copies, blur or per-frame timers. No device/network code lives here. */
#include "dsr_ui_cyber.h"
#include "dsr_ui_style.h"
#include "dsr_board.h"
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
 lv_style_t text,contour,card,primary,pressed,disabled,focus,muted,track,knob,scroll,decor,action,glow,flat,backdrop;
 lv_style_t device,device_on,device_pressed,power_on,power_off,power_disabled,inset,warning,danger,press_edge;
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
 lv_style_set_text_color(s,lv_color_hex(DSR_CYBER_TEXT));lv_style_set_border_color(s,lv_color_hex(border==DSR_CYBER_DANGER||border==0xffc107?border:DSR_CYBER_CYAN));
 lv_style_set_border_width(s,DSR_CYBER_BORDER_WIDTH);lv_style_set_radius(s,18);lv_style_set_shadow_width(s,0);
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
 lv_style_set_text_font(&hud.text,dsr_sc_font(&dsr_roboto_14));
 lv_style_init(&hud.contour);lv_style_set_border_width(&hud.contour,DSR_CYBER_BORDER_WIDTH);lv_style_set_border_color(&hud.contour,lv_color_hex(DSR_CYBER_CYAN));
 surface(&hud.card,DSR_CYBER_ELEVATED,DSR_CYBER_BORDER);
 surface(&hud.primary,0x0065ce,DSR_CYBER_CYAN);
 lv_style_set_bg_grad_color(&hud.primary,lv_color_hex(0x003c8e));lv_style_set_bg_grad_dir(&hud.primary,LV_GRAD_DIR_VER);
 surface(&hud.pressed,0x0075d9,DSR_CYBER_CYAN);
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
 /* Shared chamfer surfaces keep padding and the caller's semantic fill. */
 lv_style_init(&hud.decor);lv_style_set_border_width(&hud.decor,DSR_CYBER_BORDER_WIDTH);
 lv_style_set_border_opa(&hud.decor,LV_OPA_TRANSP);lv_style_set_radius(&hud.decor,18);
 lv_style_set_border_color(&hud.decor,lv_color_hex(DSR_CYBER_CYAN));
 lv_style_set_bg_grad_color(&hud.decor,lv_color_hex(0x08182b));lv_style_set_bg_grad_dir(&hud.decor,LV_GRAD_DIR_VER);
 lv_style_init(&hud.action);lv_style_set_border_width(&hud.action,DSR_CYBER_BORDER_WIDTH);
 lv_style_set_border_opa(&hud.action,LV_OPA_TRANSP);lv_style_set_radius(&hud.action,18);
 lv_style_set_bg_color(&hud.action,lv_color_hex(0x0065ce));
 lv_style_set_border_color(&hud.action,lv_color_hex(DSR_CYBER_CYAN));
 lv_style_set_bg_grad_color(&hud.action,lv_color_hex(0x003688));lv_style_set_bg_grad_dir(&hud.action,LV_GRAD_DIR_VER);
 lv_style_init(&hud.glow);lv_style_set_outline_width(&hud.glow,1);lv_style_set_outline_pad(&hud.glow,1);
 lv_style_set_outline_color(&hud.glow,lv_color_hex(DSR_CYBER_CYAN));lv_style_set_outline_opa(&hud.glow,LV_OPA_COVER);
 lv_style_set_shadow_width(&hud.glow,0);
 lv_style_init(&hud.flat);lv_style_set_bg_color(&hud.flat,lv_color_hex(DSR_CYBER_ELEVATED));
 lv_style_set_bg_opa(&hud.flat,LV_OPA_COVER);lv_style_set_border_width(&hud.flat,0);lv_style_set_radius(&hud.flat,0);
 lv_style_init(&hud.backdrop);lv_style_set_bg_color(&hud.backdrop,lv_color_hex(DSR_CYBER_BG));
 lv_style_set_bg_opa(&hud.backdrop,LV_OPA_80);lv_style_set_border_width(&hud.backdrop,0);lv_style_set_radius(&hud.backdrop,0);
 /* Owner reference 2026-10: dark navy tiles in both states; the chamfered
  * double contour is drawn by frame_draw, the power button carries "on". */
 surface(&hud.device,0x0f243c,DSR_CYBER_CYAN);lv_style_set_bg_grad_color(&hud.device,lv_color_hex(0x09182b));lv_style_set_bg_grad_dir(&hud.device,LV_GRAD_DIR_VER);
 lv_style_set_border_width(&hud.device,0);lv_style_set_radius(&hud.device,18);
 surface(&hud.device_on,0x112c4b,DSR_CYBER_CYAN);lv_style_set_bg_grad_color(&hud.device_on,lv_color_hex(0x0a1b31));lv_style_set_bg_grad_dir(&hud.device_on,LV_GRAD_DIR_VER);
 lv_style_set_border_width(&hud.device_on,0);lv_style_set_radius(&hud.device_on,18);
 surface(&hud.device_pressed,0x17406a,DSR_CYBER_CYAN);lv_style_set_bg_grad_color(&hud.device_pressed,lv_color_hex(0x0c2442));lv_style_set_bg_grad_dir(&hud.device_pressed,LV_GRAD_DIR_VER);
 lv_style_set_border_width(&hud.device_pressed,0);
 surface(&hud.power_on,0x0079f2,DSR_CYBER_CYAN);lv_style_set_bg_grad_color(&hud.power_on,lv_color_hex(0x0039a3));lv_style_set_bg_grad_dir(&hud.power_on,LV_GRAD_DIR_VER);lv_style_set_border_width(&hud.power_on,DSR_CYBER_BORDER_WIDTH);lv_style_set_radius(&hud.power_on,LV_RADIUS_CIRCLE);
 surface(&hud.power_off,0x123453,0x3387a7);lv_style_set_bg_grad_color(&hud.power_off,lv_color_hex(0x06182c));lv_style_set_bg_grad_dir(&hud.power_off,LV_GRAD_DIR_VER);lv_style_set_border_width(&hud.power_off,DSR_CYBER_BORDER_WIDTH);lv_style_set_radius(&hud.power_off,LV_RADIUS_CIRCLE);
 surface(&hud.power_disabled,0x243a51,0x304a65);lv_style_set_radius(&hud.power_disabled,LV_RADIUS_CIRCLE);lv_style_set_border_width(&hud.power_disabled,DSR_CYBER_BORDER_WIDTH);
 surface(&hud.inset,0x061425,0x284b6c);lv_style_set_bg_grad_color(&hud.inset,lv_color_hex(0x102a43));lv_style_set_bg_grad_dir(&hud.inset,LV_GRAD_DIR_VER);
 surface(&hud.warning,0x3e3415,0xffc107);lv_style_set_bg_grad_color(&hud.warning,lv_color_hex(0x1d241f));lv_style_set_bg_grad_dir(&hud.warning,LV_GRAD_DIR_VER);
 surface(&hud.danger,0x7d1c29,DSR_CYBER_DANGER);lv_style_set_bg_grad_color(&hud.danger,lv_color_hex(0x350e1c));lv_style_set_bg_grad_dir(&hud.danger,LV_GRAD_DIR_VER);
 lv_style_init(&hud.press_edge);lv_style_set_bg_color(&hud.press_edge,lv_color_hex(0x0084ee));lv_style_set_bg_grad_color(&hud.press_edge,lv_color_hex(0x00479b));lv_style_set_bg_grad_dir(&hud.press_edge,LV_GRAD_DIR_VER);
 lv_style_set_border_width(&hud.press_edge,DSR_CYBER_BORDER_WIDTH);lv_style_set_border_color(&hud.press_edge,lv_color_hex(DSR_CYBER_CYAN));lv_style_set_transition(&hud.press_edge,&hud.no_transition);
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
  if(!lv_obj_check_type(o,&lv_obj_class)&&!lv_obj_check_type(o,&lv_list_text_class))dsr_ui_cyber_panel(o,false);
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
static void chamfer(lv_layer_t *layer,lv_area_t a,int cut,uint32_t color,lv_opa_t opa,int width);
void dsr_ui_cyber_draw_contour(lv_layer_t *layer,const lv_area_t *a,uint32_t color,lv_opa_t opacity)
{
 int w=lv_area_get_width(a),h=lv_area_get_height(a);if(w<8||h<8)return;
 int cut=h<40?4:9;if(cut>w/4)cut=w/4;
 chamfer(layer,(lv_area_t){a->x1+1,a->y1+1,a->x2-1,a->y2-1},cut,color,opacity,2);
}
static void panel_draw(lv_event_t *e)
{
 lv_obj_t *o=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(o,&a);lv_layer_t *layer=lv_event_get_layer(e);
 uintptr_t flags=(uintptr_t)lv_event_get_user_data(e);bool disabled=lv_obj_has_state(o,LV_STATE_DISABLED);
 uint32_t border=lv_color_to_u32(lv_obj_get_style_border_color(o,0))&0xffffff;
 bool danger=border==DSR_CYBER_DANGER;
 uint32_t contour=danger||border==0xffc107||border==0xf0a020||border==0xffc65b?border:DSR_CYBER_CYAN;
 int w=lv_area_get_width(&a),h=lv_area_get_height(&a);
 /* Dashboard chamfers also frame fields and actions, preserving semantic
  * fills and red/yellow feedback without allocating decorative objects. */
 if(w<8||h<8)return;
 int cut=h<40?4:9;if(cut>w/4)cut=w/4;
 dsr_ui_cyber_draw_contour(layer,&a,contour,disabled?LV_OPA_20:LV_OPA_80);
 if((w>=80||(flags&2))&&h>=40)
  chamfer(layer,(lv_area_t){a.x1+5,a.y1+5,a.x2-5,a.y2-5},cut-2,contour,disabled?LV_OPA_10:LV_OPA_40,1);
 if(w>=80&&h>=40){
  int span=h>70?26:16;if(span>w/4)span=w/4;
  for(int i=0;i<5;i++){int end=a.x2-12-i*6;line(layer,end-span,a.y1+6,end,a.y1+6+span,disabled?0x21344a:danger?0x9b2536:0x1b4568);}
 }
 if(w>=56&&h>=28)line(layer,a.x1+14,a.y2-4,a.x1+40,a.y2-4,disabled?0x36556d:(flags&1)?0x48d9ff:contour);
}
void dsr_ui_cyber_contour(lv_obj_t *o){init();lv_obj_add_style(o,&hud.contour,0);}
/* dsr_sc_panel falls back to local properties once its shared style table is
 * full. Local values outrank every style added later, so a skin surface that
 * owns fill/radius first drops them (power buttons stayed navy, not blue). */
static void own_surface(lv_obj_t *o,bool radius)
{lv_obj_remove_local_style_prop(o,LV_STYLE_BG_COLOR,0);if(radius)lv_obj_remove_local_style_prop(o,LV_STYLE_RADIUS,0);}
void dsr_ui_cyber_panel(lv_obj_t *o,bool primary)
{init();if(primary)own_surface(o,false);lv_obj_remove_local_style_prop(o,LV_STYLE_RADIUS,0);lv_obj_remove_style(o,&hud.action,0);lv_obj_remove_style(o,&hud.decor,0);lv_obj_remove_event_cb(o,panel_draw);lv_obj_add_style(o,primary?&hud.action:&hud.decor,0);lv_obj_add_event_cb(o,panel_draw,LV_EVENT_DRAW_MAIN_END,(void *)(uintptr_t)(primary?1:0));}
void dsr_ui_cyber_header_back(lv_obj_t *o)
{lv_obj_remove_event_cb(o,panel_draw);lv_obj_add_event_cb(o,panel_draw,LV_EVENT_DRAW_MAIN_END,(void *)(uintptr_t)2);}
void dsr_ui_cyber_glow(lv_obj_t *o,bool on)
{init();if(on)lv_obj_add_style(o,&hud.glow,0);else lv_obj_remove_style(o,&hud.glow,0);}
/* Chamfered contour of the owner's HUD reference: eight joined segments. */
static void chamfer(lv_layer_t *layer,lv_area_t a,int cut,uint32_t color,lv_opa_t opa,int width)
{
 lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=lv_color_hex(color);d.opa=opa;d.width=width;d.round_start=d.round_end=1;
 const lv_point_precise_t p[9]={{a.x1+cut,a.y1},{a.x2-cut,a.y1},{a.x2,a.y1+cut},{a.x2,a.y2-cut},{a.x2-cut,a.y2},{a.x1+cut,a.y2},{a.x1,a.y2-cut},{a.x1,a.y1+cut},{a.x1+cut,a.y1}};
 for(unsigned i=0;i<8;i++){d.p1=p[i];d.p2=p[i+1];lv_draw_line(layer,&d);}
}
/* flags: 1 inner contour, 2 status strip, 4 on, 8 warning, 16 diagonal inlay. */
static void frame_draw(lv_event_t *e)
{
 lv_obj_t *o=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(o,&a);lv_layer_t *layer=lv_event_get_layer(e);
 uintptr_t flags=(uintptr_t)lv_event_get_user_data(e);bool pressed=lv_obj_has_state(o,LV_STATE_PRESSED);
 int w=lv_area_get_width(&a),h=lv_area_get_height(&a);
 if(flags&16){int span=h>70?26:16;if(span>w/4)span=w/4;for(int i=0;i<5;i++){int end=a.x2-12-i*6;line(layer,end-span,a.y1+6,end,a.y1+6+span,0x1b4568);}}
 chamfer(layer,(lv_area_t){a.x1+1,a.y1+1,a.x2-1,a.y2-1},9,DSR_CYBER_CYAN,(flags&4)||pressed?LV_OPA_COVER:LV_OPA_80,2);
 if(flags&1)chamfer(layer,(lv_area_t){a.x1+5,a.y1+5,a.x2-5,a.y2-5},7,DSR_CYBER_CYAN,LV_OPA_40,1);
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_opa=LV_OPA_COVER;d.radius=1;
 if(flags&2){d.bg_color=lv_color_hex((flags&8)?0xffc107:(flags&4)?DSR_CYBER_CYAN:0x2f6f9c);lv_area_t strip={a.x1+4,a.y1+18,a.x1+5,a.y2-18};lv_draw_rect(layer,&d,&strip);}
 d.bg_color=lv_color_hex(DSR_CYBER_CYAN);lv_area_t bar={a.x1+14,a.y2-5,a.x1+42,a.y2-4};lv_draw_rect(layer,&d,&bar);
}
static void frame(lv_obj_t *o,bool on,uintptr_t flags)
{
 init();own_surface(o,true);lv_obj_remove_style(o,&hud.decor,0);lv_obj_remove_style(o,&hud.action,0);lv_obj_remove_style(o,&hud.contour,0);
 lv_obj_add_style(o,on?&hud.device_on:&hud.device,0);lv_obj_add_style(o,&hud.device_pressed,LV_STATE_PRESSED);dsr_ui_cyber_glow(o,false);
 lv_obj_remove_event_cb(o,panel_draw);lv_obj_remove_event_cb(o,frame_draw);lv_obj_add_event_cb(o,frame_draw,LV_EVENT_DRAW_MAIN_END,(void *)flags);
}
void dsr_ui_cyber_device(lv_obj_t *o,bool on,bool warning)
{frame(o,on,1|2|16|(on?4:0)|(warning?8:0));}
void dsr_ui_cyber_frame(lv_obj_t *o,bool inner)
{frame(o,false,inner?1|16:0);}
static void power_draw(lv_event_t *e)
{
 lv_obj_t *o=lv_event_get_target(e);if(lv_obj_has_state(o,LV_STATE_DISABLED))return;
 lv_area_t a;lv_obj_get_coords(o,&a);int side=lv_area_get_width(&a);
 lv_draw_arc_dsc_t arc;lv_draw_arc_dsc_init(&arc);arc.center=(lv_point_t){(a.x1+a.x2)/2,(a.y1+a.y2)/2};
 /* Reference ring: cyan rim, dark gap, then the blue face of an active button. */
 if(side>=40&&lv_obj_get_style_outline_width(o,0)>0){
  arc.radius=(uint16_t)(side/2-2);arc.width=3;arc.color=lv_color_hex(0x061631);arc.opa=LV_OPA_COVER;arc.start_angle=0;arc.end_angle=360;lv_draw_arc(lv_event_get_layer(e),&arc);
  arc.radius=(uint16_t)(side/2-5);arc.width=1;arc.color=lv_color_hex(0x5fc8ff);arc.opa=LV_OPA_70;lv_draw_arc(lv_event_get_layer(e),&arc);
  return;
 }
 arc.radius=(uint16_t)(side/2-5);arc.width=1;arc.color=lv_color_hex(DSR_CYBER_CYAN);arc.opa=LV_OPA_80;arc.start_angle=210;arc.end_angle=320;lv_draw_arc(lv_event_get_layer(e),&arc);
 arc.radius=(uint16_t)(side/2-2);arc.color=lv_color_hex(0x071a32);arc.opa=LV_OPA_60;arc.start_angle=25;arc.end_angle=155;lv_draw_arc(lv_event_get_layer(e),&arc);
}
void dsr_ui_cyber_power(lv_obj_t *o,bool on,bool enabled)
{
 init();own_surface(o,true);lv_obj_remove_style(o,&hud.contour,0);lv_obj_add_style(o,enabled?(on?&hud.power_on:&hud.power_off):&hud.power_disabled,0);
 dsr_ui_cyber_glow(o,on&&enabled);lv_obj_add_event_cb(o,power_draw,LV_EVENT_DRAW_MAIN_END,NULL);
}
void dsr_ui_cyber_inset(lv_obj_t *o){init();own_surface(o,true);lv_obj_remove_style(o,&hud.contour,0);lv_obj_add_style(o,&hud.inset,0);}
void dsr_ui_cyber_warning(lv_obj_t *o,bool danger){init();own_surface(o,false);lv_obj_add_style(o,danger?&hud.danger:&hud.warning,0);if(danger){lv_obj_add_style(o,&hud.danger,LV_STATE_PRESSED);lv_obj_set_style_bg_color(o,lv_color_hex(0xb42332),0);lv_obj_set_style_bg_grad_color(o,lv_color_hex(0x8f1d2b),0);lv_obj_set_style_border_color(o,lv_color_hex(DSR_CYBER_DANGER),0);lv_obj_set_style_bg_color(o,lv_color_hex(0x8f1d2b),LV_STATE_PRESSED);}}
/* Touch buttons share one state style; native fields retain their focus style. */
void dsr_ui_cyber_button(lv_obj_t *o){init();lv_obj_add_style(o,&hud.press_edge,LV_STATE_PRESSED);}
/* One retained state; pixels are read directly from the native Flash texture. */
extern const uint8_t _binary_cyber_garden_dusk_rgb565_start[];
static const lv_image_dsc_t shutter_garden={.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=152,.h=128,.stride=304},.data_size=38912,.data=_binary_cyber_garden_dusk_rgb565_start};
struct dsr_cyber_shutter {lv_obj_t *well;int open,confirmed;bool known,preview;};
typedef dsr_cyber_shutter_t shutter_t;
static void shutter_frame(void *data,int32_t open)
{if(dsr_board_display_sleeping())return;shutter_t *s=data;s->open=open;lv_obj_invalidate(s->well);}
static void shutter_delete(lv_event_t *e)
{shutter_t *s=lv_event_get_user_data(e);lv_anim_delete(s,shutter_frame);lv_free(s);}
static lv_area_t shutter_aperture(const shutter_t *s)
{lv_area_t a;lv_obj_get_coords(s->well,&a);return (lv_area_t){a.x1+12,a.y1+22,a.x2-12,a.y2-12};}
/* Same shutter mechanics in all skins; neutral metal in Graphite/Light,
 * their configured accent on the guide rails. The outdoor image stays fixed. */
static uint32_t shutter_color(uint32_t color)
{
 unsigned theme=dsr_sc_style();if(theme==DSR_SC_STYLE_CYBER_BLUE)return color;
 if(color==DSR_CYBER_CYAN||color==0x00bfe9||color==0x006a9d)return lv_color_to_u32(dsr_sc_text_color(DSR_SC_BLUE))&0xffffff;
 if(color==0xffc65b)return lv_color_to_u32(dsr_sc_text_color(0xffb466))&0xffffff;
 unsigned luma=(((color>>16)&255)*3+((color>>8)&255)*6+(color&255))/10;
 unsigned value=theme==DSR_SC_STYLE_LIGHT?224-luma/2:12+luma*3/4;
 return (value<<16)|(value<<8)|value;
}
static void shutter_line(lv_layer_t *layer,int x1,int y1,int x2,int y2,uint32_t color)
{line(layer,x1,y1,x2,y2,shutter_color(color));}
static void shutter_rect(lv_layer_t *layer,lv_area_t a,uint32_t fill,uint32_t border,int radius)
{
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(shutter_color(fill));d.bg_opa=LV_OPA_COVER;d.border_width=1;d.border_color=lv_color_hex(shutter_color(border));d.radius=radius;lv_draw_rect(layer,&d,&a);
}
static void cover_draw(lv_event_t *e)
{
 shutter_t *s=lv_event_get_user_data(e);lv_area_t a;lv_obj_get_coords(s->well,&a);lv_layer_t *layer=lv_event_get_layer(e);
 lv_area_t glass=shutter_aperture(s);int top=glass.y1,bottom=glass.y2,height=lv_area_get_height(&glass);
 int edge=top+height*(10000-s->open)/10000;
 /* Full outer frame, sill and independent recessed shutter guides. */
 shutter_rect(layer,(lv_area_t){a.x1+1,a.y1+1,a.x2-1,a.y2-1},0x04101c,0x00bfe9,5);
 shutter_rect(layer,(lv_area_t){a.x1+5,a.y1+5,a.x2-5,a.y2-4},0x1b3950,0x7198b0,3);
 shutter_rect(layer,(lv_area_t){glass.x1-2,top-2,glass.x2+2,bottom+2},0x081826,0x91c5dc,1);
 if(s->known){
  lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=&shutter_garden;
  image.scale_x=lv_area_get_width(&glass)*LV_SCALE_NONE/152;image.scale_y=height*LV_SCALE_NONE/128;
  lv_area_t source={glass.x1,glass.y1,glass.x1+151,glass.y1+127};lv_draw_image(layer,&image,&source);
 }else{
  shutter_rect(layer,glass,0x101f2c,0x38536a,0);
  for(int y=top+8;y<bottom;y+=18)shutter_line(layer,glass.x1+4,y,glass.x2-4,y,0x243b4c);
 }
 if((s->known||s->preview)&&edge>top){
  lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_opa=LV_OPA_COVER;d.bg_grad.dir=LV_GRAD_DIR_VER;d.bg_grad.stops_count=2;
  d.bg_grad.stops[0]=(lv_grad_stop_t){.color=lv_color_hex(shutter_color(0x098aca)),.opa=LV_OPA_COVER,.frac=0};
  d.bg_grad.stops[1]=(lv_grad_stop_t){.color=lv_color_hex(shutter_color(0x035491)),.opa=LV_OPA_COVER,.frac=255};
  /* Each seam follows the bottom rail; the curtain disappears into the top cassette. */
  for(int end=edge-1;end>=top;end-=10){int start=end-9;if(start<top)start=top;lv_area_t slat={glass.x1,start,glass.x2,end};lv_draw_rect(layer,&d,&slat);shutter_line(layer,glass.x1,end,glass.x2,end,0x032c4b);if(start>top)shutter_line(layer,glass.x1,start,glass.x2,start,0x51d6f4);}
 }
 /* Machined guide profiles remain above the texture and moving slats. */
 for(unsigned i=0;i<2;i++){int x=i?glass.x2+1:glass.x1-6;
  shutter_rect(layer,(lv_area_t){x,top-2,x+5,bottom+4},0x17374d,0x4a819e,1);
  shutter_line(layer,x+2,top,x+2,bottom+2,0x9bb9c8);shutter_line(layer,i?x+5:x,top,i?x+5:x,bottom+3,DSR_CYBER_CYAN);
 }
 if(s->known||s->preview){
  int rail=edge;if(rail>bottom)rail=bottom;if(rail<top+1)rail=top+1;
  shutter_rect(layer,(lv_area_t){glass.x1,rail-1,glass.x2,rail+2},0x72bdd7,s->preview?0xffc65b:DSR_CYBER_CYAN,1);
  shutter_line(layer,glass.x1+5,rail,glass.x2-5,rail,0xc0f5ff);
  if(s->preview&&s->known){int actual=top+height*(10000-s->confirmed)/10000;if(actual>bottom)actual=bottom;shutter_line(layer,glass.x2+2,actual,glass.x2+8,actual,0xffffff);}
 }
 shutter_rect(layer,(lv_area_t){a.x1+3,a.y1+3,a.x2-3,top-2},0x15314b,0x83bad0,4);
 shutter_line(layer,a.x1+8,a.y1+5,a.x2-8,a.y1+5,0xb8d9e6);shutter_line(layer,a.x1+7,top-4,a.x2-7,top-4,0x006a9d);
 shutter_rect(layer,(lv_area_t){a.x1+5,bottom+5,a.x2-5,a.y2-2},0x1f4359,0x72bed7,1);
 shutter_line(layer,a.x1+10,a.y2-3,a.x2-10,a.y2-3,DSR_CYBER_CYAN);
}
static int shutter_percent(int percent){return percent<0?0:percent>100?100:percent;}
dsr_cyber_shutter_t *dsr_ui_cyber_cover_transition(lv_obj_t *o,int from,int percent,bool known,bool moving)
{
 percent=shutter_percent(percent);if(from<0||from>100)from=percent;
 shutter_t *s=lv_malloc(sizeof(*s));if(!s)return NULL;*s=(shutter_t){.well=o,.open=(moving?from:percent)*100,.confirmed=percent*100,.known=known};
 lv_obj_add_event_cb(o,cover_draw,LV_EVENT_DRAW_MAIN_END,s);lv_obj_add_event_cb(o,shutter_delete,LV_EVENT_DELETE,s);
 if(known&&moving&&from!=percent){lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,s);lv_anim_set_exec_cb(&a,shutter_frame);lv_anim_set_values(&a,from*100,percent*100);lv_anim_set_duration(&a,300);lv_anim_set_path_cb(&a,lv_anim_path_linear);lv_anim_start(&a);}
 return s;
}
void dsr_ui_cyber_cover_update(dsr_cyber_shutter_t *s,int percent,bool known,bool moving)
{
 if(!s)return;
 int target=shutter_percent(percent)*100;
 if(!s->preview&&s->known==known&&s->confirmed==target&&moving)return;
 int from=s->open;bool changed=s->known!=known||s->preview,tween=s->known&&known&&moving&&from!=target;
 lv_anim_delete(s,shutter_frame);s->preview=false;s->known=known;s->confirmed=target;
 if(tween){lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,s);lv_anim_set_exec_cb(&a,shutter_frame);lv_anim_set_values(&a,from,target);lv_anim_set_duration(&a,300);lv_anim_set_path_cb(&a,lv_anim_path_linear);lv_anim_start(&a);}
 else if(changed||s->open!=target){s->open=target;lv_obj_invalidate(s->well);}
}
void dsr_ui_cyber_cover_preview(dsr_cyber_shutter_t *s,int target,int confirmed)
{if(!s)return;lv_anim_delete(s,shutter_frame);s->preview=true;s->open=shutter_percent(target)*100;s->confirmed=shutter_percent(confirmed)*100;lv_obj_invalidate(s->well);}
void dsr_ui_cyber_cover_restore(dsr_cyber_shutter_t *s,int confirmed)
{if(!s)return;lv_anim_delete(s,shutter_frame);s->preview=false;s->confirmed=s->open=shutter_percent(confirmed)*100;lv_obj_invalidate(s->well);}
int dsr_ui_cyber_cover_target(const dsr_cyber_shutter_t *s,int screen_y)
{if(!s)return 0;lv_area_t glass=shutter_aperture(s);int h=lv_area_get_height(&glass);return h>0?shutter_percent(((glass.y2+1-screen_y)*100+h/2)/h):0;}
void dsr_ui_cyber_cover(lv_obj_t *o,int percent,bool known)
{(void)dsr_ui_cyber_cover_transition(o,percent,percent,known,false);}
static void navigation_draw(lv_event_t *e)
{
 lv_obj_t *o=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(o,&a);unsigned selected=(unsigned)(uintptr_t)lv_event_get_user_data(e);
 lv_layer_t *layer=lv_event_get_layer(e);
 if(selected<4){
  lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(0x07416d);d.bg_opa=LV_OPA_COVER;
  d.bg_grad.dir=LV_GRAD_DIR_VER;d.bg_grad.stops_count=2;
  d.bg_grad.stops[0]=(lv_grad_stop_t){.color=lv_color_hex(0x0d4b76),.opa=LV_OPA_COVER,.frac=0};d.bg_grad.stops[1]=(lv_grad_stop_t){.color=lv_color_hex(0x081a31),.opa=LV_OPA_COVER,.frac=255};
  lv_area_t b={a.x1+(int)selected*64+1,a.y1+2,a.x1+(int)(selected+1)*64-1,a.y2};lv_draw_rect(layer,&d,&b);
 }
 for(int x=64;x<300;x+=64)line(layer,a.x1+x,a.y1+8,a.x1+x,a.y2-7,0x23486a);
 line(layer,a.x1,a.y1,a.x2,a.y1,0x1675aa);
}
void dsr_ui_cyber_navigation(lv_obj_t *o,unsigned current)
{lv_obj_add_event_cb(o,navigation_draw,LV_EVENT_DRAW_MAIN_END,(void *)(uintptr_t)current);}
static void background_draw(lv_event_t *e)
{
 lv_obj_t *o=lv_event_get_target(e);lv_area_t a;lv_obj_get_coords(o,&a);lv_layer_t *layer=lv_event_get_layer(e);
 line(layer,a.x1,a.y1+105,a.x1+100,a.y1+5,DSR_CYBER_BORDER);
 line(layer,a.x2-68,a.y2-5,a.x2,a.y2-73,DSR_CYBER_CYAN);
}
void dsr_ui_cyber_background(lv_obj_t *o,bool active)
{
 lv_obj_remove_event_cb(o,background_draw);
 if(active){
  /* Tint the original Flash artwork in the normal RGB565 draw path. No second
   * bitmap, decoder, PSRAM copy or intermediate render layer. */
  lv_obj_set_style_bg_image_recolor(o,lv_color_hex(0x001c36),0);lv_obj_set_style_bg_image_recolor_opa(o,LV_OPA_70,0);
  lv_obj_add_event_cb(o,background_draw,LV_EVENT_DRAW_MAIN_END,NULL);
 }
}
