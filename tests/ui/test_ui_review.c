/* Real 320x480 LVGL rendering of the project components. This fixture is never
 * part of the ESP-IDF firmware. All populated review frames carry DEMO labels. */
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_config.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

static unsigned checks,commands,frames_saved,cover_commands,stop_commands,device_edits;
static unsigned edits,settings_opens,add_opens;
static bool photo_enabled;
static uint16_t photo_pixels[4][DSR_PHOTO_BYTES/2];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static unsigned char pixels[320*480*3];
static uint16_t buffer[320*40];
static lv_display_t *display;
static lv_indev_t *pointer;
static bool pressed;
static lv_point_t touch;
static const char *output;
static unsigned long flushed;
static dsr_mobile_model_t fixture={.demo=true,.home_name="Mój dom",.room_count=4,.device_count=5,
 .rooms={{.name="Salon",.icon=0},{.name="Kuchnia",.icon=1},{.name="Sypialnia",.icon=2},{.name="Łazienka",.icon=3}},
 .devices={
 {.name="Światło w salonie",.model="Shelly Plus 1PM",.room=0,.kind=DSR_DEVICE_LIGHT,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=18.6,.has_voltage=true,.voltage_v=230.1,.has_current=true,.current_a=.081},
 {.name="Drzwi wejściowe",.model="DEMO contact sensor",.room=0,.kind=DSR_DEVICE_CONTACT,.online=true,.has_contact=true,.contact_open=true,.has_battery=true,.battery_pct=92,.has_temperature=true,.temperature_c=22.4},
 {.name="Roleta w salonie",.model="Shelly Plus 2PM",.room=0,.kind=DSR_DEVICE_COVER,.online=true,.has_position=true,.position_pct=65,.can_cover=true},
 {.name="Gniazdko kuchnia",.model="Shelly Plug S",.room=1,.kind=DSR_DEVICE_PLUG,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=68.4},
 {.name="Oświetlenie lustra",.model="Shelly 1PM",.room=3,.kind=DSR_DEVICE_RELAY,.can_switch=true}}};

