/* Actual LVGL/discovery/migration/fleet. SDK/worker/HTTP/storage are MODEL. */
#define main original_ui_main
#include "test_ui.c"
#undef main
unsigned test_light_requests(void);
static void light_response(bool on,uint8_t brightness,uint8_t conditions)
{
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.light={.brightness=brightness,.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS,.conditions=conditions}}};response.device.online=true;response.device.output=on;response.device.has_switch=false;deliver(DSR_OK,DSR_WIFI_IDLE);
}
static void point(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(20);lv_indev_read(pointer);lv_obj_update_layout(lv_screen_active());}
static lv_obj_t *light_input(lv_obj_t *o)
{
 if(lv_obj_check_type(o,&lv_arc_class)&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *input=light_input(lv_obj_get_child(o,(int)i));if(input)return input;}return NULL;
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();CHECK(!in_flight);
 dsr_network_device(NULL);click("Dodaj ręcznie (IP)");CHECK(dsr_ui_onboarding_active());click("Dalej");CHECK(!dsr_ui_onboarding_active());lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"192.168.1.80");lv_dropdown_set_selected(find_type(lv_screen_active(),&lv_dropdown_class),2);click("Check & save device");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.endpoint_kind==DSR_ENDPOINT_LIGHT);response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_UNSUPPORTED,DSR_WIFI_IDLE);CHECK(!dsr_network_busy()&&!publish_jobs);
 dsr_lan_device_t identity={.host="192.168.1.80",.id="shellydimmerg3-aabbccddeeff",.mac="AABBCCDDEEFF",.model="S3DM-0A101WWL",.name="Światło testowe",.generation=3};
 unsigned starts=test_rpc_starts(),confirms=test_rpc_confirms();dsr_ui_ble_rpc_open(&rpc_candidate);CHECK(test_rpc_starts()==starts);click("Sprawdź BLE");CHECK(test_rpc_starts()==starts+1&&test_rpc_confirms()==confirms);
 test_rpc_update(DSR_BLE_RPC_CONFIRM,DSR_OK,true,&identity);dsr_ui_ble_rpc_tick();CHECK(find_label(lv_screen_active(),identity.id)&&test_rpc_confirms()==confirms);click("Przekaż Wi-Fi");CHECK(test_rpc_confirms()==confirms+1);
 test_rpc_update(DSR_BLE_RPC_LAN_READY,DSR_OK,false,&identity);dsr_ui_ble_rpc_tick();CHECK(!in_flight&&!publish_jobs);
 test_lan_found("{\"gen\":3,\"id\":\"shellydimmerg3-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"S3DM-0A101WWL\",\"name\":\"Światło testowe\",\"auth_en\":false}","192.168.1.80");dsr_ui_ble_rpc_tick();CHECK(in_flight&&submitted.kind==DSR_JOB_LAN_STA_VERIFY&&!publish_jobs);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);
 test_lan_inspected(DSR_OK,"{\"light:0\":{\"id\":0,\"output\":true,\"brightness\":38}}");dsr_ui_ble_rpc_tick();CHECK(find_label(lv_screen_active(),"Nazwa i pomieszczenie")&&!dsr_ui_ble_rpc_active());click("Zapisz");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.endpoint_kind==DSR_ENDPOINT_LIGHT);light_response(true,38,0);
 cJSON *r=room_document(),*entry=cJSON_GetObjectItemCaseSensitive(r,"devices")->child;CHECK(entry&&!strcmp(cJSON_GetObjectItemCaseSensitive(entry,"kind")->valuestring,"light"));uint32_t id=(uint32_t)cJSON_GetObjectItemCaseSensitive(entry,"id")->valueint;cJSON_Delete(r);CHECK(id&&dsr_network_layout_reserved(id));
 dsr_mobile_device_t typed;CHECK(dsr_network_typed_device(id,&typed)&&typed.endpoint_kind==DSR_ENDPOINT_LIGHT&&typed.can_light);
 dsr_ui_usage_open(id);CHECK(find_label(lv_screen_active(),"Zużycie"));screenshot("265-light-history-unsupported");dsr_network_device_control(id);CHECK(find_label(lv_screen_active(),"38%")&&!find_label(lv_screen_active(),"DEMO • dane przykładowe"));screenshot("260-light-confirmed-readback");
 lv_obj_t *dial=light_input(lv_screen_active());CHECK(dial&&!lv_obj_has_state(dial,LV_STATE_DISABLED));lv_area_t bounds;lv_obj_get_coords(dial,&bounds);int cx=(bounds.x1+bounds.x2)/2,cy=(bounds.y1+bounds.y2)/2,radius=lv_area_get_width(&bounds)/2-15,sx=cx-radius,ex=cx-radius*7/10,ey=cy-radius*7/10;
 unsigned before=requests,intents=test_light_requests();point(sx,cy,true);point(ex,ey,true);CHECK(requests==before&&test_light_requests()==intents&&dsr_network_typed_device(id,&typed)&&typed.light.brightness==38);int selected=lv_arc_get_value(dial);CHECK(selected>=0&&selected<=100&&selected!=38);screenshot("261-light-preview-no-POST");
 /* Poll and locale changes preserve the native widget and its visible value. */
 tick();CHECK(light_input(lv_screen_active())==dial&&lv_arc_get_value(dial)==selected);r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString("de"));publish_document(r);tick();CHECK(light_input(lv_screen_active())==dial&&lv_arc_get_value(dial)==selected&&requests==before);
 point(ex,ey,false);tick();CHECK(test_light_requests()==intents+1&&in_flight&&submitted.kind==DSR_JOB_LIGHT&&submitted.light.command==DSR_LIGHT_SET_BRIGHTNESS&&submitted.light.brightness==selected);light_response(true,(uint8_t)selected,0);screenshot("262-light-readback-de");
 r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString("en"));publish_document(r);tick();screenshot("263-light-readback-en");r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString("pl"));publish_document(r);tick();
 /* Release outside the enlarged native touch area cancels the intent. */
 dial=light_input(lv_screen_active());lv_obj_get_coords(dial,&bounds);intents=test_light_requests();point(sx,cy,true);point(ex,ey,true);point(ex,bounds.y2+70,false);tick();CHECK(test_light_requests()==intents&&!in_flight);
 /* A stale gesture must not survive an offline/online round trip. */
 dial=light_input(lv_screen_active());point(sx,cy,true);point(ex,ey,true);intents=test_light_requests();wifi.connected=false;tick();point(ex,ey,false);tick();CHECK(test_light_requests()==intents&&!in_flight);wifi.connected=true;tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);light_response(true,0,DSR_LIGHT_UNCALIBRATED);CHECK(find_label(lv_screen_active(),"0%")&&find_label(lv_screen_active(),"Sterowanie światłem zablokowane")&&lv_obj_has_state(light_input(lv_screen_active()),LV_STATE_DISABLED));screenshot("264-light-uncalibrated-blocked");
 before=requests;click("Włączony");point(sx,cy,true);point(ex,ey,false);tick();CHECK(requests==before&&test_light_requests()==intents);wifi.connected=false;tick();CHECK(!in_flight&&dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 for(unsigned i=0;i<2;i++){dsr_network_device_control(id);dsr_network_home(NULL);}lv_mem_monitor_t before_memory,after_memory;lv_mem_monitor(&before_memory);for(unsigned i=0;i<40;i++){dsr_network_device_control(id);dsr_network_home(NULL);}lv_mem_monitor(&after_memory);CHECK(before_memory.free_size==after_memory.free_size);printf("Light UI heap %lu→%lu;40 navigation cycles; MODEL backend\n",(unsigned long)before_memory.free_size,(unsigned long)after_memory.free_size);
 /* Light cannot be newly assigned to Switch groups. Existing legacy entries
  * remain removable; a second click must not re-add the unsupported family. */
 before=requests;dsr_ui_config_registry(NULL);click("Grupy");click("+ Dodaj wpis");CHECK(!find_type(lv_screen_active(),&lv_checkbox_class));dsr_network_home(NULL);
 r=room_document();cJSON *group=cJSON_CreateObject();cJSON_AddNumberToObject(group,"id",91);cJSON_AddStringToObject(group,"name","Legacy Light group");cJSON *members=cJSON_AddArrayToObject(group,"members");cJSON_AddItemToArray(members,cJSON_CreateNumber(id));cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"groups"),group);publish_document(r);tick();
 dsr_ui_config_registry(NULL);click("Grupy");click("Legacy Light group");lv_obj_t *member=find_type(lv_screen_active(),&lv_checkbox_class);CHECK(member&&lv_obj_has_state(member,LV_STATE_CHECKED));lv_obj_remove_state(member,LV_STATE_CHECKED);lv_obj_send_event(member,LV_EVENT_VALUE_CHANGED,NULL);CHECK(!lv_obj_has_state(member,LV_STATE_CHECKED));lv_obj_add_state(member,LV_STATE_CHECKED);lv_obj_send_event(member,LV_EVENT_VALUE_CHANGED,NULL);CHECK(!lv_obj_has_state(member,LV_STATE_CHECKED));click("Zapisz");dsr_ui_workspace_tick();r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"groups")->child,"members"))==0);cJSON_Delete(r);CHECK(requests==before);dsr_network_home(NULL);
 printf("PASS: %u actual Light LCD checks; %u frames; two B1 consents, typed BIND, native preview/release, poll+locale preservation and stale cancellation; SDK/storage MODEL\n",checks,screenshots);lv_deinit();return 0;
}
