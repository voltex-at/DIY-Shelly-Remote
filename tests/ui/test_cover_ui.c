/* Actual LVGL, discovery form, migration and fleet. Radio/worker/NVS providers
 * are MODEL. Genuine HTTP/registry/urgent-worker suites are separate. */
#define main original_ui_main
#include "test_ui.c"
#undef main
unsigned test_cover_requests(void);
static void cover_response(uint8_t state,uint8_t position,uint8_t flags,uint8_t errors)
{
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_voltage=true,.voltage_v=230,.has_current=true,.current_a=.014,.cover={.state=state,.current_pos=position,.flags=flags,.errors=errors}}};response.device.online=true;response.device.has_switch=false;response.device.has_power=true;response.device.power_w=3.2;deliver(DSR_OK,DSR_WIFI_IDLE);
}
static void point(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(20);lv_indev_read(pointer);lv_obj_update_layout(lv_screen_active());}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();CHECK(!in_flight);
 dsr_network_device(NULL);choose_method("Dodaj ręcznie (IP)");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"192.168.1.80");lv_dropdown_set_selected(find_type(lv_screen_active(),&lv_dropdown_class),1);click("Check & save device");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.endpoint_kind==DSR_ENDPOINT_COVER&&submitted.connection.device.generation==DSR_GEN2_PLUS);response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_UNSUPPORTED,DSR_WIFI_IDLE);CHECK(!dsr_network_busy()&&publish_jobs==0);
 /* Genuine B1 LCD lifecycle to typed LAN adoption: no SDK until consent1,
  * no Wi-Fi write until consent2, and no binding until station+HTTP identity. */
 dsr_lan_device_t identity={.host="192.168.1.80",.id="shellyplus2pm-aabbccddeeff",.mac="AABBCCDDEEFF",.model="SNSW-102P16EU",.name="Roleta testowa",.generation=2};
 unsigned starts=test_rpc_starts(),confirms=test_rpc_confirms();dsr_ui_ble_rpc_open(&rpc_candidate);CHECK(test_rpc_starts()==starts);click("Sprawdź BLE");CHECK(test_rpc_starts()==starts+1&&test_rpc_confirms()==confirms);
 test_rpc_update(DSR_BLE_RPC_CONFIRM,DSR_OK,true,&identity);dsr_ui_ble_rpc_tick();CHECK(find_label(lv_screen_active(),identity.id)&&test_rpc_confirms()==confirms);click("Przekaż Wi-Fi");CHECK(test_rpc_confirms()==confirms+1);
 test_rpc_update(DSR_BLE_RPC_LAN_READY,DSR_OK,false,&identity);dsr_ui_ble_rpc_tick();CHECK(!in_flight&&publish_jobs==0);
 test_lan_found("{\"gen\":2,\"id\":\"shellyplus2pm-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-102P16EU\",\"name\":\"Roleta testowa\",\"auth_en\":false}","192.168.1.80");dsr_ui_ble_rpc_tick();CHECK(in_flight&&submitted.kind==DSR_JOB_LAN_STA_VERIFY&&!strcmp(submitted.ap_identity.id,identity.id)&&publish_jobs==0);
 response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);test_lan_inspected(DSR_OK,"{\"cover:0\":{\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":38}}");dsr_ui_ble_rpc_tick();
 CHECK(find_label(lv_screen_active(),"Nazwa i pomieszczenie")&&!dsr_ui_ble_rpc_active()&&test_rpc_confirms()==confirms+1&&publish_jobs==0);
 lv_obj_t *choice=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(choice);click("Zapisz");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.endpoint_kind==DSR_ENDPOINT_COVER&&submitted.connection.device.channel==0);
 cover_response(DSR_COVER_STATE_STOPPED,38,DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,0);CHECK(!dsr_network_busy());
 cJSON *r=room_document(),*entry=cJSON_GetObjectItemCaseSensitive(r,"devices")->child;CHECK(entry&&!strcmp(cJSON_GetObjectItemCaseSensitive(entry,"kind")->valuestring,"cover"));uint32_t id=(uint32_t)cJSON_GetObjectItemCaseSensitive(entry,"id")->valueint;CHECK(id&&dsr_network_layout_reserved(id));cJSON_Delete(r);
 dsr_mobile_device_t typed;CHECK(dsr_network_typed_device(id,&typed)&&typed.endpoint_kind==DSR_ENDPOINT_COVER);
 dsr_ui_usage_open(id);CHECK(find_label(lv_screen_active(),"Zużycie"));screenshot("256-cover-history-explicitly-unsupported");dsr_network_device_control(id);
 click("Roleta testowa");CHECK(find_label(lv_screen_active(),"Pozycja 38%")&&!find_label(lv_screen_active(),"DEMO • dane przykładowe"));screenshot("250-cover-confirmed-readback");
 const char *locales[]={"en","de","pl"};unsigned locale_requests=requests;
 for(unsigned i=0;i<3;i++){r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(locales[i]));publish_document(r);tick();CHECK(requests==locale_requests);CHECK(find_label(lv_screen_active(),i==0?"Position 38%":i==1?"Position 38%":"Pozycja 38%"));if(i<2)screenshot(i==0?"254-cover-readback-en":"255-cover-readback-de");}
 /* 238 px now marks 67% of the resized aperture; intent remains release-only. */
 unsigned before=requests,intents=test_cover_requests();point(160,270,true);point(160,250,true);point(160,238,true);CHECK(requests==before&&test_cover_requests()==intents&&find_label(lv_screen_active(),"Pozycja 38%"));screenshot("251-cover-drag-preview-no-POST");point(160,238,false);tick();
 CHECK(test_cover_requests()==intents+1&&in_flight&&submitted.kind==DSR_JOB_COVER&&submitted.cover.command==DSR_COVER_GOTO&&submitted.cover.position==67&&find_label(lv_screen_active(),"Pozycja 38%"));
 click("Otwórz");CHECK(test_cover_requests()==intents+1);click("Stop");tick();CHECK(stop_pending&&stop_result.operation_id>0&&in_flight);lv_obj_t *stop_label=find_label(lv_screen_active(),"Stop");CHECK(stop_label&&lv_obj_has_state(lv_obj_get_parent(stop_label),LV_STATE_DISABLED));
 stop_result.result=DSR_OK;stop_result.device.online=true;stop_result.telemetry.cover=(dsr_cover_status_t){.state=DSR_COVER_STATE_STOPPED,.flags=DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,.current_pos=41};stop_ready=true;tick();CHECK(stop_pending&&in_flight); /* outer result owns first handoff */
 cover_response(DSR_COVER_STATE_CLOSING,60,DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,0);CHECK(!stop_pending&&!in_flight&&find_label(lv_screen_active(),"Pozycja 41%")&&!find_label(lv_screen_active(),"Pozycja 60%"));screenshot("252-cover-urgent-STOP-final-readback");
 before=test_cover_requests();point(160,250,true);point(160,207,true);wifi.connected=false;tick();point(160,207,false);tick();CHECK(test_cover_requests()==before&&!in_flight);wifi.connected=true;tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);cover_response(DSR_COVER_STATE_STOPPED,41,DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,0);
 /* Owner video: failed write/readback must recover via GET, with no replay. */
 click("Otwórz");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER);unsigned movements=job_counts[DSR_JOB_COVER];
 response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Nieznana pozycja"));
 lv_tick_inc(1000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&job_counts[DSR_JOB_COVER]==movements);
 cover_response(DSR_COVER_STATE_OPENING,55,DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,0);CHECK(find_label(lv_screen_active(),"Pozycja 55%"));
 lv_tick_inc(1000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);cover_response(DSR_COVER_STATE_STOPPED,67,DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,0);CHECK(find_label(lv_screen_active(),"Pozycja 67%")&&job_counts[DSR_JOB_COVER]==movements);screenshot("257-cover-lost-readback-recovered");
 /* Auth errors and invalid position cannot move; STOP can stop a known
  * device while calibrating, without any calibration command in this UI. */
 click("Otwórz");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_COVER&&submitted.cover.command==DSR_COVER_OPEN);cover_response(DSR_COVER_STATE_CALIBRATING,0,DSR_COVER_VALID,0);before=test_cover_requests();click("Zamknij");CHECK(test_cover_requests()==before);CHECK(find_label(lv_screen_active(),"Nieznana pozycja"));screenshot("253-cover-calibration-gates");
 click("Stop");tick();CHECK(stop_pending);
 dsr_network_device(NULL);click("Bluetooth");lv_obj_t *next_label=find_label(lv_screen_active(),"Dalej");CHECK(next_label&&lv_obj_has_state(lv_obj_get_parent(next_label),LV_STATE_DISABLED));unsigned scans=test_ble_scans();click("Dalej");CHECK(test_ble_scans()==scans&&stop_pending);click("Anuluj");dsr_network_device_control(id);
 before=requests;CHECK(dsr_network_request_pairing_revoke()==DSR_BUSY&&dsr_network_request_ota(DSR_JOB_OTA_CHECK,0,false)==DSR_BUSY&&dsr_network_request_ap(DSR_JOB_AP_SCAN,NULL,NULL,NULL,false)==DSR_BUSY&&dsr_network_bind_verified(&submitted.connection,0)==DSR_BUSY);lv_tick_inc(5000);tick();CHECK(requests==before&&!in_flight&&!dsr_activity_restart_reserve());stop_result.result=DSR_AUTH_REQUIRED;stop_result.device.online=false;stop_ready=true;tick();before=test_cover_requests();click("Stop");CHECK(test_cover_requests()==before&&find_label(lv_screen_active(),"Błąd logowania"));
 wifi.connected=false;tick();CHECK(!in_flight&&!stop_pending&&job_counts[DSR_JOB_SET_OUTPUT]==0);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: %u actual Cover LCD checks; %u frames; discovery/typed BIND, one drag-release intent, pending STOP ownership and final readback; radio/worker/storage MODEL\n",checks,screenshots);lv_deinit();return 0;
}
