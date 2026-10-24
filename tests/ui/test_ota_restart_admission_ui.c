/* Real owner poll and atomic activity gate. SDK reset/task are not executed. */
#define main original_ui_main
#include "test_ui.c"
#undef main
void test_native_ota_versions(const char *,const char *,uint32_t);
void test_automation_manual_ready(bool);
static void observed(bool trv,double target)
{
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=true;
 if(trv)response.telemetry=(dsr_telemetry_t){.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET},.trv_target_c=target};
 else response.device.has_switch=true;
 deliver(DSR_OK,DSR_WIFI_IDLE);
}
int main(int argc,char **argv)
{
 CHECK(argc==3);output=argv[1];bool manual=!strcmp(argv[2],"manual"),trv=!strcmp(argv[2],"trv");lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();cJSON *r=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Background relay\",\"kind\":\"relay\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0}]"));
 if(trv)cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"devices")->child,"kind",cJSON_CreateString("sensor"));
 cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles",cJSON_CreateArray());publish_document(r);
 boot_loaded_count=1;boot_loaded[0]=(dsr_connection_t){.layout_id=1,.device={.host="192.0.2.1",.device_id="restart-relay",.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_SWITCH}};
 if(trv)boot_loaded[0].device.endpoint_kind=DSR_ENDPOINT_BLU;
 dsr_ui_init();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);observed(trv,20);CHECK(!in_flight);
 if(trv){
  dsr_network_device_control(1);lv_tick_inc(16000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);
  click("+");lv_tick_inc(1300);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&!job_counts[DSR_JOB_TRV]);test_automation_manual_ready(false);
 }
 test_native_ota_set(DSR_NATIVE_STAGED,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_versions("0.7.76-dev","0.7.77",70001);dsr_ui_native_ota_open(NULL);
 restart_activity_gate=true;CHECK(dsr_activity_begin()); /* already accepted writer */
 if(manual){click("Później");lv_tick_inc(6000);click("Uruchom ponownie");}
 else lv_tick_inc(11000);
 tick();CHECK(in_flight==trv&&!dsr_activity_restart_pending());
 unsigned attempted=restart_requests;if(!trv)CHECK(attempted>0);lv_tick_inc(1000);tick();
 if(!manual)CHECK(find_label(lv_screen_active(),"Automatyczny restart za 0 s"));
 CHECK(in_flight==trv&&!dsr_activity_restart_pending()); /* writer was not bypassed */
 if(trv){
  observed(true,20);CHECK(!in_flight&&!dsr_activity_restart_pending());
  /* The config task may finish after native_trv_tick checked durability.
   * Its completed ACK owns no activity lease; the RAM intent still must drain. */
  dsr_activity_end();tick();CHECK(!in_flight&&!dsr_activity_restart_pending());
  click("Uruchom ponownie");CHECK(!dsr_activity_restart_pending());
  test_automation_manual_ready(true);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_TRV&&submitted.trv.half_c==41&&!dsr_activity_restart_pending());
  CHECK(dsr_activity_begin());observed(true,20.5);CHECK(!in_flight&&!dsr_activity_restart_pending());
 }
 dsr_activity_end();tick();CHECK(dsr_activity_restart_pending()&&find_label(lv_screen_active(),"Przyjęto żądanie restartu."));
 unsigned accepted=restart_requests;lv_tick_inc(10000);tick();CHECK(restart_requests==accepted&&!in_flight);
 dsr_activity_restart_cancel();restart_activity_gate=false;
 printf("PASS: %s OTA restart uses real idle admission outside poll lease, drains writer, stops new background reads and accepts once; reset MODEL\n",manual?"manual":"automatic");lv_deinit();return 0;
}
