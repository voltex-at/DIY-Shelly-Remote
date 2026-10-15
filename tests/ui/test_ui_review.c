/* Real 320x480 LVGL rendering of the project components. This fixture is never
 * part of the ESP-IDF firmware. All populated review frames carry DEMO labels. */
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
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
 {.name="Lampa podłogowa",.model="Shelly 1 Mini",.room=0,.kind=DSR_DEVICE_LIGHT,.online=true,.can_switch=true,.has_power=true},
 {.name="Gniazdko kuchnia",.model="Shelly Plug S",.room=1,.kind=DSR_DEVICE_PLUG,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=68.4},
 {.name="Lampka nocna",.model="Shelly Plus 1",.room=2,.kind=DSR_DEVICE_LIGHT,.online=true,.can_switch=true},
 {.name="Oświetlenie lustra",.model="Shelly 1PM",.room=3,.kind=DSR_DEVICE_RELAY,.can_switch=true}}};

static void set_output(unsigned index,bool on){(void)on;CHECK(index<5);commands++;}
static void noop(void){}
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
 CHECK(find(lv_screen_active(),"DEMO")!=NULL||!strcmp(name,"08-live-empty"));
 char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",output,name);FILE *f=fopen(path,"wb");CHECK(f);
 fprintf(f,"P6\n320 480\n255\n");CHECK(fwrite(pixels,1,sizeof(pixels),f)==sizeof(pixels));fclose(f);
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
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);
 lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
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
 printf("PASS: %u review checks, 8 real LVGL frames at 320x480, real pointer hit-testing, 80 navigation cycles; demo never sends commands\n",checks);
 lv_deinit();return 0;
}
