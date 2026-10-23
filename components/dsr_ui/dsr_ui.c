#include "dsr_ui_provision.h"
#include "dsr_ui_brand.h"
#include "dsr_style_hint.h"
#include "dsr_ui.h"
#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_ui_cyber.h"
#include "dsr_board.h"
#include "esp_app_desc.h"
#include "dsr_workspace.h"
#include "dsr_system_info.h"
#include "dsr_restart.h"
#include "dsr_blu.h"
#include "dsr_panel_pairing.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static lv_obj_t *touch_status;
static unsigned touches;
static lv_obj_t *expiry_label;
static lv_obj_t *ota_expiry_icon;
static void web_ota_warning(lv_obj_t *label);
static void web_card_fit(lv_obj_t *label);
static lv_obj_t *web_ota_message(lv_obj_t *body,const char *text,int y,bool warning,lv_obj_t **icon);
static uint32_t issued_at;
static bool expiry_pair;
static lv_obj_t *restart_status,*restart_confirm_button,*restart_cancel_button;
/* Accepted is process-lifetime state, not a screen state. Navigating away must
 * neither cancel backend admission nor submit a second request on reopening. */
static bool restart_accepted;
static void restart_confirm(lv_event_t *event);
static lv_obj_t *revoke_state_label,*revoke_status,*revoke_confirm_button,*revoke_cancel_button;
static lv_obj_t *web_pairing_state,*web_pairing_generate;
static bool revoke_pending;
static const char *revoke_message="Potwierdź unieważnienie przyciskiem poniżej.";
void dsr_ui_settings_hide(void)
{expiry_label=ota_expiry_icon=NULL;restart_status=NULL;restart_confirm_button=NULL;restart_cancel_button=NULL;
 revoke_state_label=revoke_status=revoke_confirm_button=revoke_cancel_button=NULL;
 web_pairing_state=web_pairing_generate=NULL;}
void dsr_ui_settings_tick(void)
{
 if(expiry_label&&(uint32_t)(lv_tick_get()-issued_at)>=120000){
  dsr_sc_label_set_text(expiry_label,expiry_pair?"Kod parowania wygasł.":"Zgoda na OTA wygasła.");web_ota_warning(expiry_label);if(expiry_pair)web_card_fit(expiry_label);expiry_label=NULL;
 }
}

void dsr_ui_home(lv_event_t *event);
static void menu(lv_event_t *event);
static void hardware(lv_event_t *event);

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

