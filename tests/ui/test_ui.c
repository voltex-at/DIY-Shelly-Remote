/* Actual LVGL + actual UI source. Only platform/radio are fixture providers.
 * Screenshots are rendered pixels, not HTML mockups or simulated device data
 * included in firmware. */
#include "dsr_ui.h"
#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_board.h"
#include "dsr_workspace.h"
#include "dsr_ble.h"
#include "esp_app_desc.h"
#include "dsr_system_info.h"
#include "dsr_actions.h"
#include "dsr_restart.h"
#include "dsr_native_ota.h"
#include "../../components/dsr_platform/dsr_activity_internal.h"
#include "src/widgets/buttonmatrix/lv_buttonmatrix_private.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static unsigned screenshots,requests;
static unsigned job_counts[DSR_JOB_RGB+1];
void test_native_ota_set(dsr_native_ota_state_t state,dsr_native_ota_error_t error,dsr_manifest_result_t why);
void test_native_ota_uncertain(void);
void test_native_ota_notes(const char *text);
unsigned test_native_ota_cancels(void);
void test_workspace_enable(void);
void test_workspace_disable(void);
void test_workspace_defer(bool enabled);
void test_workspace_finish(bool success);
void test_workspace_photos(bool enabled);
void test_workspace_fault(unsigned fault);
void test_panel_ready(bool ready);
void test_ota_accept(bool accepted);
void test_pair_accept(bool accepted);
void test_pairing_state(bool available,bool paired,bool uncertain);
unsigned test_pair_code_requests(void);
void test_lan_found(const char *identity,const char *host);
void test_lan_inspected(dsr_result_t result,const char *components);
void test_ble_packet(const uint8_t *bytes,size_t length,unsigned address);
void test_ble_finish(dsr_ble_phase_t phase);
void test_ap_temporary(bool value);
void test_ap_handoff_request(void);
bool test_ap_handoff_available(void);
unsigned test_ap_cancels(void);
unsigned test_lan_cancels(void);
#define CHECK(x) do {checks++; if (!(x)) {fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); abort();}} while(0)
static dsr_job_t submitted;
static dsr_event_t response;
static bool in_flight, result_ready,stale_event;
static dsr_result_t next_submit_result;
static dsr_job_t maintenance;static bool maintenance_ready;
static dsr_result_t maintenance_result=DSR_OK;
static bool hold_maintenance;
static unsigned publish_jobs,unbind_jobs;
void test_workspace_raw_fault(unsigned fault);
static dsr_settings_t stored = {.brightness = 70, .idle_seconds = 60};
static dsr_wifi_info_t wifi;
static unsigned char canvas[320 * 480 * 3];
static uint16_t draw_buffer[320 * 40];
static lv_display_t *display;
static const char *output;
static lv_indev_t *pointer;
static lv_point_t touch;
static bool pressed;
static unsigned ready_count;
/* Backend MODEL only: no activity reservation, task, or esp_restart. Real
 * admission/dispatcher races are covered by the separate native suite. */
static unsigned restart_requests;
static dsr_restart_result_t restart_response=DSR_RESTART_BUSY;
dsr_restart_result_t dsr_restart_request(void){restart_requests++;return restart_response;}
static void read_pointer(lv_indev_t *input,lv_indev_data_t *data)
{(void)input;data->point=touch;data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
static void tap(int x,int y)
{touch=(lv_point_t){x,y};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_obj_update_layout(lv_screen_active());}
static void ready(lv_event_t *event){(void)event;ready_count++;}
static void key(lv_obj_t *kb,const char *text)
{
 lv_obj_update_layout(kb);const lv_buttonmatrix_t *matrix=(const lv_buttonmatrix_t *)kb;lv_area_t area;lv_obj_get_coords(kb,&area);
 for(unsigned i=0;i<matrix->btn_cnt;i++)if(!strcmp(lv_keyboard_get_button_text(kb,i),dsr_sc_tr(text))){
  lv_area_t b=matrix->button_areas[i];tap(area.x1+(b.x1+b.x2)/2,area.y1+(b.y1+b.y2)/2);return;
 }CHECK(!"Keyboard key missing");
}

const esp_app_desc_t *esp_app_get_description(void) {static const esp_app_desc_t d = {.version="0.3.0-wifi-dev"}; return &d;}
const char *dsr_board_hardware_id(void){return "lcdwiki-es3c35p-n16r8";}
void dsr_system_info_snapshot(dsr_system_info_t *out)
{*out=(dsr_system_info_t){.internal_free=140000,.internal_largest=73728,.psram_free=8000000,.flash_bytes=16777216};}
esp_err_t dsr_board_set_backlight(uint8_t value) {(void)value; return ESP_OK;}
static unsigned display_applies;
static uint8_t applied_brightness;static uint16_t applied_seconds;
static bool apply_failure;
esp_err_t dsr_board_apply_display(uint8_t brightness,uint16_t seconds)
{display_applies++;applied_brightness=brightness;applied_seconds=seconds;return apply_failure?ESP_FAIL:ESP_OK;}
void dsr_board_sleep_display(void) {}
/* First-format observer seams (#28); the real gate/handshake is host-tested
 * in test_boot_memory/test_startup. These only record the call protocol. */
static bool provision_gate;static unsigned provision_task_starts,provision_quiesces,provision_backlight;
static uint32_t provision_progress_value;static unsigned provision_progress_reads;
esp_err_t dsr_board_restore_backlight(void){provision_backlight++;return ESP_OK;}
bool dsr_boot_ui_gate(void){return provision_gate;}
void dsr_boot_provisioning_ui(bool open){provision_gate=open;}
static bool fail_task_start;
esp_err_t dsr_board_start_ui_task(bool (*ready)(void)){provision_task_starts++;return !fail_task_start&&ready==dsr_boot_ui_gate&&provision_gate?ESP_OK:ESP_ERR_NO_MEM;}
void dsr_board_ui_quiesce(void){CHECK(!provision_gate);provision_quiesces++;}
/* Models the UI task rendering while main is blocked with the gate open. */
static unsigned provision_holds;
void dsr_board_ui_hold(uint32_t ms){CHECK(provision_gate);provision_holds++;for(uint32_t t=0;t<ms;t+=250){lv_tick_inc(250);lv_timer_handler();}}
uint32_t dsr_storage_format_progress(void){provision_progress_reads++;return provision_progress_value;}
bool dsr_board_touch_available(void) {return true;}
esp_err_t dsr_settings_load(dsr_settings_t *out) {*out=stored; return ESP_OK;}
esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found) {memset(out,0,sizeof(*out)); *found=false; return ESP_OK;}
static dsr_connection_t boot_loaded[DSR_MAX_DEVICES];static size_t boot_loaded_count;
esp_err_t dsr_connections_load(dsr_connection_t *out,size_t capacity,size_t *count){CHECK(capacity>=boot_loaded_count);memcpy(out,boot_loaded,boot_loaded_count*sizeof(*out));*count=boot_loaded_count;return ESP_OK;}
bool dsr_connection_valid(const dsr_connection_t *c,bool bound){return c&&c->device.host[0]&&(!bound||c->device.device_id[0]);}
bool dsr_wifi_connected(void) {return wifi.connected&&!dsr_wifi_temporary_active();}
void dsr_wifi_get_info(dsr_wifi_info_t *out) {*out=wifi;}
void dsr_wifi_cancel_connect(void) {CHECK(submitted.kind==DSR_JOB_WIFI_CONNECT);}
dsr_result_t dsr_worker_submit(const dsr_job_t *job)
{if(job->kind==DSR_JOB_LAYOUT_PUBLISH||job->kind==DSR_JOB_UNBIND){CHECK(!maintenance_ready&&!in_flight);CHECK(dsr_activity_begin());maintenance=*job;maintenance_ready=true;if(job->kind==DSR_JOB_UNBIND)unbind_jobs++;else publish_jobs++;return DSR_OK;}if(next_submit_result!=DSR_OK){dsr_result_t result=next_submit_result;next_submit_result=DSR_OK;return result;}CHECK(!in_flight);if(!dsr_activity_begin())return DSR_BUSY;requests++;CHECK(job->kind<=DSR_JOB_RGB);job_counts[job->kind]++;submitted=*job; in_flight=true; return DSR_OK;}
bool dsr_worker_receive(dsr_event_t *out)
{if(maintenance_ready&&!hold_maintenance){*out=(dsr_event_t){.id=maintenance.id,.result=maintenance_result,.device=maintenance.connection.device,.layout_id=maintenance.connection.layout_id,.telemetry=maintenance.connection.telemetry};maintenance_ready=false;dsr_activity_end();return true;}if(!result_ready)return false; *out=response; result_ready=false;if(stale_event){stale_event=false;return true;} /* synthetic unrelated event leaves current MODEL job in flight */ in_flight=false;dsr_activity_end();CHECK(!dsr_activity_restart_reserve()); /* real poll lease covers the receive->persistent-follow-up gap */ return true;}
/* Urgent SDK lane MODEL; genuine sole-worker mutex/event ownership is tested
 * in test_battery.c. This fixture lets actual LCD consume delayed results. */
static bool stop_pending,stop_ready;static dsr_stop_result_t stop_result;
dsr_result_t dsr_worker_stop_request(const dsr_connection_t *c,uint32_t id)
{if(stop_pending||!dsr_activity_begin())return DSR_BUSY;stop_pending=true;stop_result=(dsr_stop_result_t){.operation_id=id,.layout_id=c->layout_id,.device=c->device,.telemetry=c->telemetry};return DSR_OK;}
bool dsr_worker_stop_pending(void){return stop_pending;}
bool dsr_worker_stop_receive(dsr_stop_result_t *out,bool outer_pending)
{if(!stop_ready||outer_pending)return false;*out=stop_result;stop_pending=stop_ready=false;memset(&stop_result,0,sizeof(stop_result));dsr_activity_end();return true;}

static void flush(lv_display_t *d,const lv_area_t *area,uint8_t *data)
{
    const uint16_t *pixels=(const uint16_t *)data;
    for(int y=area->y1;y<=area->y2;y++)for(int x=area->x1;x<=area->x2;x++) {
        uint16_t p=*pixels++;
        if(x<0||x>=320||y<0||y>=480)continue;
        unsigned at=(unsigned)(y*320+x)*3;
        canvas[at]=(unsigned char)(((p>>11)&31)*255/31);
        canvas[at+1]=(unsigned char)(((p>>5)&63)*255/63);
        canvas[at+2]=(unsigned char)((p&31)*255/31);
    }
    lv_display_flush_ready(d);
}
dsr_result_t dsr_worker_action_stop_request(const dsr_connection_t *c,uint32_t id){return dsr_worker_stop_request(c,id);}

