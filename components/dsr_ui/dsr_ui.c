#include "dsr_ui.h"
#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_board.h"
#include "esp_app_desc.h"
#include "lvgl.h"
#include <stdint.h>

static lv_obj_t *touch_status;
static lv_obj_t *brightness_text;
static unsigned touches;
static uint8_t brightness = 70;
void dsr_ui_home(lv_event_t *event);
static void menu(lv_event_t *event);

lv_obj_t *dsr_ui_label(lv_obj_t *parent, const char *text, int x, int y, int width)
{
    lv_obj_t *obj = lv_label_create(parent);
    dsr_sc_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, width);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_color(obj, dsr_sc_color(DSR_SC_TEXT), 0);
    lv_obj_set_style_text_font(obj, &dsr_roboto_14, 0);
    return obj;
}

lv_obj_t *dsr_ui_button(lv_obj_t *parent, const char *text, int x, int y, int width,
                        lv_event_cb_t callback, void *context)
{
    lv_obj_t *obj = dsr_sc_button(parent,x,y,width,48,DSR_SC_CARD,DSR_SC_RADIUS,callback,context);
    lv_obj_set_style_text_color(obj,dsr_sc_color(DSR_SC_TEXT),0);
    lv_obj_t *caption = lv_label_create(obj);
    lv_obj_set_style_text_font(caption, &dsr_roboto_16, 0);
    lv_obj_set_width(caption,width-16);lv_label_set_long_mode(caption,LV_LABEL_LONG_MODE_WRAP);lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
    dsr_sc_label_set_text(caption, dsr_sc_tr(text));
    lv_obj_center(caption);
    return obj;
}

void dsr_ui_primary(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, dsr_sc_color(DSR_SC_BLUE), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x287ec7), LV_STATE_PRESSED);
}

void dsr_ui_textarea(lv_obj_t *obj)
{
    lv_obj_set_style_text_font(obj, &dsr_roboto_18, 0);
    lv_obj_set_style_bg_color(obj, dsr_sc_color(DSR_SC_PANEL), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xf0f4fb), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(0x98aac0), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_border_color(obj, dsr_sc_color(DSR_SC_EDGE), 0);
    lv_obj_set_style_border_color(obj, dsr_sc_color(DSR_SC_BLUE), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xf0f4fb), LV_PART_CURSOR);
}

static void shell_navigation(lv_event_t *event)
{
 unsigned index=(unsigned)(uintptr_t)lv_event_get_user_data(event);
 if(index<2){dsr_network_home(NULL);if(index==1)dsr_mobile_show(DSR_MOBILE_ROOMS,0);}
 else if(index==3)menu(NULL);else if(index==4)dsr_network_device(NULL);
}

