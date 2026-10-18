/* "Zużycie" (Verbrauch): live power gauge of one device. Energy history,
 * its storage and the Energie overview were removed in 0.7.51 (owner). */
#include "dsr_ui_internal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define DSR_PI 3.14159265358979323846

static bool active;
static uint32_t selected,next_draw;

void dsr_ui_usage_hide(void){active=false;}
static void back(lv_event_t *event)
{(void)event;dsr_network_device_control(selected);}
static void settings(lv_event_t *event){(void)event;dsr_network_device_settings(selected);}
static lv_obj_t *button(lv_obj_t *parent,const char *text,int x,int y,int width,lv_event_cb_t cb,uintptr_t data,bool chosen)
{
 lv_obj_t *b=dsr_sc_button(parent,x,y,width,40,chosen?DSR_SC_BLUE:DSR_SC_CARD,8,cb,(void *)data);
 lv_obj_t *label=dsr_sc_text(b,text,2,12,width-4,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);return b;
}
static void usage_header(lv_obj_t *root,const char *title)
{
 dsr_sc_header(root,title,false,dsr_wifi_connected(),back,NULL);
 /* This header is rebuilt by explicit range changes. The pinned label DOTS
  * mode temporarily mutates its text during layout; clip preserves the exact
  * title and bounds while charts rebuild in an event callback. */
 lv_obj_t *caption=lv_obj_get_child(root,1);lv_label_set_long_mode(caption,LV_LABEL_LONG_MODE_CLIP);
}
/* "Zużycie": the Shelly Smart Control consumption gauge (owner reference
 * 2026-10-18) instead of the old line charts. Live power from the last
 * confirmed reading. */