static void tick(void) {lv_tick_inc(250);lv_timer_handler();for(unsigned i=0;i<20&&maintenance_ready&&!hold_maintenance;i++){lv_tick_inc(250);lv_timer_handler();}lv_refr_now(display);}
static void screenshot(const char *name)
{
    lv_obj_update_layout(lv_screen_active());lv_refr_now(display);
    lv_obj_t *badge=NULL;
    unsigned frame=(unsigned)strtoul(name,NULL,10);
    unsigned badge_y=frame>=300?8:frame>=180&&frame<=199?8:frame>=160&&frame<=177?195:frame>=130?80:8;
    if(strtoul(name,NULL,10)>=50){badge=dsr_sc_panel(lv_layer_top(),226,badge_y,84,23,0x5c492a,5);dsr_sc_text(badge,"TEST / DEMO",4,5,76,&dsr_roboto_11,0xffd68a);}
    lv_obj_update_layout(lv_screen_active());lv_obj_invalidate(lv_screen_active());lv_refr_now(display);
    if(badge){unsigned at=((badge_y+12)*320+230)*3;CHECK(canvas[at]>60&&canvas[at+1]>40&&canvas[at+2]<70);}
    char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",output,name);
    FILE *f=fopen(path,"wb");CHECK(f!=NULL);fprintf(f,"P6\n320 480\n255\n");
    CHECK(fwrite(canvas,1,sizeof(canvas),f)==sizeof(canvas));fclose(f);screenshots++;if(badge)lv_obj_delete(badge);
}
static lv_obj_t *find_label(lv_obj_t *root,const char *text)
{
    if(lv_obj_check_type(root,&lv_label_class)&&!strcmp(lv_label_get_text(root),dsr_sc_tr(text)))return root;
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *f=find_label(lv_obj_get_child(root,(int)i),text);if(f)return f;}
    return NULL;
}
static lv_obj_t *find_type(lv_obj_t *root,const lv_obj_class_t *type)
{
    if(lv_obj_check_type(root,type))return root;
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *f=find_type(lv_obj_get_child(root,(int)i),type);if(f)return f;}
    return NULL;
}
static void click(const char *text)
{
    lv_obj_t *label=find_label(lv_screen_active(),text);if(!label)fprintf(stderr,"Missing UI label: %s\n",text);CHECK(label!=NULL);
    (void)lv_obj_send_event(lv_obj_get_parent(label),LV_EVENT_CLICKED,NULL);
    lv_obj_update_layout(lv_screen_active());
}
/* Legacy flow setup is explicit: choosing a radio card never navigates. */
static void choose_method(const char *label)
{
 const char *method=!strcmp(label,"Bluetooth LE")?"Bluetooth":!strcmp(label,"Skanuj sieć Wi-Fi")?"Sieć lokalna":label;
 lv_obj_t *text=find_label(lv_screen_active(),method);CHECK(text);
 lv_obj_scroll_to_view(lv_obj_get_parent(text),LV_ANIM_OFF);click(method);click("Dalej");
}
static lv_obj_t *power_target(lv_obj_t *root)
{
 if(lv_obj_get_width(root)==86&&lv_obj_get_height(root)==86&&lv_obj_has_flag(root,LV_OBJ_FLAG_CLICKABLE))return root;
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=power_target(lv_obj_get_child(root,(int)i));if(o)return o;}return NULL;
}
static lv_obj_t *nth_type(lv_obj_t *root,const lv_obj_class_t *type,unsigned *skip)
{
    if(lv_obj_check_type(root,type)){if(!*skip)return root;(*skip)--;}
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=nth_type(lv_obj_get_child(root,(int)i),type,skip);if(o)return o;}return NULL;
}
static void deliver(dsr_result_t result,dsr_wifi_status_t detail)
{if(dsr_actions_job_owned(submitted.id)){dsr_result_t allowed=dsr_actions_worker_allowed(submitted.id,&submitted.connection);if(allowed!=DSR_OK)result=allowed;}response.id=submitted.id;response.result=result;response.wifi_detail=detail;result_ready=true;tick();if(submitted.kind==DSR_JOB_BIND&&!in_flight)tick();}
static void battery_regression(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,false,0);
 wifi.connected=false;tick();CHECK(!in_flight);test_workspace_disable();
 dsr_ui_open_settings();click("Bateria i zasilanie");CHECK(find_label(lv_screen_active(),"Pomiar na żądanie"));
 CHECK(find_label(lv_screen_active(),"Ładowanie i obecność akumulatora: brak wiarygodnych danych."));
 CHECK(find_label(lv_screen_active(),"?"));screenshot("130-battery-unknown");
 next_submit_result=DSR_BUSY;click("Zmierz napięcie");tick();
 CHECK(!in_flight&&find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));
 unsigned before=requests;click("Zmierz napięcie");CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ&&requests==before+1);
 CHECK(!submitted.connection.device.host[0]&&!submitted.connection.credentials.password[0]);
 CHECK(find_label(lv_screen_active(),"Odczyt napięcia…"));screenshot("131-battery-reading");
 lv_obj_t *button=lv_obj_get_parent(find_label(lv_screen_active(),"Zmierz napięcie"));CHECK(lv_obj_has_state(button,LV_STATE_DISABLED));
 response.battery=(dsr_battery_sample_t){.has_raw=true,.has_voltage=true,.raw=2001,.adc_mv=2025,.bat_mv=4050};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"4.050 V")&&find_label(lv_screen_active(),"4.05V"));CHECK(!lv_obj_has_state(button,LV_STATE_DISABLED));
 screenshot("132-battery-voltage");
 lv_tick_inc(30001);tick();CHECK(find_label(lv_screen_active(),"?"));CHECK(find_label(lv_screen_active(),"Pomiar nieaktualny. Zmierz ponownie."));screenshot("133-battery-stale");
 click("Zmierz napięcie");response.battery=(dsr_battery_sample_t){.has_raw=true,.raw=2001,.calibration_error=ESP_ERR_NOT_SUPPORTED};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Kalibracja ADC niedostępna. Napięcie nieznane."));CHECK(!find_label(lv_screen_active(),"4.050 V"));screenshot("134-battery-uncalibrated");
 click("Zmierz napięcie");response.battery=(dsr_battery_sample_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Pomiar niedostępny. Spróbuj ponownie."));screenshot("135-battery-adc-error");
 /* A completion after Back must never touch deleted battery-page labels. */
 click("Zmierz napięcie");tap(160,444);CHECK(!find_label(lv_screen_active(),"Napięcie wejścia BAT"));
 response.battery=(dsr_battery_sample_t){.has_voltage=true,.bat_mv=4100};deliver(DSR_OK,DSR_WIFI_IDLE);
 tap(298,22);CHECK(find_label(lv_screen_active(),"4.100 V"));
 for(unsigned language=1;language<=2;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);dsr_ui_battery_open(NULL);
  CHECK(find_label(lv_screen_active(),"Zmierz napięcie"));
  screenshot(language==1?"136-battery-english":"137-battery-german");
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,0);dsr_ui_open_settings();dsr_ui_battery_open(NULL);lv_obj_update_layout(lv_screen_active());
 for(unsigned n=0;n<10;n++){tap(160,444);tap(298,22);lv_obj_update_layout(lv_screen_active());}
 lv_mem_monitor_t before_memory,after_memory;lv_mem_monitor(&before_memory);
 for(unsigned n=0;n<40;n++){tap(160,444);tap(298,22);lv_obj_update_layout(lv_screen_active());}
 lv_mem_monitor(&after_memory);
 printf("Battery UI heap before=%lu after=%lu; 40 open/back cycles; on-demand host ADC fixture\n",(unsigned long)before_memory.free_size,(unsigned long)after_memory.free_size);
 CHECK(after_memory.free_size==before_memory.free_size);
}
static void display_regression(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,false,0);wifi.connected=false;tick();CHECK(!in_flight);
 dsr_ui_open_settings();click("Ekran i komfort");CHECK(find_label(lv_screen_active(),"Screen off / touch to wake"));click("Ekran");
 lv_obj_t *brightness=find_type(lv_screen_active(),&lv_slider_class);
 lv_obj_t *timeout=find_type(lv_screen_active(),&lv_dropdown_class);
 CHECK(brightness&&timeout&&lv_slider_get_value(brightness)==70&&lv_dropdown_get_selected(timeout)==3);
 screenshot("140-display-default");
 lv_slider_set_value(brightness,35,LV_ANIM_OFF);lv_obj_send_event(brightness,LV_EVENT_VALUE_CHANGED,NULL);lv_dropdown_set_selected(timeout,0);
 next_submit_result=DSR_BUSY;click("Zapisz");CHECK(!in_flight&&find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));
 unsigned before=requests,applies=display_applies;click("Zapisz");
 CHECK(in_flight&&submitted.kind==DSR_JOB_DISPLAY_SAVE&&submitted.settings.brightness==35&&submitted.settings.idle_seconds==0);
 CHECK(!submitted.settings.ssid[0]&&!submitted.settings.password[0]);
 CHECK(lv_obj_has_state(brightness,LV_STATE_DISABLED)&&lv_obj_has_state(timeout,LV_STATE_DISABLED));
 click("Zapisz");CHECK(requests==before+1);screenshot("141-display-saving");
 /* Global header Home clears the active LCD directly, bypassing ui_screen. */
 tap(22,22);CHECK(!find_label(lv_screen_active(),"Wygaszanie ekranu"));
 response.brightness=35;response.idle_seconds=0;deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(display_applies==applies+1&&applied_brightness==35&&applied_seconds==0);
 dsr_ui_display_open(NULL);brightness=find_type(lv_screen_active(),&lv_slider_class);timeout=find_type(lv_screen_active(),&lv_dropdown_class);
 CHECK(lv_slider_get_value(brightness)==35&&lv_dropdown_get_selected(timeout)==0);screenshot("142-display-saved");
 /* Save failure neither applies hardware nor changes cached preferences. */
 lv_slider_set_value(brightness,80,LV_ANIM_OFF);lv_dropdown_set_selected(timeout,4);click("Zapisz");
 applies=display_applies;deliver(DSR_IO,DSR_WIFI_IDLE);CHECK(display_applies==applies&&find_label(lv_screen_active(),"Nie potwierdzono zapisu. Spróbuj ponownie."));
 CHECK(!lv_obj_has_state(brightness,LV_STATE_DISABLED));screenshot("143-display-save-error");
 tap(160,444);dsr_ui_display_open(NULL);brightness=find_type(lv_screen_active(),&lv_slider_class);CHECK(lv_slider_get_value(brightness)==35);
 /* Back/reopen retains pending inhibition; response applies independently. */
 lv_slider_set_value(brightness,55,LV_ANIM_OFF);timeout=find_type(lv_screen_active(),&lv_dropdown_class);lv_dropdown_set_selected(timeout,4);click("Zapisz");tap(160,444);dsr_ui_display_open(NULL);CHECK(find_label(lv_screen_active(),"Zapisywanie ustawień…"));
 brightness=find_type(lv_screen_active(),&lv_slider_class);CHECK(lv_obj_has_state(brightness,LV_STATE_DISABLED));
 response.brightness=55;response.idle_seconds=120;apply_failure=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Zapisano. Błąd sterownika ekranu.")&&lv_slider_get_value(brightness)==55&&lv_dropdown_get_selected(find_type(lv_screen_active(),&lv_dropdown_class))==4);screenshot("144-display-driver-error");
 apply_failure=false;dsr_ui_display_init(20,60);CHECK(applied_brightness==20&&applied_seconds==60);
 for(unsigned language=1;language<=2;language++){dsr_sc_appearance(DSR_SC_BLUE,false,language);dsr_ui_display_open(NULL);CHECK(find_label(lv_screen_active(),"Wygaszanie ekranu"));screenshot(language==1?"145-display-english":"146-display-german");}
 dsr_sc_appearance(DSR_SC_BLUE,false,0);dsr_ui_display_open(NULL);
 for(unsigned n=0;n<10;n++){tap(160,444);dsr_ui_display_open(NULL);lv_obj_update_layout(lv_screen_active());}
 lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(unsigned n=0;n<40;n++){tap(160,444);dsr_ui_display_open(NULL);lv_obj_update_layout(lv_screen_active());}
 lv_mem_monitor(&b);CHECK(a.free_size==b.free_size);
 printf("Display UI heap before=%lu after=%lu;40 pending-safe navigation cycles; hardware fixture only\n",(unsigned long)a.free_size,(unsigned long)b.free_size);
}
static cJSON *room_document(void)
{char *json=dsr_workspace_snapshot();CHECK(json);cJSON *r=cJSON_Parse(json);free(json);CHECK(r);return r;}
static void publish_document(cJSON *r)
{char *json=cJSON_PrintUnformatted(r);CHECK(json);CHECK(dsr_workspace_submit(json,dsr_workspace_revision()));free(json);cJSON_Delete(r);}
static void tap_row(const char *text)
{
 lv_obj_t *l=find_label(lv_screen_active(),text);CHECK(l);lv_obj_t *b=lv_obj_get_parent(l);
 lv_obj_scroll_to_view(b,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
 lv_area_t a;lv_obj_get_coords(b,&a);CHECK(a.x1>=0&&a.x2<320&&a.y1>=86&&a.y2<=416&&lv_obj_get_height(b)>=44);
 tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static void global_settings_regression(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,false,0);wifi.connected=false;test_workspace_disable();
 dsr_ui_open_settings();tick();CHECK(find_label(lv_screen_active(),"Ustawienia pilota"));screenshot("150-global-settings");
 tap_row("Wygląd i język");CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));click("Wstecz");CHECK(find_label(lv_screen_active(),"Ustawienia pilota"));
 snprintf(wifi.ssid,sizeof(wifi.ssid),"WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
 tap_row("Sieć i połączenia");CHECK(find_label(lv_screen_active(),"Wi-Fi: brak połączenia"));
 lv_obj_t *ip=find_label(lv_screen_active(),"IPv4: —");CHECK(ip);lv_obj_t *ssid=lv_obj_get_child(lv_obj_get_parent(ip),1);CHECK(lv_obj_get_height(ssid)==20&&lv_obj_get_y(ssid)+lv_obj_get_height(ssid)<lv_obj_get_y(ip));screenshot("151-global-network");
 tap_row("Bluetooth LE • skan i setup");CHECK(dsr_ui_ble_active());click("Wstecz");CHECK(find_label(lv_screen_active(),"Sieć i połączenia"));click("Wstecz");
 test_panel_ready(false);tap_row("Panel WWW / bezpieczeństwo");CHECK(!find_label(lv_screen_active(),"0123456789abcdef"));screenshot("152-global-web-unavailable");
 tap_row("Wygeneruj kod parowania");CHECK(find_label(lv_screen_active(),"Nie utworzono kodu. HTTPS niedostępny lub zajęty."));click("Wstecz");
 test_panel_ready(true);tap_row("Panel WWW / bezpieczeństwo");CHECK(!find_label(lv_screen_active(),"0123456789abcdef"));
 CHECK(find_label(lv_screen_active(),"0123456789ABCDEF0123456789ABCDEF\n0123456789ABCDEF0123456789ABCDEF"));
 test_pair_accept(false);tap_row("Wygeneruj kod parowania");CHECK(find_label(lv_screen_active(),"Nie utworzono kodu. HTTPS niedostępny lub zajęty."));click("Wstecz");
 test_pair_accept(true);tap_row("Panel WWW / bezpieczeństwo");
 tap_row("Wygeneruj kod parowania");CHECK(find_label(lv_screen_active(),"0123456789abcdef"));lv_tick_inc(120001);tick();CHECK(!find_label(lv_screen_active(),"0123456789abcdef")&&find_label(lv_screen_active(),"Kod parowania wygasł."));click("Wstecz");
 tap_row("System");tap_row("Informacje i diagnostyka");CHECK(find_label(lv_screen_active(),"lcdwiki-es3c35p-n16r8"));screenshot("153-global-system-info");click("Wstecz");
 tap_row("System");tap_row("Firmware Update");CHECK(find_label(lv_screen_active(),"Sprawdź aktualizacje")&&!find_label(lv_screen_active(),"Zainstaluj"));screenshot("154-global-native-ota-idle");
 test_ota_accept(false);tap_row("Zezwól na OTA • 2 minuty");CHECK(find_label(lv_screen_active(),"Nie udzielono zgody OTA. HTTPS niedostępny lub zajęty."));click("Wstecz");
 tap_row("System");tap_row("Firmware Update");test_ota_accept(true);tap_row("Zezwól na OTA • 2 minuty");CHECK(find_label(lv_screen_active(),"OTA dozwolone przez 2 minuty. Wybierz BIN w panelu."));lv_tick_inc(120001);tick();CHECK(find_label(lv_screen_active(),"Zgoda na OTA wygasła."));click("Wstecz");
 test_workspace_enable();cJSON *baseline=room_document();tap_row("Wygląd i język");
 lv_obj_t *language=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(language);lv_dropdown_set_selected(language,2);lv_obj_send_event(language,LV_EVENT_VALUE_CHANGED,NULL);
 click("Zapisz");dsr_ui_workspace_tick();CHECK(dsr_sc_language()==2&&find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));screenshot("155-global-appearance-german");
 cJSON *saved=room_document();const char *collections[]={"rooms","devices","scenes","dashboards"};
 for(unsigned i=0;i<4;i++){CHECK(cJSON_Compare(cJSON_GetObjectItemCaseSensitive(baseline,collections[i]),cJSON_GetObjectItemCaseSensitive(saved,collections[i]),true));}
 cJSON_Delete(baseline);cJSON_Delete(saved);
 click("Wstecz");CHECK(find_label(lv_screen_active(),"Ustawienia pilota"));screenshot("156-global-settings-german");
 tap_row("Wygląd i język");language=find_type(lv_screen_active(),&lv_dropdown_class);lv_dropdown_set_selected(language,1);lv_obj_send_event(language,LV_EVENT_VALUE_CHANGED,NULL);test_workspace_defer(true);click("Zapisz");
 dsr_ui_open_settings();CHECK(find_label(lv_screen_active(),"Poczekaj na zakończenie zapisu."));CHECK(find_label(lv_screen_active(),"Wygląd i język"));
 lv_dropdown_set_selected(language,0);lv_obj_send_event(language,LV_EVENT_VALUE_CHANGED,NULL);test_workspace_finish(true);dsr_ui_workspace_tick();language=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(language&&lv_dropdown_get_selected(language)==0&&dsr_sc_language()==1);CHECK(find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
 dsr_ui_open_settings();CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));click("Zostań");CHECK(lv_dropdown_get_selected(language)==0);
 dsr_ui_open_settings();click("Odrzuć");CHECK(find_label(lv_screen_active(),"Ustawienia pilota"));test_workspace_defer(false);screenshot("157-global-settings-english");
 tap_row("Wygląd i język");language=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(lv_dropdown_get_selected(language)==1);
 lv_dropdown_set_selected(language,0);lv_obj_send_event(language,LV_EVENT_VALUE_CHANGED,NULL);saved=room_document();publish_document(saved);click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));
 dsr_ui_open_settings();click("Odrzuć");dsr_sc_appearance(DSR_SC_BLUE,false,0);dsr_ui_open_settings();
 dsr_ui_edit_dashboard();CHECK(!find_label(lv_screen_active(),"Wygląd i język")&&!find_label(lv_screen_active(),"Urządzenia")&&!find_label(lv_screen_active(),"Zezwól na OTA • 2 minuty"));dsr_ui_open_settings();
 tap_row("System");tap_row("Konfiguracja wpisów i scen");CHECK(find_label(lv_screen_active(),"Urządzenia")&&find_label(lv_screen_active(),"Sceny"));click("Wstecz");
 for(unsigned i=0;i<10;i++){tap_row("System");tap_row("Informacje i diagnostyka");click("Wstecz");}
 lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(unsigned i=0;i<40;i++){tap_row("Panel WWW / bezpieczeństwo");click("Wstecz");tap_row("System");tap_row("Informacje i diagnostyka");click("Wstecz");}
 lv_mem_monitor(&b);CHECK(a.free_size==b.free_size);printf("Global Settings heap before=%lu after=%lu;40 web/system cycles; host only\n",(unsigned long)a.free_size,(unsigned long)b.free_size);
 test_workspace_disable();
}
static void native_ota_regression(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,false,0);wifi.connected=false;tick();
 if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_INVALID);
 unsigned before=requests,cancels=test_native_ota_cancels();dsr_ui_native_ota_open(NULL);
 CHECK(requests==before&&!find_label(lv_screen_active(),"Zainstaluj"));
 tap_row("Zmień kanał");CHECK(requests==before);tap_row("Dev • wersje testowe");
 CHECK(find_label(lv_screen_active(),"Kanał: Dev • wersje testowe")&&requests==before);
 tap_row("Sprawdź aktualizacje");CHECK(in_flight&&submitted.kind==DSR_JOB_OTA_CHECK&&requests==before+1);
 CHECK(!find_label(lv_screen_active(),"Zainstaluj")&&!find_label(lv_screen_active(),"Sprawdź aktualizacje"));
 click("Wstecz");CHECK(test_native_ota_cancels()>cancels&&find_label(lv_screen_active(),"System"));
 test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_MANIFEST,DSR_MANIFEST_UNTRUSTED_KEY);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"System"));dsr_ui_native_ota_open(NULL);
 CHECK(find_label(lv_screen_active(),"Brak zatwierdzonego klucza wydawcy. Instalacja zablokowana.")&&!find_label(lv_screen_active(),"Zainstaluj"));screenshot("240-native-ota-untrusted-pl");
 tap_row("Sprawdź aktualizacje");CHECK(in_flight&&submitted.kind==DSR_JOB_OTA_CHECK);
 test_native_ota_set(DSR_NATIVE_AVAILABLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);
 lv_obj_t *notes=find_label(lv_screen_active(),"<b>plain untrusted</b>\n#ff0000 literal#");CHECK(notes&&!lv_label_get_recolor(notes));
 CHECK(find_label(lv_screen_active(),"9.1.2")&&find_label(lv_screen_active(),"1857984 B"));screenshot("241-native-ota-available-pl");
 for(unsigned language=1;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);dsr_ui_native_ota_open(NULL);lv_obj_t *label=find_label(lv_screen_active(),"9.1.2");CHECK(label&&lv_obj_get_height(label)<=40);
  screenshot(language==1?"242-native-ota-available-en":"243-native-ota-available-de");
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,0);char linebreaks[321];memset(linebreaks,'\n',320);linebreaks[320]=0;test_native_ota_notes(linebreaks);dsr_ui_native_ota_open(NULL);
 lv_obj_t *button=lv_obj_get_parent(find_label(lv_screen_active(),"Sprawdź aktualizacje"));CHECK(button&&lv_obj_get_y(button)<=620);tap_row("Sprawdź aktualizacje");
 test_native_ota_set(DSR_NATIVE_AVAILABLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);
 before=requests;tap_row("Zainstaluj");CHECK(requests==before&&find_label(lv_screen_active(),"Zainstalować podpisane wydanie?"));screenshot("244-native-ota-physical-consent");
 lv_tick_inc(120001);tick();CHECK(find_label(lv_screen_active(),"Zgoda wygasła. Sprawdź wydanie ponownie.")&&!find_label(lv_screen_active(),"USB podłączone • instaluj")&&requests==before);
 tap_row("Sprawdź aktualizacje");test_native_ota_set(DSR_NATIVE_AVAILABLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);
 tap_row("Zainstaluj");before=requests;tap(238,444);CHECK(requests==before+1&&in_flight&&submitted.kind==DSR_JOB_OTA_INSTALL&&!find_label(lv_screen_active(),"USB podłączone • instaluj"));screenshot("245-native-ota-installing");
 test_native_ota_set(DSR_NATIVE_STAGED,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Instalacja zakończona. Uruchom pilot ponownie.")&&!find_label(lv_screen_active(),"Zainstaluj")&&!find_label(lv_screen_active(),"Sprawdź aktualizacje"));screenshot("246-native-ota-staged");
 test_native_ota_uncertain();tick();CHECK(find_label(lv_screen_active(),"Nie potwierdzono wyboru startowego firmware. Dalsze zapisy zablokowane do restartu.")&&!find_label(lv_screen_active(),"Anulowano. Nie uruchomiono nowego firmware."));screenshot("247-native-ota-uncertain");
 restart_response=DSR_RESTART_BUSY;unsigned restarts=restart_requests;tap_row("Uruchom ponownie");CHECK(restart_requests==restarts+1&&find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));
 restart_response=DSR_RESTART_ACCEPTED;tap_row("Uruchom ponownie");CHECK(find_label(lv_screen_active(),"Przyjęto żądanie restartu."));tap_row("Uruchom ponownie");CHECK(restart_requests==restarts+2);
 test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_INVALID);dsr_ui_native_ota_open(NULL);click("Wstecz");tick();lv_mem_monitor_t start,end;lv_mem_monitor(&start);
 for(unsigned i=0;i<40;i++){dsr_ui_native_ota_open(NULL);click("Wstecz");}
 tick();lv_mem_monitor(&end);CHECK(end.free_size+512>=start.free_size);
 printf("Native OTA LCD: explicit channel/physical-power consent, manifest-only check, EMPTY errors, hide/late result, TTL, raw bounded notes, install/staged/uncertain/restart and40 navigation cycles; backend MODEL\n");
}
static void restart_open(void)
{dsr_ui_open_settings();tap_row("System");tap_row("Restart pilota");CHECK(find_label(lv_screen_active(),"Ustawienia i zdjęcia pozostaną. Niezapisane zmiany zostaną utracone."));}
static lv_obj_t *restart_control(const char *text)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label);lv_obj_t *button=lv_obj_get_parent(label);
 lv_obj_update_layout(lv_screen_active());lv_area_t area;lv_obj_get_coords(button,&area);
 CHECK(area.x1>=0&&area.x2<320&&area.y1>=86&&area.y2<404&&lv_obj_get_height(button)>=44);
 return button;
}
static void restart_regression(void)
{
 wifi.connected=false;test_workspace_disable();unsigned before=restart_requests;
 const char *translated[]={"Restart pilota","Ustawienia i zdjęcia pozostaną. Niezapisane zmiany zostaną utracone.","Uruchom ponownie","Anuluj"};
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);restart_open();
  for(unsigned i=0;i<4;i++){CHECK(find_label(lv_screen_active(),translated[i]));if(language&&i<3)CHECK(strcmp(dsr_sc_tr(translated[i]),translated[i]));}
  CHECK(!lv_obj_has_state(restart_control("Uruchom ponownie"),LV_STATE_DISABLED));restart_control("Anuluj");
  lv_area_t question,warning,prompt,confirm;
  lv_obj_get_coords(find_label(lv_screen_active(),"Czy uruchomić pilot ponownie?"),&question);
  lv_obj_get_coords(find_label(lv_screen_active(),translated[1]),&warning);
  lv_obj_t *prompt_label=find_label(lv_screen_active(),"Potwierdź restart przyciskiem poniżej.");CHECK(prompt_label);lv_obj_get_coords(prompt_label,&prompt);
  lv_obj_get_coords(restart_control("Uruchom ponownie"),&confirm);
  CHECK(question.y2<warning.y1&&warning.y2<prompt.y1&&prompt.y2<confirm.y1&&warning.x1>=12&&warning.x2<308);
  screenshot(language==0?"160-restart-confirm-pl":language==1?"161-restart-confirm-en":"162-restart-confirm-de");
  click("Anuluj");CHECK(find_label(lv_screen_active(),"System")&&!find_label(lv_screen_active(),"Ustawienia i zdjęcia pozostaną. Niezapisane zmiany zostaną utracone.")&&restart_requests==before);
  restart_open();click("Wstecz");CHECK(find_label(lv_screen_active(),"Ustawienia pilota")&&restart_requests==before);
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,0);restart_open();restart_response=DSR_RESTART_BUSY;click("Uruchom ponownie");
 CHECK(restart_requests==before+1&&find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));CHECK(!lv_obj_has_state(restart_control("Uruchom ponownie"),LV_STATE_DISABLED));screenshot("163-restart-busy");
 restart_response=DSR_RESTART_UNAVAILABLE;click("Uruchom ponownie");
 CHECK(restart_requests==before+2&&find_label(lv_screen_active(),"Restart jest niedostępny. Spróbuj później."));CHECK(!lv_obj_has_state(restart_control("Uruchom ponownie"),LV_STATE_DISABLED));screenshot("164-restart-unavailable");
 click("Anuluj");restart_open();CHECK(!find_label(lv_screen_active(),"Restart jest niedostępny. Spróbuj później."));click("Wstecz");
 for(unsigned i=0;i<10;i++){restart_open();click("Anuluj");}
 lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(unsigned i=0;i<40;i++){restart_open();click(i%2?"Anuluj":"Wstecz");CHECK(restart_requests==before+2);}
 lv_mem_monitor(&b);CHECK(a.free_size==b.free_size);printf("Restart UI heap before=%lu after=%lu;40 confirm/cancel/back cycles; backend MODEL\n",(unsigned long)a.free_size,(unsigned long)b.free_size);
 /* Acceptance is deliberately LAST: the production process latch cannot be
  * reset by tests, navigation, synthetic programmatic events, or reopening. */
 restart_open();restart_response=DSR_RESTART_ACCEPTED;click("Uruchom ponownie");
 CHECK(restart_requests==before+3&&find_label(lv_screen_active(),"Przyjęto żądanie restartu."));
 CHECK(lv_obj_has_state(restart_control("Uruchom ponownie"),LV_STATE_DISABLED));CHECK(lv_obj_has_state(restart_control("Anuluj"),LV_STATE_DISABLED));screenshot("165-restart-accepted-model");
 click("Uruchom ponownie");click("Anuluj");CHECK(restart_requests==before+3);
 click("Wstecz");restart_open();CHECK(find_label(lv_screen_active(),"Przyjęto żądanie restartu."));
 CHECK(lv_obj_has_state(restart_control("Uruchom ponownie"),LV_STATE_DISABLED));click("Uruchom ponownie");CHECK(restart_requests==before+3);
 dsr_network_home(NULL);restart_open();click("Uruchom ponownie");CHECK(restart_requests==before+3&&!dsr_activity_restart_pending());
}
static void revoke_open(void)
{dsr_ui_open_settings();tap_row("Panel WWW / bezpieczeństwo");tap_row("Unieważnij parowanie WWW");CHECK(find_label(lv_screen_active(),"Stare cookie utracą dostęp. Trwające operacje mogą się zakończyć. Wi-Fi i dane pozostaną."));}
static void revoke_regression(void)
{
 wifi.connected=false;test_panel_ready(true);test_pair_accept(true);test_workspace_disable();unsigned before=requests;
 const char *warning="Stare cookie utracą dostęp. Trwające operacje mogą się zakończyć. Wi-Fi i dane pozostaną.";
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);test_pairing_state(true,true,false);revoke_open();
  CHECK(find_label(lv_screen_active(),"Dostęp WWW: aktywny.")&&find_label(lv_screen_active(),"Potwierdź unieważnienie przyciskiem poniżej."));
  if(language)CHECK(strcmp(dsr_sc_tr(warning),warning)&&strcmp(dsr_sc_tr("Unieważnij parowanie"),"Unieważnij parowanie"));
  restart_control("Unieważnij parowanie");restart_control("Anuluj");
  lv_area_t warn,status,confirm;lv_obj_get_coords(find_label(lv_screen_active(),warning),&warn);lv_obj_get_coords(find_label(lv_screen_active(),"Potwierdź unieważnienie przyciskiem poniżej."),&status);lv_obj_get_coords(restart_control("Unieważnij parowanie"),&confirm);CHECK(warn.y2<status.y1&&status.y2<confirm.y1);
  screenshot(language==0?"170-revoke-confirm-pl":language==1?"171-revoke-confirm-en":"172-revoke-confirm-de");
  click("Anuluj");CHECK(find_label(lv_screen_active(),"Panel WWW / bezpieczeństwo")&&requests==before);
  revoke_open();click("Wstecz");CHECK(find_label(lv_screen_active(),"Ustawienia pilota")&&requests==before);
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,0);test_pairing_state(false,false,false);revoke_open();CHECK(find_label(lv_screen_active(),"Stan parowania niedostępny."));screenshot("176-revoke-snapshot-unavailable");click("Wstecz");
 test_pairing_state(true,true,true);revoke_open();CHECK(find_label(lv_screen_active(),"Trwałe unieważnienie: niepotwierdzone.")&&!find_label(lv_screen_active(),"Dostęp WWW: aktywny."));screenshot("177-revoke-uncertain");click("Wstecz");
 test_pairing_state(true,true,false);revoke_open();next_submit_result=DSR_BUSY;click("Unieważnij parowanie");CHECK(requests==before&&!in_flight&&find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));
 click("Unieważnij parowanie");CHECK(in_flight&&submitted.kind==DSR_JOB_PAIRING_REVOKE&&requests==before+1);CHECK(!submitted.connection.device.host[0]&&!submitted.connection.credentials.password[0]);
 CHECK(lv_obj_has_state(restart_control("Unieważnij parowanie"),LV_STATE_DISABLED)&&lv_obj_has_state(restart_control("Anuluj"),LV_STATE_DISABLED));click("Unieważnij parowanie");click("Anuluj");CHECK(requests==before+1&&find_label(lv_screen_active(),"Unieważnianie parowania…"));screenshot("173-revoke-pending");
 /* Exercise genuine poll matching with an explicitly synthetic stale MODEL
  * event; it neither completes nor releases the current queued operation. */
 response=(dsr_event_t){.id=submitted.id+123,.result=DSR_OK};stale_event=result_ready=true;tick();CHECK(in_flight&&find_label(lv_screen_active(),"Unieważnianie parowania…"));
 revoke_open();CHECK(find_label(lv_screen_active(),"Unieważnianie parowania…")&&lv_obj_has_state(restart_control("Unieważnij parowanie"),LV_STATE_DISABLED)&&requests==before+1);
 click("Wstecz");dsr_ui_open_settings();tap_row("Panel WWW / bezpieczeństwo");unsigned codes=test_pair_code_requests();
 lv_obj_t *generate=lv_obj_get_parent(find_label(lv_screen_active(),"Wygeneruj kod parowania"));CHECK(lv_obj_has_state(generate,LV_STATE_DISABLED));click("Wygeneruj kod parowania");CHECK(test_pair_code_requests()==codes&&!find_label(lv_screen_active(),"0123456789abcdef"));
 dsr_ui_open_settings();tap_row("Panel WWW / bezpieczeństwo");generate=lv_obj_get_parent(find_label(lv_screen_active(),"Wygeneruj kod parowania"));CHECK(lv_obj_has_state(generate,LV_STATE_DISABLED));
 test_pairing_state(true,false,true);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);CHECK(!in_flight&&requests==before+1);
 CHECK(!lv_obj_has_state(generate,LV_STATE_DISABLED)&&!find_label(lv_screen_active(),"Unieważnianie parowania…")&&find_label(lv_screen_active(),"Trwałe unieważnienie: niepotwierdzone."));
 revoke_open();CHECK(find_label(lv_screen_active(),"Nie potwierdzono trwałego unieważnienia. Spróbuj ponownie.")&&find_label(lv_screen_active(),"Trwałe unieważnienie: niepotwierdzone."));CHECK(!lv_obj_has_state(restart_control("Unieważnij parowanie"),LV_STATE_DISABLED));screenshot("174-revoke-failure");
 click("Unieważnij parowanie");CHECK(in_flight&&requests==before+2);dsr_network_home(NULL);test_pairing_state(true,false,false);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);revoke_open();
 CHECK(find_label(lv_screen_active(),"Parowanie unieważniono. Nowy dostęp wymaga nowego kodu.")&&find_label(lv_screen_active(),"Dostęp WWW: nieaktywny."));screenshot("175-revoke-success");click("Anuluj");
 revoke_open();response=(dsr_event_t){.id=submitted.id,.result=DSR_IO};stale_event=result_ready=true;tick();CHECK(find_label(lv_screen_active(),"Parowanie unieważniono. Nowy dostęp wymaga nowego kodu.")&&!find_label(lv_screen_active(),"Nie potwierdzono trwałego unieważnienia. Spróbuj ponownie."));click("Anuluj");
 codes=test_pair_code_requests();tap_row("Wygeneruj kod parowania");CHECK(test_pair_code_requests()==codes+1&&find_label(lv_screen_active(),"0123456789abcdef"));click("Wstecz");
 for(unsigned i=0;i<10;i++){revoke_open();click("Anuluj");}
 lv_mem_monitor_t a,b;lv_mem_monitor(&a);for(unsigned i=0;i<40;i++){revoke_open();click("Anuluj");CHECK(requests==before+2);}lv_mem_monitor(&b);CHECK(a.free_size==b.free_size);
 printf("Pairing revoke UI heap before=%lu after=%lu;40 open/cancel cycles; snapshot/worker MODEL\n",(unsigned long)a.free_size,(unsigned long)b.free_size);test_pairing_state(true,false,false);
}
static void room_name(const char *name)
{lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_textarea_set_text(field,name);}
static void room_delete_button(void)
{
 lv_obj_t *label=find_label(lv_screen_active(),"Usuń pomieszczenie");CHECK(label);
 lv_obj_t *button=lv_obj_get_parent(label);lv_obj_scroll_to_view(button,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
 lv_area_t a;lv_obj_get_coords(button,&a);CHECK(a.x1>=0&&a.x2<320&&a.y1>=118&&a.y2<420);CHECK(lv_obj_get_height(button)>=44);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static unsigned json_allocations,json_fail_at;
static void *json_alloc(size_t size){json_allocations++;return json_fail_at&&json_allocations==json_fail_at?NULL:malloc(size);}
static void storage_error_regression(void)
{
 test_workspace_disable();test_workspace_enable();wifi.connected=false;tick();
 dsr_config_health_storage(DSR_STORAGE_NONEMPTY,ESP_FAIL);dsr_config_health_workspace(DSR_WORKSPACE_MOUNTING,ESP_FAIL);
 unsigned before=requests;
 for(unsigned fault=1;fault<=4;fault++)for(unsigned surface=0;surface<3;surface++){
  test_workspace_fault(0);dsr_network_rooms();test_workspace_fault(fault);
  tap(surface==2?280:160,surface==0?116:surface==1?380:450);
  CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));CHECK(find_label(lv_screen_active(),"Nie można otworzyć edytora"));CHECK(!find_label(lv_screen_active(),"Zapisz"));
  if(fault==1&&surface==0){screenshot("120-storage-unavailable-room");click("Diagnostyka");CHECK(find_label(lv_screen_active(),"SPIFFS: niepusty, niezamontowany"));CHECK(find_label(lv_screen_active(),"Nie kasuj i nie formatuj pamięci."));screenshot("121-storage-read-only-diagnostics");}
  test_workspace_fault(0);click("Wstecz");CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 }
 for(unsigned fault=1;fault<=4;fault++){
  dsr_network_home(NULL);test_workspace_fault(fault);tap(224,116);CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));if(fault==1)screenshot("122-storage-pencil-error");test_workspace_fault(0);click("Wstecz");CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
 }
 /* Actual cJSON allocator failures: parse, duplicate, new object and field. */
 cJSON_Hooks hooks={.malloc_fn=json_alloc,.free_fn=free};char *json=dsr_workspace_snapshot();CHECK(json);cJSON_InitHooks(&hooks);json_fail_at=json_allocations=0;cJSON *parsed=cJSON_Parse(json);CHECK(parsed);unsigned parse_count=json_allocations;json_allocations=0;cJSON *duplicate=cJSON_Duplicate(parsed,true);CHECK(duplicate);unsigned duplicate_count=json_allocations;cJSON_Delete(parsed);cJSON_Delete(duplicate);cJSON_InitHooks(NULL);free(json);
 unsigned faults[]={1,parse_count+1,parse_count+duplicate_count+1,parse_count+duplicate_count+2};
 for(unsigned i=0;i<4;i++){
  dsr_network_rooms();json_allocations=0;json_fail_at=faults[i];cJSON_InitHooks(&hooks);tap(160,116);cJSON_InitHooks(NULL);json_fail_at=0;
  CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));CHECK(!find_label(lv_screen_active(),"Zapisz"));if(i==1)screenshot("123-storage-baseline-memory-error");click("Wstecz");
 }
 /* All three real pointer targets still open the form with healthy storage. */
 dsr_config_health_storage(DSR_STORAGE_READY,ESP_OK);dsr_config_health_workspace(DSR_WORKSPACE_READY,ESP_OK);
 for(unsigned surface=0;surface<3;surface++){dsr_network_rooms();tap(surface==2?280:160,surface==0?116:surface==1?380:450);CHECK(find_label(lv_screen_active(),"Nowe pomieszczenie"));click("Wstecz");}
 dsr_network_home(NULL);tap(224,116);CHECK(find_label(lv_screen_active(),"Edytuj pilot"));CHECK(!find_label(lv_screen_active(),"Web Panel • parowanie"));
 dsr_ui_open_settings();test_panel_ready(false);click("Panel WWW / bezpieczeństwo");click("Wygeneruj kod parowania");CHECK(find_label(lv_screen_active(),"Nie utworzono kodu. HTTPS niedostępny lub zajęty."));CHECK(!find_label(lv_screen_active(),"0123456789abcdef"));screenshot("124-storage-https-unavailable");test_panel_ready(true);click("Wstecz");
 click("Panel WWW / bezpieczeństwo");click("Wygeneruj kod parowania");CHECK(find_label(lv_screen_active(),"0123456789abcdef"));click("Wstecz");CHECK(requests==before);test_workspace_disable();
 test_workspace_enable();
 for(unsigned language=1;language<3;language++){
  cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(r);dsr_network_rooms();test_workspace_fault(1);tap(160,116);CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna"));screenshot(language==1?"125-storage-error-english":"127-storage-error-german");click("Diagnostyka");CHECK(find_label(lv_screen_active(),"Nie kasuj i nie formatuj pamięci."));screenshot(language==1?"126-storage-diagnostics-english":"128-storage-diagnostics-german");test_workspace_fault(0);click("Wstecz");
 }
 test_workspace_disable();dsr_sc_appearance(DSR_SC_BLUE,false,0);
}
static void rooms_regression(void)
{
 /* The network connection stays intact; only shared layout starts empty. */
 wifi.connected=false;tick();unsigned before=requests;dsr_connection_t bound=submitted.connection;
 test_workspace_disable();test_workspace_enable();test_workspace_photos(true);dsr_network_rooms();
 CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);CHECK(find_label(lv_screen_active(),"Brak pomieszczeń"));screenshot("90-rooms-empty");
 tap(160,380);CHECK(find_label(lv_screen_active(),"Nowe pomieszczenie"));screenshot("91-room-new");
 click("Zapisz");CHECK(find_label(lv_screen_active(),"Podaj nazwę pomieszczenia."));
 room_name("Salon");tap(236,442);dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS&&!find_type(lv_screen_active(),&lv_textarea_class));
 CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);CHECK(find_label(lv_screen_active(),"Salon"));screenshot("92-rooms-created");
 /* Real pointer on the card's 44x44 edit target; it must not open the room. */
 tap(134,168);CHECK(!dsr_mobile_active());CHECK(find_label(lv_screen_active(),"Edytuj pomieszczenie"));room_name("Salon 2");
 lv_obj_t *icon=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(icon);lv_dropdown_set_selected(icon,2);lv_obj_send_event(icon,LV_EVENT_VALUE_CHANGED,NULL);
 click("Wybierz zdjęcie z pamięci");test_workspace_defer(true);tap(236,442);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisywanie…"));screenshot("93-room-saving");
 click("Wstecz");CHECK(find_label(lv_screen_active(),"Poczekaj na zakończenie zapisu."));
 cJSON *r=room_document();CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child,"name")->valuestring,"Salon"));cJSON_Delete(r);
 test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));screenshot("94-room-save-failed");
 click("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);test_workspace_defer(false);
 tap(160,116);room_name("Łazienka Küche");click("Wybierz zdjęcie z pamięci");click("Zmień zdjęcie z pamięci");click("Zapisz");dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);screenshot("95-rooms-photos");
 r=room_document();cJSON *rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms");CHECK(cJSON_GetArraySize(rooms)==2);
 CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(rooms->child,"name")->valuestring,"Salon 2"));CHECK(cJSON_GetObjectItemCaseSensitive(rooms->child,"icon")->valueint==2);
 CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(rooms->child,"photo")->valuestring,dsr_photo_id(0)));
 /* Reordering from Web changes list positions, not which ID gets edited. */
 cJSON *moved=cJSON_DetachItemFromArray(rooms,0);cJSON_AddItemToArray(rooms,moved);publish_document(r);dsr_network_rooms();tap(134,168);
 CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Łazienka Küche"));room_name("Pokój 2");click("Zapisz");dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 r=room_document();rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms");CHECK(cJSON_GetObjectItemCaseSensitive(rooms->child,"id")->valueint==2);CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(rooms->child->next,"name")->valuestring,"Salon 2"));cJSON_Delete(r);
 /* An already-open room also follows ID when a Web reorder is published. */
 tap(60,228);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);r=room_document();rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms");moved=cJSON_DetachItemFromArray(rooms,0);cJSON_AddItemToArray(rooms,moved);publish_document(r);
 char *json=dsr_workspace_snapshot();dsr_mobile_model_t view_model={0};CHECK(dsr_mobile_configure(&view_model,json));free(json);dsr_mobile_update(&view_model);CHECK(find_label(lv_screen_active(),"Pokój 2"));tap(160,116);CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Pokój 2"));click("Wstecz");
 /* Single-room contextual entry and unsaved-change protection. */
 dsr_network_rooms();tap(215,228);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);tap(160,116);room_name("Nie zapisuj");click("Wstecz");CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));screenshot("96-room-discard");click("Zostań");CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Nie zapisuj"));click("Wstecz");click("Odrzuć");CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);
 /* Pending save A / later edit B, followed by an external Web edit. */
 dsr_ui_edit_room(2);room_name("Wersja A");test_workspace_defer(true);click("Zapisz");room_name("Wersja B");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
 test_workspace_defer(false);r=room_document();publish_document(r);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));screenshot("97-room-conflict");click("Zapisz");click("Wstecz");click("Odrzuć");
 /* A Web write may supersede our completed write before the next LCD tick.
  * It must neither report our draft as saved nor trap Back in pending state. */
 dsr_ui_edit_room(2);room_name("Zapis LCD");test_workspace_defer(true);click("Zapisz");test_workspace_finish(true);test_workspace_defer(false);r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child->next,"name",cJSON_CreateString("Zapis Web"));publish_document(r);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));click("Wstecz");CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));click("Odrzuć");
 /* Deleted IDs cannot accidentally edit whichever room occupies that index. */
 dsr_ui_edit_room(999);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 dsr_ui_edit_room(2);room_name("ąąąąąąąąąąąąąąąąąąąąąąąą");click("Zapisz");CHECK(find_label(lv_screen_active(),"Podaj krótszą nazwę pomieszczenia."));click("Wstecz");click("Odrzuć");
 /* Reference protection includes a real binding-shaped definition and scene. */
 r=room_document();cJSON *devices=cJSON_GetObjectItemCaseSensitive(r,"devices");cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"id",7);cJSON_AddStringToObject(d,"name","TEST Shelly");cJSON_AddStringToObject(d,"host",bound.device.host);cJSON_AddNumberToObject(d,"channel",bound.device.channel);cJSON_AddStringToObject(d,"kind","relay");cJSON_AddNumberToObject(d,"room",2);cJSON_AddItemToArray(devices,d);
 cJSON *scene=cJSON_Parse("{\"id\":1,\"name\":\"TEST scene\",\"actions\":[{\"device\":7,\"command\":\"on\"}]}");cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"scenes"),scene);
 cJSON *tiles=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles");cJSON_AddItemToArray(tiles,cJSON_Parse("{\"id\":1,\"type\":\"room\",\"ref\":2,\"span\":1,\"height\":1,\"fields\":255}"));publish_document(r);
 dsr_ui_edit_room(2);room_delete_button();CHECK(find_label(lv_screen_active(),"Pomieszczenie jest używane"));screenshot("98-room-delete-blocked");tap(160,329);click("Wstecz");
 /* Removing dashboard refs alone still leaves the device/scene protected. */
 r=room_document();cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles"),0);publish_document(r);dsr_ui_edit_room(2);room_delete_button();CHECK(find_label(lv_screen_active(),"Pomieszczenie jest używane"));tap(160,329);click("Wstecz");
 /* Explicitly move the device first, then confirm deletion, then durable Save. */
 r=room_document();cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"devices")->child,"room"),1);publish_document(r);
 dsr_ui_edit_room(2);room_delete_button();CHECK(find_label(lv_screen_active(),"Usunąć pomieszczenie?"));screenshot("99-room-delete-confirm");click("Anuluj");room_delete_button();click("Usuń");CHECK(find_label(lv_screen_active(),"Usunięcie pomieszczenia"));
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==2);cJSON_Delete(r);click("Zapisz");dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS&&!find_label(lv_screen_active(),"Zapisz"));
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"devices"))==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"scenes"))==1);d=cJSON_GetObjectItemCaseSensitive(r,"devices")->child;CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(d,"host")->valuestring,bound.device.host));CHECK(cJSON_GetObjectItemCaseSensitive(d,"channel")->valueint==bound.device.channel);cJSON_Delete(r);
 /* New rooms appear in the existing LAN wizard and dashboard tile selector. */
 dsr_ui_edit_dashboard();click("Pulpity");click("Mój pulpit");click("+ Pomieszczenie");click("Zapisz");dsr_ui_workspace_tick();r=room_document();tiles=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles");CHECK(tiles->child&&cJSON_GetObjectItemCaseSensitive(tiles->child,"ref")->valueint==1);cJSON_Delete(r);
 wifi.connected=true;dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":2,\"id\":\"shellyplus1-test\",\"mac\":\"aabbccddeeff\",\"model\":\"TEST model\",\"auth_en\":false}","192.168.1.88");dsr_ui_lan_tick();click("TEST model");test_lan_inspected(DSR_OK,"{\"switch:1\":{\"id\":1,\"output\":false}}");dsr_ui_lan_tick();lv_obj_t *room=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(room);CHECK(strstr(lv_dropdown_get_options(room),"Salon 2"));CHECK(lv_dropdown_get_option_count(room)==2);dsr_ui_lan_reset();wifi.connected=false;
 dsr_network_rooms();for(unsigned i=1;i<8;i++){dsr_ui_add_room();room_name("Pokój TEST");click("Zapisz");dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);}
 dsr_ui_add_room();CHECK(find_label(lv_screen_active(),"Limit: 8 pomieszczeń. Edytuj lub usuń istniejący pokój."));screenshot("100-rooms-limit");click("Wstecz");
 /* PL/EN/DE use the same production form and reference warning. */
 for(unsigned language=1;language<3;language++){
  r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(r);dsr_network_rooms();screenshot(language==1?"101-rooms-english":"103-rooms-german");dsr_ui_edit_room(1);screenshot(language==1?"102-room-edit-english":"104-room-edit-german");room_delete_button();screenshot(language==1?"105-room-blocked-english":"106-room-blocked-german");tap(160,329);click("Wstecz");
 }
 CHECK(requests==before);CHECK(!memcmp(&submitted.connection,&bound,sizeof(bound)));
 dsr_network_rooms();dsr_ui_edit_room(1);click("Wstecz");lv_refr_now(display);lv_mem_monitor_t memory_before,memory_after;lv_mem_monitor(&memory_before);
 for(unsigned i=0;i<40;i++){dsr_ui_edit_room(1);click("Wstecz");}lv_refr_now(display);lv_mem_monitor(&memory_after);CHECK(memory_after.free_size==memory_before.free_size);CHECK(memory_after.free_biggest_size>32000);
 printf("Rooms UI heap before=%u after=%u largest=%u; 40 edit/back cycles, 8 rooms\n",(unsigned)memory_before.free_size,(unsigned)memory_after.free_size,(unsigned)memory_after.free_biggest_size);
 test_workspace_photos(false);dsr_network_home(NULL);tap(188,116);CHECK(find_label(lv_screen_active(),"Bluetooth"));dsr_network_home(NULL);
}
static void p0_generic(const char *collection,const char *name)
{dsr_ui_edit_dashboard();click(collection);if(name)click(name);}
static void p0_visible_delete(const char *text)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label);lv_obj_t *button=lv_obj_get_parent(label);lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(button,&a);
 CHECK(a.x1>=12&&a.x2<308&&a.y1>=106&&a.y2<398&&lv_obj_get_height(button)>=44); /* no scroll-to-view rescue */
 tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static lv_obj_t *p0_modal(const char *name)
{char title[160];snprintf(title,sizeof(title),dsr_sc_tr("Usunąć \"%s\"?"),name);lv_obj_t *label=find_label(lv_screen_active(),title);if(!label){lv_obj_t *description=find_label(lv_screen_active(),"Po potwierdzeniu wybierz Zapisz. Dane zostaną zmienione dopiero po udanym zapisie.");CHECK(description);label=lv_obj_get_child(lv_obj_get_parent(description),0);CHECK(lv_obj_check_type(label,&lv_label_class));}lv_label_long_mode_t mode=lv_label_get_long_mode(label);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_WRAP);CHECK(!strcmp(lv_label_get_text(label),title)&&strstr(lv_label_get_text(label),name));lv_label_set_long_mode(label,mode);lv_obj_update_layout(lv_screen_active());return label;}
static cJSON *p0_data(void)
{cJSON *r=room_document();cJSON_DeleteItemFromObjectCaseSensitive(r,"revision");return r;}
static void p0_editor_regression(void)
{
 wifi.connected=false;dsr_sc_appearance(DSR_SC_BLUE,false,0);dsr_network_home(NULL);test_workspace_disable();test_workspace_enable();
 cJSON *r=room_document();cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"dashboards"),cJSON_Parse("{\"id\":8,\"name\":\"Pulpit drugi\",\"tiles\":[]}"));
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"rooms"),cJSON_Parse("{\"id\":7,\"name\":\"Pusty pokój\",\"icon\":0,\"photo\":\"\"}"));cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"rooms"),cJSON_Parse("{\"id\":9,\"name\":\"Drugi pokój\",\"icon\":0,\"photo\":\"\"}"));publish_document(r);
 /* The first assertion after ACK fails on 0.7.18: it used to retain the
  * entity editor despite a clean, exactly published durable snapshot. */
 p0_generic("Pulpity","Mój pulpit");room_name("Pulpit A");test_workspace_defer(true);unsigned revision=dsr_workspace_revision();click("Zapisz");dsr_ui_workspace_tick();CHECK(find_type(lv_screen_active(),&lv_textarea_class)&&dsr_workspace_revision()==revision);
 p0_visible_delete("Usuń pulpit");CHECK(find_label(lv_screen_active(),"Poczekaj na zakończenie zapisu.")&&!find_label(lv_screen_active(),"Po potwierdzeniu wybierz Zapisz. Dane zostaną zmienione dopiero po udanym zapisie."));
 test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(!find_type(lv_screen_active(),&lv_textarea_class)&&find_label(lv_screen_active(),"Pulpit A")&&find_label(lv_screen_active(),"+ Dodaj wpis"));CHECK(dsr_workspace_revision()==revision+1);screenshot("180-editor-clean-ack-list");
 dsr_ui_select_dashboard();click("Pulpit drugi");dsr_ui_workspace_tick();CHECK(!dsr_mobile_active()&&find_label(lv_screen_active(),"Wybierz pulpit"));test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_HOME);dsr_ui_select_dashboard();click("Pulpit A");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_HOME);
 p0_generic("Pulpity","Pulpit A");room_name("Zapis A");click("Zapisz");room_name("Edycja B");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Edycja B")&&find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
 click("Zapisz");test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze.")&&!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Edycja B"));
 /* A real Web publication at the expected next revision is not our A. */
 click("Zapisz");test_workspace_finish(false);test_workspace_defer(false);r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"name",cJSON_CreateString("WEB dashboard"));publish_document(r);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie.")&&find_type(lv_screen_active(),&lv_textarea_class));
 /* Navigation/redraw after submit consumes no later navigation opportunity. */
 p0_generic("Pulpity","WEB dashboard");room_name("Pulpit A");test_workspace_defer(true);click("Zapisz");click("Wstecz");click("Pulpit drugi");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Pulpit drugi"));test_workspace_defer(false);
 p0_generic("Pulpity","Pulpit A");cJSON *unchanged=p0_data();p0_visible_delete("Usuń pulpit");p0_modal("Pulpit A");CHECK(find_label(lv_screen_active(),"Po potwierdzeniu wybierz Zapisz. Dane zostaną zmienione dopiero po udanym zapisie."));screenshot("181-dashboard-delete-confirm");click("Anuluj");click("Zapisz");dsr_ui_workspace_tick();r=p0_data();CHECK(cJSON_Compare(unchanged,r,true));cJSON_Delete(unchanged);cJSON_Delete(r);
 click("Pulpit A");unchanged=p0_data();p0_visible_delete("Usuń pulpit");test_workspace_defer(true);click("Zapisz");click("Usuń");CHECK(find_label(lv_screen_active(),"Poczekaj na zakończenie zapisu."));test_workspace_finish(true);dsr_ui_workspace_tick();r=p0_data();CHECK(cJSON_Compare(unchanged,r,true));cJSON_Delete(unchanged);cJSON_Delete(r);test_workspace_defer(false);
 click("Pulpit A");p0_visible_delete("Usuń pulpit");p0_modal("Pulpit A");click("Usuń");CHECK(!find_label(lv_screen_active(),"Pulpit A")&&find_label(lv_screen_active(),"Pulpit drugi"));r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"dashboards"))==2);cJSON_Delete(r);
 test_workspace_defer(true);click("Zapisz");dsr_ui_workspace_tick();r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"dashboards"))==2);cJSON_Delete(r);test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Pulpit drugi")&&find_label(lv_screen_active(),"+ Dodaj wpis")&&!find_label(lv_screen_active(),"Pulpity"));test_workspace_defer(false);
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"dashboards"))==1&&cJSON_GetObjectItemCaseSensitive(r,"active_dashboard")->valueint==8);cJSON_Delete(r);
 click("Pulpit drugi");p0_visible_delete("Usuń pulpit");CHECK(find_label(lv_screen_active(),"Zachowaj przynajmniej jeden pulpit."));click("Wstecz");click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Edytuj pilot"));click("Zapisz");dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
 /* Generic rooms use the same clean entity->collection contract. */
 p0_generic("Pomieszczenia","Pusty pokój");room_name("Pokój A");test_workspace_defer(true);click("Zapisz");CHECK(find_type(lv_screen_active(),&lv_textarea_class));test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Pokój A")&&!find_type(lv_screen_active(),&lv_textarea_class));test_workspace_defer(false);
 /* Actual contextual entry from ROOM detail must return to stable ID 7. */
 dsr_network_rooms();tap(60,228);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM);tap(160,116);room_name("Pokój trwały");test_workspace_defer(true);click("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM&&find_label(lv_screen_active(),"Pokój trwały"));test_workspace_defer(false);screenshot("182-room-ack-stable-context");
 /* Each type of existing reference must prevent generic room deletion. */
 r=room_document();cJSON *tiles=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles");cJSON_AddItemToArray(tiles,cJSON_Parse("{\"id\":1,\"type\":\"room\",\"ref\":7,\"span\":1,\"height\":1,\"fields\":255}"));publish_document(r);
 p0_generic("Pomieszczenia","Pokój trwały");p0_visible_delete("Usuń pomieszczenie");CHECK(find_label(lv_screen_active(),"Najpierw usuń powiązane karty lub działania."));click("Wstecz");
 r=room_document();tiles=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"tiles");cJSON_DeleteItemFromArray(tiles,0);cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"devices"),cJSON_Parse("{\"id\":1,\"name\":\"Fixture lamp\",\"kind\":\"light\",\"host\":\"192.168.1.20\",\"channel\":0,\"room\":7}"));cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"scenes"),cJSON_Parse("{\"id\":1,\"name\":\"Fixture scene\",\"actions\":[{\"device\":1,\"command\":\"on\"}]}"));publish_document(r);
 p0_generic("Pomieszczenia","Pokój trwały");p0_visible_delete("Usuń pomieszczenie");CHECK(find_label(lv_screen_active(),"Najpierw usuń powiązane karty lub działania."));
 r=room_document();cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"devices")->child,"room"),0);publish_document(r);
 p0_generic("Pomieszczenia","Pokój trwały");p0_visible_delete("Usuń pomieszczenie");p0_modal("Pokój trwały");click("Anuluj");CHECK(find_type(lv_screen_active(),&lv_textarea_class));p0_visible_delete("Usuń pomieszczenie");click("Usuń");click("Zapisz");dsr_ui_workspace_tick();r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==1&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"devices"))==1&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"scenes"))==1);cJSON_Delete(r);
 /* Fresh reopen reads the acknowledged model, not the deleted draft object. */
 p0_generic("Pomieszczenia",NULL);CHECK(!find_label(lv_screen_active(),"Pokój trwały")&&find_label(lv_screen_active(),"Drugi pokój"));
 click("Drugi pokój");p0_visible_delete("Usuń pomieszczenie");r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child,"name",cJSON_CreateString("Pokój WEB"));publish_document(r);click("Usuń");click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==1&&!strcmp(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child,"name")->valuestring,"Pokój WEB"));cJSON_Delete(r);
 char wide[48];memset(wide,'W',47);wide[47]=0;
 r=room_document();cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"dashboards"),cJSON_Parse("{\"id\":11,\"name\":\"Wide dashboard\",\"tiles\":[]}"));cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child->next,"name",cJSON_CreateString(wide));publish_document(r);
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);CHECK(dsr_sc_language()==language);
  r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child,"name",cJSON_CreateString(wide));publish_document(r);p0_generic("Pomieszczenia",wide);p0_visible_delete("Usuń pomieszczenie");lv_obj_t *title=p0_modal(wide),*description=find_label(lv_screen_active(),"Po potwierdzeniu wybierz Zapisz. Dane zostaną zmienione dopiero po udanym zapisie.");CHECK(description);lv_area_t a,b,c;lv_obj_get_coords(title,&a);lv_obj_get_coords(description,&b);lv_obj_get_coords(lv_obj_get_parent(find_label(lv_screen_active(),"Anuluj")),&c);CHECK(a.y2<b.y1&&b.y2<c.y1&&lv_obj_get_height(title)==44);screenshot(language==0?"183-delete-wide-pl":language==1?"184-delete-wide-en":"185-delete-wide-de");click("Anuluj");
  p0_generic("Pulpity",wide);p0_visible_delete("Usuń pulpit");p0_modal(wide);screenshot(language==0?"186-dashboard-wide-pl":language==1?"187-dashboard-wide-en":"188-dashboard-wide-de");click("Anuluj");
 }
 dsr_network_home(NULL);dsr_sc_appearance(DSR_SC_BLUE,false,0);p0_generic("Pomieszczenia",wide);for(unsigned i=0;i<40;i++){p0_visible_delete("Usuń pomieszczenie");click("Anuluj");tick();}lv_mem_monitor_t before,after;lv_mem_monitor(&before);for(unsigned i=0;i<40;i++){p0_visible_delete("Usuń pomieszczenie");click("Anuluj");tick();}lv_mem_monitor(&after);printf("P0 generic delete heap before=%lu after=%lu;40 cancel cycles, host workspace MODEL\n",(unsigned long)before.free_size,(unsigned long)after.free_size);fflush(stdout);CHECK(before.free_size==after.free_size);
 dsr_network_home(NULL);test_workspace_disable();dsr_sc_appearance(DSR_SC_BLUE,false,0);
}
static void polling_regression(void)
{
 dsr_network_home(NULL);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *name=find_label(lv_screen_active(),"Desk lamp");CHECK(name);
 unsigned before=requests;
 for(unsigned i=0;i<2;i++){
  lv_tick_inc(5000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);
  CHECK(find_label(lv_screen_active(),"Desk lamp")==name);CHECK(find_label(lv_screen_active(),"Nieprzypisane • Włączone"));CHECK(!find_label(lv_screen_active(),"Odczyt stanu…"));
  response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Desk lamp")==name);
 }
 CHECK(requests==before+2);screenshot("70-home-poll-stable");
 lv_tick_inc(5000);tick();CHECK(submitted.kind==DSR_JOB_PROBE);before=requests;
 tap(274,178);CHECK(submitted.kind==DSR_JOB_PROBE&&requests==before);CHECK(find_label(lv_screen_active(),"Nieprzypisane • Wysyłanie polecenia…"));screenshot("71-command-queued-behind-read");
 tap(274,178);CHECK(requests==before); /* disabled duplicate, never lost/replayed */
 deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT&&!submitted.on);CHECK(requests==before+1);CHECK(!find_label(lv_screen_active(),"Nieprzypisane • Wyłączone"));
 response.device.output=false;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Nieprzypisane • Wyłączone"));
 tap(274,178);CHECK(submitted.kind==DSR_JOB_SET_OUTPUT&&submitted.on);response.device.output=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 /* A failed read cancels the queued intent, rather than blindly sending SET. */
 lv_tick_inc(5000);tick();before=requests;tap(274,178);response.device.online=false;deliver(DSR_AUTH_REQUIRED,DSR_WIFI_IDLE);CHECK(requests==before);CHECK(find_label(lv_screen_active(),"Błąd logowania"));
 click("Desk lamp");click("Odśwież stan");response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 lv_obj_t *label=find_label(lv_screen_active(),"230.4 V");CHECK(label);
 lv_tick_inc(5000);tick();CHECK(submitted.kind==DSR_JOB_PROBE);CHECK(find_label(lv_screen_active(),"230.4 V")==label);CHECK(find_label(lv_screen_active(),"Włączone • Online"));
 before=requests;click("Odśwież stan");CHECK(requests==before); /* joins read */
 deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"230.4 V")==label);screenshot("72-details-poll-freshness");
 lv_tick_inc(1000);tick();CHECK(find_label(lv_screen_active(),"Odczyt potwierdzony 1 s temu"));CHECK(find_label(lv_screen_active(),"230.4 V")==label);
 /* A refused queue admission preserves one intent until admission succeeds. */
 lv_tick_inc(5000);tick();before=requests;tap(160,262);CHECK(submitted.kind==DSR_JOB_PROBE);next_submit_result=DSR_BUSY;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(!in_flight&&requests==before);
 tick();CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT);response.device.output=false;deliver(DSR_OK,DSR_WIFI_IDLE);
 lv_tick_inc(5000);tick();CHECK(submitted.kind==DSR_JOB_PROBE);deliver(DSR_OK,DSR_WIFI_IDLE);click("Odśwież stan");deliver(DSR_OK,DSR_WIFI_IDLE);
 /* Losing Wi-Fi while queued cancels intent even if the read later succeeds. */
 lv_tick_inc(5000);tick();before=requests;tap(160,262);wifi.connected=false;tick();CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(requests==before&&!in_flight);
 wifi.connected=true;tick();CHECK(submitted.kind==DSR_JOB_PROBE);click("Odśwież stan");response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(!find_label(lv_screen_active(),"Brak odpowiedzi"));
 dsr_network_home(NULL);
}
static void ble_regression(void)
{
 dsr_network_device(NULL);choose_method("Bluetooth LE");CHECK(find_label(lv_screen_active(),"Odbieranie reklam Bluetooth…"));screenshot("80-ble-start");tick();CHECK(find_label(lv_screen_active(),"Odbieranie reklam Bluetooth…"));screenshot("81-ble-scanning");
 const uint8_t rpc[]={17,7,0x5f,0x44,0x49,0x5f,0x43,0x56,0x53,0x5f,0x43,0x50,0x52,0x5f,0x53,0x4f,0x6d,0x5f};
 const uint8_t blu[]={6,0xff,0xa9,0x0b,0x0b,2,0},bthome[]={6,0x16,0xd2,0xfc,0x40,1,92};
 /* The native worker may keep polling the relay while BLE owns its own task. */
 wifi.connected=true;tick();if(in_flight){response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 lv_tick_inc(5000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(dsr_ui_ble_active());
 test_ble_packet(rpc,sizeof(rpc),1);test_ble_packet(blu,sizeof(blu),2);test_ble_packet(bthome,sizeof(bthome),3);test_ble_finish(DSR_BLE_DONE);tick();
 CHECK(find_label(lv_screen_active(),"Shelly • RPC / konfiguracja"));CHECK(find_label(lv_screen_active(),"Shelly BLU DoorWindow"));CHECK(find_label(lv_screen_active(),"Pozostałe reklamy"));screenshot("82-ble-results");
 click("Shelly BLU DoorWindow");tick();CHECK(find_label(lv_screen_active(),"Wykryto • nie sparowano"));screenshot("83-ble-detail");click("Wstecz");click("Ponów");click("Anuluj");tick();CHECK(find_label(lv_screen_active(),"Skan przerwany"));screenshot("84-ble-cancelled");
 click("Ponów");test_ble_finish(DSR_BLE_DONE);tick();CHECK(find_label(lv_screen_active(),"Nie znaleziono urządzeń BLE"));screenshot("85-ble-empty");
 click("Ponów");test_ble_finish(DSR_BLE_UNAVAILABLE);tick();CHECK(find_label(lv_screen_active(),"Bluetooth niedostępny"));screenshot("86-ble-unavailable");
 for(unsigned language=1;language<3;language++){dsr_sc_appearance(DSR_SC_BLUE,false,language);dsr_ui_ble_open(NULL);click("Ponów");test_ble_packet(blu,sizeof(blu),2);test_ble_finish(DSR_BLE_DONE);tick();screenshot(language==1?"87-ble-english":"88-ble-german");}
 dsr_sc_appearance(DSR_SC_BLUE,false,0);dsr_network_home(NULL);lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(unsigned i=0;i<10;i++){dsr_ui_ble_open(NULL);click("Ponów");click("Anuluj");tick();click("Wstecz");dsr_network_home(NULL);tick();}
 lv_mem_monitor(&b);CHECK(b.free_size+1024>=a.free_size);CHECK(!dsr_ui_ble_active());
 if(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 wifi.connected=false;tick();
 printf("BLE UI heap before=%u after=%u; 10 scan/cancel/exit cycles; host radio fixture only\n",(unsigned)a.free_size,(unsigned)b.free_size);
}
static void bounds(lv_obj_t *obj,int max_y)
{lv_area_t a;lv_obj_get_coords(obj,&a);CHECK(a.x1>=0&&a.x2<320&&a.y1>=0&&a.y2<max_y);}
static void keyboard_regression(void)
{
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);lv_obj_t *root=dsr_ui_screen("Connect to Wi-Fi",false);
  dsr_ui_label(root,"SSID: TEST / DEMO",16,92,288);
  lv_obj_t *field=lv_textarea_create(root);dsr_ui_textarea(field);lv_obj_set_pos(field,16,130);lv_obj_set_size(field,288,44);lv_textarea_set_one_line(field,true);lv_textarea_set_password_mode(field,true);
  lv_obj_t *kb=lv_keyboard_create(root);dsr_ui_keyboard(kb);dsr_ui_keyboard_focus(kb,field);lv_obj_add_event_cb(kb,ready,LV_EVENT_READY,NULL);
  lv_obj_update_layout(root);CHECK(lv_obj_get_height(kb)==240);CHECK(lv_obj_get_style_text_font(kb,LV_PART_ITEMS)==&dsr_roboto_22);bounds(field,240);
  const lv_buttonmatrix_t *matrix=(const lv_buttonmatrix_t *)kb;for(unsigned i=0;i<matrix->btn_cnt;i++){lv_area_t b=matrix->button_areas[i];CHECK(b.x1>=0&&b.x2<320);CHECK(lv_area_get_height(&b)>=40);}
  lv_textarea_set_text(field,"abcdef123!xyz");lv_textarea_set_cursor_pos(field,LV_TEXTAREA_CURSOR_LAST);
  for(unsigned i=0;i<4;i++){key(kb,LV_SYMBOL_LEFT);}
  CHECK(lv_textarea_get_cursor_pos(field)==9);key(kb,"x");CHECK(!strcmp(lv_textarea_get_text(field),"abcdef123x!xyz"));key(kb,LV_SYMBOL_BACKSPACE);CHECK(!strcmp(lv_textarea_get_text(field),"abcdef123!xyz"));
  lv_textarea_set_cursor_pos(field,0);key(kb,LV_SYMBOL_LEFT);CHECK(lv_textarea_get_cursor_pos(field)==0);lv_textarea_set_cursor_pos(field,LV_TEXTAREA_CURSOR_LAST);key(kb,LV_SYMBOL_RIGHT);CHECK(lv_textarea_get_cursor_pos(field)==13);
  lv_textarea_set_text(field,"");key(kb,LV_SYMBOL_UP);key(kb,language==2?"Z":"Y");CHECK(!strcmp(lv_textarea_get_text(field),language==2?"Z":"Y"));key(kb,LV_SYMBOL_UP);
  lv_textarea_set_text(field,"ąäóabc");lv_textarea_set_cursor_pos(field,3);key(kb,"123");key(kb,"ąä");key(kb,"Ż");CHECK(!strcmp(lv_textarea_get_text(field),"ąäóŻabc"));key(kb,LV_SYMBOL_BACKSPACE);CHECK(!strcmp(lv_textarea_get_text(field),"ąäóabc"));key(kb,"ABC");
  bool ascii[127]={0};ascii[' ']=true;const lv_keyboard_mode_t modes[]={LV_KEYBOARD_MODE_TEXT_LOWER,LV_KEYBOARD_MODE_TEXT_UPPER,LV_KEYBOARD_MODE_SPECIAL,LV_KEYBOARD_MODE_USER_1};
  for(unsigned m=0;m<4;m++){lv_keyboard_set_mode(kb,modes[m]);matrix=(const lv_buttonmatrix_t *)kb;for(unsigned i=0;i<matrix->btn_cnt;i++){const unsigned char *s=(const unsigned char *)lv_keyboard_get_button_text(kb,i);if(s[0]>=33&&s[0]<127&&!s[1])ascii[s[0]]=true;}}
  for(unsigned i=32;i<127;i++)CHECK(ascii[i]);
  lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_TEXT_LOWER);screenshot(language==0?"30-keyboard-pl":language==1?"31-keyboard-en":"32-keyboard-de");
  lv_textarea_set_accepted_chars(field,"0123456789.");lv_textarea_set_text(field,"192.168.1.7");dsr_ui_keyboard_focus(kb,field);CHECK(lv_keyboard_get_mode(kb)==LV_KEYBOARD_MODE_NUMBER);key(kb,LV_SYMBOL_LEFT);key(kb,"0");CHECK(!strcmp(lv_textarea_get_text(field),"192.168.1.07"));screenshot(language==0?"33-keyboard-ip":language==1?"34-keyboard-ip-en":"35-keyboard-ip-de");
  unsigned before=ready_count;key(kb,"Gotowe");CHECK(ready_count==before+1);CHECK(lv_textarea_get_password_mode(field));
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,0);lv_obj_clean(lv_screen_active());
}