static void set_output(unsigned index,bool on){(void)on;CHECK(index<5);commands++;}
static void noop(void){}
/* Review fixture links dsr_ui_config.c without dsr_ui.c: LVGL theme is fixed here. */
void dsr_ui_apply_theme(void){}
/* The review display never sleeps (board backlight MODEL). */
bool dsr_board_display_sleeping(void){return false;}
static void edit_dashboard(void){edits++;}
static void open_settings(void){settings_opens++;}
static void add_device(void){add_opens++;}
static void set_cover(unsigned index,int direction){CHECK(index==2);CHECK(direction==1||direction==-1);cover_commands++;}
static void stop_cover(unsigned index){CHECK(index==2);stop_commands++;}
static void edit_device(uint32_t id){CHECK(id==97);device_edits++;}
const void *dsr_photo_pixels(const char *id){return photo_enabled&&id[0]>='1'&&id[0]<='4'?photo_pixels[id[0]-'1']:NULL;}
static void read_pointer(lv_indev_t *input,lv_indev_data_t *data)
{(void)input;data->point=touch;data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
static void tap(int x,int y)
{
 lv_obj_update_layout(lv_screen_active());touch=(lv_point_t){x,y};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);
 pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
}
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *data)
{
 const uint16_t *src=(const uint16_t *)data;flushed+=(unsigned long)(a->x2-a->x1+1)*(unsigned long)(a->y2-a->y1+1);
 for(int y=a->y1;y<=a->y2;y++)for(int x=a->x1;x<=a->x2;x++){
  uint16_t p=*src++;if(x<0||x>=320||y<0||y>=480)continue;unsigned at=(unsigned)(y*320+x)*3;
  pixels[at]=(unsigned char)(((p>>11)&31)*255/31);pixels[at+1]=(unsigned char)(((p>>5)&63)*255/63);pixels[at+2]=(unsigned char)((p&31)*255/31);
 }lv_display_flush_ready(d);
}
static lv_obj_t *find(lv_obj_t *o,const char *text)
{
 if(lv_obj_check_type(o,&lv_label_class)&&!strcmp(lv_label_get_text(o),text))return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *f=find(lv_obj_get_child(o,(int)i),text);if(f)return f;}return NULL;
}
static lv_obj_t *find_part(lv_obj_t *o,const char *text)
{
 if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o),text))return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *f=find_part(lv_obj_get_child(o,(int)i),text);if(f)return f;}return NULL;
}
static unsigned count_cards(lv_obj_t *o)
{
 /* Count the 24 populated fixture tiles, independently of their adaptive height. */
 unsigned count=lv_obj_check_type(o,&lv_label_class)&&!strcmp(lv_label_get_text(o),fixture.devices[0].name);
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)count+=count_cards(lv_obj_get_child(o,(int)i));
 return count;
}
static void save(const char *name)
{
 lv_obj_update_layout(lv_screen_active());lv_obj_invalidate(lv_screen_active());lv_refr_now(display);
 CHECK(lv_obj_get_scroll_x(lv_screen_active())==0&&lv_obj_get_scroll_y(lv_screen_active())==0);
 CHECK(find(lv_screen_active(),"DEMO")!=NULL||find(lv_screen_active(),dsr_sc_tr("DEMO • dane przykładowe"))!=NULL||!strcmp(name,"08-live-empty")||!strcmp(name,"09-icon-alignment"));
 char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",output,name);FILE *f=fopen(path,"wb");CHECK(f);
 fprintf(f,"P6\n320 480\n255\n");CHECK(fwrite(pixels,1,sizeof(pixels),f)==sizeof(pixels));fclose(f);frames_saved++;
}
static void check_icon_alignment(void)
{
 /* Check the actual RGB565 raster, not just object coordinates. Blue borders
  * are excluded from the white glyph's bounding box. Include the production
  * 40px / 2px-border and 86px / 3px-border circular power buttons. */
 static const int sizes[]={18,20,22,30,35,56};
 unsigned failures=0;
 for(unsigned kind=0;kind<=DSR_ICON_SCENES;kind++)for(unsigned at=0;at<sizeof(sizes)/sizeof(sizes[0]);at++){
  int size=sizes[at],side=size==22?40:size==35?86:size+18;
  for(int border=0;border<=3;border++){
   lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);
   lv_obj_set_style_bg_color(root,lv_color_hex(0),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
   lv_obj_t *circle=dsr_sc_panel(root,20,20,side,side,0,LV_RADIUS_CIRCLE);
   lv_obj_set_style_border_color(circle,lv_color_hex(0x0000ff),0);lv_obj_set_style_border_width(circle,border,0);
   lv_obj_t *icon=dsr_sc_icon_centered(circle,(dsr_ui_icon_t)kind,size,0xffffff);
   lv_obj_update_layout(root);lv_obj_invalidate(root);lv_refr_now(display);lv_area_t area,art;lv_obj_get_coords(circle,&area);lv_obj_get_coords(icon,&art);
   CHECK(abs((area.x1+area.x2)-(art.x1+art.x2))<=1);
   CHECK(abs((area.y1+area.y2)-(art.y1+art.y2))<=1);
   CHECK(!lv_obj_has_flag(icon,LV_OBJ_FLAG_CLICKABLE));
   int left=320,right=-1,top=480,bottom=-1;
   for(int y=area.y1;y<=area.y2;y++)for(int x=area.x1;x<=area.x2;x++){
    unsigned p=(unsigned)(y*320+x)*3;
    if(pixels[p]>64&&pixels[p+1]>64&&pixels[p+2]>64){
     if(x<left)left=x;
     if(x>right)right=x;
     if(y<top)top=y;
     if(y>bottom)bottom=y;
    }
   }
   CHECK(right>=left&&bottom>=top);
   int dx=(left+right)-(area.x1+area.x2),dy=(top+bottom)-(area.y1+area.y2);
   if(abs(dx)>2||abs(dy)>2){if(border==0)fprintf(stderr,"Icon %u size %d: pixel centre offset %.1f,%.1f\n",kind,size,dx/2.,dy/2.);failures++;}
  }
 }
 CHECK(failures==0);
}
static void save_icon_atlas(void)
{
 lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);
 lv_obj_set_style_bg_color(root,lv_color_hex(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
 dsr_sc_text(root,"IKONY / LVGL 320 x 480",12,10,296,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(root,"Material Symbols Rounded • Apache 2.0",12,33,296,&dsr_roboto_11,DSR_SC_MUTED);
 static const char *names[]={"Pulpit","Dom","Energia","Ustawienia","Dodaj","ON / OFF","Wstecz","Edytuj","Informacja","Wtyczka","Światło","Salon","Sypialnia","Kuchnia","Łazienka","Wi-Fi","Siatka","Rozwiń","Góra","Dół","Drzwi","Roleta","Bateria","Sceny"};
 for(unsigned i=0;i<=DSR_ICON_SCENES;i++){
  int x=-3+(int)(i%4)*80,y=58+(int)(i/4)*68;
  lv_obj_t *circle=dsr_sc_panel(root,x+18,y,48,48,DSR_SC_PANEL,LV_RADIUS_CIRCLE);
  lv_obj_set_style_border_color(circle,lv_color_hex(DSR_SC_BLUE),0);lv_obj_set_style_border_width(circle,2,0);
  dsr_sc_icon_centered(circle,(dsr_ui_icon_t)i,26,DSR_SC_TEXT);
  lv_obj_t *label=dsr_sc_text(root,names[i],x+1,y+49,78,&dsr_roboto_11,DSR_SC_MUTED);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
 }
 save("09-icon-alignment");
}
static void click_power(int size)
{
 lv_obj_update_layout(lv_screen_active());
 lv_obj_t *root=lv_screen_active();bool done=false;
 for(unsigned i=0;i<lv_obj_get_child_count(root)&&!done;i++){
  lv_obj_t *parent=lv_obj_get_child(root,(int)i);
  for(unsigned j=0;j<lv_obj_get_child_count(parent);j++){
   lv_obj_t *child=lv_obj_get_child(parent,(int)j);
   if(lv_obj_get_width(child)==size&&lv_obj_get_height(child)==size&&lv_obj_has_flag(child,LV_OBJ_FLAG_CLICKABLE)){
    (void)lv_obj_send_event(child,LV_EVENT_CLICKED,NULL);done=true;break;
   }
  }
 }CHECK(done);
}
static void navigation_and_chips(void)
{
 lv_obj_update_layout(lv_screen_active());lv_obj_t *bar=NULL;
 for(unsigned i=0;i<lv_obj_get_child_count(lv_screen_active());i++){lv_obj_t *o=lv_obj_get_child(lv_screen_active(),(int)i);if(lv_obj_get_width(o)==320&&lv_obj_get_height(o)==52)bar=o;}
 CHECK(bar);unsigned count=0;
 for(unsigned i=0;i<lv_obj_get_child_count(bar);i++){lv_obj_t *b=lv_obj_get_child(bar,(int)i);if(!lv_obj_has_flag(b,LV_OBJ_FLAG_CLICKABLE))continue;CHECK(lv_obj_get_width(b)==64);CHECK(lv_obj_get_height(b)>=44);CHECK(lv_obj_has_state(b,LV_STATE_DISABLED)==(count==2));lv_area_t area;lv_obj_get_coords(b,&area);CHECK(area.x1==(int)count*64&&area.x2==(int)(count+1)*64-1);count++;}CHECK(count==5);
 /* Shelly-style value capsule: one pill, one segment per value, separators between. */
 lv_obj_t *power=find_part(lv_screen_active(),"18.6 W");CHECK(power&&strstr(lv_label_get_text(power),"230.1 V"));lv_obj_t *chip=lv_obj_get_parent(power);
 CHECK(lv_obj_get_width(chip)<180);CHECK(lv_obj_get_width(chip)>=lv_obj_get_width(power)+12);
 lv_obj_t *position=find(lv_screen_active(),"Pozycja 65%");if(!position)position=find(lv_screen_active(),"Position 65%");CHECK(position);CHECK(lv_obj_get_width(lv_obj_get_parent(position))<170);
 unsigned before=edits;tap(236,72);CHECK(edits==before+1);
 before=settings_opens;tap(224,455);CHECK(settings_opens==before+1);before=add_opens;tap(288,455);CHECK(add_opens==before+1);
 tap(286,72);CHECK(find(lv_screen_active(),dsr_sc_tr("Informacje o pulpicie")));char counts[96];snprintf(counts,sizeof(counts),dsr_sc_tr("Urządzenia: %u • online: %u"),5u,4u);CHECK(find(lv_screen_active(),counts));
 dsr_mobile_model_t update=fixture;update.devices[4].online=true;dsr_mobile_update(&update);lv_obj_update_layout(lv_screen_active());CHECK(find(lv_screen_active(),dsr_sc_tr("Informacje o pulpicie")));snprintf(counts,sizeof(counts),dsr_sc_tr("Urządzenia: %u • online: %u"),5u,5u);CHECK(find(lv_screen_active(),counts));dsr_mobile_update(&fixture);
 tap(160,324);CHECK(!find(lv_screen_active(),dsr_sc_tr("Informacje o pulpicie")));
 tap(96,455);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);tap(32,455);CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
}
static lv_obj_t *parent_of(const char *text)
{lv_obj_t *label=find(lv_screen_active(),dsr_sc_tr(text));CHECK(label);return lv_obj_get_parent(label);}
static void tap_stop(void)
{lv_area_t a;lv_obj_t *button=parent_of("Stop");lv_obj_get_coords(button,&a);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);}
/* The cover frame also holds a passive divider: count only touch targets. */
static lv_obj_t *nth_clickable(lv_obj_t *parent,unsigned n)
{for(unsigned i=0;i<lv_obj_get_child_count(parent);i++){lv_obj_t *c=lv_obj_get_child(parent,(int)i);if(lv_obj_has_flag(c,LV_OBJ_FLAG_CLICKABLE)&&!n--)return c;}return NULL;}
static void cover_card_tap(bool open)
{
 lv_obj_t *card=parent_of("Roleta w salonie");lv_obj_scroll_to_view(card,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *control=nth_clickable(lv_obj_get_child(card,3),open?0:1);CHECK(control);CHECK(lv_obj_get_width(control)>=44&&lv_obj_get_height(control)>=44);
 lv_area_t a;lv_obj_get_coords(control,&a);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static unsigned outdoor_pixels(void)
{
 /* Neutral metal is grey; count the coloured outdoor photograph inside the
  * actual glass aperture, rather than requiring blue slats in Graphite. */
 /* Exclude the guide edges and the coloured handle at the closed sill. */
 unsigned n=0;for(int y=204;y<316;y++)for(int x=80;x<224;x++){
  unsigned p=(unsigned)(y*320+x)*3;int high=pixels[p],low=high;for(unsigned k=1;k<3;k++){if(pixels[p+k]>high)high=pixels[p+k];if(pixels[p+k]<low)low=pixels[p+k];}if(high-low>30)n++;
 }return n;
}
static void cover_views(void)
{
 dsr_mobile_model_t m=fixture;m.demo=true;dsr_mobile_device_t *d=&m.devices[2];
 *d=(dsr_mobile_device_t){.name="Roleta w salonie",.kind=DSR_DEVICE_COVER,.online=true,.room=0,.has_position=true,.position_pct=65,.has_power=true,.power_w=4.1,.has_voltage=true,.voltage_v=230.4,.has_frequency=true,.frequency_hz=50,.can_cover=true,.can_stop_cover=true,.cover_state=DSR_COVER_STOPPED};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,2);save("11-cover-details");
 CHECK(find(lv_screen_active(),"Pozycja 65%"));CHECK(find(lv_screen_active(),"4.1 W"));CHECK(find(lv_screen_active(),"230.4 V"));CHECK(find_part(lv_screen_active(),"50.0 Hz"));
 unsigned middle=outdoor_pixels();CHECK(middle>4000);
 lv_obj_t *open=parent_of("Otwórz"),*close=parent_of("Zamknij"),*stop=parent_of("Stop");lv_area_t a,b;
 CHECK(lv_obj_get_width(open)==144&&lv_obj_get_height(open)==48);CHECK(lv_obj_get_width(close)==144&&lv_obj_get_height(close)==48);CHECK(lv_obj_get_width(stop)>=44&&lv_obj_get_height(stop)>=44);
 lv_obj_get_coords(open,&a);lv_obj_get_coords(close,&b);CHECK(a.x2<b.x1);CHECK(a.y1==374&&a.y2<428);CHECK(b.x2<320);
 lv_obj_t *metrics=lv_obj_get_parent(find_part(lv_screen_active(),"50.0 Hz"));CHECK(lv_obj_get_width(metrics)==184);
 lv_obj_get_coords(find(lv_screen_active(),"4.1 W"),&a);lv_obj_get_coords(find(lv_screen_active(),"230.4 V"),&b);CHECK(a.x2<b.x1);
 d->power_w=123456;d->voltage_v=12345.6;d->frequency_hz=1234.5;dsr_mobile_update(&m);lv_obj_update_layout(lv_screen_active());lv_obj_t *long_metric=find_part(lv_screen_active(),"12345.6 V");CHECK(long_metric);lv_point_t text_size;lv_text_get_size(&text_size,lv_label_get_text(long_metric),lv_obj_get_style_text_font(long_metric,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);CHECK(lv_label_get_long_mode(long_metric)==(text_size.x>lv_obj_get_width(long_metric)?LV_LABEL_LONG_MODE_SCROLL_CIRCULAR:LV_LABEL_LONG_MODE_CLIP));CHECK(find_part(lv_screen_active(),"1234.5 Hz"));lv_obj_get_coords(long_metric,&a);CHECK(a.x2<=307);
 d->power_w=4.1;d->voltage_v=230.4;d->frequency_hz=50;dsr_mobile_update(&m);
 tap(75,377);CHECK(find(lv_screen_active(),"Otwarte 100%"));CHECK(cover_commands==0);
 tap(231,377);CHECK(find(lv_screen_active(),"Zamknięte 0%"));CHECK(cover_commands==0);
 tap_stop();CHECK(find(lv_screen_active(),"Zamknięte 0%"));CHECK(stop_commands==0);
 d->position_pct=0;dsr_mobile_update(&m);save("60-cover-closed");CHECK(outdoor_pixels()==0);CHECK(find(lv_screen_active(),"Zamknięte 0%"));
 d->position_pct=100;dsr_mobile_update(&m);save("61-cover-open");CHECK(outdoor_pixels()>middle);CHECK(find(lv_screen_active(),"Otwarte 100%"));
 d->position_pct=65;d->cover_state=DSR_COVER_OPENING;dsr_mobile_update(&m);save("62-cover-opening");CHECK(find_part(lv_screen_active(),"Otwieranie"));
 /* A background sample never overrides confirmed position/state with busy. */
 d->busy=true;dsr_mobile_update(&m);CHECK(find_part(lv_screen_active(),"Otwieranie"));CHECK(find(lv_screen_active(),"Pozycja 65%"));CHECK(!find(lv_screen_active(),"Odczyt stanu…"));d->busy=false;
 d->cover_state=DSR_COVER_CLOSING;dsr_mobile_update(&m);save("63-cover-closing");
 d->cover_state=DSR_COVER_STOPPED;d->has_position=false;dsr_mobile_update(&m);save("64-cover-position-unknown");CHECK(find(lv_screen_active(),"Nieznana pozycja"));CHECK(outdoor_pixels()==0);CHECK(!find(lv_screen_active(),"Pozycja 65%"));
 d->has_position=true;d->online=false;dsr_mobile_update(&m);save("65-cover-offline");CHECK(!find(lv_screen_active(),"230.4 V"));CHECK(outdoor_pixels()==0);CHECK(lv_obj_has_state(parent_of("Otwórz"),LV_STATE_DISABLED));
 tap(75,377);tap(231,377);tap_stop();CHECK(cover_commands==0&&stop_commands==0);
 d->online=true;d->error=DSR_BAD_RESPONSE;dsr_mobile_update(&m);save("66-cover-command-error");CHECK(find(lv_screen_active(),"Stan niepotwierdzony"));CHECK(outdoor_pixels()==0);
 d->error=DSR_OK;d->position_pct=NAN;dsr_mobile_update(&m);CHECK(find(lv_screen_active(),"Nieznana pozycja"));d->position_pct=101;dsr_mobile_update(&m);CHECK(find(lv_screen_active(),"Nieznana pozycja"));
 d->position_pct=65;d->command_pending=true;d->busy=true;dsr_mobile_update(&m);save("67-cover-command-pending");CHECK(find_part(lv_screen_active(),"Wysyłanie polecenia…"));
 d->command_pending=false;d->busy=false;d->has_frequency=false;d->has_voltage=false;d->has_power=false;dsr_mobile_update(&m);CHECK(!find(lv_screen_active(),"50.0 Hz"));CHECK(!find(lv_screen_active(),"230.4 V"));
 dsr_sc_appearance(DSR_SC_BLUE,false,1);dsr_mobile_show(DSR_MOBILE_DETAILS,2);save("68-cover-english");CHECK(find(lv_screen_active(),"Open"));CHECK(find(lv_screen_active(),"Close"));
 dsr_sc_appearance(DSR_SC_BLUE,false,2);dsr_mobile_show(DSR_MOBILE_DETAILS,2);save("69-cover-german");CHECK(find(lv_screen_active(),"Öffnen"));CHECK(find(lv_screen_active(),"Schließen"));dsr_sc_appearance(DSR_SC_BLUE,false,0);
 /* Non-demo transport requires an actual callback and explicit capabilities. */
 m.demo=false;d->id=97;d->can_cover=false;d->can_stop_cover=false;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,2);
 tap(75,377);tap(231,377);tap_stop();CHECK(cover_commands==0&&stop_commands==0);CHECK(find(lv_screen_active(),"Brak obsługi Cover RPC"));
 tap(258,74);CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS);lv_obj_t *edit=parent_of("Edytuj kartę");lv_obj_get_coords(edit,&a);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);CHECK(device_edits==1);dsr_mobile_show(DSR_MOBILE_DETAILS,2);
 d->can_cover=true;d->can_stop_cover=true;dsr_mobile_update(&m);tap(75,377);tap(231,377);tap_stop();CHECK(cover_commands==2&&stop_commands==1);CHECK(find(lv_screen_active(),"Pozycja 65%"));
 /* Preview cannot leak commands, edits, metrics or fake state into live model. */
 dsr_mobile_cover_preview();tap(75,377);tap(231,377);tap_stop();tap(294,22);CHECK(cover_commands==2&&stop_commands==1&&device_edits==1);
 strcpy(m.devices[2].name,"Updated cover");dsr_mobile_update(&m);CHECK(!find(lv_screen_active(),"Updated cover"));tap(20,20);dsr_mobile_show(DSR_MOBILE_DETAILS,2);lv_obj_update_layout(lv_screen_active());CHECK(find(lv_screen_active(),"Updated cover"));CHECK(!find(lv_screen_active(),"DEMO • dane przykładowe"));
 dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_update_layout(lv_screen_active());
 lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned i=0;i<80;i++){dsr_mobile_cover_preview();tap(75,377);tap(231,377);tap_stop();dsr_mobile_show(DSR_MOBILE_HOME,0);}
 lv_mem_monitor(&after);CHECK(after.free_size+1024>=before.free_size);printf("Cover heap before=%u after=%u largest=%u; 80 preview/control/exit cycles\n",(unsigned)before.free_size,(unsigned)after.free_size,(unsigned)after.free_biggest_size);
 dsr_mobile_update(&fixture);
}
static double now_ms(void){struct timespec t;timespec_get(&t,TIME_UTC);return (double)t.tv_sec*1e3+(double)t.tv_nsec/1e6;}
/* Host timing (relative only; ESP32-S3 numbers differ). A poll that changes one
 * device value must not rebuild or repaint the other 23 cards. */
