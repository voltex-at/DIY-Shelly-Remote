#include "dsr_ui.h"
#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_board.h"
#include "esp_app_desc.h"
#include "dsr_workspace.h"
#include "dsr_system_info.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static lv_obj_t *touch_status;
static unsigned touches;
static lv_obj_t *expiry_label;
static uint32_t issued_at;
static bool expiry_pair;
void dsr_ui_settings_hide(void){expiry_label=NULL;}
void dsr_ui_settings_tick(void)
{
 if(expiry_label&&(uint32_t)(lv_tick_get()-issued_at)>=120000){
  dsr_sc_label_set_text(expiry_label,expiry_pair?"Kod parowania wygasł.":"Zgoda na OTA wygasła.");expiry_label=NULL;
 }
}

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
 else if(index==3)dsr_ui_open_settings();else if(index==4)dsr_network_device(NULL);
}

lv_obj_t *dsr_ui_screen(const char *title, bool navigation)
{
    dsr_network_reset_view();
    dsr_ui_workspace_hide();
    dsr_mobile_hide();
    touch_status = NULL;
    dsr_ui_display_hide();
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

static void sleep_display(lv_event_t *event) { (void)event; dsr_board_sleep_display(); }

void dsr_ui_open_settings(void) { if(dsr_ui_settings_can_open())menu(NULL); }

static void hardware(lv_event_t *event);
static void cover_preview(lv_event_t *event)
{(void)event;dsr_mobile_cover_preview();}
/* Every global section starts with a fresh LCD body; no cached object pointers
 * survive Back/Home. Pair codes and tokens are never stored in this module. */
static void settings_back(lv_event_t *event){(void)event;dsr_ui_open_settings();}
static lv_obj_t *settings_body(const char *title)
{
 lv_obj_t *root=dsr_ui_screen(title,false);
 lv_obj_t *body=dsr_sc_panel(root,12,86,296,318,DSR_SC_BG,0);
 lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);
 dsr_ui_button(root,"Wstecz",12,420,296,settings_back,NULL);return body;
}
static lv_obj_t *row(lv_obj_t *body,const char *text,int y,lv_event_cb_t action)
{return dsr_ui_button(body,text,0,y,296,action,NULL);}
static void unavailable(lv_obj_t *body,const char *text,int y)
{lv_obj_t *b=row(body,text,y,NULL);lv_obj_add_state(b,LV_STATE_DISABLED);}
void dsr_ui_settings_network(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Sieć i połączenia");dsr_wifi_info_t wifi;dsr_wifi_get_info(&wifi);
 dsr_sc_text(body,wifi.connected?"Wi-Fi: połączono":"Wi-Fi: brak połączenia",4,4,284,&dsr_roboto_16,DSR_SC_TEXT);
 lv_obj_t *ssid=dsr_sc_text(body,wifi.ssid[0]?wifi.ssid:"—",4,34,284,&dsr_roboto_14,DSR_SC_MUTED);
 lv_obj_set_height(ssid,20);lv_label_set_long_mode(ssid,LV_LABEL_LONG_MODE_DOTS);
 char text[48];snprintf(text,sizeof(text),"IPv4: %s",wifi.connected&&wifi.ip[0]?wifi.ip:"—");
 dsr_sc_text(body,text,4,62,284,&dsr_roboto_14,DSR_SC_MUTED);
 row(body,"Wi-Fi setup",98,dsr_network_wifi);row(body,"Bluetooth LE • tylko skan",156,dsr_ui_ble_settings);
 dsr_sc_text(body,"Parowanie BLE i onboarding AP nie są wdrożone.",4,218,284,&dsr_roboto_12,DSR_SC_MUTED);
 unavailable(body,"Wi-Fi AP • niedostępne",266);row(body,"Dodaj urządzenie Shelly",324,dsr_network_device);
 row(body,"Odśwież",382,dsr_ui_settings_network);
}
static void web_panel(lv_event_t *event);
static void pair_browser(lv_event_t *event)
{
 (void)event;char code[17]={0};if(dsr_panel_ready())dsr_panel_pair_begin(code);
 lv_obj_t *body=settings_body("Parowanie Web Panelu");
 if(!code[0]){dsr_sc_text(body,"Nie utworzono kodu. HTTPS niedostępny lub zajęty.",4,12,284,&dsr_roboto_18,DSR_SC_TEXT);row(body,"Panel WWW / bezpieczeństwo",110,web_panel);return;}
 dsr_sc_text(body,"Jednorazowy kod • 120 s • 5 prób",4,8,284,&dsr_roboto_14,DSR_SC_MUTED);
 expiry_label=dsr_sc_text(body,code,4,54,284,&dsr_roboto_18,DSR_SC_TEXT);issued_at=lv_tick_get();expiry_pair=true;memset(code,0,sizeof(code));
 dsr_sc_text(body,"Sprawdź odcisk certyfikatu w przeglądarce przed wpisaniem kodu.",4,112,284,&dsr_roboto_14,DSR_SC_MUTED);
 row(body,"Panel WWW / bezpieczeństwo",214,web_panel);
}
static void web_panel(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Panel WWW / bezpieczeństwo");
 dsr_wifi_info_t wifi;dsr_wifi_get_info(&wifi);dsr_config_health_t health;dsr_config_health_snapshot(&health);
 dsr_sc_text(body,dsr_panel_ready()?"HTTPS uruchomiony":"HTTPS niedostępny",4,4,284,&dsr_roboto_16,DSR_SC_TEXT);
 char address[48];if(wifi.connected&&wifi.ip[0])snprintf(address,sizeof(address),"https://%s",wifi.ip);else snprintf(address,sizeof(address),"IPv4: —");
 dsr_sc_text(body,address,4,36,284,&dsr_roboto_14,DSR_SC_BLUE);dsr_sc_text(body,"https://shelly-remote.local",4,62,284,&dsr_roboto_14,DSR_SC_BLUE);
 const char *fp=dsr_tls_fingerprint();char digest[80];snprintf(digest,sizeof(digest),"%.32s\n%.32s",fp?fp:"",fp&&strlen(fp)>32?fp+32:"");
 dsr_sc_text(body,"SHA-256 certyfikatu tego pilota",4,100,284,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(body,fp&&strlen(fp)==64?digest:"—",4,128,284,&dsr_roboto_11,DSR_SC_TEXT);
 char status[64];snprintf(status,sizeof(status),"SPIFFS: %d / 0x%x",(int)health.storage,(unsigned)health.storage_error);
 dsr_sc_text(body,status,4,172,284,&dsr_roboto_12,DSR_SC_MUTED);
 row(body,"Wygeneruj kod parowania",210,pair_browser);
 dsr_sc_text(body,"Sesje i unieważnianie: brak widoku LCD. Kod nie jest stałym hasłem.",4,276,284,&dsr_roboto_12,DSR_SC_MUTED);
 row(body,"Diagnostyka konfiguracji",340,dsr_ui_config_diagnostics);
 row(body,"Odśwież",398,web_panel);
}
static void ota_allowed(lv_event_t *event)
{
 (void)event;bool accepted=dsr_panel_allow_ota();lv_obj_t *body=settings_body("Lokalne OTA • Web Panel");
 lv_obj_t *status=dsr_sc_text(body,accepted?"OTA dozwolone przez 2 minuty. Wybierz BIN w panelu.":"Nie udzielono zgody OTA. HTTPS niedostępny lub zajęty.",4,12,284,&dsr_roboto_18,DSR_SC_TEXT);
 if(accepted){expiry_label=status;issued_at=lv_tick_get();expiry_pair=false;}
 dsr_sc_text(body,"To lokalny tryb deweloperski, nie podpisany updater z pilota.",4,146,284,&dsr_roboto_14,DSR_SC_MUTED);
}
static void firmware(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Firmware Update");
 dsr_sc_text(body,"Current version",4,4,284,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(body,esp_app_get_description()->version,4,32,284,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(body,dsr_board_hardware_id(),4,62,284,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(body,"Kanał: Dev • wersja testowa",4,94,284,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(body,"Natywne podpisane OTA: niewdrożone (#24). Brak zweryfikowanego wydania.",4,136,284,&dsr_roboto_14,DSR_SC_MUTED);
 unavailable(body,"Sprawdź aktualizacje • niedostępne",218);unavailable(body,"Zainstaluj • niedostępne",276);
 row(body,"Zezwól na OTA • 2 minuty",334,ota_allowed);
}
static void system_info(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Informacje i diagnostyka");dsr_system_info_t info;dsr_system_info_snapshot(&info);
 dsr_sc_text(body,esp_app_get_description()->version,4,4,284,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(body,dsr_board_hardware_id(),4,36,284,&dsr_roboto_12,DSR_SC_MUTED);
 char text[160];snprintf(text,sizeof(text),dsr_sc_tr("RAM wolna: %lu B\nNajwiększy blok: %lu B\nPSRAM wolna: %lu B\nFlash dostępna: %lu B / 0x%x"),(unsigned long)info.internal_free,(unsigned long)info.internal_largest,(unsigned long)info.psram_free,(unsigned long)info.flash_bytes,(unsigned)info.flash_error);
 dsr_sc_text(body,text,4,76,284,&dsr_roboto_14,DSR_SC_TEXT);
 row(body,"Diagnostyka konfiguracji",194,dsr_ui_config_diagnostics);
 dsr_sc_text(body,"Restart z LCD: niewdrożony. Eksport i reset: niewdrożone; dane zachowane.",4,256,284,&dsr_roboto_14,DSR_SC_MUTED);
 row(body,"Odśwież",342,system_info);
}
static void system_menu(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("System");
 row(body,"Firmware Update",0,firmware);row(body,"Informacje i diagnostyka",58,system_info);
 row(body,"Hardware tests",116,hardware);row(body,"Konfiguracja wpisów i scen",174,dsr_ui_config_registry);
 row(body,"Podgląd rolety • DEMO",232,cover_preview);unavailable(body,"Restart • niedostępny",290);
}
static void screen_settings(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Ekran i komfort");row(body,"Ekran",0,dsr_ui_display_open);
 row(body,"Screen off / touch to wake",58,sleep_display);
 dsr_sc_text(body,"Pierwszy dotyk wybudza bez wykonania akcji.",4,128,284,&dsr_roboto_14,DSR_SC_MUTED);
}
static void menu(lv_event_t *event)
{
 (void)event;lv_obj_t *root=dsr_ui_screen("Ustawienia pilota",true);
 lv_obj_t *body=dsr_sc_panel(root,12,86,296,330,DSR_SC_BG,0);
 lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);
 const char *names[]={"Sieć i połączenia","Wygląd i język","Ekran i komfort","Bateria i zasilanie","Panel WWW / bezpieczeństwo","System"};
 const dsr_ui_icon_t icons[]={DSR_ICON_WIFI,DSR_ICON_GRID,DSR_ICON_DASHBOARD,DSR_ICON_BATTERY,DSR_ICON_HOME,DSR_ICON_SETTINGS};
 const lv_event_cb_t callbacks[]={dsr_ui_settings_network,dsr_ui_config_appearance,screen_settings,dsr_ui_battery_open,web_panel,system_menu};
 for(unsigned i=0;i<6;i++){
  lv_obj_t *button=row(body,names[i],(int)i*58,callbacks[i]);lv_obj_t *caption=lv_obj_get_child(button,0);
  lv_obj_set_width(caption,236);lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_LEFT,0);lv_obj_align(caption,LV_ALIGN_LEFT_MID,48,0);
  dsr_sc_icon(button,icons[i],12,13,22,DSR_SC_BLUE);
 }
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
