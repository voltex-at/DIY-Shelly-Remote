#define _POSIX_C_SOURCE 200809L /* setenv/tzset for the local clock (file-local; strict -std=c11 host builds) */
#include "dsr_ui_internal.h"
#include "dsr_core.h"
#include "dsr_history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Header status bar: clock under the Wi-Fi icon and the battery icon with an
 * estimated level. The BAT voltage is the only measured quantity; the percentage
 * is an ESTIMATE for a single Li-ion cell and is always shown with "~". There is
 * no reliable charge/presence signal on this board, so no charging animation. */
#ifndef DSR_TIMEZONE_POSIX
#define DSR_TIMEZONE_POSIX "CET-1CEST,M3.5.0,M10.5.0/3" /* Europe/Vienna; override with -DDSR_TIMEZONE_POSIX */
#endif
#define FRESH_MS 60000u       /* a reading older than this is shown as unknown */
#define AUTO_MS 20000u        /* periodic BAT read while idle (firmware only) */
#define BUSY_RETRY_MS 2000u   /* worker was busy: try again soon */
#define CLOCK_RETRY_MS 300000u
#define FILL_MAX 30
#define LOW_PERCENT 10
#define NO_PERCENT 0xffu
#ifdef ESP_PLATFORM
static bool auto_enabled=true;
#else
static bool auto_enabled; /* host tests opt in with dsr_ui_status_auto(true) */
#endif
static dsr_battery_sample_t sample;
static dsr_result_t result=DSR_IO;
static bool attempted,pending,active,rejected,timezone_ready;
static uint32_t measured,next_auto,next_clock;
static lv_obj_t *button_ref,*outline,*fill,*pct_label,*clock_label,*value,*status,*age,*estimate,*measure;
static unsigned shown_percent=NO_PERCENT;
static int64_t (*clock_source)(void); /* NULL = time(); host tests inject unset/fixed clocks */

