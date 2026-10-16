#include "dsr_history.h"
void test_history_time(uint32_t utc);
void test_history_clear(void);
void test_timer_boot_offset(uint32_t offset);
uint32_t test_history_due_now(void);
unsigned test_history_due_calls(void);
static void history_regression(void)
{
 CHECK(!wifi.connected&&!in_flight&&!maintenance_ready);
 /* ESP timer starts before LVGL. A long board init must not delay the
  * history worker's initial load or its five-second retry deadline. */
 unsigned due_calls=test_history_due_calls();test_timer_boot_offset(120000);tick();
 CHECK(test_history_due_calls()>due_calls);
 CHECK(test_history_due_now()==lv_tick_get()+120000u);test_timer_boot_offset(0);
 uint32_t a=review_entry_id("Relay A");test_history_clear();test_history_time(1760000100);
 dsr_network_device_control(a);tap(160,65);CHECK(!dsr_mobile_active()&&find_type(lv_screen_active(),&lv_chart_class)&&find_label(lv_screen_active(),"Brak próbek dla tego pomiaru"));screenshot("241-history-empty");
 tap(40,65);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);wifi.connected=true;tick();CHECK(in_flight);
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_voltage=true,.voltage_v=230,.has_current=true,.current_a=0,.has_energy=true,.energy_wh=0}};response.device.online=response.device.has_switch=response.device.has_power=true;response.device.power_w=0;deliver(DSR_OK,DSR_WIFI_IDLE);
 if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 dsr_network_device_control(a);tap(160,65);CHECK(find_type(lv_screen_active(),&lv_chart_class));CHECK(find_label(lv_screen_active(),"Punkty: rzeczywiste średnie z okien 5 min."));screenshot("242-history-real-zero");
 click("24 h");click("Wh");CHECK(find_label(lv_screen_active(),"Wh: odczyt licznika Shelly, nie suma W."));lv_obj_t *heading=find_label(lv_screen_active(),"Relay A");CHECK(heading);lv_area_t head_area;lv_obj_get_coords(heading,&head_area);CHECK(head_area.y1>=0&&head_area.y2<=44);screenshot("243-history-energy-counter");unsigned header_pixels=0;for(unsigned y=9;y<33;y++)for(unsigned x=44;x<180;x++){unsigned at=(y*320+x)*3;if(canvas[at]>180&&canvas[at+1]>180&&canvas[at+2]>180)header_pixels++;}CHECK(header_pixels>30);
 click("V");click("A");click("W");click("1 h");tap(267,65);CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS&&find_label(lv_screen_active(),"Relay A"));tap(20,20);
 /* Exact selected ID persists through Web ordering and locale changes. */
 cJSON *original=room_document();for(unsigned language=1;language<=2;language++){cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(r);dsr_ui_history_open(a);CHECK(find_label(lv_screen_active(),"Relay A"));screenshot(language==1?"244-history-english":"245-history-german");tap(40,65);}
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(original,"revision"),dsr_workspace_revision());publish_document(original);wifi.connected=false;tick();dsr_ui_energy_open();CHECK(find_label(lv_screen_active(),"Brak świeżych pomiarów mocy"));screenshot("246-energy-offline");
 tap(20,20);dsr_network_device_control(a);tap(160,65);test_history_time(0);lv_tick_inc(5001);tick();CHECK(find_label(lv_screen_active(),"Brak poprawnego czasu UTC. W historii pozostaje przerwa."));screenshot("247-history-no-utc");
 /* Explicit clock action has no file I/O in the LVGL owner. Cancel returns
  * to ordinary controls; sole worker transport tests cover SDK execution. */
 click("Synchronizuj czas UTC");CHECK(dsr_history_clock_pending());click("Anuluj synchronizację");CHECK(!dsr_history_clock_pending());test_history_time(1760000400);tap(40,65);
 lv_mem_monitor_t before,after;dsr_ui_history_open(a);click("24 h");click("Wh");tap(40,65);lv_refr_now(display);lv_mem_monitor(&before);
 for(unsigned i=0;i<40;i++){dsr_ui_history_open(a);click("24 h");click("Wh");tap(40,65);}lv_refr_now(display);lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);
 printf("History UI heap %u→%u;40 actual chart open/range/metric/back cycles; provider MODEL, actual history aggregation\n",(unsigned)before.free_size,(unsigned)after.free_size);dsr_network_home(NULL);
}
