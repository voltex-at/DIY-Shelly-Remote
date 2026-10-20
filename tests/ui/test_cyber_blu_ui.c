/* Real production LVGL; gateway/radio/storage are host fixtures. */
#define _POSIX_C_SOURCE 200809L
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_cyber.h"
#include "../../components/dsr_ui/dsr_ui_cyber_icons.h"
static unsigned refreshed,usage_hits;
static void usage_blu(uint32_t id){CHECK(id==31);usage_hits++;}
static unsigned color(lv_color_t c){return lv_color_to_u32(c)&0xffffff;}
static lv_obj_t *box(lv_obj_t *o,int w,int h)
{if(lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h)return o;for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *f=box(lv_obj_get_child(o,i),w,h);if(f)return f;}return NULL;}
static void contained(lv_obj_t *child,lv_obj_t *parent)
{lv_area_t a,b;lv_obj_get_coords(child,&a);lv_obj_get_coords(parent,&b);if(!(a.x1>=b.x1&&a.x2<=b.x2&&a.y1>=b.y1&&a.y2<=b.y2))fprintf(stderr,"containment child %d,%d..%d,%d parent %d,%d..%d,%d\n",a.x1,a.y1,a.x2,a.y2,b.x1,b.y1,b.x2,b.y2);CHECK(a.x1>=b.x1&&a.x2<=b.x2&&a.y1>=b.y1&&a.y2<=b.y2);}
static void refresh_blu(uint32_t id){CHECK(id==31);refreshed++;}
static void unused_action(void){}
static unsigned chosen_icon(dsr_ui_icon_t role)
{for(unsigned i=0;i<DSR_SC_DEVICE_ICONS;i++)if(dsr_sc_device_icons[i]==role)return i+1;CHECK(false);return 0;}
static dsr_mobile_model_t sensor_model(void)
{
 dsr_mobile_model_t m={.wifi=true,.device_count=1,.home_name="Mój dom",.dashboard_name="Mój pulpit"};
 dsr_mobile_device_t *d=&m.devices[0];
 *d=(dsr_mobile_device_t){.id=31,.name="BLU-Sensor 2",.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.can_refresh=true,.has_contact=true,.has_battery=true,.battery_pct=100,.has_illuminance=true,.illuminance_lux=4,.blu_age_s=14,.has_sample_time=true,.sample_ms=lv_tick_get()-1000,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|DSR_BLU_HAS_RSSI,.rssi=-58}};
 strcpy(d->identity,"shellyplus1pm-aabbccddeeff_3c2ef571d52a");return m;
}
static void show_sensor(dsr_mobile_model_t *m){dsr_mobile_update(m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());}
static void states(unsigned skin,unsigned lang)
{
 dsr_sc_appearance(DSR_SC_BLUE,skin,lang);dsr_ui_apply_theme();
 dsr_mobile_model_t m=sensor_model();char name[96];
 const char *names[]={"window-closed","window-open","window-no-data","door-closed","door-open","door-no-data","gateway-offline","gateway-error","waiting","disabled","long-name"};
 for(unsigned state=0;state<11;state++){
  m=sensor_model();dsr_mobile_device_t *d=&m.devices[0];
  if(state==1||state==4){d->blu.flags|=DSR_BLU_OPEN;d->contact_open=true;}
  if(state>=3&&state<=5)d->icon=(uint8_t)chosen_icon(DSR_ICON_DOOR);
  if(state==2||state==5){d->blu.flags=0;d->has_contact=false;}
  if(state==6){d->online=false;d->error=DSR_OFFLINE;}
  if(state==7)d->blu.errors=DSR_BLU_ERROR_DECRYPT;
  if(state==8)d->busy=true;
  if(state==9)d->can_refresh=false;
  if(state==10)snprintf(d->name,sizeof(d->name),"Łazienka — bardzo długa nazwa urządzenia");
  show_sensor(&m);
  snprintf(name,sizeof(name),"%03u-blu-%s-%s",700+skin*40+lang*12+state,names[state],lang==0?"pl":lang==1?"en":"de");screenshot(name);
  if(skin==3){
   const char *status=state==1||state==4?"Otwarte":state==2||state==5||state==6||state==7?"Brak danych":"Zamknięte";
   lv_obj_t *label=find_label(lv_screen_active(),status);CHECK(label&&lv_obj_get_style_text_font(label,0)==&dsr_roboto_28);
   CHECK(color(lv_obj_get_style_text_color(label,0))==(state==1||state==4?0xffb52e:state==2||state==5||state==6||state==7?0x8295ab:0x00cfff));
   contained(label,lv_obj_get_parent(label));
   lv_obj_t *scroll=box(lv_screen_active(),304,334);CHECK(scroll&&lv_obj_has_flag(scroll,LV_OBJ_FLAG_SCROLLABLE));
   lv_obj_t *button=lv_obj_get_parent(find_label(lv_screen_active(),state==8?"Odczyt stanu…":"Odśwież status"));
   CHECK(lv_obj_get_width(button)==304&&lv_obj_get_height(button)==48&&lv_obj_get_parent(button)==scroll);
   CHECK(lv_obj_has_state(button,LV_STATE_DISABLED)==(state==8||state==9));
   lv_area_t nav;lv_obj_t *bar=box(lv_screen_active(),320,52);CHECK(bar);lv_obj_get_coords(bar,&nav);CHECK(nav.y1==428&&nav.y2==479);
   lv_area_t r;lv_obj_get_coords(scroll,&r);CHECK(r.y1==90&&r.y2==423);
   if(state!=7){contained(button,scroll);}
   CHECK(find_label(lv_screen_active(),"Shelly BLU • przez bramkę"));
   if(state==0){CHECK(find_label(lv_screen_active(),"4 lx"));CHECK(find_label(lv_screen_active(),"-58 dBm"));char expected[96];snprintf(expected,sizeof(expected),dsr_sc_tr("Ostatni sygnał czujnika %u s temu"),14u);CHECK(find_label(lv_screen_active(),expected));snprintf(expected,sizeof(expected),dsr_sc_tr("Odpowiedź odebrana %u s temu"),1u);CHECK(find_label(lv_screen_active(),expected));}
   if(state==2||state==5||state==6||state==7)CHECK(!find_label(lv_screen_active(),"Zamknięte"));
   char raw[112];snprintf(raw,sizeof(raw),"native-%s",name);screenshot(raw);
  }
 }
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 const dsr_mobile_actions_t a={.refresh=refresh_blu,.open_settings=unused_action,.open_battery=unused_action,.add_device=unused_action};dsr_mobile_bind(&a);
 if(getenv("DSR_TEST_BLU_MEMORY")){
  dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_ui_apply_theme();dsr_mobile_model_t sample=sensor_model();
  for(unsigned i=0;i<8;i++){show_sensor(&sample);dsr_mobile_show(DSR_MOBILE_HOME,0);}show_sensor(&sample);lv_refr_now(display);
  lv_mem_monitor_t mem;lv_mem_monitor(&mem);printf("BLU_LIVE_FREE=%lu BLU_LARGEST=%lu\n",(unsigned long)mem.free_size,(unsigned long)mem.free_biggest_size);
  for(unsigned i=0;i<5;i++){
   sample=sensor_model();if(i==1||i==4){sample.devices[0].blu.flags|=DSR_BLU_OPEN;sample.devices[0].contact_open=true;}
   if(i==2){sample.devices[0].blu.flags=0;sample.devices[0].has_contact=false;}
   if(i>=3)sample.devices[0].icon=(uint8_t)chosen_icon(DSR_ICON_DOOR);
   show_sensor(&sample);char name[64];snprintf(name,sizeof(name),"native-baseline-blu-%u",i);screenshot(name);
  }
  dsr_mobile_hide();lv_deinit();return 0;
 }
 for(unsigned skin=0;skin<4;skin++)for(unsigned lang=0;lang<3;lang++)states(skin,lang);
 dsr_mobile_actions_t interactive=a;interactive.open_usage=usage_blu;dsr_mobile_bind(&interactive);
 /* Pointer refresh: only one callback; disabled states never submit. */
 dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_ui_apply_theme();dsr_mobile_model_t m=sensor_model();show_sensor(&m);
 lv_obj_t *primary=lv_obj_get_parent(find_label(lv_screen_active(),"Odśwież status"));unsigned normal=color(lv_obj_get_style_bg_color(primary,0));
 touch=(lv_point_t){160,400};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);CHECK(lv_obj_has_state(primary,LV_STATE_PRESSED));CHECK(color(lv_obj_get_style_bg_color(primary,0))!=normal);screenshot("native-861-blu-refresh-pressed");
 pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();CHECK(refreshed==1);
 m.devices[0].busy=true;show_sensor(&m);tap(160,400);CHECK(refreshed==1);
 m.devices[0].busy=false;m.devices[0].can_refresh=false;show_sensor(&m);tap(160,400);CHECK(refreshed==1);
 m=sensor_model();m.devices[0].has_temperature=m.devices[0].has_humidity=true;m.devices[0].temperature_c=21.5;m.devices[0].humidity_pct=45;show_sensor(&m);
 lv_obj_t *scroll=box(lv_screen_active(),304,334);CHECK(scroll);lv_obj_scroll_to_y(scroll,100,LV_ANIM_OFF);lv_obj_update_layout(scroll);CHECK(lv_obj_get_scroll_y(scroll)>0);
 lv_obj_scroll_to_y(scroll,0,LV_ANIM_OFF);
 touch=(lv_point_t){136,300};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);
 for(int i=1;i<=10;i++){touch.y=300-i*10;lv_tick_inc(20);lv_indev_read(pointer);}
 pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_obj_update_layout(scroll);CHECK(lv_obj_get_scroll_y(scroll)>0);screenshot("native-862-blu-content-scrolled");
 int saved=lv_obj_get_scroll_y(scroll);m.devices[0].battery_pct=91;dsr_mobile_update(&m);scroll=box(lv_screen_active(),304,334);CHECK(scroll&&lv_obj_get_scroll_y(scroll)>0);CHECK(saved>0);
 /* Help stays a local read-only modal during asynchronous snapshots. */
 m=sensor_model();show_sensor(&m);tap(300,20);CHECK(find_label(lv_screen_active(),"Informacje o czujniku"));m.devices[0].battery_pct=91;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Informacje o czujniku"));screenshot("native-863-blu-help");click("Zamknij");CHECK(!find_label(lv_screen_active(),"Informacje o czujniku"));
 /* Removing the selected device during Help must not reinterpret its Close
  * button as a dashboard counter label. Closing returns to the current home. */
 m=sensor_model();show_sensor(&m);tap(300,20);m.device_count=0;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Informacje o czujniku"));click("Zamknij");CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
 m=sensor_model();show_sensor(&m);CHECK(!find_label(lv_screen_active(),"Zużycie"));tap(266,64);CHECK(usage_hits==0&&dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS);tap(50,64);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);
 /* Warm caches, then compare the same empty root after 80 open/close cycles. */
 for(unsigned i=0;i<8;i++){show_sensor(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);}
 dsr_mobile_hide();lv_obj_clean(lv_screen_active());lv_refr_now(display);lv_mem_monitor_t a_mem,b_mem;lv_mem_monitor(&a_mem);
 for(unsigned i=0;i<80;i++){show_sensor(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);}
 dsr_mobile_hide();lv_obj_clean(lv_screen_active());lv_refr_now(display);lv_mem_monitor(&b_mem);CHECK(b_mem.free_size>=a_mem.free_size);CHECK(b_mem.free_biggest_size>32000);
 printf("BLU layout: 80 open/close cycles, LVGL free %lu -> %lu, largest %lu; host only\n",(unsigned long)a_mem.free_size,(unsigned long)b_mem.free_size,(unsigned long)b_mem.free_biggest_size);
 /* Native 80px masks, including tilt artwork not inferred by telemetry. */
 dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_ui_apply_theme();
 lv_obj_t *root=dsr_ui_screen("BLU: A8 / LVGL 9.3",false);
 const dsr_ui_icon_t roles[]={DSR_ICON_WINDOW_CLOSED,DSR_ICON_WINDOW_OPEN,DSR_ICON_WINDOW_NO_DATA,DSR_ICON_DOOR_CLOSED,DSR_ICON_DOOR_OPEN,DSR_ICON_DOOR_NO_DATA,DSR_ICON_WINDOW_TILTED};
 const char *names[]={"window-closed","window-open","window-no-data","door-closed","door-open","door-no-data","window-tilted"};
 for(unsigned i=0;i<7;i++){
  CHECK(dsr_ui_cyber_icon(roles[i],80)->header.w==80);CHECK(dsr_ui_cyber_icon(roles[i],24)->header.w==24);
  int x=8+(int)(i%3)*104,y=65+(int)(i/3)*105;
  dsr_sc_icon(root,roles[i],x+10,y,80,i==1||i==4||i==6?0xffb52e:i==2||i==5?0x8295ab:0x00cfff);
  dsr_sc_text(root,names[i],x,y+82,100,&dsr_roboto_12,DSR_SC_TEXT);
 }
 screenshot("native-858-blu-icons");
 printf("PASS: %u BLU layout checks; %u real LVGL frames; host gateway/radio/storage MODEL\n",checks,screenshots);
 lv_deinit();return 0;
}
