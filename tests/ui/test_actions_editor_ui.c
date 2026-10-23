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
static void review_shot(const char *state,unsigned theme,unsigned language)
{if(language==2){char name[80];snprintf(name,sizeof(name),"actions-%u-%s",theme,state);screenshot(name);}}
static unsigned survey_issues;
static void review_issue(bool good,const char *problem)
{if(!good){survey_issues++;fprintf(stderr,"Actions issue theme=%u language=%u: %s\n",dsr_sc_style(),dsr_sc_language(),problem);}}
static void review_press(const char *text)
{
 lv_obj_t *root=lv_screen_active();lv_obj_update_layout(root);for(unsigned i=lv_obj_get_child_count(root);i>0;i--){lv_obj_t *label=find_label(lv_obj_get_child(root,(int)i-1),text);if(!label)continue;lv_obj_t *button=lv_obj_get_parent(label);lv_obj_scroll_to_view(button,LV_ANIM_OFF);lv_obj_update_layout(root);lv_area_t a;lv_obj_get_coords(button,&a);CHECK(a.y1>=0&&a.y2<480);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);return;}fprintf(stderr,"Actions review: missing control '%s' theme=%u language=%u\n",text,dsr_sc_style(),dsr_sc_language());screenshot("actions-review-failure");CHECK(false);
}
static void review_select(const char *name,bool scene)
{
 lv_obj_t *search=find_type(lv_screen_active(),&lv_textarea_class);CHECK(search);lv_textarea_set_text(search,name);lv_obj_send_event(find_type(lv_screen_active(),&lv_keyboard_class),LV_EVENT_READY,NULL);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *target=NULL;if(!scene)target=find_type(lv_screen_active(),&lv_checkbox_class);
 else{search=find_type(lv_screen_active(),&lv_textarea_class);CHECK(search);lv_obj_t *panel=lv_obj_get_parent(search);unsigned count=0;for(unsigned i=0;i<lv_obj_get_child_count(panel);i++){lv_obj_t *b=lv_obj_get_child(panel,(int)i);if(lv_obj_check_type(b,&lv_obj_class)&&lv_obj_has_flag(b,LV_OBJ_FLAG_CLICKABLE)&&lv_obj_get_height(b)==48){target=b;count++;}}CHECK(count==1);review_issue(find_label(target,name)!=NULL,"device picker truncates the selected device name");}
 CHECK(target);lv_obj_scroll_to_view(target,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(target,&a);CHECK(a.y1>=138&&a.y2<398);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static void review_rejected_run(const char *caption,uint8_t kind,dsr_action_command_t first)
{
 unsigned writes=job_counts[DSR_JOB_SET_OUTPUT]+job_counts[DSR_JOB_COVER];tick();dsr_ui_actions_tick();review_press(caption);for(unsigned i=0;i<5&&dsr_actions_active();i++)tick();dsr_actions_snapshot_t result;dsr_actions_snapshot(&result);CHECK(!result.active&&result.kind==kind&&result.count==2&&result.steps[0].command==first&&result.steps[0].outcome==DSR_ACTION_FAILED&&result.steps[1].outcome==DSR_ACTION_FAILED);CHECK(job_counts[DSR_JOB_SET_OUTPUT]+job_counts[DSR_JOB_COVER]==writes);dsr_ui_actions_tick();
}
static void actions_survey_review(unsigned theme,unsigned language)
{
 dsr_ui_workspace_hide();dsr_ui_actions_hide();cJSON *doc=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"rooms",cJSON_Parse("[{\"id\":1,\"name\":\"Küche und Esszimmer\",\"icon\":0,\"photo\":\"\"}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Deckenbeleuchtung Küche und Esszimmer\",\"host\":\"192.0.2.1\",\"channel\":0,\"kind\":\"relay\",\"room\":1},{\"id\":2,\"name\":\"Steckdose Terrasse\",\"host\":\"192.0.2.2\",\"channel\":0,\"kind\":\"plug\",\"room\":1},{\"id\":3,\"name\":\"Rollladen Terrasse\",\"host\":\"192.0.2.3\",\"channel\":0,\"kind\":\"cover\",\"room\":1}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"dashboards",cJSON_Parse("[{\"id\":1,\"name\":\"Startbereich\",\"tiles\":[]}]"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"active_dashboard",cJSON_CreateNumber(1));cJSON_ReplaceItemInObjectCaseSensitive(doc,"groups",cJSON_CreateArray());cJSON_ReplaceItemInObjectCaseSensitive(doc,"scenes",cJSON_CreateArray());publish_document(doc);dsr_network_home(NULL);CHECK(dsr_sc_style()==theme&&dsr_sc_language()==language);
 unsigned writes=job_counts[DSR_JOB_SET_OUTPUT]+job_counts[DSR_JOB_COVER];dsr_ui_actions_open(0);review_press("+ Grupa");action_editor_name("Licht im Erdgeschoss");review_shot("group-name",theme,language);
 tap(32,22);bool guarded=find_label(lv_screen_active(),"Odrzucić zmiany?")!=NULL;review_issue(guarded,"header Back discards the first-step group draft");if(guarded){review_press("Zostań");lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_area_t viewport;lv_obj_get_coords(lv_obj_get_parent(field),&viewport);CHECK(viewport.y2<420);}else{dsr_ui_edit_action(0,0);action_editor_name("Licht im Erdgeschoss");}
 review_press("Pokój i zdjęcie (opcjonalnie)");lv_obj_t *room=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(room);lv_dropdown_set_selected(room,1);lv_obj_send_event(room,LV_EVENT_VALUE_CHANGED,NULL);review_shot("appearance",theme,language);review_press("Gotowe");review_press("Dalej");CHECK(!find_label(lv_screen_active(),"Rollladen Terrasse"));review_select("Deckenbeleuchtung Küche und Esszimmer",false);review_select("Steckdose Terrasse",false);
 lv_obj_t *search=find_type(lv_screen_active(),&lv_textarea_class);CHECK(search);lv_textarea_set_text(search,"");lv_obj_send_event(find_type(lv_screen_active(),&lv_keyboard_class),LV_EVENT_READY,NULL);char selected[40];snprintf(selected,sizeof(selected),dsr_sc_tr("Wybrano: %u"),2u);CHECK(find_label(lv_screen_active(),selected));lv_obj_t *selection=lv_obj_get_parent(find_type(lv_screen_active(),&lv_textarea_class));for(unsigned i=0;i<lv_obj_get_child_count(selection);i++){lv_obj_t *check=lv_obj_get_child(selection,(int)i);if(!lv_obj_check_type(check,&lv_checkbox_class))continue;CHECK(lv_obj_has_state(check,LV_STATE_CHECKED));CHECK(lv_color_eq(lv_obj_get_style_bg_color(check,LV_PART_INDICATOR),dsr_sc_color(DSR_SC_BLUE)));}review_shot("group-devices",theme,language);review_press("Dalej");review_shot("group-summary",theme,language);
 char *before=dsr_workspace_snapshot();CHECK(before);test_workspace_defer(true);review_press("Zapisz grupę");test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));review_shot("save-error",theme,language);char *after=dsr_workspace_snapshot();CHECK(after&&!strcmp(before,after));free(before);free(after);review_press("Zapisz grupę");test_workspace_finish(true);dsr_ui_workspace_tick();test_workspace_defer(false);CHECK(find_label(lv_screen_active(),"Licht im Erdgeschoss"));review_shot("groups",theme,language);
 doc=room_document();cJSON *group=cJSON_GetObjectItemCaseSensitive(doc,"groups")->child;CHECK(group&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(group,"members"))==2&&cJSON_GetObjectItemCaseSensitive(group,"room")->valueint==1);uint32_t id=(uint32_t)cJSON_GetObjectItemCaseSensitive(group,"id")->valueint;cJSON_Delete(doc);
 dsr_ui_edit_action(0,id);action_editor_name("Beleuchtung am Abend");review_press("Dalej");review_press("Dalej");review_press("Zapisz grupę");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Beleuchtung am Abend"));review_rejected_run("Włącz grupę",0,DSR_ACTION_ON);dsr_ui_actions_open(0);review_rejected_run("Wyłącz grupę",0,DSR_ACTION_OFF);
 dsr_ui_actions_open(1);review_press("+ Scena");action_editor_name("Abend auf der Terrasse");review_press("Dalej");review_press("+ Dodaj polecenie");review_select("Deckenbeleuchtung Küche und Esszimmer",true);review_press("Wyłącz");review_press("+ Dodaj polecenie");review_select("Rollladen Terrasse",true);review_press("Zamknij");review_shot("scene-steps",theme,language);review_press("Dalej");review_shot("scene-summary",theme,language);review_press("Zapisz scenę");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Abend auf der Terrasse"));review_shot("scenes",theme,language);
 doc=room_document();cJSON *scene=cJSON_GetObjectItemCaseSensitive(doc,"scenes")->child;CHECK(scene);id=(uint32_t)cJSON_GetObjectItemCaseSensitive(scene,"id")->valueint;cJSON *steps=cJSON_GetObjectItemCaseSensitive(scene,"actions");CHECK(cJSON_GetArraySize(steps)==2&&!strcmp(cJSON_GetObjectItemCaseSensitive(steps->child,"command")->valuestring,"off")&&!strcmp(cJSON_GetObjectItemCaseSensitive(steps->child->next,"command")->valuestring,"close"));cJSON_Delete(doc);CHECK(job_counts[DSR_JOB_SET_OUTPUT]+job_counts[DSR_JOB_COVER]==writes);
 dsr_ui_edit_action(1,id);review_press("Dalej");review_press("Wyłącz");review_press("Włącz");lv_obj_t *title=find_label(lv_screen_active(),"1. Deckenbeleuchtung Küche und Esszimmer");CHECK(title);lv_obj_t *panel=lv_obj_get_parent(title),*move=NULL;for(unsigned i=0;i<lv_obj_get_child_count(panel);i++){lv_obj_t *b=lv_obj_get_child(panel,(int)i);if(lv_obj_get_x(b)==170&&lv_obj_get_y(b)==48)move=b;}CHECK(move&&!lv_obj_has_state(move,LV_STATE_DISABLED));lv_obj_scroll_to_view(move,LV_ANIM_OFF);lv_obj_update_layout(panel);lv_area_t order;lv_obj_get_coords(move,&order);tap((order.x1+order.x2)/2,(order.y1+order.y2)/2);CHECK(find_label(lv_screen_active(),"1. Rollladen Terrasse"));review_shot("scene-edit",theme,language);review_press("Dalej");review_press("Zapisz scenę");dsr_ui_workspace_tick();
 doc=room_document();steps=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"scenes")->child,"actions");CHECK(cJSON_GetObjectItemCaseSensitive(steps->child,"device")->valueint==3&&!strcmp(cJSON_GetObjectItemCaseSensitive(steps->child->next,"command")->valuestring,"on"));cJSON_Delete(doc);
 review_rejected_run("Uruchom scenę",1,DSR_ACTION_CLOSE);CHECK(job_counts[DSR_JOB_SET_OUTPUT]+job_counts[DSR_JOB_COVER]==writes);review_shot("scene-unavailable",theme,language);dsr_network_home(NULL);
}
#include "group_controls_review.inc"
#include "group_trv_review.inc"
#include "group_details_review.inc"
#include "scene_target_review.inc"
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=false;test_workspace_enable();dsr_ui_init();tick();
 cJSON *doc=room_document();cJSON *devices=cJSON_Parse("[{\"id\":1,\"name\":\"Relay A\",\"host\":\"192.0.2.1\",\"channel\":0,\"kind\":\"relay\",\"room\":0},{\"id\":2,\"name\":\"Relay B\",\"host\":\"192.0.2.2\",\"channel\":0,\"kind\":\"relay\",\"room\":0},{\"id\":3,\"name\":\"Roleta\",\"host\":\"192.0.2.3\",\"channel\":0,\"kind\":\"cover\",\"room\":0}]");CHECK(devices);cJSON_ReplaceItemInObjectCaseSensitive(doc,"devices",devices);publish_document(doc);tick();
 CHECK(dsr_ui_search_match("ŚWIATŁO ŁAZIENKA","światło"));CHECK(dsr_ui_search_match("KÜCHE","küche"));CHECK(dsr_ui_search_match("Küche","KÜCHE"));CHECK(dsr_ui_search_match("Przekaźnik","ŹNIK"));CHECK(dsr_ui_search_match("A",""));CHECK(!dsr_ui_search_match("A","B"));CHECK(!dsr_ui_search_match("A","AB"));
 if(getenv("DSR_UI_FOCUS_SCENE_TARGET")){scene_target_review();lv_deinit();return 0;}
 if(getenv("DSR_UI_FOCUS_GROUP_DETAILS")){group_details_review();lv_deinit();return 0;}
 if(getenv("DSR_UI_FOCUS_GROUP_TRV")){group_trv_review();lv_deinit();return 0;}
 if(getenv("DSR_UI_FOCUS_GROUP_CONTROLS")){group_controls_review();lv_deinit();return 0;}
 if(getenv("DSR_UI_FOCUS_ACTION_REVIEW")){const unsigned themes[]={0,2,3};for(unsigned l=0;l<3;l++)for(unsigned t=0;t<3;t++)actions_survey_review(themes[t],(l+2)%3);CHECK(survey_issues==0);}
 else{contextual_actions_regression();more_editor_checks();}
 if(getenv("DSR_UI_FOCUS_ACTION_REVIEW"))printf("PASS: groups/scenes survey, %u checks, %u frames; create/edit/order, save failure/retry, guarded header Back, rejected unbound actions; host providers\n",checks,screenshots);
 else printf("PASS: %u genuine LVGL scene/group editor checks; %u frames; explicit command, optional appearance, durable ACK/later edits, failure/discard, no SET; HTTP/storage MODEL\n",checks,screenshots);
 lv_deinit();return 0;
}
