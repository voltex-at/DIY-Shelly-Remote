/* Real LVGL/owner; count hidden Dashboard transitions and retained widgets.
 * Transport, radio and hardware remain explicit host providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static unsigned home_builds,deleted;
static unsigned unchanged_draws;
void test_native_ota_progress(size_t received,size_t total);
static void drawn(lv_event_t *event){(void)event;unchanged_draws++;}
void __real_dsr_mobile_show(dsr_mobile_page_t page,unsigned index);
void __wrap_dsr_mobile_show(dsr_mobile_page_t page,unsigned index)
{if(page==DSR_MOBILE_HOME)home_builds++;__real_dsr_mobile_show(page,index);}
static void view_deleted(lv_event_t *event){(void)event;deleted++;}
int main(int argc,char **argv)
{
 CHECK(argc==3);output=argv[1];const char *mode=argv[2];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();cJSON *r=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Navigation relay\",\"kind\":\"relay\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0}]"));publish_document(r);
 boot_loaded_count=1;boot_loaded[0]=(dsr_connection_t){.layout_id=1,.device={.host="192.0.2.1",.device_id="screen-relay",.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_SWITCH}};
 dsr_ui_init();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=response.device.has_power=true;response.device.power_w=10;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(!in_flight);
 unsigned before=home_builds;
 if(!strcmp(mode,"usage"))dsr_ui_usage_open(1);
 else if(!strcmp(mode,"settings"))dsr_ui_device_settings_open(1);
 else if(!strcmp(mode,"blu"))dsr_ui_blu_open(NULL);
 else if(!strcmp(mode,"rooms")){dsr_ui_open_settings();lv_obj_update_layout(lv_screen_active());before=home_builds;tap(96,455);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);}
 else if(!strcmp(mode,"actions")){
  dsr_ui_actions_execute(1,9999,false);lv_obj_t *w=lv_obj_get_child(lv_screen_active(),0);CHECK(w);lv_obj_add_event_cb(w,view_deleted,LV_EVENT_DELETE,NULL);lv_tick_inc(1500);dsr_ui_actions_tick();CHECK(!deleted);
 }
 else if(!strcmp(mode,"usage-stable")){
  dsr_ui_usage_open(1);lv_obj_t *w=lv_obj_get_child(lv_screen_active(),0);CHECK(w);lv_obj_add_event_cb(w,view_deleted,LV_EVENT_DELETE,NULL);lv_tick_inc(6000);dsr_ui_usage_tick();CHECK(!deleted);
  touch=(lv_point_t){160,240};pressed=true;lv_indev_read(pointer);lv_tick_inc(10000);dsr_ui_usage_tick();CHECK(!deleted);pressed=false;lv_indev_read(pointer);dsr_ui_usage_tick();CHECK(deleted);
  before=home_builds;
 }else if(!strcmp(mode,"ota")){
  test_native_ota_set(DSR_NATIVE_INSTALLING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_progress(500,1000);dsr_ui_native_ota_open(NULL);lv_refr_now(display);
  lv_obj_t *percent=find_label(lv_screen_active(),"50%");CHECK(percent);lv_obj_add_event_cb(percent,drawn,LV_EVENT_DRAW_MAIN,NULL);dsr_ui_native_ota_tick();lv_refr_now(display);CHECK(!unchanged_draws);
  test_native_ota_progress(600,1000);dsr_ui_native_ota_tick();lv_refr_now(display);CHECK(unchanged_draws&&find_label(lv_screen_active(),"60%"));
 }else CHECK(!"unknown screen case");
 CHECK(home_builds==before);
 printf("PASS: screen work %s; no hidden Dashboard, unchanged/held content retained; transport MODEL\n",mode);lv_deinit();return 0;
}
