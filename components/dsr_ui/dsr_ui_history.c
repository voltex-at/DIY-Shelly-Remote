#include "dsr_ui_internal.h"
#include "dsr_history.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool active,overview;
static uint32_t selected,next_draw;
static unsigned hours=1,metric;
static void draw(void);
void dsr_ui_history_hide(void){active=false;}
static void back(lv_event_t *event)
{(void)event;if(overview)dsr_network_home(NULL);else dsr_network_device_control(selected);}
static void settings(lv_event_t *event){(void)event;dsr_network_device_settings(selected);}
static void range(lv_event_t *event){hours=(unsigned)(uintptr_t)lv_event_get_user_data(event);draw();}
static void measure(lv_event_t *event){metric=(unsigned)(uintptr_t)lv_event_get_user_data(event);draw();}
static void clock_request(lv_event_t *event){(void)event;(void)dsr_history_request_clock();draw();}
static void clock_cancel(lv_event_t *event){(void)event;dsr_history_cancel_clock();draw();}
static void open_row(lv_event_t *event){dsr_ui_history_open((uint32_t)(uintptr_t)lv_event_get_user_data(event));}
static lv_obj_t *button(lv_obj_t *parent,const char *text,int x,int y,int width,lv_event_cb_t cb,uintptr_t data,bool chosen)
{
 lv_obj_t *b=dsr_sc_button(parent,x,y,width,40,chosen?DSR_SC_BLUE:DSR_SC_CARD,8,cb,(void *)data);
 lv_obj_t *label=dsr_sc_text(b,text,2,12,width-4,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);return b;
}
static void time_label(uint32_t utc,char out[48])
{
 if(!utc){snprintf(out,48,"— UTC");return;}
 time_t value=(time_t)utc;struct tm *t=gmtime(&value);
 if(!t){snprintf(out,48,"— UTC");return;}
 snprintf(out,48,"%02d.%02d %02d:%02d UTC",t->tm_mday,t->tm_mon+1,t->tm_hour,t->tm_min);
}
static void clock_button(lv_obj_t *root,const dsr_history_status_t *s)
{
 bool busy=s->clock_pending||s->clock_busy;
 button(root,busy?"Anuluj synchronizację":"Synchronizuj czas UTC",12,426,296,busy?clock_cancel:clock_request,0,false);
}
static const char *failure(const dsr_history_status_t *s)
{
 if(s->state==DSR_HISTORY_LOADING)return "Wczytywanie historii…";
 if(s->state==DSR_HISTORY_FUTURE)return "Nowszy format historii. Pliki zachowane.";
 if(s->state==DSR_HISTORY_UNAVAILABLE)return "Historia niedostępna: brak pamięci.";
 if(s->state==DSR_HISTORY_IO)return "Błąd zapisu historii. Dane mogą być tylko w RAM.";
 if(s->clock_pending||s->clock_busy)return "Synchronizacja czasu UTC…";
 if(!s->clock_valid)return "Brak poprawnego czasu UTC. W historii pozostaje przerwa.";
 return NULL;
}
static double number(const dsr_history_point_t *p)
{return metric==0?p->power:metric==1?p->voltage:metric==2?p->current:p->energy_wh;}
static void chart(lv_obj_t *root,const dsr_history_series_t *series)
{
 const char *units[]={"W","V","A","Wh"};double low=DBL_MAX,high=-DBL_MAX;unsigned count=0,resets=0;
 if(series)for(unsigned i=0;i<series->count;i++){
  const dsr_history_point_t *p=&series->points[i];if(p->flags&(DSR_HISTORY_GAP|DSR_HISTORY_RESET))resets++;
  if(p->mask&(1u<<metric)){double v=number(p);if(v<low)low=v;if(v>high)high=v;count++;}
 }
 lv_obj_t *plot=lv_chart_create(root);lv_obj_set_pos(plot,20,203);lv_obj_set_size(plot,280,127);
 lv_obj_set_style_bg_color(plot,dsr_sc_color(DSR_SC_PANEL),0);lv_obj_set_style_border_width(plot,0,0);lv_obj_set_style_line_width(plot,0,LV_PART_ITEMS);lv_obj_set_style_size(plot,4,4,LV_PART_INDICATOR);
 lv_chart_set_type(plot,LV_CHART_TYPE_SCATTER);unsigned points=hours*12u+1u;lv_chart_set_point_count(plot,count?count:1);lv_chart_set_axis_range(plot,LV_CHART_AXIS_PRIMARY_Y,0,1000);lv_chart_set_axis_range(plot,LV_CHART_AXIS_PRIMARY_X,0,(int32_t)(points-1));
 lv_chart_series_t *line=count?lv_chart_add_series(plot,dsr_sc_color(DSR_SC_BLUE),LV_CHART_AXIS_PRIMARY_Y):NULL;
 if(count&&series){
  uint32_t first=series->from_utc-series->from_utc%DSR_HISTORY_STEP;
  unsigned plotted=0;
  for(unsigned i=0;i<series->count;i++){
   const dsr_history_point_t *p=&series->points[i];if(!(p->mask&(1u<<metric))||p->utc<first)continue;unsigned at=(p->utc-first)/DSR_HISTORY_STEP;
   if(at<points){double scaled=high==low?500:(number(p)-low)*1000/(high-low);lv_chart_set_series_value_by_id2(plot,line,plotted++,(int32_t)at,(int32_t)lround(scaled));}
  }
 }
 char text[128];if(count)snprintf(text,sizeof(text),"%.6g – %.6g %s",low,high,units[metric]);else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Brak próbek dla tego pomiaru"));
 dsr_sc_text(root,text,12,178,296,&dsr_roboto_12,DSR_SC_TEXT);
 char from[48],to[48];time_label(series?series->from_utc:0,from);time_label(series?series->to_utc:0,to);snprintf(text,sizeof(text),"%s / %s",from,to);dsr_sc_text(root,text,12,337,296,&dsr_roboto_11,DSR_SC_MUTED);
 dsr_sc_text(root,metric==3?"Wh: odczyt licznika Shelly, nie suma W.":"Punkty: rzeczywiste średnie z okien 5 min.",12,359,296,&dsr_roboto_11,DSR_SC_MUTED);
 if(resets)dsr_sc_text(root,"Przerwy lub reset licznika. Bez interpolacji.",12,379,296,&dsr_roboto_11,0xffc18c);
}
static void history_header(lv_obj_t *root,const char *title)
{
 dsr_sc_header(root,title,false,dsr_wifi_connected(),back,NULL);
 /* This header is rebuilt by explicit range changes. The pinned label DOTS
  * mode temporarily mutates its text during layout; clip preserves the exact
  * title and bounds while charts rebuild in an event callback. */
 lv_obj_t *caption=lv_obj_get_child(root,1);lv_label_set_long_mode(caption,LV_LABEL_LONG_MODE_CLIP);
}
static void detail(lv_obj_t *root)
{
 dsr_mobile_device_t d={0};bool exists=dsr_network_history_device(selected,&d);dsr_history_series_t *series=NULL;dsr_history_status_t status;
 dsr_history_status(&status);
 history_header(root,exists?d.name:"Wykresy");
 button(root,"Sterowanie",0,44,109,back,0,false);button(root,"Wykresy",111,44,101,NULL,0,true);button(root,"Ustawienia",214,44,106,settings,0,false);
 button(root,"1 h",12,96,142,range,1,hours==1);button(root,"24 h",166,96,142,range,24,hours==24);
 const char *units[]={"W","V","A","Wh"};for(unsigned i=0;i<4;i++)button(root,units[i],12+(int)i*76,142,68,measure,i,metric==i);
 if(exists&&d.identity[0]){
  dsr_history_key_t key={.layout_id=d.id,.generation=(uint8_t)d.generation,.channel=d.channel,.kind=d.endpoint_kind};memcpy(key.device_id,d.identity,sizeof(key.device_id));series=dsr_history_query(&key,hours);if(series)status=series->status;
 }
 chart(root,series);const char *message=exists&&d.identity[0]&&d.endpoint_kind!=DSR_ENDPOINT_SWITCH?(d.endpoint_kind==DSR_ENDPOINT_HT?"Historia H&T nie jest obsługiwana":(d.endpoint_kind==DSR_ENDPOINT_LIGHT||d.endpoint_kind==DSR_ENDPOINT_RGB)?"Historia światła nie jest obsługiwana":"Historia rolet nie jest obsługiwana"):failure(&status);
 if(!message&&!series)message="Brak aktywnego połączenia";
 if(!message&&series&&!series->count)message="Brak próbek. Historia zbiera rzeczywiste odczyty.";
 if(!message)message=status.durable?"Historia lokalna • zapis potwierdzony":"Bieżące okno RAM. Restart może utracić do 5 min.";
 dsr_sc_text(root,message,12,397,296,&dsr_roboto_11,DSR_SC_MUTED);clock_button(root,&status);dsr_history_series_free(series);
}
static bool duplicate(unsigned index,const dsr_mobile_device_t *device)
{
 dsr_mobile_device_t previous;
 for(unsigned i=0;i<index;i++)if(dsr_network_energy_at(i,&previous)&&previous.identity[0]&&previous.generation==device->generation&&previous.endpoint_kind==device->endpoint_kind&&previous.channel==device->channel&&!strcmp(previous.identity,device->identity))return true;
 return false;
}
static void energy(lv_obj_t *root)
{
 history_header(root,"Energia");
 double sum=0;unsigned comparable=0,total=0;dsr_mobile_device_t d;
 for(unsigned i=0;dsr_network_energy_at(i,&d);i++){total++;if(d.identity[0]&&!duplicate(i,&d)&&d.online&&d.has_power&&d.has_sample_time&&lv_tick_get()-d.sample_ms<=15000){sum+=d.power_w;comparable++;}}
 char text[144];if(comparable)snprintf(text,sizeof(text),dsr_sc_tr("%.1f W • świeże liczniki: %u/%u"),sum,comparable,total);else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Brak świeżych pomiarów mocy"));
 dsr_sc_text(root,text,12,51,296,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(root,"Suma tylko porównywalnych W. Braki nie są zerem.",12,80,296,&dsr_roboto_11,DSR_SC_MUTED);
 lv_obj_t *list=dsr_sc_panel(root,12,111,296,282,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);
 for(unsigned i=0;dsr_network_energy_at(i,&d);i++){
  lv_obj_t *row=dsr_sc_button(list,0,(int)i*82,280,74,DSR_SC_CARD,10,open_row,(void *)(uintptr_t)d.id);
  dsr_sc_text(row,d.name,10,9,260,&dsr_roboto_14,DSR_SC_TEXT);
  if(d.online&&(!d.has_sample_time||lv_tick_get()-d.sample_ms>15000))snprintf(text,sizeof(text),"%s",dsr_sc_tr("Odczyt starszy niż 15 s"));
  else if(d.online&&d.has_energy)snprintf(text,sizeof(text),"%.6g Wh • %s",d.energy_wh,dsr_sc_tr("licznik Shelly"));
  else snprintf(text,sizeof(text),"%s",dsr_sc_tr(d.online?"Brak obsługiwanych pomiarów":"Offline / odczyty niedostępne"));
  dsr_sc_text(row,text,10,36,260,&dsr_roboto_12,DSR_SC_MUTED);if(!d.id)lv_obj_add_state(row,LV_STATE_DISABLED);
 }
 if(!total)dsr_sc_text(list,"Brak urządzeń",12,24,260,&dsr_roboto_16,DSR_SC_MUTED);
 dsr_history_status_t status;dsr_history_status(&status);const char *message=failure(&status);
 dsr_sc_text(root,message?message:"24 h lokalnie • 5 min • oś czasu UTC",12,400,296,&dsr_roboto_11,DSR_SC_MUTED);clock_button(root,&status);
}
static void draw(void)
{
 if(!active)return;
 lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);lv_obj_set_style_bg_color(root,dsr_sc_color(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);lv_obj_scroll_to(root,0,0,LV_ANIM_OFF);
 if(overview)energy(root);else detail(root);lv_obj_update_layout(root);next_draw=lv_tick_get()+5000;
}
void dsr_ui_energy_open(void)
{dsr_network_home(NULL);dsr_mobile_hide();active=overview=true;draw();}
void dsr_ui_history_open(uint32_t id)
{dsr_network_home(NULL);dsr_mobile_hide();selected=id;hours=1;metric=0;active=true;overview=false;draw();}
void dsr_ui_history_tick(void)
{if(active&&(int32_t)(lv_tick_get()-next_draw)>=0)draw();}
