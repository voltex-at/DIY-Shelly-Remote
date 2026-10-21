/* A bounded local draft. No charger control, GPIO inference or Flash on LVGL. */
#include "dsr_ui_internal.h"
#include <stdio.h>
#include <string.h>

enum {NOMINAL,CAPACITY,EMPTY,MID,FULL,FIELD_COUNT};

#ifdef ESP_PLATFORM
#include "esp_attr.h"
#else
#define EXT_RAM_BSS_ATTR
#endif
/* UI-owned cached state, never accessed by an ISR or while cache is disabled.
 * Keep it beside the LVGL pool in PSRAM, preserving the internal-RAM budget. */

static EXT_RAM_BSS_ATTR lv_obj_t *body,*keyboard,*type,*fields[FIELD_COUNT],*reference_field;
static EXT_RAM_BSS_ATTR lv_obj_t *status,*save,*read_button,*apply_button,*unity_button,*reload,*gain_label,*reading,*dialog,*error_panel,*new_profile,*trend_switch;
static EXT_RAM_BSS_ATTR dsr_battery_profile_t baseline,draft,submitted;
static EXT_RAM_BSS_ATTR dsr_battery_sample_t reference;
static EXT_RAM_BSS_ATTR uint32_t reference_at;
static EXT_RAM_BSS_ATTR bool active,pending,loading,known,reference_valid,waiting_read,uncertain_draft;
static void back(lv_event_t *event);
bool dsr_ui_battery_calibration_active(void){return active;}
static void text(lv_obj_t *o,const char *s)
{if(o&&strcmp(lv_label_get_text(o),dsr_sc_tr(s)))dsr_sc_label_set_text(o,s);}
void dsr_ui_battery_calibration_hide(void)
{
 active=false;body=keyboard=type=reference_field=status=save=read_button=apply_button=unity_button=reload=gain_label=reading=dialog=error_panel=new_profile=trend_switch=NULL;
 memset(fields,0,sizeof(fields));reference_valid=waiting_read=false;
 /* An accepted save/load outlives this screen. Reopening cannot resubmit it. */
}
static void close_keyboard(lv_event_t *event)
{
 (void)event;if(!keyboard)return;
 lv_obj_t *field=lv_keyboard_get_textarea(keyboard);if(field)lv_obj_remove_state(field,LV_STATE_FOCUSED);
 lv_keyboard_set_textarea(keyboard,NULL);lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);lv_obj_set_height(body,260);
}
static void focus(lv_event_t *event)
{
 if(pending||!known||!keyboard)return;
 dsr_ui_keyboard_focus(keyboard,lv_event_get_target(event));
 /* Both classic and Cyber leave the 420px action bar accessible. */
 lv_obj_set_size(keyboard,320,224);lv_obj_set_align(keyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(keyboard,0,192);lv_obj_set_height(body,100);
 lv_obj_update_layout(body);lv_obj_scroll_to_view_recursive(lv_event_get_target(event),LV_ANIM_OFF);
}
static void controls(void)
{
 if(!active)return;
 lv_obj_t *items[]={type,fields[0],fields[1],fields[2],fields[3],fields[4],reference_field,save,read_button,apply_button,unity_button,trend_switch};
 for(unsigned i=0;i<sizeof(items)/sizeof(items[0]);i++){
  if(pending||!known)lv_obj_add_state(items[i],LV_STATE_DISABLED);else lv_obj_remove_state(items[i],LV_STATE_DISABLED);
 }
 if(pending){lv_obj_add_state(reload,LV_STATE_DISABLED);lv_obj_add_state(new_profile,LV_STATE_DISABLED);}else{lv_obj_remove_state(reload,LV_STATE_DISABLED);lv_obj_remove_state(new_profile,LV_STATE_DISABLED);}
 if(known){lv_obj_remove_flag(body,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(error_panel,LV_OBJ_FLAG_HIDDEN);}
 else{lv_obj_add_flag(body,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(error_panel,LV_OBJ_FLAG_HIDDEN);}
 if(waiting_read)lv_obj_add_state(read_button,LV_STATE_DISABLED);
}
static bool capture(dsr_battery_profile_t *p)
{
 *p=draft;p->kind=(uint8_t)lv_dropdown_get_selected(type);p->trend_enabled=lv_obj_has_state(trend_switch,LV_STATE_CHECKED);
 uint16_t values[FIELD_COUNT];
 for(unsigned i=0;i<FIELD_COUNT;i++)if(!dsr_battery_parse_number(lv_textarea_get_text(fields[i]),&values[i]))return false;
 p->nominal_mv=values[NOMINAL];p->capacity_mah=values[CAPACITY];p->empty_mv=values[EMPTY];p->mid_mv=values[MID];p->full_mv=values[FULL];
 return dsr_battery_profile_valid(p);
}
static void populate(void)
{
 lv_dropdown_set_selected(type,draft.kind);
 if(draft.trend_enabled)lv_obj_add_state(trend_switch,LV_STATE_CHECKED);else lv_obj_remove_state(trend_switch,LV_STATE_CHECKED);
 const uint16_t values[]={draft.nominal_mv,draft.capacity_mah,draft.empty_mv,draft.mid_mv,draft.full_mv};
 char n[16];for(unsigned i=0;i<FIELD_COUNT;i++){snprintf(n,sizeof(n),"%u",values[i]);lv_textarea_set_text(fields[i],n);}
 snprintf(n,sizeof(n),"%u / 1000",draft.gain_permille);text(gain_label,n);
}
static void changed(lv_event_t *event)
{(void)event;if(!pending)text(status,"Zmiany obowiązują po zapisaniu.");}
static void leave(lv_event_t *event)
{(void)event;dsr_ui_battery_calibration_hide();dsr_ui_battery_return();}
static void stay(lv_event_t *event)
{(void)event;if(dialog){lv_obj_delete(dialog);dialog=NULL;}}
static void back(lv_event_t *event)
{
 (void)event;if(pending){text(status,"Poczekaj na zakończenie zapisu.");return;}
 dsr_battery_profile_t candidate;
 if((!known&&!uncertain_draft)||(capture(&candidate)&&dsr_battery_profile_equal(&candidate,&baseline))){leave(NULL);return;}
 if(dialog)return;
 close_keyboard(NULL);
 dialog=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);lv_obj_set_style_bg_opa(dialog,180,0);lv_obj_add_flag(dialog,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *panel=dsr_sc_panel(dialog,12,117,296,246,DSR_SC_PANEL,14);
 dsr_sc_text(panel,"Odrzucić zmiany?",14,16,268,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(panel,"Niezapisane zmiany tego formularza zostaną utracone.",14,53,268,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(panel,"Zostań",12,188,130,stay,NULL);dsr_ui_button(panel,"Odrzuć",154,188,130,leave,NULL);
}
bool dsr_ui_battery_calibration_back(void)
{if(!active)return false;back(NULL);return true;}
static void commit(lv_event_t *event)
{
 (void)event;if(pending||!known)return;
 close_keyboard(NULL);
 dsr_battery_profile_t candidate;
 if(!capture(&candidate)){text(status,"Sprawdź liczby i progi: 0% < 50% < 100%. Odstęp min. 100 mV.");return;}
 dsr_result_t r=dsr_network_request_battery_profile(DSR_JOB_BATTERY_PROFILE_SAVE,&candidate);
 if(r!=DSR_OK){text(status,"Pilot jest zajęty. Spróbuj ponownie.");return;}
 submitted=candidate;pending=true;loading=false;controls();text(status,"Zapisywanie ustawień…");
}
static void load(lv_event_t *event)
{
 (void)event;if(pending)return;
 /* Retry exists only when the boot/readback failed: never discards a draft. */
 if(known)return;
 dsr_result_t r=dsr_network_request_battery_profile(DSR_JOB_BATTERY_PROFILE_LOAD,NULL);
 if(r!=DSR_OK){text(status,"Pilot jest zajęty. Spróbuj ponownie.");return;}
 pending=loading=true;controls();text(status,"Odczyt profilu baterii…");
}
static void measure(lv_event_t *event)
{
 (void)event;if(pending||waiting_read||!known)return;
 close_keyboard(NULL);
 dsr_result_t r=dsr_network_request_battery();
 if(r!=DSR_OK){text(status,"Pilot jest zajęty. Spróbuj ponownie.");return;}
 waiting_read=true;reference_valid=false;controls();text(status,"Odczyt napięcia…");
}
void dsr_ui_battery_calibration_sample(const dsr_event_t *event)
{
 if(!active||!waiting_read)return;
 waiting_read=false;reference_valid=event->result==DSR_OK&&event->battery.has_voltage;
 reference=event->battery;reference_at=lv_tick_get();controls();
 text(status,reference_valid?"Wpisz równoczesny odczyt multimetru w mV.":"Pomiar niedostępny. Spróbuj ponownie.");
}
void dsr_ui_battery_calibration_tick(void)
{
 if(!active||!reading)return;
 char s[100];
 if(reference_valid&&(uint32_t)(lv_tick_get()-reference_at)<30000)snprintf(s,sizeof(s),dsr_sc_tr("Pomiar bazowy: %u mV • %lu s"),(unsigned)(reference.adc_mv*2),(unsigned long)((lv_tick_get()-reference_at)/1000));
 else snprintf(s,sizeof(s),"%s",dsr_sc_tr(waiting_read?"Odczyt napięcia…":"Brak świeżego pomiaru bazowego."));
 text(reading,s);
}
static void apply(lv_event_t *event)
{
 (void)event;if(pending||!known)return;
 uint16_t mv,gain;
 if(!reference_valid||(uint32_t)(lv_tick_get()-reference_at)>=30000||
    !dsr_battery_parse_number(lv_textarea_get_text(reference_field),&mv)||!dsr_battery_reference_gain(&reference,mv,&gain)){
  text(status,"Zmierz ponownie. Wpisz 2500–4350 mV; korekta maks. ±10%.");return;
 }
 close_keyboard(NULL);draft.gain_permille=gain;char n[32];snprintf(n,sizeof(n),"%u / 1000",gain);text(gain_label,n);changed(NULL);
}
static void unity(lv_event_t *event)
{(void)event;if(pending||!known)return;draft.gain_permille=1000;text(gain_label,"1000 / 1000");changed(NULL);}
static void replace_confirm(lv_event_t *event)
{
 (void)event;stay(NULL);dsr_battery_profile_defaults(&draft);known=true;uncertain_draft=true;
 populate();controls();text(status,"Nowy profil jest wyłączony. Sprawdź dane i zapisz.");
}
static void replace(lv_event_t *event)
{
 (void)event;if(pending||dialog)return;
 dialog=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);lv_obj_set_style_bg_opa(dialog,180,0);lv_obj_add_flag(dialog,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *panel=dsr_sc_panel(dialog,12,117,296,246,DSR_SC_PANEL,14);
 dsr_sc_text(panel,"Utworzyć nowy profil?",14,16,268,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(panel,"Tylko profil baterii zostanie zastąpiony po naciśnięciu Zapisz.",14,53,268,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(panel,"Anuluj",12,188,130,stay,NULL);dsr_ui_button(panel,"Utwórz",154,188,130,replace_confirm,NULL);
}
void dsr_ui_battery_calibration_result(const dsr_event_t *event,dsr_job_kind_t kind)
{
 bool ours=pending;pending=false;
 if(event->result==DSR_OK&&dsr_battery_profile_valid(&event->battery_profile)&&
    (kind!=DSR_JOB_BATTERY_PROFILE_SAVE||!ours||dsr_battery_profile_equal(&submitted,&event->battery_profile))){
  baseline=event->battery_profile;known=true;
  bool keep_draft=kind==DSR_JOB_BATTERY_PROFILE_LOAD&&uncertain_draft&&active;
  if(!keep_draft)draft=baseline;
  uncertain_draft=false;
  if(active){if(!keep_draft)populate();text(status,keep_draft?"Zmiany obowiązują po zapisaniu.":"Ustawienia zapisane.");}
 }else{
  /* A failed commit/readback can be uncertain. Retain the draft, require a
   * successful reload before another write, and never claim it was saved. */
  known=false;if(kind==DSR_JOB_BATTERY_PROFILE_SAVE)uncertain_draft=true;
  if(active)text(status,"Zapis lub odczyt niepotwierdzony. Wczytaj profil ponownie.");
 }
 loading=false;if(active)controls();
}
static lv_obj_t *number(lv_obj_t *parent,const char *name,int y,const char *placeholder)
{
 dsr_sc_text(parent,name,12,y,272,&dsr_roboto_14,DSR_SC_MUTED);
 lv_obj_t *f=lv_textarea_create(parent);lv_obj_set_pos(f,12,y+35);lv_obj_set_size(f,272,44);dsr_ui_textarea(f);
 lv_textarea_set_one_line(f,true);lv_textarea_set_max_length(f,5);lv_textarea_set_accepted_chars(f,"0123456789");lv_textarea_set_placeholder_text(f,placeholder);
 lv_obj_add_event_cb(f,focus,LV_EVENT_FOCUSED,NULL);lv_obj_add_event_cb(f,focus,LV_EVENT_CLICKED,NULL);lv_obj_add_event_cb(f,changed,LV_EVENT_VALUE_CHANGED,NULL);return f;
}
void dsr_ui_battery_calibration_open(lv_event_t *event)
{
 (void)event;bool was_pending=pending;uncertain_draft=false;
 known=dsr_ui_battery_profile_current(&baseline);draft=baseline;
 if(!dsr_battery_profile_valid(&draft))dsr_battery_profile_defaults(&draft);
 lv_obj_t *root=dsr_ui_screen("Kalibracja baterii",false);active=true;pending=was_pending;
 body=dsr_sc_panel(root,12,86,296,260,DSR_SC_BG,0);lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 lv_obj_t *intro=dsr_sc_panel(body,0,0,296,110,DSR_SC_CARD,10);
 dsr_sc_text(intro,"Tylko ogniwo 1S Li-Po / Li-ion 3,6–3,7 V, ładowane do 4,2 V. Profil nie zmienia ładowarki.",12,10,272,&dsr_roboto_14,DSR_SC_TEXT);
 lv_obj_t *form=dsr_sc_panel(body,0,118,296,548,DSR_SC_CARD,10);
 dsr_sc_text(form,"Typ ogniwa",12,10,272,&dsr_roboto_14,DSR_SC_MUTED);
 type=lv_dropdown_create(form);lv_obj_set_pos(type,12,35);lv_obj_set_size(type,272,44);
 char choices[180];snprintf(choices,sizeof(choices),"%s\nLi-Po 1S (4.2 V)\nLi-ion 1S (4.2 V)",dsr_sc_tr("Wyłącz szacowanie"));lv_dropdown_set_options(type,choices);lv_obj_set_style_text_font(type,dsr_sc_font(&dsr_roboto_14),0);dsr_sc_border_width(type,1,0);dsr_sc_border_color(type,dsr_sc_color(DSR_SC_EDGE),0);lv_obj_add_event_cb(type,changed,LV_EVENT_VALUE_CHANGED,NULL);
 static const char *const names[]={"Napięcie nominalne (3600 / 3700 mV)","Pojemność z etykiety (1–20000 mAh)","Napięcie dla 0% (2800–3600 mV)","Napięcie dla 50% (3500–4100 mV)","Napięcie dla 100% (4000–4250 mV)"};
 for(unsigned i=0;i<FIELD_COUNT;i++)fields[i]=number(form,names[i],90+(int)i*90,"");
 lv_obj_t *trend_card=dsr_sc_panel(body,0,674,296,160,DSR_SC_CARD,10);
 dsr_sc_text(trend_card,"Animacja z trendu napięcia",12,10,194,&dsr_roboto_14,DSR_SC_TEXT);
 trend_switch=lv_switch_create(trend_card);lv_obj_set_pos(trend_switch,216,10);lv_obj_set_size(trend_switch,64,44);dsr_sc_border_width(trend_switch,1,0);dsr_sc_border_color(trend_switch,dsr_sc_color(DSR_SC_EDGE),0);lv_obj_set_style_bg_color(trend_switch,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR|LV_STATE_CHECKED);lv_obj_add_event_cb(trend_switch,changed,LV_EVENT_VALUE_CHANGED,NULL);
 dsr_sc_text(trend_card,"Po min. 10 s wzrostu i min. 20 mV: prawdopodobne ładowanie. Spadek, zastój lub brak pomiarów wygasza animację.",12,64,272,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *cal=dsr_sc_panel(body,0,842,296,350,DSR_SC_CARD,10);
 dsr_sc_text(cal,"Korekta pomiaru",12,10,272,&dsr_roboto_18,DSR_SC_TEXT);
 reading=dsr_sc_text(cal,"Brak świeżego pomiaru bazowego.",12,41,272,&dsr_roboto_12,DSR_SC_MUTED);
 read_button=dsr_ui_button(cal,"Zmierz napięcie",12,78,272,measure,NULL);
 reference_field=number(cal,"Równoczesny odczyt multimetru (mV)",138,"");
 apply_button=dsr_ui_button(cal,"Oblicz korektę",12,224,272,apply,NULL);
 gain_label=dsr_sc_text(cal,"",12,282,130,&dsr_roboto_14,DSR_SC_GLOW);
 unity_button=dsr_ui_button(cal,"Korekta 1:1",148,282,136,unity,NULL);
 lv_obj_t *note=dsr_sc_panel(body,0,1200,296,175,DSR_SC_PANEL,10);
 dsr_sc_text(note,"~% to szacunek z trzech punktów napięcia. Obciążenie, temperatura i USB wpływają na wynik. Pojemność nie mierzy prądu ani czasu pracy. Animacja z trendu nie potwierdza ładowania.",12,10,272,&dsr_roboto_14,DSR_SC_MUTED);
 error_panel=dsr_sc_panel(root,12,86,296,260,DSR_SC_CARD,10);
 dsr_sc_text(error_panel,"Zapis lub odczyt niepotwierdzony. Wczytaj profil ponownie.",12,12,272,&dsr_roboto_18,0xffc107);
 new_profile=dsr_ui_button(error_panel,"Nowy profil",12,136,272,replace,NULL);
 reload=dsr_ui_button(error_panel,"Wczytaj ponownie",12,196,272,load,NULL);
 status=dsr_sc_text(root,pending?(loading?"Odczyt profilu baterii…":"Zapisywanie ustawień…"):known?"Sprawdź dane ogniwa i progi. Wartości wstępne są przybliżone.":"Zapis lub odczyt niepotwierdzony. Wczytaj profil ponownie.",14,350,292,&dsr_roboto_12,known?DSR_SC_MUTED:0xffc107);lv_obj_set_height(status,60);
 dsr_ui_button(root,"Wstecz",12,420,108,back,NULL);save=dsr_ui_button(root,"Zapisz",128,420,180,commit,NULL);
 keyboard=lv_keyboard_create(root);dsr_ui_keyboard(keyboard);lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_READY,NULL);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_CANCEL,NULL);
 populate();controls();
 text(status,pending?(loading?"Odczyt profilu baterii…":"Zapisywanie ustawień…"):known?"Sprawdź dane ogniwa i progi. Wartości wstępne są przybliżone.":"Zapis lub odczyt niepotwierdzony. Wczytaj profil ponownie.");
 reference_valid=dsr_ui_battery_reference(&reference,&reference_at)&&reference.adc_mv>0;dsr_ui_battery_calibration_tick();
}
