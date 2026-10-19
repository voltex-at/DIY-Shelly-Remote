/* Genuine LVGL LAN wizard, BLU card/details, workspace; SDK/HTTP/storage MODEL. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static lv_obj_t *dropdown_with(lv_obj_t *root,const char *needle)
{
 if(lv_obj_check_type(root,&lv_dropdown_class)&&strstr(lv_dropdown_get_options(root),needle))return root;
 for(uint32_t i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *found=dropdown_with(lv_obj_get_child(root,i),needle);if(found)return found;}
 return NULL;
}
static unsigned option_index(lv_obj_t *dd,const char *needle)
{
 const char *o=lv_dropdown_get_options(dd),*hit=strstr(o,needle);unsigned n=0;for(const char *p=o;p<hit;p++)n+=*p=='\n';return n;
}

static void trv_response(float target,uint8_t pos,dsr_result_t result)
{
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_blu_battery=true,.blu_battery_pct=100,.trv_target_c=target,.trv_current_c=21.5f,
  .blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_RSSI,.trv_pos=pos,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET,.rssi=-51}}};
 strcpy(response.device.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a");response.device.online=result==DSR_OK;deliver(result,DSR_WIFI_IDLE);
}
static void blu_response(bool motion,bool online)
{
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_illuminance=true,.illuminance_lux=317,.has_blu_battery=true,.blu_battery_pct=92,
  .blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_MOTION|(motion?DSR_BLU_MOTION:0)|DSR_BLU_HAS_RSSI,.rssi=-71}}};
 strcpy(response.device.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a");response.device.online=online;deliver(online?DSR_OK:DSR_OFFLINE,DSR_WIFI_IDLE);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();CHECK(!in_flight);
 /* Gateway with a relay and two BLU devices on cids 200 and 203. */
 dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":2,\"id\":\"shellyplus1pm-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-001P16EU\",\"auth_en\":false}","192.168.1.70");dsr_ui_lan_tick();click("Shelly Plus 1PM");
 test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false},\"bthomedevice:200\":{\"id\":200},\"bthomedevice:203\":{\"id\":203}}");dsr_ui_lan_tick();
 lv_obj_t *channel=dropdown_with(lv_screen_active(),"Czujnik BLU 4");CHECK(channel&&strstr(lv_dropdown_get_options(channel),"Czujnik BLU 1")&&!strstr(lv_dropdown_get_options(channel),"Czujnik BLU 2"));
 CHECK(find_label(lv_screen_active(),"Czujniki BLU są odczytywane przez tę bramkę Shelly."));
 CHECK(lv_dropdown_get_selected(channel)==0&&!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Shelly Plus 1PM"));lv_dropdown_set_selected(channel,option_index(channel,"Czujnik BLU 4"));lv_obj_send_event(channel,LV_EVENT_VALUE_CHANGED,NULL);CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Czujnik BLU 4"));screenshot("360-lan-blu-choice");
 click("Zapisz");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.endpoint_kind==DSR_ENDPOINT_BLU&&submitted.connection.device.channel==3&&submitted.connection.device.generation==DSR_GEN2_PLUS&&!strcmp(submitted.connection.device.device_id,"shellyplus1pm-aabbccddeeff"));
 blu_response(true,true);tick();
 cJSON *doc=room_document();uint32_t id=0;for(cJSON *v=cJSON_GetObjectItemCaseSensitive(doc,"devices")->child;v;v=v->next)if(!strcmp(cJSON_GetObjectItemCaseSensitive(v,"host")->valuestring,"192.168.1.70")){id=(uint32_t)cJSON_GetObjectItemCaseSensitive(v,"id")->valueint;CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(v,"kind")->valuestring,"sensor")&&cJSON_GetObjectItemCaseSensitive(v,"channel")->valueint==3);}cJSON_Delete(doc);CHECK(id&&dsr_network_layout_reserved(id));dsr_network_home(NULL);tick();
 dsr_mobile_device_t typed;CHECK(dsr_network_typed_device(id,&typed)&&typed.endpoint_kind==DSR_ENDPOINT_BLU&&!typed.can_switch&&!typed.has_power&&typed.has_illuminance&&typed.illuminance_lux==317&&typed.has_battery&&typed.battery_pct==92&&(typed.blu.flags&DSR_BLU_MOTION));
 dsr_network_device_control(id);CHECK(find_label(lv_screen_active(),"Shelly BLU • przez bramkę")&&find_label(lv_screen_active(),"Ruch wykryty")&&find_label(lv_screen_active(),"317 lx")&&find_label(lv_screen_active(),"Bateria 92% • -71 dBm"));screenshot("361-blu-motion-details");
 /* Read-only: no output command anywhere; refresh is a probe. */
 unsigned before=requests;click("Odśwież status");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&requests==before+1);blu_response(false,true);tick();CHECK(find_label(lv_screen_active(),"Brak ruchu"));
 /* Owner video 0.7.44: values blinked to "—" every 15 s. Poll in flight and up to 45 s old: keep showing them. */
 lv_tick_inc(20000);tick();CHECK(find_label(lv_screen_active(),"Brak ruchu")&&find_label(lv_screen_active(),"317 lx")&&!find_label(lv_screen_active(),"Bramka nie odpowiada")&&!find_label(lv_screen_active(),"Odczyt stanu…"));screenshot("365-blu-poll-keeps-values");
 if(in_flight){blu_response(false,true);tick();}
 click("Odśwież status");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);blu_response(false,false);tick();CHECK(find_label(lv_screen_active(),"Bramka nie odpowiada")&&!find_label(lv_screen_active(),"Brak ruchu")&&!find_label(lv_screen_active(),"317 lx")&&find_label(lv_screen_active(),"Bateria —"));screenshot("362-blu-gateway-offline");

 /* BLU TRV (owner reference: Shelly Smart Control): value chips, 4..30 C dial, -/+ send 1.2 s after the last tap. */
 click("Odśwież status");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);trv_response(20.0f,28,DSR_OK);tick();
 CHECK(find_label(lv_screen_active(),"21.5 °C")&&find_label(lv_screen_active(),"zawór 28%")&&find_label(lv_screen_active(),"Bateria 100%")&&find_label(lv_screen_active(),"Temperatura")&&find_label(lv_screen_active(),"20.0 °C"));
 CHECK(find_label(lv_screen_active(),"4 °C")&&find_label(lv_screen_active(),"30 °C")&&!find_label(lv_screen_active(),"Przycisk 0")&&!find_label(lv_screen_active(),"317 lx"));screenshot("366-trv-thermostat");
 dsr_mobile_device_t trv_typed;CHECK(dsr_network_typed_device(id,&trv_typed)&&trv_typed.has_trv_target&&trv_typed.trv_target_c==20.0&&trv_typed.has_temperature&&trv_typed.temperature_c==21.5); /* has_illuminance shares has_trv_target: the screen check above proves no lux */
 unsigned trv_before=requests;click("+");CHECK(find_label(lv_screen_active(),"20.5 °C")&&find_label(lv_screen_active(),"Wysyłanie do termostatu przez bramkę…")&&requests==trv_before&&!in_flight);
 click("+");CHECK(find_label(lv_screen_active(),"21.0 °C"));lv_tick_inc(1000);lv_timer_handler();CHECK(!in_flight&&requests==trv_before); /* still within 1.2 s of the last tap */
 lv_tick_inc(300);lv_timer_handler();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_TRV&&submitted.trv.command==DSR_TRV_SET_TARGET&&submitted.trv.half_c==42&&submitted.connection.device.endpoint_kind==DSR_ENDPOINT_BLU);
 trv_response(21.0f,40,DSR_OK);tick();CHECK(find_label(lv_screen_active(),"Nastawa")&&find_label(lv_screen_active(),"zawór 40%")&&find_label(lv_screen_active(),"21.0 °C"));screenshot("367-trv-target-set");
 for(unsigned i=0;i<40;i++){click("-");}
 CHECK(find_label(lv_screen_active(),"4.0 °C"));
 for(unsigned i=0;i<60;i++){click("+");}
 CHECK(find_label(lv_screen_active(),"30.0 °C"));lv_tick_inc(1300);lv_timer_handler();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_TRV&&submitted.trv.half_c==60);trv_response(21.0f,40,DSR_BAD_RESPONSE);tick();
 CHECK(!in_flight&&find_label(lv_screen_active(),"Temperatura"));
 /* Room list: TRV row has the Shelly pill (-, target, +) and sends the same way. Polls are automatic (no refresh button, as in the app). */
 lv_tick_inc(31000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);trv_response(21.0f,40,DSR_OK);tick(); /* after a failed write the poll backs off to 30 s */
 dsr_network_home(NULL);tick();dsr_mobile_show(DSR_MOBILE_DEVICES,0);tick();CHECK(find_label(lv_screen_active(),"21.0")&&find_label(lv_screen_active(),"°C"));screenshot("368-trv-list-pill");
 trv_before=requests;click("+");CHECK(find_label(lv_screen_active(),"21.5")&&!in_flight);lv_tick_inc(1300);lv_timer_handler();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_TRV&&submitted.trv.half_c==43);trv_response(21.5f,40,DSR_OK);tick();
 /* Offline TRV: -/+ do nothing. */
 lv_tick_inc(16000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);trv_response(21.5f,40,DSR_OFFLINE);tick();
 CHECK(!find_label(lv_screen_active(),"21 lx")&&!find_label(lv_screen_active(),"22 lx")); /* offline TRV: target never shown as lux */
 trv_before=requests;click("+");lv_tick_inc(1300);lv_timer_handler();tick();CHECK(requests==trv_before&&!in_flight);
 lv_tick_inc(31000);tick();if(in_flight&&submitted.kind==DSR_JOB_PROBE){blu_response(false,true);tick();}
 dsr_network_device_control(id);tick();
 /* Back to a motion sensor on this channel for the EN screen check. */
 click("Odśwież status");tick();if(in_flight){blu_response(false,true);tick();}CHECK(find_label(lv_screen_active(),"Shelly BLU • przez bramkę"));
 doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString("en"));publish_document(doc);tick();CHECK(find_label(lv_screen_active(),"Shelly BLU • via gateway"));click("Refresh status");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);blu_response(false,true);tick();CHECK(find_label(lv_screen_active(),"No motion"));screenshot("363-blu-details-en");
 lv_mem_monitor_t m1,m2;for(unsigned i=0;i<2;i++){dsr_network_device_control(id);dsr_network_home(NULL);}lv_mem_monitor(&m1);for(unsigned i=0;i<40;i++){dsr_network_device_control(id);dsr_network_home(NULL);}lv_mem_monitor(&m2);CHECK(m1.free_size==m2.free_size);
 /* BLU Gateway Gen3: channels from GetComponents, labelled by gateway name or model + MAC tail. */
 dsr_network_home(NULL);wifi.connected=true;dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":3,\"id\":\"shellyblugwg3-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"S3GW-1DBT001\",\"auth_en\":false}","192.168.1.71");dsr_ui_lan_tick();click("Shelly BLU Gateway Gen3");
 test_lan_inspected(DSR_OK,"{\"ble\":{},\"blugw\":{}}");test_lan_components("{\"components\":[{\"key\":\"bthomedevice:200\",\"config\":{\"id\":200,\"addr\":\"aa:bb:cc:00:b5:67\",\"name\":null},\"attrs\":{\"model_id\":8}},{\"key\":\"bthomedevice:201\",\"config\":{\"id\":201,\"addr\":\"aa:bb:cc:00:0b:11\",\"name\":null},\"attrs\":{\"model_id\":2}},{\"key\":\"bthomedevice:202\",\"config\":{\"id\":202,\"addr\":\"aa:bb:cc:00:aa:01\",\"name\":\"Ruch korytarz\"},\"attrs\":{\"model_id\":5}},{\"key\":\"bthomedevice:203\",\"config\":{\"id\":203,\"addr\":\"aa:bb:cc:00:aa:02\",\"name\":null},\"attrs\":{\"model_id\":5}}],\"offset\":0,\"total\":4}");dsr_ui_lan_tick();
 lv_obj_t *gw_channel=dropdown_with(lv_screen_active(),"BLU Door/Window 0b11");CHECK(gw_channel);const char *opts=lv_dropdown_get_options(gw_channel);
 CHECK(strstr(opts,"BLU TRV b567")&&strstr(opts,"Ruch korytarz")&&strstr(opts,"BLU sensor aa02")&&!strstr(opts,"BLU sensor 1"));
 CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"BLU TRV b567"));
 lv_dropdown_set_selected(gw_channel,option_index(gw_channel,"BLU Door/Window 0b11"));lv_obj_send_event(gw_channel,LV_EVENT_VALUE_CHANGED,NULL);
 CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"BLU Door/Window 0b11"));screenshot("364-blu-gateway-labels");
 dsr_ui_lan_reset();
 /* Cyber layout uses the same bound gateway, cid and read-only PROBE. */
 doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString("cyber_blue"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString("pl"));publish_document(doc);dsr_network_home(NULL);dsr_network_device_control(id);tick();
 if(in_flight){blu_response(false,true);tick();}
 CHECK(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE);
 unsigned probes=job_counts[DSR_JOB_PROBE],sets=job_counts[DSR_JOB_SET_OUTPUT];
 click("Odśwież status");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.connection.device.endpoint_kind==DSR_ENDPOINT_BLU&&submitted.connection.device.channel==3);
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_illuminance=true,.illuminance_lux=4,.has_blu_battery=true,.blu_battery_pct=100,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|DSR_BLU_HAS_RSSI,.rssi=-58}}};strcpy(response.device.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a");response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);tick();
 CHECK(find_label(lv_screen_active(),"Zamknięte")&&find_label(lv_screen_active(),"4 lx")&&find_label(lv_screen_active(),"-58 dBm"));screenshot("native-860-blu-gateway-closed");
 click("Odśwież status");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device=submitted.connection.device;response.device.online=true;response.telemetry.blu.flags|=DSR_BLU_OPEN;deliver(DSR_OK,DSR_WIFI_IDLE);tick();CHECK(find_label(lv_screen_active(),"Otwarte"));
 click("Odśwież status");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device=submitted.connection.device;response.device.online=true;response.telemetry.blu.errors=DSR_BLU_ERROR_DECRYPT;deliver(DSR_OK,DSR_WIFI_IDLE);tick();CHECK(find_label(lv_screen_active(),"Brak danych")&&!find_label(lv_screen_active(),"Otwarte")&&!find_label(lv_screen_active(),"Zamknięte"));
 CHECK(job_counts[DSR_JOB_PROBE]>=probes+3&&job_counts[DSR_JOB_SET_OUTPUT]==sets);
 printf("PASS: %u actual BLU LCD checks; %u frames; LAN gateway choice/typed BIND/read-only probe/offline/PL EN; heap %lu→%lu; SDK/storage MODEL\n",checks,screenshots,(unsigned long)m1.free_size,(unsigned long)m2.free_size);lv_deinit();return 0;
}