static void perf_dashboard(void)
{
 setvbuf(stdout,NULL,_IONBF,0);
 dsr_mobile_model_t m=fixture;m.configured_layout=true;m.tile_count=24;
 for(unsigned i=0;i<24;i++)m.tiles[i]=(dsr_mobile_tile_t){.index=(uint8_t)(i%5),.span=2,.height=1,.fields=255};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_update_layout(lv_screen_active());
 lv_mem_monitor_t mem;lv_mem_monitor(&mem);printf("PERF 24 mixed tiles built: LVGL free %lu B largest %lu B\n",(unsigned long)mem.free_size,(unsigned long)mem.free_biggest_size);
 double t0=now_ms();lv_obj_invalidate(lv_screen_active());lv_refr_now(display);printf("PERF full repaint %.2f ms\n",now_ms()-t0);
 lv_obj_t *other=find(lv_screen_active(),"Drzwi wejściowe");CHECK(other);
 enum{N=40};double t=now_ms();flushed=0;
 for(unsigned i=0;i<N;i++){dsr_mobile_update(&m);lv_refr_now(display);}
 double same=(now_ms()-t)/N;unsigned long same_px=flushed/N;bool same_kept=find(lv_screen_active(),"Drzwi wejściowe")==other;
 t=now_ms();flushed=0;
 for(unsigned i=0;i<N;i++){m.devices[3].power_w=68.4+(double)(i%2);dsr_mobile_update(&m);lv_refr_now(display);}
 double hidden=(now_ms()-t)/N;unsigned long hidden_px=flushed/N;CHECK(hidden_px==0);CHECK(find_part(lv_screen_active(),"69.4 W"));
 t=now_ms();flushed=0;
 for(unsigned i=0;i<N;i++){m.devices[0].power_w=18.6+(double)(i%2);dsr_mobile_update(&m);lv_refr_now(display);}
 double value=(now_ms()-t)/N;unsigned long value_px=flushed/N;bool value_kept=find(lv_screen_active(),"Drzwi wejściowe")==other;
 CHECK(find_part(lv_screen_active(),"19.6 W"));CHECK(value_px>0&&value_px<20000);
 printf("PERF offscreen value: %.2f ms/%lu px; visible value keeps control objects\n",hidden,hidden_px);
 t=now_ms();flushed=0;
 for(unsigned i=0;i<N;i++){m.devices[3].power_w=68.4+(double)(i%2);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_refr_now(display);}
 double full=(now_ms()-t)/N;unsigned long full_px=flushed/N;
 printf("PERF 24-tile full rebuild (pre-0.7.50 poll path, host): %.2f ms/%lu px\n",full,full_px);
 /* Regressions: an unchanged poll touches nothing; a one-device change keeps
  * the other cards and repaints less than the screen, faster than a rebuild. */
 CHECK(same_kept&&same_px==0);CHECK(value_kept&&value_px<320ul*480ul&&value<full/2);
 lv_mem_monitor(&mem);CHECK(mem.free_size>8192);
 printf("PERF 24-tile poll (host): unchanged %.2f ms/%lu px kept=%d; one value %.2f ms/%lu px kept=%d\n",same,same_px,same_kept,value,value_px,value_kept);
}
int main(int argc,char **argv)
{
 CHECK(argc==2||argc==3);output=argv[1];lv_init();display=lv_display_create(320,480);
 lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 if(argc==3){
  FILE *f=fopen(argv[2],"rb");CHECK(f);char text[DSR_CONFIG_MAX_JSON+1];size_t length=fread(text,1,DSR_CONFIG_MAX_JSON,f);fclose(f);text[length]=0;
  dsr_mobile_model_t model={.demo=true};CHECK(dsr_mobile_configure(&model,text));dsr_mobile_update(&model);dsr_mobile_show(DSR_MOBILE_HOME,0);save("01-home");lv_deinit();return 0;
 }
 const char *files[]={"salon","kuchnia","sypialnia","lazienka"};
 for(unsigned i=0;i<4;i++){
  char path[512];snprintf(path,sizeof(path),"%s/%s.rgb",DSR_TEST_PHOTO_DIR,files[i]);FILE *f=fopen(path,"rb");CHECK(f);CHECK(fread(photo_pixels[i],1,DSR_PHOTO_BYTES,f)==DSR_PHOTO_BYTES);fclose(f);
  memset(fixture.rooms[i].photo,(int)('1'+i),32);fixture.rooms[i].photo[32]=0;
 }photo_enabled=true;
 check_icon_alignment();save_icon_atlas();
 const uint32_t letters[]={0x105,0x119,0x142,0x144,0x15b,0x17a,0x17c,0xf3,0xe4,0xf6,0xfc,0xdf,0xf00c};
 for(unsigned i=0;i<sizeof(letters)/sizeof(letters[0]);i++){lv_font_glyph_dsc_t glyph;CHECK(lv_font_get_glyph_dsc(&dsr_roboto_14,&glyph,letters[i],0));}
 const dsr_mobile_actions_t actions={.set_output=set_output,.open_settings=open_settings,.add_device=add_device,.edit_dashboard=edit_dashboard,.select_dashboard=noop,.set_cover=set_cover,.stop_cover=stop_cover,.edit_device=edit_device};dsr_mobile_bind(&actions);
 dsr_sc_appearance(DSR_SC_BLUE,false,0); /* review frames are the PL owner workspace; boot default is EN */
 dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_HOME,0);save("01-home");
 CHECK(find(lv_screen_active(),"Światło w salonie"));
 navigation_and_chips();
 cover_card_tap(true);CHECK(find(lv_screen_active(),"Pozycja 100%"));CHECK(commands==0);
 cover_card_tap(false);CHECK(find(lv_screen_active(),"Pozycja 0%"));CHECK(commands==0);
 dsr_mobile_update(&fixture);
 dsr_mobile_show(DSR_MOBILE_ROOMS,0);save("02-rooms");CHECK(find(lv_screen_active(),"Łazienka"));
 tap(84,176);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);save("04-room-salon");
 tap(84,166);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);save("05-device-details-on");
 CHECK(find(lv_screen_active(),"18.6 W"));CHECK(find(lv_screen_active(),"230.1 V"));CHECK(find(lv_screen_active(),"0.081 A"));
 tap(160,262);CHECK(commands==0);CHECK(find(lv_screen_active(),"Wyłączone • Online"));CHECK(find(lv_screen_active(),"0.000 A"));save("06-device-details-off");
 tap(20,20);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);
 tap(20,20);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 tap(32,455);CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
 dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_DEVICES,0);save("03-devices");
 fixture.devices[0].online=false;dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_DETAILS,0);save("07-device-offline");
 CHECK(find(lv_screen_active(),"Offline • stan niepotwierdzony"));CHECK(!find(lv_screen_active(),"18.6 W"));
 unsigned old=commands;click_power(86);CHECK(commands==old);
 fixture.devices[0].online=true;fixture.demo=false;dsr_mobile_update(&fixture);click_power(86);CHECK(commands==1);
 dsr_mobile_model_t empty={0};dsr_mobile_update(&empty);dsr_mobile_show(DSR_MOBILE_HOME,0);save("08-live-empty");
 CHECK(find(lv_screen_active(),"Brak urządzeń"));CHECK(!find(lv_screen_active(),"DEMO"));CHECK(!find(lv_screen_active(),"18.6 W"));
 lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned i=0;i<80;i++){fixture.demo=true;dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_ROOMS,0);dsr_mobile_show(DSR_MOBILE_DETAILS,0);dsr_mobile_update(&empty);dsr_mobile_show(DSR_MOBILE_HOME,0);}
 lv_mem_monitor(&after);CHECK(after.free_size+1024>=before.free_size);
 fixture.configured_layout=true;fixture.tile_count=24;for(unsigned i=0;i<24;i++)fixture.tiles[i]=(dsr_mobile_tile_t){.index=0,.span=2,.height=1,.fields=255};
 dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_update_layout(lv_screen_active());
 CHECK(count_cards(lv_screen_active())==24);lv_mem_monitor(&after);CHECK(after.free_size>2048);
 printf("24-tile dashboard: LVGL heap free %lu B, largest %lu B (128 KB pool, same as firmware)\n",(unsigned long)after.free_size,(unsigned long)after.free_biggest_size);
 perf_dashboard();
 fixture.configured_layout=true;fixture.dashboard_id=2;strcpy(fixture.dashboard_name,"Pomieszczenia");fixture.tile_count=4;
 for(unsigned i=0;i<4;i++)fixture.tiles[i]=(dsr_mobile_tile_t){.type=1,.index=i,.span=1,.height=1,.fields=255};
 dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_HOME,0);save("10-home-room-photos");CHECK(find(lv_screen_active(),"Salon"));
 cover_views();
 fixture.configured_layout=false;fixture.tile_count=0;fixture.dashboard_name[0]=0;fixture.dashboard_id=1;
 dsr_mobile_update(&fixture);dsr_sc_appearance(DSR_SC_BLUE,false,1);dsr_mobile_show(DSR_MOBILE_HOME,0);save("12-home-english");CHECK(find(lv_screen_active(),"My dashboard"));
 navigation_and_chips();
 dsr_sc_appearance(DSR_SC_BLUE,false,2);dsr_mobile_show(DSR_MOBILE_HOME,0);save("13-home-german");CHECK(find(lv_screen_active(),"Mein Dashboard"));
 navigation_and_chips();
 dsr_sc_appearance(DSR_SC_BLUE,false,0);
 dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_HOME,0);tap(286,72);save("14-dashboard-info");CHECK(find(lv_screen_active(),"Informacje o pulpicie"));tap(160,324);
 printf("PASS: %u review checks, %u real LVGL frames at 320x480, 576 raster-centering cases, real pointer hit-testing, 80 navigation cycles; demo never sends commands\n",checks,frames_saved);
 lv_deinit();return 0;
}

void dsr_network_migrated_layout(const dsr_device_t *device,uint32_t id){(void)device;(void)id;}
