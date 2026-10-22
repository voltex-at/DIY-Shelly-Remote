#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_ble.h"
#include <stdio.h>
#include <string.h>

static bool active,building,detail,from_settings;
static unsigned selected;
static uint32_t observed;
static dsr_ble_snapshot_t scan;
static dsr_ble_device_t selected_device;
static lv_obj_t *notice,*progress,*counter;
static void show_results(void);
static void show_scanning(void);
static void start(lv_event_t *event);
bool dsr_ui_ble_active(void){return active;}
void dsr_ui_ble_reset(void)
{if(active&&!building)dsr_ble_cancel();active=false;notice=progress=counter=NULL;}
static lv_obj_t *screen(void)
{building=true;lv_obj_t *root=dsr_onboarding_screen("Bluetooth LE",2);building=false;active=true;return root;}
static void leave(lv_event_t *event){(void)event;dsr_ble_cancel();if(from_settings)dsr_ui_settings_network(NULL);else dsr_network_device(NULL);}
static void stop(lv_event_t *event){(void)event;dsr_ble_cancel();if(notice)dsr_sc_label_set_text(notice,"Zatrzymywanie skanu…");}
static void back(lv_event_t *event){(void)event;detail=false;show_results();}
static void provision(lv_event_t *event){(void)event;dsr_ui_ble_rpc_open(&selected_device);}
static void pair_blu(lv_event_t *event){(void)event;dsr_ui_blu_open(&selected_device);}
/* Title: BLU model name, else the advertised local name (Gen2+ Shelly sends
 * e.g. "ShellyPlus1PM-…"), else a Shelly manufacturer hint, else unknown. */
