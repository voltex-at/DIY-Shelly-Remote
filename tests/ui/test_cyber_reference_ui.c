/* Reference refinements: real LVGL geometry, pointer dispatch and health data.
 * Board, ADC and storage providers remain host fixtures, not hardware tests. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_config_health.h"

static unsigned outputs,movements;
static int direction;
static void output_set(unsigned i,bool on){CHECK(i==0&&!on);outputs++;}
static void cover_set(unsigned i,int value){CHECK(i==1);direction=value;movements++;}
static lv_area_t ref_box(lv_obj_t *o)
{CHECK(o);lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(o,&a);return a;}
static lv_obj_t *sized(lv_obj_t *o,int w,int h)
{if(lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h)return o;for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=sized(lv_obj_get_child(o,(int)i),w,h);if(r)return r;}return NULL;}
static lv_obj_t *role(lv_obj_t *o,dsr_ui_icon_t kind)
{
 if(lv_obj_get_style_image_recolor_opa(o,0)==LV_OPA_COVER)
  for(unsigned i=0;i<lv_obj_get_event_count(o);i++)if((uintptr_t)lv_event_dsc_get_user_data(lv_obj_get_event_dsc(o,i))==(uintptr_t)kind)return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=role(lv_obj_get_child(o,(int)i),kind);if(r)return r;}return NULL;
}
static void no_white_icons(lv_obj_t *o)
{
 if(lv_obj_get_style_image_recolor_opa(o,0)==LV_OPA_COVER)CHECK(!lv_color_eq(lv_obj_get_style_image_recolor(o,0),dsr_sc_text_color(DSR_SC_TEXT)));
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)no_white_icons(lv_obj_get_child(o,(int)i));
}
static void contained(lv_obj_t *o)
{
 if(lv_obj_check_type(o,&lv_label_class)){
  lv_area_t a=ref_box(o),p=ref_box(lv_obj_get_parent(o));CHECK(a.x1>=p.x1&&a.x2<=p.x2&&a.y1>=p.y1&&a.y2<=p.y2);
 }
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)contained(lv_obj_get_child(o,(int)i));
}
static void press_ref(const char *name)
{
 lv_obj_t *text=find_label(lv_screen_active(),name);CHECK(text);lv_obj_t *b=lv_obj_get_parent(text);lv_obj_scroll_to_view(b,LV_ANIM_OFF);lv_area_t a=ref_box(b);CHECK(a.y1>=0&&a.y2<480);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static void snap(const char *name,unsigned lang)
{char s[96];snprintf(s,sizeof(s),"native-reference-%s-%s",name,lang==0?"pl":lang==1?"en":"de");screenshot(s);no_white_icons(lv_screen_active());}
static void locale(unsigned lang)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString("cyber_blue"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(lang==0?"pl":lang==1?"en":"de"));publish_document(doc);dsr_network_home(NULL);tick();CHECK(dsr_sc_style()==3&&dsr_sc_language()==lang);
}
static void hardware_pages(unsigned lang)
{
 dsr_ui_system_open(NULL);contained(lv_obj_get_parent(find_label(lv_screen_active(),"Firmware Update")));snap("system",lang);press_ref("Hardware tests");
 lv_obj_t *info=lv_obj_get_parent(find_label(lv_screen_active(),"Hardware test"));contained(info);CHECK(role(info,DSR_ICON_DOCUMENT)&&role(info,DSR_ICON_TOUCH));snap("hardware-ready",lang);
 press_ref("Check touchscreen");CHECK(find_label(lv_screen_active(),"1"));tap(35,25);char count[64];snprintf(count,sizeof(count),dsr_sc_tr("Corner %u detected\nTouch count: %u"),1,lang+1);CHECK(find_label(lv_screen_active(),count));
 press_ref("Back");dsr_ui_system_open(NULL);press_ref("Hardware tests");press_ref("Check display colors");CHECK(find_label(lv_screen_active(),"RED")&&find_label(lv_screen_active(),"WHITE"));snap("display-colors",lang);
 touch_available=false;dsr_ui_system_open(NULL);press_ref("Hardware tests");CHECK(find_label(lv_screen_active(),"Touch controller: NOT responding"));contained(lv_obj_get_parent(find_label(lv_screen_active(),"Hardware test")));snap("hardware-no-touch",lang);touch_available=true;
}
static void battery_page(unsigned lang)
{
 dsr_ui_battery_open(NULL);tick();dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=4120}};
 lv_obj_update_layout(lv_screen_active());lv_obj_t *heading=find_label(lv_screen_active(),"Bateria i zasilanie");if(!heading)snap("battery-heading-missing",lang);CHECK(heading);lv_point_t size;lv_text_get_size(&size,lv_label_get_text(heading),lv_obj_get_style_text_font(heading,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);CHECK(size.x<=lv_obj_get_width(heading));
 /* Use the real request/completion path: a previous locale's busy rejection
  * must not be inherited by a supposedly healthy test capture. */
 next_submit_result=DSR_OK;press_ref("Zmierz napięcie");CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ);CHECK(find_label(lv_screen_active(),"Odczyt napięcia…"));snap("battery-reading",lang);
 response.battery=e.battery;deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Napięcie BAT • dzielnik nominalny 2:1"));
 lv_obj_t *back=lv_obj_get_parent(find_label(lv_screen_active(),"Wstecz")),*measure=lv_obj_get_parent(find_label(lv_screen_active(),"Zmierz napięcie"));
 lv_area_t a=ref_box(back),b=ref_box(measure);CHECK(a.x2<b.x1&&a.y1==420&&b.y1==420);contained(back);contained(measure);
 lv_obj_t *power=lv_obj_get_parent(find_label(lv_screen_active(),"4.120 V"));contained(power);CHECK(lv_obj_get_height(power)<=114);snap("battery-voltage",lang);
 const dsr_event_t bad[]={{.result=DSR_IO},{.result=DSR_OK}};
 for(unsigned i=0;i<2;i++){dsr_ui_battery_result(&bad[i]);contained(power);snap(i?"battery-uncalibrated":"battery-error",lang);}
 dsr_ui_battery_result(&e);lv_tick_inc(31000);dsr_ui_battery_tick();contained(power);snap("battery-stale",lang);
 lv_obj_t *memory=lv_obj_get_parent(find_label(lv_screen_active(),"Pamięć i radio"));contained(memory);lv_obj_scroll_to_view(find_label(lv_screen_active(),"BLE"),LV_ANIM_OFF);snap("battery-memory",lang);
 next_submit_result=DSR_BUSY;press_ref("Zmierz napięcie");CHECK(find_label(lv_screen_active(),"Pilot jest zajęty. Spróbuj ponownie."));contained(power);press_ref("Wstecz");CHECK(find_label(lv_screen_active(),"Sieć i połączenia"));
}
static void diagnostic_pages(unsigned lang)
{
 const dsr_storage_phase_t states[]={DSR_STORAGE_READY,DSR_STORAGE_NONEMPTY,DSR_STORAGE_READ_ERROR,DSR_STORAGE_PROVISION_ERROR,DSR_STORAGE_NOT_STARTED};
 const char *labels[]={"SPIFFS: zamontowany","SPIFFS: niepusty, niezamontowany","SPIFFS: błąd odczytu","SPIFFS: błąd inicjalizacji","SPIFFS: niedostępny"};
 for(unsigned i=0;i<5;i++){
  dsr_config_health_storage(states[i],i?ESP_FAIL:ESP_OK);dsr_config_health_workspace(DSR_WORKSPACE_READY,ESP_OK);dsr_config_health_panel(i?ESP_FAIL:ESP_OK);
  unsigned count=requests;dsr_ui_config_diagnostics(NULL);CHECK(requests==count&&find_label(lv_screen_active(),labels[i]));
  lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),labels[i])),*body=lv_obj_get_parent(card);
  for(unsigned j=0;j<lv_obj_get_child_count(body);j++)contained(lv_obj_get_child(body,(int)j));
  CHECK(!find_label(lv_screen_active(),"Formatuj")&&!find_label(lv_screen_active(),"Usuń"));
  char name[48];snprintf(name,sizeof(name),"diagnostics-%u-top",i);snap(name,lang);lv_obj_scroll_to_y(body,1000,LV_ANIM_OFF);snprintf(name,sizeof(name),"diagnostics-%u-bottom",i);snap(name,lang);
  CHECK(find_label(lv_screen_active(),"Nie kasuj i nie formatuj pamięci."));press_ref("Wstecz");CHECK(find_label(lv_screen_active(),"Sieć i połączenia"));
 }
}
static void dashboard(unsigned lang)
{
 dsr_mobile_actions_t actions={.set_output=output_set,.set_cover=cover_set};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.device_count=2,.dashboard_name="Mój pulpit"};
 m.devices[0]=(dsr_mobile_device_t){.id=31,.name="Lampa",.identity="reference-lamp",.kind=DSR_DEVICE_RELAY,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=3.2,.has_voltage=true,.voltage_v=233.1};
 m.devices[1]=(dsr_mobile_device_t){.id=32,.name="Roleta",.identity="reference-cover",.kind=DSR_DEVICE_COVER,.endpoint_kind=DSR_ENDPOINT_COVER,.online=true,.can_cover=true,.has_position=true,.position_pct=50};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);
 lv_obj_t *root=lv_screen_active(),*card=lv_obj_get_parent(find_label(root,"Lampa")),*button=sized(card,58,58);
 lv_area_t a=ref_box(card),b=ref_box(button);CHECK(b.y1>ref_box(find_label(card,"Lampa")).y2&&b.y2<a.y2-3&&b.x2<a.x2-5);CHECK(role(root,DSR_ICON_HOME)&&role(root,DSR_ICON_GRID)&&role(root,DSR_ICON_CPU));snap("dashboard",lang);
 unsigned n=outputs;tap((b.x1+b.x2)/2,(b.y1+b.y2)/2);CHECK(outputs==n+1);
 const dsr_ui_icon_t arrows[]={DSR_ICON_UP,DSR_ICON_DOWN};
 for(unsigned i=0;i<2;i++){button=lv_obj_get_parent(role(root,arrows[i]));b=ref_box(button);n=movements;tap((b.x1+b.x2)/2,(b.y1+b.y2)/2);CHECK(movements==n+1&&direction==(i?-1:1));}
 m.devices[1].busy=true;dsr_mobile_update(&m);root=lv_screen_active();button=lv_obj_get_parent(role(root,DSR_ICON_UP));CHECK(lv_obj_has_state(button,LV_STATE_DISABLED));b=ref_box(button);n=movements;tap((b.x1+b.x2)/2,(b.y1+b.y2)/2);CHECK(movements==n);
 m.devices[1].busy=false;m.devices[1].online=false;dsr_mobile_update(&m);CHECK(lv_obj_has_state(lv_obj_get_parent(role(lv_screen_active(),DSR_ICON_DOWN)),LV_STATE_DISABLED));snap("dashboard-offline",lang);
 m.room_count=1;strcpy(m.rooms[0].name,"Salon");m.rooms[0].id=1;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_ROOMS,0);snap("rooms",lang);
}
static void thermostat(unsigned lang)
{
 dsr_mobile_model_t m={.wifi=true,.device_count=1};m.devices[0]=(dsr_mobile_device_t){.id=31,.identity="reference-trv",.name="Thermostat",.kind=DSR_DEVICE_SENSOR,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_temperature=true,.temperature_c=21.2,.has_trv_target=true,.trv_target_c=21,.has_battery=true,.battery_pct=100,.blu={.errors=DSR_BLU_TRV,.trv_pos=0},.has_sample_time=true,.sample_ms=lv_tick_get()};
 for(unsigned i=0;i<2;i++){
  m.devices[0].online=!i;m.devices[0].error=i?DSR_OFFLINE:DSR_OK;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_t *root=lv_screen_active();
  lv_obj_t *caption=find_label(root,"Nastawa temperatury");CHECK(!caption||lv_obj_get_parent(caption)==root);CHECK(lv_obj_get_parent(find_label(root,i?"Offline":"Online"))==root);
  CHECK(lv_obj_get_parent(role(root,i?DSR_ICON_OFFLINE:DSR_ICON_ONLINE))==root);snap(i?"trv-offline":"trv-online",lang);
 }
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 for(unsigned lang=0;lang<3;lang++){locale(lang);hardware_pages(lang);battery_page(lang);diagnostic_pages(lang);dashboard(lang);thermostat(lang);}
 printf("PASS: %u reference refinement checks, %u native frames; PL/EN/DE; no physical acceptance\n",checks,screenshots);lv_deinit();return 0;
}
