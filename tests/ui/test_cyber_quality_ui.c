/* Genuine LVGL pixels and pointer dispatch. Network/ADC/storage are fixtures. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_cyber_icons.h"

static unsigned writes,last_target;
static void position(unsigned index,uint8_t target)
{CHECK(index==0&&target<=100);writes++;last_target=target;}
static void move(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_refr_now(display);}
static void snap(const char *name)
{char text[100];snprintf(text,sizeof(text),"native-quality-%s",name);screenshot(text);}
static bool has_preview(lv_obj_t *o)
{if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o),dsr_sc_tr("Podgląd")))return true;for(unsigned i=0;i<lv_obj_get_child_count(o);i++)if(has_preview(lv_obj_get_child(o,(int)i)))return true;return false;}
static dsr_mobile_model_t cover(void)
{
 dsr_mobile_model_t m={.wifi=true,.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=31,.name="Roleta taras",.identity="quality-cover",.host="192.0.2.20",.generation=DSR_GEN2_PLUS,.kind=DSR_DEVICE_COVER,.endpoint_kind=DSR_ENDPOINT_COVER,.online=true,.has_position=true,.can_position=true,.can_cover=true,.can_stop_cover=true,.cover_state=DSR_COVER_STOPPED,.position_pct=100,.has_voltage=true,.voltage_v=232.9,.has_power=true,.power_w=0};return m;
}
static void show(dsr_mobile_model_t *m)
{dsr_mobile_update(m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());lv_refr_now(display);}
static unsigned char garden[sizeof(canvas)];
static void scene_below(int percent)
{
 lv_obj_update_layout(lv_screen_active());lv_obj_invalidate(lv_screen_active());lv_refr_now(display);
 int edge=202+108*(100-percent)/100;
 /* The outdoor view stays fixed. Only the curtain and its bottom rail move. */
 for(int y=edge+5;y<308;y++)for(int x=88;x<220;x++)CHECK(!memcmp(&canvas[(y*320+x)*3],&garden[(y*320+x)*3],3));
 if(percent<90){int y=202+(edge-202)/2;CHECK(memcmp(&canvas[(y*320+150)*3],&garden[(y*320+150)*3],3));}
}
static void shutter(void)
{
 dsr_mobile_actions_t a={.set_position=position};dsr_mobile_bind(&a);dsr_mobile_model_t m=cover();show(&m);memcpy(garden,canvas,sizeof(garden));
 const unsigned states[]={0,25,50,84,100};
 for(unsigned i=0;i<5;i++){m.devices[0].position_pct=states[i];show(&m);scene_below((int)states[i]);char s[50];snprintf(s,sizeof(s),"cover-position-%03u",states[i]);snap(s);}
 m.devices[0].position_pct=50;show(&m);unsigned before=writes;
 move(150,262,true);CHECK(!writes);CHECK(find_label(lv_screen_active(),"Pozycja 50%"));
 for(int p=0;p<=100;p+=5){
  move(150,310-108*p/100,true);CHECK(writes==before);CHECK(find_label(lv_screen_active(),"Pozycja 50%"));scene_below(p);
  char s[50];snprintf(s,sizeof(s),"cover-drag-%03d",p);snap(s);
 }
 move(150,202,false);CHECK(writes==before+1&&last_target==100);CHECK(find_label(lv_screen_active(),"Pozycja 50%"));scene_below(50);
 /* Polling during a gesture retains the draft and updates the actual marker. */
 move(150,224,true);m.devices[0].position_pct=60;dsr_mobile_update(&m);lv_obj_update_layout(lv_screen_active());lv_refr_now(display);CHECK(find_label(lv_screen_active(),"Pozycja 60%"));CHECK(writes==before+1);snap("cover-drag-with-readback");
 move(150,224,false);CHECK(writes==before+2&&last_target==80);
 /* A drag outside, connection loss and device replacement never send a target. */
 move(150,260,true);move(20,260,true);move(20,260,false);CHECK(writes==before+2);
 move(150,260,true);CHECK(has_preview(lv_screen_active()));m.devices[0].online=false;m.devices[0].error=DSR_OFFLINE;dsr_mobile_update(&m);move(150,220,false);CHECK(writes==before+2&&!has_preview(lv_screen_active()));snap("cover-offline");CHECK(find_label(lv_screen_active(),"Ostatnia pozycja: 60%")&&find_label(lv_screen_active(),"Offline")&&!find_label(lv_screen_active(),"Nieznana pozycja"));
 m=cover();m.devices[0].position_pct=50;show(&m);move(150,260,true);strcpy(m.devices[0].identity,"replacement");dsr_mobile_update(&m);move(150,220,false);CHECK(writes==before+2);
 m=cover();m.devices[0].has_position=false;show(&m);move(150,260,true);move(150,220,false);CHECK(writes==before+2);snap("cover-unknown");
 /* Teardown deletes the retained animation state as well as every LVGL object. */
 m=cover();for(unsigned i=0;i<3;i++){show(&m);m.devices[0].position_pct=40;m.devices[0].cover_state=DSR_COVER_CLOSING;dsr_mobile_update(&m);dsr_mobile_hide();lv_obj_clean(lv_screen_active());}
 lv_refr_now(display);lv_mem_monitor_t start,end;lv_mem_monitor(&start);
 for(unsigned i=0;i<100;i++){m=cover();show(&m);m.devices[0].position_pct=40;m.devices[0].cover_state=DSR_COVER_CLOSING;dsr_mobile_update(&m);dsr_mobile_hide();lv_obj_clean(lv_screen_active());}
 lv_refr_now(display);lv_mem_monitor(&end);CHECK(end.free_size>=start.free_size);
 printf("Shutter animation teardown: %lu -> %lu bytes free, 100 cycles, host pool\n",(unsigned long)start.free_size,(unsigned long)end.free_size);
}
static void icons(void)
{
 const dsr_ui_icon_t roles[]={DSR_ICON_WINDOW_CLOSED,DSR_ICON_WINDOW_OPEN,DSR_ICON_DOOR_CLOSED,DSR_ICON_DOOR_OPEN};
 const char *names[]={"Zamknięte","Otwarte","Zamknięte","Otwarte"};
 lv_obj_t *root=dsr_ui_screen("Okna i drzwi",false);
 for(unsigned i=0;i<4;i++){
  int x=12+(int)(i%2)*152,y=90+(int)(i/2)*160;lv_obj_t *card=dsr_sc_panel(root,x,y,144,152,DSR_SC_CARD,10);
  dsr_sc_icon(card,roles[i],32,8,80,DSR_SC_GLOW);dsr_sc_icon(card,roles[i],60,92,24,DSR_SC_GLOW);
  lv_obj_t *label=dsr_sc_text(card,names[i],4,125,136,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);
 }snap("window-door-icons");
 const dsr_ui_icon_t semantic[]={DSR_ICON_VOLTAGE,DSR_ICON_CURRENT,DSR_ICON_WATTAGE,DSR_ICON_BATTERY,DSR_ICON_MEMORY,DSR_ICON_PSRAM,DSR_ICON_GRAPHICS,DSR_ICON_FIRMWARE,DSR_ICON_HARDWARE,DSR_ICON_DIAGNOSTICS,DSR_ICON_SCENES,DSR_ICON_RESTART,DSR_ICON_SETTINGS,DSR_ICON_REFRESH,DSR_ICON_ONLINE,DSR_ICON_OFFLINE};
 root=dsr_ui_screen("Ikony sekcji i statusów",false);
 for(unsigned i=0;i<16;i++){
  const lv_image_dsc_t *mask=dsr_ui_cyber_icon(semantic[i],24);CHECK(mask&&mask->header.cf==LV_COLOR_FORMAT_A8);
  lv_obj_t *card=dsr_sc_panel(root,12+(int)(i%4)*76,90+(int)(i/4)*76,68,66,DSR_SC_CARD,8);dsr_sc_icon(card,semantic[i],18,16,32,DSR_SC_GLOW);
 }snap("semantic-icons");
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_ui_apply_theme();
 shutter();icons();printf("PASS: %u Cyber quality checks; %u genuine LVGL frames; gesture previews never replace confirmed state\n",checks,screenshots);lv_deinit();return 0;
}