static const char *title(const dsr_ble_device_t *d)
{
 static char text[48];const char *model=dsr_ble_model_name(d);if(model[0])return model;
 if(d->name[0])return d->name;
 if(d->evidence&DSR_BLE_MFG){if(d->has_model)snprintf(text,sizeof(text),dsr_sc_tr("Shelly • model 0x%04X"),(unsigned)d->model_id);else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Shelly • model nieznany"));return text;}
 return dsr_sc_tr(d->kind==DSR_BLE_SHELLY_RPC?"Shelly • konfiguracja BLE":"Nierozpoznane urządzenie");
}
/* Shelly MAC from the manufacturer block when present, else the radio address
 * (NimBLE stores it little-endian). Discovery hint only, not authenticated. */
static void address_text(const dsr_ble_device_t *d,char *out,size_t size)
{
 const uint8_t *a=d->address;
 if(d->has_mac)snprintf(out,size,"MAC %02X:%02X:%02X:%02X:%02X:%02X",d->mac[0],d->mac[1],d->mac[2],d->mac[3],d->mac[4],d->mac[5]);
 else snprintf(out,size,dsr_sc_tr("Adres BLE %02X:%02X:%02X:%02X:%02X:%02X"),a[5],a[4],a[3],a[2],a[1],a[0]);
}
static const char *rpc_state(const dsr_ble_device_t *d)
{return d->kind==DSR_BLE_SHELLY_BLU?"Shelly BLU • czujnik/przycisk":d->has_flags?((d->flags&DSR_BLE_FLAG_RPC)?"RPC przez BLE: włączone":"RPC przez BLE: wyłączone"):d->kind==DSR_BLE_SHELLY_RPC?"RPC: do sprawdzenia po połączeniu":"Nierozpoznane urządzenie BLE";}
static const char *kind(const dsr_ble_device_t *d)
{return d->kind==DSR_BLE_SHELLY_RPC?"Shelly • konfiguracja BLE":d->kind==DSR_BLE_SHELLY_BLU?"Shelly BLU • urządzenie BLE":(d->evidence&DSR_BLE_BTHOME)?"BTHome • producent nieznany":"Nierozpoznane urządzenie BLE";}
/* Short form for the 320 px list row; the detail shows the full sentence. */
static const char *rpc_short(const dsr_ble_device_t *d)
{return d->kind==DSR_BLE_SHELLY_BLU?"Shelly BLU":d->has_flags?((d->flags&DSR_BLE_FLAG_RPC)?"RPC BLE: tak":"RPC BLE: nie"):d->kind==DSR_BLE_SHELLY_RPC?"RPC do sprawdzenia":"Inne urządzenie";}
static void truncated(lv_obj_t *o,int height){lv_obj_set_height(o,height);lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP);}
static void choose(lv_event_t *event)
{
 selected=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(selected>=scan.count)return;detail=true;
 selected_device=scan.devices[selected];const dsr_ble_device_t *d=&selected_device;lv_obj_t *root=screen();
 lv_obj_t *card=dsr_sc_panel(root,12,104,296,98,DSR_SC_CARD,14);dsr_sc_icon(card,d->kind==DSR_BLE_SHELLY_BLU?DSR_ICON_CONTACT:DSR_ICON_PLUG,10,23,32,dsr_sc_icon_accent());
 truncated(dsr_sc_text(card,title(d),54,10,230,&dsr_roboto_14,DSR_SC_TEXT),22);
 char advertised[96];address_text(d,advertised,sizeof(advertised));
 truncated(dsr_sc_text(card,advertised,54,37,230,&dsr_roboto_12,DSR_SC_MUTED),18);
 truncated(dsr_sc_text(card,kind(d),54,65,230,&dsr_roboto_12,DSR_SC_MUTED),18);
 dsr_sc_text(root,"Wykryto • nie sparowano",16,217,288,&dsr_roboto_16,DSR_SC_BLUE);
 char text[96];snprintf(text,sizeof(text),"RSSI %d dBm",d->rssi);dsr_sc_chip(root,text,16,249,200,DSR_SC_TEXT);
 const int capability_y=277;
 if(d->evidence&DSR_BLE_BTHOME)dsr_sc_chip(root,d->encrypted?"BTHome v2 • szyfrowane":"BTHome v2 • jawne",16,capability_y,288,DSR_SC_TEXT);
 else if(d->kind!=DSR_BLE_OTHER||d->has_flags)dsr_sc_chip(root,rpc_state(d),16,capability_y,288,(d->has_flags&&!(d->flags&DSR_BLE_FLAG_RPC))?0xffc18c:DSR_SC_TEXT);
 if(d->has_flags&&(d->flags&(DSR_BLE_FLAG_AUTH|DSR_BLE_FLAG_PAIRING))){char extra[80];snprintf(extra,sizeof(extra),"%s%s%s",(d->flags&DSR_BLE_FLAG_AUTH)?dsr_sc_tr("Hasło włączone"):"",(d->flags&DSR_BLE_FLAG_AUTH)&&(d->flags&DSR_BLE_FLAG_PAIRING)?" • ":"",(d->flags&DSR_BLE_FLAG_PAIRING)?dsr_sc_tr("Tryb parowania"):"");dsr_sc_chip(root,extra,16+212,249,76,DSR_SC_MUTED);}
 notice=dsr_sc_text(root,"",16,310,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(root,d->kind==DSR_BLE_SHELLY_RPC?"Otwarty setup Gen2+: sprawdź urządzenie, a następnie osobno potwierdź przekazanie Wi-Fi.":d->kind==DSR_BLE_SHELLY_BLU?"Sparuj czujnik, ustaw jego parametry i wybierz bramkę Shelly.":"Nie potwierdzono modelu ani RPC. Tego urządzenia nie można dodać do Wi-Fi przez BLE.",16,347,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Wstecz",back,d->kind==DSR_BLE_SHELLY_RPC?"Dodaj przez BLE":d->kind==DSR_BLE_SHELLY_BLU?"Paruj BLU":NULL,d->kind==DSR_BLE_SHELLY_RPC?provision:d->kind==DSR_BLE_SHELLY_BLU?pair_blu:NULL);
}
static void show_results(void)
{
 detail=false;dsr_ble_snapshot(&scan);observed=scan.serial;lv_obj_t *root=screen();
 lv_obj_t *list=dsr_sc_panel(root,12,104,296,253,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);int y=0;
 const char *groups[]={"Shelly • RPC / konfiguracja","Shelly BLU","Pozostałe urządzenia"};
 for(unsigned group=0;group<3;group++){
  bool heading=false;
  for(unsigned i=0;i<scan.count;i++){
   const dsr_ble_device_t *d=&scan.devices[i];if((unsigned)d->kind!=group)continue;
   if(!heading){dsr_sc_text(list,groups[group],4,y,288,&dsr_roboto_12,DSR_SC_MUTED);y+=26;heading=true;}
   char advertised[80],status[96];address_text(d,advertised,sizeof(advertised));
   snprintf(status,sizeof(status),"%d dBm • %s",d->rssi,dsr_sc_tr(rpc_short(d)));
   dsr_onboarding_result(list,y,d->kind==DSR_BLE_SHELLY_BLU?DSR_ICON_CONTACT:DSR_ICON_PLUG,title(d),advertised,status,false,choose,(void *)(uintptr_t)i);y+=96;
  }
 }
 if(!scan.count)dsr_sc_text(list,scan.phase==DSR_BLE_UNAVAILABLE?"Bluetooth niedostępny":"Nie znaleziono urządzeń BLE",12,35,272,&dsr_roboto_18,DSR_SC_TEXT);
 notice=dsr_sc_text(root,scan.phase==DSR_BLE_CANCELLED?"Skan przerwany":scan.phase==DSR_BLE_UNAVAILABLE?"Sprawdź firmware i spróbuj ponownie.":"BLU może wymagać krótkiego naciśnięcia przycisku.",16,369,288,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *retry=dsr_onboarding_footer(root,"Wstecz",leave,"Ponów",start);if(scan.busy)lv_obj_add_state(retry,LV_STATE_DISABLED);
}
static void start(lv_event_t *event)
{
 (void)event;detail=false;
 if(!dsr_ble_scan()){show_results();if(notice&&scan.busy)dsr_sc_label_set_text(notice,"Operacja w toku. Poczekaj.");return;}
 show_scanning();
}
static void show_scanning(void)
{
 dsr_ble_snapshot(&scan);observed=scan.serial;lv_obj_t *root=screen();
 dsr_sc_icon(root,DSR_ICON_CONTACT,132,101,56,DSR_SC_BLUE);
 notice=dsr_sc_text(root,"Wyszukiwanie urządzeń Bluetooth…",16,181,288,&dsr_roboto_18,DSR_SC_TEXT);
 counter=dsr_sc_text(root,"",16,237,288,&dsr_roboto_14,DSR_SC_MUTED);
 progress=lv_bar_create(root);lv_obj_set_pos(progress,16,276);lv_obj_set_size(progress,288,8);lv_bar_set_range(progress,0,20);lv_obj_set_style_bg_color(progress,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);
 dsr_sc_text(root,"Skan trwa do 20 s. Działa także bez Wi-Fi. Naciśnij krótko przycisk BLU, aby wykryć urządzenie.",16,311,288,&dsr_roboto_14,DSR_SC_MUTED);
 if(!dsr_wifi_connected())dsr_sc_text(root,"Wi-Fi offline • BLE dostępne",16,391,288,&dsr_roboto_12,DSR_SC_BLUE);
 dsr_onboarding_footer(root,"Anuluj",stop,"Wstecz",leave);
}
static void open_ble(lv_event_t *event)
{
 (void)event;dsr_ble_snapshot(&scan);detail=false;observed=scan.serial;
 if(scan.busy){show_scanning();return;}
 if(scan.phase!=DSR_BLE_IDLE){show_results();return;}
 lv_obj_t *root=screen();dsr_sc_icon(root,DSR_ICON_CONTACT,132,102,56,DSR_SC_BLUE);
 dsr_sc_text(root,"Wyszukaj urządzenia w pobliżu",16,183,288,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(root,"Urządzenia Shelly ogłaszające RPC oraz Shelly BLU / BTHome. Wykrycie nie oznacza sparowania ani dodania do LAN.",16,248,288,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Wstecz",leave,"Szukaj",start);
}
void dsr_ui_ble_open(lv_event_t *event){from_settings=false;open_ble(event);}
void dsr_ui_ble_scan_open(lv_event_t *event)
{from_settings=false;dsr_ble_snapshot(&scan);if(scan.busy)show_scanning();else start(event);}
void dsr_ui_ble_settings(lv_event_t *event){from_settings=true;open_ble(event);}
void dsr_ui_ble_tick(void)
{
 if(!active)return;
 if(detail){
  if(notice){dsr_ble_snapshot_t latest;dsr_ble_snapshot(&latest);uint32_t age=(latest.snapshot_ms-selected_device.seen_ms)/1000;dsr_sc_label_set_text_fmt(notice,age>30?"Ostatni sygnał urządzenia %u s temu. Możliwy brak zasięgu.":"Ostatni sygnał urządzenia %u s temu.",age);}return;
 }
 dsr_ble_snapshot_t next;dsr_ble_snapshot(&next);
 if(next.phase==DSR_BLE_SCANNING||next.phase==DSR_BLE_STARTING){
  scan=next;if(counter)dsr_sc_label_set_text_fmt(counter,"%u / 20 s • wykryto %u",(unsigned)(scan.elapsed_ms/1000),scan.count);
  if(progress)lv_bar_set_value(progress,(int)(scan.elapsed_ms/1000),LV_ANIM_OFF);
  return;
 }
 if(next.serial!=observed){scan=next;show_results();}
}
