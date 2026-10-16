/* Actual LVGL + UI/fleet/ACK lifecycle, only radio/worker/storage providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();CHECK(!in_flight);
 dsr_network_device(NULL);choose_method("Dodaj ręcznie (IP)");
 lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"192.168.1.80");
 click("Check & save device");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND);
 dsr_job_t old=submitted;
 click("Anuluj");CHECK(dsr_ui_onboarding_active());
 unsigned scans=test_ble_scans();choose_method("Bluetooth LE");
 bool started=dsr_ui_ble_active();
 CHECK(test_ble_scans()==scans+(started?1u:0u));
 response=(dsr_event_t){.device=old.connection.device};
 strcpy(response.device.device_id,"shellyplus1-aabbccddeeff");strcpy(response.device.name,"Old binding");
 response.device.generation=DSR_GEN2_PLUS;response.device.online=response.device.has_switch=true;
 test_workspace_defer(true);hold_maintenance=true;
 deliver(DSR_OK,DSR_WIFI_IDLE);
 /* Both durable stores remain part of the old owner. The gap between RAM
  * verification and private NVS ACK cannot enable a new method either. */
 CHECK(dsr_network_bind_busy()&&dsr_ui_onboarding_active());
 click("Dalej");CHECK(test_ble_scans()==scans);
 test_workspace_finish(true);test_workspace_defer(false);tick();
 CHECK(maintenance_ready&&maintenance.kind==DSR_JOB_LAYOUT_PUBLISH&&dsr_network_bind_busy());
 click("Dalej");CHECK(test_ble_scans()==scans&&dsr_ui_onboarding_active());
 hold_maintenance=false;tick();CHECK(!dsr_network_bind_busy());
 fprintf(stderr,"Late old BIND success: new BLE active=%u HOME active=%u bind busy=%u\n",dsr_ui_ble_active(),dsr_mobile_active(),dsr_network_busy());
 /* Either admission waits for BIND, or the new session owns its view. A
  * completion from the old session cannot replace either one with HOME. */
 CHECK(!dsr_mobile_active());
 if(started)CHECK(dsr_ui_ble_active());
 else{CHECK(dsr_ui_onboarding_active());click("Dalej");CHECK(dsr_ui_ble_active()&&test_ble_scans()==scans+1);}
 printf("PASS: %u isolated actual onboarding/BIND lifecycle checks\n",checks);lv_deinit();return 0;
}
