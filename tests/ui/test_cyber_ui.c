/* Production LVGL 9.3 widgets and screen code. Radio, storage and ADC MODEL. */
#define _POSIX_C_SOURCE 200809L
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_cyber.h"
#include "../../components/dsr_ui/dsr_ui_brand.h"
void test_native_ota_progress(size_t received,size_t total);

static unsigned rgb(lv_color_t c){return lv_color_to_u32(c)&0xffffff;}
static lv_obj_t *theme_choice(void)
{unsigned skip=1;lv_obj_t *o=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);CHECK(o&&lv_dropdown_get_option_count(o)==4);return o;}
static void select_skin(unsigned s)
{
 dsr_ui_open_settings();tap_row("Wygląd i język");lv_obj_t *o=theme_choice();
 CHECK(strstr(lv_dropdown_get_options(o),"Shelly HUD — Cyber Blue"));
 lv_dropdown_set_selected(o,s);lv_obj_send_event(o,LV_EVENT_VALUE_CHANGED,NULL);click("Zapisz");tick();
 CHECK(dsr_sc_style()==s&&style_hint==s);
 cJSON *doc=room_document();CHECK(dsr_sc_style_from_name(cJSON_GetObjectItemCaseSensitive(doc,"theme")->valuestring)==s);cJSON_Delete(doc);
}
static unsigned hits,cover_hits,stop_hits,position_hits,last_position;
static void output_hit(unsigned index,bool on){CHECK(index==0&&!on);hits++;}
static void cover_hit(unsigned index,int direction){CHECK(index==1&&(direction==1||direction==-1));cover_hits++;}
static void stop_hit(unsigned index){CHECK(index==1);stop_hits++;}
static void position_hit(unsigned index,uint8_t position){CHECK(index==1&&position<=100);position_hits++;last_position=position;}
static lv_obj_t *sized(lv_obj_t *o,int w,int h)
{if(lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h)return o;for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *found=sized(lv_obj_get_child(o,(int32_t)i),w,h);if(found)return found;}return NULL;}
static void gauge_pointer(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();}
static double full_paint_ms(void)
{
 struct timespec a,b;CHECK(clock_gettime(CLOCK_MONOTONIC,&a)==0);
 for(unsigned i=0;i<8;i++){lv_obj_invalidate(lv_screen_active());lv_refr_now(display);}
 CHECK(clock_gettime(CLOCK_MONOTONIC,&b)==0);return ((b.tv_sec-a.tv_sec)*1000.0+(b.tv_nsec-a.tv_nsec)/1000000.0)/8;
}
static void gallery(void)
{
 dsr_ui_workspace_hide(); /* Same editor exit as real navigation, before direct fixture rendering. */
 dsr_mobile_model_t m={.wifi=true,.device_count=4,.room_count=1,.home_name="Mój dom",.dashboard_name="Mój pulpit",
  .rooms={{.name="Salon",.id=1}},.devices={
  {.id=1,.name="Shelly Plus 1PM",.kind=DSR_DEVICE_RELAY,.online=true,.output=true,.can_switch=true,.has_power=true,.has_voltage=true,.power_w=18.6,.voltage_v=232.9},
  {.id=2,.name="SPSH-002PE16EU",.kind=DSR_DEVICE_COVER,.online=true,.has_position=true,.position_pct=84,.can_cover=true,.can_stop_cover=true,.cover_state=DSR_COVER_STOPPED,.has_power=true,.has_voltage=true,.power_w=0,.voltage_v=232.9},
  {.id=3,.name="BLU Window",.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_contact=true,.has_battery=true,.battery_pct=92,.has_illuminance=true,.illuminance_lux=18,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN}},
  {.id=4,.name="Shelly Plug S",.kind=DSR_DEVICE_PLUG,.online=false}}};
 m.devices[1].can_position=true;
 m.devices[2].has_sample_time=true;m.devices[2].sample_ms=lv_tick_get();m.devices[2].blu_age_s=4;
 const dsr_mobile_actions_t a={.set_output=output_hit,.set_cover=cover_hit,.stop_cover=stop_hit,.set_position=position_hit};
 for(unsigned i=0;i<m.device_count;i++)snprintf(m.devices[i].identity,sizeof(m.devices[i].identity),"skin-fixture-%u",i);
 dsr_mobile_bind(&a);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);screenshot("600-cyber-home");
 m.devices[0].output=false;dsr_mobile_update(&m);screenshot("624-cyber-home-off");m.devices[0].output=true;dsr_mobile_update(&m);
 dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_t *power=power_target(lv_screen_active());CHECK(power);lv_obj_update_layout(lv_screen_active());lv_area_t pa;lv_obj_get_coords(power,&pa);tap((pa.x1+pa.x2)/2,(pa.y1+pa.y2)/2);CHECK(hits==1);screenshot("601-cyber-relay");
 dsr_mobile_show(DSR_MOBILE_DETAILS,1);CHECK(find_label(lv_screen_active(),"Pozycja 84%"));screenshot("602-cyber-cover");
 lv_obj_update_layout(lv_screen_active());lv_obj_t *rail=sized(lv_screen_active(),136,182);CHECK(rail);
 lv_area_t well;lv_obj_get_coords(lv_obj_get_child(rail,0),&well);
 const unsigned targets[]={0,50,100};
 for(unsigned i=0;i<3;i++){
  int y=well.y2-3-(int)targets[i]*(lv_area_get_height(&well)-6)/100;
  gauge_pointer((well.x1+well.x2)/2,y,true);CHECK(position_hits==i);
  gauge_pointer((well.x1+well.x2)/2,y,false);CHECK(position_hits==i+1&&last_position==targets[i]);
  CHECK(find_label(lv_screen_active(),"Pozycja 84%")); /* Target never replaces confirmed data. */
 }
 tap(80,390);tap(235,390);tap(272,275);CHECK(cover_hits==2&&stop_hits==1);
 dsr_mobile_show(DSR_MOBILE_DETAILS,2);CHECK(find_label(lv_screen_active(),"Zamknięte"));screenshot("603-cyber-blu-closed");
 m.devices[2].contact_open=true;m.devices[2].blu.flags|=DSR_BLU_OPEN;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Otwarte"));screenshot("604-cyber-blu-open");
 for(unsigned lang=1;lang<=2;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,lang);dsr_ui_apply_theme();
  m.devices[2].blu.flags&=~DSR_BLU_OPEN;m.devices[2].contact_open=false;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,2);
  lv_obj_t *status=find_label(lv_screen_active(),dsr_sc_tr("Zamknięte"));CHECK(status);lv_obj_update_layout(status);CHECK(lv_obj_get_height(status)<=36);
  screenshot(lang==1?"620-cyber-blu-en":"621-cyber-blu-de");
 }
 dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,0);dsr_ui_apply_theme();
 m.configured_layout=true;m.tile_count=24;
 for(unsigned i=0;i<24;i++)m.tiles[i]=(dsr_mobile_tile_t){.type=0,.index=i%4,.span=2,.height=1,.fields=255};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_refr_now(display);
 lv_mem_monitor_t stress;lv_mem_monitor(&stress);CHECK(stress.free_size>4096);CHECK(stress.free_biggest_size>4096);
 printf("Cyber Blue 24-tile stress: %lu bytes free, largest %lu, 128 KiB LVGL pool; no physical timing measurement\n",(unsigned long)stress.free_size,(unsigned long)stress.free_biggest_size);
 double premium=full_paint_ms();dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_GRAPHITE,0);dsr_ui_apply_theme();dsr_mobile_show(DSR_MOBILE_HOME,0);double graphite=full_paint_ms();
 printf("Full 320x480 repaint, same 24-tile fixture, host mean of 8: Premium %.2f ms, Graphite %.2f ms; not ESP32 latency\n",premium,graphite);
 dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,0);dsr_ui_apply_theme();dsr_mobile_show(DSR_MOBILE_HOME,0);
 screenshot("615-cyber-24-tiles");
 dsr_ui_open_settings();screenshot("605-cyber-settings");
 test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_MANIFEST,DSR_MANIFEST_INVALID);dsr_ui_native_ota_open(NULL);screenshot("606-cyber-firmware-error");
 dsr_ui_battery_open(NULL);dsr_event_t battery={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3800}};dsr_ui_battery_result(&battery);CHECK(find_label(lv_screen_active(),"3.800 V"));screenshot("607-cyber-diagnostics");
 strcpy(wifi.ssid,"Home Wi-Fi");strcpy(wifi.ip,"192.168.1.104");dsr_ui_settings_network(NULL);screenshot("616-cyber-network-settings");
 dsr_network_wifi(NULL);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_WIFI_SCAN);
 response.scan=(dsr_wifi_scan_t){.count=4,.items={{.ssid="Home Wi-Fi",.rssi=-54,.security=DSR_WIFI_WPA2},{.ssid="ESP32 Lab",.rssi=-69,.security=DSR_WIFI_WPA2},{.ssid="Guest network",.rssi=-84,.security=DSR_WIFI_WPA2},{.ssid="Workshop",.rssi=-88,.security=DSR_WIFI_WPA2}}};deliver(DSR_OK,DSR_WIFI_IDLE);screenshot("617-cyber-wifi-networks");
 tap(150,145);CHECK(find_type(lv_screen_active(),&lv_textarea_class));CHECK(find_type(lv_screen_active(),&lv_keyboard_class));screenshot("618-cyber-wifi-password");
 test_native_ota_set(DSR_NATIVE_INSTALLING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_progress(500,1000);dsr_ui_native_ota_open(NULL);lv_obj_t *bar=find_type(lv_screen_active(),&lv_bar_class);CHECK(bar&&lv_bar_get_value(bar)==500);screenshot("619-cyber-firmware-progress");test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_INVALID);
 dsr_network_device(NULL);screenshot("608-cyber-onboarding");
 choose_method("Dodaj ręcznie (IP)");screenshot("609-cyber-form");
 dsr_network_rooms();dsr_ui_edit_room(1);screenshot("610-cyber-room-editor");
 room_delete_button();CHECK(find_label(lv_screen_active(),"Usunąć pomieszczenie?"));screenshot("611-cyber-delete-confirmation");click("Anuluj");
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb&&!lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));
 CHECK(rgb(lv_obj_get_style_bg_color(kb,LV_PART_ITEMS))==DSR_CYBER_ELEVATED);screenshot("612-cyber-keyboard");key(kb,"Gotowe");
 dsr_ui_open_settings();tap_row("Wygląd i język");lv_obj_t *d=theme_choice();lv_dropdown_open(d);lv_obj_t *list=lv_dropdown_get_list(d);
 CHECK(rgb(lv_obj_get_style_bg_color(list,0))==DSR_CYBER_ELEVATED);screenshot("613-cyber-dropdown");lv_dropdown_close(d);
 dsr_ui_open_settings();lv_obj_t *msg=lv_msgbox_create(NULL);lv_msgbox_add_title(msg,"Błąd połączenia");lv_msgbox_add_text(msg,"Spróbuj ponownie.");lv_msgbox_add_footer_button(msg,"OK");
 CHECK(rgb(lv_obj_get_style_bg_color(msg,0))==DSR_CYBER_ELEVATED);
 CHECK(lv_obj_get_style_text_font(lv_msgbox_get_title(msg),0)==&dsr_roboto_14);
 CHECK(rgb(lv_obj_get_style_bg_color(lv_msgbox_get_header(msg),0))==DSR_CYBER_ELEVATED);screenshot("614-cyber-native-message");lv_msgbox_close(msg);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();
 cJSON *doc=room_document();cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(doc,"rooms"),cJSON_Parse("{\"id\":1,\"name\":\"Salon\",\"icon\":0,\"photo\":\"\"}"));publish_document(doc);dsr_ui_init();tick();
 const void *background=dsr_ui_brand_background();
 /* Editing and canceling cannot change the active or saved appearance. */
 dsr_ui_open_settings();tap_row("Wygląd i język");lv_obj_t *d=theme_choice();lv_dropdown_set_selected(d,3);lv_obj_send_event(d,LV_EVENT_VALUE_CHANGED,NULL);
 CHECK(dsr_sc_style()==DSR_SC_STYLE_GRAPHITE);click("Wstecz");click("Odrzuć");CHECK(dsr_sc_style()==DSR_SC_STYLE_GRAPHITE);
 lv_mem_monitor_t legacy,first;lv_mem_monitor(&legacy);
 select_skin(3);lv_mem_monitor(&first);
 printf("LVGL free before first Cyber selection %lu, after %lu (same appearance screen; includes lazy widget/style caches)\n",(unsigned long)legacy.free_size,(unsigned long)first.free_size);
 CHECK(rgb(dsr_sc_color(DSR_SC_BG))==DSR_CYBER_BG&&rgb(dsr_sc_color(DSR_SC_BLUE))==DSR_CYBER_BLUE);
 CHECK(dsr_ui_brand_background()==background); /* Reuse original dark art. */
 gallery();
 /* Re-enter from the saved document, as startup does (no device reboot). */
 dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_LIGHT,0);dsr_ui_apply_theme();dsr_network_home(NULL);tick();CHECK(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE);
 /* Fill LVGL's lazy style/font caches for both directions before measuring. */
 for(unsigned warm=0;warm<3;warm++)for(unsigned i=0;i<4;i++)select_skin(i);
 /* Compare quiescent empty roots: retained allocation slack in a live screen
  * varies with TLSF fragmentation when the parent LVGL theme is rebuilt. */
 lv_obj_clean(dsr_ui_screen("Memory checkpoint",false));lv_refr_now(display);
 lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(unsigned cycle=0;cycle<20;cycle++)for(unsigned s=0;s<4;s++)select_skin(s);
 lv_obj_clean(dsr_ui_screen("Memory checkpoint",false));lv_refr_now(display);
 lv_mem_monitor(&b);fprintf(stderr,"skin cycle memory: %lu -> %lu, largest %lu\n",(unsigned long)a.free_size,(unsigned long)b.free_size,(unsigned long)b.free_biggest_size);CHECK(b.free_size>=a.free_size);CHECK(b.free_biggest_size>32000);
 printf("Cyber Blue LVGL heap after warmup: %lu -> %lu bytes free; largest %lu; 80 saved skin switches, no growth; 128 KiB pool\n",(unsigned long)a.free_size,(unsigned long)b.free_size,(unsigned long)b.free_biggest_size);
 select_skin(0);CHECK(dsr_ui_brand_background()==background&&rgb(dsr_sc_color(DSR_SC_BG))==DSR_SC_BG);
 CHECK(!job_counts[DSR_JOB_SET_OUTPUT]&&!job_counts[DSR_JOB_COVER]&&!restart_requests);
 printf("PASS: %u Cyber Blue checks; %u real LVGL 320x480 frames; selection/save/cancel/reload, four skins, dialogs/keyboard/dropdown, pointer callback; storage/radio MODEL\n",checks,screenshots);
 lv_deinit();return 0;
}
