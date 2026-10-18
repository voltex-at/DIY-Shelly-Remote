#include "dsr_ui_internal.h"
#include <stdio.h>
#include <string.h>

static dsr_battery_sample_t sample;
static dsr_result_t result=DSR_IO;
static bool attempted,pending,active,rejected,from_home;
static uint32_t measured;
static lv_obj_t *header_value,*value,*status,*age,*measure;
static void header_deleted(lv_event_t *event)
{if(lv_event_get_target(event)==header_value)header_value=NULL;}
static void label(lv_obj_t *obj,const char *text)
{if(obj&&strcmp(lv_label_get_text(obj),dsr_sc_tr(text)))dsr_sc_label_set_text(obj,text);}
void dsr_ui_battery_hide(void){active=false;value=status=age=measure=NULL;}
static void back(lv_event_t *event){(void)event;if(from_home)dsr_network_home(NULL);else dsr_ui_open_settings();}
static void request(lv_event_t *event)
{
 (void)event;if(pending)return;
 dsr_result_t accepted=dsr_network_request_battery();
 rejected=accepted!=DSR_OK;
 if(accepted==DSR_OK){pending=true;if(measure)lv_obj_add_state(measure,LV_STATE_DISABLED);}
 else{dsr_ui_battery_tick();return;}
 dsr_ui_battery_tick();
}
void dsr_ui_battery_tick(void)
{
 uint32_t elapsed=lv_tick_get()-measured;
 bool valid=attempted&&result==DSR_OK&&sample.has_voltage;
 char text[64];
 if(header_value){
  if(valid&&elapsed<30000)snprintf(text,sizeof(text),"%u.%02uV",sample.bat_mv/1000,(sample.bat_mv%1000)/10);
  else snprintf(text,sizeof(text),"?");
  label(header_value,text);
 }
 if(!active)return;
 if(valid)snprintf(text,sizeof(text),"%u.%03u V",sample.bat_mv/1000,sample.bat_mv%1000);
 else snprintf(text,sizeof(text),"— V");
 label(value,text);
 label(status,pending?"Odczyt napięcia…":rejected?"Pilot jest zajęty. Spróbuj ponownie.":!attempted?"Pomiar na żądanie":result!=DSR_OK?"Pomiar niedostępny. Spróbuj ponownie.":!sample.has_voltage?"Kalibracja ADC niedostępna. Napięcie nieznane.":elapsed>=30000?"Pomiar nieaktualny. Zmierz ponownie.":"Napięcie BAT • dzielnik nominalny 2:1");
 if(attempted){snprintf(text,sizeof(text),dsr_sc_tr("Ostatni odczyt: %lu s temu"),(unsigned long)(elapsed/1000));label(age,text);}
 else label(age,"Brak pomiaru");
}
void dsr_ui_battery_result(const dsr_event_t *event)
{
 pending=false;attempted=true;measured=lv_tick_get();sample=event->battery;result=event->result;
 if(active&&measure)lv_obj_remove_state(measure,LV_STATE_DISABLED);
 dsr_ui_battery_tick();
}
static void show(void);
void dsr_ui_battery_open(lv_event_t *event){(void)event;from_home=false;show();}
/* Bottom bar entry (former "Energia", 0.7.51): back returns to the dashboard. */
void dsr_ui_battery_show(void){from_home=true;show();}
static void show(void)
{
 lv_obj_t *root=dsr_ui_screen("Bateria i zasilanie",false);active=true;
 dsr_sc_text(root,"Napięcie wejścia BAT",14,94,292,&dsr_roboto_14,DSR_SC_MUTED);
 value=dsr_sc_text(root,"— V",14,124,292,&dsr_roboto_28,DSR_SC_TEXT);
 status=dsr_sc_text(root,"Pomiar na żądanie",14,174,292,&dsr_roboto_14,DSR_SC_TEXT);
 age=dsr_sc_text(root,"Brak pomiaru",14,220,292,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(root,"Ładowanie i obecność akumulatora: brak wiarygodnych danych.",14,253,292,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_sc_text(root,"Procent baterii wymaga potwierdzenia typu ogniwa i pomiaru multimetrem.",14,305,292,&dsr_roboto_12,DSR_SC_MUTED);
 measure=dsr_ui_button(root,"Zmierz napięcie",12,359,296,request,NULL);
 if(pending)lv_obj_add_state(measure,LV_STATE_DISABLED);
 dsr_ui_button(root,"Wstecz",12,420,296,back,NULL);dsr_ui_battery_tick();
}
void dsr_ui_battery_header(lv_obj_t *root)
{
 lv_obj_t *button=dsr_sc_button(root,276,0,44,44,DSR_SC_BG,0,dsr_ui_battery_open,NULL);
 lv_obj_set_style_bg_opa(button,LV_OPA_TRANSP,0);
 lv_obj_t *outline=dsr_sc_panel(button,0,12,38,19,DSR_SC_BG,3);
 lv_obj_set_style_border_width(outline,1,0);lv_obj_set_style_border_color(outline,dsr_sc_color(DSR_SC_MUTED),0);
 dsr_sc_panel(button,38,18,3,7,DSR_SC_MUTED,1);
 header_value=dsr_sc_text(button,"?",2,16,34,&dsr_roboto_12,DSR_SC_TEXT);
 lv_obj_set_style_text_align(header_value,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_add_event_cb(header_value,header_deleted,LV_EVENT_DELETE,NULL);dsr_ui_battery_tick();
}
