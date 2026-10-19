/* Real LVGL Wi-Fi/OTA presentation. Radio, updater and storage MODEL only. */
#define main original_ui_main
#include "test_ui.c"
#undef main
void test_native_ota_progress(size_t received,size_t total);
void test_native_ota_uncertain(void);
static void shot(unsigned number,const char *name,unsigned language)
{char path[96];snprintf(path,sizeof(path),"%u-cyber-%s-%s",number,name,language==0?"pl":language==1?"en":"de");screenshot(path);}
static lv_area_t area(lv_obj_t *o){lv_area_t a;CHECK(o);lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(o,&a);return a;}
static lv_obj_t *prefix(lv_obj_t *root,const char *text)
{
 if(lv_obj_check_type(root,&lv_label_class)&&!strncmp(lv_label_get_text(root),text,12))return root;
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=prefix(lv_obj_get_child(root,(int)i),text);if(o)return o;}return NULL;
}
static void press_label(const char *text)
{
 lv_obj_t *l=find_label(lv_screen_active(),text);if(!l&&strlen(text)>=12)l=prefix(lv_screen_active(),text);if(!l)fprintf(stderr,"Missing tap label: %s\n",text);CHECK(l);lv_obj_t *b=lv_obj_get_parent(l);lv_obj_scroll_to_view(b,LV_ANIM_OFF);lv_area_t a=area(b);
 CHECK(a.y1>=0&&a.y2<480&&lv_obj_get_height(b)>=44);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static void checks_for_language(unsigned language)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString("cyber_blue"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));publish_document(doc);dsr_network_home(NULL);tick();
 dsr_network_wifi(NULL);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_WIFI_SCAN);
 response.scan=(dsr_wifi_scan_t){.count=5,.items={{.ssid="Home Wi-Fi",.rssi=-54,.security=DSR_WIFI_WPA2},{.ssid="ESP32 Lab",.rssi=-69,.security=DSR_WIFI_WPA2},{.ssid="Guest network",.rssi=-84,.security=DSR_WIFI_WPA2},{.ssid="Workshop",.rssi=-88,.security=DSR_WIFI_WPA2},{.ssid="Warszawa_Lodz_Kueche_long_SSID",.rssi=-61,.security=DSR_WIFI_WPA2}}};deliver(DSR_OK,DSR_WIFI_IDLE);shot(660,"wifi-list",language);
 press_label("Warszawa_Lodz_Kueche_long_SSID");
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class),*kb=find_type(lv_screen_active(),&lv_keyboard_class);
 CHECK(field&&kb&&lv_textarea_get_password_mode(field));
 lv_area_t input=area(field),keys=area(kb),connect=area(lv_obj_get_parent(find_label(lv_screen_active(),"Connect")));
 CHECK(keys.y1==256&&keys.y2==479&&input.y2<connect.y1&&connect.y2<keys.y1);CHECK(lv_obj_has_state(field,LV_STATE_FOCUSED));
 shot(661,"wifi-password",language);unsigned writes=job_counts[DSR_JOB_WIFI_CONNECT];lv_textarea_set_text(field,"short");press_label("Connect");
 CHECK(job_counts[DSR_JOB_WIFI_CONNECT]==writes);CHECK(find_label(lv_screen_active(),"Use 8-63 characters or a 64-digit hex key."));CHECK(!find_label(lv_screen_active(),"short"));shot(662,"wifi-password-error",language);press_label("Back");dsr_network_home(NULL);
 test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_CLOCK,DSR_MANIFEST_INVALID);dsr_ui_native_ota_open(NULL);
 lv_obj_t *notice=find_label(lv_screen_active(),"Nie ustawiono czasu. Sprawdź dostęp do internetu i spróbuj ponownie.");CHECK(notice&&lv_obj_get_style_text_font(notice,0)==&dsr_roboto_14);
 lv_obj_t *code=find_label(lv_screen_active(),"Kod błędu: 4/0/0/1");
 /* Technical code follows the full wrapped message inside its scroll viewport. */
 if(!code){lv_obj_t *parent=lv_obj_get_parent(notice);for(unsigned i=0;i<lv_obj_get_child_count(parent);i++){lv_obj_t *c=lv_obj_get_child(parent,(int)i);if(lv_obj_check_type(c,&lv_label_class)&&c!=notice&&lv_obj_get_style_text_font(c,0)==&dsr_roboto_12)code=c;}}
 CHECK(code);lv_area_t message=area(notice),technical=area(code);CHECK(message.y2<technical.y1);
 shot(663,"ota-error",language);
 test_native_ota_uncertain();dsr_ui_native_ota_open(NULL);CHECK(find_label(lv_screen_active(),"Nie potwierdzono wyboru startowego firmware. Dalsze zapisy zablokowane do restartu."));shot(664,"ota-boot-warning",language);
 test_native_ota_set(DSR_NATIVE_INSTALLING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_progress(500,1000);dsr_ui_native_ota_open(NULL);
 lv_obj_t *bar=find_type(lv_screen_active(),&lv_bar_class);CHECK(bar&&lv_bar_get_value(bar)==500&&find_label(lv_screen_active(),"50%")&&find_label(lv_screen_active(),"500 / 1000 B"));shot(665,"ota-progress",language);
 test_native_ota_progress(2000,1000);dsr_ui_native_ota_tick();CHECK(lv_bar_get_value(bar)==1000&&find_label(lv_screen_active(),"100%"));
 test_native_ota_progress(500,0);dsr_ui_native_ota_tick();CHECK(lv_bar_get_value(bar)==0&&find_label(lv_screen_active(),"— %"));
 test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_INVALID);dsr_network_home(NULL);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 for(unsigned language=0;language<3;language++)checks_for_language(language);
 CHECK(!job_counts[DSR_JOB_WIFI_CONNECT]&&!job_counts[DSR_JOB_OTA_INSTALL]&&!restart_requests);
 printf("PASS: %u Cyber Wi-Fi/OTA checks; %u real LVGL frames; PL/EN/DE, list scroll, masked password/validation, footer/keyboard, long errors, measured progress; radio/updater/storage MODEL\n",checks,screenshots);
 lv_deinit();return 0;
}
