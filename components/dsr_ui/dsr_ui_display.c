/* Existing display preferences, serialized by the network worker's NVS path. */
#include "dsr_ui_internal.h"
#include "dsr_board.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *slider,*timeout,*status,*save,*brightness_label;
static bool active,pending;
static esp_err_t apply_error;
void dsr_ui_display_init(uint8_t brightness,uint16_t seconds)
{apply_error=dsr_board_apply_display(brightness,seconds);}
static uint16_t extra_seconds;
static const uint16_t seconds_options[]={0,15,30,60,120,300,600,1800,3600};
void dsr_ui_display_hide(void)
{active=false;slider=timeout=status=save=brightness_label=NULL;}
static void controls(void)
{
 if(!active)return;
 lv_obj_t *items[]={slider,timeout,save};
 for(unsigned i=0;i<3;i++)if(pending)lv_obj_add_state(items[i],LV_STATE_DISABLED);else lv_obj_remove_state(items[i],LV_STATE_DISABLED);
}
static void changed(lv_event_t *event)
{
 (void)event;if(!active)return;
 dsr_sc_label_set_text_fmt(brightness_label,"Brightness: %u%%",(unsigned)lv_slider_get_value(slider));
}
static void duration(uint16_t seconds)
{
 char options[180];snprintf(options,sizeof(options),"%s\n15 s\n30 s\n60 s\n120 s\n300 s\n600 s\n1800 s\n3600 s",dsr_sc_tr("Wyłączone"));
 unsigned selected=9;for(unsigned i=0;i<9;i++)if(seconds==seconds_options[i])selected=i;
 extra_seconds=seconds;
 if(selected==9){size_t length=strlen(options);snprintf(options+length,sizeof(options)-length,"\n%u s",(unsigned)extra_seconds);}
 lv_dropdown_set_options(timeout,options);lv_dropdown_set_selected(timeout,selected);
}
static void commit(lv_event_t *event)
{
 (void)event;if(pending||!active)return;
 uint32_t choice=lv_dropdown_get_selected(timeout);
 uint16_t seconds=choice<9?seconds_options[choice]:extra_seconds;
 dsr_result_t result=dsr_network_request_display((uint8_t)lv_slider_get_value(slider),seconds);
 if(result!=DSR_OK){dsr_sc_label_set_text(status,"Pilot jest zajęty. Spróbuj ponownie.");return;}
 pending=true;controls();dsr_sc_label_set_text(status,"Zapisywanie ustawień…");
}
void dsr_ui_display_result(const dsr_event_t *event)
{
 pending=false;
 esp_err_t applied=ESP_OK;
 if(event->result==DSR_OK){applied=dsr_board_apply_display(event->brightness,event->idle_seconds);apply_error=applied;}
 if(active){
  if(event->result==DSR_OK){lv_slider_set_value(slider,event->brightness,LV_ANIM_OFF);duration(event->idle_seconds);changed(NULL);}
  controls();dsr_sc_label_set_text(status,event->result!=DSR_OK?"Nie potwierdzono zapisu. Spróbuj ponownie.":applied!=ESP_OK?"Zapisano. Błąd sterownika ekranu.":"Ustawienia zapisane.");}
}
static void back(lv_event_t *event){(void)event;dsr_ui_open_settings();}
void dsr_ui_display_open(lv_event_t *event)
{
 (void)event;
 dsr_settings_t current;dsr_network_wifi_settings(&current);
 lv_obj_t *root=dsr_ui_screen("Ekran",false);active=true;
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){
  dsr_sc_panel(root,12,82,296,92,DSR_SC_CARD,10);
  dsr_sc_panel(root,12,181,296,91,DSR_SC_CARD,10);
  dsr_sc_panel(root,12,342,296,64,DSR_SC_CARD,10);
 }
 brightness_label=dsr_sc_text(root,"",14,94,292,&dsr_roboto_14,DSR_SC_TEXT);
 slider=lv_slider_create(root);lv_obj_set_pos(slider,24,140);lv_obj_set_size(slider,272,18);
 lv_slider_set_range(slider,10,100);lv_slider_set_value(slider,current.brightness<10?10:current.brightness,LV_ANIM_OFF);
 lv_obj_add_event_cb(slider,changed,LV_EVENT_VALUE_CHANGED,NULL);changed(NULL);
 dsr_sc_text(root,"Wygaszanie ekranu",14,185,292,&dsr_roboto_14,DSR_SC_TEXT);
 timeout=lv_dropdown_create(root);lv_obj_set_pos(timeout,12,214);lv_obj_set_size(timeout,296,44);
 duration(current.idle_seconds);
 save=dsr_ui_button(root,"Zapisz",12,284,296,commit,NULL);
 status=dsr_sc_text(root,pending?"Zapisywanie ustawień…":apply_error!=ESP_OK?"Błąd sterownika ekranu.":"Zmiany obowiązują po zapisaniu.",14,350,292,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(root,"Wstecz",12,420,296,back,NULL);controls();
}
