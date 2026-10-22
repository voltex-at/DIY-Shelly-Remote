/* Owner photo/video regressions. Real LVGL/pointer; SDK/radio/Flash MODEL. */
#define main original_ui_main
#include "test_ui.c"
#undef main
void test_native_ota_progress(size_t received,size_t total);
static void sample(void)
{response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
static void probe(void)
{lv_tick_inc(5100);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);}
static unsigned labels(lv_obj_t *o,const char *text)
{unsigned n=lv_obj_check_type(o,&lv_label_class)&&!strcmp(lv_label_get_text(o),dsr_sc_tr(text));for(unsigned i=0;i<lv_obj_get_child_count(o);i++)n+=labels(lv_obj_get_child(o,(int)i),text);return n;}
static void red_button(const char *text)
{lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label);lv_obj_t *b=lv_obj_get_parent(label);CHECK(lv_color_eq(lv_obj_get_style_bg_color(b,0),lv_color_hex(0xb42332)));}
static unsigned dial_writes;static uint8_t dial_target;
static void dial_set(unsigned i,uint8_t half){CHECK(i==0);dial_writes++;dial_target=half;}
static void dial_pointer(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(30);lv_indev_read(pointer);lv_timer_handler();}
static void repaint_regression(void)
{
 dsr_mobile_model_t m={.device_count=2};
 for(unsigned i=0;i<2;i++){m.devices[i]=(dsr_mobile_device_t){.id=42+i,.kind=DSR_DEVICE_RELAY,.online=true,.can_switch=true,.has_power=true,.power_w=10};snprintf(m.devices[i].name,48,"Latency relay %u",i);strcpy(m.devices[i].identity,"latency-fixture");}
 dsr_mobile_bind(NULL);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_update_layout(lv_screen_active());tap(286,72);
 lv_obj_t *close=find_label(lv_screen_active(),"Zamknij");CHECK(close);lv_area_t a;lv_obj_get_coords(close,&a);dial_pointer((a.x1+a.x2)/2,(a.y1+a.y2)/2,true);
 m.devices[1].power_w=11;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Zamknij")==close);
 dial_pointer((a.x1+a.x2)/2,(a.y1+a.y2)/2,false);CHECK(!find_label(lv_screen_active(),"Zamknij"));
 dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_t *name=find_label(lv_screen_active(),"Latency relay 0");CHECK(name);
 m.devices[1].power_w=12;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Latency relay 0")==name);
 dsr_ui_battery_show();lv_obj_t *battery=find_label(lv_screen_active(),"Napięcie wejścia BAT");m.devices[1].power_w=13;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Napięcie wejścia BAT")==battery&&!find_label(lv_screen_active(),"Latency relay 0"));dsr_ui_open_settings();
}
static bool has_lux(lv_obj_t *o)
{if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o)," lx"))return true;for(unsigned i=0;i<lv_obj_get_child_count(o);i++)if(has_lux(lv_obj_get_child(o,(int)i)))return true;return false;}
static unsigned sleeping_cover_draws;
static void count_sleeping_cover(lv_event_t *event){(void)event;sleeping_cover_draws++;}
static bool sleep_has_text(lv_obj_t *o,const char *text)
{
 if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o),text))return true;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)if(sleep_has_text(lv_obj_get_child(o,(int)i),text))return true;
 return false;
}
static lv_obj_t *cover_well(lv_obj_t *o)
{
 if(lv_obj_get_width(o)==176&&lv_obj_get_height(o)==154&&!lv_obj_get_child_count(o))return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *well=cover_well(lv_obj_get_child(o,(int)i));if(well)return well;}return NULL;
}
static void sleep_repaint_regression(void)
{
 unsigned prior_style=dsr_sc_style(),prior_language=dsr_sc_language();
 dsr_ui_open_settings();dsr_mobile_bind(NULL);
 dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,0);dsr_ui_apply_theme();
 dsr_mobile_model_t m={.wifi=true,.device_count=1,.devices={{.id=72,.name="Sleeping relay",.identity="sleep-fixture",.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true,.has_sample_time=true,.has_power=true,.power_w=10,.fields=1}}};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DEVICES,0);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *name=find_label(lv_screen_active(),"Sleeping relay");CHECK(name);lv_obj_t *card=lv_obj_get_parent(name);
 display_sleeping=true;m.devices[0].output=true;m.devices[0].power_w=20;dsr_mobile_update(&m);
 CHECK(find_label(lv_screen_active(),"Sleeping relay")==name&&lv_obj_is_valid(card));
 CHECK(!sleep_has_text(lv_screen_active(),"20.0 W"));
 m.devices[0].power_w=30;dsr_mobile_update(&m);lv_tick_inc(100);lv_timer_handler();
 CHECK(find_label(lv_screen_active(),"Sleeping relay")==name&&!sleep_has_text(lv_screen_active(),"30.0 W"));
 display_sleeping=false;lv_tick_inc(40);lv_timer_handler();
 CHECK(sleep_has_text(lv_screen_active(),"30.0 W")&&!sleep_has_text(lv_screen_active(),"20.0 W"));
 m.devices[0].sample_ms=lv_tick_get();dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_t *age=find_label(lv_screen_active(),"Odczyt potwierdzony 0 s temu");CHECK(age);
 display_sleeping=true;lv_tick_inc(2100);dsr_mobile_update(&m);
 CHECK(find_label(lv_screen_active(),"Odczyt potwierdzony 0 s temu")==age);
 m.devices[0].sample_ms=lv_tick_get();dsr_mobile_update(&m);
 display_sleeping=false;lv_tick_inc(40);lv_timer_handler();CHECK(find_label(lv_screen_active(),"Odczyt potwierdzony 0 s temu"));
 m.devices[0]=(dsr_mobile_device_t){.id=73,.name="Sleeping cover",.identity="sleep-cover",.kind=DSR_DEVICE_COVER,.endpoint_kind=DSR_ENDPOINT_COVER,.online=true,.has_position=true,.position_pct=20,.cover_state=DSR_COVER_STOPPED};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);
 m.devices[0].cover_state=DSR_COVER_OPENING;m.devices[0].position_pct=80;dsr_mobile_update(&m);
 lv_obj_update_layout(lv_screen_active());lv_obj_t *well=cover_well(lv_screen_active());CHECK(well);
 lv_obj_add_event_cb(well,count_sleeping_cover,LV_EVENT_DRAW_MAIN,NULL);lv_refr_now(display);
 unsigned draws=sleeping_cover_draws;display_sleeping=true;lv_tick_inc(120);lv_timer_handler();lv_refr_now(display);
 CHECK(sleeping_cover_draws==draws);
 m.devices[0].position_pct=90;dsr_mobile_update(&m);CHECK(cover_well(lv_screen_active())==well);
 display_sleeping=false;lv_tick_inc(40);lv_timer_handler();CHECK(find_label(lv_screen_active(),"Pozycja 90%"));
 dsr_ui_open_settings();
 dsr_sc_appearance(DSR_SC_BLUE,prior_style,prior_language);dsr_ui_apply_theme();
}
static void thermostat(void)
{
 dsr_mobile_model_t m={.device_count=1};m.devices[0]=(dsr_mobile_device_t){.id=42,.kind=DSR_DEVICE_SENSOR,.endpoint_kind=DSR_ENDPOINT_BLU,.generation=DSR_GEN2_PLUS,.online=true,.has_trv_target=true,.trv_target_c=20,.has_temperature=true,.temperature_c=21.5,.has_battery=true,.battery_pct=90,.blu={.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET,.trv_pos=28}};
 strcpy(m.devices[0].name,"Thermostat");strcpy(m.devices[0].identity,"paired-blu");strcpy(m.devices[0].host,"192.168.1.51");
 const dsr_mobile_actions_t a={.set_trv=dial_set};dsr_mobile_bind(&a);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());
 dsr_mobile_show(DSR_MOBILE_DEVICE_SETTINGS,0);CHECK(!has_lux(lv_screen_active()));dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());
 CHECK(has_icon(lv_screen_active(),DSR_ICON_VALVE,26)&&has_icon(lv_screen_active(),DSR_ICON_BATTERY,24));
 CHECK(find_type(lv_screen_active(),&lv_arc_class)&&find_label(lv_screen_active(),"Nastawa")&&!find_label(lv_screen_active(),"AKTUALNA\nTEMPERATURA"));
 dial_pointer(160,140,true);dial_pointer(244,222,true);lv_tick_inc(2000);lv_timer_handler();CHECK(dial_writes==0); /* held pointer never dispatches */
 dial_pointer(244,222,false);lv_tick_inc(1000);lv_timer_handler();CHECK(dial_writes==0);lv_tick_inc(300);lv_timer_handler();CHECK(dial_writes==1&&dial_target>=50&&dial_target<=52);screenshot("503-owner-thermostat-drag");
 /* Unsaved taps cannot execute or paint over the next settings screen. */
 click("+");dsr_ui_open_settings();lv_obj_update_layout(lv_screen_active());lv_tick_inc(2000);lv_timer_handler();CHECK(dial_writes==1&&find_label(lv_screen_active(),"Ustawienia pilota")&&!find_label(lv_screen_active(),"Thermostat"));
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());dial_pointer(160,140,true);dial_pointer(244,222,true);m.devices[0].trv_target_c=21;dsr_mobile_update(&m);dial_pointer(244,222,false);lv_tick_inc(1400);lv_timer_handler();CHECK(dial_writes==1); /* external target invalidates the draft */
 dsr_ui_open_settings();
}
static void ota_buttons_visible(void)
{
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);
  test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_MANIFEST,DSR_MANIFEST_POLICY);dsr_ui_native_ota_open(NULL);lv_obj_update_layout(lv_screen_active());
  lv_obj_t *check=lv_obj_get_parent(find_label(lv_screen_active(),"Sprawdź aktualizacje")),*channel=lv_obj_get_parent(find_label(lv_screen_active(),"Zmień kanał"));
  lv_area_t a,b,viewport,back;lv_obj_get_coords(check,&a);lv_obj_get_coords(channel,&b);lv_obj_get_coords(lv_obj_get_parent(channel),&viewport);lv_obj_get_coords(lv_obj_get_parent(find_label(lv_screen_active(),"Wstecz")),&back);
  CHECK(a.y1>=viewport.y1&&a.y2<viewport.y2&&b.y1>a.y2&&b.y2<viewport.y2&&b.y2<back.y1);
  lv_area_t web;lv_obj_get_coords(lv_obj_get_parent(find_label(lv_screen_active(),"Zezwól na OTA • 2 minuty")),&web);CHECK(web.y1>viewport.y2);
  screenshot(language==0?"507-owner-ota-buttons-pl":language==1?"508-owner-ota-buttons-en":"509-owner-ota-buttons-de");
  tap(b.x1+24,b.y2-4);CHECK(find_label(lv_screen_active(),"Wybierz kanał aktualizacji"));
 }
 dsr_ui_open_settings();
}
static void sensor_states(void)
{
 dsr_mobile_model_t m={.device_count=1};m.devices[0]=(dsr_mobile_device_t){.id=42,.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_contact=true,.has_illuminance=true,.illuminance_lux=18,.has_battery=true,.battery_pct=100,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN}};
 strcpy(m.devices[0].name,"BLU contact");strcpy(m.devices[0].identity,"contact-fixture");
 dsr_mobile_bind(NULL);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);CHECK(has_icon(lv_screen_active(),DSR_ICON_WINDOW_CLOSED,64)&&find_label(lv_screen_active(),"Zamknięte"));screenshot("504-owner-window-closed");
 m.devices[0].contact_open=true;m.devices[0].blu.flags|=DSR_BLU_OPEN;dsr_mobile_update(&m);CHECK(has_icon(lv_screen_active(),DSR_ICON_WINDOW_OPEN,64)&&find_label(lv_screen_active(),"Otwarte"));screenshot("505-owner-window-open");
 m.devices[0].icon=8;dsr_mobile_update(&m);CHECK(has_icon(lv_screen_active(),DSR_ICON_DOOR_OPEN,64));screenshot("506-owner-door-open");
 m.devices[0].contact_open=false;m.devices[0].blu.flags&=(uint8_t)~DSR_BLU_OPEN;dsr_mobile_update(&m);CHECK(has_icon(lv_screen_active(),DSR_ICON_DOOR_CLOSED,64));
 dsr_mobile_show(DSR_MOBILE_DEVICE_SETTINGS,0);CHECK(has_lux(lv_screen_active())); /* Real Window illumination must survive the TRV fix. */
 dsr_mobile_show(DSR_MOBILE_DETAILS,0);m.devices[0].online=false;dsr_mobile_update(&m);CHECK(!has_icon(lv_screen_active(),DSR_ICON_DOOR_CLOSED,64)&&!find_label(lv_screen_active(),"Zamknięte"));dsr_ui_open_settings();
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();
 cJSON *doc=room_document();cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(doc,"rooms"),cJSON_Parse("{\"id\":1,\"name\":\"Salon\",\"icon\":0,\"photo\":\"\"}"));
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(doc,"devices"),cJSON_Parse("{\"id\":42,\"name\":\"Owner relay\",\"kind\":\"relay\",\"host\":\"192.168.1.51\",\"channel\":0,\"room\":1}"));
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"dashboards")->child,"tiles"),cJSON_Parse("{\"id\":42,\"type\":\"device\",\"ref\":42,\"span\":2,\"height\":1,\"fields\":255}"));publish_document(doc);
 boot_loaded[0]=(dsr_connection_t){.layout_id=42,.device={.host="192.168.1.51",.name="Owner relay",.device_id="shelly-owner",.generation=DSR_GEN2_PLUS}};boot_loaded_count=1;
 dsr_ui_init();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);
 /* Real bottom Battery target on Settings, including a tap near its left edge. */
 dsr_ui_open_settings();lv_obj_update_layout(lv_screen_active());tap(134,452);CHECK(find_label(lv_screen_active(),"Napięcie wejścia BAT"));
 unsigned before=requests;click("Zmierz napięcie");CHECK(requests==before&&submitted.kind==DSR_JOB_PROBE&&find_label(lv_screen_active(),"Odczyt napięcia…")&&!find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));
 CHECK(!dsr_activity_restart_reserve());next_submit_result=DSR_BUSY;sample();CHECK(!in_flight&&dsr_network_busy()&&requests==before);CHECK(!dsr_activity_restart_reserve());tick();CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ&&requests==before+1);
 response=(dsr_event_t){.battery={.has_voltage=true,.bat_mv=3980}};deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"3.980 V"));screenshot("500-owner-battery");
 /* The settings row is independently reachable by an actual pointer. */
 click("Wstecz");tap_row("Bateria i zasilanie");CHECK(find_label(lv_screen_active(),"Napięcie wejścia BAT"));click("Wstecz");
 probe();before=requests;CHECK(dsr_network_request_display(80,120)==DSR_OK&&requests==before);sample();CHECK(in_flight&&submitted.kind==DSR_JOB_DISPLAY_SAVE&&submitted.settings.brightness==80&&submitted.settings.idle_seconds==120);
 response=(dsr_event_t){.brightness=80,.idle_seconds=120};deliver(DSR_OK,DSR_WIFI_IDLE);
 probe();before=requests;test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_INVALID);dsr_ui_native_ota_open(NULL);tap_row("Sprawdź aktualizacje");CHECK(dsr_network_ota_busy()&&requests==before&&submitted.kind==DSR_JOB_PROBE);
 CHECK(dsr_network_request_wifi(DSR_JOB_WIFI_SCAN,NULL)==DSR_BUSY);sample();CHECK(in_flight&&submitted.kind==DSR_JOB_OTA_CHECK&&requests==before+1);
 test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_NO_RELEASE,DSR_MANIFEST_INVALID);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);
 /* Progress is real bytes, starts at zero; full Cancel target stays above Back. */
 test_native_ota_set(DSR_NATIVE_INSTALLING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_progress(0,1000);dsr_ui_native_ota_open(NULL);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *bar=find_type(lv_screen_active(),&lv_bar_class);CHECK(bar&&lv_bar_get_value(bar)==0);lv_area_t cancel,back;lv_obj_get_coords(lv_obj_get_parent(find_label(lv_screen_active(),"Anuluj")),&cancel);lv_obj_get_coords(lv_obj_get_parent(find_label(lv_screen_active(),"Wstecz")),&back);CHECK(cancel.y1==360&&cancel.y2<back.y1&&cancel.y2<420);
 test_native_ota_progress(500,1000);dsr_ui_native_ota_tick();CHECK(lv_bar_get_value(bar)==500);screenshot("501-owner-ota-half");test_native_ota_progress(1000,1000);dsr_ui_native_ota_tick();CHECK(lv_bar_get_value(bar)==1000);
 test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_INVALID);wifi.connected=false;tick();
 /* Graphite and Light share this compact layout; Cyber Blue has its own HUD
  * battery screen and is covered by the Cyber suites. */
 for(unsigned language=0;language<3;language++)for(unsigned theme=0;theme<2;theme++){
  doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==0?"graphite":"light"));publish_document(doc);dsr_network_home(NULL);lv_obj_update_layout(lv_screen_active());CHECK(!labels(lv_screen_active(),"Mój dom")&&labels(lv_screen_active(),"Mój pulpit")==1);
  lv_obj_t *power=NULL;for(unsigned i=0;i<lv_obj_get_child_count(lv_screen_active());i++){lv_obj_t *o=lv_obj_get_child(lv_screen_active(),(int)i);for(unsigned j=0;j<lv_obj_get_child_count(o);j++){lv_obj_t *c=lv_obj_get_child(o,(int)j);for(unsigned k=0;k<lv_obj_get_child_count(c);k++){lv_obj_t *b=lv_obj_get_child(c,(int)k);if(lv_obj_get_width(b)==58&&lv_obj_get_height(b)==58&&lv_obj_has_flag(b,LV_OBJ_FLAG_CLICKABLE))power=b;}}}CHECK(power);lv_area_t p,c;lv_obj_get_coords(power,&p);lv_obj_get_coords(lv_obj_get_parent(power),&c);CHECK(p.y1>=c.y1+24&&p.y2<=c.y2-4&&p.x2<=c.x2-8);
  dsr_ui_edit_room(1);red_button("Usuń pomieszczenie");dsr_ui_open_settings();dsr_ui_config_registry(NULL);click("Urządzenia");click("Owner relay");red_button("Usuń urządzenie");
  dsr_ui_open_settings();lv_obj_update_layout(lv_screen_active());tap(160,452);CHECK(find_label(lv_screen_active(),"Napięcie wejścia BAT"));
  dsr_onboarding_screen("Bluetooth LE",2);lv_obj_update_layout(lv_screen_active());char caption[40];snprintf(caption,sizeof(caption),dsr_sc_tr("Krok %u z 3"),2u);lv_obj_t *step=find_label(lv_screen_active(),caption);CHECK(step&&lv_obj_get_y(step)>=8&&lv_obj_get_y(step)+lv_obj_get_height(step)<=32);CHECK(!labels(lv_screen_active(),"Mój dom"));tap(52,20);CHECK(dsr_mobile_active());
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,2);CHECK(!strcmp(dsr_sc_tr("Paruj BLU"),"BLU koppeln"));CHECK(!strstr(dsr_sc_tr("Sparuj czujnik, ustaw jego parametry i wybierz bramkę Shelly."),"Sparuj"));
 lv_obj_t *root=dsr_ui_screen("Scroll test",false),*list=dsr_sc_panel(root,12,86,296,318,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);dsr_sc_scrollbar(list);dsr_sc_panel(list,0,0,296,900,DSR_SC_CARD,8);lv_obj_update_layout(root);lv_area_t h,v;lv_obj_get_scrollbar_area(list,&h,&v);CHECK(lv_area_get_width(&v)==2&&lv_area_get_height(&v)==36&&v.x1>307&&v.x2<320);lv_obj_invalidate(root);lv_refr_now(display);unsigned pixel=((unsigned)((v.y1+v.y2)/2)*320u+(unsigned)v.x1)*3u;CHECK(canvas[pixel]>60&&canvas[pixel+1]>60&&canvas[pixel+2]>60);screenshot("502-owner-scrollbar");
 unsigned hits=ready_count;dsr_sc_button(root,12,86,36,36,DSR_SC_CARD,8,ready,NULL);lv_obj_update_layout(root);tap(9,104);CHECK(ready_count==hits+1);
 repaint_regression();sleep_repaint_regression();thermostat();sensor_states();ota_buttons_visible();
 CHECK(job_counts[DSR_JOB_SET_OUTPUT]==0&&job_counts[DSR_JOB_COVER]==0);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: %u owner UX LCD checks; %u frames; genuine LVGL/pointer; SDK/transport MODEL\n",checks,screenshots);lv_deinit();return 0;
}
