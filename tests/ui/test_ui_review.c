/* Real 320x480 LVGL rendering of the project components. This fixture is never
 * part of the ESP-IDF firmware. All populated review frames carry DEMO labels. */
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,commands;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static unsigned char pixels[320*480*3];
static uint16_t buffer[320*40];
static lv_display_t *display;
static lv_indev_t *pointer;
static bool pressed;
static lv_point_t touch;
static const char *output;
static dsr_mobile_model_t fixture={.demo=true,.home_name="Mój dom",.room_count=4,.device_count=5,
 .rooms={{.name="Salon",.icon=0},{.name="Kuchnia",.icon=1},{.name="Sypialnia",.icon=2},{.name="Łazienka",.icon=3}},
 .devices={
 {.name="Światło w salonie",.model="Shelly Plus 1PM",.room=0,.kind=DSR_DEVICE_LIGHT,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=18.6,.has_voltage=true,.voltage_v=230.1,.has_current=true,.current_a=.081},
 {.name="Drzwi wejściowe",.model="Shelly BLU Door/Window",.room=0,.kind=DSR_DEVICE_CONTACT,.online=true,.has_contact=true,.contact_open=true,.has_battery=true,.battery_pct=92,.has_temperature=true,.temperature_c=22.4},
 {.name="Roleta w salonie",.model="Shelly Plus 2PM",.room=0,.kind=DSR_DEVICE_COVER,.online=true,.has_position=true,.position_pct=65,.can_cover=true},
 {.name="Gniazdko kuchnia",.model="Shelly Plug S",.room=1,.kind=DSR_DEVICE_PLUG,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=68.4},
 {.name="Oświetlenie lustra",.model="Shelly 1PM",.room=3,.kind=DSR_DEVICE_RELAY,.can_switch=true}}};

static void set_output(unsigned index,bool on){(void)on;CHECK(index<5);commands++;}
static void noop(void){}
const void *dsr_photo_pixels(const char *id){(void)id;return NULL;}
static void read_pointer(lv_indev_t *input,lv_indev_data_t *data)
{(void)input;data->point=touch;data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
static void tap(int x,int y)
{
 lv_obj_update_layout(lv_screen_active());touch=(lv_point_t){x,y};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);
 pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
}
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *data)
{
 const uint16_t *src=(const uint16_t *)data;
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
static void save(const char *name)
{
 lv_obj_update_layout(lv_screen_active());lv_refr_now(display);
 CHECK(find(lv_screen_active(),"DEMO")!=NULL||!strcmp(name,"08-live-empty")||!strcmp(name,"09-icon-alignment"));
 char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",output,name);FILE *f=fopen(path,"wb");CHECK(f);
 fprintf(f,"P6\n320 480\n255\n");CHECK(fwrite(pixels,1,sizeof(pixels),f)==sizeof(pixels));fclose(f);
}
static void check_icon_alignment(void)
{
 /* Check the actual RGB565 raster, not just object coordinates. Blue borders
  * are excluded from the white glyph's bounding box. Include the production
  * 40px / 2px-border and 86px / 3px-border circular power buttons. */
 static const int sizes[]={18,20,22,30,35,56};
 unsigned failures=0;
 for(unsigned kind=0;kind<=DSR_ICON_CHEVRON;kind++)for(unsigned at=0;at<sizeof(sizes)/sizeof(sizes[0]);at++){
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
 dsr_sc_text(root,"Wspólne centrowanie • oryginalne symbole",12,33,296,&dsr_roboto_11,DSR_SC_MUTED);
 static const char *names[]={"Pulpit","Dom","Energia","Ustawienia","Dodaj","ON / OFF","Wstecz","Edytuj","Informacja","Wtyczka","Światło","Salon","Sypialnia","Kuchnia","Łazienka","Wi-Fi","Siatka","Rozwiń"};
 for(unsigned i=0;i<=DSR_ICON_CHEVRON;i++){
  int x=15+(int)(i%3)*104,y=58+(int)(i/3)*68;
  lv_obj_t *circle=dsr_sc_panel(root,x+18,y,48,48,DSR_SC_PANEL,LV_RADIUS_CIRCLE);
  lv_obj_set_style_border_color(circle,lv_color_hex(DSR_SC_BLUE),0);lv_obj_set_style_border_width(circle,2,0);
  dsr_sc_icon_centered(circle,(dsr_ui_icon_t)i,26,DSR_SC_TEXT);
  lv_obj_t *label=dsr_sc_text(root,names[i],x-3,y+49,90,&dsr_roboto_11,DSR_SC_MUTED);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
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
 check_icon_alignment();save_icon_atlas();
 const dsr_mobile_actions_t actions={.set_output=set_output,.open_settings=noop,.add_device=noop};dsr_mobile_bind(&actions);
 dsr_mobile_update(&fixture);dsr_mobile_show(DSR_MOBILE_HOME,0);save("01-home");
 CHECK(find(lv_screen_active(),"Światło w salonie"));
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
 printf("PASS: %u review checks, 9 real LVGL frames at 320x480, 432 raster-centering cases, real pointer hit-testing, 80 navigation cycles; demo never sends commands\n",checks);
 lv_deinit();return 0;
}
