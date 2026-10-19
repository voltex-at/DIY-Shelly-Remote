#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
#include "dsr_native_ota.h"
#include "dsr_restart.h"
#include <stdio.h>
#include <string.h>

static bool active,building,confirming,restart_accepted;
static dsr_ota_channel_t selected_channel=DSR_OTA_STABLE;
static dsr_native_ota_snapshot_t snapshot,shown;
static uint32_t consent_operation;
static lv_obj_t *progress_label,*progress_bar,*local_notice,*back_button,*countdown_label;
/* After a verified install: restart by itself unless the user postpones. */
#define DSR_OTA_AUTO_RESTART_MS 10000u
#define DSR_OTA_AUTO_RETRY_MS 5000u
static bool auto_restart_armed;static uint32_t auto_restart_at,armed_operation;
void dsr_ui_native_ota_hide(void)
{
 if(active&&!building)dsr_native_ota_cancel();
 active=false;auto_restart_armed=false;progress_label=progress_bar=local_notice=back_button=countdown_label=NULL;
}
static lv_obj_t *screen(void)
{
 building=true;lv_obj_t *root=dsr_ui_screen("Firmware Update",false);building=false;active=true;
 lv_obj_t *body=dsr_sc_panel(root,12,86,296,318,DSR_SC_BG,0);
 lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 back_button=dsr_ui_button(root,"Wstecz",12,420,296,dsr_ui_system_open,NULL);return body;
}
static lv_obj_t *raw(lv_obj_t *body,const char *text,int y)
{
 lv_obj_t *label=dsr_sc_text(body,"",4,y,284,&dsr_roboto_14,DSR_SC_TEXT);
 lv_label_set_recolor(label,false);lv_label_set_text(label,text);return label;
}
static void show(void);
static void cancelled_action(lv_event_t *event)
{(void)event;dsr_native_ota_cancel();confirming=false;show();}
static const char *status_text(void)
{
 switch(snapshot.state){
 case DSR_NATIVE_CHECKING:return "Sprawdzanie wydania i podpisu…";
 case DSR_NATIVE_INSTALLING:return "Instalowanie. Nie odłączaj zasilania.";
 case DSR_NATIVE_AVAILABLE:return "Podpis i zgodność manifestu potwierdzone.";
 case DSR_NATIVE_STAGED:return "Instalacja zakończona. Uruchom pilot ponownie.";
 case DSR_NATIVE_CANCELLED:return "Anulowano. Nie uruchomiono nowego firmware.";
 case DSR_NATIVE_FAILED:
  if(snapshot.boot_selection_attempted)return "Nie potwierdzono wyboru startowego firmware. Dalsze zapisy zablokowane do restartu.";
  switch(snapshot.error){
  case DSR_NATIVE_ERROR_OFFLINE:return "Połącz pilot z domowym Wi-Fi.";
  case DSR_NATIVE_ERROR_BUSY:return "Pilot jest zajęty. Spróbuj ponownie.";
  case DSR_NATIVE_ERROR_CLOCK:return "Nie ustawiono czasu. Sprawdź dostęp do internetu i spróbuj ponownie.";
  case DSR_NATIVE_ERROR_NO_RELEASE:return snapshot.channel==DSR_OTA_STABLE?"Brak wydania Stable. Wersje testowe: Zmień kanał, potem Dev.":"Brak wydania w wybranym kanale.";
  case DSR_NATIVE_ERROR_RATE_LIMIT:return "GitHub chwilowo ogranicza zapytania. Spróbuj ponownie za godzinę.";
  case DSR_NATIVE_ERROR_MANIFEST_DOWNLOAD:return "Nie pobrano manifestu wydania. Sprawdź internet i spróbuj ponownie.";
  case DSR_NATIVE_ERROR_MANIFEST:
   if(snapshot.manifest_result==DSR_MANIFEST_UNTRUSTED_KEY)return "Brak zatwierdzonego klucza wydawcy. Instalacja zablokowana.";
   if(snapshot.manifest_result==DSR_MANIFEST_BAD_SIGNATURE)return "Nieprawidłowy podpis wydania. Instalacja zablokowana.";
   if(snapshot.manifest_result==DSR_MANIFEST_POLICY)return "Brak zgodnej nowszej wersji z poprawnym manifestem.";
   return "Nie zweryfikowano manifestu. Spróbuj ponownie później.";
  case DSR_NATIVE_ERROR_SLOT:return "Układ partycji jest niezgodny lub OTA jest zajęte.";
  case DSR_NATIVE_ERROR_BOOT:return "Pilot nie jest gotowy do aktualizacji.";
  case DSR_NATIVE_ERROR_EXPIRED:return "Zgoda wygasła. Sprawdź wydanie ponownie.";
  case DSR_NATIVE_ERROR_CHANGED:return "Wydanie zmieniło się po potwierdzeniu. Sprawdź je ponownie.";
  case DSR_NATIVE_ERROR_MEMORY:return "Brak pamięci. Spróbuj ponownie później.";
  case DSR_NATIVE_ERROR_LOCAL_VERSION:return "Nie rozpoznano lokalnej wersji firmware.";
  case DSR_NATIVE_ERROR_INSTALL:return "Nie ukończono instalacji. Zachowano bieżący firmware.";
  default:return "Nie pobrano poprawnego katalogu wydań. Sprawdź internet i spróbuj ponownie.";
  }
 default:return "Sprawdź dostępne podpisane wydanie.";
 }
}
static void check(lv_event_t *event)
{
 (void)event;confirming=false;
 dsr_result_t result=dsr_network_request_ota(DSR_JOB_OTA_CHECK,selected_channel,false);
 if(result==DSR_OK)show();
 else if(local_notice)dsr_sc_label_set_text(local_notice,"Pilot jest zajęty. Spróbuj ponownie.");
}
static void install(lv_event_t *event)
{
 (void)event;dsr_native_ota_snapshot(&snapshot);
 if(!confirming||snapshot.state!=DSR_NATIVE_AVAILABLE||snapshot.operation_id!=consent_operation||snapshot.channel!=selected_channel){confirming=false;show();return;}
 /* This physical button explicitly attests stable USB power. No inferred ADC
  * charging status and no automatic consent after a CHECK or HTTP request. */
 dsr_result_t result=dsr_network_request_ota(DSR_JOB_OTA_INSTALL,selected_channel,true);
 confirming=false;
 if(result==DSR_OK)show();else if(local_notice)dsr_sc_label_set_text(local_notice,"Pilot jest zajęty. Spróbuj ponownie.");
}
static void prompt(lv_event_t *event)
{
 (void)event;dsr_native_ota_snapshot(&snapshot);if(snapshot.state!=DSR_NATIVE_AVAILABLE)return;
 confirming=true;consent_operation=snapshot.operation_id;shown=snapshot;
 lv_obj_t *body=screen();
 dsr_sc_text(body,"Zainstalować podpisane wydanie?",4,4,284,&dsr_roboto_18,DSR_SC_TEXT);
 raw(body,snapshot.version,60);
 char size[48];snprintf(size,sizeof(size),"%lu B",(unsigned long)snapshot.total);raw(body,size,90);
 dsr_sc_text(body,"Podłącz stabilne zasilanie USB. Nie odłączaj go podczas zapisu. Instalacja nie uruchamia pilota ponownie automatycznie.",4,132,284,&dsr_roboto_14,DSR_SC_TEXT);
 local_notice=dsr_sc_text(body,"Potwierdzenie jest jednorazowe i ważne przez 120 s od sprawdzenia.",4,245,284,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *root=lv_obj_get_parent(body);lv_obj_delete(back_button);back_button=NULL;
 dsr_ui_button(root,"Anuluj",12,420,140,cancelled_action,NULL);
 dsr_ui_primary(dsr_ui_button(root,"USB podłączone • instaluj",168,420,140,install,NULL));
}
static void restart(lv_event_t *event)
{
 (void)event;if(restart_accepted)return;
 dsr_restart_result_t result=dsr_restart_request();
 if(result==DSR_RESTART_ACCEPTED){restart_accepted=true;if(local_notice)dsr_sc_label_set_text(local_notice,"Przyjęto żądanie restartu.");}
 else if(local_notice)dsr_sc_label_set_text(local_notice,"Pilot jest zajęty. Spróbuj ponownie.");
}
static void countdown(void)
{
 if(!countdown_label)return;
 if(!auto_restart_armed||restart_accepted){lv_label_set_text(countdown_label,"");return;}
 uint32_t left=auto_restart_at-lv_tick_get();if(left>DSR_OTA_AUTO_RESTART_MS)left=0;
 dsr_sc_label_set_text_fmt(countdown_label,"Automatyczny restart za %u s",(unsigned)((left+999u)/1000u));
}
static void postpone(lv_event_t *event)
{(void)event;auto_restart_armed=false;show();}
static void channel_apply(lv_event_t *event)
{selected_channel=(dsr_ota_channel_t)(uintptr_t)lv_event_get_user_data(event);dsr_native_ota_cancel();confirming=false;show();}
static void channel_prompt(lv_event_t *event)
{
 (void)event;confirming=true;lv_obj_t *body=screen();
 dsr_sc_text(body,"Wybierz kanał aktualizacji",4,4,284,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(body,"Dev zawiera wersje testowe. Wybór kanału nie rozpoczyna pobierania ani instalacji.",4,64,284,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_ui_button(body,"Stable",0,200,296,channel_apply,(void *)(uintptr_t)DSR_OTA_STABLE);
 dsr_ui_button(body,"Dev • wersje testowe",0,258,296,channel_apply,(void *)(uintptr_t)DSR_OTA_DEV);
}
static void progress(void)
{
 if(!progress_label)return;
 char text[80];snprintf(text,sizeof(text),"%lu / %lu B",(unsigned long)snapshot.received,(unsigned long)snapshot.total);
 lv_label_set_text(progress_label,text);
 if(progress_bar){uint32_t value=snapshot.total?(uint32_t)(((uint64_t)snapshot.received*1000u)/snapshot.total):0;if(value>1000)value=1000;lv_bar_set_value(progress_bar,(int32_t)value,LV_ANIM_OFF);}
}
static void show(void)
{
 dsr_native_ota_snapshot(&snapshot);shown=snapshot;confirming=false;lv_obj_t *body=screen();
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 if(hud){
  lv_obj_t *version=dsr_sc_panel(body,0,0,296,56,DSR_SC_CARD,10);dsr_sc_icon(version,DSR_ICON_CPU,12,12,32,DSR_SC_BLUE);
  lv_obj_t *channel=dsr_sc_panel(body,0,64,296,38,DSR_SC_CARD,10);dsr_sc_icon(channel,DSR_ICON_GIT_BRANCH,12,7,24,DSR_SC_BLUE);
  lv_obj_t *message=dsr_sc_panel(body,0,108,296,snapshot.state==DSR_NATIVE_AVAILABLE?64:82,DSR_SC_PANEL,10);
  if(snapshot.state==DSR_NATIVE_FAILED){dsr_ui_cyber_warning(message,false);dsr_sc_icon(message,DSR_ICON_WARNING,8,7,24,0xffc107);}
 }
 dsr_sc_text(body,"Current version",hud?54:4,hud?6:4,hud?230:284,hud?&dsr_roboto_11:&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *installed=raw(body,snapshot.installed,hud?26:30);
 if(hud){lv_obj_set_x(installed,54);lv_obj_set_width(installed,230);lv_obj_set_style_text_font(installed,&dsr_roboto_22,0);}
 dsr_sc_text(body,selected_channel==DSR_OTA_DEV?"Kanał: Dev • wersje testowe":"Kanał: Stable",hud?48:4,hud?74:64,hud?236:284,&dsr_roboto_14,DSR_SC_TEXT);
 local_notice=dsr_sc_text(body,status_text(),hud?(snapshot.state==DSR_NATIVE_FAILED?42:12):4,hud?114:104,hud?(snapshot.state==DSR_NATIVE_FAILED?244:272):284,hud?&dsr_roboto_12:&dsr_roboto_14,DSR_SC_TEXT);
 bool running=snapshot.state==DSR_NATIVE_CHECKING||snapshot.state==DSR_NATIVE_INSTALLING;
 if(running){
  lv_obj_set_height(body,260);lv_obj_set_height(local_notice,84);lv_label_set_long_mode(local_notice,LV_LABEL_LONG_MODE_DOTS);
  progress_bar=lv_bar_create(body);lv_obj_set_pos(progress_bar,4,202);lv_obj_set_size(progress_bar,284,12);lv_bar_set_range(progress_bar,0,1000);lv_obj_set_style_bg_color(progress_bar,dsr_sc_color(DSR_SC_EDGE),0);lv_obj_set_style_bg_color(progress_bar,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);lv_obj_set_style_radius(progress_bar,6,0);lv_obj_set_style_radius(progress_bar,6,LV_PART_INDICATOR);
  progress_label=dsr_sc_text(body,"",4,224,284,&dsr_roboto_14,DSR_SC_MUTED);progress();
  dsr_ui_button(lv_obj_get_parent(body),"Anuluj",12,360,296,cancelled_action,NULL);return;
 }
 if(snapshot.state==DSR_NATIVE_FAILED){
  /* error/fetch/HTTP/manifest: what the owner reports from a failed check. */
  lv_obj_t *code=dsr_sc_text(body,"",4,snapshot.boot_selection_attempted?196:164,284,&dsr_roboto_12,DSR_SC_MUTED);
  dsr_sc_label_set_text_fmt(code,"Kod błędu: %d/%d/%d/%d",(int)snapshot.error,(int)snapshot.fetch_result,snapshot.http_status,(int)snapshot.manifest_result);
 }
 if(snapshot.state==DSR_NATIVE_STAGED&&!snapshot.boot_selection_attempted){
  if(!restart_accepted&&armed_operation!=snapshot.operation_id){armed_operation=snapshot.operation_id;auto_restart_armed=true;auto_restart_at=lv_tick_get()+DSR_OTA_AUTO_RESTART_MS;}
  countdown_label=dsr_sc_text(body,"",4,166,284,&dsr_roboto_14,DSR_SC_TEXT);countdown();
  dsr_ui_primary(dsr_ui_button(body,"Uruchom ponownie",0,206,296,restart,NULL));
  if(auto_restart_armed)dsr_ui_button(body,"Później",0,262,296,postpone,NULL);
  return;
 }
 if(snapshot.boot_selection_attempted){dsr_ui_button(body,"Uruchom ponownie",0,270,296,restart,NULL);return;}
 /* Both main actions fit inside the initial 318 px viewport above Back. */
 int y=198;
 if(snapshot.state==DSR_NATIVE_AVAILABLE){
  raw(body,snapshot.version,186);char size[48];snprintf(size,sizeof(size),"%lu B",(unsigned long)snapshot.total);raw(body,size,214);
  dsr_ui_primary(dsr_ui_button(body,"Zainstaluj",0,250,296,prompt,NULL));
  dsr_sc_text(body,"Opis wydania z GitHuba • niezweryfikowany",4,314,284,&dsr_roboto_12,DSR_SC_MUTED);
  lv_obj_t *notes=raw(body,snapshot.notes[0]?snapshot.notes:"—",354);lv_obj_update_layout(notes);
  if(lv_obj_get_height(notes)>240){lv_obj_set_height(notes,240);lv_label_set_long_mode(notes,LV_LABEL_LONG_DOT);}
  y=354+lv_obj_get_height(notes)+24;
 }
 dsr_ui_button(body,"Sprawdź aktualizacje",0,y,296,check,NULL);
 dsr_ui_button(body,"Zmień kanał",0,y+58,296,channel_prompt,NULL);
 dsr_ui_button(body,"Zezwól na OTA • 2 minuty",0,y+130,296,dsr_ui_web_ota_open,NULL);
}
void dsr_ui_native_ota_open(lv_event_t *event)
{(void)event;show();}
void dsr_ui_native_ota_tick(void)
{
 if(!active)return;
 dsr_native_ota_snapshot(&snapshot);
 if(snapshot.state!=shown.state||snapshot.error!=shown.error||snapshot.operation_id!=shown.operation_id){show();return;}
 if(auto_restart_armed&&!restart_accepted&&snapshot.state==DSR_NATIVE_STAGED&&!snapshot.boot_selection_attempted){
  if((int32_t)(lv_tick_get()-auto_restart_at)>=0){
   restart(NULL);
   if(!restart_accepted)auto_restart_at=lv_tick_get()+DSR_OTA_AUTO_RETRY_MS; /* busy: retry, still cancellable */
   else{auto_restart_armed=false;if(countdown_label)lv_label_set_text(countdown_label,"");}
  }else countdown();
 }
 if(!confirming)progress();
}
