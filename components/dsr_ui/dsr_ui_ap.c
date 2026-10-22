#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
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
static void retry(lv_event_t *event);
static void line(lv_obj_t *root,const char *text,int y,const lv_font_t *font,uint32_t color)
{
 bool full=root==lv_screen_active();int width=full?288:268;
 lv_point_t size;lv_text_get_size(&size,text,dsr_sc_font(font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 if(size.x>width)font=&dsr_roboto_12;
 lv_obj_t *label=dsr_sc_text(root,dsr_sc_user(text),full?16:12,y,width,font,color);lv_label_set_long_mode(label,LV_LABEL_LONG_WRAP);
}
static lv_obj_t *content(lv_obj_t *root)
{
 lv_obj_t *card=dsr_sc_panel(root,12,104,296,296,DSR_SC_CARD,12);
 dsr_sc_border_width(card,1,0);dsr_sc_border_color(card,dsr_sc_color(DSR_SC_EDGE),0);
 return card;
}
static void large_icon_draw(lv_event_t *event)
{
 lv_obj_t *icon=lv_event_get_target(event);lv_area_t a;lv_obj_get_coords(icon,&a);lv_layer_t *layer=lv_event_get_layer(event);lv_color_t color=lv_obj_get_style_line_color(icon,0);
 lv_draw_rect_dsc_t dot;lv_draw_rect_dsc_init(&dot);dot.bg_color=color;dot.bg_opa=LV_OPA_COVER;dot.radius=LV_RADIUS_CIRCLE;
 if((uintptr_t)lv_event_get_user_data(event)==DSR_ICON_WIFI){
  lv_draw_arc_dsc_t arc;lv_draw_arc_dsc_init(&arc);arc.color=color;arc.width=4;arc.rounded=1;arc.center=(lv_point_t){a.x1+36,a.y1+58};arc.start_angle=220;arc.end_angle=320;
  for(unsigned r=12;r<=36;r+=12){arc.radius=(uint16_t)r;lv_draw_arc(layer,&arc);}
  lv_area_t point={a.x1+32,a.y1+54,a.x1+39,a.y1+61};lv_draw_rect(layer,&dot,&point);
 }else{
  lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.color=color;line.width=3;line.round_start=line.round_end=1;
  const lv_point_precise_t points[4]={{a.x1+32,a.y1+6},{a.x1+59,a.y1+54},{a.x1+5,a.y1+54},{a.x1+32,a.y1+6}};
  for(unsigned i=0;i<3;i++){line.p1=points[i];line.p2=points[i+1];lv_draw_line(layer,&line);}
  line.width=4;line.p1=(lv_point_precise_t){a.x1+32,a.y1+23};line.p2=(lv_point_precise_t){a.x1+32,a.y1+36};lv_draw_line(layer,&line);
  lv_area_t point={a.x1+30,a.y1+43,a.x1+34,a.y1+47};lv_draw_rect(layer,&dot,&point);
 }
}
static void large_icon(lv_obj_t *card,dsr_ui_icon_t kind,int y,int size,uint32_t color)
{
 if(dsr_sc_style()!=DSR_SC_STYLE_CYBER_BLUE&&kind==DSR_ICON_WIFI){dsr_sc_icon(card,kind,(296-size)/2,y,size,color);return;}
 /* Draw at the final size instead of magnifying the compact 24/32 px mask. */
 lv_obj_t *icon=lv_obj_create(card);lv_obj_remove_style_all(icon);lv_obj_set_size(icon,size,size);lv_obj_set_style_line_color(icon,dsr_sc_text_color(color),0);
 lv_obj_remove_flag(icon,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);lv_obj_align(icon,LV_ALIGN_TOP_MID,0,y);
 lv_obj_add_event_cb(icon,large_icon_draw,LV_EVENT_DRAW_MAIN,(void *)(uintptr_t)kind);
}
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
 building=true;lv_obj_t *root=dsr_onboarding_screen(title,identity.id[0]?3:2);building=false;active=true;dsr_ap_visibility(true);
 /* These short translated headings fit one line. Avoid transient ellipsis
  * mutation while the newly created screen has not completed its layout. */
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *child=lv_obj_get_child(root,(int)i);if(lv_obj_check_type(child,&lv_label_class)&&lv_obj_get_y(child)==46){lv_label_set_long_mode(child,LV_LABEL_LONG_CLIP);dsr_sc_label_set_text(child,title);break;}}
 return root;
}
static void message(const char *title,const char *text)
{
 lv_obj_t *root=screen(title),*card=content(root);lv_obj_t *spinner=lv_spinner_create(card);lv_obj_set_pos(spinner,124,16);lv_obj_set_size(spinner,48,48);lv_spinner_set_anim_params(spinner,1000,200);
 notice=dsr_sc_text(card,text,12,76,268,&dsr_roboto_16,DSR_SC_TEXT);dsr_onboarding_footer(root,"Anuluj",cancel,NULL,NULL);
}
static void error(dsr_result_t result)
{
 if(dsr_wifi_temporary_active()){
  deferred_error=result;cleanup=true;cleanup_retry_at=lv_tick_get();return_after_restore=false;phase=AP_RESTORING;dsr_ap_cancel();message("Powrót do domu","Przywracanie domowego Wi-Fi. Nie wyłączaj pilota.");return;
 }
 phase=AP_ERROR;
 if(lan_started||waiting_lan_inspect){dsr_lan_cancel();lan_started=waiting_lan_inspect=false;}
 const char *text=result==DSR_IDENTITY_MISMATCH?"Tożsamość urządzenia nie zgadza się. Dalsze wysyłanie zablokowane.":result==DSR_AUTH_REQUIRED?"AP wymaga logowania. Obsługiwane są fabryczne AP bez hasła API.":result==DSR_UNSUPPORTED?"Ten AP lub tryb Wi-Fi nie jest obsługiwany. Dane pilota zachowane.":"Nie potwierdzono dodania. Domowe ustawienia pilota zachowane.";
 lv_obj_t *root=screen("Wi-Fi AP • wynik"),*card=content(root);uint32_t gold=dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xf0a020:0xffc107;
 large_icon(card,DSR_ICON_WARNING,8,64,gold);
 lv_obj_t *warning=dsr_sc_panel(card,12,84,268,24,DSR_SC_PANEL,8);
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_warning(warning,false);
 else{dsr_sc_border_width(warning,1,0);dsr_sc_border_color(warning,dsr_sc_color(gold),0);}
 notice=dsr_sc_text(warning,text,12,10,240,&dsr_roboto_16,gold);lv_obj_update_layout(notice);int height=lv_obj_get_height(notice)+24;lv_obj_set_height(warning,height);
 dsr_sc_text(card,"Sprawdź tryb konfiguracji Shelly. Jeśli AP zniknął po przekazaniu Wi-Fi, wyszukaj urządzenie w sieci lokalnej.",12,84+height+16,268,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Anuluj",cancel,"Ponów",retry);
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
 phase=AP_CONNECT_CONFIRM;lv_obj_t *root=screen("Połączyć z AP?"),*card=content(root);
 line(card,selected.ssid,16,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(card,"Pilot tymczasowo opuści domową sieć. Sterowanie Shelly i Web Panel będą niedostępne.",12,80,268,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(card,"Wybierz własne urządzenie. Sama nazwa AP nie potwierdza jego autentyczności.",12,200,268,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Anuluj",cancel,"Połącz z AP",connect_ap);
}
static void show_results(void)
{
 phase=AP_PICK;lv_obj_t *root=screen("Fabryczne AP Shelly");lv_obj_t *list=dsr_sc_panel(root,12,104,296,258,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);
 unsigned count=0;
 for(unsigned i=0;i<scan.count;i++)if(dsr_ap_candidate(scan.items[i].ssid)){
  char detail[48];snprintf(detail,sizeof(detail),"RSSI %d dBm",scan.items[i].rssi);
  lv_obj_t *row=dsr_sc_button(list,0,(int)count++*112,296,104,DSR_SC_CARD,12,choose,(void *)(uintptr_t)i);
  dsr_sc_icon(row,DSR_ICON_WIFI,12,36,28,DSR_SC_BLUE);
  lv_obj_t *ssid=dsr_sc_text(row,dsr_sc_user(scan.items[i].ssid),52,10,228,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_height(ssid,36);lv_label_set_long_mode(ssid,LV_LABEL_LONG_MODE_WRAP);
  dsr_sc_text(row,dsr_sc_user(detail),52,50,228,&dsr_roboto_12,DSR_SC_MUTED);
  dsr_sc_text(row,scan.items[i].security==DSR_WIFI_OPEN?"Otwarty AP • do sprawdzenia":"AP chroniony • nieobsługiwany",52,74,228,&dsr_roboto_12,DSR_SC_MUTED);
 }
 if(!count){lv_obj_t *empty=dsr_sc_panel(list,0,0,296,258,DSR_SC_CARD,12);large_icon(empty,DSR_ICON_WIFI,20,72,DSR_SC_MUTED);dsr_sc_text(empty,"Nie znaleziono fabrycznego AP Shelly. Włącz tryb konfiguracji urządzenia.",12,112,268,&dsr_roboto_16,DSR_SC_TEXT);}
 char count_text[48];snprintf(count_text,sizeof(count_text),dsr_sc_tr("Znaleziono AP: %u"),count);dsr_sc_text(root,dsr_sc_user(count_text),16,378,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Anuluj",cancel,"Ponów",show_scan);
}
static void show_scan(lv_event_t *event)
{
 (void)event;dsr_result_t result=request(DSR_JOB_AP_SCAN,NULL,NULL,NULL,false);
 if(result!=DSR_OK){error(result);return;}phase=AP_SCAN;message("Fabryczne AP Shelly","Skanowanie Wi-Fi. Pilot pozostaje w domowej sieci.");
}
static void retry(lv_event_t *event)
{
 (void)event;if(phase!=AP_ERROR||waiting||cleanup||lease||dsr_wifi_temporary_active())return;
 /* Retry only discovery. Never replay the credential write or reuse identity. */
 memset(&identity,0,sizeof(identity));memset(&selected,0,sizeof(selected));memset(&scan,0,sizeof(scan));deferred_error=DSR_OK;
 show_scan(NULL);
}
void dsr_ui_ap_open(lv_event_t *event)
{
 (void)event;
 if(dsr_ui_ble_rpc_active())return;
 if(waiting||lease||cleanup||dsr_wifi_temporary_active()){
  cleanup=true;cleanup_retry_at=lv_tick_get();return_after_restore=false;phase=AP_RESTORING;dsr_ap_cancel();message("Powrót do domu","Kończenie poprzedniej operacji i przywracanie domowego Wi-Fi.");return;
 }
 deferred_error=DSR_OK;phase=AP_INTRO;cleanup=return_after_restore=lan_started=waiting_lan_inspect=false;memset(&scan,0,sizeof(scan));memset(&identity,0,sizeof(identity));
 lv_obj_t *root=screen("Dodaj przez Wi-Fi AP"),*card=content(root);
 dsr_sc_icon(card,DSR_ICON_WIFI,12,12,32,DSR_SC_BLUE);dsr_sc_text(card,"Fabryczne urządzenie Shelly",56,16,224,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(card,"Najpierw wyszukamy AP i sprawdzimy lokalne API. Dane domowej sieci wyślesz dopiero po osobnym potwierdzeniu na ekranie.",12,72,268,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(card,"Pilot musi mieć działające domowe Wi-Fi. Obsługiwane są otwarte, fabryczne AP bez hasła API.",12,216,268,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Anuluj",cancel,"Szukaj AP",show_scan);
}
void dsr_ui_ap_scan_open(lv_event_t *event)
{dsr_ui_ap_open(event);if(active&&phase==AP_INTRO&&!cleanup)show_scan(NULL);}
static void apply(lv_event_t *event)
{
 (void)event;if(phase!=AP_DISCLOSE)return;
 dsr_result_t result=request(DSR_JOB_AP_PROVISION,NULL,&identity,NULL,true);
 if(result!=DSR_OK){error(result);return;}phase=AP_APPLY;message("Dołączanie do domu","Wysyłanie konfiguracji jeden raz. Następnie pilot przywróci domowe Wi-Fi i sprawdzi urządzenie w LAN.");
}
static void show_disclose(void)
{
 phase=AP_DISCLOSE;deadline=lv_tick_get()+120000;lv_obj_t *root=screen("Przekazać dane Wi-Fi?");
 lv_obj_t *body=dsr_sc_panel(root,12,104,296,296,DSR_SC_BG,0);lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 lv_obj_t *device=dsr_sc_panel(body,0,0,296,112,DSR_SC_CARD,12);
 line(device,identity.model,12,&dsr_roboto_18,DSR_SC_TEXT);line(device,identity.id,44,&dsr_roboto_12,DSR_SC_MUTED);
 char home_ssid[33]={0};(void)dsr_network_ap_home(home_ssid);line(device,home_ssid,76,&dsr_roboto_16,DSR_SC_TEXT);memset(home_ssid,0,sizeof(home_ssid));
 lv_obj_t *consent=dsr_sc_panel(body,0,120,296,1,DSR_SC_CARD,12);
 lv_obj_t *text=dsr_sc_text(consent,"Przekaż nazwę i hasło zapisanej domowej sieci temu urządzeniu. Otwarty AP i HTTP nie zapewniają kryptograficznego uwierzytelnienia.",12,12,268,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_update_layout(text);int height=lv_obj_get_height(text)+28;lv_obj_set_height(consent,height);
 /* Apply the normal Cyber frame after the final panel height is known. */
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_panel(consent,false);
 dsr_sc_text(body,"Potwierdź tylko dla własnego Shelly. Podgląd WWW jest wstrzymany.",12,120+height+8,268,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_onboarding_footer(root,"Anuluj",cancel,"Przekaż Wi-Fi",apply);
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
