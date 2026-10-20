/* Real LVGL UI, asynchronous BLE/backend providers are explicit models. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_blu.h"
void test_blu_phase(dsr_blu_phase_t phase,bool active);
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=false;test_workspace_disable();dsr_ui_init();tick();
 for(unsigned theme=0;theme<4;theme++)for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,theme,lang);
  dsr_ble_device_t candidate={.address={2,2,3,4,5,6},.model_id=5,.has_model=true,.kind=DSR_BLE_SHELLY_BLU,.evidence=DSR_BLE_MFG,.has_flags=true,.flags=DSR_BLE_FLAG_PAIRING};
  dsr_ui_blu_open(&candidate);click("Skanuj i połącz");lv_tick_inc(25001);dsr_ui_blu_tick();
  CHECK(find_label(lv_screen_active(),"Nie potwierdzono operacji. Sprawdź zasięg i ponów odczyt."));
  dsr_ui_blu_hide();dsr_ui_blu_open(&candidate);click("Skanuj i połącz");
  const uint8_t packet[]={6,0xff,0xa9,0x0b,0x0b,5,0};test_ble_packet(packet,sizeof(packet),2);test_ble_finish(DSR_BLE_DONE);dsr_ui_blu_tick();
  test_blu_phase(DSR_BLU_CLOSING,true);dsr_ui_blu_tick();CHECK(find_label(lv_screen_active(),"Zamykanie połączenia BLE • do 4 s"));
  test_blu_phase(DSR_BLU_QUARANTINED,false);dsr_ui_blu_tick();CHECK(find_label(lv_screen_active(),"Połączenie BLE nie zakończyło się prawidłowo. Ponów po chwili lub uruchom pilot ponownie."));dsr_ui_blu_hide();
  restart_open();test_blu_phase(DSR_BLU_CLOSING,true);restart_response=DSR_RESTART_BUSY;unsigned before=restart_requests;
  click("Uruchom ponownie");CHECK(restart_requests==before+1&&!dsr_blu_active());
  CHECK(find_label(lv_screen_active(),"Kończenie Bluetooth przed restartem…")&&lv_obj_has_state(restart_control("Uruchom ponownie"),LV_STATE_DISABLED));
  if(lang)CHECK(strcmp(dsr_sc_tr("Kończenie Bluetooth przed restartem…"),"Kończenie Bluetooth przed restartem…"));
  /* Other work may still own admission: finite wait, no forced restart. */
  lv_tick_inc(10100);dsr_ui_settings_tick();CHECK(find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie.")&&!lv_obj_has_state(restart_control("Uruchom ponownie"),LV_STATE_DISABLED));
  test_blu_phase(DSR_BLU_CLOSING,true);click("Uruchom ponownie");before=restart_requests;click("Anuluj");
  lv_tick_inc(1000);dsr_ui_settings_tick();CHECK(restart_requests==before); /* no stale page callback */
 }
 dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,0);
 restart_open();test_rpc_update(DSR_BLE_RPC_CLOSING,DSR_BUSY,true,NULL);restart_response=DSR_RESTART_BUSY;
 unsigned cancels=test_rpc_cancels();click("Uruchom ponownie");CHECK(test_rpc_cancels()==cancels+1&&find_label(lv_screen_active(),"Kończenie Bluetooth przed restartem…"));
 test_rpc_update(DSR_BLE_RPC_QUARANTINED,DSR_IO,false,NULL);restart_response=DSR_RESTART_ACCEPTED;
 lv_tick_inc(100);dsr_ui_settings_tick();CHECK(find_label(lv_screen_active(),"Przyjęto żądanie restartu.")&&lv_obj_has_state(restart_control("Anuluj"),LV_STATE_DISABLED));
 unsigned before=restart_requests;lv_tick_inc(1000);dsr_ui_settings_tick();CHECK(restart_requests==before);
 printf("PASS: %u actual LVGL BLE/restart checks; 4 themes x 3 languages, timeout/cancel/one-click deferred restart; radio/backend models\n",checks);
 lv_deinit();return 0;
}
