/* Route atlas: production LVGL; radio, clock, ADC and storage are fixtures.
 * Complements the actor suites, rather than imitating screens in a browser. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_device_settings.h"
#include "../../components/dsr_ui/dsr_ui_provision.h"
void test_settings_enable(bool enabled);
void test_settings_complete(bool enabled);
static unsigned level_writes,color_writes;
static uint8_t last_level;
static void level_set(unsigned i,uint8_t v){CHECK(i==0&&v<=100);last_level=v;level_writes++;}
static void color_set(unsigned i,uint8_t r,uint8_t g,uint8_t b){CHECK(i==0);(void)r;(void)g;(void)b;color_writes++;}
static void ignored_refresh(uint32_t id){(void)id;}
static void snap(const char *name){char s[128];snprintf(s,sizeof(s),"native-full-%s",name);screenshot(s);}
static void scrolls(lv_obj_t *o,const char *name,unsigned *n)
{
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return;
 lv_obj_update_layout(o);
 int range=lv_obj_get_scroll_bottom(o)+lv_obj_get_scroll_y(o);
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_SCROLLABLE)&&range>16&&!lv_obj_check_type(o,&lv_textarea_class)){
  int height=lv_obj_get_height(o);if(height<44)return;
  for(int y=height-30;y<range+height-30;y+=height-30){
   lv_obj_scroll_to_y(o,y,LV_ANIM_OFF);char s[128];snprintf(s,sizeof(s),"%s-scroll-%u",name,++*n);snap(s);
  }
  lv_obj_scroll_to_y(o,0,LV_ANIM_OFF);
 }
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)scrolls(lv_obj_get_child(o,(int)i),name,n);
}
static void route(const char *name){snap(name);unsigned n=0;scrolls(lv_screen_active(),name,&n);}
static void press(const char *text)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);if(!label){fprintf(stderr,"Atlas missing label: %s\n",text);snap("missing-label");}CHECK(label);
 lv_obj_t *b=lv_obj_get_parent(label);lv_obj_scroll_to_view(b,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
 lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);
}
static void setup(void)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString("cyber_blue"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString("pl"));
 cJSON *rooms=cJSON_Parse("[{\"id\":1,\"name\":\"Salon\",\"icon\":0,\"photo\":\"\"},{\"id\":2,\"name\":\"Sypialnia\",\"icon\":1,\"photo\":\"\"}]");cJSON_ReplaceItemInObjectCaseSensitive(doc,"rooms",rooms);
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Lampa salon\",\"kind\":\"relay\",\"host\":\"192.0.2.20\",\"channel\":0,\"room\":1}]"));
 cJSON *dash=cJSON_GetObjectItemCaseSensitive(doc,"dashboards")->child;cJSON_ReplaceItemInObjectCaseSensitive(dash,"tiles",cJSON_Parse("[{\"id\":1,\"type\":\"device\",\"ref\":1,\"span\":2,\"height\":1,\"fields\":255}]"));
 publish_document(doc);dsr_network_home(NULL);tick();CHECK(dsr_sc_style()==3);
}
static void global_routes(void)
{
 dsr_ui_open_settings();route("settings");press("Sieć i połączenia");route("network");
 dsr_ui_open_settings();press("Ekran i komfort");route("comfort");press("Ekran");route("display");
 lv_dropdown_open(find_type(lv_screen_active(),&lv_dropdown_class));snap("display-timeout-list");lv_dropdown_close(find_type(lv_screen_active(),&lv_dropdown_class));
 dsr_ui_open_settings();press("Panel WWW / bezpieczeństwo");route("web-security");press("Wygeneruj kod parowania");route("web-pair-code");lv_tick_inc(120001);dsr_ui_settings_tick();route("web-pair-expired");
 dsr_ui_open_settings();press("Panel WWW / bezpieczeństwo");press("Unieważnij parowanie WWW");route("web-revoke-confirm");
 dsr_ui_open_settings();press("System");route("system");press("Informacje i diagnostyka");route("system-info");press("Diagnostyka konfiguracji");route("config-diagnostics");
 dsr_ui_open_settings();press("System");press("Restart pilota");route("restart-confirm");
 dsr_ui_open_settings();press("System");press("Hardware tests");route("hardware");press("Check touchscreen");route("touch-test");press("Back");
 dsr_ui_open_settings();press("System");press("Hardware tests");press("Check display colors");route("display-colors");
 dsr_ui_open_settings();press("Wygląd i język");route("appearance");dsr_network_home(NULL);
 dsr_ui_battery_open(NULL);route("battery-unknown");dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3800}};dsr_ui_battery_result(&e);route("battery-measured");lv_tick_inc(30001);dsr_ui_battery_tick();route("battery-stale");
 e.result=DSR_IO;dsr_ui_battery_result(&e);route("battery-error");
 dsr_ui_usage_open(1);route("usage-no-binding");dsr_network_home(NULL);
}
static void editing_routes(void)
{
 dsr_ui_select_dashboard();route("dashboard-picker");dsr_ui_edit_dashboard();route("editor-menu");
 press("Pulpity");route("dashboard-list");press("Mój pulpit");route("dashboard-edit");press("Dane");route("tile-fields");
 dsr_network_home(NULL);dsr_ui_config_registry(NULL);route("registry");
 press("Urządzenia");route("device-list-editor");press("Lampa salon");route("device-registry-edit");
 dsr_network_home(NULL);dsr_ui_edit_local_device(1);route("device-card-edit");
 dsr_network_home(NULL);dsr_ui_add_room();route("room-create");dsr_network_home(NULL);dsr_ui_edit_room(1);route("room-edit");
 dsr_network_home(NULL);dsr_ui_edit_action(1,0);route("scene-step-name");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"Wieczór");press("Dalej");route("scene-step-actions");press("+ Dodaj polecenie");route("scene-device-picker");
 dsr_network_home(NULL);dsr_ui_edit_action(0,0);route("group-step-name");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"Oświetlenie");press("Dalej");route("group-member-picker");
 dsr_network_home(NULL);dsr_ui_actions_open(0);route("groups");dsr_network_home(NULL);dsr_ui_actions_open(1);route("scenes");
 dsr_network_home(NULL);test_workspace_fault(1);dsr_ui_edit_dashboard();route("storage-unavailable");test_workspace_fault(0);dsr_network_home(NULL);
}
static void lighting_routes(void)
{
 dsr_mobile_actions_t a={.set_brightness=level_set,.set_rgb=color_set,.refresh=ignored_refresh};dsr_mobile_bind(&a);
 dsr_mobile_model_t m={.wifi=true,.device_count=1,.room_count=2,.home_name="Mój dom",.rooms={{.name="Salon",.id=1},{.name="Sypialnia",.id=2}}};
 dsr_mobile_device_t *d=&m.devices[0];*d=(dsr_mobile_device_t){.id=31,.name="Shelly Dimmer Gen3",.identity="atlas-light",.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_LIGHT,.kind=DSR_DEVICE_LIGHT,.online=true,.output=true,.can_light=true,.can_refresh=true,.has_power=true,.power_w=24.8,.has_voltage=true,.voltage_v=232.1,.has_current=true,.current_a=.11,.has_sample_time=true,.sample_ms=lv_tick_get(),.light={.brightness=75,.flags=DSR_LIGHT_BRIGHTNESS}};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);route("home");dsr_mobile_show(DSR_MOBILE_ROOMS,0);route("rooms");dsr_mobile_show(DSR_MOBILE_ROOM,1);route("room-detail");dsr_mobile_show(DSR_MOBILE_DEVICES,0);route("devices");
 for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,3,lang);dsr_ui_apply_theme();dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);char name[48];snprintf(name,sizeof(name),"dimmer-%s",lang==0?"pl":lang==1?"en":"de");route(name);
 }
 dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_mobile_show(DSR_MOBILE_DETAILS,0);
 lv_obj_t *slider=find_type(lv_screen_active(),&lv_slider_class);CHECK(slider);lv_area_t box;lv_obj_get_coords(slider,&box);int y=(box.y1+box.y2)/2;
 touch=(lv_point_t){100,y};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);touch.x=210;lv_tick_inc(20);lv_indev_read(pointer);CHECK(!level_writes);snap("dimmer-preview");pressed=false;lv_tick_inc(20);lv_indev_read(pointer);CHECK(level_writes==1);
 press("+");CHECK(level_writes==2&&last_level==80&&find_label(lv_screen_active(),"75%"));
 press("-");CHECK(level_writes==3&&last_level==70&&find_label(lv_screen_active(),"75%"));
 d->online=false;dsr_mobile_update(&m);route("dimmer-offline");CHECK(lv_obj_has_state(find_type(lv_screen_active(),&lv_slider_class),LV_STATE_DISABLED));
 d->online=true;d->endpoint_kind=DSR_ENDPOINT_RGB;d->kind=DSR_DEVICE_RGB;d->can_rgb=true;d->rgb=(dsr_rgb_status_t){.r=0,.g=180,.b=255,.brightness=78,.flags=DSR_RGB_VALID};strcpy(d->name,"Shelly RGB");
 for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,3,lang);dsr_ui_apply_theme();dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);char name[64];snprintf(name,sizeof(name),"rgb-brightness-%u",lang);route(name);press("Kolor RGB");snprintf(name,sizeof(name),"rgb-color-%u",lang);route(name);
 }
 dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_mobile_show(DSR_MOBILE_DETAILS,0);route("rgb-brightness");press("Kolor RGB");route("rgb-color");
 slider=find_type(lv_screen_active(),&lv_slider_class);lv_slider_set_value(slider,120,LV_ANIM_OFF);lv_obj_send_event(slider,LV_EVENT_VALUE_CHANGED,NULL);CHECK(!color_writes);route("rgb-preview");press("Zastosuj");CHECK(color_writes==1);
 d->rgb.flags|=DSR_RGB_TRANSITION;dsr_mobile_update(&m);route("rgb-transition");d->online=false;dsr_mobile_update(&m);route("rgb-offline");
 d->online=true;d->rgb.flags=DSR_RGB_VALID;dsr_mobile_update(&m);
 for(unsigned cycle=0;cycle<8;cycle++){dsr_mobile_show(DSR_MOBILE_DETAILS,0);press("Kolor RGB");press("Jasność");}
 dsr_mobile_hide();lv_obj_clean(lv_screen_active());lv_refr_now(display);lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned cycle=0;cycle<60;cycle++){dsr_mobile_show(DSR_MOBILE_DETAILS,0);press("Kolor RGB");press("Jasność");dsr_mobile_hide();lv_obj_clean(lv_screen_active());lv_refr_now(display);}
 lv_mem_monitor(&after);CHECK(after.free_size==before.free_size&&after.used_cnt==before.used_cnt);
 printf("Cyber lighting memory: %lu -> %lu bytes free after 60 brightness/color/back cycles, 128 KiB LVGL host pool; not target RAM/PSRAM\n",(unsigned long)before.free_size,(unsigned long)after.free_size);
 *d=(dsr_mobile_device_t){.id=31,.name="Shelly H&T Gen3",.endpoint_kind=DSR_ENDPOINT_HT,.kind=DSR_DEVICE_SENSOR,.online=true,.can_refresh=true,.has_temperature=true,.temperature_c=21.4,.has_humidity=true,.humidity_pct=46.2,.has_battery=true,.battery_pct=85};
 for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,3,lang);dsr_ui_apply_theme();d->online=true;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);char name[64];snprintf(name,sizeof(name),"ht-online-%u",lang);route(name);d->online=false;dsr_mobile_update(&m);snprintf(name,sizeof(name),"ht-sleeping-%u",lang);route(name);CHECK(!find_label(lv_screen_active(),"21.4 °C"));
 }
 dsr_sc_appearance(DSR_SC_BLUE,3,0);
}
static void discovery_routes(void)
{
 dsr_network_home(NULL);wifi.connected=true;dsr_network_device(NULL);route("add-method");choose_method("Dodaj ręcznie (IP)");route("manual-ip");
 dsr_network_home(NULL);dsr_ui_lan_open(NULL);route("lan-scanning");test_lan_found("{\"gen\":2,\"id\":\"shellyplus2pm-atlas\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-002P16EU\",\"auth_en\":true}","192.0.2.88");dsr_ui_lan_tick();route("lan-results");press("Shelly Plus 2PM");route("lan-credentials");
 press("Sprawdź");test_lan_inspected(DSR_AUTH_REQUIRED,NULL);dsr_ui_lan_tick();route("lan-auth-error");
 dsr_network_home(NULL);dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":2,\"id\":\"shellyplus2pm-atlas\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-002P16EU\",\"auth_en\":false}","192.0.2.88");dsr_ui_lan_tick();press("Shelly Plus 2PM");test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false},\"switch:1\":{\"id\":1,\"output\":true}}");dsr_ui_lan_tick();route("lan-components");
 dsr_network_home(NULL);wifi.connected=false;dsr_ui_lan_open(NULL);route("lan-no-wifi");wifi.connected=true;dsr_network_home(NULL);
 dsr_ui_ap_open(NULL);route("ap-intro");press("Szukaj AP");route("ap-scanning");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN);response=(dsr_event_t){.scan={.count=1,.items={{.ssid="ShellyPlus1-112233445566",.security=DSR_WIFI_OPEN,.rssi=-50}}}};deliver(DSR_OK,DSR_WIFI_CONNECTED);route("ap-results");press("ShellyPlus1-112233445566");route("ap-connect-confirm");press("Połącz z AP");route("ap-inspect");test_ap_temporary(true);response=(dsr_event_t){.ap_identity=ap_identity};deliver(DSR_OK,DSR_WIFI_CONNECTED);route("ap-disclose");press("Anuluj");tick();test_ap_temporary(false);if(in_flight){response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);}dsr_ui_ap_tick();dsr_network_home(NULL);
 dsr_ui_ble_open(NULL);route("ble-intro");if(find_label(lv_screen_active(),"Szukaj"))press("Szukaj");else press("Ponów");route("ble-scanning");
 const uint8_t rpc[]={17,7,0x5f,0x44,0x49,0x5f,0x43,0x56,0x53,0x5f,0x43,0x50,0x52,0x5f,0x53,0x4f,0x6d,0x5f},blu[]={6,0xff,0xa9,0x0b,0x0b,2,0};
 test_ble_packet(rpc,sizeof(rpc),1);test_ble_packet(blu,sizeof(blu),2);test_ble_finish(DSR_BLE_DONE);tick();route("ble-results");press("Shelly BLU DoorWindow");route("ble-details");press("Wstecz");press("Ponów");press("Anuluj");tick();route("ble-cancelled");press("Ponów");test_ble_finish(DSR_BLE_DONE);tick();route("ble-empty");press("Ponów");test_ble_finish(DSR_BLE_UNAVAILABLE);tick();route("ble-unavailable");dsr_network_home(NULL);
 dsr_ui_ble_rpc_open(&rpc_candidate);route("ble-rpc-intro");press("Sprawdź BLE");route("ble-rpc-working");dsr_lan_device_t identity={.host="192.0.2.70",.id="shellyplus1-112233445566",.mac="112233445566",.model="SNSW-001X16EU",.generation=2};test_rpc_update(DSR_BLE_RPC_CONFIRM,DSR_OK,true,&identity);dsr_ui_ble_rpc_tick();route("ble-rpc-disclose");press("Anuluj");route("ble-rpc-closing");test_rpc_update(DSR_BLE_RPC_CANCELLED,DSR_OK,false,NULL);dsr_ui_ble_rpc_tick();dsr_network_home(NULL);
}
static void finish_settings(void)
{
 for(unsigned i=0;i<8;i++){
  tick();if(!in_flight)return;response=(dsr_event_t){.device=submitted.connection.device,.layout_id=submitted.connection.layout_id};
  if(submitted.kind==DSR_JOB_DEVICE_SETTINGS)dsr_device_settings_execute(&submitted,&response);
  else{CHECK(submitted.kind==DSR_JOB_PROBE);response.device.online=response.device.has_switch=true;response.result=DSR_OK;}
  deliver(response.result,DSR_WIFI_IDLE);
 }CHECK(false);
}
static void settings_routes(void)
{
 dsr_network_home(NULL);dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":3,\"id\":\"shelly1pmg3-atlas\",\"mac\":\"aabbccddeeff\",\"model\":\"S3SW-001P16EU\",\"auth_en\":false,\"name\":\"Atlas Shelly\"}","192.0.2.58");dsr_ui_lan_tick();press("Atlas Shelly");test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false}}");dsr_ui_lan_tick();press("Zapisz");CHECK(in_flight);
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_voltage=true,.voltage_v=232.1,.has_current=true,.current_a=.11,.has_energy=true,.energy_wh=1234}};response.device.online=response.device.has_switch=response.device.has_power=true;response.device.power_w=24.8;deliver(DSR_OK,DSR_WIFI_IDLE);tick();
 cJSON *doc=room_document();uint32_t id=0;for(cJSON *d=cJSON_GetObjectItemCaseSensitive(doc,"devices")->child;d;d=d->next)if(!strcmp(cJSON_GetObjectItemCaseSensitive(d,"name")->valuestring,"Atlas Shelly"))id=(uint32_t)cJSON_GetObjectItemCaseSensitive(d,"id")->valueint;cJSON_Delete(doc);CHECK(id);
 for(unsigned lang=0;lang<3;lang++){dsr_sc_appearance(DSR_SC_BLUE,3,lang);dsr_ui_usage_open(id);char name[64];snprintf(name,sizeof(name),"usage-live-%u",lang);route(name);}
 dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_ui_usage_open(id);route("usage-live");lv_tick_inc(16000);dsr_ui_usage_tick();route("usage-stale");
 test_settings_enable(true);test_settings_complete(true);
 const char *ns[]={"Sys","WiFi","BLE","Cloud","MQTT","Switch","Input","Cover","Light","RGB","RGBW","Temperature","Humidity"};
 const char *labels[]={"Ogólne","Sieć Wi-Fi","Bluetooth","Shelly Cloud","MQTT","Przekaźnik • 0","Wejście • 0","Roleta • 0","Oświetlenie • 0","RGB • 0","RGBW • 0","Temperatura • 0","Wilgotność • 0"};
 for(unsigned g=0;g<13;g++){
  dsr_ui_device_settings_open(id);finish_settings();if(!g)route("shelly-all-groups");press(labels[g]);finish_settings();char name[96];snprintf(name,sizeof(name),"shelly-%s-fields",ns[g]);route(name);
  dsr_device_settings_snapshot_t state;dsr_device_settings_snapshot(&state);unsigned types=0;
  for(unsigned i=0;i<state.field_count;i++){
   const dsr_setting_field_t *f=dsr_setting_field(state.fields[i]);if(!f||(types&(1u<<f->type)))continue;types|=1u<<f->type;
   press(f->label);snprintf(name,sizeof(name),"shelly-%s-editor-%u",ns[g],f->type);route(name);
   if(strcmp(f->path,"sta.ssid")&&strcmp(f->path,"sta1.ssid")){press("Dalej");snprintf(name,sizeof(name),"shelly-%s-confirm-%u",ns[g],f->type);route(name);}
   press("Wstecz");snprintf(name,sizeof(name),"shelly-%s-return-%u",ns[g],f->type);snap(name);
  }
 }
 test_settings_complete(false);test_settings_enable(false);dsr_network_home(NULL);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();setup();global_routes();editing_routes();lighting_routes();discovery_routes();settings_routes();
 CHECK(!job_counts[DSR_JOB_SET_OUTPUT]&&!job_counts[DSR_JOB_OTA_INSTALL]&&!restart_requests);
 printf("PASS: %u full atlas checks; %u genuine LVGL frames, including all scroll viewports; host providers only\n",checks,screenshots);lv_deinit();return 0;
}
