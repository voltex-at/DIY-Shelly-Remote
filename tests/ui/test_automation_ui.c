/* Actual LVGL requested/readback state. SDK/workspace providers are MODELS. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static void capture_trv(unsigned index,uint8_t half){(void)index;(void)half;CHECK(false);}
int main(int argc,char **argv){
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");test_workspace_enable();dsr_ui_init();
 dsr_mobile_actions_t controls={.set_trv=capture_trv};dsr_mobile_bind(&controls);
 const unsigned themes[]={0,2,3};
 for(unsigned t=0;t<3;t++){
  dsr_sc_appearance(DSR_SC_BLUE,themes[t],1);dsr_ui_apply_theme();
  dsr_mobile_model_t m={.wifi=true,.device_count=1};
  m.devices[0]=(dsr_mobile_device_t){.id=3,.online=true,.has_trv_target=true,.trv_target_c=20,.has_temperature=true,.temperature_c=21.5,.endpoint_kind=DSR_ENDPOINT_BLU,.generation=2,.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET}};
  strcpy(m.devices[0].name,"Thermostat");strcpy(m.devices[0].host,"192.0.2.1");strcpy(m.devices[0].identity,"gateway_trv");
  dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);dsr_mobile_trv_request_state(3,44,1234,true);
  m.devices[0].busy=m.devices[0].command_pending=true;dsr_mobile_update(&m);lv_refr_now(display);
  CHECK(find_label(lv_screen_active(),"22.0 °C"));CHECK(m.devices[0].trv_target_c==20);
  char shot[64];snprintf(shot,sizeof(shot),"automation-trv-wait-%u",themes[t]);screenshot(shot);
  m.devices[0].busy=m.devices[0].command_pending=false;m.devices[0].error=DSR_IO;dsr_mobile_update(&m);
  CHECK(find_label(lv_screen_active(),"22.0 °C"));CHECK(find_label(lv_screen_active(),"Storage error"));snprintf(shot,sizeof(shot),"automation-trv-error-%u",themes[t]);screenshot(shot);
  dsr_mobile_trv_request_state(3,44,1233,false);CHECK(find_label(lv_screen_active(),"22.0 °C"));
  dsr_mobile_trv_request_state(3,44,1234,false);m.devices[0].error=DSR_OK;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"20.0 °C"));
 }
 printf("automation LCD draft: %u checks; actual LVGL, no control call\n",checks);lv_deinit();return 0;
}
