/* Atlas of the exactly three skins x PL/EN/DE over the same production LVGL
 * routes, with an automatic geometry audit of every captured 320x480 frame.
 * Radio, worker, storage, clock and OTA are fixtures; pixels are real LVGL. */
#define main original_ui_main
#include "test_ui.c"
#undef main
void test_native_ota_progress(size_t received,size_t total);
static const char *const theme_names[]={"graphite","light","cyber_blue"};
static const unsigned theme_ids[]={DSR_SC_STYLE_GRAPHITE,DSR_SC_STYLE_LIGHT,DSR_SC_STYLE_CYBER_BLUE};
static const char *const lang_names[]={"pl","en","de"};
static unsigned theme_at,lang_at,frames;
static FILE *report;
static char screen_id[96];
enum {F_CUT,F_OFFSCREEN,F_TINY,F_SMALL,F_OVERLAP,F_ELIDED,F_KINDS};
static const char *const finding_names[F_KINDS]={"text-cut","offscreen","tiny-target","small-target","overlap","elided"};
static unsigned findings[3][F_KINDS];

/* ---- Geometry audit ------------------------------------------------------ */
static void note(unsigned kind,lv_obj_t *o,const char *text)
{
 lv_area_t a;lv_obj_get_coords(o,&a);findings[theme_at][kind]++;
 fprintf(report,"%s\t%s\t%d,%d,%d,%d\t%s\n",screen_id,finding_names[kind],(int)a.x1,(int)a.y1,(int)lv_area_get_width(&a),(int)lv_area_get_height(&a),text?text:"");
}
static bool scrolls(lv_obj_t *o)
{return lv_obj_has_flag(o,LV_OBJ_FLAG_SCROLLABLE)&&(lv_obj_get_scroll_top(o)+lv_obj_get_scroll_bottom(o)>0||lv_obj_get_scroll_left(o)+lv_obj_get_scroll_right(o)>0);}
static bool inside_scroll(lv_obj_t *o)
{for(lv_obj_t *p=lv_obj_get_parent(o);p;p=lv_obj_get_parent(p))if(scrolls(p))return true;return false;}
static bool target(lv_obj_t *o)
{
 return lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE)&&!lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN)&&!lv_obj_has_state(o,LV_STATE_DISABLED)&&
  o!=lv_screen_active()&&o!=lv_layer_top()&&!scrolls(o)&&!lv_obj_check_type(o,&lv_label_class)&&!lv_obj_check_type(o,&lv_keyboard_class)&&
  !lv_obj_check_type(o,&lv_buttonmatrix_class)&&!lv_obj_check_type(o,&lv_slider_class)&&!lv_obj_check_type(o,&lv_arc_class)&&
  !lv_obj_check_type(o,&lv_switch_class)&&lv_obj_is_visible(o);
}
static void audit_label(lv_obj_t *o,const lv_area_t *a)
{
 const char *text=lv_label_get_text(o);size_t n=strlen(text);if(!n)return;
 if(n>=3&&!strcmp(text+n-3,"..."))note(F_ELIDED,o,text);
 lv_obj_t *p=lv_obj_get_parent(o);if(!p||p==lv_screen_active()||p==lv_layer_top()||scrolls(p))return;
 lv_area_t pa;lv_obj_get_coords(p,&pa);
 /* Wrapped lines below/above the container are cut off by it. */
 if(a->y2>pa.y2+1||a->y1<pa.y1-1){note(F_CUT,o,text);return;}
 if(a->x2>pa.x2+1||a->x1<pa.x1-1){
  lv_point_t size;lv_text_get_size(&size,text,lv_obj_get_style_text_font(o,LV_PART_MAIN),lv_obj_get_style_text_letter_space(o,LV_PART_MAIN),0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
  int visible=(a->x2<pa.x2?a->x2:pa.x2)-(a->x1>pa.x1?a->x1:pa.x1)+1;
  if(size.x>visible+1)note(F_CUT,o,text);
 }
}
static void audit(lv_obj_t *o)
{
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return;
 lv_area_t a;lv_obj_get_coords(o,&a);
 bool label=lv_obj_check_type(o,&lv_label_class);
 if(o!=lv_screen_active()&&o!=lv_layer_top()&&lv_obj_is_visible(o)){
  if((a.x1<0||a.x2>319||a.y1<0||a.y2>479)&&!inside_scroll(o)&&(label||target(o)))note(F_OFFSCREEN,o,label?lv_label_get_text(o):"");
  if(label)audit_label(o,&a);
  if(target(o)){int w=lv_area_get_width(&a),h=lv_area_get_height(&a),m=w<h?w:h;if(m<32)note(F_TINY,o,"");else if(m<44)note(F_SMALL,o,"");}
 }
 uint32_t count=lv_obj_get_child_count(o);
 for(uint32_t i=0;i<count;i++){
  lv_obj_t *c=lv_obj_get_child(o,(int32_t)i);if(!target(c))continue;lv_area_t ca;lv_obj_get_coords(c,&ca);
  for(uint32_t j=i+1;j<count;j++){lv_obj_t *d=lv_obj_get_child(o,(int32_t)j);if(!target(d))continue;lv_area_t da;lv_obj_get_coords(d,&da);
   if(ca.x1<da.x2&&da.x1<ca.x2&&ca.y1<da.y2&&da.y1<ca.y2)note(F_OVERLAP,c,"");}
 }
 for(uint32_t i=0;i<count;i++)audit(lv_obj_get_child(o,(int32_t)i));
}
/* Untouched production pixels: no test badge, no Cyber-only atlas copy. */
static void shot(const char *name)
{
 /* An optional exact route list refreshes only affected frames after the full
  * stage. The default CI atlas and its assertions still cover every route. */
 const char *filter=getenv("DSR_UI_ATLAS_SCREENS");
 if(filter&&*filter){bool selected=false;const char *p=filter;while(*p){const char *end=strchr(p,',');size_t n=end?(size_t)(end-p):strlen(p);if(strlen(name)==n&&!strncmp(name,p,n))selected=true;if(!end)break;p=end+1;}if(!selected)return;}
 lv_obj_update_layout(lv_screen_active());lv_refr_now(display);
 snprintf(screen_id,sizeof(screen_id),"%s-%s-%s",theme_names[theme_at],lang_names[lang_at],name);
 audit(lv_screen_active());audit(lv_layer_top());
 char path[512];snprintf(path,sizeof(path),"%s/skin-%s.ppm",output,screen_id);
 FILE *f=fopen(path,"wb");CHECK(f);fprintf(f,"P6\n320 480\n255\n");CHECK(fwrite(canvas,1,sizeof(canvas),f)==sizeof(canvas));fclose(f);frames++;
}
static void press(const char *text)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);if(!label)fprintf(stderr,"Skin atlas %s missing label: %s\n",screen_id,text);CHECK(label);
 lv_obj_t *b=lv_obj_get_parent(label);lv_obj_scroll_to_view(b,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
 lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);
}
/* Answer any LCD-started worker job with a plausible fixture result. */
static void settle(void)
{
 for(unsigned i=0;i<6&&in_flight;i++){
  dsr_job_kind_t kind=submitted.kind;response=(dsr_event_t){.device=submitted.connection.device};
  if(kind==DSR_JOB_BATTERY_PROFILE_LOAD||kind==DSR_JOB_BATTERY_PROFILE_SAVE)response.battery_profile=battery_stored;
  else if(kind==DSR_JOB_BATTERY_READ)response.battery=(dsr_battery_sample_t){.has_raw=true,.has_voltage=true,.raw=2001,.adc_mv=1925,.bat_mv=3850};
  else if(kind==DSR_JOB_WIFI_SCAN)response.scan=(dsr_wifi_scan_t){.count=2,.items={{.ssid="Home Wi-Fi",.rssi=-48,.channel=6,.security=DSR_WIFI_WPA2},{.ssid="Sąsiad – długa sieć 5G",.rssi=-81,.channel=11,.security=DSR_WIFI_WPA2_WPA3}}};
  deliver(DSR_OK,kind==DSR_JOB_WIFI_CONNECT?DSR_WIFI_CONNECTED:DSR_WIFI_IDLE);
 }
 CHECK(!in_flight);
}

