/* Owner's LCD reports, real LVGL 9.3 pixels and pointer events.
 * Network/ADC/storage remain fixture providers. No claim of physical RF. */
#define main original_ui_main
#include "test_ui.c"
#undef main

static lv_area_t rect(lv_obj_t *o)
{lv_area_t a;CHECK(o);lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(o,&a);return a;}
static void shot(const char *name,unsigned theme)
{char path[96];snprintf(path,sizeof(path),"dev069-%u-%s",theme,name);screenshot(path);}
static void common(unsigned theme)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString("de"));publish_document(doc);dsr_network_home(NULL);tick();dsr_ui_open_settings();CHECK(dsr_sc_style()==theme);
 dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3902,.has_percent=true,.percent=63,.percent_estimated=true}};
 dsr_ui_battery_result(&e);
 lv_obj_t *percent=find_label(lv_screen_active(),"~63%");CHECK(percent);
 lv_area_t a=rect(percent),b=rect(lv_obj_get_parent(percent));
 CHECK(a.y1>=b.y1+8&&a.y2<=b.y1+32); /* percentage belongs INSIDE the battery */
 CHECK(a.x1>=b.x1+4&&a.x2<=b.x2-4);
 lv_obj_t *last=find_label(lv_screen_active(),"System");CHECK(last);
 lv_obj_t *row=lv_obj_get_parent(last);lv_obj_scroll_to_view(row,LV_ANIM_OFF);a=rect(row);
 CHECK(a.y2<=420&&a.x1>=12&&a.x2<=307); /* full last row, before bottom navigation */
 shot("settings-bottom",theme);
}
static lv_obj_t *containing(lv_obj_t *o,const char *text)
{
 if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o),text))return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=containing(lv_obj_get_child(o,(int)i),text);if(r)return r;}return NULL;
}
static void noop(void){}
static void trv_noop(unsigned index,uint8_t target){(void)index;(void)target;}
static void dashboard(unsigned theme)
{
 dsr_ui_open_settings();dsr_mobile_actions_t actions={.add_device=noop,.edit_dashboard=noop,.select_dashboard=noop,.set_trv=trv_noop};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard im Erdgeschoss",.room_count=1,.rooms={{.id=1,.name="Küche und Esszimmer"}},.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=1,.identity="dev069-relay",.name="Deckenbeleuchtung Küche",.room=0,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true,.can_switch=true,.has_power=true,.power_w=0,.has_voltage=true,.voltage_v=231.8,.fields=255};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *name=find_label(lv_screen_active(),m.dashboard_name);CHECK(name);lv_area_t selector=rect(lv_obj_get_parent(name));CHECK(selector.y1<60&&selector.x2<170);
 lv_obj_t *measurement=containing(lv_screen_active(),"231.8 V");CHECK(measurement&&strstr(lv_label_get_text(measurement),"0.0 W")&&!strstr(lv_label_get_text(measurement),"..."));
 lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));lv_area_t a=rect(card),v=rect(lv_obj_get_parent(measurement));
 CHECK(v.x1>=a.x1+60&&v.x2<=a.x2-80&&v.y2<a.y2-5&&v.y1<a.y1+100);shot("dashboard-relay",theme);
 m.devices[0].power_w=12345.6;dsr_mobile_update(&m);measurement=containing(lv_screen_active(),"231.8 V");CHECK(measurement&&strstr(lv_label_get_text(measurement),"12345.6 W"));
 /* Real TRV +/- hit areas, without relying on expanded invisible targets. */
 m.devices[0]=(dsr_mobile_device_t){.id=2,.identity="dev069-trv",.name="Thermostat Schlafzimmer",.room=0,.kind=DSR_DEVICE_SENSOR,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_temperature=true,.temperature_c=21.5,.has_trv_target=true,.trv_target_c=21,.blu_age_s=1,.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET|DSR_BLU_TRV_CHECKED}};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *minus=find_label(lv_screen_active(),"-"),*plus=find_label(lv_screen_active(),"+");CHECK(minus&&plus);
 CHECK(lv_obj_get_width(lv_obj_get_parent(minus))>=48&&lv_obj_get_height(lv_obj_get_parent(plus))>=48);
 card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));a=rect(card);v=rect(lv_obj_get_parent(lv_obj_get_parent(minus)));CHECK(v.x1>=a.x1+8&&v.x2<a.x2-5&&v.y1>a.y1+80);CHECK(find_label(card,"21.0 °C"));CHECK(lv_obj_get_height(card)<=170&&!containing(card,"Ventil"));shot("dashboard-trv",theme);
 m.devices[0]=(dsr_mobile_device_t){.id=3,.identity="dev069-window",.name="Fenster Schlafzimmer",.room=0,.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_contact=true,.contact_open=true,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|DSR_BLU_OPEN}};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);CHECK(!find_label(lv_screen_active(),"Brak dodatkowych pomiarów"));card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));lv_obj_t *box=lv_obj_get_child(card,3);a=rect(card);v=rect(box);CHECK(lv_obj_get_height(box)==56&&lv_obj_get_height(card)<=132&&v.y1>a.y1+24);shot("dashboard-window",theme);
 dsr_mobile_show(DSR_MOBILE_DEVICES,0);shot("devices-tabs",theme);
 m.devices[0].has_illuminance=true;m.devices[0].illuminance_lux=0;strcpy(m.devices[0].name,"Deckenbeleuchtung im großen Wohnzimmer");dsr_mobile_update(&m);CHECK(containing(lv_screen_active(),"0 lx"));shot("window-long-name-real-reading",theme);
}
static void access_point(unsigned theme)
{
 wifi.connected=false;dsr_network_home(NULL);tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 wifi.connected=true;dsr_ui_ap_open(NULL);
 lv_obj_update_layout(lv_screen_active());
 lv_obj_t *battery=NULL;for(unsigned i=0;i<lv_obj_get_child_count(lv_screen_active());i++){lv_obj_t *o=lv_obj_get_child(lv_screen_active(),(int)i);if(lv_obj_get_x(o)==256)battery=o;}
 CHECK(battery&&lv_obj_is_visible(battery)&&lv_obj_get_child_count(battery)==1);
 shot("ap-intro",theme);click("Szukaj AP");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN);shot("ap-scanning",theme);
 response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Nie znaleziono fabrycznego AP Shelly. Włącz tryb konfiguracji urządzenia."));shot("ap-empty",theme);
 click("Ponów");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Ponów"));lv_obj_t *notice=find_label(lv_screen_active(),"Nie potwierdzono dodania. Domowe ustawienia pilota zachowane.");CHECK(notice);lv_obj_t *frame=lv_obj_get_parent(notice);CHECK(lv_obj_get_width(frame)==296&&lv_obj_get_x(notice)>=12);if(theme==3)CHECK(lv_obj_get_style_border_width(frame,0)>=2);shot("ap-error",theme);
 unsigned writes=job_counts[DSR_JOB_AP_PROVISION];click("Ponów");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN&&job_counts[DSR_JOB_AP_PROVISION]==writes);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);click("Anuluj");
 ap_pick();shot("ap-connect-consent",theme);ap_disclose();shot("ap-share-consent",theme);ap_cancel_finish();CHECK(job_counts[DSR_JOB_AP_PROVISION]==writes&&dsr_sc_style()==theme);
}
static unsigned position_writes,position_target;
static void position_set(unsigned index,uint8_t target){CHECK(index==0);position_writes++;position_target=target;}
static void pointer_move(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_refr_now(display);}
static void cover_skin(unsigned theme)
{
 dsr_ui_open_settings();dsr_mobile_actions_t a={.set_position=position_set};dsr_mobile_bind(&a);
 dsr_mobile_model_t m={.wifi=true,.device_count=1};m.devices[0]=(dsr_mobile_device_t){.id=4,.identity="dev069-cover",.name="Rollladen Terrasse",.kind=DSR_DEVICE_COVER,.endpoint_kind=DSR_ENDPOINT_COVER,.online=true,.can_cover=true,.can_stop_cover=true,.can_position=true,.has_position=true,.position_pct=50,.cover_state=DSR_COVER_STOPPED};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_t *title=find_label(lv_screen_active(),m.devices[0].name);CHECK(title&&lv_obj_get_y(lv_obj_get_parent(title))==52&&lv_obj_get_style_text_align(title,0)==LV_TEXT_ALIGN_CENTER);CHECK(!has_icon(lv_screen_active(),DSR_ICON_EDIT,24));shot("cover-stopped",theme);
 char confirmed[40];snprintf(confirmed,sizeof(confirmed),"%s 50%%",dsr_sc_tr("Pozycja"));
 unsigned before=position_writes;pointer_move(150,226,true);CHECK(position_writes==before);CHECK(find_label(lv_screen_active(),confirmed));shot("cover-preview",theme);pointer_move(150,226,false);CHECK(position_writes==before+1&&position_target>=75&&position_target<=85);
 m.devices[0].position_pct=80;m.devices[0].cover_state=DSR_COVER_OPENING;dsr_mobile_update(&m);lv_tick_inc(150);lv_timer_handler();shot("cover-moving",theme);
 m.devices[0].has_position=false;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Nieznana pozycja"));shot("cover-unknown",theme);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 const unsigned themes[]={0,2,3};for(unsigned i=0;i<3;i++){common(themes[i]);dashboard(themes[i]);access_point(themes[i]);cover_skin(themes[i]);}
 printf("PASS: dev069 focused UI, %u assertions, %u rendered frames; host providers\n",checks,screenshots);
 lv_deinit();return 0;
}
