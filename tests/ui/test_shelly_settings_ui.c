/* Genuine LCD editor/actor with deterministic HTTP/radio/storage providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_device_settings.h"
#include "dsr_blu.h"
void test_settings_enable(bool enabled);
unsigned test_settings_writes(void);
void test_blu_pin_required(bool pin);
unsigned test_blu_writes(void);
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
static uint32_t add_relay(void)
{
 dsr_ui_lan_open(NULL);test_lan_found("{\"gen\":4,\"id\":\"shelly1g4-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"S4SW-001X16EU\",\"auth_en\":false,\"name\":\"Settings test relay\"}","192.168.1.70");dsr_ui_lan_tick();
 /* Name is deliberately supplied by the fixture identity parser. */
 lv_obj_t *label=find_label(lv_screen_active(),"Settings test relay");
 CHECK(label);lv_obj_send_event(lv_obj_get_parent(label),LV_EVENT_CLICKED,NULL);
 test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false}}");dsr_ui_lan_tick();click("Zapisz");CHECK(in_flight&&submitted.kind==DSR_JOB_BIND);
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);tick();
 cJSON *doc=room_document();const cJSON *entry=cJSON_GetObjectItemCaseSensitive(doc,"devices")->child;CHECK(entry);uint32_t id=(uint32_t)cJSON_GetObjectItemCaseSensitive(entry,"id")->valueint;cJSON_Delete(doc);dsr_network_home(NULL);return id;
}
static void select_group(const char *name)
{char text[96];snprintf(text,sizeof(text),"%s • 0",dsr_sc_tr(name));click(text);finish_settings();}
static void open_editor(uint32_t id)
{dsr_network_device_settings(id);CHECK(find_label(lv_screen_active(),"Ustawienia Shelly"));click("Ustawienia Shelly");finish_settings();CHECK(dsr_ui_device_settings_active()&&dsr_device_settings_sensitive());}
static void check_keyboard_layout(void)
{
 lv_obj_t *root=lv_screen_active(),*keyboard=find_type(root,&lv_keyboard_class);if(!keyboard)return;lv_obj_update_layout(root);lv_area_t keys;lv_obj_get_coords(keyboard,&keys);CHECK(keys.y1==240);
 for(uint32_t i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *child=lv_obj_get_child(root,(int32_t)i);if(child==keyboard||lv_obj_has_flag(child,LV_OBJ_FLAG_HIDDEN))continue;lv_area_t box;lv_obj_get_coords(child,&box);CHECK(box.y2<keys.y1);
 }
}
static void frame(const char *prefix,unsigned locale)
{check_keyboard_layout();char name[64];snprintf(name,sizeof(name),"%s%s-%u",dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?"7":"",prefix,locale);screenshot(name);}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();dsr_sc_appearance(DSR_SC_BLUE,0,0);uint32_t id=add_relay();
 const char *languages[]={"pl","en","de"};
 for(unsigned pass=0;pass<6;pass++){
  unsigned l=pass%3;bool hud=pass>=3;
  cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(languages[l]));cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(hud?"cyber_blue":"graphite"));publish_document(doc);if(hud)dsr_network_home(NULL);tick();test_settings_enable(true);
  open_editor(id);frame("380-shelly-groups",l);select_group("Przekaźnik");CHECK(find_label(lv_screen_active(),"Opóźnienie wyłączenia (s)"));frame("381-shelly-fields",l);
  click("Opóźnienie wyłączenia (s)");lv_obj_t *input=find_type(lv_screen_active(),&lv_textarea_class);CHECK(input);lv_textarea_set_text(input,"120");frame("382-shelly-number",l);click("Dalej");CHECK(test_settings_writes()==0);click("Wstecz");CHECK(test_settings_writes()==0);
  click("Opóźnienie wyłączenia (s)");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"120");click("Dalej");frame("383-shelly-confirm",l);click("Zapisz w Shelly");finish_settings();CHECK(test_settings_writes()==1&&find_label(lv_screen_active(),"120"));
  click("Wstecz");click("Sieć Wi-Fi");finish_settings();click("Nazwa sieci podstawowej");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"Changed network");click("Dalej");input=find_type(lv_screen_active(),&lv_textarea_class);CHECK(input&&lv_textarea_get_password_mode(input));lv_textarea_set_text(input,"secret-fixture-123");frame("384-shelly-password",l);click("Dalej");CHECK(!find_label(lv_screen_active(),"secret-fixture-123")&&find_label(lv_screen_active(),"Changed network"));frame("385-shelly-network-confirm",l);click("Wstecz");CHECK(test_settings_writes()==1);
  dsr_network_home(NULL);CHECK(!dsr_ui_device_settings_active()&&!dsr_device_settings_sensitive());test_settings_enable(false);
  dsr_ble_device_t candidate={.address={2,2,3,4,5,6},.model_id=5,.has_model=true,.kind=DSR_BLE_SHELLY_BLU,.evidence=DSR_BLE_MFG,.has_flags=true,.flags=DSR_BLE_FLAG_PAIRING};test_blu_pin_required(true);dsr_ui_blu_open(&candidate);CHECK(dsr_device_settings_sensitive());frame("386-blu-physical-confirm",l);click("Skanuj i połącz");const uint8_t packet[]={6,0xff,0xa9,0x0b,0x0b,5,0};test_ble_packet(packet,sizeof(packet),2);test_ble_finish(DSR_BLE_DONE);tick();tick();CHECK(find_label(lv_screen_active(),"PIN urządzenia BLU"));input=find_type(lv_screen_active(),&lv_textarea_class);CHECK(input&&lv_textarea_get_password_mode(input));lv_textarea_set_text(input,"123456");frame("387-blu-pin",l);click("Dalej");tick();CHECK(find_label(lv_screen_active(),"Wybierz bramkę Shelly"));frame("388-blu-settings",l);
  char field[96];snprintf(field,sizeof(field),"%s: 60",dsr_sc_tr("Podtrzymanie ruchu (s)"));click(field);lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"120");click("Dalej");CHECK(!test_blu_writes());frame("389-blu-confirm",l);click("Zapisz");tick();CHECK(test_blu_writes()==1);snprintf(field,sizeof(field),"%s: 120",dsr_sc_tr("Podtrzymanie ruchu (s)"));CHECK(find_label(lv_screen_active(),field));
  click("Zapomnij parowanie");CHECK(find_label(lv_screen_active(),"Karta i konfiguracja bramki pozostaną. Ponowne połączenie będzie wymagało parowania."));frame("390-blu-forget",l);click("Wstecz");click("Gotowe");CHECK(!dsr_ui_blu_active()&&!dsr_device_settings_sensitive());
 }
 test_settings_enable(false);dsr_network_home(NULL);printf("PASS: %u genuine Shelly settings/BLU wizard LCD checks; %u frames; PL EN DE, explicit confirmation, masked secrets, actual settings actor; HTTP/radio MODEL\n",checks,screenshots);lv_deinit();return 0;
}