lv_obj_t *dsr_ui_screen(const char *title, bool navigation)
{
    dsr_network_reset_view();
    dsr_ui_workspace_hide();
    dsr_mobile_hide();
    touch_status = brightness_text = NULL;
    lv_obj_t *root = lv_screen_active();
    lv_obj_clean(root);
    lv_obj_remove_style_all(root);
    lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
    lv_obj_set_style_bg_color(root, dsr_sc_color(DSR_SC_BG), 0);
    lv_obj_set_style_text_color(root, lv_color_hex(0xf0f4fb), 0);
    lv_obj_set_style_text_font(root, &dsr_roboto_14, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    dsr_sc_header(root,"Mój dom",false,dsr_wifi_connected(),dsr_ui_home,NULL);
    lv_obj_t *heading=dsr_sc_text(root,title,12,46,296,&dsr_roboto_18,DSR_SC_TEXT);
    lv_obj_set_height(heading,24);lv_label_set_long_mode(heading,LV_LABEL_LONG_MODE_DOTS);
    dsr_sc_panel(root,12,75,296,1,DSR_SC_BLUE,0);
    if(navigation)dsr_sc_navigation(root,3,27,shell_navigation);
    return root;
}

static void touch_hit(lv_event_t *event)
{
    unsigned corner = (unsigned)(uintptr_t)lv_event_get_user_data(event);
    touches++;
    dsr_sc_label_set_text_fmt(touch_status, "Corner %u detected\nTouch count: %u", corner, touches);
}

static void touch_test(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("", false);
    lv_obj_clean(root);
    dsr_ui_button(root, "1", 4, 4, 72, touch_hit, (void *)(uintptr_t)1);
    dsr_ui_button(root, "2", 244, 4, 72, touch_hit, (void *)(uintptr_t)2);
    dsr_ui_button(root, "3", 4, 428, 72, touch_hit, (void *)(uintptr_t)3);
    dsr_ui_button(root, "4", 244, 428, 72, touch_hit, (void *)(uintptr_t)4);
    touch_status = dsr_ui_label(root, "Tap all four corners.\nEach press should match\nthe corner number.", 30, 170, 260);
    lv_obj_set_style_text_align(touch_status, LV_TEXT_ALIGN_CENTER, 0);
    dsr_ui_button(root, "Back", 100, 325, 120, dsr_ui_home, NULL);
}

static void colors(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Display colors", true);
    dsr_ui_label(root, "Each label should match its color.", 16, 88, 288);
    const char *names[] = {"RED", "GREEN", "BLUE", "WHITE"};
    const uint32_t values[] = {0xff0000, 0x00ff00, 0x0000ff, 0xffffff};
    for (unsigned i = 0; i < 4; i++) {
        lv_obj_t *bar = lv_obj_create(root);
        lv_obj_set_pos(bar, 16, 133 + (int)i * 63);
        lv_obj_set_size(bar, 288, 48);
        lv_obj_set_style_bg_color(bar, lv_color_hex(values[i]), 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 8, 0);
        lv_obj_t *caption = lv_label_create(bar);
        dsr_sc_label_set_text(caption, names[i]);
        lv_obj_set_style_text_color(caption, lv_color_hex(i == 2 ? 0xffffff : 0x000000), 0);
        lv_obj_center(caption);
    }
}

static void change_brightness(lv_event_t *event)
{
    uint8_t percent = (uint8_t)lv_slider_get_value(lv_event_get_target(event));
    if (dsr_board_set_backlight(percent) == ESP_OK) {
        brightness = percent;
        dsr_sc_label_set_text_fmt(brightness_text, "Brightness: %u%%", (unsigned)brightness);
    }
}

static void sleep_display(lv_event_t *event) { (void)event; dsr_board_sleep_display(); }

static void firmware(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Firmware Update", true);
    dsr_ui_label(root, "Current version", 16, 100, 288);
    dsr_ui_label(root, esp_app_get_description()->version, 16, 133, 288);
    dsr_ui_label(root, "Lokalna aktualizacja OTA jest dostępna w sparowanym Web Panelu. Wymaga zgody na ekranie pilota.", 16, 200, 288);
}

static void settings(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Settings", true);
    brightness_text = dsr_ui_label(root, "", 16, 92, 288);
    dsr_sc_label_set_text_fmt(brightness_text, "Brightness: %u%%", (unsigned)brightness);
    lv_obj_t *slider = lv_slider_create(root);
    lv_obj_set_pos(slider, 24, 144);
    lv_obj_set_size(slider, 272, 18);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, change_brightness, LV_EVENT_VALUE_CHANGED, NULL);
    dsr_ui_button(root, "Screen off / touch to wake", 16, 204, 288, sleep_display, NULL);
    dsr_ui_button(root, "Firmware Update", 16, 270, 288, firmware, NULL);
    dsr_ui_button(root, "Diagnostyka konfiguracji", 16, 326, 288, dsr_ui_config_diagnostics, NULL);
    dsr_ui_label(root, "Ekran gaśnie po 60 sekundach.\nJasność jest ustawieniem tymczasowym.", 16, 384, 288);
}

void dsr_ui_open_settings(void) { menu(NULL); }

static void hardware(lv_event_t *event);
static void cover_preview(lv_event_t *event)
{(void)event;dsr_mobile_cover_preview();}
static void menu(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Menu", true);
    dsr_ui_button(root, "Wi-Fi setup", 16, 84, 288, dsr_network_wifi, NULL);
    dsr_ui_button(root, "Dodaj urządzenie Shelly", 16, 140, 288, dsr_network_device, NULL);
    dsr_ui_button(root, "Hardware tests", 16, 196, 288, hardware, NULL);
    dsr_ui_button(root, "Settings", 16, 252, 288, settings, NULL);
    dsr_ui_button(root, "Podgląd rolety • DEMO", 16, 308, 288, cover_preview, NULL);
    dsr_ui_button(root, "Bateria i zasilanie", 16, 364, 288, dsr_ui_battery_open, NULL);
}

void dsr_ui_home(lv_event_t *event) { dsr_network_home(event); }

static void hardware(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Hardware tests", true);
    dsr_ui_label(root, "Hardware test", 16, 100, 288);
    dsr_ui_label(root, "Display and touch test tools.\nCheck colors, corners and wake.", 16, 137, 288);
    lv_obj_t *status = dsr_ui_label(root, dsr_board_touch_available() ? "Touch controller: responding" : "Touch controller: NOT responding", 16, 200, 288);
    lv_obj_set_style_text_color(status, lv_color_hex(dsr_board_touch_available() ? 0x72dfb0 : 0xffb466), 0);
    dsr_ui_button(root, "Check touchscreen", 16, 252, 288, touch_test, NULL);
    dsr_ui_button(root, "Check display colors", 16, 316, 288, colors, NULL);
    dsr_ui_label(root, "ES3C35P / 320 x 480", 16, 390, 288);
}

void dsr_ui_init(void)
{
    lv_display_t *display=lv_display_get_default();
    lv_theme_t *theme=lv_theme_default_init(display,dsr_sc_color(DSR_SC_BLUE),dsr_sc_color(DSR_SC_BLUE),true,&dsr_roboto_14);
    lv_display_set_theme(display,theme);
    dsr_network_init();dsr_network_start_view();
}
