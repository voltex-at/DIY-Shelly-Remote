/* Focused OTA review: real pinned LVGL, model actor/Flash/restart only. */
#define main original_ui_main
#include "test_ui.c"
#undef main
void test_native_ota_progress(size_t received,size_t total);
void test_native_ota_versions(const char *installed,const char *candidate,uint32_t operation);
static unsigned findings,skin,language,operation=100;
static const char *stage;
static lv_area_t ota_box(lv_obj_t *o)
{CHECK(o);lv_area_t a;lv_obj_get_coords(o,&a);return a;}
static void review(bool ok,const char *reason)
{checks++;if(!ok){findings++;fprintf(stderr,"OTA skin%u lang%u %s: %s\n",skin,language,stage,reason);}}
static void audit(void)
{
 lv_obj_t *root=lv_screen_active(),*body=NULL;
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *o=lv_obj_get_child(root,(int)i);if(lv_obj_get_width(o)==296&&lv_obj_get_y(o)==86)body=o;
  if(lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE)&&lv_obj_get_y(o)==420){
   for(unsigned j=0;j<lv_obj_get_child_count(o);j++){
    lv_obj_t *label=lv_obj_get_child(o,(int)j);if(!lv_obj_check_type(label,&lv_label_class))continue;
    lv_area_t a=ota_box(o),b=ota_box(label);review(b.y1>=a.y1+6&&b.y2<=a.y2-6,"bottom action caption is clipped");
   }
  }
 }
 CHECK(body);int bottom=-8;
 for(unsigned i=0;i<lv_obj_get_child_count(body);i++){
  lv_obj_t *o=lv_obj_get_child(body,(int)i);if(lv_obj_get_width(o)!=296)continue;
  review(lv_obj_get_y(o)>=bottom+8,"adjacent cards/actions overlap or lack gap");bottom=lv_obj_get_y(o)+lv_obj_get_height(o);
  if(lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))continue;
  lv_area_t a=ota_box(o);
  for(unsigned j=0;j<lv_obj_get_child_count(o);j++){
   lv_obj_t *label=lv_obj_get_child(o,(int)j);if(!lv_obj_check_type(label,&lv_label_class))continue;
   lv_area_t b=ota_box(label);review(b.y1>=a.y1+6&&b.y2<=a.y2-6,"card label is clipped");
  }
 }
}
static void shot(const char *name)
{
 stage=name;lv_obj_update_layout(lv_screen_active());audit();
 /* Representative DE in all skins; long messages/consent also PL/EN. */
 if(language!=2&&strcmp(name,"consent")&&strcmp(name,"uncertain")&&strcmp(name,"staged"))return;
 char path[96];snprintf(path,sizeof(path),"ota-%u-%u-%s",skin,language,name);screenshot(path);
}
static void open_state(dsr_native_ota_state_t state,dsr_native_ota_error_t error,dsr_manifest_result_t manifest)
{
 test_native_ota_set(state,error,manifest);test_native_ota_detail(DSR_OTA_DEV,DSR_FETCH_ARGUMENT,0);
 test_native_ota_versions("0.7.70-dev","0.7.71",++operation);dsr_ui_native_ota_open(NULL);lv_obj_update_layout(lv_screen_active());
}
static void message(const char *text,bool warning)
{
 lv_obj_t *l=find_label(lv_screen_active(),text);CHECK(l);lv_obj_t *card=lv_obj_get_parent(l);lv_area_t a=ota_box(card),b=ota_box(l);
 if(skin==DSR_SC_STYLE_CYBER_BLUE)review(lv_obj_get_style_border_width(card,0)==2,"message needs Cyber Blue frame");
 if(warning){uint32_t gold=skin==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;
  review((lv_color_to_u32(lv_obj_get_style_text_color(l,0))&0xffffff)==gold,"important message needs gold text");
  review((lv_color_to_u32(lv_obj_get_style_border_color(card,0))&0xffffff)==gold,"important message needs gold frame");
 }
 if(card!=lv_screen_active()&&!lv_obj_has_flag(card,LV_OBJ_FLAG_SCROLLABLE))review(b.x1>=a.x1+8&&b.x2<=a.x2-8&&b.y1>=a.y1+8&&b.y2<=a.y2-8,"message needs internal padding and full text");
}
static void flow(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,skin,language);dsr_ui_apply_theme();open_state(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);shot("overview");
 click("Zmień kanał");shot("channel");click("Dev • wersje testowe");
 open_state(DSR_NATIVE_AVAILABLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_notes("Verbesserungen der Bedienoberfläche.\nDaten bleiben erhalten.");dsr_ui_native_ota_open(NULL);shot("available");
 unsigned before=requests;click("Instaluj");shot("consent");CHECK(requests==before);
 review(!find_label(lv_screen_active(),"Podłącz stabilne zasilanie USB. Nie odłączaj go podczas zapisu. Instalacja nie uruchamia pilota ponownie automatycznie."),"consent contradicts the 10-second automatic restart");
 open_state(DSR_NATIVE_CHECKING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);shot("checking");
 open_state(DSR_NATIVE_INSTALLING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_progress(929000,1858000);dsr_ui_native_ota_tick();shot("installing");message("Instalowanie. Nie odłączaj zasilania.",true);
 open_state(DSR_NATIVE_STAGED,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);shot("staged");
 char countdown[80];snprintf(countdown,sizeof(countdown),dsr_sc_tr("Automatyczny restart za %u s"),10u);
 lv_obj_t *notice=find_label(lv_screen_active(),"Instalacja zakończona. Uruchom pilot ponownie."),*count=find_label(lv_screen_active(),countdown);CHECK(notice&&count);
 lv_obj_t *status_card=lv_obj_get_parent(notice);lv_area_t c=ota_box(count),a=ota_box(status_card);
 if(lv_obj_get_parent(count)==status_card)review(c.y1>=ota_box(notice).y2+8&&c.x1>=a.x1+8&&c.y2<=a.y2-8,"countdown needs space inside status frame");
 else review(c.y1>=a.y2+8,"countdown overlaps status frame");
 click("Później");CHECK(!find_label(lv_screen_active(),"Później"));
 open_state(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_OFFLINE,DSR_MANIFEST_INVALID);shot("offline");message("Połącz pilot z domowym Wi-Fi.",true);
 open_state(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_MANIFEST,DSR_MANIFEST_BAD_SIGNATURE);shot("signature");message("Nieprawidłowy podpis wydania. Instalacja zablokowana.",true);CHECK(!find_label(lv_screen_active(),"Instaluj"));
 test_native_ota_uncertain();dsr_ui_native_ota_open(NULL);shot("uncertain");message("Nie potwierdzono wyboru startowego firmware. Dalsze zapisy zablokowane do restartu.",true);CHECK(!find_label(lv_screen_active(),"Instaluj"));
}
static void seed_battery(unsigned percent)
{
 dsr_event_t event={.result=percent<=100?DSR_OK:DSR_IO,.battery={.has_voltage=true,.adc_mv=2000,.has_percent=true,.percent_estimated=true,.percent=(uint8_t)percent}};
 dsr_ui_battery_result(&event);
}
static void power_flow(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,skin,language);dsr_ui_apply_theme();seed_battery(19);
 open_state(DSR_NATIVE_AVAILABLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);click("Instaluj");shot("battery-low");
 lv_obj_t *accept=lv_obj_get_parent(find_label(lv_screen_active(),"Instaluj"));CHECK(lv_obj_has_state(accept,LV_STATE_DISABLED));
 unsigned before=job_counts[DSR_JOB_OTA_INSTALL];click("Instaluj");CHECK(job_counts[DSR_JOB_OTA_INSTALL]==before&&!in_flight);
 click("Sprawdź baterię");CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ);
 response=(dsr_event_t){.battery={.has_voltage=true,.has_percent=true,.percent_estimated=true,.percent=19,.adc_mv=1745}};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Bateria poniżej 20%. Doładuj pilot do co najmniej 20% i sprawdź baterię ponownie."));
 CHECK(!find_label(lv_screen_active(),"Sprawdzanie baterii…"));
 CHECK(lv_obj_has_state(lv_obj_get_parent(find_label(lv_screen_active(),"Instaluj")),LV_STATE_DISABLED));
 CHECK(ota_box(lv_obj_get_parent(find_label(lv_screen_active(),"Sprawdź baterię"))).y2<=403);
 click("Sprawdź baterię");CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ);
 response=(dsr_event_t){.battery={.has_voltage=true,.has_percent=true,.percent_estimated=true,.percent=20,.adc_mv=1750}};deliver(DSR_OK,DSR_WIFI_IDLE);
 accept=lv_obj_get_parent(find_label(lv_screen_active(),"Instaluj"));CHECK(!lv_obj_has_state(accept,LV_STATE_DISABLED));shot("battery-ready");
 seed_battery(19);dsr_ui_native_ota_tick();CHECK(lv_obj_has_state(lv_obj_get_parent(find_label(lv_screen_active(),"Instaluj")),LV_STATE_DISABLED));
 lv_tick_inc(30000);dsr_ui_native_ota_tick();CHECK(!lv_obj_has_state(lv_obj_get_parent(find_label(lv_screen_active(),"Instaluj")),LV_STATE_DISABLED));shot("battery-unknown");
 lv_obj_t *unknown=find_label(lv_screen_active(),"Procent baterii nieznany. Zgoda jednorazowa: 120 s od sprawdzenia.");CHECK(unknown);
 CHECK(ota_box(lv_obj_get_parent(unknown)).y2<=403); /* Complete fallback notice fits above the footer. */
 CHECK(find_label(lv_screen_active(),"Instaluj potwierdza stabilne zasilanie USB. Nie odłączaj go podczas zapisu. Restart za 10 s; Później odracza."));
 open_state(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_BATTERY_LOW,DSR_MANIFEST_OK);shot("battery-worker-low");CHECK(!find_label(lv_screen_active(),"Instaluj"));
 if(skin==DSR_SC_STYLE_CYBER_BLUE){lv_obj_t *change=lv_obj_get_parent(find_label(lv_screen_active(),"Zmień kanał"));CHECK(lv_obj_get_child_count(change)>=2);}
 seed_battery(255);
}
static void countdown_review(void)
{
 skin=DSR_SC_STYLE_CYBER_BLUE;language=2;dsr_sc_appearance(DSR_SC_BLUE,skin,language);dsr_ui_apply_theme();restart_response=DSR_RESTART_ACCEPTED;
 open_state(DSR_NATIVE_STAGED,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);
 for(unsigned second=10;;second--){
  if(second<10){lv_tick_inc(1000);}
  dsr_ui_native_ota_tick();dsr_ui_native_ota_restart_tick();
  char text[80],path[80];snprintf(text,sizeof(text),dsr_sc_tr("Automatyczny restart za %u s"),second);CHECK(find_label(lv_screen_active(),text));
  CHECK(restart_requests==(second==0?1u:0u));snprintf(path,sizeof(path),"ota-countdown-%02u",second);screenshot(path);
  if(!second)break;
 }
 CHECK(!job_counts[DSR_JOB_OTA_INSTALL]);printf("PASS: countdown 10 through 0, exactly one MODEL restart\n");
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 test_workspace_disable();dsr_ui_init();tick();
 /* Complete the startup Wi-Fi scan before testing OTA's own job admission. */
 if(in_flight){CHECK(submitted.kind==DSR_JOB_WIFI_SCAN);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);}
 const unsigned skins[]={DSR_SC_STYLE_GRAPHITE,DSR_SC_STYLE_LIGHT,DSR_SC_STYLE_CYBER_BLUE};
 if(getenv("DSR_UI_OTA_COUNTDOWN_REVIEW")){countdown_review();lv_deinit();return 0;}
 for(unsigned i=0;i<3;i++){skin=skins[i];language=2;flow();}
 skin=DSR_SC_STYLE_CYBER_BLUE;for(language=0;language<2;language++)flow();
 CHECK(!job_counts[DSR_JOB_OTA_INSTALL]&&!restart_requests); /* review never installs */
 for(unsigned i=0;i<3;i++){skin=skins[i];language=2;power_flow();}
 skin=DSR_SC_STYLE_CYBER_BLUE;for(language=0;language<2;language++)power_flow();
 dsr_sc_appearance(DSR_SC_BLUE,DSR_SC_STYLE_GRAPHITE,0);dsr_ui_apply_theme();native_ota_regression();
 CHECK(!findings);printf("PASS: %u OTA UI checks, %u representative frames; actor/Flash/restart MODEL\n",checks,screenshots);lv_deinit();return 0;
}