/* ---- Fixture model: every device kind and data state --------------------- */
static void act_index_bool(unsigned i,bool on){(void)i;(void)on;}
static void act_void(void){}
static void act_index_int(unsigned i,int d){(void)i;(void)d;}
static void act_id(uint32_t id){(void)id;}
static void act_index(unsigned i){(void)i;}
static void act_index_u8(unsigned i,uint8_t v){(void)i;(void)v;}
static void act_kind(uint8_t k){(void)k;}
static void act_kind_id(uint8_t k,uint32_t id){(void)k;(void)id;}
static void act_id_bool(uint32_t id,bool on){(void)id;(void)on;}
static void act_rgb(unsigned i,uint8_t r,uint8_t g,uint8_t b){(void)i;(void)r;(void)g;(void)b;}
static const dsr_mobile_actions_t stub_actions={.set_output=act_index_bool,.open_settings=act_void,.add_device=act_void,.edit_dashboard=act_void,
 .set_cover=act_index_int,.select_dashboard=act_void,.refresh=act_id,.edit_device=act_id,.stop_cover=act_index,.set_position=act_index_u8,
 .add_room=act_void,.edit_room=act_id,.open_usage=act_id,.open_battery=act_void,.open_actions=act_kind,.leave_actions=act_void,
 .edit_action=act_kind_id,.run_scene=act_id,.run_group=act_id_bool,.set_brightness=act_index_u8,.set_rgb=act_rgb,.set_trv=act_index_u8,.device_settings=act_id};
