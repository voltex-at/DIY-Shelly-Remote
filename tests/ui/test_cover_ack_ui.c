/* Real fleet/LCD handoff. Worker events and radio are explicit providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static void actual_position(uint8_t state,uint8_t position)
{
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.cover={.state=state,.current_pos=position,.flags=DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL}}};
 response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();cJSON *r=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Cover ACK\",\"kind\":\"cover\",\"host\":\"192.0.2.1\",\"channel\":0,\"room\":0}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"groups",cJSON_Parse("[{\"id\":1,\"name\":\"Cover group\",\"members\":[1]}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles",cJSON_CreateArray());publish_document(r);
 boot_loaded_count=1;boot_loaded[0]=(dsr_connection_t){.layout_id=1,.device={.host="192.0.2.1",.device_id="cover-ack",.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_COVER}};
 dsr_ui_init();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);actual_position(DSR_COVER_STATE_STOPPED,38);
 dsr_network_device_control(1);click("Cover ACK");CHECK(find_label(lv_screen_active(),"Pozycja 38%"));
 dsr_mobile_device_t before,after;CHECK(dsr_network_typed_device(1,&before));
 /* A lost ACK is still uncertain, but its read-only recovery must start
  * immediately rather than leaving a one-second gap for unrelated work. */
 click("Otwórz");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER);
 response=(dsr_event_t){.device=submitted.connection.device,.command_phase=DSR_COMMAND_WRITE};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.telemetry_only);
 CHECK(find_label(lv_screen_active(),"Polecenie niepotwierdzone")&&!find_label(lv_screen_active(),"Polecenie przyjęte"));
 actual_position(DSR_COVER_STATE_OPENING,40);CHECK(find_label(lv_screen_active(),"Pozycja 40%")&&!find_label(lv_screen_active(),"Polecenie niepotwierdzone"));
 lv_tick_inc(1000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);actual_position(DSR_COVER_STATE_STOPPED,38);
 CHECK(dsr_network_typed_device(1,&before));
 click("Otwórz");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER);
 response=(dsr_event_t){.device=submitted.connection.device,.command_phase=DSR_COMMAND_ACK};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&find_label(lv_screen_active(),"Polecenie przyjęte"));
 CHECK(dsr_network_typed_device(1,&after)&&after.sample_ms==before.sample_ms&&after.position_pct==38&&after.cover_state==DSR_COVER_STOPPED);
 unsigned moves=job_counts[DSR_JOB_COVER];
 response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Polecenie przyjęte")&&!find_label(lv_screen_active(),"Brak odpowiedzi")&&!find_label(lv_screen_active(),"Odczyt niepotwierdzony"));
 CHECK(dsr_network_typed_device(1,&after)&&after.online&&after.sample_ms==before.sample_ms&&after.position_pct==38);
 CHECK(!in_flight);lv_tick_inc(1000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&job_counts[DSR_JOB_COVER]==moves);
 actual_position(DSR_COVER_STATE_OPENING,45);CHECK(find_label(lv_screen_active(),"Pozycja 45%")&&!find_label(lv_screen_active(),"Polecenie przyjęte"));
 /* STOP ACK cannot claim that the motor already stopped or invent a sample. */
 CHECK(dsr_network_typed_device(1,&before));click("Stop");tick();CHECK(stop_pending);
 stop_result.result=DSR_OK;stop_result.command_phase=DSR_COMMAND_ACK;stop_result.device.online=false;memset(&stop_result.telemetry,0,sizeof(stop_result.telemetry));stop_ready=true;tick();
 CHECK(!stop_pending&&in_flight&&submitted.kind==DSR_JOB_PROBE&&find_label(lv_screen_active(),"Polecenie przyjęte"));
 CHECK(dsr_network_typed_device(1,&after)&&after.position_pct==45&&after.cover_state==DSR_COVER_OPENING&&after.sample_ms==before.sample_ms);
 actual_position(DSR_COVER_STATE_STOPPED,47);CHECK(find_label(lv_screen_active(),"Pozycja 47%")&&!find_label(lv_screen_active(),"Polecenie przyjęte"));
 CHECK(job_counts[DSR_JOB_COVER]==moves);
 /* Superseding an unsent move must keep the previous real sample while
  * STOP is being sent. Its ACK is independent of the cancelled move. */
 CHECK(dsr_network_typed_device(1,&before));click("Otwórz");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER);
 click("Stop");tick();CHECK(stop_pending);
 response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_BUSY,DSR_WIFI_IDLE);
 CHECK(dsr_network_typed_device(1,&after)&&after.online&&after.position_pct==47&&after.sample_ms==before.sample_ms);
 CHECK(find_label(lv_screen_active(),"Pozycja 47%")&&!find_label(lv_screen_active(),"Brak odpowiedzi"));
 stop_result.result=DSR_OK;stop_result.command_phase=DSR_COMMAND_ACK;stop_result.device.online=false;memset(&stop_result.telemetry,0,sizeof(stop_result.telemetry));stop_ready=true;tick();
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&find_label(lv_screen_active(),"Polecenie przyjęte"));actual_position(DSR_COVER_STATE_STOPPED,48);
 /* STOP submitted while an accepted move awaits GET must not borrow ACK. */
 click("Otwórz");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER);
 response=(dsr_event_t){.device=submitted.connection.device,.command_phase=DSR_COMMAND_ACK};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);click("Stop");tick();CHECK(stop_pending);
 CHECK(dsr_network_typed_device(1,&after)&&after.command_phase==DSR_COMMAND_NONE&&!find_label(lv_screen_active(),"Polecenie przyjęte"));
 actual_position(DSR_COVER_STATE_OPENING,49);
 stop_result.result=DSR_OK;stop_result.command_phase=DSR_COMMAND_ACK;stop_result.device.online=false;stop_ready=true;tick();
 CHECK(find_label(lv_screen_active(),"Polecenie przyjęte"));actual_position(DSR_COVER_STATE_STOPPED,50);
 /* A busy group's disabled movement controls are not missing capability. */
 dsr_ui_actions_group_execute(1,(dsr_group_intent_t){.command=DSR_ACTION_OPEN});tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER);dsr_mobile_open_group(1);
 response=(dsr_event_t){.device=submitted.connection.device,.command_phase=DSR_COMMAND_ACK};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&find_label(lv_screen_active(),"Polecenie przyjęte")&&!find_label(lv_screen_active(),"Brak obsługi Cover RPC"));
 actual_position(DSR_COVER_STATE_OPENING,51);
 printf("PASS: %u real fleet/LCD Cover ACK checks; background retry, actual motion, independent STOP and no replay; transport MODEL\n",checks);lv_deinit();return 0;
}
