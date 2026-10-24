#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
#include "dsr_system_info.h"
#include "dsr_board.h"
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_attr.h"
#else
#define EXT_RAM_BSS_ATTR
#endif
/* UI-owned cached state, never accessed by an ISR or while cache is disabled.
 * Keep it beside the LVGL pool in PSRAM, preserving the internal-RAM budget. */

static EXT_RAM_BSS_ATTR dsr_battery_sample_t sample;
static dsr_result_t result=DSR_IO;
static EXT_RAM_BSS_ATTR bool attempted,pending,manual_pending,active,rejected,from_home;
static EXT_RAM_BSS_ATTR uint32_t measured,monitor_at;
static EXT_RAM_BSS_ATTR dsr_battery_profile_t profile;
static EXT_RAM_BSS_ATTR dsr_battery_trend_t trend;
static EXT_RAM_BSS_ATTR bool profile_known;
/* One 50 ms frame timer, created once and paused unless the charging fill is
 * shown (never while the display sleeps): smooth, with no allocation churn. */
static EXT_RAM_BSS_ATTR lv_timer_t *charge_timer;
static void charge_frame(lv_timer_t *timer);
static void refresh_values(void);
void dsr_ui_battery_init(void)
{
 profile_known=dsr_battery_profile_load(&profile)==ESP_OK;monitor_at=lv_tick_get()-15000;dsr_battery_trend_reset(&trend);
 if(!charge_timer){charge_timer=lv_timer_create(charge_frame,50,NULL);if(charge_timer)lv_timer_pause(charge_timer);}
}
bool dsr_ui_battery_profile_current(dsr_battery_profile_t *out)
{if(out)*out=profile;return profile_known;}
/* Last confirmed sample remains usable during a pending refresh. A profile
 * change already invalidates has_percent; stale/failed values never pass. */
