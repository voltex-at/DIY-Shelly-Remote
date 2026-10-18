/* Production LCD editors; workspace/transport providers are MODELS. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_ui_search.h"
static void editor_keyboard_bounds(void)
{
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb&&!lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));lv_area_t keys;lv_obj_get_coords(kb,&keys);CHECK(keys.y1==240);
 lv_obj_t *root=lv_screen_active();for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *c=lv_obj_get_child(root,(int)i);if(c==kb||lv_obj_has_flag(c,LV_OBJ_FLAG_HIDDEN))continue;lv_area_t box;lv_obj_get_coords(c,&box);CHECK(box.y2<keys.y1);}
}
static void press_order(bool down)
{
 lv_obj_t *label=find_label(lv_screen_active(),"1. Roleta");CHECK(label);lv_obj_t *panel=lv_obj_get_parent(label),*button=NULL;
 for(unsigned i=0;i<lv_obj_get_child_count(panel);i++){lv_obj_t *b=lv_obj_get_child(panel,(int)i);if(lv_obj_get_x(b)==(down?170:118)&&lv_obj_get_y(b)==48)button=b;}
 CHECK(button);lv_obj_scroll_to_view(button,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());lv_area_t box;lv_obj_get_coords(button,&box);CHECK(box.y2<420&&box.y1>=138);tap((box.x1+box.x2)/2,(box.y1+box.y2)/2);
}
static void more_editor_checks(void)
{
 unsigned execution_before=requests;
 dsr_ui_edit_action(1,0);action_editor_name("Cover test");click("Dalej");click("+ Dodaj polecenie");action_editor_select("Roleta",true);CHECK(find_label(lv_screen_active(),"Otwórz")&&find_label(lv_screen_active(),"Zamknij"));click("Stop");
 click("+ Dodaj polecenie");action_editor_select("Relay A",true);click("Wyłącz");press_order(true);CHECK(find_label(lv_screen_active(),"1. Relay A"));
 click("Wyłącz");click("Zmień urządzenie");click("Wstecz");CHECK(find_label(lv_screen_active(),"1. Relay A"));click("Usuń");CHECK(find_label(lv_screen_active(),"1. Roleta"));click("Dalej");click("Zapisz scenę");dsr_ui_workspace_tick();
 cJSON *doc=room_document(),*scene=cJSON_GetObjectItemCaseSensitive(doc,"scenes")->child;CHECK(scene);cJSON *a=cJSON_GetObjectItemCaseSensitive(scene,"actions");CHECK(cJSON_GetArraySize(a)==1&&cJSON_GetObjectItemCaseSensitive(a->child,"device")->valueint==3&&!strcmp(cJSON_GetObjectItemCaseSensitive(a->child,"command")->valuestring,"stop"));cJSON_Delete(doc);
 const char *languages[]={"pl","en","de"};char frame[64];
 for(unsigned l=0;l<3;l++){
  doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(languages[l]));publish_document(doc);dsr_network_home(NULL);tick();dsr_ui_edit_action(0,0);
  action_editor_name("   ");click("Dalej");CHECK(find_label(lv_screen_active(),"Nazwa grupy"));
  editor_keyboard_bounds();snprintf(frame,sizeof(frame),"410-group-keyboard-%s",languages[l]);screenshot(frame);action_editor_name("Lampy na parterze");
  click("Dalej");CHECK(!find_label(lv_screen_active(),"Roleta"));action_editor_select("Relay A",false);snprintf(frame,sizeof(frame),"411-group-selection-%s",languages[l]);screenshot(frame);click("Dalej");snprintf(frame,sizeof(frame),"412-group-review-%s",languages[l]);screenshot(frame);
  click("Zmień nazwę i opcje");click("Anuluj");click("Odrzuć");
  dsr_ui_edit_action(1,0);action_editor_name("Dobranoc");click("Dalej");click("+ Dodaj polecenie");action_editor_select("Relay B",true);click("Wyłącz");snprintf(frame,sizeof(frame),"413-scene-commands-%s",languages[l]);screenshot(frame);click("Dalej");snprintf(frame,sizeof(frame),"414-scene-review-%s",languages[l]);screenshot(frame);click("Wstecz");click("Wstecz");click("Anuluj");click("Odrzuć");
 }
 CHECK(requests==execution_before);dsr_network_home(NULL);lv_refr_now(display);lv_mem_monitor_t before,after;lv_mem_monitor(&before);for(unsigned i=0;i<40;i++){dsr_ui_edit_action((uint8_t)(i%2),0);dsr_network_home(NULL);}lv_refr_now(display);lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=false;test_workspace_enable();dsr_ui_init();tick();
 cJSON *doc=room_document();cJSON *devices=cJSON_Parse("[{\"id\":1,\"name\":\"Relay A\",\"host\":\"192.0.2.1\",\"channel\":0,\"kind\":\"relay\",\"room\":0},{\"id\":2,\"name\":\"Relay B\",\"host\":\"192.0.2.2\",\"channel\":0,\"kind\":\"relay\",\"room\":0},{\"id\":3,\"name\":\"Roleta\",\"host\":\"192.0.2.3\",\"channel\":0,\"kind\":\"cover\",\"room\":0}]");CHECK(devices);cJSON_ReplaceItemInObjectCaseSensitive(doc,"devices",devices);publish_document(doc);tick();
 CHECK(dsr_ui_search_match("ŚWIATŁO ŁAZIENKA","światło"));CHECK(dsr_ui_search_match("KÜCHE","küche"));CHECK(dsr_ui_search_match("Küche","KÜCHE"));CHECK(dsr_ui_search_match("Przekaźnik","ŹNIK"));CHECK(dsr_ui_search_match("A",""));CHECK(!dsr_ui_search_match("A","B"));CHECK(!dsr_ui_search_match("A","AB"));
 contextual_actions_regression();more_editor_checks();
 printf("PASS: %u genuine LVGL scene/group editor checks; %u frames; explicit command, optional appearance, durable ACK/later edits, failure/discard, no SET; HTTP/storage MODEL\n",checks,screenshots);lv_deinit();return 0;
}
