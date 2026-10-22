#include "dsr_ui_internal.h"
#include "dsr_activity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool active,building,lease,cleanup,back_after_cleanup,waiting,lan_started,inspecting;
static enum {INTRO,WORKING,DISCLOSE,FIND,VERIFY,COMPONENTS,ERROR,CLOSING} phase;
static dsr_ble_device_t selected;
static dsr_ble_rpc_snapshot_t session;
static dsr_lan_device_t found;
static dsr_lan_snapshot_t *lan_copy;
static uint32_t observed,deadline,next_scan;
static void cancel(lv_event_t *event);
static void release(void)
{
 free(lan_copy);lan_copy=NULL;
 if(lease){lease=false;dsr_activity_end();}
 if(!active)dsr_ble_rpc_visibility(false);
}
bool dsr_ui_ble_rpc_active(void){return active||lease||cleanup||waiting;}
void dsr_ui_ble_rpc_reset(void)
{
 if(building)return;
 active=false;
 if(lan_started||inspecting){dsr_lan_cancel();lan_started=inspecting=false;}
 if(lease||waiting||dsr_ble_rpc_active()){
  dsr_ble_rpc_cancel(session.generation);cleanup=true;back_after_cleanup=false;
 }else dsr_ble_rpc_visibility(false);
}
static lv_obj_t *screen(const char *title)
{
 building=true;lv_obj_t *root=dsr_onboarding_screen(title,session.has_identity?3:2);building=false;
 active=true;dsr_ble_rpc_visibility(true);
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *child=lv_obj_get_child(root,(int)i);if(lv_obj_check_type(child,&lv_label_class)&&lv_obj_get_style_y(child,0)==46){lv_label_set_long_mode(child,LV_LABEL_LONG_CLIP);dsr_sc_label_set_text(child,title);break;}}
 return root;
}
static void line(lv_obj_t *root,const char *value,int y,const lv_font_t *font)
{
 lv_obj_t *label=dsr_sc_text(root,value,16,y,288,font,DSR_SC_TEXT);
 lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);lv_obj_set_height(label,26);
}
static void message(const char *title,const char *text)
{
 lv_obj_t *root=screen(title);dsr_sc_text(root,text,16,105,288,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_onboarding_footer(root,"Anuluj",cancel,NULL,NULL);
}
static void failed(dsr_result_t result)
{
 phase=ERROR;
 const char *text=session.phase==DSR_BLE_RPC_QUARANTINED?"Połączenie BLE nie zakończyło się prawidłowo. Ponów po chwili lub uruchom pilot ponownie.":result==DSR_BUSY?"Operacja w toku. Poczekaj.":result==DSR_IDENTITY_MISMATCH?"Tożsamość urządzenia nie zgadza się. Dalsze wysyłanie zablokowane.":
  result==DSR_AUTH_REQUIRED?"Urządzenie wymaga zabezpieczonego BLE lub logowania. Ten kreator obsługuje otwarte okno konfiguracji.":
  result==DSR_UNSUPPORTED?"Wymagany obsługiwany Gen2+ lub H&T Gen3 w otwartym setup oraz HTTP w LAN. Bonding i fabryczne HTTPS 2.0 nie są obsługiwane.":
  "Nie potwierdzono urządzenia w domowej sieci. Zapis Wi-Fi mógł dotrzeć; nie zostanie automatycznie ponowiony.";
 if(dsr_ble_rpc_active()||waiting){dsr_ble_rpc_cancel(session.generation);cleanup=true;back_after_cleanup=false;}
 else release();
 message("Bluetooth • wynik",text);
}
static void leave(void)
{
 active=false;cleanup=back_after_cleanup=false;release();dsr_network_device(NULL);
}
static void cancel(lv_event_t *event)
{
 (void)event;
 if(lan_started||inspecting){dsr_lan_cancel();lan_started=inspecting=false;}
 if(lease||waiting||dsr_ble_rpc_active()){
  dsr_ble_rpc_cancel(session.generation);cleanup=true;back_after_cleanup=true;phase=CLOSING;
  message("Bluetooth • kończenie","Zamykanie połączenia BLE. Zapis Wi-Fi nie będzie ponawiany.");
 }else leave();
}
static void connect_device(lv_event_t *event)
{
 (void)event;if(phase!=INTRO)return;
 if(dsr_network_ota_busy()){failed(DSR_BUSY);return;}
 if(!dsr_network_ap_home(NULL)||dsr_ui_ap_active()){failed(DSR_OFFLINE);return;}
 if(!dsr_activity_begin()){failed(DSR_BUSY);return;}lease=true;
 dsr_result_t result=dsr_ble_rpc_start(&selected);
 if(result!=DSR_OK){release();failed(result);return;}
 dsr_ble_rpc_snapshot(&session);observed=0;phase=WORKING;
 message("Sprawdź urządzenie BLE","Łączenie GATT i odczyt tożsamości oraz możliwości. Dane domowego Wi-Fi nie są jeszcze wysyłane.");
}
static void apply(lv_event_t *event)
{
 (void)event;if(phase!=DISCLOSE)return;
 char home[33]={0};bool valid=dsr_network_ap_home(home);
 dsr_result_t result=valid?dsr_ble_rpc_confirm(session.generation,home):DSR_OFFLINE;
 memset(home,0,sizeof(home));
 if(result!=DSR_OK){failed(result);return;}
 phase=WORKING;message("Dołączanie przez BLE","Ponowne sprawdzenie setup i tożsamości, jeden zapis Wi-Fi, a następnie odczyt stanu. Pilot pozostaje w domowej sieci.");
}
static void disclose(void)
{
 phase=DISCLOSE;deadline=lv_tick_get()+120000;lv_obj_t *root=screen("Przekazać dane Wi-Fi?");
 line(root,session.device.model,104,&dsr_roboto_18);line(root,session.device.id,137,&dsr_roboto_12);
 char home[33]={0};(void)dsr_network_ap_home(home);line(root,home,173,&dsr_roboto_16);memset(home,0,sizeof(home));
 dsr_sc_text(root,"Przekaż nazwę i hasło zapisanej sieci temu Shelly. Otwarte GATT nie uwierzytelnia kryptograficznie urządzenia; użyj tylko własnego sprzętu.",16,205,288,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(root,"Osobne potwierdzenie na pilocie. Podgląd WWW jest wstrzymany.",16,330,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Anuluj",cancel,"Przekaż Wi-Fi",apply);
}
void dsr_ui_ble_rpc_open(const dsr_ble_device_t *candidate)
{
 if(lease||waiting||cleanup||dsr_ble_rpc_active()){
  cleanup=true;back_after_cleanup=false;dsr_ble_rpc_cancel(session.generation);phase=CLOSING;
  message("Bluetooth • kończenie","Zamykanie poprzedniej sesji. Poczekaj na potwierdzenie rozłączenia.");return;
 }
 if(!candidate||candidate->kind!=DSR_BLE_SHELLY_RPC||!(candidate->evidence&DSR_BLE_RPC_HINTS))return;
 if(dsr_ui_ap_active())return;
 selected=*candidate;memset(&session,0,sizeof(session));observed=0;phase=INTRO;
 lv_obj_t *root=screen("Dodać przez Bluetooth?");
 line(root,selected.name[0]?selected.name:"Shelly • konfiguracja BLE",100,&dsr_roboto_18);
 dsr_sc_text(root,"Pilot pozostaje w domowej sieci. Najpierw sprawdzi rzeczywiste GATT i tożsamość. Samo wykrycie urządzenia BLE nie potwierdza autentyczności.",16,151,288,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(root,"Tylko obsługiwany Shelly Gen2+ lub H&T Gen3 w otwartym setup. Hasło Wi-Fi wymaga kolejnej zgody.",16,294,288,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Anuluj",cancel,"Sprawdź BLE",connect_device);
}
void dsr_ui_ble_rpc_result(const dsr_event_t *event)
{
 waiting=false;if(cleanup||!active)return;
 if(phase!=VERIFY)return;
 if(event->result!=DSR_OK){failed(event->result);return;}
 dsr_credentials_t credentials={0};
 if(!dsr_lan_inspect(found.host,found.id,&credentials)){failed(DSR_BUSY);return;}
 phase=COMPONENTS;inspecting=true;observed=0;
}
static bool same(const dsr_lan_device_t *a,const dsr_lan_device_t *b)
{return a->generation==b->generation&&!strcmp(a->id,b->id)&&!strcmp(a->mac,b->mac);}
void dsr_ui_ble_rpc_tick(void)
{
 if(cleanup){
  if(dsr_ble_rpc_active()||waiting)return;
  bool back=back_after_cleanup;cleanup=false;release();
  if(back)leave();else if(active&&phase==CLOSING){active=false;dsr_ble_rpc_visibility(false);dsr_ui_ble_open(NULL);}
  return;
 }
 if(!active)return;
 uint32_t now=lv_tick_get();
 if(phase==DISCLOSE&&(int32_t)(now-deadline)>=0){cancel(NULL);return;}
 if(phase==WORKING||phase==DISCLOSE){
  dsr_ble_rpc_snapshot_t current;dsr_ble_rpc_snapshot(&current);
  if(current.generation!=session.generation){failed(DSR_IDENTITY_MISMATCH);return;}
  if(current.serial==observed)return;
  session=current;observed=current.serial;
  if(current.phase==DSR_BLE_RPC_CONFIRM){if(phase!=DISCLOSE)disclose();return;}
  if(current.active)return;
  if(current.phase!=DSR_BLE_RPC_LAN_READY){failed(current.result);return;}
  phase=FIND;deadline=now+45000;next_scan=now;lan_started=false;observed=0;
  message("Sprawdź w domowej sieci","Szukanie tej samej tożsamości oraz odczyt SSID i IP przez HTTP w LAN. Sam zapis Wi-Fi nie oznacza dodania.");
 }
 if(phase==FIND||phase==COMPONENTS){
  if((int32_t)(now-deadline)>=0){failed(DSR_OFFLINE);return;}
  if(phase==FIND&&!lan_started){if((int32_t)(now-next_scan)<0)return;next_scan=now+1000;lan_started=dsr_lan_scan();observed=0;return;}
  if(!lan_copy)lan_copy=calloc(1,sizeof(*lan_copy));
  if(!lan_copy){failed(DSR_IO);return;}
  dsr_lan_snapshot(lan_copy);if(lan_copy->serial==observed)return;observed=lan_copy->serial;
  if(phase==FIND){
   if(lan_copy->phase==DSR_LAN_SCANNING)return;
   for(unsigned i=0;i<lan_copy->count;i++)if(same(&lan_copy->devices[i],&session.device)){
    found=lan_copy->devices[i];dsr_result_t result=dsr_network_request_station(&found);
    if(result==DSR_BUSY){observed=0;return;}if(result!=DSR_OK){failed(result);return;}
    waiting=true;phase=VERIFY;return;
   }
   lan_started=false;next_scan=now+1000;
  }else{
   if(lan_copy->phase==DSR_LAN_INSPECTING)return;
   inspecting=false;
   if(lan_copy->phase!=DSR_LAN_INSPECTED){failed(lan_copy->result);return;}
   if(!same(&lan_copy->selected,&found)||strcmp(lan_copy->selected.host,found.host)){failed(DSR_IDENTITY_MISMATCH);return;}
   if(!lan_copy->capabilities.switches&&!lan_copy->capabilities.covers&&!lan_copy->capabilities.lights&&!lan_copy->capabilities.rgbs&&!lan_copy->capabilities.ht){failed(DSR_UNSUPPORTED);return;}
   dsr_lan_capabilities_t caps=lan_copy->capabilities;active=false;release();dsr_ui_lan_adopt(&found,&caps);
  }
 }
}
