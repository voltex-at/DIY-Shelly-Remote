/* Actual LVGL + actual UI source. Only platform/radio are fixture providers.
 * Screenshots are rendered pixels, not HTML mockups or simulated device data
 * included in firmware. */
#include "dsr_ui.h"
#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_board.h"
#include "dsr_workspace.h"
#include "esp_app_desc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
void test_workspace_enable(void);
void test_workspace_disable(void);
#define CHECK(x) do {checks++; if (!(x)) {fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); abort();}} while(0)
static dsr_job_t submitted;
static dsr_event_t response;
static bool in_flight, result_ready;
static dsr_settings_t stored = {.brightness = 70, .idle_seconds = 60};
static dsr_wifi_info_t wifi;
static unsigned char canvas[320 * 480 * 3];
static uint16_t draw_buffer[320 * 40];
static lv_display_t *display;
static const char *output;

const esp_app_desc_t *esp_app_get_description(void) {static const esp_app_desc_t d = {.version="0.3.0-wifi-dev"}; return &d;}
esp_err_t dsr_board_set_backlight(uint8_t value) {(void)value; return ESP_OK;}
void dsr_board_sleep_display(void) {}
bool dsr_board_touch_available(void) {return true;}
esp_err_t dsr_settings_load(dsr_settings_t *out) {*out=stored; return ESP_OK;}
esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found) {memset(out,0,sizeof(*out)); *found=false; return ESP_OK;}
bool dsr_wifi_connected(void) {return wifi.connected;}
void dsr_wifi_get_info(dsr_wifi_info_t *out) {*out=wifi;}
void dsr_wifi_cancel_connect(void) {CHECK(submitted.kind==DSR_JOB_WIFI_CONNECT);}
dsr_result_t dsr_worker_submit(const dsr_job_t *job)
{CHECK(!in_flight); submitted=*job; in_flight=true; return DSR_OK;}
bool dsr_worker_receive(dsr_event_t *out)
{if(!result_ready)return false; *out=response; result_ready=false; in_flight=false; return true;}

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
static void tick(void) {lv_tick_inc(250);lv_timer_handler();lv_refr_now(display);}
static void screenshot(const char *name)
{
    lv_obj_update_layout(lv_screen_active()); lv_refr_now(display);
    char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",output,name);
    FILE *f=fopen(path,"wb");CHECK(f!=NULL);fprintf(f,"P6\n320 480\n255\n");
    CHECK(fwrite(canvas,1,sizeof(canvas),f)==sizeof(canvas));fclose(f);
}
static lv_obj_t *find_label(lv_obj_t *root,const char *text)
{
    if(lv_obj_check_type(root,&lv_label_class)&&!strcmp(lv_label_get_text(root),text))return root;
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
    lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label!=NULL);
    (void)lv_obj_send_event(lv_obj_get_parent(label),LV_EVENT_CLICKED,NULL);
    lv_obj_update_layout(lv_screen_active());
}
static void deliver(dsr_result_t result,dsr_wifi_status_t detail)
{response.id=submitted.id;response.result=result;response.wifi_detail=detail;result_ready=true;tick();}
static void bounds(lv_obj_t *obj,int max_y)
{lv_area_t a;lv_obj_get_coords(obj,&a);CHECK(a.x1>=0&&a.x2<320&&a.y1>=0&&a.y2<max_y);}

int main(int argc,char **argv)
{
    CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
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
    lv_textarea_set_text(password,"not-a-real-password");click("Connect");CHECK(in_flight&&submitted.kind==DSR_JOB_WIFI_CONNECT);
    CHECK(!strcmp(submitted.settings.ssid,"Home Wi-Fi"));screenshot("04-connecting");
    deliver(DSR_OFFLINE,DSR_WIFI_BAD_PASSWORD);CHECK(find_type(lv_screen_active(),&lv_keyboard_class)!=NULL);
    CHECK(find_label(lv_screen_active(),"Authentication failed. Check the password.")!=NULL);screenshot("05-password-error");
    click("Connect");wifi=(dsr_wifi_info_t){.connected=true,.status=DSR_WIFI_CONNECTED,.ssid="Home Wi-Fi",.ip="192.168.1.42"};
    deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(find_label(lv_screen_active(),"IP address: 192.168.1.42")!=NULL);screenshot("06-connected");
    click("Continue");CHECK(find_label(lv_screen_active(),"Brak urządzeń")!=NULL);
    CHECK(find_label(lv_screen_active(),"ON")==NULL);screenshot("07-home");
    dsr_network_device(NULL);screenshot("08-device-form");
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
    dsr_network_device(NULL);password=find_type(lv_screen_active(),&lv_textarea_class);
    lv_textarea_set_text(password,"192.168.1.77");click("Check & save device");CHECK(submitted.kind==DSR_JOB_BIND);
    response.device=(dsr_device_t){.host="192.168.1.77",.name="Desk lamp",.device_id="shellyplus1-AABBCCDDEEFF",
        .generation=DSR_GEN2_PLUS,.online=true,.has_switch=true,.output=true};
    deliver(DSR_OK,DSR_WIFI_IDLE);screenshot("11-configured-switch");
    wifi.connected=false;tick();screenshot("12-switch-offline");
    CHECK(find_label(lv_screen_active(),"Desk lamp")!=NULL);
    click("Desk lamp");CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);
    CHECK(find_label(lv_screen_active(),"Offline • stan niepotwierdzony")!=NULL);
    test_workspace_enable();dsr_ui_edit_dashboard();
    CHECK(find_label(lv_screen_active(),"Edytuj pilot"));screenshot("13-workspace-editor");
    click("Pomieszczenia");click("+ Dodaj wpis");
    lv_obj_t *name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);lv_textarea_set_text(name,"Salon testowy");click("Zapisz");
    char *configuration=dsr_workspace_snapshot();CHECK(configuration);CHECK(dsr_config_revision(configuration)==2);
    dsr_mobile_model_t model={0};CHECK(dsr_mobile_configure(&model,configuration));free(configuration);
    CHECK(model.room_count==1);CHECK(!strcmp(model.rooms[0].name,"Salon testowy"));
    dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));
    configuration=dsr_workspace_snapshot();CHECK(dsr_workspace_submit(configuration,2));free(configuration);dsr_ui_workspace_tick();
    CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));
    dsr_network_home(NULL);dsr_ui_workspace_tick();
    test_workspace_disable();
    printf("PASS: %u real LVGL UI checks; thirteen 320x480 screenshots; touch editor shares versioned configuration; 60 navigation cycles at 128 KiB LVGL heap\n",checks);
    lv_deinit();return 0;
}
