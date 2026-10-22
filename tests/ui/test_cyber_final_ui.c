/* Production LVGL, pointer gestures and durable ACK sequencing. Platform,
 * radio, ADC and Flash are explicit host providers; no hardware acceptance. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_cyber.h"
static lv_area_t final_box(lv_obj_t *o){CHECK(o);lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(o,&a);return a;}
static void final_press(const char *text)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label);lv_obj_t *b=lv_obj_get_parent(label);lv_obj_scroll_to_view(b,LV_ANIM_OFF);lv_area_t a=final_box(b);CHECK(a.y1>=0&&a.y2<480);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static void final_contours(lv_obj_t *o)
{
 if(lv_obj_get_style_border_width(o,0)>0){CHECK(lv_obj_get_style_border_width(o,0)==DSR_CYBER_BORDER_WIDTH);uint32_t c=lv_color_to_u32(lv_obj_get_style_border_color(o,0))&0xffffff;CHECK(c==DSR_CYBER_CYAN||c==DSR_CYBER_DANGER||c==0xffc107||c==0xffc65b||c==0xffbf24||c==0xf0a020||c==0xe1bb70);}
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)final_contours(lv_obj_get_child(o,(int)i));
}
static void final_snap(const char *name,unsigned lang){char s[100];snprintf(s,sizeof(s),"native-final-%s-%s",name,lang==0?"pl":lang==1?"en":"de");screenshot(s);final_contours(lv_screen_active());}
static void gesture(int from,int to)
{
 touch=(lv_point_t){280,from};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);
 int step=to>from?8:-8;for(int y=from;step>0?y<to:y>to;y+=step){touch.y=y;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();}
 pressed=false;lv_tick_inc(20);lv_indev_read(pointer);for(unsigned i=0;i<30;i++){lv_tick_inc(20);lv_timer_handler();}
}
static void fixture(unsigned lang)
{
 cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(lang==0?"pl":lang==1?"en":"de"));cJSON_ReplaceItemInObjectCaseSensitive(r,"theme",cJSON_CreateString("cyber_blue"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"rooms",cJSON_Parse("[{\"id\":1,\"name\":\"Salon\",\"icon\":0,\"photo\":\"\"}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Kanał pierwszy Żółć ÄÖÜß\",\"host\":\"192.0.2.20\",\"channel\":0,\"kind\":\"relay\",\"room\":1},{\"id\":2,\"name\":\"Drugi kanał\",\"host\":\"192.0.2.20\",\"channel\":1,\"kind\":\"relay\",\"room\":1}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"groups",cJSON_Parse("[{\"id\":1,\"name\":\"Grupa\",\"members\":[1,2]}]"));cJSON_ReplaceItemInObjectCaseSensitive(r,"scenes",cJSON_Parse("[{\"id\":1,\"name\":\"Scena\",\"actions\":[{\"device\":1,\"command\":\"on\"},{\"device\":2,\"command\":\"off\"}]}]"));
 cJSON *dash=cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child;cJSON_ReplaceItemInObjectCaseSensitive(dash,"tiles",cJSON_Parse("[{\"id\":1,\"type\":\"device\",\"ref\":1,\"span\":2,\"height\":1,\"fields\":255},{\"id\":2,\"type\":\"device\",\"ref\":2,\"span\":2,\"height\":1,\"fields\":255}]"));
 publish_document(r);dsr_network_home(NULL);tick();
}
static void battery_gestures(unsigned lang)
{
 dsr_ui_battery_open(NULL);lv_obj_t *power=lv_obj_get_parent(find_label(lv_screen_active(),"— V")),*viewport=lv_obj_get_parent(power);CHECK(lv_obj_has_flag(viewport,LV_OBJ_FLAG_CLICKABLE));
 unsigned n=requests;final_snap("battery-top",lang);gesture(378,120);gesture(378,120);CHECK(lv_obj_get_scroll_y(viewport)>0&&lv_obj_get_scroll_bottom(viewport)<=2);CHECK(requests==n);final_snap("battery-bottom",lang);
 gesture(120,378);gesture(120,378);CHECK(lv_obj_get_scroll_y(viewport)==0);CHECK(final_box(lv_obj_get_parent(find_label(lv_screen_active(),"Wstecz"))).y1==420);
 tap(26,22);CHECK(find_label(lv_screen_active(),"Ustawienia pilota"));
 dsr_ui_battery_show();lv_obj_update_layout(lv_screen_active());tap(26,22);CHECK(dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_HOME);
}
static cJSON *workspace_document(void){char *json=dsr_workspace_snapshot();cJSON *r=json?cJSON_Parse(json):NULL;free(json);CHECK(r);return r;}
static void deletion(unsigned lang)
{
 dsr_ui_edit_local_device(1);final_press("Usuń urządzenie");char question[128];snprintf(question,sizeof(question),dsr_sc_tr("Usunąć \"%s\"?"),"Kanał pierwszy Żółć ÄÖÜß");CHECK(find_label(lv_screen_active(),question));final_snap("delete-confirm",lang);
 final_press("Anuluj");
 cJSON *doc=workspace_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"devices"))==2);cJSON_Delete(doc);
 final_press("Usuń urządzenie");test_workspace_defer(true);final_press("Usuń");CHECK(find_label(lv_screen_active(),"Zapisywanie…"));doc=workspace_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"devices"))==2);cJSON_Delete(doc);
 test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Nie potwierdzono zapisu usunięcia. Spróbuj ponownie."));final_snap("delete-write-error",lang);
 final_press("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Urządzenie usunięte."));final_snap("delete-success",lang);test_workspace_defer(false);
 doc=workspace_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"devices"))==1);CHECK(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"devices")->child,"channel")->valueint==1);
 CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"scenes")->child,"actions"))==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"groups")->child,"members"))==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"rooms"))>0);cJSON_Delete(doc);
 final_press("Wstecz");CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICES);
 dsr_ui_edit_local_device(2);lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_textarea_set_text(field,"Niezapisana zmiana");lv_obj_update_layout(lv_screen_active());tap(26,22);CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));final_press("Zostań");CHECK(!strcmp(lv_textarea_get_text(field),"Niezapisana zmiana"));tap(26,22);final_press("Odrzuć");
}
static void dashboard_rows(unsigned lang)
{
 dsr_mobile_model_t m={.wifi=true,.device_count=3,.room_count=1,.rooms={{.id=1,.name="Pomieszczenie z bardzo długą nazwą ÄÖÜß"}}};
 for(unsigned i=0;i<3;i++)m.devices[i]=(dsr_mobile_device_t){.id=31+i,.name="Długa nazwa urządzenia Żółć ÄÖÜß",.identity="final-fixture",.kind=i==1?DSR_DEVICE_COVER:DSR_DEVICE_RELAY,.endpoint_kind=i==1?DSR_ENDPOINT_COVER:DSR_ENDPOINT_SWITCH,.online=i!=2,.output=i==0,.room=i==2?255:0,.cover_state=DSR_COVER_OPENING};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_t *name=find_label(lv_screen_active(),"Długa nazwa urządzenia Żółć ÄÖÜß");CHECK(name);lv_obj_t *card=lv_obj_get_parent(name);lv_obj_t *state=find_label(card,"Włączone");CHECK(state);lv_area_t a=final_box(state),c=final_box(card);CHECK(a.y1>final_box(name).y2&&a.y2<c.y2-24&&lv_obj_get_height(card)>=110);CHECK(!strcmp(m.rooms[0].name,"Pomieszczenie z bardzo długą nazwą ÄÖÜß"));final_snap("dashboard-two-rows",lang);
}
static void indicators(unsigned lang)
{
 dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=4120}};dsr_ui_battery_result(&e);dsr_network_home(NULL);CHECK(find_label(lv_screen_active(),"4.12V"));final_snap("header-voltage",lang);
 const unsigned levels[]={0,7,50,100};for(unsigned i=0;i<4;i++){e.battery.has_percent=true;e.battery.percent=(uint8_t)levels[i];dsr_ui_battery_result(&e);char name[48];snprintf(name,sizeof(name),"header-provider-fixture-%u",levels[i]);final_snap(name,lang);}
 e.battery.has_charge_state=true;e.battery.charging=true;dsr_ui_battery_result(&e);final_snap("header-charging-provider-fixture",lang);lv_mem_monitor_t a,b;lv_mem_monitor(&a);for(unsigned i=0;i<100;i++){lv_tick_inc(200);dsr_ui_battery_tick();}lv_mem_monitor(&b);CHECK(a.free_size==b.free_size);
 display_sleeping=true;dsr_ui_battery_tick();final_snap("header-sleep-paused",lang);display_sleeping=false;e.battery.charging=false;dsr_ui_battery_result(&e);lv_tick_inc(30001);dsr_ui_battery_tick();CHECK(find_label(lv_screen_active(),"—"));final_snap("header-stale",lang);
 e.battery=(dsr_battery_sample_t){0};dsr_ui_battery_result(&e);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 for(unsigned lang=0;lang<3;lang++){fixture(lang);battery_gestures(lang);deletion(lang);dashboard_rows(lang);indicators(lang);}
 printf("PASS: %u final Cyber checks, %u frames; real pointer scroll, localized deletion/failure, optional battery capabilities; host providers\n",checks,screenshots);lv_deinit();return 0;
}