static void clock_deleted(lv_event_t *event)
{if(lv_event_get_target(event)==clock_label)clock_label=NULL;}
static void button_deleted(lv_event_t *event)
{if(lv_event_get_target(event)==button_ref)button_ref=outline=fill=pct_label=NULL,shown_percent=NO_PERCENT;}
static void label(lv_obj_t *obj,const char *text)
{if(obj&&strcmp(lv_label_get_text(obj),dsr_sc_tr(text)))dsr_sc_label_set_text(obj,text);}
void dsr_ui_battery_hide(void){active=false;value=status=age=estimate=measure=NULL;}
void dsr_ui_status_auto(bool enabled){auto_enabled=enabled;next_auto=lv_tick_get();next_clock=next_auto;}
void dsr_ui_status_clock_source(int64_t (*source)(void)){clock_source=source;}
static void back(lv_event_t *event){(void)event;dsr_ui_open_settings();}
static void request(lv_event_t *event)
{
 (void)event;if(pending)return;
 dsr_result_t accepted=dsr_network_request_battery();
 rejected=accepted!=DSR_OK;
 if(accepted==DSR_OK){pending=true;if(measure)lv_obj_add_state(measure,LV_STATE_DISABLED);}
 else{dsr_ui_battery_tick();return;}
 dsr_ui_battery_tick();
}
static void width_cb(void *object,int32_t value_px){lv_obj_set_width((lv_obj_t *)object,value_px);}
static void pulse_cb(void *object,int32_t opa){lv_obj_set_style_bg_opa((lv_obj_t *)object,(lv_opa_t)opa,0);}
static uint32_t level_color(unsigned percent)
{return percent<=LOW_PERCENT?0xe5645a:percent<30?0xe1bb70:0x6fcf97;}
/* Animates the fill to the new level; below LOW_PERCENT it also pulses. */
static void show_level(unsigned percent)
{
 if(!fill||percent==shown_percent)return;
 int32_t target=percent==NO_PERCENT?0:(int32_t)(percent*FILL_MAX/100u);
 if(percent!=NO_PERCENT&&target<2)target=2;
 lv_anim_delete(fill,width_cb);lv_anim_delete(fill,pulse_cb);
 lv_obj_set_style_bg_opa(fill,LV_OPA_COVER,0);
 if(percent!=NO_PERCENT)lv_obj_set_style_bg_color(fill,dsr_sc_color(level_color(percent)),0);
 lv_anim_t grow;lv_anim_init(&grow);lv_anim_set_var(&grow,fill);lv_anim_set_exec_cb(&grow,width_cb);
 lv_anim_set_values(&grow,lv_obj_get_width(fill),target);lv_anim_set_duration(&grow,500);lv_anim_start(&grow);
 if(percent!=NO_PERCENT&&percent<=LOW_PERCENT){
  lv_anim_t pulse;lv_anim_init(&pulse);lv_anim_set_var(&pulse,fill);lv_anim_set_exec_cb(&pulse,pulse_cb);
  lv_anim_set_values(&pulse,LV_OPA_COVER,LV_OPA_30);lv_anim_set_duration(&pulse,700);lv_anim_set_playback_duration(&pulse,700);
  lv_anim_set_repeat_count(&pulse,LV_ANIM_REPEAT_INFINITE);lv_anim_start(&pulse);
 }
 shown_percent=percent;
}
static void tick_clock(uint32_t now)
{
 if(auto_enabled&&!timezone_ready){setenv("TZ",DSR_TIMEZONE_POSIX,1);tzset();timezone_ready=true;}
 int64_t utc=clock_source?clock_source():(int64_t)time(NULL);char text[6];
 bool valid=dsr_clock_format_hhmm(utc,text);
 if(clock_label&&strcmp(lv_label_get_text(clock_label),text))dsr_sc_label_set_text(clock_label,text);
 /* The header clock needs the SNTP sync that used to be a manual button. One
  * bounded request per attempt; retried every 5 minutes while it fails. */
 if(auto_enabled&&!valid&&dsr_wifi_connected()&&(int32_t)(now-next_clock)>=0){
  next_clock=now+(dsr_history_request_clock()?CLOCK_RETRY_MS:BUSY_RETRY_MS);
 }
}
static void tick_auto_measure(uint32_t now)
{
 if(!auto_enabled||pending||(int32_t)(now-next_auto)<0)return;
 if(dsr_network_request_battery()==DSR_OK){
  pending=true;rejected=false;next_auto=now+AUTO_MS;
  if(measure)lv_obj_add_state(measure,LV_STATE_DISABLED);
 }else next_auto=now+BUSY_RETRY_MS;
}
void dsr_ui_battery_tick(void)
{
 uint32_t now=lv_tick_get(),elapsed=now-measured;
 bool valid=attempted&&result==DSR_OK&&sample.has_voltage;
 bool fresh=valid&&elapsed<FRESH_MS;
 uint8_t percent=0;bool known=fresh&&dsr_battery_estimate_percent(sample.bat_mv,&percent);
 char text[80];
 tick_clock(now);tick_auto_measure(now);
 if(pct_label){
  if(known)snprintf(text,sizeof(text),"~%u%%",(unsigned)percent);else snprintf(text,sizeof(text),"?");
  label(pct_label,text);
  show_level(known?percent:NO_PERCENT);
 }
 if(!active)return;
 if(valid)snprintf(text,sizeof(text),"%u.%03u V",sample.bat_mv/1000,sample.bat_mv%1000);
 else snprintf(text,sizeof(text),"— V");
 label(value,text);
 label(status,pending?"Odczyt napięcia…":rejected?"Pilot jest zajęty. Spróbuj ponownie.":!attempted?"Pomiar na żądanie":result!=DSR_OK?"Pomiar niedostępny. Spróbuj ponownie.":!sample.has_voltage?"Kalibracja ADC niedostępna. Napięcie nieznane.":elapsed>=FRESH_MS?"Pomiar nieaktualny. Zmierz ponownie.":"Napięcie wejścia BAT zmierzone.");
 if(attempted){snprintf(text,sizeof(text),dsr_sc_tr("Ostatni odczyt: %lu s temu"),(unsigned long)(elapsed/1000));label(age,text);}
 else label(age,"Brak pomiaru");
 if(known){snprintf(text,sizeof(text),dsr_sc_tr("Szacunek poziomu: ok. %u%%"),(unsigned)percent);label(estimate,text);}
 else label(estimate,fresh?"Szacunek poza zakresem ogniwa Li-ion.":"Szacunek poziomu: brak pomiaru");
}
void dsr_ui_battery_result(const dsr_event_t *event)
{
 pending=false;attempted=true;measured=lv_tick_get();sample=event->battery;result=event->result;
 if(active&&measure)lv_obj_remove_state(measure,LV_STATE_DISABLED);
 dsr_ui_battery_tick();
}
void dsr_ui_battery_open(lv_event_t *event)
{
 (void)event;lv_obj_t *root=dsr_ui_screen("Bateria i zasilanie",false);active=true;
 dsr_sc_text(root,"Napięcie wejścia BAT",14,94,292,&dsr_roboto_14,DSR_SC_MUTED);
 value=dsr_sc_text(root,"— V",14,124,292,&dsr_roboto_28,DSR_SC_TEXT);
 status=dsr_sc_text(root,"Pomiar na żądanie",14,174,292,&dsr_roboto_14,DSR_SC_TEXT);
 age=dsr_sc_text(root,"Brak pomiaru",14,220,292,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(root,"Ładowanie i obecność akumulatora: brak wiarygodnych danych.",14,253,292,&dsr_roboto_14,DSR_SC_MUTED);
 estimate=dsr_sc_text(root,"Szacunek poziomu: brak pomiaru",14,305,292,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(root,"Zakłada ogniwo Li-ion; wartość orientacyjna.",14,330,292,&dsr_roboto_12,DSR_SC_MUTED);
 measure=dsr_ui_button(root,"Zmierz napięcie",12,359,296,request,NULL);
 if(pending)lv_obj_add_state(measure,LV_STATE_DISABLED);
 dsr_ui_button(root,"Wstecz",12,420,296,back,NULL);dsr_ui_battery_tick();
}
void dsr_ui_battery_header(lv_obj_t *root)
{
 clock_label=dsr_sc_text(root,"--:--",232,25,44,&dsr_roboto_11,DSR_SC_MUTED);
 lv_obj_set_style_text_align(clock_label,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_add_event_cb(clock_label,clock_deleted,LV_EVENT_DELETE,NULL);
 lv_obj_t *button=dsr_sc_button(root,276,0,44,44,DSR_SC_BG,0,dsr_ui_battery_open,NULL);
 lv_obj_set_style_bg_opa(button,LV_OPA_TRANSP,0);
 button_ref=button;
 outline=dsr_sc_panel(button,3,4,34,16,DSR_SC_BG,3);
 lv_obj_set_style_border_width(outline,1,0);lv_obj_set_style_border_color(outline,dsr_sc_color(DSR_SC_MUTED),0);
 dsr_sc_panel(button,37,9,3,6,DSR_SC_MUTED,1);
 fill=dsr_sc_panel(button,5,6,0,12,DSR_SC_OFF,2);
 pct_label=dsr_sc_text(button,"?",0,25,44,&dsr_roboto_11,DSR_SC_TEXT);
 lv_obj_set_style_text_align(pct_label,LV_TEXT_ALIGN_CENTER,0);
 shown_percent=NO_PERCENT-1; /* force the first draw */
 lv_obj_add_event_cb(button,button_deleted,LV_EVENT_DELETE,NULL);dsr_ui_battery_tick();
}
