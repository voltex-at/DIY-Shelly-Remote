/* Actual LVGL + actual UI source. Only platform/radio are fixture providers.
 * Screenshots are rendered pixels, not HTML mockups or simulated device data
 * included in firmware. */
#include "dsr_ui.h"
#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_board.h"
#include "dsr_workspace.h"
#include "esp_app_desc.h"
#include "src/widgets/buttonmatrix/lv_buttonmatrix_private.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static unsigned screenshots,requests;
void test_workspace_enable(void);
void test_workspace_disable(void);
void test_workspace_defer(bool enabled);
void test_workspace_finish(bool success);
void test_lan_found(const char *identity,const char *host);
void test_lan_inspected(dsr_result_t result,const char *components);
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
static lv_indev_t *pointer;
static lv_point_t touch;
static bool pressed;
static unsigned ready_count;
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
esp_err_t dsr_board_set_backlight(uint8_t value) {(void)value; return ESP_OK;}
void dsr_board_sleep_display(void) {}
bool dsr_board_touch_available(void) {return true;}
esp_err_t dsr_settings_load(dsr_settings_t *out) {*out=stored; return ESP_OK;}
esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found) {memset(out,0,sizeof(*out)); *found=false; return ESP_OK;}
bool dsr_wifi_connected(void) {return wifi.connected;}
void dsr_wifi_get_info(dsr_wifi_info_t *out) {*out=wifi;}
void dsr_wifi_cancel_connect(void) {CHECK(submitted.kind==DSR_JOB_WIFI_CONNECT);}
dsr_result_t dsr_worker_submit(const dsr_job_t *job)
{CHECK(!in_flight);requests++;submitted=*job; in_flight=true; return DSR_OK;}
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
    CHECK(fwrite(canvas,1,sizeof(canvas),f)==sizeof(canvas));fclose(f);screenshots++;
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
    lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label!=NULL);
    (void)lv_obj_send_event(lv_obj_get_parent(label),LV_EVENT_CLICKED,NULL);
    lv_obj_update_layout(lv_screen_active());
}
static lv_obj_t *nth_type(lv_obj_t *root,const lv_obj_class_t *type,unsigned *skip)
{
    if(lv_obj_check_type(root,type)){if(!*skip)return root;(*skip)--;}
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=nth_type(lv_obj_get_child(root,(int)i),type,skip);if(o)return o;}return NULL;
}
static void deliver(dsr_result_t result,dsr_wifi_status_t detail)
{response.id=submitted.id;response.result=result;response.wifi_detail=detail;result_ready=true;tick();}
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
    dsr_network_device(NULL);screenshot("14-add-device-wizard");click("Dodaj ręcznie (IP)");screenshot("08-device-form");
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
    dsr_network_device(NULL);click("Dodaj ręcznie (IP)");password=find_type(lv_screen_active(),&lv_textarea_class);
    lv_textarea_set_text(password,"192.168.1.77");click("Check & save device");CHECK(submitted.kind==DSR_JOB_BIND);
    response.device=(dsr_device_t){.host="192.168.1.77",.name="Desk lamp",.device_id="shellyplus1-AABBCCDDEEFF",
        .generation=DSR_GEN2_PLUS,.online=true,.has_switch=true,.output=true};
    deliver(DSR_OK,DSR_WIFI_IDLE);screenshot("11-configured-switch");
    wifi.connected=false;tick();screenshot("12-switch-offline");
    CHECK(find_label(lv_screen_active(),"Desk lamp")!=NULL);
    click("Desk lamp");CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);
    CHECK(find_label(lv_screen_active(),"Offline • stan niepotwierdzony")!=NULL);
    dsr_ui_open_settings();screenshot("40-settings-menu");click("Settings");screenshot("41-display-settings");
    test_workspace_enable();dsr_ui_edit_dashboard();
    CHECK(find_label(lv_screen_active(),"Edytuj pilot"));screenshot("13-workspace-editor");
    click("Pomieszczenia");click("+ Dodaj wpis");
    lv_obj_t *name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);lv_textarea_set_text(name,"Salon testowy");click("Zapisz");
    char *configuration=dsr_workspace_snapshot();CHECK(configuration);CHECK(dsr_config_revision(configuration)==2);
    dsr_mobile_model_t model={0};CHECK(dsr_mobile_configure(&model,configuration));free(configuration);
    CHECK(model.room_count==1);CHECK(!strcmp(model.rooms[0].name,"Salon testowy"));
    dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));
    test_workspace_defer(true);lv_textarea_set_text(name,"Wersja A");click("Zapisz");
    lv_textarea_set_text(name,"Wersja B");click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisywanie…"));
    test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
    screenshot("20-workspace-save-race");
    CHECK(!strcmp(lv_textarea_get_text(name),"Wersja B"));configuration=dsr_workspace_snapshot();CHECK(strstr(configuration,"Wersja A"));CHECK(!strstr(configuration,"Wersja B"));free(configuration);
    click("Zapisz");test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));CHECK(!strcmp(lv_textarea_get_text(name),"Wersja B"));
    click("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano • zsynchronizowano"));configuration=dsr_workspace_snapshot();CHECK(dsr_config_revision(configuration)==4);CHECK(strstr(configuration,"Wersja B"));free(configuration);
    test_workspace_defer(false);configuration=dsr_workspace_snapshot();CHECK(dsr_workspace_submit(configuration,4));free(configuration);dsr_ui_workspace_tick();
    CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));
    dsr_network_home(NULL);dsr_ui_workspace_tick();
    wifi.connected=false;dsr_ui_lan_open(NULL);tick();CHECK(find_label(lv_screen_active(),"Najpierw połącz Wi-Fi"));screenshot("19-lan-no-wifi");
    wifi.connected=true;dsr_network_device(NULL);click("Skanuj sieć Wi-Fi");tick();CHECK(dsr_ui_lan_active());screenshot("15-lan-scanning");
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
    configuration=dsr_workspace_snapshot();CHECK(configuration);CHECK(dsr_mobile_configure(&model,configuration));free(configuration);CHECK(model.device_count==1&&model.devices[0].room==0&&model.devices[0].channel==1);
    dsr_ui_lan_reset();test_workspace_disable();
    printf("PASS: %u real LVGL UI checks; %u screenshots; keyboard cursor/UTF-8/ASCII, shared shell, LAN and async save regression; 60 navigation cycles\n",checks,screenshots);
    lv_deinit();return 0;
}