bool dsr_ui_battery_level(uint8_t *percent)
{
 if(!attempted||result!=DSR_OK||!sample.has_percent||sample.percent>100||(uint32_t)(lv_tick_get()-measured)>=30000)return false;
 if(percent)*percent=sample.percent;
 return true;
}
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
 refresh_values();
}
static EXT_RAM_BSS_ATTR lv_obj_t *header_value,*value,*status,*age,*measure,*percent_value,*note_value;
static EXT_RAM_BSS_ATTR lv_obj_t *header_icon;
static int header_fill=-1;
static EXT_RAM_BSS_ATTR bool header_charging,header_low,header_inferred;
/* Only pointers; the five visible values are owned/freed by their screen. */
static EXT_RAM_BSS_ATTR lv_obj_t *memory_values[5],*memory_bars[5],*memory_detail;
static EXT_RAM_BSS_ATTR uint32_t diag_at;
static void header_deleted(lv_event_t *event)
{if(lv_event_get_target(event)==header_value)header_value=NULL;}
static void icon_deleted(lv_event_t *event)
{if(lv_event_get_target(event)==header_icon)header_icon=NULL;}
static void battery_draw(lv_event_t *event)
{
 lv_obj_t *o=lv_event_get_target(event);lv_area_t a;lv_obj_get_coords(o,&a);lv_layer_t *layer=lv_event_get_layer(event);
 uint32_t color=header_low?DSR_CYBER_DANGER:DSR_SC_GLOW;
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_opa=LV_OPA_TRANSP;d.border_width=DSR_CYBER_BORDER_WIDTH;d.border_color=dsr_sc_color(color);d.radius=3;
 lv_area_t cell={a.x1+1,a.y1+10,a.x1+46,a.y1+32};lv_draw_rect(layer,&d,&cell);
 d.border_width=0;d.radius=1;d.bg_opa=LV_OPA_COVER;d.bg_color=dsr_sc_color(color);
 lv_area_t tip={a.x1+47,a.y1+17,a.x1+49,a.y1+25};lv_draw_rect(layer,&d,&tip);
 /* The dim fill stays behind the percentage: no second row or charge glyph
  * competes with the value. Unknown level remains an empty outline + dash. */
 d.bg_opa=LV_OPA_30;
 if(header_fill>0){lv_area_t fill={cell.x1+3,cell.y1+3,cell.x1+2+40*header_fill/100,cell.y2-3};if(fill.x2>=fill.x1)lv_draw_rect(layer,&d,&fill);}
}
static void charge_frame(lv_timer_t *timer)
{
 if(!header_icon||!header_charging||dsr_board_display_sleeping()){lv_timer_pause(timer);return;}
 uint32_t phase=lv_tick_get()%2000;int fill=phase<1600?(int)(phase/16):100;
 if(fill!=header_fill){header_fill=fill;lv_obj_invalidate(header_icon);}
}
static void label(lv_obj_t *obj,const char *text)
{if(obj&&strcmp(lv_label_get_text(obj),dsr_sc_tr(text)))dsr_sc_label_set_text(obj,text);}
void dsr_ui_battery_hide(void){active=false;value=status=age=measure=memory_detail=percent_value=note_value=NULL;memset(memory_values,0,sizeof(memory_values));memset(memory_bars,0,sizeof(memory_bars));}
static void memory_free(unsigned index,size_t available,size_t total,bool stack)
{
 char text[64];size_t unit=stack?1:1024;
 bool known=total&&available<=total;
 if(known)snprintf(text,sizeof(text),dsr_sc_tr(stack?"Min. %lu/%lu B":"Wolne %lu/%lu KiB"),(unsigned long)(available/unit),(unsigned long)(total/unit));
 else snprintf(text,sizeof(text),dsr_sc_tr(stack?"Min. %lu/— B":"Wolne %lu/— KiB"),(unsigned long)(available/unit));
 label(memory_values[index],text);
 /* Unknown capacity is distinct from a measured empty heap. No fake ratio,
  * no overflow on 32-bit size_t, and no new widgets or bar animation. */
 lv_bar_set_value(memory_bars[index],known?(int32_t)((uint64_t)available*100/total):0,LV_ANIM_OFF);
 if(known)lv_obj_remove_state(memory_bars[index],LV_STATE_DISABLED);else lv_obj_add_state(memory_bars[index],LV_STATE_DISABLED);
}
static void back(lv_event_t *event){(void)event;if(from_home)dsr_network_home(NULL);else dsr_ui_open_settings();}
static void request(lv_event_t *event)
{
 (void)event;
 /* A real tap may adopt an already-running background ADC read. Keep one
  * worker job; only this explicit user intent changes the button/status. */
 dsr_result_t accepted=pending?DSR_OK:dsr_network_request_battery();
 rejected=accepted!=DSR_OK;
 if(accepted==DSR_OK){pending=manual_pending=true;if(measure)lv_obj_add_state(measure,LV_STATE_DISABLED);}
 else{refresh_values();return;}
 refresh_values();
}
void dsr_ui_battery_tick(void)
{
 dsr_ui_battery_calibration_tick();
 uint32_t interval=profile.trend_enabled?1000:20000;
 if(dsr_board_display_sleeping()||dsr_ui_battery_calibration_active())dsr_battery_trend_reset(&trend);
 if(profile_known&&profile.kind!=DSR_BATTERY_NONE&&!pending&&!dsr_ui_battery_calibration_active()&&!dsr_board_display_sleeping()&&
    (uint32_t)(lv_tick_get()-monitor_at)>=interval){
  monitor_at=lv_tick_get();
  /* Automatic reads update cached values in place, without imitating a tap. */
  if(dsr_network_request_battery_auto()==DSR_OK)pending=true;
  else monitor_at-=interval-(interval<5000?interval:5000); /* Bounded retry, no backlog. */
 }
 refresh_values();
}
/* Drawing a header or completing a request only paints cached values. It must
 * never start background I/O inside another control's touch callback. */