static void multi_device_regression(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,false,0);wifi.connected=false;tick();
 if(in_flight){response.device=submitted.connection.device;response.device.online=false;deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 test_workspace_enable();dsr_network_home(NULL);tick();wifi.connected=true;tick();
 dsr_connection_t a={.device={.host="192.168.1.50",.name="Relay A",.device_id="shelly-A",.generation=DSR_GEN2_PLUS,.online=true,.has_switch=true,.has_power=true,.power_w=11,.output=false},.credentials={.username="admin",.password="first-private"},.telemetry={.has_voltage=true,.voltage_v=230.1,.has_current=true,.current_a=.05}};
 dsr_connection_t b=a;strcpy(b.device.host,"192.168.1.51");strcpy(b.device.name,"Relay B");strcpy(b.device.device_id,"shelly-B");strcpy(b.credentials.password,"second-private");b.device.power_w=44;b.telemetry.voltage_v=240.2;b.telemetry.current_a=.19;
 CHECK(dsr_network_bind_verified(&a,0)==DSR_OK);response=(dsr_event_t){.device=a.device,.telemetry=a.telemetry};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Relay A"));
 CHECK(dsr_network_bind_verified(&b,0)==DSR_OK);response=(dsr_event_t){.device=b.device,.telemetry=b.telemetry};deliver(DSR_OK,DSR_WIFI_IDLE);
 char *json=dsr_workspace_snapshot();dsr_mobile_model_t layout={0};CHECK(json&&dsr_mobile_configure(&layout,json));CHECK(!strstr(json,"first-private")&&!strstr(json,"second-private")&&!strstr(json,"device_id"));free(json);
 CHECK(layout.device_count==2&&find_label(lv_screen_active(),"Relay A")&&find_label(lv_screen_active(),"Relay B"));uint32_t id_a=layout.devices[0].id;
 screenshot("200-two-real-bindings-fixture");click("Relay B");CHECK(find_label(lv_screen_active(),"240.2 V")&&find_label(lv_screen_active(),"44.0 W"));
 lv_tick_inc(5000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&!strcmp(submitted.connection.device.device_id,"shelly-A"));dsr_job_t a_poll=submitted;
 unsigned before=requests;lv_obj_t *power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);CHECK(requests==before&&find_label(lv_screen_active(),"Wysyłanie polecenia…"));
 response=(dsr_event_t){.device=a_poll.connection.device};response.device.online=false;deliver(DSR_OFFLINE,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT&&!strcmp(submitted.connection.device.device_id,"shelly-B")&&submitted.on&&requests==before+1);
 response=(dsr_event_t){.device=b.device,.telemetry=b.telemetry};response.device.output=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"240.2 V")&&find_label(lv_screen_active(),"Włączone • Online"));screenshot("201-B-readback-A-offline");
 dsr_network_home(NULL);CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));screenshot("202-one-offline-other-online");
 dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":2,\"id\":\"shelly-A\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-001X16EU\",\"auth_en\":false}","192.168.1.60");dsr_ui_lan_tick();CHECK(find_label(lv_screen_active(),"Już dodane"));screenshot("203-already-added-LAN");dsr_network_home(NULL);
 strcpy(a.device.host,"192.168.1.60");CHECK(dsr_network_bind_verified(&a,0)==DSR_OK);response=(dsr_event_t){.device=a.device,.telemetry=a.telemetry,.layout_id=id_a};deliver(DSR_OK,DSR_WIFI_IDLE);
 json=dsr_workspace_snapshot();CHECK(json&&dsr_mobile_configure(&layout,json));free(json);CHECK(layout.device_count==2&&layout.devices[0].id==id_a&&!strcmp(layout.devices[0].host,"192.168.1.60")&&!strcmp(layout.devices[1].host,"192.168.1.51"));
 click("Relay A");CHECK(find_label(lv_screen_active(),"230.1 V")&&find_label(lv_screen_active(),"11.0 W"));screenshot("204-A-DHCP-stable-card-ID");
 wifi.connected=false;tick();dsr_network_home(NULL);
}
#include "review_multi_regression.inc"
#include "bind_publication_regression.inc"
#include "device_settings_regression.inc"
#include "history_regression.inc"
#include "actions_regression.inc"
static const dsr_ap_identity_t ap_identity={.id="shellyplus1-112233445566",.mac="112233445566",.model="SNSW-001X16EU",.ssid="ShellyPlus1-112233445566",.generation=2};
static void ap_pick(void)
{
 wifi.connected=false;tick();if(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 wifi.connected=true;dsr_ui_ap_open(NULL);unsigned before=requests;CHECK(dsr_ap_sensitive()&&!in_flight);
 click("Szukaj AP");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN&&requests==before+1&&!submitted.ap_confirmed);
 response=(dsr_event_t){.scan={.count=3,.items={{.ssid="Home Wi-Fi",.security=DSR_WIFI_WPA2},{.ssid="ShellyPlus1-112233445566",.security=DSR_WIFI_OPEN,.bssid={1,2,3,4,5,6},.channel=6},{.ssid="ShellyPlus1-AABBCCDDEEFF",.security=DSR_WIFI_WPA2}}}};
 deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Fabryczne AP Shelly")&&!find_label(lv_screen_active(),"Home Wi-Fi"));
 before=requests;click("ShellyPlus1-112233445566");CHECK(!in_flight&&requests==before&&find_label(lv_screen_active(),"Połączyć z AP?"));
}
static void ap_disclose(void)
{
 click("Połącz z AP");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_INSPECT&&submitted.ap_confirmed&&submitted.ap.bssid[0]==1);
 CHECK(!dsr_activity_restart_reserve());test_ap_temporary(true);response=(dsr_event_t){.ap_identity=ap_identity};deliver(DSR_OK,DSR_WIFI_CONNECTED);
 CHECK(!in_flight&&find_label(lv_screen_active(),"Przekazać dane Wi-Fi?")&&find_label(lv_screen_active(),ap_identity.id));CHECK(!dsr_wifi_connected());
 CHECK(!dsr_activity_restart_reserve());
}
static void ap_cancel_finish(void)
{
 click("Anuluj");tick();CHECK(in_flight&&submitted.kind==DSR_JOB_AP_RESTORE);test_ap_temporary(false);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);
 CHECK(!dsr_ui_ap_active()&&!dsr_ap_sensitive());wifi.connected=false;tick();if(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
}
static void ap_regression(void)
{
 dsr_settings_t original=stored;dsr_sc_appearance(DSR_SC_BLUE,false,0);
 /* Authenticated WWW can queue only opening: a busy worker defers consume,
  * then the actual LCD intro appears without scanning or sharing Wi-Fi. */
 wifi.connected=false;dsr_network_home(NULL);tick();
 if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 wifi.connected=true;tick();CHECK(test_ap_handoff_available());unsigned ap_scans=job_counts[DSR_JOB_AP_SCAN];
 test_ap_handoff_request();bool deferred=in_flight;tick();
 if(deferred){CHECK(!dsr_ui_ap_active());response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 tick();CHECK(dsr_ui_ap_active()&&find_label(lv_screen_active(),"Dodaj przez Wi-Fi AP")&&job_counts[DSR_JOB_AP_SCAN]==ap_scans);
 click("Anuluj");CHECK(!dsr_ui_ap_active());wifi.connected=false;tick();

 ap_pick();screenshot("220-ap-connect-consent-pl");ap_disclose();screenshot("221-ap-disclose-pl");unsigned before=job_counts[DSR_JOB_AP_PROVISION];ap_cancel_finish();CHECK(job_counts[DSR_JOB_AP_PROVISION]==before); /* never PROVISION before the second physical consent */
 /* Hide/reopen while inspection is pending: no new session, no old ACK disclosure. */
 ap_pick();click("Połącz z AP");unsigned cancelled=test_ap_cancels();dsr_network_home(NULL);dsr_ui_ap_open(NULL);screenshot("229-ap-reopen-debug");CHECK(test_ap_cancels()>cancelled&&find_label(lv_screen_active(),"Kończenie poprzedniej operacji i przywracanie domowego Wi-Fi."));
 test_ap_temporary(true);response=(dsr_event_t){.ap_identity=ap_identity};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(!find_label(lv_screen_active(),"Przekazać dane Wi-Fi?"));CHECK(in_flight&&submitted.kind==DSR_JOB_AP_RESTORE);
 /* A failed SDK restore stays masked and holds the session lease. */
 response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);CHECK(dsr_wifi_temporary_active()&&!dsr_wifi_connected()&&!dsr_activity_restart_reserve());CHECK(find_label(lv_screen_active(),"Nie przywrócono połączenia. Ustawienia zapisane bez zmian. Anuluj ponawia powrót do domu."));screenshot("222-ap-restore-failed-pl");ap_cancel_finish();
 /* Timeout after consent also restores, without starting a credential POST. */
 ap_pick();ap_disclose();before=requests;lv_tick_inc(120001);tick();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_AP_RESTORE&&requests==before+1);test_ap_temporary(false);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(!dsr_ui_ap_active());wifi.connected=false;tick();
 /* A write timeout is uncertain: restore, verify the actual LAN identity+STA,
  * then use the genuine component/name/room/channel BIND form. Never replay POST. */
 ap_pick();ap_disclose();click("Przekaż Wi-Fi");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_PROVISION&&submitted.ap_confirmed&&!strcmp(submitted.ap_identity.id,ap_identity.id));before=requests;
 test_ap_temporary(false);response=(dsr_event_t){.ap_write_result=DSR_OFFLINE};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(find_label(lv_screen_active(),"Sprawdź w domowej sieci"));
 test_lan_found("{\"gen\":2,\"id\":\"shellyplus1-112233445566\",\"mac\":\"112233445566\",\"model\":\"SNSW-001X16EU\",\"auth_en\":false}","192.168.1.77");next_submit_result=DSR_BUSY;tick();CHECK(!in_flight&&requests==before);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_AP_VERIFY&&requests==before+1&&!strcmp(submitted.connection.device.host,"192.168.1.77"));
 response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(!find_label(lv_screen_active(),"Nazwa i pomieszczenie"));test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false}}");tick();
 CHECK(find_label(lv_screen_active(),"Nazwa i pomieszczenie")&&!dsr_ui_ap_active()&&!dsr_ap_sensitive());CHECK(requests==before+1);screenshot("223-ap-verified-lan-bind-form");wifi.connected=false;dsr_network_home(NULL);tick();
 for(unsigned language=1;language<=2;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);ap_pick();screenshot(language==1?"224-ap-connect-consent-en":"226-ap-connect-consent-de");ap_disclose();screenshot(language==1?"225-ap-disclose-en":"227-ap-disclose-de");
  lv_obj_t *label=find_label(lv_screen_active(),"Przekaż nazwę i hasło zapisanej domowej sieci temu urządzeniu. Otwarty AP i HTTP nie zapewniają kryptograficznego uwierzytelnienia.");CHECK(label);bounds(label,324);ap_cancel_finish();
 }
 /* Errors after a possible POST never claim that Wi-Fi details were unsent. */
 dsr_sc_appearance(DSR_SC_BLUE,false,0);ap_pick();ap_disclose();click("Przekaż Wi-Fi");test_ap_temporary(false);response=(dsr_event_t){.ap_write_result=DSR_IDENTITY_MISMATCH};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(find_label(lv_screen_active(),"Tożsamość urządzenia nie zgadza się. Dalsze wysyłanie zablokowane."));screenshot("228-ap-identity-error");click("Anuluj");wifi.connected=false;tick();
 /* Hidden restore failure retains cleanup ownership and retries after 1 s;
  * a delayed PROVISION ACK cannot reopen the wizard or start LAN discovery. */
 ap_pick();ap_disclose();click("Przekaż Wi-Fi");dsr_network_home(NULL);before=job_counts[DSR_JOB_AP_VERIFY];response=(dsr_event_t){.ap_write_result=DSR_OK};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(in_flight&&submitted.kind==DSR_JOB_AP_RESTORE);
 response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_DRIVER_ERROR);CHECK(!in_flight&&dsr_ui_ap_active()&&!dsr_activity_restart_reserve());tick();CHECK(!in_flight);lv_tick_inc(1000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_AP_RESTORE);
 test_ap_temporary(false);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(!dsr_ui_ap_active()&&job_counts[DSR_JOB_AP_VERIFY]==before);wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 /* DHCP-only restore failure releases ownership once the SDK home profile is
  * installed, with no false BIND or provisioning replay. */
 ap_pick();ap_disclose();dsr_network_home(NULL);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_AP_RESTORE);test_ap_temporary(false);wifi.connected=false;response=(dsr_event_t){0};deliver(DSR_OFFLINE,DSR_WIFI_TIMED_OUT);CHECK(!dsr_ui_ap_active()&&dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 /* Relative deadlines also work after 24.9 days of LVGL uptime. */
 lv_tick_inc(UINT32_C(0x80001000)-lv_tick_get());ap_pick();ap_disclose();ap_cancel_finish();
 ap_pick();ap_disclose();click("Przekaż Wi-Fi");test_ap_temporary(false);response=(dsr_event_t){.ap_write_result=DSR_OK};deliver(DSR_OK,DSR_WIFI_CONNECTED);unsigned cancels=test_lan_cancels();click("Anuluj");CHECK(test_lan_cancels()>cancels);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_AP_RESTORE);test_ap_temporary(false);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 CHECK(!memcmp(&original,&stored,sizeof(stored)));CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("AP LCD: two consents, delayed ACK/reopen, failed restore, cancellation, uncertain POST and verified LAN handoff; radio remains a host fixture\n");
}
#include "ble_rpc_regression.inc"
#include "onboarding_regression.inc"
int main(int argc,char **argv)
{
    CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);keyboard_regression();
    dsr_ui_init();tick();CHECK(in_flight&&submitted.kind==DSR_JOB_WIFI_SCAN);screenshot("01-scanning");
    response.scan=(dsr_wifi_scan_t){.count=4,.items={
        {.ssid="Home Wi-Fi",.rssi=-39,.security=DSR_WIFI_WPA2},
        {.ssid="A very long network name 12345678",.rssi=-58,.security=DSR_WIFI_WPA2_WPA3},
        {.ssid="Guest network",.rssi=-64,.security=DSR_WIFI_OPEN},
        {.ssid="Office enterprise",.rssi=-72,.security=DSR_WIFI_UNSUPPORTED}}};
    deliver(DSR_OK,DSR_WIFI_IDLE);screenshot("02-network-list");
    click("Home Wi-Fi");screenshot("03-password-keyboard");
    lv_obj_t *password=find_type(lv_screen_active(),&lv_textarea_class);CHECK(password!=NULL);
    CHECK(lv_textarea_get_password_mode(password));bounds(password,208);
    lv_obj_t *button=find_label(lv_screen_active(),"Connect");CHECK(button!=NULL);bounds(lv_obj_get_parent(button),272);
    lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb!=NULL);bounds(kb,480);
    lv_color_t foreground=lv_obj_get_style_text_color(password,LV_PART_MAIN);
    lv_color_t background=lv_obj_get_style_bg_color(password,LV_PART_MAIN);
    CHECK(foreground.red>200&&foreground.green>200&&foreground.blue>200);
    CHECK(background.red<60&&background.green<60&&background.blue<80);
    click("Connect");CHECK(!in_flight);CHECK(find_label(lv_screen_active(),"Use 8-63 characters or a 64-digit hex key.")!=NULL);
    lv_textarea_set_text(password,"not-a-real-password");unsigned joins=requests;key(kb,"Gotowe");CHECK(requests==joins+1);CHECK(in_flight&&submitted.kind==DSR_JOB_WIFI_CONNECT);
    CHECK(!strcmp(submitted.settings.ssid,"Home Wi-Fi"));screenshot("04-connecting");
    deliver(DSR_OFFLINE,DSR_WIFI_BAD_PASSWORD);CHECK(find_type(lv_screen_active(),&lv_keyboard_class)!=NULL);
    CHECK(find_label(lv_screen_active(),"Authentication failed. Check the password.")!=NULL);screenshot("05-password-error");
    click("Connect");wifi=(dsr_wifi_info_t){.connected=true,.status=DSR_WIFI_CONNECTED,.ssid="Home Wi-Fi",.ip="192.168.1.42"};
    deliver(DSR_OK,DSR_WIFI_CONNECTED);char ip_label[64];snprintf(ip_label,sizeof(ip_label),dsr_sc_tr("IP address: %s"),"192.168.1.42");CHECK(find_label(lv_screen_active(),ip_label)!=NULL);screenshot("06-connected");
    click("Continue");CHECK(find_label(lv_screen_active(),"Brak urządzeń")!=NULL);
    CHECK(find_label(lv_screen_active(),"ON")==NULL);screenshot("07-home");
    dsr_network_device(NULL);screenshot("14-add-device-wizard");choose_method("Dodaj ręcznie (IP)");screenshot("08-device-form");
    password=find_type(lv_screen_active(),&lv_textarea_class);CHECK(password!=NULL);
    foreground=lv_obj_get_style_text_color(password,LV_PART_MAIN);CHECK(foreground.red>200);
    dsr_network_wifi(NULL);tick();deliver(DSR_OK,DSR_WIFI_IDLE);click("Guest network");
    CHECK(find_type(lv_screen_active(),&lv_keyboard_class)==NULL);screenshot("09-open-network");
    click("Connect");CHECK(!submitted.settings.password[0]);click("Cancel");deliver(DSR_OFFLINE,DSR_WIFI_CANCELLED);
    CHECK(find_label(lv_screen_active(),"Home Wi-Fi")!=NULL);screenshot("10-cancelled");
    dsr_wifi_scan_t many={.count=20};
    for(unsigned i=0;i<20;i++){snprintf(many.items[i].ssid,33,"Network %02u",i);many.items[i].security=DSR_WIFI_WPA2;many.items[i].rssi=-40-(int)i;}
    lv_mem_monitor_t before,after;
    dsr_network_home(NULL);tick();
    lv_mem_monitor(&before);
    for(unsigned i=0;i<60;i++) {
        dsr_network_wifi(NULL);tick();response.scan=many;deliver(DSR_OK,DSR_WIFI_IDLE);
        click("Network 00");click("Back");click("Home");
    }
    tick();lv_mem_monitor(&after);
    printf("UI heap before=%u after=%u largest=%u\n",(unsigned)before.free_size,(unsigned)after.free_size,(unsigned)after.free_biggest_size);
    fflush(stdout);CHECK(after.free_size+1024>=before.free_size);
    test_workspace_enable(); /* Fresh bindings require durable shared ACK. */
    dsr_network_device(NULL);choose_method("Dodaj ręcznie (IP)");password=find_type(lv_screen_active(),&lv_textarea_class);
    lv_textarea_set_text(password,"192.168.1.77");click("Check & save device");CHECK(submitted.kind==DSR_JOB_BIND);
    response.device=(dsr_device_t){.host="192.168.1.77",.name="Desk lamp",.device_id="shellyplus1-AABBCCDDEEFF",
        .generation=DSR_GEN2_PLUS,.online=true,.has_switch=true,.output=true,.has_power=true,.power_w=4.1};
    response.telemetry=(dsr_telemetry_t){.has_voltage=true,.has_current=true,.voltage_v=230.4,.current_a=0.018};
    deliver(DSR_OK,DSR_WIFI_IDLE);screenshot("11-configured-switch");test_workspace_disable();
    polling_regression();
    click("Desk lamp");CHECK(find_label(lv_screen_active(),"230.4 V"));CHECK(find_label(lv_screen_active(),"0.018 A"));screenshot("50-real-metrics-fixture");
    lv_obj_t *power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT);
    response.device.online=false;deliver(DSR_AUTH_REQUIRED,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Błąd logowania"));CHECK(!find_label(lv_screen_active(),"230.4 V"));screenshot("51-command-auth-error");
    dsr_network_home(NULL);screenshot("53-card-auth-error");CHECK(find_label(lv_screen_active(),"Błąd logowania"));click("Desk lamp");
    click("Odśwież stan");CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"230.4 V"));
    power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);response.device.online=false;deliver(DSR_OFFLINE,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));screenshot("52-command-timeout");
    lv_tick_inc(10000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));
    click("Odśwież stan");CHECK(submitted.kind==DSR_JOB_PROBE);deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(!find_label(lv_screen_active(),"Brak odpowiedzi"));
    power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);response.device.online=false;deliver(DSR_BAD_RESPONSE,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Stan niepotwierdzony"));screenshot("54-readback-unconfirmed");click("Odśwież stan");CHECK(submitted.kind==DSR_JOB_PROBE);response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);
    dsr_network_home(NULL);
    wifi.connected=false;tick();screenshot("12-switch-offline");
    CHECK(find_label(lv_screen_active(),"Desk lamp")!=NULL);
    click("Desk lamp");CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);
    CHECK(find_label(lv_screen_active(),"Offline • stan niepotwierdzony")!=NULL);
    ble_regression();
    dsr_ui_open_settings();screenshot("40-settings-menu");
    click("System");click("Podgląd rolety • DEMO");CHECK(find_label(lv_screen_active(),"Pozycja 65%"));CHECK(find_label(lv_screen_active(),"DEMO • dane przykładowe"));screenshot("42-cover-preview-on-device");
    unsigned before_preview=submitted.id;click("Otwórz");CHECK(find_label(lv_screen_active(),"Otwarte 100%"));click("Zamknij");CHECK(find_label(lv_screen_active(),"Zamknięte 0%"));click("Stop");CHECK(submitted.id==before_preview);tap(20,20);CHECK(!find_label(lv_screen_active(),"DEMO • dane przykładowe"));CHECK(find_label(lv_screen_active(),"Desk lamp"));
    dsr_ui_open_settings();click("Ekran i komfort");screenshot("41-display-settings");
    p0_editor_regression();test_workspace_enable();dsr_ui_edit_dashboard();
    CHECK(find_label(lv_screen_active(),"Edytuj pilot"));screenshot("13-workspace-editor");
    click("Pomieszczenia");click("+ Dodaj wpis");
    lv_obj_t *name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);lv_textarea_set_text(name,"Salon testowy");click("Zapisz");
    char *configuration=dsr_workspace_snapshot();CHECK(configuration);CHECK(dsr_config_revision(configuration)==2);
    dsr_mobile_model_t model={0};CHECK(dsr_mobile_configure(&model,configuration));free(configuration);
    CHECK(model.room_count==1);CHECK(!strcmp(model.rooms[0].name,"Salon testowy"));
    dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Salon testowy")&&!find_type(lv_screen_active(),&lv_textarea_class));click("Salon testowy");name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);
    test_workspace_defer(true);lv_textarea_set_text(name,"Wersja A");click("Zapisz");
    lv_textarea_set_text(name,"Wersja B");click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisywanie…"));
    test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
    screenshot("20-workspace-save-race");
    CHECK(!strcmp(lv_textarea_get_text(name),"Wersja B"));configuration=dsr_workspace_snapshot();CHECK(strstr(configuration,"Wersja A"));CHECK(!strstr(configuration,"Wersja B"));free(configuration);
    click("Zapisz");test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));CHECK(!strcmp(lv_textarea_get_text(name),"Wersja B"));
    click("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Wersja B")&&!find_type(lv_screen_active(),&lv_textarea_class));configuration=dsr_workspace_snapshot();CHECK(dsr_config_revision(configuration)==4);CHECK(strstr(configuration,"Wersja B"));free(configuration);click("Wersja B");
    test_workspace_defer(false);configuration=dsr_workspace_snapshot();CHECK(dsr_workspace_submit(configuration,4));free(configuration);dsr_ui_workspace_tick();
    CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));
    dsr_network_home(NULL);dsr_ui_workspace_tick();
    wifi.connected=false;dsr_ui_lan_open(NULL);tick();CHECK(find_label(lv_screen_active(),"Najpierw połącz Wi-Fi"));screenshot("19-lan-no-wifi");
    wifi.connected=true;dsr_network_device(NULL);choose_method("Skanuj sieć Wi-Fi");tick();CHECK(dsr_ui_lan_active());screenshot("15-lan-scanning");
    const char info[]="{\"gen\":2,\"id\":\"shellyplus2pm-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-002P16EU\",\"auth_en\":true}";
    test_lan_found(info,"192.168.1.88");tick();CHECK(find_label(lv_screen_active(),"SNSW-002P16EU"));screenshot("16-lan-results");click("SNSW-002P16EU");
    unsigned skip=1;password=nth_type(lv_screen_active(),&lv_textarea_class,&skip);CHECK(password);
    lv_obj_send_event(password,LV_EVENT_CLICKED,NULL);lv_obj_update_layout(lv_screen_active());bounds(password,240);screenshot("36-lan-password-keyboard");key(find_type(lv_screen_active(),&lv_keyboard_class),"Gotowe");
    lv_textarea_set_text(password,"wrong-password");click("Sprawdź");
    test_lan_inspected(DSR_AUTH_REQUIRED,"{}");tick();CHECK(find_label(lv_screen_active(),"Błąd logowania. Sprawdź dane."));screenshot("17-lan-auth-error");
    skip=1;password=nth_type(lv_screen_active(),&lv_textarea_class,&skip);CHECK(password);lv_textarea_set_text(password,"correct-password");click("Sprawdź");
    const char components[]="{\"switch:1\":{\"id\":1,\"output\":false},\"switch:4\":{\"id\":4,\"output\":true}}";
    test_lan_inspected(DSR_OK,components);tick();CHECK(find_label(lv_screen_active(),"Kanał z odpowiedzi API"));screenshot("18-lan-components");
    name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);lv_textarea_set_text(name,"Światło testowe");
    skip=0;lv_obj_t *room=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);CHECK(room);lv_dropdown_set_selected(room,1);
    skip=1;lv_obj_t *channels=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);CHECK(channels);CHECK(lv_dropdown_get_option_count(channels)==2);
    click("Zapisz");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.channel==1);CHECK(!strcmp(submitted.connection.device.device_id,"shellyplus2pm-aabbccddeeff"));
    response.device=submitted.connection.device;response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
    configuration=dsr_workspace_snapshot();CHECK(configuration);CHECK(dsr_mobile_configure(&model,configuration));free(configuration);CHECK(model.device_count>=1&&model.devices[model.device_count-1].room==0&&model.devices[model.device_count-1].channel==1);
    dsr_ui_lan_reset();storage_error_regression();rooms_regression();test_workspace_disable();battery_regression();display_regression();global_settings_regression();revoke_regression();restart_regression();
    multi_device_regression();device_settings_regression();history_regression();actions_regression();bind_rebind_regression();review_multi_regression();review_deleted_migration_regression();
    ap_regression();native_ota_regression();rpc_regression();onboarding_regression();
    printf("PASS: %u real LVGL UI checks; %u screenshots; keyboard, LAN, BLE, contextual rooms, stable IDs, references, CAS/save regression; 60 navigation + 40 room edit cycles\n",checks,screenshots);
    lv_deinit();return 0;
}
