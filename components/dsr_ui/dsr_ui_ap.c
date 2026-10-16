#include "dsr_ui_internal.h"
#include "dsr_workspace.h"
#include "dsr_activity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool active,building,lease,cleanup,waiting,return_after_restore,lan_started,waiting_lan_inspect;
static enum {AP_INTRO,AP_SCAN,AP_PICK,AP_CONNECT_CONFIRM,AP_CONNECTING,AP_DISCLOSE,AP_APPLY,AP_FIND,AP_VERIFY,AP_COMPONENTS,AP_RESTORING,AP_ERROR} phase;
static dsr_wifi_scan_t scan;
static dsr_wifi_ap_t selected;
static dsr_ap_identity_t identity;
static dsr_lan_device_t found;
static uint32_t deadline,observed,next_scan,cleanup_retry_at;
static lv_obj_t *notice;
static dsr_lan_snapshot_t *lan_copy;
static dsr_result_t deferred_error=DSR_OK;
static dsr_result_t request(dsr_job_kind_t kind,const dsr_wifi_ap_t *ap,const dsr_ap_identity_t *device,const char *host,bool confirmed)
{dsr_result_t result=dsr_network_request_ap(kind,ap,device,host,confirmed);if(result==DSR_OK)waiting=true;return result;}
static void show_scan(lv_event_t *event);
static void cancel(lv_event_t *event);
static void show_disclose(void);
static void line(lv_obj_t *root,const char *text,int y,const lv_font_t *font,uint32_t color)
{lv_obj_t *label=dsr_sc_text(root,text,16,y,288,font,color);lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);lv_obj_set_height(label,26);}
static void release(void)
{free(lan_copy);lan_copy=NULL;if(lease){lease=false;dsr_activity_end();}if(!active)dsr_ap_visibility(false);}
bool dsr_ui_ap_active(void){return active||cleanup||lease;}
void dsr_ui_ap_reset(void)
{
 active=false;notice=NULL;
 if(!building&&(lan_started||waiting_lan_inspect)){dsr_lan_cancel();lan_started=waiting_lan_inspect=false;}
 if(!building&&(waiting||lease||dsr_wifi_temporary_active())){cleanup=true;cleanup_retry_at=lv_tick_get();return_after_restore=false;dsr_ap_cancel();}
 if(!building&&!cleanup)dsr_ap_visibility(false);
}
static lv_obj_t *screen(const char *title)
{
 building=true;lv_obj_t *root=dsr_ui_screen(title,false);building=false;active=true;dsr_ap_visibility(true);
 /* These short translated headings fit one line. Avoid transient ellipsis
  * mutation while the newly created screen has not completed its layout. */
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *child=lv_obj_get_child(root,(int)i);if(lv_obj_check_type(child,&lv_label_class)&&lv_obj_get_y(child)==46){lv_label_set_long_mode(child,LV_LABEL_LONG_CLIP);dsr_sc_label_set_text(child,title);break;}}
 return root;
}
static void message(const char *title,const char *text)
{lv_obj_t *root=screen(title);notice=dsr_sc_text(root,text,16,105,288,&dsr_roboto_16,DSR_SC_TEXT);dsr_ui_button(root,"Anuluj",12,420,296,cancel,NULL);}
static void error(dsr_result_t result)
{
 if(dsr_wifi_temporary_active()){
  deferred_error=result;cleanup=true;cleanup_retry_at=lv_tick_get();return_after_restore=false;phase=AP_RESTORING;dsr_ap_cancel();message("Powrót do domu","Przywracanie domowego Wi-Fi. Nie wyłączaj pilota.");return;
 }
 phase=AP_ERROR;
 const char *text=result==DSR_IDENTITY_MISMATCH?"Tożsamość urządzenia nie zgadza się. Dalsze wysyłanie zablokowane.":result==DSR_AUTH_REQUIRED?"AP wymaga logowania. Obsługiwane są fabryczne AP bez hasła API.":result==DSR_UNSUPPORTED?"Ten AP lub tryb Wi-Fi nie jest obsługiwany. Dane pilota zachowane.":"Nie potwierdzono dodania. Domowe ustawienia pilota zachowane.";
 message("Wi-Fi AP • wynik",text);
 if(!dsr_wifi_temporary_active())release();
}
static void leave(void)
{active=false;cleanup=false;return_after_restore=false;release();memset(&identity,0,sizeof(identity));memset(&selected,0,sizeof(selected));dsr_network_device(NULL);}
static void cancel(lv_event_t *event)
{
 (void)event;
 if(lan_started||waiting_lan_inspect){dsr_lan_cancel();lan_started=waiting_lan_inspect=false;}
 if(lease||dsr_wifi_temporary_active()){
  dsr_ap_cancel();cleanup=true;cleanup_retry_at=lv_tick_get();return_after_restore=true;phase=AP_RESTORING;
  message("Powrót do domu","Przywracanie domowego Wi-Fi. Nie wyłączaj pilota.");
 }else if(dsr_network_busy()){cleanup=true;cleanup_retry_at=lv_tick_get();return_after_restore=true;phase=AP_RESTORING;message("Wi-Fi AP","Kończenie operacji…");}
 else leave();
}
static void connect_ap(lv_event_t *event)
{
 (void)event;
 if(!dsr_wifi_connected()){error(DSR_OFFLINE);return;}
 if(!dsr_activity_begin()){error(DSR_BUSY);return;}lease=true;
 dsr_result_t result=request(DSR_JOB_AP_INSPECT,&selected,NULL,NULL,true);
 if(result!=DSR_OK){release();error(result);return;}
 phase=AP_CONNECTING;deadline=lv_tick_get()+120000;
 message("Sprawdź punkt dostępowy","Łączenie z wybranym AP i odczyt tożsamości. Dane domowego Wi-Fi nie są jeszcze wysyłane.");
}
static void choose(lv_event_t *event)
{
 unsigned index=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(index>=scan.count)return;selected=scan.items[index];
 if(selected.security!=DSR_WIFI_OPEN){error(DSR_UNSUPPORTED);return;}
 bool valid=dsr_network_ap_home(NULL);if(!valid){error(DSR_OFFLINE);return;}
 phase=AP_CONNECT_CONFIRM;lv_obj_t *root=screen("Połączyć z AP?");
 line(root,selected.ssid,100,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(root,"Pilot tymczasowo opuści domową sieć. Sterowanie Shelly i Web Panel będą niedostępne.",16,154,288,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(root,"Wybierz własne urządzenie. Sama nazwa AP nie potwierdza jego autentyczności.",16,263,288,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(root,"Anuluj",12,420,140,cancel,NULL);dsr_ui_button(root,"Połącz z AP",168,420,140,connect_ap,NULL);
}
static void show_results(void)
{
 phase=AP_PICK;lv_obj_t *root=screen("Fabryczne AP Shelly");lv_obj_t *list=dsr_sc_panel(root,12,88,296,274,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);
 unsigned count=0;
 for(unsigned i=0;i<scan.count;i++)if(dsr_ap_candidate(scan.items[i].ssid)){
  lv_obj_t *row=dsr_sc_button(list,0,(int)count++*84,296,76,DSR_SC_CARD,10,choose,(void *)(uintptr_t)i);
  lv_obj_t *ssid=dsr_sc_text(row,scan.items[i].ssid,12,10,272,&dsr_roboto_14,DSR_SC_TEXT);lv_label_set_long_mode(ssid,LV_LABEL_LONG_CLIP);lv_obj_set_height(ssid,22);
  dsr_sc_text(row,scan.items[i].security==DSR_WIFI_OPEN?"Otwarty AP • do sprawdzenia":"AP chroniony • nieobsługiwany",12,40,272,&dsr_roboto_12,DSR_SC_MUTED);
 }
 if(!count)dsr_sc_text(list,"Nie znaleziono fabrycznego AP Shelly. Włącz tryb konfiguracji urządzenia.",8,30,280,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_ui_button(root,"Anuluj",12,420,140,cancel,NULL);dsr_ui_button(root,"Ponów",168,420,140,show_scan,NULL);
}
static void show_scan(lv_event_t *event)
{
 (void)event;dsr_result_t result=request(DSR_JOB_AP_SCAN,NULL,NULL,NULL,false);
 if(result!=DSR_OK){error(result);return;}phase=AP_SCAN;message("Fabryczne AP Shelly","Skanowanie Wi-Fi. Pilot pozostaje w domowej sieci.");
}
void dsr_ui_ap_open(lv_event_t *event)
{
 (void)event;
 if(waiting||lease||cleanup||dsr_wifi_temporary_active()){
  cleanup=true;cleanup_retry_at=lv_tick_get();return_after_restore=false;phase=AP_RESTORING;dsr_ap_cancel();message("Powrót do domu","Kończenie poprzedniej operacji i przywracanie domowego Wi-Fi.");return;
 }
 deferred_error=DSR_OK;phase=AP_INTRO;cleanup=return_after_restore=lan_started=waiting_lan_inspect=false;memset(&scan,0,sizeof(scan));memset(&identity,0,sizeof(identity));
 lv_obj_t *root=screen("Dodaj przez Wi-Fi AP");
 dsr_sc_text(root,"Fabryczne urządzenie Shelly",16,99,288,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(root,"Najpierw wyszukamy AP i sprawdzimy lokalne API. Dane domowej sieci wyślesz dopiero po osobnym potwierdzeniu na ekranie.",16,148,288,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(root,"Pilot musi mieć działające domowe Wi-Fi. Obsługiwane są otwarte, fabryczne AP bez hasła API.",16,281,288,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(root,"Anuluj",12,420,140,cancel,NULL);dsr_ui_button(root,"Szukaj AP",168,420,140,show_scan,NULL);
}
static void apply(lv_event_t *event)
{
 (void)event;if(phase!=AP_DISCLOSE)return;
 dsr_result_t result=request(DSR_JOB_AP_PROVISION,NULL,&identity,NULL,true);
 if(result!=DSR_OK){error(result);return;}phase=AP_APPLY;message("Dołączanie do domu","Wysyłanie konfiguracji jeden raz. Następnie pilot przywróci domowe Wi-Fi i sprawdzi urządzenie w LAN.");
}
static void show_disclose(void)
{
 phase=AP_DISCLOSE;deadline=lv_tick_get()+120000;lv_obj_t *root=screen("Przekazać dane Wi-Fi?");
 line(root,identity.model,92,&dsr_roboto_18,DSR_SC_TEXT);line(root,identity.id,125,&dsr_roboto_12,DSR_SC_MUTED);
 char home_ssid[33]={0};(void)dsr_network_ap_home(home_ssid);line(root,home_ssid,161,&dsr_roboto_16,DSR_SC_TEXT);memset(home_ssid,0,sizeof(home_ssid));
 dsr_sc_text(root,"Przekaż nazwę i hasło zapisanej domowej sieci temu urządzeniu. Otwarty AP i HTTP nie zapewniają kryptograficznego uwierzytelnienia.",16,205,288,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(root,"Potwierdź tylko dla własnego Shelly. Podgląd WWW jest wstrzymany.",16,324,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_ui_button(root,"Anuluj",12,420,140,cancel,NULL);dsr_ui_button(root,"Przekaż Wi-Fi",168,420,140,apply,NULL);
}
void dsr_ui_ap_result(const dsr_event_t *event,dsr_job_kind_t kind)
{
 waiting=false;
 if(kind==DSR_JOB_AP_RESTORE){
  bool back=return_after_restore;cleanup=false;
  if(event->result!=DSR_OK){
   if(!dsr_wifi_temporary_active())release();
   if(active){phase=AP_ERROR;deadline=lv_tick_get()+120000;message("Powrót do domu","Nie przywrócono połączenia. Ustawienia zapisane bez zmian. Anuluj ponawia powrót do domu.");}
   else if(dsr_wifi_temporary_active()){cleanup=true;cleanup_retry_at=lv_tick_get()+1000;}
   return;
  }
  if(back){leave();return;}release();
  if(active&&deferred_error!=DSR_OK){dsr_result_t failed=deferred_error;deferred_error=DSR_OK;error(failed);}else if(active)dsr_ui_ap_open(NULL);return;
 }
 if(cleanup)return;
 if(!active)return;
 if(kind==DSR_JOB_AP_SCAN){if(event->result!=DSR_OK){error(event->result);return;}scan=event->scan;show_results();}
 else if(kind==DSR_JOB_AP_INSPECT){if(event->result!=DSR_OK){error(event->result);return;}identity=event->ap_identity;show_disclose();}
 else if(kind==DSR_JOB_AP_PROVISION){
  if(event->result!=DSR_OK){error(event->result);return;}
  if(event->ap_write_result==DSR_AUTH_REQUIRED||event->ap_write_result==DSR_IDENTITY_MISMATCH||event->ap_write_result==DSR_INVALID||event->ap_write_result==DSR_UNSUPPORTED){error(event->ap_write_result);return;}
  phase=AP_FIND;lan_started=false;waiting_lan_inspect=false;deadline=lv_tick_get()+45000;next_scan=lv_tick_get();
  message("Sprawdź w domowej sieci","Pilot wrócił do domu. Szukanie tej samej tożsamości w LAN; sam zapis konfiguracji nie oznacza sukcesu.");
 }else if(kind==DSR_JOB_AP_VERIFY){
  if(event->result!=DSR_OK){error(event->result);return;}
  dsr_credentials_t credentials={0};if(!dsr_lan_inspect(found.host,found.id,&credentials)){error(DSR_BUSY);return;}
  phase=AP_COMPONENTS;waiting_lan_inspect=true;observed=0;
 }
}
void dsr_ui_ap_tick(void)
{
 if(cleanup){if(!dsr_network_busy()&&(int32_t)(lv_tick_get()-cleanup_retry_at)>=0){
  if(lease||dsr_wifi_temporary_active())(void)request(DSR_JOB_AP_RESTORE,NULL,NULL,NULL,false);
  else if(return_after_restore)leave();else{cleanup=false;release();if(active)dsr_ui_ap_open(NULL);}
 }return;}
 if(!active)return;
 uint32_t now=lv_tick_get();if((phase==AP_DISCLOSE||(phase==AP_ERROR&&dsr_wifi_temporary_active()))&&(int32_t)(now-deadline)>=0){cancel(NULL);return;}
 if(phase==AP_FIND){
  if((int32_t)(now-deadline)>=0){error(DSR_OFFLINE);return;}
  if(!lan_started){if((int32_t)(now-next_scan)<0)return;next_scan=now+1000;lan_started=dsr_lan_scan();observed=0;return;}
  if(!lan_copy)lan_copy=calloc(1,sizeof(*lan_copy));
  if(!lan_copy){error(DSR_IO);return;}
  dsr_lan_snapshot(lan_copy);const dsr_lan_snapshot_t *snapshot=lan_copy;if(snapshot->serial==observed)return;observed=snapshot->serial;
  if(snapshot->phase==DSR_LAN_SCANNING)return;
  for(unsigned i=0;i<snapshot->count;i++)if(!strcmp(snapshot->devices[i].id,identity.id)&&!strcmp(snapshot->devices[i].mac,identity.mac)&&snapshot->devices[i].generation==identity.generation){
   found=snapshot->devices[i];dsr_result_t result=request(DSR_JOB_AP_VERIFY,NULL,&identity,found.host,false);if(result==DSR_BUSY){observed=0;return;}if(result!=DSR_OK){error(result);return;}phase=AP_VERIFY;return;
  }
  lan_started=false;next_scan=now+1000;
 }else if(phase==AP_COMPONENTS&&waiting_lan_inspect){
  if(!lan_copy)lan_copy=calloc(1,sizeof(*lan_copy));
  if(!lan_copy){error(DSR_IO);return;}
  dsr_lan_snapshot(lan_copy);const dsr_lan_snapshot_t *snapshot=lan_copy;if(snapshot->serial==observed)return;observed=snapshot->serial;if(snapshot->phase==DSR_LAN_INSPECTING)return;
  waiting_lan_inspect=false;
  if(snapshot->phase!=DSR_LAN_INSPECTED){error(snapshot->result);return;}
  if(strcmp(snapshot->selected.id,found.id)||strcmp(snapshot->selected.mac,found.mac)||strcmp(snapshot->selected.host,found.host)||snapshot->selected.generation!=found.generation){error(DSR_IDENTITY_MISMATCH);return;}
  dsr_lan_capabilities_t caps=snapshot->capabilities;active=false;release();dsr_ui_lan_adopt(&found,&caps);
 }
}