static dsr_ui_icon_t section_icon(const char *text)
{
 if(!strcmp(text,"Firmware Update")||!strcmp(text,"Aktualizacja firmware")||!strcmp(text,"Wersja firmware"))return DSR_ICON_FIRMWARE;
 if(!strcmp(text,"Informacje i diagnostyka")||!strcmp(text,"Diagnostyka konfiguracji")||!strcmp(text,"Diagnostyka"))return DSR_ICON_DIAGNOSTICS;
 if(!strcmp(text,"Hardware tests")||!strcmp(text,"Check touchscreen"))return DSR_ICON_HARDWARE;
 if(!strcmp(text,"Check display colors")||!strcmp(text,"Ekran"))return DSR_ICON_GRAPHICS;
 if(!strcmp(text,"Konfiguracja wpisów i scen")||!strcmp(text,"Konfiguracja wpisów")||!strcmp(text,"Sceny")||!strcmp(text,"+ Scena"))return DSR_ICON_SCENES;
 if(!strcmp(text,"Restart pilota")||!strcmp(text,"Uruchom ponownie"))return DSR_ICON_RESTART;
 if(!strcmp(text,"Pulpity")||!strcmp(text,"Grupy")||!strcmp(text,"+ Grupa"))return DSR_ICON_GRID;
 if(!strcmp(text,"Pomieszczenia")||!strcmp(text,"+ Pomieszczenie"))return DSR_ICON_HOME;
 if(!strcmp(text,"Urządzenia")||!strcmp(text,"+ Urządzenie"))return DSR_ICON_PLUG;
 if(!strcmp(text,"Ustawienia Shelly")||!strcmp(text,"Użyj nazwy z Shelly"))return DSR_ICON_SETTINGS;
 if(!strcmp(text,"Odśwież status")||!strcmp(text,"Odczytaj ponownie"))return DSR_ICON_REFRESH;
 return DSR_ICON_COUNT;
}
lv_obj_t *dsr_ui_button(lv_obj_t *parent, const char *text, int x, int y, int width,
                        lv_event_cb_t callback, void *context)
{
    lv_obj_t *obj = dsr_sc_button(parent,x,y,width,48,DSR_SC_CARD,DSR_SC_RADIUS,callback,context);
    lv_obj_set_style_text_color(obj,dsr_sc_color(DSR_SC_TEXT),0);
    lv_obj_t *caption = lv_label_create(obj);
    lv_obj_set_style_text_font(caption, &dsr_roboto_16, 0);
    lv_obj_set_width(caption,width-16);lv_label_set_long_mode(caption,LV_LABEL_LONG_MODE_WRAP);lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
    lv_label_set_text(caption, dsr_sc_tr(text));
    lv_obj_center(caption);
    if(parent==lv_screen_active()&&(!strcmp(text,"Wstecz")||!strcmp(text,"Back")))dsr_sc_header_back(parent,callback,context);
    if(!strncmp(text,"Usuń",strlen("Usuń"))||(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&!strcmp(text,"Unieważnij parowanie"))){lv_obj_set_style_bg_color(obj,dsr_sc_color(0xb42332),0);lv_obj_set_style_bg_color(obj,dsr_sc_color(0x8f1d2b),LV_STATE_PRESSED);dsr_sc_border_color(obj,dsr_sc_color(0xe14d5b),0);lv_obj_set_style_text_color(caption,lv_color_hex(0xffffff),0);if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_warning(obj,true);}
    else if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&(!strcmp(text,"Zapisz")||!strcmp(text,"Dalej")||!strcmp(text,"Sprawdź aktualizacje")||!strcmp(text,"Zmierz napięcie")||!strcmp(text,"Odśwież stan")||!strcmp(text,"Refresh")||!strcmp(text,"Odśwież")))dsr_ui_primary(obj);
    if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&width>=100){
     dsr_ui_icon_t icon=DSR_ICON_COUNT;
     if(!strcmp(text,"Sprawdź aktualizacje")||!strcmp(text,"Refresh")||!strcmp(text,"Odśwież")||!strcmp(text,"Odśwież stan"))icon=DSR_ICON_REFRESH;
     else if(!strcmp(text,"Zmierz napięcie"))icon=DSR_ICON_ACTIVITY;
     else if(!strcmp(text,"Zapisz"))icon=DSR_ICON_SAVE;
     else if(!strcmp(text,"Zmień kanał"))icon=DSR_ICON_GIT_BRANCH;
     else if(!strcmp(text,"Sprawdź baterię"))icon=DSR_ICON_BATTERY;
     else if(!strncmp(text,"Usuń",strlen("Usuń")))icon=DSR_ICON_TRASH;
     else if(!strcmp(text,"Wi-Fi setup"))icon=DSR_ICON_SETTINGS;
     else if(!strcmp(text,"Bluetooth LE • skan i setup"))icon=DSR_ICON_BLUETOOTH;
     else if(!strcmp(text,"Wi-Fi AP • dodaj Shelly"))icon=DSR_ICON_ROUTER;
     else if(!strcmp(text,"Dodaj urządzenie Shelly"))icon=DSR_ICON_PLUS;
     else if(!strcmp(text,"Wstecz")||!strcmp(text,"Back"))icon=DSR_ICON_BACK;
     if(icon==DSR_ICON_COUNT)icon=section_icon(text);
     if(width<170&&icon!=DSR_ICON_SAVE&&icon!=DSR_ICON_BACK)icon=DSR_ICON_COUNT;
     if(icon!=DSR_ICON_COUNT){bool compact=icon==DSR_ICON_BACK&&width<130;lv_obj_set_width(caption,width-(compact?44:58));lv_obj_align(caption,LV_ALIGN_RIGHT_MID,compact?-4:-8,0);dsr_sc_icon(obj,icon,compact?8:14,10,compact?24:28,icon==DSR_ICON_TRASH?0xffffff:dsr_sc_icon_accent());}
    }
    return obj;
}

void dsr_ui_primary(lv_obj_t *obj)
{
    if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_panel(obj,true);
    lv_obj_set_style_bg_color(obj, dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?lv_color_hex(0x0065ce):dsr_sc_color(DSR_SC_BLUE), 0);
    lv_obj_set_style_bg_color(obj, dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?lv_color_hex(0x0084ee):dsr_sc_color(0x287ec7), LV_STATE_PRESSED);
}

void dsr_ui_textarea(lv_obj_t *obj)
{
    lv_obj_set_style_text_font(obj, &dsr_roboto_18, 0);
    lv_obj_set_style_bg_color(obj, dsr_sc_color(DSR_SC_PANEL), 0);
    lv_obj_set_style_text_color(obj, dsr_sc_color(0xf0f4fb), 0);
    lv_obj_set_style_text_color(obj, dsr_sc_color(0x98aac0), LV_PART_TEXTAREA_PLACEHOLDER);
    dsr_sc_border_color(obj, dsr_sc_color(DSR_SC_EDGE), 0);
    dsr_sc_border_color(obj, dsr_sc_color(DSR_SC_BLUE), LV_STATE_FOCUSED);
    dsr_sc_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_bg_color(obj, dsr_sc_color(0xf0f4fb), LV_PART_CURSOR);
    if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_panel(obj,false);
}