static const char *const device_tags[]={"relay-on","relay-long-name-offline","relay-never-read","relay-sending","relay-auth-error","cover-moving","cover-open",
 "dimmer","rgb","ht","blu-door-open","blu-motion","trv","plug-stale"};
static dsr_mobile_model_t model_all(void)
{
 dsr_mobile_model_t m={.wifi=true,.home_name="Mój dom",.room_count=2,.rooms={{.name="Salon",.id=1},{.name="Sypialnia na poddaszu z oknem dachowym",.id=2}}};
 uint32_t now=lv_tick_get();unsigned n=0;dsr_mobile_device_t *d=m.devices;
#define BASE .generation=DSR_GEN2_PLUS,.can_refresh=true,.fields=255
#define SAMPLED .has_sample_time=true,.sample_ms=now
 d[n++]=(dsr_mobile_device_t){.id=1,.name="Lampa salon",.identity="atlas-1",.endpoint_kind=DSR_ENDPOINT_SWITCH,.kind=DSR_DEVICE_RELAY,.room=0,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=18.6,.has_voltage=true,.voltage_v=232.9,.has_current=true,.current_a=.081,.has_energy=true,.energy_wh=12345.6,BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=2,.name="Gniazdko w garderobie przy oknie balkonowym",.identity="atlas-2",.endpoint_kind=DSR_ENDPOINT_SWITCH,.kind=DSR_DEVICE_PLUG,.room=1,.error=DSR_OFFLINE,.can_switch=true,BASE,.has_sample_time=true,.sample_ms=now-60000};
 d[n++]=(dsr_mobile_device_t){.id=3,.name="Kinkiet",.identity="atlas-3",.endpoint_kind=DSR_ENDPOINT_SWITCH,.kind=DSR_DEVICE_RELAY,.room=0,.can_switch=true,BASE};
 d[n++]=(dsr_mobile_device_t){.id=4,.name="Bojler",.identity="atlas-4",.endpoint_kind=DSR_ENDPOINT_SWITCH,.kind=DSR_DEVICE_RELAY,.room=0,.online=true,.busy=true,.command_pending=true,.can_switch=true,.has_power=true,.power_w=0,BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=5,.name="Pompa ogrodowa",.identity="atlas-5",.endpoint_kind=DSR_ENDPOINT_SWITCH,.kind=DSR_DEVICE_RELAY,.room=255,.error=DSR_AUTH_REQUIRED,.can_switch=true,BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=6,.name="Roleta salon",.identity="atlas-6",.endpoint_kind=DSR_ENDPOINT_COVER,.kind=DSR_DEVICE_COVER,.room=0,.online=true,.can_cover=true,.can_position=true,.can_stop_cover=true,.has_position=true,.position_pct=45,.cover_state=DSR_COVER_OPENING,.has_power=true,.power_w=96.2,BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=7,.name="Roleta sypialnia",.identity="atlas-7",.endpoint_kind=DSR_ENDPOINT_COVER,.kind=DSR_DEVICE_COVER,.room=1,.online=true,.can_cover=true,.can_position=true,.can_stop_cover=true,.has_position=true,.position_pct=100,.cover_state=DSR_COVER_STOPPED,BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=8,.name="Ściemniacz jadalnia",.identity="atlas-8",.endpoint_kind=DSR_ENDPOINT_LIGHT,.kind=DSR_DEVICE_LIGHT,.room=0,.online=true,.output=true,.can_switch=true,.can_light=true,.light={.brightness=75,.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS},.has_power=true,.power_w=24.8,BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=9,.name="Taśma LED",.identity="atlas-9",.endpoint_kind=DSR_ENDPOINT_RGB,.kind=DSR_DEVICE_RGB,.room=1,.online=true,.output=true,.can_switch=true,.can_rgb=true,.rgb={.r=0,.g=180,.b=255,.brightness=78,.flags=DSR_RGB_VALID},BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=10,.name="Termometr H&T",.identity="atlas-10",.endpoint_kind=DSR_ENDPOINT_HT,.kind=DSR_DEVICE_SENSOR,.room=1,.online=true,.has_temperature=true,.temperature_c=21.4,.has_humidity=true,.humidity_pct=46,.has_battery=true,.battery_pct=85,.ht={.flags=DSR_HT_VALID},BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=11,.name="Drzwi tarasowe",.identity="atlas-11",.endpoint_kind=DSR_ENDPOINT_BLU,.kind=DSR_DEVICE_CONTACT,.room=0,.online=true,.has_contact=true,.contact_open=true,.has_battery=true,.battery_pct=72,.blu_age_s=12,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|DSR_BLU_OPEN},BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=12,.name="Ruch korytarz",.identity="atlas-12",.endpoint_kind=DSR_ENDPOINT_BLU,.kind=DSR_DEVICE_SENSOR,.room=0,.online=true,.has_battery=true,.battery_pct=9,.blu_age_s=40,.has_illuminance=true,.illuminance_lux=120,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_MOTION|DSR_BLU_MOTION},BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=13,.name="Grzejnik salon",.identity="atlas-13",.endpoint_kind=DSR_ENDPOINT_BLU,.kind=DSR_DEVICE_SENSOR,.room=0,.online=true,.has_temperature=true,.temperature_c=20.4,.has_trv_target=true,.trv_target_c=21.5,.has_battery=true,.battery_pct=64,.blu_age_s=30,.blu={.flags=DSR_BLU_VALID,.trv_pos=35,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET|DSR_BLU_TRV_CHECKED},BASE,SAMPLED};
 d[n++]=(dsr_mobile_device_t){.id=14,.name="Gniazdko biurko",.identity="atlas-14",.endpoint_kind=DSR_ENDPOINT_SWITCH,.kind=DSR_DEVICE_PLUG,.room=1,.can_switch=true,BASE,.has_sample_time=true,.sample_ms=now-60000};
