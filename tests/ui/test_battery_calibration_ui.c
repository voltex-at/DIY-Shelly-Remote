/* Real 128 KiB LVGL, production form/estimator, explicit worker/ADC/NVS models. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_cyber.h"
static lv_area_t box(lv_obj_t *o)
{CHECK(o);lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(o,&a);return a;}
static void press(const char *caption)
{
 lv_obj_t *l=find_label(lv_screen_active(),caption);CHECK(l);lv_obj_t *b=lv_obj_get_parent(l);lv_obj_scroll_to_view_recursive(b,LV_ANIM_OFF);lv_area_t a=box(b);CHECK(a.y1>=0&&a.y2<480);CHECK(lv_obj_get_height(b)>=44);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static lv_obj_t *field(unsigned index)
{lv_obj_t *o=nth_type(lv_screen_active(),&lv_textarea_class,&index);CHECK(o);return o;}
static void set(unsigned i,const char *value){lv_textarea_set_text(field(i),value);}
static void snap(const char *stage,unsigned lang)
{char name[120];snprintf(name,sizeof(name),"battery-calibration-%s-%s",stage,lang==0?"pl":lang==1?"en":"de");screenshot(name);}
static void audit(lv_obj_t *o)
{
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return;
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&lv_obj_get_style_border_width(o,0)>0)CHECK(lv_obj_get_style_border_width(o,0)==2);
 if(lv_obj_check_type(o,&lv_label_class)){
  lv_obj_t *p=lv_obj_get_parent(o);
  if(p&&!lv_obj_has_flag(p,LV_OBJ_FLAG_SCROLLABLE)&&p!=lv_screen_active()){
   CHECK(lv_obj_get_y(o)>=0);CHECK(lv_obj_get_y(o)+lv_obj_get_height(o)<=lv_obj_get_height(p));
  }
 }
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)audit(lv_obj_get_child(o,(int)i));
}
static void profile_reply(dsr_result_t result)
{
 CHECK(in_flight&&(submitted.kind==DSR_JOB_BATTERY_PROFILE_SAVE||submitted.kind==DSR_JOB_BATTERY_PROFILE_LOAD));
 if(result==DSR_OK&&submitted.kind==DSR_JOB_BATTERY_PROFILE_SAVE)battery_stored=submitted.battery_profile;
 response=(dsr_event_t){.battery_profile=battery_stored};deliver(result,DSR_WIFI_IDLE);
}
static void reset_profile(void)
{
 if(in_flight){CHECK(submitted.kind==DSR_JOB_BATTERY_READ);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);}
 dsr_ui_open_settings();dsr_battery_profile_defaults(&battery_stored);battery_load_failure=false;dsr_ui_battery_init();
}
static void form(unsigned lang,unsigned theme)
{
 reset_profile();dsr_sc_appearance(DSR_SC_BLUE,theme,lang);dsr_ui_battery_open(NULL);press("Kalibracja baterii");
 CHECK(dsr_ui_battery_calibration_active());CHECK(!in_flight);CHECK(!strcmp(lv_textarea_get_text(field(0)),"3700"));
 snap("initial",lang);audit(lv_screen_active());
 lv_obj_t *t=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(t&&lv_dropdown_get_selected(t)==0);lv_dropdown_set_selected(t,DSR_BATTERY_LIPO_1S);
 lv_obj_t *trend_toggle=find_type(lv_screen_active(),&lv_switch_class);CHECK(trend_toggle&&lv_obj_has_state(trend_toggle,LV_STATE_CHECKED));
 lv_obj_scroll_to_view_recursive(lv_obj_get_parent(trend_toggle),LV_ANIM_OFF);snap("trend-option",lang);audit(lv_screen_active());lv_obj_remove_state(trend_toggle,LV_STATE_CHECKED);
 set(1,"1450");lv_obj_scroll_to_view_recursive(field(2),LV_ANIM_OFF);snap("voltage-anchors",lang);unsigned before=requests;set(2,"3800");press("Zapisz");CHECK(requests==before&&!in_flight&&find_label(lv_screen_active(),"Sprawdź liczby i progi: 0% < 50% < 100%. Odstęp min. 100 mV."));snap("invalid",lang);
 set(2,"3300");
 /* Real tap focuses a field and scrolls it above the actual numeric keyboard. */
 lv_obj_t *f=field(4);lv_obj_scroll_to_view_recursive(f,LV_ANIM_OFF);lv_area_t a=box(f);tap(a.x1+36,(a.y1+a.y2)/2);
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb&&!lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));CHECK(lv_keyboard_get_mode(kb)==LV_KEYBOARD_MODE_NUMBER);a=box(f);CHECK(a.y2<192);CHECK(box(lv_obj_get_parent(find_label(lv_screen_active(),"Zapisz"))).y1==420);snap("keyboard",lang);audit(lv_screen_active());lv_obj_send_event(kb,LV_EVENT_READY,NULL);
 /* Header and battery-icon navigation use the same unsaved-draft guard. */
 tap(26,22);CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));snap("unsaved",lang);press("Zostań");CHECK(!strcmp(lv_textarea_get_text(field(1)),"1450"));tap(298,22);CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));press("Zostań");
 next_submit_result=DSR_BUSY;press("Zapisz");CHECK(requests==before&&!in_flight&&find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));
 press("Zapisz");CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_PROFILE_SAVE&&submitted.battery_profile.capacity_mah==1450&&submitted.battery_profile.kind==DSR_BATTERY_LIPO_1S);CHECK(!submitted.connection.device.host[0]&&!submitted.connection.credentials.password[0]);snap("saving",lang);
 tap(26,22);CHECK(dsr_ui_battery_calibration_active()&&find_label(lv_screen_active(),"Poczekaj na zakończenie zapisu."));
 before=requests;press("Zapisz");CHECK(requests==before);profile_reply(DSR_IO);CHECK(find_label(lv_screen_active(),"Zapis lub odczyt niepotwierdzony. Wczytaj profil ponownie."));snap("write-error",lang);audit(lv_screen_active());
 press("Wczytaj ponownie");CHECK(submitted.kind==DSR_JOB_BATTERY_PROFILE_LOAD);profile_reply(DSR_OK);CHECK(!strcmp(lv_textarea_get_text(field(1)),"1450"));CHECK(lv_dropdown_get_selected(find_type(lv_screen_active(),&lv_dropdown_class))==DSR_BATTERY_LIPO_1S);
 press("Zapisz");profile_reply(DSR_OK);CHECK(find_label(lv_screen_active(),"Ustawienia zapisane."));snap("saved",lang);
 press("Wstecz");CHECK(find_label(lv_screen_active(),"Bateria i zasilanie"));
 /* Restart reads the stored profile. Background reads are single-slot and
  * pause during display sleep, busy control and the calibration form. */
 dsr_ui_battery_init();display_sleeping=true;lv_tick_inc(22000);dsr_ui_battery_tick();CHECK(!in_flight);display_sleeping=false;dsr_ui_battery_tick();CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ);
 response=(dsr_event_t){.battery={.has_raw=true,.has_voltage=true,.adc_mv=2000,.bat_mv=4000,.spread=6}};dsr_battery_apply_profile(&battery_stored,&response.battery);deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"~75%"));snap("estimated",lang);CHECK(!response.battery.has_charge_state&&!response.battery.charging);
 press("Kalibracja baterii");before=requests;lv_tick_inc(22000);dsr_ui_battery_tick();CHECK(requests==before&&!in_flight);press("Zmierz napięcie");CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ);
 response=(dsr_event_t){.battery={.has_raw=true,.has_voltage=true,.adc_mv=2000,.bat_mv=4000,.spread=4}};deliver(DSR_OK,DSR_WIFI_IDLE);set(5,"4200");press("Oblicz korektę");CHECK(find_label(lv_screen_active(),"1050 / 1000"));snap("reference",lang);
 set(5,"2500");press("Oblicz korektę");CHECK(find_label(lv_screen_active(),"Zmierz ponownie. Wpisz 2500–4350 mV; korekta maks. ±10%."));CHECK(find_label(lv_screen_active(),"1050 / 1000"));
 set(5,"4200");lv_tick_inc(30001);press("Oblicz korektę");CHECK(find_label(lv_screen_active(),"Zmierz ponownie. Wpisz 2500–4350 mV; korekta maks. ±10%."));snap("stale-reference",lang);
 press("Korekta 1:1");CHECK(find_label(lv_screen_active(),"1000 / 1000"));press("Wstecz");CHECK(!dsr_ui_battery_calibration_active());
 if(in_flight){CHECK(submitted.kind==DSR_JOB_BATTERY_READ);response=(dsr_event_t){.battery={.has_raw=true,.has_voltage=true,.adc_mv=2000,.bat_mv=4000,.spread=4}};dsr_battery_apply_profile(&battery_stored,&response.battery);deliver(DSR_OK,DSR_WIFI_IDLE);}
 /* Disabling affects only this profile, with no cell/charge-state inference. */
 press("Kalibracja baterii");t=find_type(lv_screen_active(),&lv_dropdown_class);lv_dropdown_set_selected(t,0);press("Zapisz");profile_reply(DSR_OK);press("Wstecz");CHECK(battery_stored.kind==DSR_BATTERY_NONE);before=requests;lv_tick_inc(40000);dsr_ui_battery_tick();CHECK(requests==before&&!in_flight);snap("disabled",lang);
 reset_profile();
}
static void broken_profile(void)
{
 reset_profile();dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,0);battery_load_failure=true;dsr_ui_battery_init();dsr_ui_battery_calibration_open(NULL);snap("boot-read-error",0);unsigned before=requests;press("Zapisz");CHECK(requests==before&&!in_flight);
 press("Wczytaj ponownie");profile_reply(DSR_IO);press("Nowy profil");CHECK(find_label(lv_screen_active(),"Utworzyć nowy profil?"));press("Anuluj");CHECK(!in_flight&&battery_stored.kind==DSR_BATTERY_NONE);
 press("Nowy profil");press("Utwórz");CHECK(find_label(lv_screen_active(),"Nowy profil jest wyłączony. Sprawdź dane i zapisz."));CHECK(!in_flight);snap("new-profile",0);battery_load_failure=false;press("Zapisz");profile_reply(DSR_OK);press("Wstecz");reset_profile();
}
static void cycles(void)
{
 reset_profile();dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,0);dsr_ui_battery_open(NULL);press("Kalibracja baterii");press("Wstecz");dsr_ui_open_settings();lv_refr_now(display);
 lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned i=0;i<40;i++){dsr_ui_battery_open(NULL);press("Kalibracja baterii");press("Wstecz");dsr_ui_open_settings();lv_refr_now(display);}
 lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);CHECK(after.free_biggest_size>32000);
 printf("Calibration: 40 closed-screen cycles, free %lu -> %lu bytes\n",(unsigned long)before.free_size,(unsigned long)after.free_size);
 /* Returning from all localized forms must still fit a full dashboard. */
 dsr_mobile_model_t m={.wifi=true,.configured_layout=true,.tile_count=24,.device_count=4,.room_count=1,
  .rooms={{.name="Salon",.id=1}},.devices={
   {.id=1,.name="Shelly Plus 1PM",.kind=DSR_DEVICE_RELAY,.online=true,.output=true,.can_switch=true,.has_power=true,.has_voltage=true,.power_w=18.6,.voltage_v=232.9},
   {.id=2,.name="SPSH-002PE16EU",.kind=DSR_DEVICE_COVER,.online=true,.has_position=true,.position_pct=84,.can_cover=true,.can_stop_cover=true,.cover_state=DSR_COVER_STOPPED,.has_power=true,.has_voltage=true,.voltage_v=232.9},
   {.id=3,.name="BLU Window",.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.can_refresh=true,.has_contact=true,.has_battery=true,.battery_pct=92,.has_illuminance=true,.illuminance_lux=18,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|DSR_BLU_HAS_RSSI,.rssi=-69}},
   {.id=4,.name="Shelly Plug S",.kind=DSR_DEVICE_PLUG,.online=false}}};
 for(unsigned i=0;i<24;i++)m.tiles[i]=(dsr_mobile_tile_t){.type=0,.index=i%4,.span=2,.height=1,.fields=255};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_refr_now(display);lv_mem_monitor(&after);
 CHECK(after.free_size>4096&&after.free_biggest_size>4096);printf("After calibration: 24 tiles, %lu bytes free, largest %lu\n",(unsigned long)after.free_size,(unsigned long)after.free_biggest_size);
}
static uint32_t header_pixels(void)
{
 lv_refr_now(display);uint32_t hash=2166136261u;
 for(unsigned y=0;y<24;y++)for(unsigned x=276;x<320;x++)for(unsigned c=0;c<3;c++)hash=(hash^canvas[(y*320+x)*3+c])*16777619u;
 return hash;
}
static void trend_animation(void)
{
 reset_profile();dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_CYBER_BLUE,0);
 battery_stored.kind=DSR_BATTERY_LIPO_1S;battery_stored.trend_enabled=1;dsr_ui_battery_init();dsr_ui_battery_open(NULL);
 for(unsigned i=0;i<=10;i++){
  if(!in_flight){lv_tick_inc(1000);dsr_ui_battery_tick();}
  CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ);
  response=(dsr_event_t){.battery={.has_raw=true,.has_voltage=true,.adc_mv=(uint16_t)(1950+i),.bat_mv=(uint16_t)(3900+2*i),.spread=4}};
  dsr_battery_apply_profile(&battery_stored,&response.battery);deliver(DSR_OK,DSR_WIFI_IDLE);
  CHECK((find_label(lv_screen_active(),"Prawdopodobne ładowanie • trend min. 10 s")!=NULL)==(i==10));
  CHECK(!response.battery.has_charge_state&&!response.battery.charging);
 }
 snap("likely-charging",0);dsr_network_home(NULL);uint32_t first=header_pixels();
 lv_mem_monitor_t before,after;lv_mem_monitor(&before);lv_tick_inc(500);dsr_ui_battery_tick();CHECK(header_pixels()!=first);lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);snap("trend-animation",0);
 dsr_ui_battery_open(NULL);
 /* Fresh but flat readings stop the trend; a stale retained percentage does
  * not keep animating. Animation never sets confirmed charger fields. */
 for(unsigned i=0;i<3;i++){
  lv_tick_inc(1000);dsr_ui_battery_tick();CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ);
  response=(dsr_event_t){.battery={.has_raw=true,.has_voltage=true,.adc_mv=1960,.bat_mv=3920,.spread=4}};dsr_battery_apply_profile(&battery_stored,&response.battery);deliver(DSR_OK,DSR_WIFI_IDLE);
 }
 CHECK(!find_label(lv_screen_active(),"Prawdopodobne ładowanie • trend min. 10 s"));snap("plateau-stopped",0);
 next_submit_result=DSR_BUSY;lv_tick_inc(1000);dsr_ui_battery_tick();CHECK(!in_flight);
 display_sleeping=true;unsigned count=requests;lv_tick_inc(10000);dsr_ui_battery_tick();CHECK(requests==count&&!in_flight);display_sleeping=false;
 dsr_ui_battery_tick();CHECK(in_flight);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);CHECK(!find_label(lv_screen_active(),"Prawdopodobne ładowanie • trend min. 10 s"));snap("trend-read-error",0);
 reset_profile();
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=false;test_workspace_disable();dsr_ui_init();tick();
 for(unsigned lang=0;lang<3;lang++)form(lang,DSR_SC_STYLE_CYBER_BLUE);
 for(unsigned theme=0;theme<3;theme++)form(theme,theme);
 broken_profile();trend_animation();cycles();
 printf("PASS: %u battery calibration UI checks, %u real LVGL frames; hardware providers are models\n",checks,screenshots);lv_deinit();return 0;
}