static void shell_navigation(lv_event_t *event)
{
 unsigned index=(unsigned)(uintptr_t)lv_event_get_user_data(event);
 if(index<2){dsr_network_home(NULL);if(index==1)dsr_mobile_show(DSR_MOBILE_ROOMS,0);}
 else if(index==2)dsr_ui_battery_open(NULL);else if(index==3)dsr_ui_open_settings();else if(index==4)dsr_network_device(NULL);
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
    dsr_sc_background(root);
    lv_obj_set_style_text_color(root, dsr_sc_color(0xf0f4fb), 0);
    lv_obj_set_style_text_font(root, &dsr_roboto_14, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    dsr_sc_header(root,"",false,dsr_wifi_connected(),dsr_ui_home,NULL);
    bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
    dsr_ui_icon_t icon=!strcmp(title,"System")||!strcmp(title,"Diagnostyka konfiguracji")?DSR_ICON_CPU:(!strcmp(title,"Bateria i zasilanie")||!strcmp(title,"Kalibracja baterii"))?DSR_ICON_BATTERY:section_icon(title);
    bool marked=hud&&icon!=DSR_ICON_COUNT;
    if(marked)dsr_sc_icon(root,icon,12,47,24,DSR_SC_GLOW);
    int heading_width=marked?264:296;const lv_font_t *heading_font=hud?&dsr_roboto_22:&dsr_roboto_18;
    lv_point_t heading_size;lv_text_get_size(&heading_size,dsr_sc_tr(title),dsr_sc_font(heading_font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    if(heading_size.x>heading_width){heading_font=&dsr_roboto_18;lv_text_get_size(&heading_size,dsr_sc_tr(title),dsr_sc_font(heading_font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(heading_size.x>heading_width)heading_font=&dsr_roboto_16;}
    lv_obj_t *heading=dsr_sc_text(root,title,marked?44:12,46,heading_width,heading_font,DSR_SC_TEXT);
    lv_obj_set_height(heading,hud?27:24);lv_label_set_long_mode(heading,LV_LABEL_LONG_MODE_DOTS);
    dsr_sc_panel(root,12,75,296,1,DSR_SC_BLUE,0);
    if(navigation)dsr_sc_navigation(root,3,31,shell_navigation);
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
    dsr_ui_button(root, "Back", 100, 325, 120, hardware, NULL);
}

static void colors(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Display colors", true);
    dsr_sc_header_back(root,hardware,NULL);
    dsr_ui_label(root, "Each label should match its color.", 16, 88, 288);
    const char *names[] = {"RED", "GREEN", "BLUE", "WHITE"};
    const uint32_t values[] = {0xff0000, 0x00ff00, 0x0000ff, 0xffffff};
    for (unsigned i = 0; i < 4; i++) {
        lv_obj_t *bar = lv_obj_create(root);
        lv_obj_set_pos(bar, 16, 133 + (int)i * 63);
        lv_obj_set_size(bar, 288, 48);
        lv_obj_set_style_bg_color(bar, lv_color_hex(values[i]), 0);
        dsr_sc_border_width(bar, 0, 0);
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
/* Every global section starts with a fresh LCD body; no cached object pointers
 * survive Back/Home. Pair codes and tokens are never stored in this module. */
static void settings_back(lv_event_t *event){(void)event;dsr_ui_open_settings();}
static lv_obj_t *settings_body(const char *title)
{
 lv_obj_t *root=dsr_ui_screen(title,false);
 lv_obj_t *body=dsr_sc_panel(root,12,86,296,318,DSR_SC_BG,0);
 lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 dsr_ui_button(root,"Wstecz",12,420,296,settings_back,NULL);return body;
}
static lv_obj_t *row(lv_obj_t *body,const char *text,int y,lv_event_cb_t action)
{return dsr_ui_button(body,text,0,y,296,action,NULL);}
/* Shared reference-style system rows: a separate icon well and a large
 * wrapping caption. The entire framed row remains the original hit target. */
static lv_obj_t *system_row(lv_obj_t *parent,const char *text,int y,int height,dsr_ui_icon_t icon,lv_event_cb_t action)
{
 lv_obj_t *button=dsr_sc_button(parent,0,y,296,height,DSR_SC_CARD,10,action,NULL);
 lv_obj_t *caption=dsr_sc_text(button,text,60,0,200,&dsr_roboto_18,DSR_SC_TEXT);
 lv_obj_align(caption,LV_ALIGN_LEFT_MID,60,0);
 lv_obj_t *well=dsr_sc_panel(button,5,5,44,height-12,DSR_SC_PANEL,8);dsr_ui_cyber_inset(well);
 dsr_sc_icon_centered(well,icon,32,DSR_SC_GLOW);
 dsr_sc_icon(button,DSR_ICON_NEXT,264,(height-24)/2-1,24,DSR_SC_GLOW);
 dsr_sc_border_color(button,dsr_sc_color(DSR_SC_GLOW),0);return button;
}
void dsr_ui_settings_network(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Sieć i połączenia");dsr_wifi_info_t wifi;dsr_wifi_get_info(&wifi);
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 if(hud){lv_obj_t *card=dsr_sc_panel(body,0,0,296,88,DSR_SC_CARD,12);dsr_sc_icon(card,DSR_ICON_WIFI,14,22,40,wifi.connected?DSR_SC_BLUE:DSR_SC_OFF);}
 dsr_sc_text(body,wifi.connected?"Wi-Fi: połączono":"Wi-Fi: brak połączenia",hud?66:4,hud?8:4,hud?216:284,hud?&dsr_roboto_12:&dsr_roboto_16,hud?DSR_SC_MUTED:DSR_SC_TEXT);
 lv_obj_t *ssid=dsr_sc_text(body,wifi.ssid[0]?wifi.ssid:"—",hud?66:4,hud?28:34,hud?216:284,hud?&dsr_roboto_22:&dsr_roboto_14,hud?DSR_SC_TEXT:DSR_SC_MUTED);
 lv_obj_set_height(ssid,hud?28:20);lv_label_set_long_mode(ssid,LV_LABEL_LONG_MODE_DOTS);
 char text[48];snprintf(text,sizeof(text),"IPv4: %s",wifi.connected&&wifi.ip[0]?wifi.ip:"—");
 dsr_sc_text(body,text,hud?66:4,62,hud?216:284,hud?&dsr_roboto_12:&dsr_roboto_14,DSR_SC_MUTED);
 row(body,"Wi-Fi setup",98,dsr_network_wifi);row(body,"Bluetooth LE • skan i setup",156,dsr_ui_ble_settings);
 dsr_sc_text(body,"BLE: otwarte okno setup Gen2+. AP: otwarty punkt Shelly.",4,218,284,&dsr_roboto_12,DSR_SC_MUTED);
 row(body,"Wi-Fi AP • dodaj Shelly",266,dsr_ui_ap_open);row(body,"Dodaj urządzenie Shelly",324,dsr_network_device);
 row(body,"Odśwież",382,dsr_ui_settings_network);
}
static void web_panel(lv_event_t *event);
static const char *pairing_state_text(void)
{
 dsr_panel_pairing_snapshot_t state;
 if(!dsr_panel_pairing_snapshot(&state))return "Stan parowania niedostępny.";
 if(state.revocation_unconfirmed)return "Trwałe unieważnienie: niepotwierdzone.";
 return state.paired?"Dostęp WWW: aktywny.":"Dostęp WWW: nieaktywny.";
}
static lv_obj_t *web_security_body(const char *title)
{
 lv_obj_t *body=settings_body(title);lv_obj_set_style_bg_opa(body,LV_OPA_TRANSP,0);
 lv_obj_set_flex_flow(body,LV_FLEX_FLOW_COLUMN);lv_obj_set_style_pad_row(body,12,0);return body;
}
static void web_card_fit(lv_obj_t *label)
{
 lv_obj_update_layout(label);int height=lv_obj_get_y(label)+lv_obj_get_height(label)+12;
 lv_obj_set_height(lv_obj_get_parent(label),height<56?56:height);
}
static void revoke_controls(void)
{
 if(web_pairing_state){
  dsr_sc_label_set_text(web_pairing_state,revoke_pending?"Unieważnianie parowania…":pairing_state_text());
  web_card_fit(web_pairing_state);
  if(revoke_pending)lv_obj_add_state(web_pairing_generate,LV_STATE_DISABLED);else lv_obj_remove_state(web_pairing_generate,LV_STATE_DISABLED);
 }
 if(!revoke_status)return;
 dsr_sc_label_set_text(revoke_status,revoke_pending?"Unieważnianie parowania…":revoke_message);
 dsr_sc_label_set_text(revoke_state_label,pairing_state_text());
 web_card_fit(revoke_status);web_card_fit(revoke_state_label);
 lv_obj_scroll_to_view(lv_obj_get_parent(revoke_status),LV_ANIM_OFF);
 lv_obj_t *items[]={revoke_confirm_button,revoke_cancel_button};
 for(unsigned i=0;i<2;i++)if(revoke_pending)lv_obj_add_state(items[i],LV_STATE_DISABLED);else lv_obj_remove_state(items[i],LV_STATE_DISABLED);
}
void dsr_ui_pairing_revoke_result(const dsr_event_t *event)
{
 if(!revoke_pending)return;
 revoke_pending=false;
 revoke_message=event->result==DSR_OK?"Parowanie unieważniono. Nowy dostęp wymaga nowego kodu.":"Nie potwierdzono trwałego unieważnienia. Spróbuj ponownie.";
 revoke_controls();
}
static void revoke_confirm(lv_event_t *event)
{
 (void)event;if(revoke_pending||!revoke_status)return;
 dsr_result_t accepted=dsr_network_request_pairing_revoke();
 if(accepted==DSR_OK)revoke_pending=true;
 else revoke_message="Pilot jest zajęty. Spróbuj ponownie.";
 revoke_controls();
}
static void revoke_cancel(lv_event_t *event)
{if(!revoke_pending)web_panel(event);}
static void revoke_screen(lv_event_t *event)
{
 (void)event;lv_obj_t *body=web_security_body("Unieważnij parowanie WWW");
 lv_obj_set_height(body,248);lv_obj_t *root=lv_obj_get_parent(body);
 revoke_state_label=web_ota_message(body,pairing_state_text(),0,false,NULL);
 web_ota_message(body,"Stare cookie utracą dostęp. Trwające operacje mogą się zakończyć. Wi-Fi i dane pozostaną.",0,true,NULL);
 revoke_status=web_ota_message(body,revoke_pending?"Unieważnianie parowania…":revoke_message,0,true,NULL);
 revoke_confirm_button=dsr_ui_button(root,"Unieważnij parowanie",12,350,180,revoke_confirm,NULL);
 revoke_cancel_button=dsr_ui_button(root,"Anuluj",204,350,104,revoke_cancel,NULL);revoke_controls();
}
static void pair_browser(lv_event_t *event)
{
 (void)event;char code[17]={0};if(!revoke_pending&&dsr_panel_ready())dsr_panel_pair_begin(code);
 lv_obj_t *body=web_security_body("Parowanie Web Panelu");
 if(!code[0]){web_ota_message(body,"Nie utworzono kodu. HTTPS niedostępny lub zajęty.",0,true,NULL);row(body,"Panel WWW / bezpieczeństwo",0,web_panel);return;}
 web_ota_message(body,"Jednorazowy kod • 120 s • 5 prób",0,false,NULL);
 expiry_label=web_ota_message(body,code,0,false,&ota_expiry_icon);lv_obj_set_style_text_font(expiry_label,&dsr_roboto_18,0);web_card_fit(expiry_label);issued_at=lv_tick_get();expiry_pair=true;memset(code,0,sizeof(code));
 web_ota_message(body,"Sprawdź odcisk certyfikatu w przeglądarce przed wpisaniem kodu.",0,true,NULL);
 row(body,"Panel WWW / bezpieczeństwo",0,web_panel);
}
static void web_panel(lv_event_t *event)
{
 (void)event;lv_obj_t *body=web_security_body("Panel WWW / bezpieczeństwo");
 dsr_wifi_info_t wifi;dsr_wifi_get_info(&wifi);dsr_config_health_t health;dsr_config_health_snapshot(&health);
 lv_obj_t *label=web_ota_message(body,dsr_panel_ready()?"HTTPS uruchomiony":"HTTPS niedostępny",0,!dsr_panel_ready(),NULL),*frame=lv_obj_get_parent(label);lv_obj_set_height(frame,92);
 char address[48];if(wifi.connected&&wifi.ip[0])snprintf(address,sizeof(address),"https://%s",wifi.ip);else snprintf(address,sizeof(address),"IPv4: —");
 lv_obj_update_layout(label);int y=lv_obj_get_y(label)+lv_obj_get_height(label)+8;
 label=dsr_sc_text(frame,address,44,y,240,&dsr_roboto_14,DSR_SC_BLUE);lv_obj_update_layout(label);y+=lv_obj_get_height(label)+8;
 label=dsr_sc_text(frame,"https://shelly-remote.local",44,y,240,&dsr_roboto_14,DSR_SC_BLUE);lv_obj_update_layout(label);y+=lv_obj_get_height(label)+8;
 web_pairing_state=dsr_sc_text(frame,revoke_pending?"Unieważnianie parowania…":pairing_state_text(),44,y,240,&dsr_roboto_14,DSR_SC_TEXT);web_card_fit(web_pairing_state);
 const char *fp=dsr_tls_fingerprint();char digest[80];snprintf(digest,sizeof(digest),"%.32s\n%.32s",fp?fp:"",fp&&strlen(fp)>32?fp+32:"");
 label=web_ota_message(body,"SHA-256 certyfikatu tego pilota",0,false,NULL);lv_obj_set_style_text_font(label,&dsr_roboto_12,0);frame=lv_obj_get_parent(label);lv_obj_update_layout(label);
 y=lv_obj_get_y(label)+lv_obj_get_height(label)+8;
 label=dsr_sc_text(frame,fp&&strlen(fp)==64?digest:"—",12,y,272,&dsr_roboto_12,DSR_SC_TEXT);lv_obj_update_layout(label);y+=lv_obj_get_height(label)+12;
 char status[64];snprintf(status,sizeof(status),"SPIFFS: %d / 0x%x",(int)health.storage,(unsigned)health.storage_error);
 label=dsr_sc_text(frame,status,12,y,272,&dsr_roboto_12,DSR_SC_MUTED);web_card_fit(label);
 web_pairing_generate=row(body,"Wygeneruj kod parowania",0,pair_browser);if(revoke_pending)lv_obj_add_state(web_pairing_generate,LV_STATE_DISABLED);
 web_ota_message(body,"Wspólne parowanie, nie lista aktywnych sesji.",0,false,NULL);
 row(body,"Unieważnij parowanie WWW",0,revoke_screen);
 row(body,"Diagnostyka konfiguracji",0,dsr_ui_config_diagnostics);
 row(body,"Odśwież",0,web_panel);
}
static uint32_t web_ota_gold(void){return dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;}
static void web_ota_warning(lv_obj_t *label)
{
 lv_obj_t *frame=lv_obj_get_parent(label);uint32_t gold=web_ota_gold();
 lv_obj_set_style_text_color(label,dsr_sc_color(gold),0);dsr_sc_border_color(frame,dsr_sc_color(gold),0);
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_warning(frame,false);
 if(ota_expiry_icon)dsr_sc_icon_color(ota_expiry_icon,gold);
}
static lv_obj_t *web_ota_message(lv_obj_t *body,const char *text,int y,bool warning,lv_obj_t **icon)
{
 lv_obj_t *frame=dsr_sc_panel(body,0,y,296,56,DSR_SC_CARD,10);
 dsr_sc_border_width(frame,1,0);dsr_sc_border_color(frame,dsr_sc_color(warning?web_ota_gold():DSR_SC_EDGE),0);
 if(warning&&dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_warning(frame,false);
 lv_obj_t *symbol=dsr_sc_icon(frame,warning?DSR_ICON_WARNING:DSR_ICON_INFO,12,12,24,warning?web_ota_gold():DSR_SC_GLOW);
 if(icon)*icon=symbol;
 lv_obj_t *label=dsr_sc_text(frame,text,44,12,240,&dsr_roboto_14,warning?web_ota_gold():DSR_SC_TEXT);
 lv_obj_update_layout(label);int h=lv_obj_get_height(label)+24;lv_obj_set_height(frame,h<56?56:h);lv_obj_update_layout(frame);return label;
}
static void web_ota_grant(lv_event_t *event)
{
 (void)event;bool accepted=dsr_panel_allow_ota(true);lv_obj_t *body=settings_body("Lokalne OTA • Web Panel");
 lv_obj_set_style_bg_opa(body,LV_OPA_TRANSP,0);
 lv_obj_t *status=web_ota_message(body,accepted?"OTA dozwolone przez 2 minuty. Wybierz BIN w panelu.":"Nie udzielono zgody OTA. HTTPS niedostępny lub zajęty.",0,!accepted,&ota_expiry_icon);
 if(accepted){expiry_label=status;issued_at=lv_tick_get();expiry_pair=false;}
 lv_obj_t *frame=lv_obj_get_parent(status);int y=lv_obj_get_y(frame)+lv_obj_get_height(frame)+12;
 lv_obj_t *help=web_ota_message(body,"Otwórz w przeglądarce System i OTA. Wybierz BIN aplikacji. Podłącz stabilne zasilanie USB.",y,false,NULL);
 frame=lv_obj_get_parent(help);y=lv_obj_get_y(frame)+lv_obj_get_height(frame)+12;
 web_ota_message(body,"To lokalny tryb deweloperski, nie podpisany updater z pilota.",y,true,NULL);
}
void dsr_ui_web_ota_open(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Lokalne OTA • Web Panel");lv_obj_set_style_bg_opa(body,LV_OPA_TRANSP,0);
 lv_obj_t *label=web_ota_message(body,"Lokalne OTA wymaga potwierdzenia USB.",0,false,NULL);
 lv_obj_t *frame=lv_obj_get_parent(label);int y=lv_obj_get_y(frame)+lv_obj_get_height(frame)+12;
 label=web_ota_message(body,"Zezwól na OTA potwierdza stabilne zasilanie USB. Nie odłączaj go podczas aktualizacji.",y,true,NULL);
 frame=lv_obj_get_parent(label);y=lv_obj_get_y(frame)+lv_obj_get_height(frame)+12;
 label=web_ota_message(body,"To lokalny tryb deweloperski, nie podpisany updater z pilota.",y,true,NULL);
 frame=lv_obj_get_parent(label);y=lv_obj_get_y(frame)+lv_obj_get_height(frame)+12;
 dsr_ui_primary(dsr_ui_button(body,"Zezwól na OTA",0,y,296,web_ota_grant,NULL));
}

static void system_menu(lv_event_t *event);
static void restart_cancel(lv_event_t *event)
{if(!restart_accepted)system_menu(event);}
static void restart_confirm(lv_event_t *event)
{
 (void)event;if(restart_accepted||!restart_status||!restart_confirm_button||!restart_cancel_button)return;
 dsr_restart_result_t result=dsr_restart_request_confirmed();
 const char *text="Restart jest niedostępny. Spróbuj później.";
 if(result==DSR_RESTART_ACCEPTED){
  restart_accepted=true;text="Przyjęto żądanie restartu.";
  lv_obj_add_state(restart_confirm_button,LV_STATE_DISABLED);lv_obj_add_state(restart_cancel_button,LV_STATE_DISABLED);
 }else if(result==DSR_RESTART_BUSY){
  text="Pilot jest zajęty. Spróbuj ponownie.";
 }
 dsr_sc_label_set_text(restart_status,text);
}
static void restart_screen(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Restart pilota");
 lv_obj_t *card=dsr_sc_panel(body,0,0,296,200,DSR_SC_CARD,12);
 dsr_sc_text(card,"Czy uruchomić pilot ponownie?",12,12,268,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(card,"Ustawienia i zdjęcia pozostaną. Niezapisane zmiany zostaną utracone.",12,66,268,&dsr_roboto_14,DSR_SC_MUTED);
 restart_status=dsr_sc_text(card,restart_accepted?"Przyjęto żądanie restartu.":"Potwierdź restart przyciskiem poniżej.",12,144,268,&dsr_roboto_14,DSR_SC_TEXT);
 restart_confirm_button=row(body,"Uruchom ponownie",210,restart_confirm);dsr_ui_primary(restart_confirm_button);
 restart_cancel_button=row(body,"Anuluj",268,restart_cancel);
 if(restart_accepted){lv_obj_add_state(restart_confirm_button,LV_STATE_DISABLED);lv_obj_add_state(restart_cancel_button,LV_STATE_DISABLED);}
}
static void system_info(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Informacje i diagnostyka");dsr_system_info_t info;dsr_system_info_snapshot(&info);
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){
  lv_obj_t *firmware=dsr_sc_panel(body,0,0,296,62,DSR_SC_CARD,8);dsr_sc_icon(firmware,DSR_ICON_FIRMWARE,10,15,32,DSR_SC_GLOW);
  dsr_sc_text(firmware,esp_app_get_description()->version,52,10,234,&dsr_roboto_16,DSR_SC_TEXT);
  dsr_sc_text(firmware,dsr_board_hardware_id(),52,35,234,&dsr_roboto_12,DSR_SC_MUTED);
  lv_obj_t *memory=dsr_sc_panel(body,0,70,296,148,DSR_SC_CARD,8);
  const dsr_ui_icon_t icons[]={DSR_ICON_MEMORY,DSR_ICON_MEMORY,DSR_ICON_PSRAM,DSR_ICON_FIRMWARE};
  const char *names[]={"RAM","Największy blok","PSRAM","Flash"};
  const size_t values[]={info.internal_free,info.internal_largest,info.psram_free,info.flash_bytes};
  for(unsigned i=0;i<4;i++){
   int y=8+(int)i*34;dsr_sc_icon(memory,icons[i],10,y,24,DSR_SC_GLOW);
   dsr_sc_text(memory,names[i],42,y+5,100,&dsr_roboto_12,DSR_SC_MUTED);
   char value[40];if(i==3&&info.flash_error)snprintf(value,sizeof(value),"%lu B / 0x%x",(unsigned long)values[i],(unsigned)info.flash_error);else snprintf(value,sizeof(value),"%lu B",(unsigned long)values[i]);
   dsr_sc_text(memory,dsr_sc_user(value),146,y+4,140,&dsr_roboto_14,DSR_SC_TEXT);
   if(i<3)dsr_sc_panel(memory,42,y+28,244,1,DSR_SC_EDGE,0);
  }
  row(body,"Diagnostyka konfiguracji",228,dsr_ui_config_diagnostics);
  dsr_sc_text(body,"Restart: w menu System, po potwierdzeniu. Eksport i reset: niewdrożone.",4,288,284,&dsr_roboto_12,DSR_SC_MUTED);
  row(body,"Odśwież",350,system_info);return;
 }
 dsr_sc_text(body,esp_app_get_description()->version,4,4,284,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(body,dsr_board_hardware_id(),4,36,284,&dsr_roboto_12,DSR_SC_MUTED);
 char text[160];snprintf(text,sizeof(text),dsr_sc_tr("RAM wolna: %lu B\nNajwiększy blok: %lu B\nPSRAM wolna: %lu B\nFlash dostępna: %lu B / 0x%x"),(unsigned long)info.internal_free,(unsigned long)info.internal_largest,(unsigned long)info.psram_free,(unsigned long)info.flash_bytes,(unsigned)info.flash_error);
 dsr_sc_text(body,text,4,76,284,&dsr_roboto_14,DSR_SC_TEXT);
 row(body,"Diagnostyka konfiguracji",194,dsr_ui_config_diagnostics);
 dsr_sc_text(body,"Restart: w menu System, po potwierdzeniu. Eksport i reset: niewdrożone.",4,256,284,&dsr_roboto_14,DSR_SC_MUTED);
 row(body,"Odśwież",342,system_info);
}
static void system_menu(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("System");
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){
  const char *names[]={"Firmware Update","Informacje i diagnostyka","Hardware tests","Konfiguracja wpisów i scen","Restart pilota","Aktywność"};
  const dsr_ui_icon_t icons[]={DSR_ICON_FIRMWARE,DSR_ICON_DIAGNOSTICS,DSR_ICON_HARDWARE,DSR_ICON_SCENES,DSR_ICON_RESTART,DSR_ICON_ACTIVITY};
  const lv_event_cb_t callbacks[]={dsr_ui_native_ota_open,system_info,hardware,dsr_ui_config_registry,restart_screen,dsr_ui_activity_open};
  for(unsigned i=0;i<6;i++)system_row(body,names[i],(int)i*62,54,icons[i],callbacks[i]);
  return;
 }
 row(body,"Firmware Update",0,dsr_ui_native_ota_open);row(body,"Informacje i diagnostyka",58,system_info);
 row(body,"Hardware tests",116,hardware);row(body,"Konfiguracja wpisów i scen",174,dsr_ui_config_registry);
 row(body,"Restart pilota",232,restart_screen);
 row(body,"Aktywność",290,dsr_ui_activity_open);
}
void dsr_ui_system_open(lv_event_t *event){system_menu(event);}
static void screen_settings(lv_event_t *event)
{
 (void)event;lv_obj_t *body=settings_body("Ekran i komfort");row(body,"Ekran",0,dsr_ui_display_open);
 row(body,"Screen off / touch to wake",58,sleep_display);
 dsr_sc_text(body,"Pierwszy dotyk wybudza bez wykonania akcji.",4,128,284,&dsr_roboto_14,DSR_SC_MUTED);
}
static void menu(lv_event_t *event)
{
 (void)event;lv_obj_t *root=dsr_ui_screen("Ustawienia pilota",true);
 lv_obj_t *body=dsr_sc_panel(root,DSR_SC_MARGIN,86,296,DSR_SC_NAV_TOP-86-DSR_SC_MARGIN,DSR_SC_BG,0);
 /* Six 48 px touch targets and five 6 px gaps fit above navigation. */
 const char *names[]={"Sieć i połączenia","Wygląd i język","Ekran i komfort","Bateria i zasilanie","Panel WWW / bezpieczeństwo","System"};
 const dsr_ui_icon_t icons[]={DSR_ICON_WIFI,DSR_ICON_PALETTE,DSR_ICON_DISPLAY,DSR_ICON_BATTERY,DSR_ICON_SHIELD,DSR_ICON_CPU};
 const lv_event_cb_t callbacks[]={dsr_ui_settings_network,dsr_ui_config_appearance,screen_settings,dsr_ui_battery_open,web_panel,system_menu};
 for(unsigned i=0;i<6;i++){
  lv_obj_t *button=dsr_sc_button(body,0,(int)i*54,296,48,DSR_SC_CARD,DSR_SC_RADIUS,callbacks[i],NULL);
  /* Leaving a fixed row while dragging must cancel its press. */
  lv_obj_remove_flag(button,LV_OBJ_FLAG_PRESS_LOCK);
  bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
  const lv_font_t *font=&dsr_roboto_16;lv_point_t size;
  lv_text_get_size(&size,dsr_sc_tr(names[i]),dsr_sc_font(font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
  if(size.x>196)font=&dsr_roboto_14;
  lv_obj_t *caption=dsr_sc_text(button,names[i],56,0,196,font,DSR_SC_TEXT);
  lv_obj_align(caption,LV_ALIGN_LEFT_MID,56,0);
  if(hud){lv_obj_t *badge=dsr_sc_panel(button,8,6,36,36,DSR_SC_PANEL,8);dsr_ui_cyber_inset(badge);}
  dsr_sc_icon(button,DSR_ICON_NEXT,260,12,24,DSR_SC_MUTED);
  dsr_sc_icon(button,icons[i],14,12,24,DSR_SC_BLUE);
 }
}

void dsr_ui_home(lv_event_t *event) { dsr_network_home(event); }

static void hardware(lv_event_t *event)
{
    (void)event;
    lv_obj_t *root = dsr_ui_screen("Hardware tests", true);
    dsr_sc_header_back(root,dsr_ui_system_open,NULL);
    if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){
     lv_obj_t *info=dsr_sc_panel(root,12,86,296,148,DSR_SC_CARD,10);
     dsr_sc_border_color(info,dsr_sc_color(DSR_SC_GLOW),0);
     lv_obj_t *well=dsr_sc_panel(info,8,10,42,42,DSR_SC_PANEL,8);dsr_ui_cyber_inset(well);
     dsr_sc_icon_centered(well,DSR_ICON_DOCUMENT,32,DSR_SC_GLOW);
     dsr_sc_text(info,"Hardware test",60,15,224,&dsr_roboto_22,DSR_SC_TEXT);
     dsr_sc_text(info,"Display and touch test tools.\nCheck colors, corners and wake.",12,60,272,&dsr_roboto_14,DSR_SC_MUTED);
     dsr_sc_panel(info,12,100,272,1,DSR_SC_BLUE,0);
     dsr_sc_icon(info,DSR_ICON_TOUCH,12,110,24,DSR_SC_GLOW);
     dsr_sc_text(info,dsr_board_touch_available()?"Touch controller: responding":"Touch controller: NOT responding",46,110,238,&dsr_roboto_14,dsr_board_touch_available()?0x72dfb0:0xffb466);
     lv_obj_t *actions=dsr_sc_panel(root,12,244,296,122,DSR_SC_BG,0);
     system_row(actions,"Check touchscreen",0,56,DSR_ICON_TOUCH,touch_test);
     system_row(actions,"Check display colors",66,56,DSR_ICON_GRAPHICS,colors);
     lv_obj_t *board=dsr_sc_panel(root,12,376,296,42,DSR_SC_CARD,10);
     dsr_sc_border_color(board,dsr_sc_color(DSR_SC_GLOW),0);
     dsr_sc_icon(board,DSR_ICON_CPU,12,8,24,DSR_SC_GLOW);
     dsr_sc_text(board,"ES3C35P / 320 x 480",46,11,236,&dsr_roboto_16,DSR_SC_TEXT);
     return;
    }
    dsr_ui_label(root, "Hardware test", 16, 100, 288);
    dsr_ui_label(root, "Display and touch test tools.\nCheck colors, corners and wake.", 16, 137, 288);
    lv_obj_t *status = dsr_ui_label(root, dsr_board_touch_available() ? "Touch controller: responding" : "Touch controller: NOT responding", 16, 200, 288);
    lv_obj_set_style_text_color(status, dsr_sc_color(dsr_board_touch_available() ? 0x72dfb0 : 0xffb466), 0);
    dsr_ui_button(root, "Check touchscreen", 16, 252, 288, touch_test, NULL);
    dsr_ui_button(root, "Check display colors", 16, 316, 288, colors, NULL);
    dsr_ui_label(root, "ES3C35P / 320 x 480", 16, 390, 288);
}

/* Default LVGL widgets (lists, checkboxes, dropdowns) follow the style too. */
void dsr_ui_apply_theme(void)
{
    dsr_style_hint_set((uint8_t)dsr_sc_style()); /* NVS write only on change */
    static int applied=-1;const int light=dsr_sc_style()==DSR_SC_STYLE_LIGHT;
    const int variant=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?2:light;
    if(applied==variant)return;
    lv_display_t *display=lv_display_get_default();if(!display)return;
    lv_theme_t *theme=lv_theme_default_init(display,dsr_sc_color(DSR_SC_BLUE),dsr_sc_color(DSR_SC_BLUE),!light,&dsr_roboto_14);
    if(variant==2)theme=dsr_ui_cyber_theme(theme);
    lv_display_set_theme(display,theme);applied=variant;
}
void dsr_ui_init(void)
{
    dsr_ui_provision_release(); /* first-format screen, if it was shown */
    dsr_ui_splash_release();    /* boot splash, before any screen clean */
    dsr_ui_apply_theme();
    dsr_network_init();dsr_network_start_view();
}
