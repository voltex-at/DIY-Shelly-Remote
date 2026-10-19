/* Real production cards/LVGL/mailbox; injected samples, no physical I2C. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_board/dsr_touch_mailbox.h"
#include "../../components/dsr_board/dsr_touch_filter.h"
#include "../../components/dsr_board/dsr_touch_lvgl.h"

static dsr_touch_mailbox_t mailbox;
static dsr_touch_filter_t filter;
static dsr_touch_lvgl_t input_state;
static unsigned hits;
static unsigned drag_releases,drag_losses;
void test_native_ota_install_detail(dsr_ota_install_result_t result,int sdk,int io);
static void input_read(lv_indev_t *input,lv_indev_data_t *data)
{
 dsr_touch_sample_t s=dsr_touch_pop(&mailbox,lv_tick_get());
 dsr_touch_lvgl_sample(input,data,&input_state,s,mailbox.count!=0);
}
static void raw(bool down,int x,int y,unsigned elapsed)
{
 lv_tick_inc(elapsed);dsr_touch_sample_t s;
 if(dsr_touch_filter(&filter,(dsr_touch_sample_t){.pressed=down,.x=(uint16_t)x,.y=(uint16_t)y},true,lv_tick_get(),&s))dsr_touch_push(&mailbox,s,lv_tick_get());
}
static void read_now(void){lv_indev_read(pointer);lv_obj_update_layout(lv_screen_active());}
static void release(bool read)
{for(unsigned i=0;i<4;i++){raw(false,0,0,10);if(read)read_now();}}
static void reset_input(void)
{lv_indev_reset(pointer,NULL);mailbox=(dsr_touch_mailbox_t){0};filter=(dsr_touch_filter_t){0};input_state=(dsr_touch_lvgl_t){0};read_now();hits=0;}
static void hit(lv_event_t *event){(void)event;hits++;}
static void button(unsigned theme)
{
 reset_input();dsr_mobile_hide();dsr_sc_appearance(DSR_SC_BLUE,theme,0);lv_obj_t *root=lv_screen_active();
 lv_obj_clean(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 dsr_sc_button(root,50,160,200,80,DSR_SC_CARD,14,hit,NULL);lv_obj_update_layout(root);
}
static lv_obj_t *list(unsigned theme,int *x,int *y)
{
 reset_input();dsr_mobile_hide();dsr_sc_appearance(DSR_SC_BLUE,theme,0);dsr_mobile_model_t m={.device_count=6};
 for(unsigned i=0;i<m.device_count;i++){
  m.devices[i]=(dsr_mobile_device_t){.id=i+1,.kind=DSR_DEVICE_RELAY,.online=true};
  snprintf(m.devices[i].name,sizeof(m.devices[i].name),"Touch relay %u",i);
  snprintf(m.devices[i].identity,sizeof(m.devices[i].identity),"touch-fixture-%u",i);
 }
 dsr_mobile_bind(NULL);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DEVICES,0);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *label=find_label(lv_screen_active(),"Touch relay 0");CHECK(label);
 lv_obj_t *card=lv_obj_get_parent(label),*parent=lv_obj_get_parent(card);lv_area_t a;lv_obj_get_coords(card,&a);
 *x=a.x1+110;*y=a.y1+60;CHECK(lv_obj_get_scroll_bottom(parent)>0);return parent;
}
static void fault(unsigned elapsed)
{
 lv_tick_inc(elapsed);dsr_touch_sample_t s;
 if(dsr_touch_filter(&filter,(dsr_touch_sample_t){0},false,lv_tick_get(),&s))dsr_touch_push(&mailbox,s,lv_tick_get());
 read_now();
}
static void drag_event(lv_event_t *event)
{
 if(lv_event_get_code(event)==LV_EVENT_RELEASED)drag_releases++;
 if(lv_event_get_code(event)==LV_EVENT_PRESS_LOST)drag_losses++;
}
static void controls(unsigned theme)
{
 button(theme);lv_obj_clean(lv_screen_active());drag_releases=drag_losses=0;
 lv_obj_t *slider=lv_slider_create(lv_screen_active());lv_obj_set_pos(slider,50,180);lv_obj_set_size(slider,200,30);lv_slider_set_range(slider,0,100);
 lv_obj_add_event_cb(slider,drag_event,LV_EVENT_ALL,NULL);lv_obj_update_layout(lv_screen_active());
 raw(true,70,195,10);read_now();raw(true,210,195,10);read_now();release(true);
 CHECK(drag_releases==1&&!drag_losses&&lv_slider_get_value(slider)>50);
 reset_input();drag_releases=drag_losses=0;raw(true,70,195,10);read_now();fault(50);CHECK(!drag_losses&&!drag_releases);fault(50);
 CHECK(drag_losses==1&&!drag_releases);raw(true,210,195,10);read_now();release(true);CHECK(!drag_releases);
 raw(true,90,195,10);read_now();release(true);CHECK(drag_releases==1); /* clean rearm */
 button(theme);lv_obj_clean(lv_screen_active());drag_releases=drag_losses=0;
 lv_obj_t *arc=lv_arc_create(lv_screen_active());lv_obj_set_pos(arc,70,130);lv_obj_set_size(arc,180,180);lv_arc_set_range(arc,0,100);lv_arc_set_value(arc,20);lv_arc_set_change_rate(arc,720);
 lv_obj_add_event_cb(arc,drag_event,LV_EVENT_ALL,NULL);lv_obj_update_layout(lv_screen_active());
 raw(true,160,138,10);read_now();raw(true,238,220,100);read_now();release(true);
 if(drag_releases!=1||drag_losses||lv_arc_get_value(arc)<=50)fprintf(stderr,"arc: releases=%u losses=%u value=%d\n",drag_releases,drag_losses,lv_arc_get_value(arc));
 CHECK(drag_releases==1&&!drag_losses&&lv_arc_get_value(arc)>50);
}
static void ota_diagnostics(void)
{
 reset_input();
 for(unsigned theme=0;theme<4;theme++)for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,theme,language);
  for(unsigned sdk=0;sdk<2;sdk++){
   test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_INSTALL,DSR_MANIFEST_OK);
   test_native_ota_detail(DSR_OTA_DEV,DSR_FETCH_IO,200);
   test_native_ota_install_detail(sdk?DSR_INSTALL_SDK:DSR_INSTALL_FETCH,sdk?ESP_ERR_NO_MEM:0,-0x7007);
   dsr_ui_native_ota_open(NULL);lv_obj_update_layout(lv_screen_active());
   CHECK(find_label(lv_screen_active(),sdk?"Nie udało się zapisać firmware. Bieżąca wersja pozostaje aktywna.":"Przerwano pobieranie. Sprawdź Wi-Fi i spróbuj ponownie."));
   lv_obj_t *channel=lv_obj_get_parent(find_label(lv_screen_active(),"Zmień kanał"));lv_area_t area,viewport;lv_obj_get_coords(channel,&area);lv_obj_get_coords(lv_obj_get_parent(channel),&viewport);
   CHECK(area.y1>=viewport.y1&&area.y2<viewport.y2);
   if(theme==3&&language==0&&!sdk)screenshot("870-ota-download-diagnostic");
  }
 }
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,input_read);
 lv_timer_set_period(lv_indev_get_read_timer(pointer),10);
 lv_indev_add_event_cb(pointer,dsr_touch_lvgl_guard,LV_EVENT_ALL,&input_state);
 lv_indev_set_scroll_limit(pointer,DSR_TOUCH_DRAG_LIMIT);
 for(unsigned theme=0;theme<4;theme++){
  button(theme);raw(true,130,190,10);read_now();release(true);CHECK(hits==1);
  button(theme);raw(true,130,190,10);read_now();
  for(unsigned i=0;i<120;i++){raw(true,130,190,10);read_now();}
  release(true);CHECK(hits==1);
  /* A single missing controller report during one hold is not a second tap. */
  button(theme);raw(true,130,190,10);read_now();raw(true,130,190,40);read_now();
  raw(false,0,0,10);read_now();raw(true,130,190,10);read_now();raw(true,130,190,40);read_now();
  release(true);CHECK(hits==1);
  /* Intentional second taps remain possible; there is no global cooldown. */
  raw(true,130,190,80);read_now();release(true);CHECK(hits==2);
  /* Native button on a non-scrolling page also rejects an out-and-back drag. */
  button(theme);raw(true,130,190,10);read_now();raw(true,170,190,10);raw(true,130,190,10);release(false);read_now();CHECK(hits==0);
  int x,y;lv_obj_t *parent=list(theme,&x,&y);raw(true,x,y,10);read_now();
  for(unsigned i=1;i<=8;i++){raw(true,x,y-(int)i*10,10);read_now();}
  release(true);CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICES&&lv_obj_get_scroll_y(parent)>0);
  list(theme,&x,&y);raw(true,x,y,10);read_now();
  for(unsigned i=1;i<=8;i++)raw(true,x,y-(int)i*10,10);
  release(false);read_now();CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICES);
  list(theme,&x,&y);raw(true,x,y,10);
  for(unsigned i=1;i<=8;i++)raw(true,x,y-(int)i*10,10);
  release(false);read_now();CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICES);
  /* A horizontal swipe cannot open a card on the vertical device list. */
  list(theme,&x,&y);raw(true,x,y,10);read_now();raw(true,x+45,y,10);read_now();release(true);CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICES);
  controls(theme);
  /* Stale taps and a queue overflow must cancel, never commit a button. */
  button(theme);raw(true,130,190,10);release(false);lv_tick_inc(510);read_now();CHECK(hits==0);
  button(theme);for(unsigned i=0;i<8;i++){raw(true,130,190,10);release(false);}read_now();CHECK(hits==0);
 }
 ota_diagnostics();
 CHECK(requests==0&&job_counts[DSR_JOB_SET_OUTPUT]==0&&job_counts[DSR_JOB_COVER]==0);
 printf("PASS: %u genuine touch checks, four themes, raw dropout and buffered swipe; I2C/SDK MODEL\n",checks);
 lv_deinit();return 0;
}
