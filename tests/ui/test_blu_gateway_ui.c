/* Production LCD, network queue and settings actor; HTTP/GATT are models. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_blu.h"
#include "dsr_device_settings.h"
#include "dsr_ui_cyber.h"
void test_settings_enable(bool enabled);
unsigned test_settings_writes(void);
void test_blu_pin_required(bool pin);
static unsigned review_failures;
static void review_check(bool ok,const char *what)
{checks++;if(!ok){fprintf(stderr,"gateway review: %s (skin %u, locale %u)\n",what,dsr_sc_style(),dsr_sc_language());review_failures++;}}
static void warning_frame(const char *text)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);review_check(label!=NULL,"important text present");if(!label)return;
 lv_obj_t *panel=lv_obj_get_parent(label);lv_obj_update_layout(panel);lv_area_t p,t;lv_obj_get_coords(panel,&p);lv_obj_get_coords(label,&t);
 uint32_t gold=dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;
 review_check((lv_color_to_u32(lv_obj_get_style_border_color(panel,0))&0xffffff)==gold,"important text has a gold frame");
 review_check((lv_color_to_u32(lv_obj_get_style_text_color(label,0))&0xffffff)==gold,"important text is gold");
 review_check(t.x1>=p.x1+8&&t.x2<=p.x2-8&&t.y1>=p.y1+8&&t.y2<=p.y2-8,"important text fits inside frame");
}
static void cyan_text_frame(const char *text)
{
 if(dsr_sc_style()!=DSR_SC_STYLE_CYBER_BLUE)return;
 lv_obj_t *label=find_label(lv_screen_active(),text);review_check(label!=NULL,"information text present");if(!label)return;
 lv_obj_t *panel=lv_obj_get_parent(label);lv_obj_update_layout(panel);lv_area_t p,t;lv_obj_get_coords(panel,&p);lv_obj_get_coords(label,&t);
 review_check((lv_color_to_u32(lv_obj_get_style_border_color(panel,0))&0xffffff)==DSR_CYBER_CYAN,"information text has a turquoise frame");
 review_check(t.x1>=p.x1+8&&t.x2<=p.x2-8&&t.y1>=p.y1+8&&t.y2<=p.y2-8,"information text fits inside frame");
 review_check(lv_area_get_height(&p)<180,"information has its own fitted frame");
 review_check(p.y1>=90&&p.y2<392,"complete information frame is visible above actions");
}
static void frame_gap(const char *first,const char *second)
{
 if(dsr_sc_style()!=DSR_SC_STYLE_CYBER_BLUE)return;
 lv_obj_t *a=find_label(lv_screen_active(),first),*b=find_label(lv_screen_active(),second);review_check(a&&b,"neighbor text present");if(!a||!b)return;
 lv_obj_update_layout(lv_screen_active());lv_area_t x,y;lv_obj_get_coords(lv_obj_get_parent(a),&x);lv_obj_get_coords(lv_obj_get_parent(b),&y);review_check(x.y2+8<y.y1,"complete text frames have an 8 px gap");
}
static void finish_settings(void)
{
 for(unsigned i=0;i<8;i++){
  tick();if(!in_flight)return;
  response=(dsr_event_t){.device=submitted.connection.device,.layout_id=submitted.connection.layout_id};
  if(submitted.kind==DSR_JOB_DEVICE_SETTINGS)dsr_device_settings_execute(&submitted,&response);
  else {CHECK(submitted.kind==DSR_JOB_PROBE);response.device.online=response.device.has_switch=true;response.result=DSR_OK;}
  deliver(response.result,DSR_WIFI_IDLE);
 }
 CHECK(!"Settings did not finish");
}
static void check_keyboard_layout(void)
{
 lv_obj_t *root=lv_screen_active(),*keyboard=find_type(root,&lv_keyboard_class);if(!keyboard||lv_obj_has_flag(keyboard,LV_OBJ_FLAG_HIDDEN))return;lv_obj_update_layout(root);lv_area_t keys;lv_obj_get_coords(keyboard,&keys);CHECK(keys.y1==240);
 for(uint32_t i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *child=lv_obj_get_child(root,(int32_t)i);if(child==keyboard||lv_obj_has_flag(child,LV_OBJ_FLAG_HIDDEN))continue;lv_area_t box;lv_obj_get_coords(child,&box);if(box.y2>=keys.y1)fprintf(stderr,"keyboard overlap: child %u rectangle %d,%d..%d,%d\n",(unsigned)i,(int)box.x1,(int)box.y1,(int)box.x2,(int)box.y2);review_check(box.y2<keys.y1,"keyboard does not overlap content or actions");
 }
}

void test_settings_gateway(unsigned mode);
void test_blu_gateway_key(bool enabled);
static const char gateway_name[]="Shelly Gateway im Erdgeschoss und Wohnzimmer";
static const char no_report[]="Czujnik zapisany na bramce. Odbiór pomiarów niepotwierdzony.";
static const char received[]="Czujnik dodany. Bramka odbiera pomiary.";
static const char partial[]="Część konfiguracji wymaga ponownego sprawdzenia. Ponowne dodanie użyje istniejącego czujnika.";
static void capture(const char *state,unsigned locale)
{check_keyboard_layout();char name[80];snprintf(name,sizeof(name),"blu-gateway-%u-%s-%u",dsr_sc_style(),state,locale);screenshot(name);}
static void pair(unsigned model)
{
 dsr_ble_device_t c={.address={2,2,3,4,5,6},.model_id=model,.has_model=true,.kind=DSR_BLE_SHELLY_BLU,.evidence=DSR_BLE_MFG,.has_flags=true,.flags=DSR_BLE_FLAG_PAIRING};
 test_blu_pin_required(false);dsr_ui_blu_open(&c);click("Skanuj i połącz");uint8_t packet[]={6,0xff,0xa9,0x0b,0x0b,(uint8_t)model,0};test_ble_packet(packet,sizeof(packet),2);test_ble_finish(DSR_BLE_DONE);tick();tick();CHECK(find_label(lv_screen_active(),"Wybierz bramkę Shelly"));
}
static lv_obj_t *gateway_button(lv_obj_t *o)
{
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE)&&lv_obj_get_width(o)==296&&lv_obj_get_y(o)==0)return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *b=gateway_button(lv_obj_get_child(o,(int)i));if(b)return b;}return NULL;
}
static void find_gateway(bool auth)
{
 click("Wybierz bramkę Shelly");char json[256];snprintf(json,sizeof(json),"{\"gen\":3,\"id\":\"shellyblugwg3-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"S3GW-1DBT001\",\"auth_en\":%s,\"name\":\"%s\"}",auth?"true":"false",gateway_name);test_lan_found(json,"192.0.2.71");tick();
}
static void choose(void)
{lv_obj_t *b=gateway_button(lv_screen_active());CHECK(b);lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);lv_obj_update_layout(lv_screen_active());}
static void run_registration(unsigned mode)
{test_settings_gateway(mode);choose();finish_settings();}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();test_blu_gateway_key(true);test_settings_enable(true);
 const char *languages[]={"pl","en","de"};
 for(unsigned pass=0;pass<5;pass++){
  unsigned locale=pass<2?2:pass-2,skin=pass==0?0:pass==1?2:3;
  cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(languages[locale]));cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(skin==3?"cyber_blue":skin==2?"light":"graphite"));publish_document(doc);dsr_network_home(NULL);tick();pair(5);
  click("Wybierz bramkę Shelly");cyan_text_frame("Wyszukiwanie bramek w sieci");capture("search",locale);
  test_lan_found("{\"gen\":1,\"id\":\"shelly1-test\",\"mac\":\"112233445566\",\"type\":\"SHSW-1\"}","192.0.2.70");tick();cyan_text_frame("Nie znaleziono zgodnej bramki w sieci");capture("empty",locale);tap(30,22);
  find_gateway(true);lv_obj_t *name=find_label(lv_screen_active(),gateway_name),*host=find_label(lv_screen_active(),"192.0.2.71");review_check(name&&host,"gateway name and host have separate complete labels");capture("choices",locale);choose();
  unsigned skip=1;lv_obj_t *password=nth_type(lv_screen_active(),&lv_textarea_class,&skip);CHECK(password&&lv_textarea_get_password_mode(password));lv_obj_t *username=find_type(lv_screen_active(),&lv_textarea_class);lv_obj_update_layout(lv_screen_active());lv_area_t viewport,u,p;lv_obj_get_coords(lv_obj_get_parent(password),&viewport);lv_obj_get_coords(username,&u);lv_obj_get_coords(password,&p);review_check(u.y1>=viewport.y1&&p.y2<=viewport.y2&&u.y2+8<p.y1,"both credential frames are fully visible and separated");CHECK(lv_obj_get_height(username)>=44&&lv_obj_get_height(password)>=44);lv_textarea_set_text(password,"synthetic-password");CHECK(!find_label(lv_screen_active(),"synthetic-password"));capture("credentials",locale);test_settings_gateway(5);click("Dalej");finish_settings();warning_frame("Sprawdź parowanie BLU i dane logowania bramki Shelly.");CHECK(!find_label(lv_screen_active(),"Zapisz kartę"));capture("auth-error",locale);click("Wstecz");
  find_gateway(false);test_settings_gateway(1);choose();cyan_text_frame("Dodawanie czujnika do bramki…");capture("register",locale);finish_settings();CHECK(find_label(lv_screen_active(),no_report));warning_frame(no_report);capture("unconfirmed",locale);
  unsigned writes_before=test_settings_writes(),revision_before=dsr_workspace_revision();click("Zapisz kartę");CHECK(!dsr_ui_blu_active()&&!dsr_device_settings_sensitive()&&find_label(lv_screen_active(),"Nazwa i pomieszczenie"));CHECK(writes_before==1&&test_settings_writes()==writes_before&&dsr_workspace_revision()==revision_before);capture("card",locale);dsr_ui_lan_reset();
  pair(5);find_gateway(false);run_registration(2);CHECK(find_label(lv_screen_active(),received));cyan_text_frame(received);capture("receiving",locale);click("Wstecz");
  find_gateway(false);run_registration(3);CHECK(find_label(lv_screen_active(),no_report)&&find_label(lv_screen_active(),partial));warning_frame(no_report);warning_frame(partial);frame_gap(no_report,partial);capture("partial",locale);click("Wstecz");
  find_gateway(false);run_registration(4);warning_frame("Model lub bramka nie obsługuje tej funkcji");CHECK(!find_label(lv_screen_active(),"Zapisz kartę"));capture("unsupported",locale);dsr_ui_blu_hide();
  if(skin==3){pair(8);find_gateway(false);choose();cyan_text_frame("Parowanie TRV z bramką");warning_frame("Bramka szuka głowic przez 30 s. Włącz tryb parowania tylko w wybranym TRV. Po rozpoczęciu krótko naciśnij jego przycisk Reset.");review_check(find_label(lv_screen_active(),locale==1?"Start":locale==2?"Starten":"Rozpocznij")!=NULL,"TRV start action translated");capture("trv-association",locale);click("Wstecz");dsr_ui_blu_hide();}
 }
 CHECK(!review_failures);test_settings_gateway(0);test_settings_enable(false);test_blu_gateway_key(false);dsr_network_home(NULL);printf("PASS: %u BLU gateway LCD checks; %u frames; actual actor/queue; HTTP/GATT/storage MODEL\n",checks,screenshots);lv_deinit();return 0;
}
