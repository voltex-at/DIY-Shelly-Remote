/* Production UI, LVGL 9.3 and settings actor. HTTP/clock/SDK are fixtures. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_device_settings.h"
#include "../../components/dsr_ui/dsr_ui_cyber.h"
void test_settings_enable(bool enabled);
void test_settings_general(bool enabled);
unsigned test_settings_writes(void);
static unsigned outputs,refreshes,edits,settings_opens,targets,positions,stops;
static uint8_t target;
static void output_changed(unsigned i,bool on){CHECK(i==0);(void)on;outputs++;}
static void refreshed(uint32_t id){CHECK(id==31);refreshes++;}
static void edited(uint32_t id){CHECK(id==31);edits++;}
static void opened(uint32_t id){CHECK(id==31);settings_opens++;}
static void adjusted(unsigned i,uint8_t half){CHECK(i==0);target=half;targets++;}
static void positioned(unsigned i,uint8_t value){CHECK(i==0&&value<=100);positions++;}
static void stopped(unsigned i){CHECK(i==0);stops++;}
static unsigned objects(lv_obj_t *o)
{unsigned n=1;for(unsigned i=0;i<lv_obj_get_child_count(o);i++)n+=objects(lv_obj_get_child(o,(int)i));return n;}
static lv_obj_t *sized(lv_obj_t *o,int w,int h)
{if(lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h)return o;for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=sized(lv_obj_get_child(o,(int)i),w,h);if(r)return r;}return NULL;}
static void snap(const char *kind,unsigned lang)
{char name[96];snprintf(name,sizeof(name),"native-recovered-%s-%s",kind,lang==0?"pl":lang==1?"en":"de");screenshot(name);}
static dsr_mobile_model_t relay(void)
{
 dsr_mobile_model_t m={.wifi=true,.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=31,.name="SPSW-104PE16",.kind=DSR_DEVICE_RELAY,.generation=DSR_GEN2_PLUS,.physical_generation=3,.online=true,.can_switch=true,.can_refresh=true,.has_sample_time=true,.sample_ms=lv_tick_get()-5000,.has_power=true,.power_w=0,.has_voltage=true,.voltage_v=231.1,.has_current=true,.current_a=0,.host="192.0.2.58",.identity="shelly1pmg3-aabbccddeeff",.fw="2.0.1"};return m;
}
static void show(dsr_mobile_model_t *m,dsr_mobile_page_t p)
{dsr_mobile_update(m);dsr_mobile_show(p,0);lv_obj_update_layout(lv_screen_active());}
static void relay_states(unsigned lang)
{
 dsr_mobile_model_t m=relay();show(&m,DSR_MOBILE_DETAILS);snap("relay-off",lang);CHECK(find_label(lv_screen_active(),"0.0 W")&&find_label(lv_screen_active(),"231.1 V"));
 CHECK(!find_label(lv_screen_active(),"Zużycie"));tap(160,274);CHECK(outputs==lang+1);tap(264,274);CHECK(refreshes==lang+1);
 m.devices[0].error=DSR_OFFLINE;m.devices[0].online=false;show(&m,DSR_MOBILE_DETAILS);snap("relay-no-response",lang);
 CHECK(find_label(lv_screen_active(),"— W")&&!find_label(lv_screen_active(),"231.1 V")&&!find_label(lv_screen_active(),"0.0 W"));tap(160,274);CHECK(outputs==lang+1);
 m=relay();m.devices[0].output=true;m.devices[0].power_w=52.3;show(&m,DSR_MOBILE_DETAILS);snap("relay-on",lang);
 m.devices[0].busy=m.devices[0].command_pending=true;show(&m,DSR_MOBILE_DETAILS);snap("relay-pending",lang);tap(160,274);CHECK(outputs==lang+1);
 m=relay();strcpy(m.devices[0].name,"Decke");show(&m,DSR_MOBILE_DEVICE_SETTINGS);snap("settings",lang);CHECK(find_label(lv_screen_active(),"shelly1pmg3-aabbccddeeff"));CHECK(!find_label(lv_screen_active(),"Dostępne pomiary"));
 click("Otwórz ustawienia Shelly");CHECK(settings_opens==lang+1);click("Edytuj kartę");CHECK(edits==lang+1);
 m.devices[0].error=DSR_AUTH_REQUIRED;show(&m,DSR_MOBILE_DEVICE_SETTINGS);snap("settings-error",lang);
 m=relay();m.devices[0].generation=DSR_GEN1;show(&m,DSR_MOBILE_DEVICE_SETTINGS);lv_obj_t *b=lv_obj_get_parent(find_label(lv_screen_active(),"Otwórz ustawienia Shelly"));CHECK(lv_obj_has_state(b,LV_STATE_DISABLED));
}
static void trv_states(unsigned lang)
{
 dsr_mobile_model_t m=relay();dsr_mobile_device_t *d=&m.devices[0];
 strcpy(d->name,"Thermostat");d->kind=DSR_DEVICE_SENSOR;d->endpoint_kind=DSR_ENDPOINT_BLU;d->has_temperature=d->has_trv_target=d->has_battery=true;d->temperature_c=20.5;d->trv_target_c=20;d->battery_pct=100;d->blu.errors=DSR_BLU_TRV;d->blu.trv_pos=47;d->sample_ms=lv_tick_get()-2000;
 show(&m,DSR_MOBILE_DETAILS);snap("trv",lang);CHECK(find_label(lv_screen_active(),"20.5 °C")&&find_label(lv_screen_active(),"20.0 °C")&&find_label(lv_screen_active(),"47%"));
 unsigned count=objects(lv_screen_active());lv_obj_t *arc=find_type(lv_screen_active(),&lv_arc_class);CHECK(arc);
 tap(198,300);CHECK(objects(lv_screen_active())==count&&arc==find_type(lv_screen_active(),&lv_arc_class));CHECK(find_label(lv_screen_active(),"20.5 °C")&&find_label(lv_screen_active(),"Nastawa • oczekuje"));
 tap(198,300);snap("trv-increase",lang);CHECK(targets==lang);lv_tick_inc(1100);lv_timer_handler();CHECK(targets==lang);lv_tick_inc(150);lv_timer_handler();CHECK(targets==lang+1&&target==42);
 /* A new physical report, not the draft, determines the model and valve. */
 d->trv_target_c=21;show(&m,DSR_MOBILE_DETAILS);CHECK(find_label(lv_screen_active(),"21.0 °C"));
 touch=(lv_point_t){258,218};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);touch=(lv_point_t){228,150};lv_tick_inc(20);lv_indev_read(pointer);pressed=false;lv_tick_inc(20);lv_indev_read(pointer);CHECK(arc!=NULL);snap("trv-dial",lang);
 /* Pending draft is cancelled by an error; no stale temperature/lux leaks. */
 d->online=false;d->error=DSR_OFFLINE;show(&m,DSR_MOBILE_DETAILS);snap("trv-offline",lang);CHECK(!find_label(lv_screen_active(),"20.5 °C")&&find_label(lv_screen_active(),"— °C"));lv_tick_inc(1400);lv_timer_handler();CHECK(targets==lang+1);
 d->online=true;d->error=DSR_OK;d->battery_pct=12;show(&m,DSR_MOBILE_DETAILS);snap("trv-low-battery",lang);
 d->has_trv_target=d->has_temperature=d->has_battery=false;show(&m,DSR_MOBILE_DETAILS);snap("trv-no-data",lang);tap(198,300);lv_tick_inc(1400);lv_timer_handler();CHECK(targets==lang+1);
 show(&m,DSR_MOBILE_DEVICE_SETTINGS);CHECK(!find_label(lv_screen_active(),"21 lx"));
}
static void cover_states(unsigned lang)
{
 dsr_mobile_model_t m=relay();dsr_mobile_device_t *d=&m.devices[0];strcpy(d->name,"SPSH-002PE16EU");d->kind=DSR_DEVICE_COVER;d->endpoint_kind=DSR_ENDPOINT_COVER;d->can_cover=d->can_position=d->can_stop_cover=d->has_position=true;
 for(unsigned i=0;i<3;i++){d->position_pct=i*50;d->cover_state=DSR_COVER_STOPPED;show(&m,DSR_MOBILE_DETAILS);snap(i==0?"cover-closed":i==1?"cover-half":"cover-open",lang);}
 d->position_pct=60;d->cover_state=DSR_COVER_CLOSING;dsr_mobile_update(&m);snap("cover-closing-start",lang);
 unsigned fill[3]={0};
 for(unsigned frame=0;frame<3;frame++){
  if(frame){lv_tick_inc(frame==1?150:200);lv_timer_handler();snap(frame==1?"cover-closing-mid":"cover-closing-end",lang);}
  /* Locate the bright metal bottom rail, independently of the garden's blue sky. */
  for(int y=194;y<322;y++){const unsigned char *p=&canvas[(y*320+150)*3];if(p[0]>80&&p[1]>140&&p[2]>160)fill[frame]=(unsigned)y;}
 }
 CHECK(fill[0]<fill[1]&&fill[1]<fill[2]); /* Curtain descends from the cassette. */
 unsigned before=stops;click("Stop");CHECK(stops==before+1);
 d->position_pct=67;d->cover_state=DSR_COVER_STOPPED;dsr_mobile_update(&m);snap("cover-stopped",lang);
 touch=(lv_point_t){160,250};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);touch.y=220;lv_tick_inc(20);lv_indev_read(pointer);unsigned p=positions;CHECK(positions==p);pressed=false;lv_tick_inc(20);lv_indev_read(pointer);CHECK(positions==p+1);
 d->online=false;d->error=DSR_OFFLINE;dsr_mobile_update(&m);snap("cover-offline",lang);CHECK(find_label(lv_screen_active(),"Nieznana pozycja"));
}
static void finish_settings(void)
{for(unsigned i=0;i<8;i++){tick();if(!in_flight)return;response=(dsr_event_t){.device=submitted.connection.device,.layout_id=submitted.connection.layout_id};if(submitted.kind==DSR_JOB_DEVICE_SETTINGS)dsr_device_settings_execute(&submitted,&response);else{CHECK(submitted.kind==DSR_JOB_PROBE);response.device.online=response.device.has_switch=true;response.result=DSR_OK;}deliver(response.result,DSR_WIFI_IDLE);}CHECK(false);}
static uint32_t bind_relay(void)
{
 dsr_network_home(NULL);dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":3,\"id\":\"shelly1pmg3-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"S3SW-001P16EU\",\"auth_en\":false,\"name\":\"Decke\"}","192.0.2.58");dsr_ui_lan_tick();click("Decke");test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false}}");dsr_ui_lan_tick();click("Zapisz");CHECK(in_flight);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);tick();cJSON *doc=room_document();const cJSON *entry=cJSON_GetObjectItemCaseSensitive(doc,"devices")->child;CHECK(entry);uint32_t id=(uint32_t)cJSON_GetObjectItemCaseSensitive(entry,"id")->valueint;cJSON_Delete(doc);return id;
}
static void general(unsigned lang,uint32_t id)
{
 dsr_ui_device_settings_open(id);finish_settings();click("Ogólne");finish_settings();CHECK(find_label(lv_screen_active(),"Nazwa urządzenia")&&find_label(lv_screen_active(),"Tryb ECO"));snap("general",lang);
 tap(140,112);CHECK(find_type(lv_screen_active(),&lv_textarea_class));click("Wstecz");
 touch=(lv_point_t){160,300};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);
 for(unsigned n=1;n<=8;n++){touch.y=300-(int)n*15;lv_tick_inc(20);lv_indev_read(pointer);}pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();CHECK(!find_type(lv_screen_active(),&lv_textarea_class));
 lv_obj_t *body=sized(lv_screen_active(),296,324);CHECK(body);lv_obj_scroll_to_y(body,150,LV_ANIM_OFF);lv_obj_update_layout(body);snap("general-scrolled",lang);
 lv_obj_t *back=lv_obj_get_parent(find_label(lv_screen_active(),"Wstecz"));lv_area_t a;lv_obj_get_coords(back,&a);CHECK(a.y1==420&&a.y2<480);
 lv_obj_scroll_to_y(body,0,LV_ANIM_OFF);click("Nazwa urządzenia");lv_obj_t *input=find_type(lv_screen_active(),&lv_textarea_class);CHECK(input);snap("general-keyboard",lang);lv_textarea_set_text(input,"Decke nowa");click("Dalej");unsigned before=test_settings_writes();CHECK(!find_type(lv_screen_active(),&lv_keyboard_class));snap("general-confirm",lang);click("Wstecz");CHECK(test_settings_writes()==before);
 click("Nazwa urządzenia");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"Decke nowa");click("Dalej");click("Zapisz w Shelly");finish_settings();CHECK(test_settings_writes()==before+1&&find_label(lv_screen_active(),"Decke nowa"));
 if(lang==0){
  const char *fields[]={"Strefa czasowa","Szerokość geograficzna","Długość geograficzna","Serwer czasu"};
  const char *values[]={"Europe/Berlin","48.2","16.4","pool.ntp.org"};
  for(unsigned i=0;i<4;i++){click(fields[i]);lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),values[i]);click("Dalej");unsigned writes=test_settings_writes();click("Zapisz w Shelly");finish_settings();CHECK(test_settings_writes()==writes+1&&find_label(lv_screen_active(),values[i]));}
  click("Tryb ECO");lv_obj_t *choice=find_type(lv_screen_active(),&lv_switch_class);CHECK(choice);lv_obj_add_state(choice,LV_STATE_CHECKED);click("Dalej");unsigned writes=test_settings_writes();click("Zapisz w Shelly");finish_settings();CHECK(test_settings_writes()==writes+1&&find_label(lv_screen_active(),"Włączone"));
  click("Szerokość geograficzna");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"91");click("Dalej");writes=test_settings_writes();click("Zapisz w Shelly");finish_settings();CHECK(test_settings_writes()==writes&&find_label(lv_screen_active(),"Sprawdź wartość i dozwolony zakres"));
 }
 dsr_network_home(NULL);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 dsr_mobile_actions_t a={.set_output=output_changed,.refresh=refreshed,.edit_device=edited,.device_settings=opened,.set_trv=adjusted,.set_position=positioned,.stop_cover=stopped};
 for(unsigned lang=0;lang<3;lang++){dsr_sc_appearance(DSR_SC_BLUE,3,lang);dsr_ui_apply_theme();dsr_mobile_bind(&a);relay_states(lang);trv_states(lang);cover_states(lang);}
 for(unsigned skin=0;skin<4;skin++){dsr_sc_appearance(DSR_SC_BLUE,skin,0);dsr_ui_apply_theme();dsr_mobile_model_t m=relay();show(&m,DSR_MOBILE_DETAILS);CHECK(!find_label(lv_screen_active(),"Zużycie"));}
 /* Warm lazy caches before measuring repeated opens and animation teardown. */
 dsr_sc_appearance(DSR_SC_BLUE,3,0);dsr_ui_apply_theme();dsr_mobile_model_t m=relay();
 for(unsigned i=0;i<8;i++){show(&m,DSR_MOBILE_DETAILS);show(&m,DSR_MOBILE_DEVICE_SETTINGS);}dsr_mobile_hide();lv_obj_clean(lv_screen_active());lv_refr_now(display);lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned i=0;i<100;i++){show(&m,DSR_MOBILE_DETAILS);show(&m,DSR_MOBILE_DEVICE_SETTINGS);}dsr_mobile_hide();lv_obj_clean(lv_screen_active());lv_refr_now(display);lv_mem_monitor(&after);CHECK(after.free_size>=before.free_size);
 uint32_t id=bind_relay();test_settings_enable(true);test_settings_general(true);
 for(unsigned lang=0;lang<3;lang++){
  cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString("cyber_blue"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(lang==0?"pl":lang==1?"en":"de"));publish_document(doc);dsr_sc_appearance(DSR_SC_BLUE,3,lang);dsr_ui_apply_theme();general(lang,id);
 }test_settings_general(false);test_settings_enable(false);
 printf("PASS: %u recovered Cyber UI checks; %u native LVGL frames; real pointer and settings actor; HTTP/SDK MODEL; LVGL free %lu -> %lu after 100 cycles\n",checks,screenshots,(unsigned long)before.free_size,(unsigned long)after.free_size);lv_deinit();return 0;
}