static void refresh_values(void)
{
 uint32_t elapsed=lv_tick_get()-measured;
 bool valid=attempted&&result==DSR_OK&&sample.has_voltage;
 bool fresh=attempted&&result==DSR_OK&&elapsed<30000;
 char text[64];
 if(header_icon){
  bool inferred=profile_known&&profile.trend_enabled&&dsr_battery_trend_likely(&trend,lv_tick_get());
  bool charging=fresh&&((sample.has_charge_state&&sample.charging)||inferred)&&!dsr_board_display_sleeping();
  int fill=fresh&&sample.has_percent&&sample.percent<=100?sample.percent:-1;
  /* 1.6 s fill + .4 s hold, advanced by the 50 ms frame timer below. */
  if(charging){uint32_t phase=lv_tick_get()%2000;fill=phase<1600?(int)(phase/16):100;}
  bool low=fresh&&sample.has_percent&&sample.percent<=10;
  if(fill!=header_fill||charging!=header_charging||low!=header_low||inferred!=header_inferred){header_fill=fill;header_charging=charging;header_low=low;header_inferred=inferred;lv_obj_invalidate(header_icon);}
 }
 if(charge_timer){if(header_icon&&header_charging)lv_timer_resume(charge_timer);else lv_timer_pause(charge_timer);}
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
 label(status,manual_pending?"Odczyt napięcia…":rejected?"Pilot jest zajęty. Spróbuj ponownie.":!attempted?"Pomiar na żądanie":result!=DSR_OK?"Pomiar niedostępny. Spróbuj ponownie.":!sample.has_voltage?"Kalibracja ADC niedostępna. Napięcie nieznane.":elapsed>=30000?"Pomiar nieaktualny. Zmierz ponownie.":sample.percent_estimated?"Szacunek z profilu • wpływ obciążenia i temperatury":"Napięcie BAT • dzielnik nominalny 2:1");
 if(status&&dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)lv_obj_set_style_text_color(status,dsr_sc_color(manual_pending?DSR_SC_BLUE:rejected||(attempted&&(!valid||elapsed>=30000))?0xffc107:DSR_SC_TEXT),0);
 if(attempted){snprintf(text,sizeof(text),dsr_sc_tr("Ostatni odczyt: %lu s temu"),(unsigned long)(elapsed/1000));label(age,text);}
 else label(age,"Brak pomiaru");
 /* Responsiveness counters (0.7.52), refreshed once per second. */
 if(memory_values[0]&&(uint32_t)(lv_tick_get()-diag_at)>=1000){
  diag_at=lv_tick_get();dsr_system_info_t info;dsr_system_info_snapshot(&info);dsr_system_runtime_t run;dsr_system_runtime(&run);lv_mem_monitor_t mem;lv_mem_monitor(&mem);
  memory_free(0,info.internal_free,info.internal_total,false);
  memory_free(1,info.psram_free,info.psram_total,false);
  memory_free(2,mem.free_size,mem.total_size,false);
  memory_free(3,run.ui_stack_free,dsr_board_ui_stack_bytes,true);
  label(memory_values[4],run.ble_active?"włączone":"wyłączone");
  lv_bar_set_value(memory_bars[4],run.ble_active?1:0,LV_ANIM_OFF);
  snprintf(text,sizeof(text),"RAM min: %lu KiB • %s: %lu KiB",(unsigned long)(run.internal_min_free/1024),dsr_sc_tr("Blok"),(unsigned long)(info.internal_largest/1024));label(memory_detail,text);
 }
}
void dsr_ui_battery_result(const dsr_event_t *event)
{
 dsr_ui_battery_calibration_sample(event);
 pending=manual_pending=false;attempted=true;measured=monitor_at=lv_tick_get();sample=event->battery;result=event->result;
 dsr_battery_trend_update(&trend,result==DSR_OK?&sample:NULL,lv_tick_get(),profile_known&&profile.kind!=DSR_BATTERY_NONE&&profile.trend_enabled&&!dsr_board_display_sleeping()&&!dsr_ui_battery_calibration_active());
 if(active&&measure)lv_obj_remove_state(measure,LV_STATE_DISABLED);
 refresh_values();
}
static void show(void);
void dsr_ui_battery_return(void){show();}
void dsr_ui_battery_open(lv_event_t *event){(void)event;if(dsr_ui_battery_calibration_back())return;from_home=false;show();}
/* Bottom bar entry (former "Energia", 0.7.51): back returns to the dashboard. */
void dsr_ui_battery_show(void){from_home=true;show();}
static void memory_card(lv_obj_t *body,int y)
{
 lv_obj_t *card=dsr_sc_panel(body,0,y,296,260,DSR_SC_CARD,10);
 dsr_sc_border_color(card,dsr_sc_color(DSR_SC_EDGE),0);dsr_sc_border_width(card,1,0);
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_sc_border_color(card,dsr_sc_color(DSR_SC_GLOW),0);
 dsr_sc_icon(card,DSR_ICON_CPU,12,8,24,DSR_SC_GLOW);dsr_sc_text(card,"Pamięć i radio",44,9,240,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_panel(card,12,36,272,1,DSR_SC_BLUE,0);
 const char *names[]={"RAM","PSRAM","Grafika","Stos ekranu","BLE"};
 const dsr_ui_icon_t icons[]={DSR_ICON_MEMORY,DSR_ICON_PSRAM,DSR_ICON_GRAPHICS,DSR_ICON_SCENES,DSR_ICON_BLUETOOTH};
 for(unsigned i=0;i<5;i++){
  int row=40+(int)i*32;
  dsr_sc_icon(card,icons[i],12,row,24,DSR_SC_GLOW);dsr_sc_text(card,names[i],44,row+2,88,&dsr_roboto_14,DSR_SC_MUTED);
  memory_values[i]=dsr_sc_text(card,"—",136,row+3,148,&dsr_roboto_12,DSR_SC_TEXT);
  lv_obj_set_style_text_align(memory_values[i],LV_TEXT_ALIGN_RIGHT,0);
  lv_obj_t *bar=memory_bars[i]=lv_bar_create(card);lv_obj_set_pos(bar,44,row+23);lv_obj_set_size(bar,240,6);
  lv_obj_remove_flag(bar,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);lv_bar_set_range(bar,0,i==4?1:100);
  lv_obj_set_style_bg_color(bar,dsr_sc_color(DSR_SC_EDGE),0);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,0);lv_obj_set_style_border_width(bar,0,0);lv_obj_set_style_radius(bar,3,0);
  lv_obj_set_style_bg_color(bar,dsr_sc_color(DSR_SC_GLOW),LV_PART_INDICATOR);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_border_width(bar,0,LV_PART_INDICATOR);lv_obj_set_style_radius(bar,3,LV_PART_INDICATOR);
 }
 dsr_sc_text(card,"Paski: wolne. Stos: min. od startu.\nBLE: stan modułu, nie zużycie.",12,208,272,&dsr_roboto_11,DSR_SC_MUTED);
 memory_detail=dsr_sc_text(card,"",12,238,272,&dsr_roboto_11,DSR_SC_MUTED);
}
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
  memory_card(body,236);
  dsr_ui_button(root,"Wstecz",12,420,108,back,NULL);
  measure=dsr_ui_button(root,"Zmierz napięcie",128,420,180,request,NULL);
  diag_at=lv_tick_get()-1000;if(manual_pending)lv_obj_add_state(measure,LV_STATE_DISABLED);refresh_values();return;
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
 memory_card(body,220);diag_at=lv_tick_get()-1000;
 measure=dsr_ui_button(root,"Zmierz napięcie",12,359,296,request,NULL);
 if(manual_pending)lv_obj_add_state(measure,LV_STATE_DISABLED);
 dsr_ui_button(root,"Wstecz",12,420,296,back,NULL);refresh_values();
}
void dsr_ui_battery_header(lv_obj_t *root)
{
 lv_obj_t *button=dsr_sc_button(root,256,0,52,DSR_SC_STATUS_H,DSR_SC_BG,0,dsr_ui_battery_open,NULL);
 lv_obj_set_style_bg_opa(button,LV_OPA_TRANSP,0);
 /* Empty icon is intentional when only a BAT voltage is known. */
 header_icon=button;lv_obj_add_event_cb(button,battery_draw,LV_EVENT_DRAW_MAIN_END,NULL);lv_obj_add_event_cb(button,icon_deleted,LV_EVENT_DELETE,NULL);
 header_fill=-2;header_value=dsr_sc_text(button,"—",5,15,38,&dsr_roboto_11,DSR_SC_TEXT);
 lv_obj_set_style_text_align(header_value,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_add_event_cb(header_value,header_deleted,LV_EVENT_DELETE,NULL);refresh_values();
}
