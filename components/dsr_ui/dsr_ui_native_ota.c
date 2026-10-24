#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
#include "dsr_native_ota.h"
#include "dsr_restart.h"
#include <stdio.h>
#include <string.h>

static bool active,building,confirming,restart_accepted,install_prompt,prompt_known,prompt_low,checking_battery;
static dsr_ota_channel_t selected_channel=DSR_OTA_STABLE;
static dsr_native_ota_snapshot_t snapshot,shown;
static uint32_t consent_operation;
static lv_obj_t *progress_label,*progress_bytes,*progress_bar,*local_notice,*back_button,*countdown_label;
/* After a verified install: restart by itself unless the user postpones. */
#define DSR_OTA_AUTO_RESTART_MS 10000u
#define DSR_OTA_AUTO_RETRY_MS 5000u
static bool auto_restart_armed;static uint32_t auto_restart_at,armed_operation;
void dsr_ui_native_ota_hide(void)
{
 if(active&&!building)dsr_native_ota_cancel();
 active=false;auto_restart_armed=false;install_prompt=checking_battery=false;progress_label=progress_bytes=progress_bar=local_notice=back_button=countdown_label=NULL;
}
static lv_obj_t *screen(void)
{
 building=true;lv_obj_t *root=dsr_ui_screen("Firmware Update",false);building=false;active=true;
 lv_obj_t *body=dsr_sc_panel(root,12,86,296,318,DSR_SC_BG,0);
 lv_obj_set_style_bg_opa(body,LV_OPA_TRANSP,0);
 lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 bool running=snapshot.state==DSR_NATIVE_CHECKING||snapshot.state==DSR_NATIVE_INSTALLING;
 back_button=dsr_ui_button(root,"Wstecz",12,420,running?108:296,dsr_ui_system_open,NULL);return body;
}
static lv_obj_t *raw(lv_obj_t *body,const char *text,int y)
{
 lv_obj_t *label=dsr_sc_text(body,"",4,y,284,&dsr_roboto_14,DSR_SC_TEXT);
 lv_label_set_recolor(label,false);lv_label_set_text(label,text);return label;
}
static void show(void);
static void prompt(lv_event_t *event);
static const char *low_battery(void)
{return "Bateria poniżej 20%. Doładuj pilot do co najmniej 20% i sprawdź baterię ponownie.";}
static bool battery_low(bool *known)
{uint8_t percent=0;*known=dsr_ui_battery_level(&percent);return *known&&percent<DSR_NATIVE_OTA_MIN_BATTERY_PERCENT;}
static uint32_t gold(void){return dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;}
static lv_obj_t *card(lv_obj_t *body,int y,int height,bool warning)
{
 lv_obj_t *o=dsr_sc_panel(body,0,y,296,height,DSR_SC_CARD,10);
 dsr_sc_border_width(o,1,0);dsr_sc_border_color(o,dsr_sc_color(warning?gold():DSR_SC_EDGE),0);
 if(warning&&dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_warning(o,false);
 return o;
}
/* Text determines its frame height. Never scroll the error inside another
 * scroller: the page owns scrolling, including diagnostic details. */
static lv_obj_t *message(lv_obj_t *body,const char *text,int y,bool warning,const lv_font_t *font)
{
 lv_obj_t *o=card(body,y,56,warning);
 dsr_sc_icon(o,warning?DSR_ICON_WARNING:DSR_ICON_INFO,12,12,24,warning?gold():DSR_SC_GLOW);
 lv_obj_t *label=dsr_sc_text(o,text,44,12,240,font,warning?gold():DSR_SC_TEXT);
 lv_obj_update_layout(label);int height=lv_obj_get_height(label)+24;lv_obj_set_height(o,height<56?56:height);
 return label;
}
static int after(lv_obj_t *o,int gap)
{lv_obj_update_layout(o);return lv_obj_get_y(o)+lv_obj_get_height(o)+gap;}
static void local_warning(const char *text)
{
 if(!local_notice)return;
 dsr_sc_label_set_text(local_notice,text);lv_obj_set_style_text_color(local_notice,lv_color_hex(gold()),0);
 lv_obj_t *o=lv_obj_get_parent(local_notice);dsr_sc_border_color(o,lv_color_hex(gold()),0);
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_warning(o,false);
}
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
  case DSR_NATIVE_ERROR_BATTERY_LOW:return low_battery();
  case DSR_NATIVE_ERROR_INSTALL:
   if(snapshot.install_result==DSR_INSTALL_SDK)return "Nie udało się zapisać firmware. Bieżąca wersja pozostaje aktywna.";
   if(snapshot.fetch_result==DSR_FETCH_IO||snapshot.fetch_result==DSR_FETCH_TIMEOUT||snapshot.fetch_result==DSR_FETCH_INCOMPLETE)return "Przerwano pobieranie. Sprawdź Wi-Fi i spróbuj ponownie.";
   return "Nie ukończono instalacji. Zachowano bieżący firmware.";
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
 else local_warning("Pilot jest zajęty. Spróbuj ponownie.");
}
static void install(lv_event_t *event)
{
 (void)event;dsr_native_ota_snapshot(&snapshot);
 if(!confirming||snapshot.state!=DSR_NATIVE_AVAILABLE||snapshot.operation_id!=consent_operation||snapshot.channel!=selected_channel){confirming=false;show();return;}
 bool known;if(battery_low(&known)){prompt(NULL);return;}
 /* The adjacent warning makes Install an explicit stable-USB attestation,
  * including unknown SOC. The worker repeats the low-battery check freshly. */
 dsr_result_t result=dsr_network_request_ota(DSR_JOB_OTA_INSTALL,selected_channel,true);
 confirming=false;
 if(result==DSR_OK)show();else local_warning("Pilot jest zajęty. Spróbuj ponownie.");
}
static void recheck_battery(lv_event_t *event)
{
 (void)event;if(!install_prompt)return;
 if(dsr_network_request_battery()==DSR_OK){checking_battery=true;if(local_notice)dsr_sc_label_set_text(local_notice,"Sprawdzanie baterii…");}
 else local_warning("Pilot jest zajęty. Spróbuj ponownie.");
}
static void prompt(lv_event_t *event)
{
 (void)event;dsr_native_ota_snapshot(&snapshot);if(snapshot.state!=DSR_NATIVE_AVAILABLE)return;
 confirming=true;consent_operation=snapshot.operation_id;shown=snapshot;
 lv_obj_t *body=screen();
 install_prompt=true;prompt_low=battery_low(&prompt_known);
 lv_obj_t *heading=message(body,"Zainstalować podpisane wydanie?",0,false,&dsr_roboto_18);
 int y=after(lv_obj_get_parent(heading),12);lv_obj_t *release=card(body,y,60,false);
 lv_obj_t *version=raw(release,snapshot.version,8);lv_obj_set_x(version,12);lv_obj_set_width(version,156);lv_obj_set_style_text_font(version,dsr_sc_font(&dsr_roboto_18),0);
 char size[48];snprintf(size,sizeof(size),"%lu B",(unsigned long)snapshot.total);lv_obj_t *bytes=raw(release,size,10);lv_obj_set_x(bytes,172);lv_obj_set_width(bytes,112);lv_obj_set_style_text_font(bytes,&dsr_roboto_12,0);lv_obj_set_style_text_align(bytes,LV_TEXT_ALIGN_RIGHT,0);
 dsr_sc_text(release,selected_channel==DSR_OTA_DEV?"Kanał: Dev • wersje testowe":"Kanał: Stable",12,32,272,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *power=message(body,prompt_low?low_battery():"Instaluj potwierdza stabilne zasilanie USB. Nie odłączaj go podczas zapisu. Restart za 10 s; Później odracza.",after(release,12),true,&dsr_roboto_14);
 int notice_y=after(lv_obj_get_parent(power),12);
 if(prompt_low){dsr_ui_button(body,"Sprawdź baterię",0,notice_y,296,recheck_battery,NULL);notice_y+=60;}
 /* While blocked, use the warning for measurement feedback; consent becomes
  * relevant once installation is available, keeping the blocked view compact. */
 local_notice=prompt_low?power:message(body,prompt_known?"Potwierdzenie jest jednorazowe i ważne przez 120 s od sprawdzenia.":"Procent baterii nieznany. Zgoda jednorazowa: 120 s od sprawdzenia.",notice_y,false,&dsr_roboto_12);
 lv_obj_t *root=lv_obj_get_parent(body);lv_obj_delete(back_button);back_button=NULL;
 dsr_ui_button(root,"Anuluj",12,420,140,cancelled_action,NULL);
 lv_obj_t *accept=dsr_ui_button(root,"Instaluj",168,420,140,install,NULL);dsr_ui_primary(accept);
 if(prompt_low)lv_obj_add_state(accept,LV_STATE_DISABLED);
}
static void restart(lv_event_t *event)
{
 (void)event;if(restart_accepted)return;
 dsr_restart_result_t result=dsr_restart_request();
 if(result==DSR_RESTART_ACCEPTED){restart_accepted=true;auto_restart_armed=false;if(countdown_label)dsr_sc_label_set_text_fmt(countdown_label,"Automatyczny restart za %u s",0u);if(local_notice)dsr_sc_label_set_text(local_notice,"Przyjęto żądanie restartu.");}
 else local_warning("Pilot jest zajęty. Spróbuj ponownie.");
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
 lv_obj_t *heading=message(body,"Wybierz kanał aktualizacji",0,false,&dsr_roboto_18);
 lv_obj_t *info=message(body,"Dev zawiera wersje testowe. Wybór kanału nie rozpoczyna pobierania ani instalacji.",after(lv_obj_get_parent(heading),12),true,&dsr_roboto_14);
 int y=after(lv_obj_get_parent(info),12);
 lv_obj_t *stable=dsr_ui_button(body,"Stable",0,y,296,channel_apply,(void *)(uintptr_t)DSR_OTA_STABLE);
 lv_obj_t *dev=dsr_ui_button(body,"Dev • wersje testowe",0,y+60,296,channel_apply,(void *)(uintptr_t)DSR_OTA_DEV);
 dsr_ui_primary(selected_channel==DSR_OTA_DEV?dev:stable);
}
static void progress(void)
{
 if(!progress_label)return;
 char text[80];snprintf(text,sizeof(text),"%lu / %lu B",(unsigned long)snapshot.received,(unsigned long)snapshot.total);
 if(progress_bytes){
  lv_label_set_text(progress_bytes,text);
  if(snapshot.total){uint64_t percent=(uint64_t)snapshot.received*100u/snapshot.total;if(percent>100)percent=100;snprintf(text,sizeof(text),"%u%%",(unsigned)percent);}else snprintf(text,sizeof(text),"— %%");
 }
 lv_label_set_text(progress_label,text);
 if(progress_bar){uint32_t value=snapshot.total?(uint32_t)(((uint64_t)snapshot.received*1000u)/snapshot.total):0;if(value>1000)value=1000;lv_bar_set_value(progress_bar,(int32_t)value,LV_ANIM_OFF);}
}
static void show(void)
{
 dsr_native_ota_snapshot(&snapshot);shown=snapshot;confirming=false;lv_obj_t *body=screen();
 lv_obj_t *version=card(body,0,56,false);dsr_sc_icon(version,DSR_ICON_FIRMWARE,12,12,32,DSR_SC_GLOW);
 dsr_sc_text(version,"Current version",54,6,230,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *installed=raw(version,snapshot.installed,26);lv_obj_set_x(installed,54);lv_obj_set_width(installed,230);lv_obj_set_style_text_font(installed,dsr_sc_font(&dsr_roboto_18),0);lv_label_set_long_mode(installed,LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
 lv_obj_t *channel=card(body,64,38,false);dsr_sc_icon(channel,DSR_ICON_GIT_BRANCH,12,7,24,DSR_SC_GLOW);
 dsr_sc_text(channel,selected_channel==DSR_OTA_DEV?"Kanał: Dev • wersje testowe":"Kanał: Stable",48,11,236,&dsr_roboto_14,DSR_SC_TEXT);
 bool running=snapshot.state==DSR_NATIVE_CHECKING||snapshot.state==DSR_NATIVE_INSTALLING;
 local_notice=message(body,status_text(),110,snapshot.state==DSR_NATIVE_FAILED||snapshot.state==DSR_NATIVE_INSTALLING,&dsr_roboto_14);
 lv_obj_t *notice_box=lv_obj_get_parent(local_notice);int y=after(notice_box,12);
 if(running){
  if(snapshot.state==DSR_NATIVE_CHECKING){
   lv_obj_t *well=card(body,y,76,false),*spinner=lv_spinner_create(well);lv_obj_set_pos(spinner,124,14);lv_obj_set_size(spinner,48,48);lv_spinner_set_anim_params(spinner,1000,200);
  }else{
   lv_obj_t *well=card(body,y,96,false);
   progress_bar=lv_bar_create(well);lv_obj_set_pos(progress_bar,12,12);lv_obj_set_size(progress_bar,272,12);lv_bar_set_range(progress_bar,0,1000);
   lv_obj_set_style_bg_color(progress_bar,dsr_sc_color(DSR_SC_EDGE),0);lv_obj_set_style_bg_color(progress_bar,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);lv_obj_set_style_radius(progress_bar,6,0);lv_obj_set_style_radius(progress_bar,6,LV_PART_INDICATOR);
   progress_label=dsr_sc_text(well,"",12,32,272,&dsr_roboto_28,DSR_SC_GLOW);lv_obj_set_style_text_align(progress_label,LV_TEXT_ALIGN_CENTER,0);
   progress_bytes=dsr_sc_text(well,"",12,70,272,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_set_style_text_align(progress_bytes,LV_TEXT_ALIGN_CENTER,0);progress();
  }
  dsr_ui_button(lv_obj_get_parent(body),"Anuluj",128,420,180,cancelled_action,NULL);return;
 }
 if(snapshot.state==DSR_NATIVE_FAILED){
  /* Diagnostic details belong below the full message inside its frame. */
  int code_y=after(local_notice,8);lv_obj_t *code=dsr_sc_text(notice_box,"",12,code_y,272,&dsr_roboto_12,DSR_SC_MUTED);
  dsr_sc_label_set_text_fmt(code,"Kod błędu: %d/%d/%d/%d",(int)snapshot.error,(int)snapshot.fetch_result,snapshot.http_status,(int)snapshot.manifest_result);
  if(snapshot.error==DSR_NATIVE_ERROR_INSTALL){
   char detail[72];snprintf(detail,sizeof(detail),"\nOTA %d • SDK 0x%x • I/O %d",(int)snapshot.install_result,(unsigned)snapshot.sdk_error,snapshot.io_result);lv_label_ins_text(code,LV_LABEL_POS_LAST,detail);
  }
  lv_obj_update_layout(code);lv_obj_set_height(notice_box,after(code,12));y=after(notice_box,12);
 }
 if(snapshot.state==DSR_NATIVE_STAGED){
  if(!restart_accepted&&armed_operation!=snapshot.operation_id){armed_operation=snapshot.operation_id;auto_restart_armed=true;auto_restart_at=lv_tick_get()+DSR_OTA_AUTO_RESTART_MS;}
  if(auto_restart_armed){
   countdown_label=dsr_sc_text(notice_box,"",12,after(local_notice,8),272,&dsr_roboto_14,DSR_SC_GLOW);countdown();lv_obj_update_layout(countdown_label);lv_obj_set_height(notice_box,after(countdown_label,12));
  }
  y=after(notice_box,12);dsr_ui_primary(dsr_ui_button(body,"Uruchom ponownie",0,y,296,restart,NULL));
  if(auto_restart_armed)dsr_ui_button(body,"Później",0,y+60,296,postpone,NULL);
  return;
 }
 if(snapshot.boot_selection_attempted){dsr_ui_button(body,"Uruchom ponownie",0,y,296,restart,NULL);return;}
 if(snapshot.state==DSR_NATIVE_AVAILABLE){
  lv_obj_t *release=card(body,y,60,false);lv_obj_t *candidate=raw(release,snapshot.version,8);lv_obj_set_x(candidate,12);lv_obj_set_width(candidate,156);lv_obj_set_style_text_font(candidate,dsr_sc_font(&dsr_roboto_18),0);
  char size[48];snprintf(size,sizeof(size),"%lu B",(unsigned long)snapshot.total);lv_obj_t *bytes=raw(release,size,30);lv_obj_set_x(bytes,12);lv_obj_set_width(bytes,272);lv_obj_set_style_text_font(bytes,&dsr_roboto_12,0);
  int action_y=after(release,12);dsr_ui_primary(dsr_ui_button(body,"Instaluj",0,action_y,296,prompt,NULL));
  lv_obj_t *notes_card=card(body,action_y+60,64,false);
  lv_obj_t *heading=dsr_sc_text(notes_card,"Opis wydania z GitHuba • niezweryfikowany",12,12,272,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_update_layout(heading);
  lv_obj_t *notes=raw(notes_card,snapshot.notes[0]?snapshot.notes:"—",after(heading,8));lv_obj_set_x(notes,12);lv_obj_set_width(notes,272);lv_obj_update_layout(notes);
  if(lv_obj_get_height(notes)>240){lv_obj_set_height(notes,240);lv_label_set_long_mode(notes,LV_LABEL_LONG_DOT);}
  lv_obj_set_height(notes_card,after(notes,12));y=after(notes_card,12);
 }
 dsr_ui_button(body,"Sprawdź aktualizacje",0,y,296,check,NULL);
 dsr_ui_button(body,"Zmień kanał",0,y+60,296,channel_prompt,NULL);
 dsr_ui_button(body,"Zezwól na OTA • 2 minuty",0,y+132,296,dsr_ui_web_ota_open,NULL);
}

void dsr_ui_native_ota_open(lv_event_t *event)
{(void)event;show();}
void dsr_ui_native_ota_tick(void)
{
 if(!active)return;
 dsr_native_ota_snapshot(&snapshot);
 if(snapshot.state!=shown.state||snapshot.error!=shown.error||snapshot.operation_id!=shown.operation_id){show();return;}
 if(install_prompt){bool known,low=battery_low(&known);if(known!=prompt_known||low!=prompt_low||(checking_battery&&!dsr_network_busy())){prompt(NULL);return;}}
 if(auto_restart_armed&&!restart_accepted&&snapshot.state==DSR_NATIVE_STAGED){
  countdown(); /* Paint zero too before dispatching the restart. */
  if((int32_t)(lv_tick_get()-auto_restart_at)>=0){
   restart(NULL);
   if(!restart_accepted)auto_restart_at=lv_tick_get()+DSR_OTA_AUTO_RETRY_MS; /* busy: retry, still cancellable */
   else auto_restart_armed=false;
  }else countdown();
 }
 if(!confirming)progress();
}
