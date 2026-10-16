#include "dsr_ui_internal.h"
#include "dsr_native_ota.h"
#include "dsr_restart.h"
#include <stdio.h>
#include <string.h>

static bool active,building,confirming,restart_accepted;
static dsr_ota_channel_t selected_channel=DSR_OTA_STABLE;
static dsr_native_ota_snapshot_t snapshot,shown;
static uint32_t consent_operation;
static lv_obj_t *progress_label,*local_notice,*back_button;
void dsr_ui_native_ota_hide(void)
{
 if(active&&!building)dsr_native_ota_cancel();
 active=false;progress_label=local_notice=back_button=NULL;
}
static lv_obj_t *screen(void)
{
 building=true;lv_obj_t *root=dsr_ui_screen("Firmware Update",false);building=false;active=true;
 lv_obj_t *body=dsr_sc_panel(root,12,86,296,318,DSR_SC_BG,0);
 lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);
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
  case DSR_NATIVE_ERROR_NO_RELEASE:return "Brak wydania w wybranym kanale.";
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
}
static void show(void)
{
 dsr_native_ota_snapshot(&snapshot);shown=snapshot;confirming=false;lv_obj_t *body=screen();
 dsr_sc_text(body,"Current version",4,4,284,&dsr_roboto_12,DSR_SC_MUTED);raw(body,snapshot.installed,30);
 dsr_sc_text(body,selected_channel==DSR_OTA_DEV?"Kanał: Dev • wersje testowe":"Kanał: Stable",4,64,284,&dsr_roboto_14,DSR_SC_TEXT);
 local_notice=dsr_sc_text(body,status_text(),4,104,284,&dsr_roboto_14,DSR_SC_TEXT);
 bool running=snapshot.state==DSR_NATIVE_CHECKING||snapshot.state==DSR_NATIVE_INSTALLING;
 if(running){
  progress_label=dsr_sc_text(body,"",4,226,284,&dsr_roboto_14,DSR_SC_MUTED);progress();
  dsr_ui_button(body,"Anuluj",0,280,296,cancelled_action,NULL);return;
 }
 if(snapshot.state==DSR_NATIVE_STAGED||snapshot.boot_selection_attempted){dsr_ui_button(body,"Uruchom ponownie",0,270,296,restart,NULL);return;}
 int y=234;
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
 dsr_ui_button(body,"Zezwól na OTA • 2 minuty",0,y+116,296,dsr_ui_web_ota_open,NULL);
}
void dsr_ui_native_ota_open(lv_event_t *event)
{(void)event;show();}
void dsr_ui_native_ota_tick(void)
{
 if(!active)return;
 dsr_native_ota_snapshot(&snapshot);
 if(snapshot.state!=shown.state||snapshot.error!=shown.error||snapshot.operation_id!=shown.operation_id){show();return;}
 if(!confirming)progress();
}
