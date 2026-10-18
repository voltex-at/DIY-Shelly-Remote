/* Actual LCD + actual actor/model/definition/API. Worker, radio and workspace
 * are explicit providers; no device execution or Flash claim. */
static void action_editor_select(const char *name,bool scene)
{
 lv_obj_t *search=find_type(lv_screen_active(),&lv_textarea_class);CHECK(search);lv_textarea_set_text(search,name);
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb);lv_obj_send_event(kb,LV_EVENT_READY,NULL);
 /* Search expands the matching room; settle layout before pointer hit testing. */
 lv_obj_update_layout(lv_screen_active());lv_obj_t *target=NULL;
 if(scene){search=find_type(lv_screen_active(),&lv_textarea_class);CHECK(search);lv_obj_t *panel=lv_obj_get_parent(search);
  for(unsigned i=0;i<lv_obj_get_child_count(panel);i++){lv_obj_t *b=lv_obj_get_child(panel,(int)i),*label=lv_obj_get_child(b,0);if(lv_obj_check_type(b,&lv_obj_class)&&label&&lv_obj_check_type(label,&lv_label_class)&&!strcmp(lv_label_get_text(label),name))target=b;}
 }else target=find_type(lv_screen_active(),&lv_checkbox_class);
 CHECK(target);lv_area_t bounds;lv_obj_get_coords(target,&bounds);tap((bounds.x1+bounds.x2)/2,(bounds.y1+bounds.y2)/2);

}
static void action_editor_name(const char *value)
{
 lv_obj_t *name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name);lv_textarea_set_text(name,value);
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb);lv_obj_send_event(kb,LV_EVENT_READY,NULL);
}
static void contextual_actions_regression(void)
{
 cJSON *before=room_document();unsigned requests_before=requests;uint32_t revision=dsr_workspace_revision();
 dsr_ui_actions_open(0);click("+ Grupa");CHECK(find_label(lv_screen_active(),"Nazwa grupy"));screenshot("257-group-name");
 action_editor_name("Context group");screenshot("261-group-after-name");click("Pokój i zdjęcie (opcjonalnie)");CHECK(find_label(lv_screen_active(),"Zdjęcie (opcjonalnie)"));tap(44,344);click("Gotowe");click("Dalej");
 CHECK(find_label(lv_screen_active(),"Wybrano: 0"));screenshot("258-group-devices");action_editor_select("Relay A",false);CHECK(find_label(lv_screen_active(),"Wybrano: 1"));screenshot("259-group-selected");click("Dalej");CHECK(find_label(lv_screen_active(),"3/3 • Podsumowanie"));screenshot("260-group-summary");
 test_workspace_defer(true);click("Zapisz grupę");CHECK(dsr_workspace_revision()==revision);cJSON *old=room_document();CHECK(cJSON_Compare(old,before,true));cJSON_Delete(old);
 click("Zmień nazwę i opcje");action_editor_name("Later group draft");test_workspace_finish(true);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));CHECK(find_type(lv_screen_active(),&lv_textarea_class));
 test_workspace_defer(false);click("Dalej");click("Dalej");click("Zapisz grupę");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Later group draft")&&!find_label(lv_screen_active(),"Edytuj grupę"));screenshot("262-group-saved-card");
 cJSON *saved=room_document(),*group=cJSON_GetObjectItemCaseSensitive(saved,"groups")->child;while(group&&strcmp(cJSON_GetObjectItemCaseSensitive(group,"name")->valuestring,"Later group draft"))group=group->next;
 CHECK(group&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(group,"members"))==1);CHECK(cJSON_GetObjectItemCaseSensitive(group,"photo")&&strlen(cJSON_GetObjectItemCaseSensitive(group,"photo")->valuestring)==32);uint32_t id=(uint32_t)cJSON_GetObjectItemCaseSensitive(group,"id")->valueint;cJSON_Delete(saved);
 dsr_ui_edit_action(0,id);action_editor_name("Must survive failure");click("Dalej");click("Dalej");test_workspace_defer(true);click("Zapisz grupę");test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));test_workspace_defer(false);
 click("Zmień nazwę i opcje");lv_obj_t *name=find_type(lv_screen_active(),&lv_textarea_class);CHECK(name&&!strcmp(lv_textarea_get_text(name),"Must survive failure"));click("Anuluj");CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));click("Odrzuć");CHECK(find_label(lv_screen_active(),"Later group draft"));
 dsr_ui_actions_open(1);click("+ Scena");CHECK(find_label(lv_screen_active(),"Nazwa sceny"));action_editor_name("Context scene");click("Dalej");CHECK(find_label(lv_screen_active(),"Co ma zrobić scena?"));click("+ Dodaj polecenie");action_editor_select("Relay A",true);CHECK(find_label(lv_screen_active(),"Wybierz polecenie"));click("Wstecz");CHECK(find_label(lv_screen_active(),"Wybierz urządzenie"));action_editor_select("Relay A",true);click("Wyłącz");CHECK(find_label(lv_screen_active(),"1. Relay A"));screenshot("263-scene-actions");click("Dalej");CHECK(find_label(lv_screen_active(),"Wyłącz"));click("Zapisz scenę");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Context scene"));screenshot("264-scene-saved-card");
 saved=room_document();cJSON *scene=cJSON_GetObjectItemCaseSensitive(saved,"scenes")->child;while(scene&&strcmp(cJSON_GetObjectItemCaseSensitive(scene,"name")->valuestring,"Context scene"))scene=scene->next;CHECK(scene);CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(scene,"actions")->child,"command")->valuestring,"off"));CHECK(!*cJSON_GetObjectItemCaseSensitive(scene,"photo")->valuestring);cJSON_Delete(saved);CHECK(requests==requests_before);
 for(unsigned language=1;language<=2;language++){
  cJSON *localized=room_document();cJSON_ReplaceItemInObjectCaseSensitive(localized,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(localized);dsr_ui_actions_open(0);dsr_ui_edit_action(0,id);CHECK(dsr_sc_language()==language);screenshot(language==1?"401-group-wizard-en":"402-group-wizard-de");click("Pokój i zdjęcie (opcjonalnie)");CHECK(find_label(lv_screen_active(),"Zdjęcie (opcjonalnie)"));screenshot(language==1?"403-group-appearance-en":"404-group-appearance-de");dsr_network_home(NULL);
 }
 CHECK(requests==requests_before);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(before,"revision"),dsr_workspace_revision());publish_document(before);tick();dsr_network_home(NULL);
}
static void actions_regression(void)
{
 CHECK(!in_flight&&!maintenance_ready);cJSON *original=room_document();uint32_t a=review_entry_id("Relay A"),b=review_entry_id("Relay B");CHECK(a&&b&&a!=b);
 cJSON *r=room_document(),*groups=cJSON_GetObjectItemCaseSensitive(r,"groups"),*scenes=cJSON_GetObjectItemCaseSensitive(r,"scenes");CHECK(cJSON_IsArray(groups));cJSON *group=cJSON_CreateObject();cJSON_AddNumberToObject(group,"id",91);cJSON_AddStringToObject(group,"name","All fixtures");cJSON *members=cJSON_AddArrayToObject(group,"members");cJSON_AddItemToArray(members,cJSON_CreateNumber(a));cJSON_AddItemToArray(members,cJSON_CreateNumber(b));cJSON_AddItemToArray(groups,group);
 cJSON *scene=cJSON_CreateObject();cJSON_AddNumberToObject(scene,"id",92);cJSON_AddStringToObject(scene,"name","Night fixture");cJSON *steps=cJSON_AddArrayToObject(scene,"actions");const char *commands[]={"on","off"};for(unsigned i=0;i<2;i++){cJSON *step=cJSON_CreateObject();cJSON_AddNumberToObject(step,"device",a);cJSON_AddStringToObject(step,"command",commands[i]);cJSON_AddItemToArray(steps,step);}cJSON_AddItemToArray(scenes,scene);
 cJSON *dashboard=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(r,"dashboards"),0),*tiles=cJSON_GetObjectItemCaseSensitive(dashboard,"tiles");cJSON *tile=cJSON_Parse("{\"id\":991,\"type\":\"scene\",\"ref\":92,\"span\":2,\"height\":1,\"fields\":255}");cJSON_InsertItemInArray(tiles,0,tile);publish_document(r);tick();
 contextual_actions_regression();
 unsigned sets=job_counts[DSR_JOB_SET_OUTPUT];dsr_ui_actions_open(0);CHECK(find_label(lv_screen_active(),"All fixtures")&&find_label(lv_screen_active(),"Cały dom • każde urządzenie ma własny wynik."));screenshot("251-groups-lcd");CHECK(job_counts[DSR_JOB_SET_OUTPUT]==sets);
 /* Offline group reports a result per endpoint without any optimistic SET. */
 CHECK(!wifi.connected);click("Włącz grupę");for(unsigned i=0;i<4&&dsr_actions_active();i++)tick();dsr_actions_snapshot_t status;dsr_actions_snapshot(&status);CHECK(!status.active&&status.count==2&&status.steps[0].outcome==DSR_ACTION_FAILED&&status.steps[1].outcome==DSR_ACTION_FAILED);CHECK(job_counts[DSR_JOB_SET_OUTPUT]==sets);screenshot("252-group-offline-partial");
 dsr_network_home(NULL);wifi.connected=true;tick();
 /* Complete any ordinary poll first; actor preflight remains genuine and
  * exact selected layout ID owns each subsequent worker completion. */
 if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 /* Earlier targeted-refresh regression intentionally retained B's auth
  * error. Clear only B by its real selected-ID manual read. */
 while(in_flight){response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 dsr_network_device_settings(b);click("Odśwież status");CHECK(in_flight&&submitted.connection.layout_id==b);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 dsr_ui_actions_open(0);click("Wyłącz grupę");unsigned a_sets=0,b_sets=0;
 for(unsigned i=0;i<16&&dsr_actions_active();i++){
  if(!in_flight){tick();continue;}
  bool owned=dsr_actions_job_owned(submitted.id);uint32_t target=submitted.connection.layout_id;
  if(owned&&submitted.kind==DSR_JOB_SET_OUTPUT){if(target==a)a_sets++;if(target==b)b_sets++;}
  response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;response.device.output=submitted.on;
  deliver(owned&&target==a?DSR_AUTH_REQUIRED:DSR_OK,DSR_WIFI_IDLE);
 }
 dsr_actions_snapshot(&status);CHECK(!status.active&&status.steps[0].outcome==DSR_ACTION_FAILED&&status.steps[0].result==DSR_AUTH_REQUIRED&&status.steps[1].outcome==DSR_ACTION_SUCCESS);CHECK(a_sets<=1&&b_sets==1);screenshot("253-group-auth-partial");
 /* Scene card RUN is separate from its editor. Cancellation of an admitted
  * queue retains ownership until the exact completion; no SET replay. */
 dsr_network_home(NULL);CHECK(find_label(lv_screen_active(),"Night fixture")&&find_label(lv_screen_active(),"Uruchom scenę"));sets=job_counts[DSR_JOB_SET_OUTPUT];click("Uruchom scenę");dsr_actions_snapshot(&status);CHECK(status.object_id==92&&status.kind==1);if(status.active)click("Anuluj pozostałe kroki");if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OK,DSR_WIFI_IDLE);}tick();CHECK(!dsr_actions_active()&&job_counts[DSR_JOB_SET_OUTPUT]<=sets+1);
 /* LCD "Aktywność" removed (owner, 0.7.51): kind 2 now shows groups, no log page. */
 dsr_ui_actions_open(2);CHECK(find_label(lv_screen_active(),"Grupy")&&find_label(lv_screen_active(),"Sceny")&&!find_label(lv_screen_active(),"Aktywność")&&!find_label(lv_screen_active(),"Wyczyść dziennik RAM"));screenshot("254-groups-scenes-only");
 dsr_actions_log_clear();dsr_action_log_page_t *log=calloc(1,sizeof(*log));CHECK(log&&dsr_actions_log(0,0,32,log)&&!log->count);free(log);
 /* A transport-only Cover observation never renders confirmed Switch OFF. */
 dsr_connection_t observed={.layout_id=b,.device={.generation=2,.endpoint_kind=DSR_ENDPOINT_COVER,.device_id="shelly-cover-observation",.host="192.0.2.43",.online=true}};
 CHECK(dsr_actions_publish_begin(dsr_workspace_revision(),true));dsr_actions_publish_at(&observed,true,false,DSR_OK,true);dsr_actions_publish_end();dsr_actions_log_clear();observed.device.online=false;
 CHECK(dsr_actions_publish_begin(dsr_workspace_revision(),true));dsr_actions_publish_at(&observed,true,false,DSR_OK,false);dsr_actions_publish_end();
 log=calloc(1,sizeof(*log));CHECK(log&&dsr_actions_log(0,0,32,log)&&log->count==1&&log->entries[0].kind==2&&log->entries[0].command!=DSR_ACTION_OFF&&log->entries[0].result!=DSR_OK);free(log);tick();
 dsr_ui_actions_open(0);tap(20,20);lv_refr_now(display);lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned i=0;i<40;i++){dsr_ui_actions_open((uint8_t)(i%3));tap(20,20);}lv_refr_now(display);lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);
 for(unsigned locale=1;locale<=2;locale++){cJSON *localized=room_document();cJSON_ReplaceItemInObjectCaseSensitive(localized,"language",cJSON_CreateString(locale==1?"en":"de"));publish_document(localized);dsr_ui_actions_open(0);CHECK(dsr_sc_language()==locale);lv_obj_t *label=find_label(lv_screen_active(),"Włącz grupę");CHECK(label&&!strcmp(lv_label_get_text(label),locale==1?"Turn group on":"Gruppe einschalten"));screenshot(locale==1?"255-groups-english":"256-groups-german");tap(20,20);}cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(original,"revision"),dsr_workspace_revision());publish_document(original);wifi.connected=false;tick();
 printf("Actions LCD heap %u→%u;40 group/scene cycles, cardRUN/cancel and per-endpoint offline/auth partial results; actual actor with worker/radio/workspace MODEL\n",(unsigned)before.free_size,(unsigned)after.free_size);
}
