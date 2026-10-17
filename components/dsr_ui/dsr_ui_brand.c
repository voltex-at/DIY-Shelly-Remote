/* Owner brand artwork (2026-10-17): boot splash with logo and plain app
 * background, each in a dark and a light variant chosen by the UI style.
 * 320x480 RGB565 (tools/build_brand_images.py), embedded in Flash rodata;
 * drawn straight from Flash, no decoder, no PSRAM/heap copy. */
#include "dsr_ui_brand.h"
#include "dsr_ui_style.h"
#include "dsr_board.h"
#include "dsr_boot.h"
#include "dsr_style_hint.h"
#include "esp_app_desc.h"

extern const uint8_t _binary_brand_splash_dark_rgb565_start[];
extern const uint8_t _binary_brand_splash_light_rgb565_start[];
extern const uint8_t _binary_brand_background_dark_rgb565_start[];
extern const uint8_t _binary_brand_background_light_rgb565_start[];

#define BRAND_IMAGE(name,bytes) {.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=320,.h=480,.stride=640},.data_size=320u*480u*2u,.data=(bytes)}
static const lv_image_dsc_t splash_dark=BRAND_IMAGE(splash_dark,_binary_brand_splash_dark_rgb565_start);
static const lv_image_dsc_t splash_light=BRAND_IMAGE(splash_light,_binary_brand_splash_light_rgb565_start);
static const lv_image_dsc_t background_dark=BRAND_IMAGE(background_dark,_binary_brand_background_dark_rgb565_start);
static const lv_image_dsc_t background_light=BRAND_IMAGE(background_light,_binary_brand_background_light_rgb565_start);

const lv_image_dsc_t *dsr_ui_brand_background(void)
{return dsr_sc_style()==DSR_SC_STYLE_LIGHT?&background_light:&background_dark;}
const lv_image_dsc_t *dsr_ui_brand_splash(void)
{return dsr_sc_style()==DSR_SC_STYLE_LIGHT?&splash_light:&splash_dark;}

/* ---- Boot splash: from board init until the real UI is constructed. ---- */
static lv_obj_t *splash;
static bool splash_task;
void dsr_ui_splash_show(void)
{
 if(splash)return;
 lv_obj_t *screen=lv_screen_active();
 lv_obj_set_style_bg_color(screen,dsr_sc_color(DSR_SC_BG),0);lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
 splash=lv_image_create(screen);lv_image_set_src(splash,dsr_ui_brand_splash());lv_obj_set_pos(splash,0,0);
 lv_obj_t *version=lv_label_create(splash);
 lv_label_set_text(version,esp_app_get_description()->version);
 lv_obj_set_style_text_font(version,&dsr_roboto_12,0);lv_obj_set_style_text_color(version,dsr_sc_color(DSR_SC_MUTED),0);
 lv_obj_align(version,LV_ALIGN_BOTTOM_MID,0,-14);
}
void dsr_ui_splash_release(void)
{if(splash){lv_obj_delete(splash);splash=NULL;}}
bool dsr_ui_splash_visible(void){return splash!=NULL;}

void dsr_ui_splash_begin(void)
{
 /* The workspace (true style) is not mounted yet: use the NVS hint. */
 dsr_sc_appearance(DSR_SC_BLUE,dsr_style_hint_get(),1); /* EN: language not known yet */
 dsr_ui_splash_show();
 dsr_boot_provisioning_ui(true);
 splash_task=dsr_board_start_ui_task(dsr_boot_ui_gate)==ESP_OK;
 if(!splash_task){dsr_boot_provisioning_ui(false);return;} /* stay dark, as before */
 dsr_board_ui_hold(120); /* first frame before backlight: no panel garbage */
 (void)dsr_board_restore_backlight();
}
void dsr_ui_splash_end(void)
{
 dsr_boot_provisioning_ui(false);
 if(splash_task)dsr_board_ui_quiesce();
 /* Objects are deleted by dsr_ui_init on the now-idle LVGL owner. */
}