/* lv_line keeps a pointer to its points: each line owns a heap copy freed with it. */
static void free_points(lv_event_t *event){lv_free(lv_event_get_user_data(event));}
static void gauge_line(lv_obj_t *root,const lv_point_precise_t *source,int width,uint32_t color,lv_opa_t opa)
{
 lv_point_precise_t *points=lv_malloc(2*sizeof(*points));if(!points)return;points[0]=source[0];points[1]=source[1];
 lv_obj_t *line=lv_line_create(root);lv_obj_add_event_cb(line,free_points,LV_EVENT_DELETE,points);lv_line_set_points(line,points,2);lv_obj_set_pos(line,0,0);lv_obj_set_size(line,320,480);
 lv_obj_set_style_line_width(line,width,0);lv_obj_set_style_line_color(line,dsr_sc_color(color),0);lv_obj_set_style_line_opa(line,opa,0);lv_obj_set_style_line_rounded(line,true,0);
 lv_obj_remove_flag(line,LV_OBJ_FLAG_CLICKABLE);
}
static void gauge_value(double watts,double scale,char *value,size_t vsize,char *unit,size_t usize,char *top,size_t tsize)
{
 bool kw=scale>1000.0;double shown=kw?watts/1000.0:watts;
 snprintf(value,vsize,kw?"%.2f":"%.1f",shown);snprintf(unit,usize,"%s",kw?"kW":"W");
 if(kw)snprintf(top,tsize,"%g",scale/1000.0);else snprintf(top,tsize,"%.0f",scale);
}
static void detail(lv_obj_t *root)
{
 dsr_mobile_device_t d={0};bool exists=dsr_network_typed_device(selected,&d);
 usage_header(root,exists?d.name:"Zużycie");
 button(root,"Sterowanie",0,44,109,back,0,false);button(root,"Zużycie",111,44,101,NULL,0,true);button(root,"Ustawienia",214,44,106,settings,0,false);
 lv_obj_t *card=dsr_sc_panel(root,12,96,296,300,0x2b2f33,18);lv_obj_set_style_border_width(card,1,0);lv_obj_set_style_border_color(card,dsr_sc_color(0x3a3f44),0);
 lv_obj_t *chip=dsr_sc_panel(card,88,12,120,32,0x1d2124,16);dsr_sc_icon(chip,DSR_ICON_ENERGY,10,6,20,0xe5322d);dsr_sc_text(chip,"Zużycie",36,8,80,&dsr_roboto_14,DSR_SC_TEXT);
 bool fresh=exists&&d.online&&d.has_power&&d.has_sample_time&&lv_tick_get()-d.sample_ms<=15000;
 if(!exists||!d.online||!d.has_power){
  const char *why=!exists?"Brak aktywnego połączenia":DSR_ENDPOINT_IS_SENSOR(d.endpoint_kind)?"Czujnik nie mierzy mocy":!d.online?"Urządzenie offline • brak odczytu mocy":"To urządzenie nie raportuje mocy";
  dsr_sc_icon(card,DSR_ICON_ENERGY,128,104,40,0x5a6066);lv_obj_t *t=dsr_sc_text(card,why,12,160,272,&dsr_roboto_16,DSR_SC_MUTED);lv_obj_set_style_text_align(t,LV_TEXT_ALIGN_CENTER,0);
  dsr_sc_text(root,"Zużycie pokazuje moc z liczników Shelly.",12,402,296,&dsr_roboto_12,DSR_SC_MUTED);return;
 }
 const double scales[]={500,1000,2000,3500,5000,10000,25000};double watts=d.power_w>0?d.power_w:0,scale=scales[0];
 for(unsigned i=0;i<sizeof(scales)/sizeof(scales[0]);i++){scale=scales[i];if(watts<=scale*0.95)break;}
 const int cx=160,cy=246,r=104;const uint32_t stops[]={0x6ee04a,0xf5e04a,0xf0a020,0xe5322d};
 dsr_sc_gradient_arc(root,cx,cy,r,14,135,270,stops,4,24,fresh);
 for(unsigned i=0;i<19;i++){ /* scale ticks, longer every fourth */
  double a=(135.0+270.0*i/18.0)*DSR_PI/180.0;int inner=r-(i%3==0?30:24),outer=r-17;
  const lv_point_precise_t tick[2]={{cx+inner*cos(a),cy+inner*sin(a)},{cx+outer*cos(a),cy+outer*sin(a)}};
  gauge_line(root,tick,2,0x8a9096,LV_OPA_60);
 }
 double fraction=watts/scale;if(fraction>1)fraction=1;double a=(135.0+270.0*fraction)*DSR_PI/180.0;
 const lv_point_precise_t needle[2]={{cx,cy},{cx+(r-34)*cos(a),cy+(r-34)*sin(a)}};
 if(fresh){gauge_line(root,needle,5,0xeceff1,LV_OPA_COVER);lv_obj_t *hub=dsr_sc_panel(root,cx-9,cy-9,18,18,0x17191c,LV_RADIUS_CIRCLE);lv_obj_set_style_border_width(hub,3,0);lv_obj_set_style_border_color(hub,dsr_sc_color(0xeceff1),0);}
 char value[24],unit[8],top[16];gauge_value(watts,scale,value,sizeof(value),unit,sizeof(unit),top,sizeof(top));
 dsr_sc_icon(root,DSR_ICON_ENERGY,cx-10,cy+14,20,DSR_SC_TEXT);
 lv_obj_t *v=dsr_sc_text(root,fresh?value:"—",cx-80,cy+36,160,&dsr_roboto_28,DSR_SC_TEXT);lv_obj_set_style_text_align(v,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_t *u=dsr_sc_text(root,unit,cx-40,cy+72,80,&dsr_roboto_16,DSR_SC_MUTED);lv_obj_set_style_text_align(u,LV_TEXT_ALIGN_CENTER,0);
 dsr_sc_text(root,"0",cx-r+6,cy+82,30,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_t *t=dsr_sc_text(root,top,cx+r-46,cy+82,40,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(t,LV_TEXT_ALIGN_RIGHT,0);
 char when[64];if(!d.has_sample_time)snprintf(when,sizeof(when),"%s",dsr_sc_tr("Ostatnia aktualizacja: brak"));
 else{unsigned age=(unsigned)((lv_tick_get()-d.sample_ms)/1000);if(age<3)snprintf(when,sizeof(when),"%s",dsr_sc_tr("Ostatnia aktualizacja: teraz"));else snprintf(when,sizeof(when),dsr_sc_tr("Ostatnia aktualizacja: %u s temu"),age);}
 lv_obj_t *w=dsr_sc_text(root,dsr_sc_user(when),12,366,296,&dsr_roboto_12,fresh?DSR_SC_TEXT:0xffc18c);lv_obj_set_style_text_align(w,LV_TEXT_ALIGN_CENTER,0);
 char counter[96];if(d.online&&d.has_energy)snprintf(counter,sizeof(counter),dsr_sc_tr("Licznik Shelly: %.3f kWh"),d.energy_wh/1000.0);else snprintf(counter,sizeof(counter),"%s",dsr_sc_tr(fresh?"Licznik energii niedostępny":"Brak świeżego odczytu mocy"));
 dsr_sc_text(root,dsr_sc_user(counter),12,402,296,&dsr_roboto_12,DSR_SC_MUTED);
}
static void draw(void)
{
 if(!active)return;
 lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);lv_obj_scroll_to(root,0,0,LV_ANIM_OFF);
 detail(root);lv_obj_update_layout(root);next_draw=lv_tick_get()+5000;
}
void dsr_ui_usage_open(uint32_t id)
{dsr_network_home(NULL);dsr_mobile_hide();selected=id;active=true;draw();}
void dsr_ui_usage_tick(void)
{if(active&&(int32_t)(lv_tick_get()-next_draw)>=0)draw();}