#undef SAMPLED
#undef BASE
 m.device_count=(uint8_t)n;return m;
}

/* ---- Routes --------------------------------------------------------------- */
static void apply(void)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme_names[theme_at]));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(lang_names[lang_at]));publish_document(doc);
 dsr_network_home(NULL);tick();CHECK(dsr_sc_style()==theme_ids[theme_at]&&dsr_sc_language()==lang_at);
}
static void system_routes(void)
{
 dsr_network_home(NULL);tick();shot("00-home-network");
 dsr_ui_open_settings();shot("01-settings");lv_obj_scroll_to_view(lv_obj_get_parent(find_label(lv_screen_active(),"System")),LV_ANIM_OFF);shot("74-settings-bottom");press("Sieć i połączenia");shot("02-network");
 dsr_ui_open_settings();press("Wygląd i język");shot("03-appearance");
 dsr_ui_open_settings();press("Ekran i komfort");press("Ekran");shot("04-display");
 dsr_ui_open_settings();press("Panel WWW / bezpieczeństwo");shot("05-web-panel");
 dsr_ui_open_settings();press("System");shot("06-system");press("Informacje i diagnostyka");shot("07-diagnostics");
 dsr_ui_open_settings();press("System");press("Restart pilota");shot("08-restart-confirm");
 dsr_ui_battery_open(NULL);shot("09-battery-unknown");
 dsr_event_t e={.result=DSR_OK,.battery={.has_raw=true,.has_voltage=true,.raw=2001,.adc_mv=1900,.bat_mv=3800}};dsr_ui_battery_result(&e);shot("10-battery-measured");
 press("Kalibracja baterii");settle();shot("11-battery-calibration");
 test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);dsr_ui_native_ota_open(NULL);shot("12-ota");
 test_native_ota_set(DSR_NATIVE_INSTALLING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);test_native_ota_progress(500,1000);dsr_ui_native_ota_tick();shot("13-ota-installing");
 test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_OFFLINE,DSR_MANIFEST_OK);dsr_ui_native_ota_tick();shot("14-ota-error");
 test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_INVALID);dsr_network_home(NULL);tick();
}
static void onboarding_routes(void)
{
 dsr_network_wifi(NULL);settle();shot("15-wifi");dsr_network_home(NULL);tick();
 dsr_network_device(NULL);shot("16-add-method");choose_method("Dodaj ręcznie (IP)");shot("17-manual-ip");
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_obj_send_event(field,LV_EVENT_CLICKED,NULL);shot("18-manual-ip-keyboard");
 dsr_network_home(NULL);tick();dsr_ui_ble_open(NULL);shot("19-ble");
 dsr_network_home(NULL);tick();dsr_ui_lan_open(NULL);shot("20-lan");
 dsr_network_home(NULL);tick();dsr_ui_ap_open(NULL);shot("21-ap");
 click("Szukaj AP");shot("63-ap-scanning");response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);shot("64-ap-empty");
 click("Ponów");response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);shot("65-ap-error");
 click("Ponów");response=(dsr_event_t){.scan={.count=2,.items={{.ssid="ShellyPlus1-112233445566",.security=DSR_WIFI_OPEN,.bssid={1,2,3,4,5,6},.channel=6},{.ssid="ShellyPlus1-AABBCCDDEEFF",.security=DSR_WIFI_WPA2}}}};deliver(DSR_OK,DSR_WIFI_IDLE);shot("66-ap-found");
 click("ShellyPlus1-112233445566");shot("67-ap-connect-consent");ap_disclose();shot("68-ap-share-consent");ap_cancel_finish();wifi.connected=true;
 dsr_network_home(NULL);tick();
}
static void editing_routes(void)
{
 dsr_ui_actions_open(1);shot("22-scenes");dsr_network_home(NULL);tick();dsr_ui_actions_open(0);shot("23-groups");
 dsr_network_home(NULL);tick();dsr_ui_edit_dashboard();shot("24-editor-menu");
 dsr_network_home(NULL);tick();dsr_ui_edit_room(1);shot("25-room-edit");
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_obj_send_event(field,LV_EVENT_CLICKED,NULL);shot("26-room-edit-keyboard");
 dsr_network_home(NULL);tick();dsr_ui_edit_room(1);room_delete_button();shot("27-room-delete-confirm");
 dsr_network_home(NULL);tick();test_workspace_fault(1);dsr_ui_edit_dashboard();shot("28-storage-unavailable");test_workspace_fault(0);
 dsr_network_home(NULL);tick();
}
static void device_routes(void)
{
 /* The network view is left first so a later poll cannot replace the model. */
 dsr_ui_open_settings();dsr_mobile_bind(&stub_actions);
 dsr_mobile_model_t m=model_all();dsr_mobile_update(&m);
 dsr_mobile_show(DSR_MOBILE_HOME,0);shot("30-home");
 dsr_mobile_model_t one=m;one.device_count=1;one.devices[0]=m.devices[12];dsr_mobile_update(&one);dsr_mobile_show(DSR_MOBILE_HOME,0);shot("70-home-trv");
 one.devices[0]=m.devices[10];dsr_mobile_update(&one);shot("71-home-window");
 snprintf(one.devices[0].name,sizeof(one.devices[0].name),"Deckenbeleuchtung im großen Wohnzimmer");snprintf(one.dashboard_name,sizeof(one.dashboard_name),"Mein Dashboard im Erdgeschoss");dsr_mobile_update(&one);shot("72-home-long-names");
 dsr_mobile_update(&m);
 dsr_mobile_show(DSR_MOBILE_ROOMS,0);shot("31-rooms");
 dsr_mobile_show(DSR_MOBILE_ROOM,0);shot("32-room");
 dsr_mobile_show(DSR_MOBILE_DEVICES,0);shot("33-devices");
 for(unsigned i=0;i<m.device_count;i++){char name[64];snprintf(name,sizeof(name),"%u-device-%s",40+i,device_tags[i]);dsr_mobile_show(DSR_MOBILE_DETAILS,i);shot(name);}
 m.wifi=false;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);shot("60-home-no-wifi");
 dsr_mobile_model_t empty={.wifi=true,.home_name="Mój dom"};dsr_mobile_update(&empty);dsr_mobile_show(DSR_MOBILE_HOME,0);shot("61-home-empty");
 dsr_mobile_show(DSR_MOBILE_ROOMS,0);shot("62-rooms-empty");
 dsr_network_home(NULL);tick();
}
static void setup(void)
{
 cJSON *doc=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"rooms",cJSON_Parse("[{\"id\":1,\"name\":\"Salon\",\"icon\":0,\"photo\":\"\"},{\"id\":2,\"name\":\"Sypialnia na poddaszu\",\"icon\":2,\"photo\":\"\"}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Lampa salon\",\"kind\":\"relay\",\"host\":\"192.0.2.20\",\"channel\":0,\"room\":1},{\"id\":2,\"name\":\"Roleta salon\",\"kind\":\"cover\",\"host\":\"192.0.2.21\",\"channel\":0,\"room\":1}]"));
 publish_document(doc);tick();
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 char path[512];snprintf(path,sizeof(path),"%s/skin-audit%s.tsv",output,getenv("DSR_UI_ATLAS_SCREENS")?"-focused":"");report=fopen(path,"w");CHECK(report);
 fprintf(report,"screen\tfinding\tx,y,w,h\ttext\n");
 setup();
 for(theme_at=0;theme_at<3;theme_at++)for(lang_at=0;lang_at<3;lang_at++){apply();system_routes();onboarding_routes();editing_routes();device_routes();}
 CHECK(!fclose(report));
 /* Never a real command, install or restart while rendering the atlas. */
 CHECK(!job_counts[DSR_JOB_SET_OUTPUT]&&!job_counts[DSR_JOB_COVER]&&!job_counts[DSR_JOB_OTA_INSTALL]&&!restart_requests);
 for(unsigned t=0;t<3;t++){printf("Skin %s audit:",theme_names[t]);for(unsigned k=0;k<F_KINDS;k++)printf(" %s=%u",finding_names[k],findings[t][k]);printf("\n");CHECK(!findings[t][F_CUT]);}
 printf("PASS: %u skin atlas frames (3 skins x PL/EN/DE), %u checks; geometry findings in skin-audit.tsv; host providers only\n",frames,checks);
 lv_deinit();return 0;
}
