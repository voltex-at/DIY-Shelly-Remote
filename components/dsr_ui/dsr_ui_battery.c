#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
#include "dsr_system_info.h"
#include "dsr_board.h"
#include <stdio.h>
#include <string.h>

static dsr_battery_sample_t sample;
static dsr_result_t result=DSR_IO;
static bool attempted,pending,active,rejected,from_home;
static uint32_t measured,monitor_at;
static dsr_battery_profile_t profile;
static dsr_battery_trend_t trend;
static bool profile_known;
void dsr_ui_battery_init(void)
{profile_known=dsr_battery_profile_load(&profile)==ESP_OK;monitor_at=lv_tick_get()-15000;dsr_battery_trend_reset(&trend);}
bool dsr_ui_battery_profile_current(dsr_battery_profile_t *out)
{if(out)*out=profile;return profile_known;}
bool dsr_ui_battery_reference(dsr_battery_sample_t *out,uint32_t *at)
{
 if(!attempted||pending||result!=DSR_OK||!sample.has_voltage||(uint32_t)(lv_tick_get()-measured)>=30000)return false;
 if(out)*out=sample;
 if(at)*at=measured;
 return true;
}
void dsr_ui_battery_profile_result(const dsr_event_t *event,dsr_job_kind_t kind)
{
 if(event->result==DSR_OK&&dsr_battery_profile_valid(&event->battery_profile)){
  profile=event->battery_profile;profile_known=true;
  /* A saved correction must never reinterpret a cached old sample in place. */
  sample.has_percent=false;sample.percent_estimated=false;attempted=false;
  monitor_at=lv_tick_get()-15000;
 }else profile_known=false;
 sample.has_percent=false;sample.percent_estimated=false;attempted=false;
 dsr_battery_trend_reset(&trend);
 dsr_ui_battery_calibration_result(event,kind);
 dsr_ui_battery_tick();
}
static lv_obj_t *header_value,*value,*status,*age,*measure,*diag,*percent_value,*note_value;
static lv_obj_t *header_icon;
static int header_fill=-1;
static bool header_charging,header_low,header_inferred;
/* Only pointers; the five visible values are owned/freed by their screen. */
static lv_obj_t *memory_values[5],*memory_detail;
static uint32_t diag_at;
static void header_deleted(lv_event_t *event)
{if(lv_event_get_target(event)==header_value)header_value=NULL;}
static void icon_deleted(lv_event_t *event)
{if(lv_event_get_target(event)==header_icon)header_icon=NULL;}
static void battery_draw(lv_event_t *event)
{
 lv_obj_t *o=lv_event_get_target(event);lv_area_t a;lv_obj_get_coords(o,&a);lv_layer_t *layer=lv_event_get_layer(event);
 uint32_t color=header_low?DSR_CYBER_DANGER:DSR_SC_GLOW;
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_opa=LV_OPA_TRANSP;d.border_width=DSR_CYBER_BORDER_WIDTH;d.border_color=dsr_sc_color(color);d.radius=3;
 lv_area_t cell={a.x1+5,a.y1+3,a.x1+35,a.y1+20};lv_draw_rect(layer,&d,&cell);
 d.border_width=0;d.radius=1;d.bg_opa=LV_OPA_COVER;d.bg_color=dsr_sc_color(color);
 lv_area_t tip={a.x1+36,a.y1+8,a.x1+38,a.y1+15};lv_draw_rect(layer,&d,&tip);
 if(header_fill>0){lv_area_t fill={cell.x1+4,cell.y1+4,cell.x1+3+23*header_fill/100,cell.y2-4};if(fill.x2>=fill.x1)lv_draw_rect(layer,&d,&fill);}
 if(header_fill<0){lv_draw_label_dsc_t t;lv_draw_label_dsc_init(&t);t.text="?";t.font=&dsr_roboto_12;t.color=dsr_sc_color(color);t.align=LV_TEXT_ALIGN_CENTER;lv_draw_label(layer,&t,&cell);}
 if(header_charging){lv_draw_label_dsc_t t;lv_draw_label_dsc_init(&t);t.text=header_inferred?"~":"+";t.font=&dsr_roboto_12;t.color=dsr_sc_color(DSR_SC_TEXT);t.align=LV_TEXT_ALIGN_CENTER;lv_draw_label(layer,&t,&cell);}
}
static void label(lv_obj_t *obj,const char *text)
{if(obj&&strcmp(lv_label_get_text(obj),dsr_sc_tr(text)))dsr_sc_label_set_text(obj,text);}
void dsr_ui_battery_hide(void){active=false;value=status=age=measure=diag=memory_detail=percent_value=note_value=NULL;memset(memory_values,0,sizeof(memory_values));}
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
 dsr_ui_battery_calibration_tick();
 uint32_t interval=profile.trend_enabled?1000:20000;
 if(dsr_board_display_sleeping()||dsr_ui_battery_calibration_active())dsr_battery_trend_reset(&trend);
 if(profile_known&&profile.kind!=DSR_BATTERY_NONE&&!pending&&!dsr_ui_battery_calibration_active()&&!dsr_board_display_sleeping()&&
    (uint32_t)(lv_tick_get()-monitor_at)>=interval){
  monitor_at=lv_tick_get();
  if(dsr_network_request_battery_auto()==DSR_OK){pending=true;if(measure)lv_obj_add_state(measure,LV_STATE_DISABLED);}
  else monitor_at-=interval-(interval<5000?interval:5000); /* Bounded retry, no backlog. */
 }
 uint32_t elapsed=lv_tick_get()-measured;
 bool valid=attempted&&result==DSR_OK&&sample.has_voltage;
 bool fresh=attempted&&result==DSR_OK&&elapsed<30000;
 char text[64];
 if(header_icon){
  bool inferred=profile_known&&profile.trend_enabled&&dsr_battery_trend_likely(&trend,lv_tick_get());
  bool charging=fresh&&((sample.has_charge_state&&sample.charging)||inferred)&&!dsr_board_display_sleeping();
  int fill=fresh&&sample.has_percent&&sample.percent<=100?sample.percent:-1;
  /* Fixed tick, no timer/animation allocation. 1.6 s fill + .4 s hold. */
  if(charging){uint32_t phase=lv_tick_get()%2000;fill=phase<1600?(int)(phase/16):100;}
  bool low=fresh&&sample.has_percent&&sample.percent<=10;
  if(fill!=header_fill||charging!=header_charging||low!=header_low||inferred!=header_inferred){header_fill=fill;header_charging=charging;header_low=low;header_inferred=inferred;lv_obj_invalidate(header_icon);}
 }
 if(header_value){
  if(fresh&&sample.has_percent&&sample.percent<=100)snprintf(text,sizeof(text),sample.percent_estimated?"~%u%%":"%u%%",sample.percent);
  else if(valid&&fresh)snprintf(text,sizeof(text),"%u.%02uV",sample.bat_mv/1000,(sample.bat_mv%1000)/10);
  else snprintf(text,sizeof(text),"—");
  label(header_value,text);
 }
 if(!active)return;
 if(valid)snprintf(text,sizeof(text),"%u.%03u V",sample.bat_mv/1000,sample.bat_mv%1000);
 else snprintf(text,sizeof(text),"— V");
 label(value,text);
 if(fresh&&sample.has_percent)snprintf(text,sizeof(text),sample.percent_estimated?"~%u%%":"%u%%",sample.percent);else snprintf(text,sizeof(text),"—%%");
 label(percent_value,text);
 label(note_value,dsr_battery_trend_likely(&trend,lv_tick_get())?"Prawdopodobne ładowanie • trend min. 10 s":profile_known&&profile.kind!=DSR_BATTERY_NONE?"~%: szacunek. Brak detekcji baterii i ładowania.":"Ładowanie, ogniwo i procent: brak wiarygodnych danych.");
 label(status,pending?"Odczyt napięcia…":rejected?"Pilot jest zajęty. Spróbuj ponownie.":!attempted?"Pomiar na żądanie":result!=DSR_OK?"Pomiar niedostępny. Spróbuj ponownie.":!sample.has_voltage?"Kalibracja ADC niedostępna. Napięcie nieznane.":elapsed>=30000?"Pomiar nieaktualny. Zmierz ponownie.":sample.percent_estimated?"Szacunek z profilu • wpływ obciążenia i temperatury":"Napięcie BAT • dzielnik nominalny 2:1");
 if(status&&dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)lv_obj_set_style_text_color(status,dsr_sc_color(pending?DSR_SC_BLUE:rejected||(attempted&&(!valid||elapsed>=30000))?0xffc107:DSR_SC_TEXT),0);
 if(attempted){snprintf(text,sizeof(text),dsr_sc_tr("Ostatni odczyt: %lu s temu"),(unsigned long)(elapsed/1000));label(age,text);}
 else label(age,"Brak pomiaru");
 /* Responsiveness counters (0.7.52), refreshed once per second. */
 if((diag||memory_values[0])&&(uint32_t)(lv_tick_get()-diag_at)>=1000){
  diag_at=lv_tick_get();dsr_system_info_t info;dsr_system_info_snapshot(&info);dsr_system_runtime_t run;dsr_system_runtime(&run);lv_mem_monitor_t mem;lv_mem_monitor(&mem);
  char lines[256];
  if(memory_values[0]){
   const unsigned long values[]={(unsigned long)(info.internal_free/1024),(unsigned long)(info.psram_free/1024),(unsigned long)(mem.free_size/1024),(unsigned long)run.ui_stack_free};
   for(unsigned i=0;i<4;i++){snprintf(text,sizeof(text),"%lu %s",values[i],i==3?"B":"KB");label(memory_values[i],text);}
   label(memory_values[4],run.ble_active?"włączone":"wyłączone");
   snprintf(lines,sizeof(lines),"RAM min: %lu KB\n%s: %lu KB",(unsigned long)(run.internal_min_free/1024),dsr_sc_tr("Blok"),(unsigned long)(info.internal_largest/1024));label(memory_detail,lines);
  }else{
   snprintf(lines,sizeof(lines),dsr_sc_tr("RAM: %lu KB wolne • min %lu KB • blok %lu KB\nPSRAM: %lu KB • grafika: %lu KB wolne\nStos ekranu: %lu B zapasu • BLE: %s"),
   (unsigned long)(info.internal_free/1024),(unsigned long)(run.internal_min_free/1024),(unsigned long)(info.internal_largest/1024),(unsigned long)(info.psram_free/1024),(unsigned long)(mem.free_size/1024),
   (unsigned long)run.ui_stack_free,dsr_sc_tr(run.ble_active?"włączone":"wyłączone"));
   if(strcmp(lv_label_get_text(diag),lines))lv_label_set_text(diag,lines);
  }
 }
}
void dsr_ui_battery_result(const dsr_event_t *event)
{
 dsr_ui_battery_calibration_sample(event);
 pending=false;attempted=true;measured=monitor_at=lv_tick_get();sample=event->battery;result=event->result;
 dsr_battery_trend_update(&trend,result==DSR_OK?&sample:NULL,lv_tick_get(),profile_known&&profile.kind!=DSR_BATTERY_NONE&&profile.trend_enabled&&!dsr_board_display_sleeping()&&!dsr_ui_battery_calibration_active());
 if(active&&measure)lv_obj_remove_state(measure,LV_STATE_DISABLED);
 dsr_ui_battery_tick();
}
static void show(void);
void dsr_ui_battery_return(void){show();}
void dsr_ui_battery_open(lv_event_t *event){(void)event;if(dsr_ui_battery_calibration_back())return;from_home=false;show();}
/* Bottom bar entry (former "Energia", 0.7.51): back returns to the dashboard. */
void dsr_ui_battery_show(void){from_home=true;show();}
static void show(void)
{
 lv_obj_t *root=dsr_ui_screen("Bateria i zasilanie",false);active=true;
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 if(hud){
  lv_obj_t *body=dsr_sc_panel(root,12,86,296,322,DSR_SC_BG,0);
  lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
  lv_obj_t *power=dsr_sc_panel(body,0,0,296,114,DSR_SC_CARD,10);
  dsr_sc_border_color(power,dsr_sc_color(DSR_SC_GLOW),0);
  dsr_sc_icon(power,DSR_ICON_POWER_SUPPLY,12,8,40,DSR_SC_GLOW);
  value=dsr_sc_text(power,"— V",64,9,156,&dsr_roboto_28,DSR_SC_TEXT);
  percent_value=dsr_sc_text(power,"—%",220,14,64,&dsr_roboto_18,DSR_SC_GLOW);
  status=dsr_sc_text(power,"Pomiar na żądanie",12,48,272,&dsr_roboto_14,DSR_SC_TEXT);
  dsr_sc_panel(power,12,86,272,1,DSR_SC_BLUE,0);
  dsr_sc_icon(power,DSR_ICON_CLOCK,12,90,20,DSR_SC_GLOW);
  age=dsr_sc_text(power,"Brak pomiaru",42,94,242,&dsr_roboto_12,DSR_SC_MUTED);
  lv_obj_t *note=dsr_sc_panel(body,0,122,296,50,DSR_SC_PANEL,8);dsr_ui_cyber_warning(note,false);
  dsr_sc_icon(note,DSR_ICON_WARNING,10,12,24,0xffc107);
  note_value=dsr_sc_text(note,"Ładowanie, ogniwo i procent: brak wiarygodnych danych.",42,8,242,&dsr_roboto_12,0xffc107);
  dsr_ui_button(body,"Kalibracja baterii",0,180,296,dsr_ui_battery_calibration_open,NULL);
  lv_obj_t *card=dsr_sc_panel(body,0,236,296,250,DSR_SC_CARD,10);
  dsr_sc_border_color(card,dsr_sc_color(DSR_SC_GLOW),0);
  dsr_sc_icon(card,DSR_ICON_CPU,12,8,24,DSR_SC_GLOW);dsr_sc_text(card,"Pamięć i radio",44,9,240,&dsr_roboto_18,DSR_SC_TEXT);
  dsr_sc_panel(card,12,36,272,1,DSR_SC_BLUE,0);
  const char *names[]={"RAM","PSRAM","Grafika","Stos ekranu","BLE"};
  const dsr_ui_icon_t icons[]={DSR_ICON_MEMORY,DSR_ICON_PSRAM,DSR_ICON_GRAPHICS,DSR_ICON_SCENES,DSR_ICON_BLUETOOTH};
  for(unsigned i=0;i<5;i++){
   int y=40+(int)i*32;
   dsr_sc_icon(card,icons[i],12,y,24,DSR_SC_GLOW);dsr_sc_text(card,names[i],44,y+4,112,&dsr_roboto_16,DSR_SC_MUTED);
   memory_values[i]=dsr_sc_text(card,"—",158,y+3,126,&dsr_roboto_18,DSR_SC_TEXT);
   lv_obj_set_style_text_align(memory_values[i],LV_TEXT_ALIGN_RIGHT,0);dsr_sc_panel(card,12,y+28,272,1,DSR_SC_EDGE,0);
  }
  memory_detail=dsr_sc_text(card,"",12,208,272,&dsr_roboto_12,DSR_SC_MUTED);
  dsr_ui_button(root,"Wstecz",12,420,108,back,NULL);
  measure=dsr_ui_button(root,"Zmierz napięcie",128,420,180,request,NULL);
  diag_at=lv_tick_get()-1000;if(pending)lv_obj_add_state(measure,LV_STATE_DISABLED);dsr_ui_battery_tick();return;
 }
 lv_obj_t *body=dsr_sc_panel(root,12,86,296,260,DSR_SC_BG,0);
 lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 dsr_sc_text(body,"Napięcie wejścia BAT",2,4,292,&dsr_roboto_14,DSR_SC_MUTED);
 value=dsr_sc_text(body,"— V",2,24,206,&dsr_roboto_28,DSR_SC_TEXT);
 percent_value=dsr_sc_text(body,"—%",212,30,80,&dsr_roboto_18,DSR_SC_GLOW);
 status=dsr_sc_text(body,"Pomiar na żądanie",2,62,292,&dsr_roboto_12,DSR_SC_TEXT);
 age=dsr_sc_text(body,"Brak pomiaru",2,98,292,&dsr_roboto_12,DSR_SC_MUTED);
 note_value=dsr_sc_text(body,"Ładowanie, ogniwo i procent: brak wiarygodnych danych.",2,120,292,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_ui_button(body,"Kalibracja baterii",0,164,296,dsr_ui_battery_calibration_open,NULL);
 lv_obj_t *card=dsr_sc_panel(body,0,220,296,126,DSR_SC_CARD,12);
 dsr_sc_text(card,"Pamięć i radio",12,8,272,&dsr_roboto_14,DSR_SC_TEXT);
 diag=dsr_sc_text(card,"…",12,32,276,&dsr_roboto_14,DSR_SC_MUTED);diag_at=lv_tick_get()-1000;
 measure=dsr_ui_button(root,"Zmierz napięcie",12,359,296,request,NULL);
 if(pending)lv_obj_add_state(measure,LV_STATE_DISABLED);
 dsr_ui_button(root,"Wstecz",12,420,296,back,NULL);dsr_ui_battery_tick();
}
void dsr_ui_battery_header(lv_obj_t *root)
{
 lv_obj_t *button=dsr_sc_button(root,276,0,44,44,DSR_SC_BG,0,dsr_ui_battery_open,NULL);
 lv_obj_set_style_bg_opa(button,LV_OPA_TRANSP,0);
 /* Empty icon is intentional when only a BAT voltage is known. */
 header_icon=button;lv_obj_add_event_cb(button,battery_draw,LV_EVENT_DRAW_MAIN_END,NULL);lv_obj_add_event_cb(button,icon_deleted,LV_EVENT_DELETE,NULL);
 header_fill=-2;header_value=dsr_sc_text(button,"—",0,26,44,&dsr_roboto_11,DSR_SC_TEXT);
 lv_obj_set_style_text_align(header_value,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_add_event_cb(header_value,header_deleted,LV_EVENT_DELETE,NULL);dsr_ui_battery_tick();
}
