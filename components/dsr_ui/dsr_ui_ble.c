#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_ble.h"
#include <stdio.h>
#include <string.h>

static bool active,building,detail;
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
{building=true;lv_obj_t *root=dsr_ui_screen("Bluetooth LE",false);building=false;active=true;return root;}
static void leave(lv_event_t *event){(void)event;dsr_ble_cancel();dsr_network_device(NULL);}
static void stop(lv_event_t *event){(void)event;dsr_ble_cancel();if(notice)dsr_sc_label_set_text(notice,"Zatrzymywanie skanu…");}
static void back(lv_event_t *event){(void)event;detail=false;show_results();}
static void lan(lv_event_t *event){dsr_ui_lan_open(event);}
static const char *title(const dsr_ble_device_t *d)
{const char *model=dsr_ble_model_name(d);return model[0]?model:d->name[0]?d->name:dsr_sc_tr("Nieznany model");}
static const char *kind(const dsr_ble_device_t *d)
{return d->kind==DSR_BLE_SHELLY_RPC?"Shelly • reklamowane RPC":d->kind==DSR_BLE_SHELLY_BLU?"Shelly BLU • reklama BLE":(d->evidence&DSR_BLE_BTHOME)?"BTHome • producent nieznany":"Niezidentyfikowana reklama";}
static void truncated(lv_obj_t *o,int height){lv_obj_set_height(o,height);lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_DOTS);}
static void choose(lv_event_t *event)
{
 selected=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(selected>=scan.count)return;detail=true;
 selected_device=scan.devices[selected];const dsr_ble_device_t *d=&selected_device;lv_obj_t *root=screen();
 lv_obj_t *card=dsr_sc_panel(root,12,88,296,82,DSR_SC_CARD,14);dsr_sc_icon(card,d->kind==DSR_BLE_SHELLY_BLU?DSR_ICON_CONTACT:DSR_ICON_PLUG,10,23,32,DSR_SC_TEXT);
 truncated(dsr_sc_text(card,title(d),54,13,230,&dsr_roboto_16,DSR_SC_TEXT),22);
 truncated(dsr_sc_text(card,kind(d),54,43,230,&dsr_roboto_12,DSR_SC_MUTED),18);
 dsr_sc_text(root,"Wykryto • nie sparowano",16,187,288,&dsr_roboto_16,DSR_SC_BLUE);
 char text[96];snprintf(text,sizeof(text),"RSSI %d dBm",d->rssi);dsr_sc_chip(root,text,16,222,200,DSR_SC_TEXT);
 if(d->evidence&DSR_BLE_BTHOME)dsr_sc_chip(root,d->encrypted?"BTHome v2 • szyfrowane":"BTHome v2 • jawne",16,250,288,DSR_SC_TEXT);
 notice=dsr_sc_text(root,"",16,282,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(root,"Skan nie dodaje do Wi-Fi. Parowanie i zapis BLE nie są jeszcze wdrożone.",16,317,288,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(root,"Wstecz",12,420,140,back,NULL);
 if(d->kind==DSR_BLE_SHELLY_RPC)dsr_ui_button(root,"Szukaj w LAN",168,420,140,lan,NULL);
}
static void show_results(void)
{
 detail=false;dsr_ble_snapshot(&scan);observed=scan.serial;lv_obj_t *root=screen();
 lv_obj_t *list=dsr_sc_panel(root,12,87,296,270,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);int y=0;
 const char *groups[]={"Shelly • RPC / konfiguracja","Shelly BLU","Pozostałe reklamy"};
 for(unsigned group=0;group<3;group++){
  bool heading=false;
  for(unsigned i=0;i<scan.count;i++){
   const dsr_ble_device_t *d=&scan.devices[i];if((unsigned)d->kind!=group)continue;
   if(!heading){dsr_sc_text(list,groups[group],4,y,288,&dsr_roboto_12,DSR_SC_MUTED);y+=26;heading=true;}
   lv_obj_t *card=dsr_sc_button(list,0,y,296,72,DSR_SC_CARD,12,choose,(void *)(uintptr_t)i);y+=80;
   dsr_sc_icon(card,d->kind==DSR_BLE_SHELLY_BLU?DSR_ICON_CONTACT:DSR_ICON_PLUG,10,20,30,DSR_SC_TEXT);
   truncated(dsr_sc_text(card,title(d),52,11,232,&dsr_roboto_14,DSR_SC_TEXT),20);
   char text[72];snprintf(text,sizeof(text),dsr_sc_tr("Wykryto • %d dBm • bez parowania"),d->rssi);truncated(dsr_sc_text(card,text,52,40,232,&dsr_roboto_12,DSR_SC_MUTED),18);
  }
 }
 if(!scan.count)dsr_sc_text(list,scan.phase==DSR_BLE_UNAVAILABLE?"Bluetooth niedostępny":"Nie znaleziono urządzeń BLE",12,35,272,&dsr_roboto_18,DSR_SC_TEXT);
 notice=dsr_sc_text(root,scan.phase==DSR_BLE_CANCELLED?"Skan przerwany":scan.phase==DSR_BLE_UNAVAILABLE?"Sprawdź firmware i spróbuj ponownie.":"BLU może wymagać krótkiego naciśnięcia przycisku.",16,369,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_ui_button(root,"Wstecz",12,420,140,leave,NULL);lv_obj_t *retry=dsr_ui_button(root,"Ponów",168,420,140,start,NULL);if(scan.busy)lv_obj_add_state(retry,LV_STATE_DISABLED);
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
 notice=dsr_sc_text(root,"Odbieranie reklam Bluetooth…",16,181,288,&dsr_roboto_18,DSR_SC_TEXT);
 counter=dsr_sc_text(root,"",16,237,288,&dsr_roboto_14,DSR_SC_MUTED);
 progress=lv_bar_create(root);lv_obj_set_pos(progress,16,276);lv_obj_set_size(progress,288,8);lv_bar_set_range(progress,0,20);lv_obj_set_style_bg_color(progress,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);
 dsr_sc_text(root,"Skan trwa do 20 s. Działa także bez Wi-Fi. Naciśnij krótko przycisk BLU, aby wysłać reklamę.",16,311,288,&dsr_roboto_14,DSR_SC_MUTED);
 if(!dsr_wifi_connected())dsr_sc_text(root,"Wi-Fi offline • BLE dostępne",16,391,288,&dsr_roboto_11,DSR_SC_BLUE);
 dsr_ui_button(root,"Anuluj",12,420,140,stop,NULL);dsr_ui_button(root,"Wstecz",168,420,140,leave,NULL);
}
void dsr_ui_ble_open(lv_event_t *event)
{
 (void)event;dsr_ble_snapshot(&scan);detail=false;observed=scan.serial;
 if(scan.busy){show_scanning();return;}
 if(scan.phase!=DSR_BLE_IDLE){show_results();return;}
 lv_obj_t *root=screen();dsr_sc_icon(root,DSR_ICON_CONTACT,132,102,56,DSR_SC_BLUE);
 dsr_sc_text(root,"Wyszukaj urządzenia w pobliżu",16,183,288,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(root,"Shelly z reklamowanym RPC oraz Shelly BLU / BTHome. Wykrycie nie oznacza sparowania ani dodania do LAN.",16,248,288,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(root,"Wstecz",12,420,140,leave,NULL);dsr_ui_button(root,"Szukaj",168,420,140,start,NULL);
}
void dsr_ui_ble_tick(void)
{
 if(!active)return;
 if(detail){
  if(notice){dsr_ble_snapshot_t latest;dsr_ble_snapshot(&latest);uint32_t age=(latest.snapshot_ms-selected_device.seen_ms)/1000;dsr_sc_label_set_text_fmt(notice,age>30?"Ostatnia reklama %u s temu. Możliwy brak zasięgu.":"Ostatnia reklama %u s temu.",age);}return;
 }
 dsr_ble_snapshot_t next;dsr_ble_snapshot(&next);
 if(next.phase==DSR_BLE_SCANNING||next.phase==DSR_BLE_STARTING){
  scan=next;if(counter)dsr_sc_label_set_text_fmt(counter,"%u / 20 s • wykryto %u",(unsigned)(scan.elapsed_ms/1000),scan.count);
  if(progress)lv_bar_set_value(progress,(int)(scan.elapsed_ms/1000),LV_ANIM_OFF);
  return;
 }
 if(next.serial!=observed){scan=next;show_results();}
}
