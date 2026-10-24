/* Actual actor, fleet and LCD; transport/Flash remain host providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
unsigned test_cover_requests(void);
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();cJSON *r=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Cover A\",\"kind\":\"cover\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0},{\"id\":2,\"name\":\"Cover B\",\"kind\":\"cover\",\"host\":\"192.0.2.2\",\"channel\":0,\"room\":0}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"groups",cJSON_Parse("[{\"id\":1,\"name\":\"Covers\",\"members\":[1,2]}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles",cJSON_CreateArray());publish_document(r);
 boot_loaded_count=2;for(unsigned i=0;i<2;i++){boot_loaded[i]=(dsr_connection_t){.layout_id=i+1,.device={.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_COVER}};snprintf(boot_loaded[i].device.host,16,"192.0.2.%u",i+1);snprintf(boot_loaded[i].device.device_id,64,"cover-%u",i);}
 dsr_ui_init();tick();for(unsigned i=0;i<2;i++){CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.cover={.flags=DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,.state=DSR_COVER_STATE_OPENING,.current_pos=40}}};response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 dsr_ui_actions_group_execute(1,(dsr_group_intent_t){.command=DSR_ACTION_OPEN});tick();CHECK(in_flight);dsr_mobile_open_group(1);
 lv_obj_t *stop=find_label(lv_screen_active(),"Stop");CHECK(stop&&!lv_obj_has_state(lv_obj_get_parent(stop),LV_STATE_DISABLED));
 unsigned moves=job_counts[DSR_JOB_COVER];click("Stop");CHECK(stop_pending&&dsr_mobile_page()==DSR_MOBILE_GROUP_CONTROL);uint32_t first=stop_result.operation_id;CHECK(test_cover_requests()==2);
 stop_ready=true;stop_result.result=DSR_OK;stop_result.device.online=true;stop_result.telemetry.cover.state=DSR_COVER_STATE_STOPPED;
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);tick();
 CHECK(stop_pending&&stop_result.operation_id!=first&&stop_result.layout_id==2);
 stop_ready=true;stop_result.result=DSR_OK;stop_result.device.online=true;stop_result.telemetry.cover.state=DSR_COVER_STATE_STOPPED;tick();CHECK(!stop_pending);
 CHECK(job_counts[DSR_JOB_COVER]==moves);dsr_actions_snapshot_t status;dsr_actions_snapshot(&status);CHECK(!status.active);
 printf("PASS: %u group STOP checks; active actor cancelled, two accepted STOPs retained through serial handoff; radio MODEL\n",checks);lv_deinit();return 0;
}
