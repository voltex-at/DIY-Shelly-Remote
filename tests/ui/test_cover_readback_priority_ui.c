/* Real owner/fleet/LVGL; worker completion and Wi-Fi are explicit providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static void observed(bool cover)
{
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=true;
 if(cover)response.telemetry.cover=(dsr_cover_status_t){.state=DSR_COVER_STATE_STOPPED,.current_pos=38,.flags=DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL};
 else response.device.has_switch=true;
 deliver(DSR_OK,DSR_WIFI_IDLE);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();cJSON *r=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Other relay\",\"kind\":\"relay\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0},{\"id\":2,\"name\":\"Cover priority\",\"kind\":\"cover\",\"host\":\"192.0.2.2\",\"channel\":0,\"room\":0}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles",cJSON_CreateArray());publish_document(r);
 boot_loaded_count=2;
 for(unsigned i=0;i<2;i++){boot_loaded[i]=(dsr_connection_t){.layout_id=i+1,.device={.generation=DSR_GEN2_PLUS,.endpoint_kind=i?DSR_ENDPOINT_COVER:DSR_ENDPOINT_SWITCH}};snprintf(boot_loaded[i].device.host,sizeof(boot_loaded[i].device.host),"192.0.2.%u",i+1);snprintf(boot_loaded[i].device.device_id,sizeof(boot_loaded[i].device.device_id),"priority-%u",i+1);}
 dsr_ui_init();tick();CHECK(in_flight&&submitted.connection.layout_id==1);observed(false);
 CHECK(in_flight&&submitted.connection.layout_id==2);observed(true);CHECK(!in_flight);
 dsr_network_device_control(2);click("Otwórz");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER);
 unsigned moves=job_counts[DSR_JOB_COVER];lv_tick_inc(5000);
 /* Navigate while the command is in flight. An overdue newly visible relay
  * must not take the one status-recovery opportunity from this command. */
 dsr_network_device_control(1);
 response=(dsr_event_t){.device=submitted.connection.device,.command_phase=DSR_COMMAND_WRITE};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.connection.layout_id==2&&submitted.telemetry_only);
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.cover={.state=DSR_COVER_STATE_OPENING,.current_pos=45,.flags=DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL}}};response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.connection.layout_id==1&&!submitted.telemetry_only);observed(false);
 CHECK(job_counts[DSR_JOB_COVER]==moves);
 dsr_network_device_control(2);CHECK(find_label(lv_screen_active(),"Pozycja 45%")&&!find_label(lv_screen_active(),"Polecenie przyjęte"));
 /* An urgent STOP has no ordinary Cover job in the owner. Its ACK must
  * nevertheless get the same one-shot readback priority, even off-screen. */
 click("Stop");tick();CHECK(stop_pending);lv_tick_inc(5000);dsr_network_device_control(1);
 stop_result.result=DSR_OK;stop_result.command_phase=DSR_COMMAND_ACK;stop_result.device.online=false;stop_ready=true;tick();
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.connection.layout_id==2&&submitted.telemetry_only);
 observed(true);CHECK(in_flight&&submitted.connection.layout_id==1);observed(false);
 CHECK(job_counts[DSR_JOB_COVER]==moves);
 printf("PASS: %u Cover recovery scheduling checks; one GET before unrelated viewport, then background resumes; no replay or synthetic ACK; transport MODEL\n",checks);lv_deinit();return 0;
}
