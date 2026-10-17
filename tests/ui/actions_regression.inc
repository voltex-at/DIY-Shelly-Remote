/* Actual LCD + actual actor/model/definition/API. Worker, radio and workspace
 * are explicit providers; no device execution or Flash claim. */
static void actions_regression(void)
{
 CHECK(!in_flight&&!maintenance_ready);cJSON *original=room_document();uint32_t a=review_entry_id("Relay A"),b=review_entry_id("Relay B");CHECK(a&&b&&a!=b);
 cJSON *r=room_document(),*groups=cJSON_GetObjectItemCaseSensitive(r,"groups"),*scenes=cJSON_GetObjectItemCaseSensitive(r,"scenes");CHECK(cJSON_IsArray(groups));cJSON *group=cJSON_CreateObject();cJSON_AddNumberToObject(group,"id",91);cJSON_AddStringToObject(group,"name","All fixtures");cJSON *members=cJSON_AddArrayToObject(group,"members");cJSON_AddItemToArray(members,cJSON_CreateNumber(a));cJSON_AddItemToArray(members,cJSON_CreateNumber(b));cJSON_AddItemToArray(groups,group);
 cJSON *scene=cJSON_CreateObject();cJSON_AddNumberToObject(scene,"id",92);cJSON_AddStringToObject(scene,"name","Night fixture");cJSON *steps=cJSON_AddArrayToObject(scene,"actions");const char *commands[]={"on","off"};for(unsigned i=0;i<2;i++){cJSON *step=cJSON_CreateObject();cJSON_AddNumberToObject(step,"device",a);cJSON_AddStringToObject(step,"command",commands[i]);cJSON_AddItemToArray(steps,step);}cJSON_AddItemToArray(scenes,scene);
 cJSON *dashboard=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(r,"dashboards"),0),*tiles=cJSON_GetObjectItemCaseSensitive(dashboard,"tiles");cJSON *tile=cJSON_Parse("{\"id\":991,\"type\":\"scene\",\"ref\":92,\"span\":2,\"height\":1,\"fields\":255}");cJSON_InsertItemInArray(tiles,0,tile);publish_document(r);tick();
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
 dsr_ui_actions_open(2);CHECK(find_label(lv_screen_active(),"Wyczyść dziennik RAM"));screenshot("254-activity-utc-lcd");click("Wyczyść dziennik RAM");dsr_action_log_page_t *log=calloc(1,sizeof(*log));CHECK(log&&dsr_actions_log(0,0,32,log)&&!log->count);free(log);CHECK(find_label(lv_screen_active(),"Brak aktywności w tym uruchomieniu"));
 /* A transport-only Cover observation never renders confirmed Switch OFF. */
 dsr_connection_t observed={.layout_id=b,.device={.generation=2,.endpoint_kind=DSR_ENDPOINT_COVER,.device_id="shelly-cover-observation",.host="192.0.2.43",.online=true}};
 CHECK(dsr_actions_publish_begin(dsr_workspace_revision(),true));dsr_actions_publish_at(&observed,true,false,DSR_OK,true);dsr_actions_publish_end();dsr_actions_log_clear();observed.device.online=false;
 CHECK(dsr_actions_publish_begin(dsr_workspace_revision(),true));dsr_actions_publish_at(&observed,true,false,DSR_OK,false);dsr_actions_publish_end();dsr_ui_actions_open(2);
 CHECK(find_label(lv_screen_active(),"Zmiana stanu • Brak odpowiedzi"));CHECK(!find_label(lv_screen_active(),"Wyłącz • Potwierdzono"));screenshot("257-cover-availability-activity");tick();
 dsr_ui_actions_open(0);tap(20,20);lv_refr_now(display);lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned i=0;i<40;i++){dsr_ui_actions_open((uint8_t)(i%3));tap(20,20);}lv_refr_now(display);lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);
 for(unsigned locale=1;locale<=2;locale++){cJSON *localized=room_document();cJSON_ReplaceItemInObjectCaseSensitive(localized,"language",cJSON_CreateString(locale==1?"en":"de"));publish_document(localized);dsr_ui_actions_open(0);CHECK(dsr_sc_language()==locale);lv_obj_t *label=find_label(lv_screen_active(),"Włącz grupę");CHECK(label&&!strcmp(lv_label_get_text(label),locale==1?"Turn group on":"Gruppe einschalten"));screenshot(locale==1?"255-groups-english":"256-groups-german");tap(20,20);}cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(original,"revision"),dsr_workspace_revision());publish_document(original);wifi.connected=false;tick();
 /* Empty groups/scenes give a concrete next step and a working Web Panel shortcut (PL/EN/DE). */
 {cJSON *saved=room_document();cJSON *empty=room_document();cJSON_ReplaceItemInObjectCaseSensitive(empty,"groups",cJSON_CreateArray());cJSON_ReplaceItemInObjectCaseSensitive(empty,"scenes",cJSON_CreateArray());publish_document(empty);
  dsr_ui_actions_open(0);CHECK(find_label(lv_screen_active(),"Brak grup. Utwórz grupę w Panelu WWW, w sekcji Grupy (cały dom).")&&find_label(lv_screen_active(),"Otwórz Panel WWW"));CHECK(!find_label(lv_screen_active(),"Włącz grupę"));screenshot("258-groups-empty-guided");
  dsr_ui_actions_open(1);CHECK(find_label(lv_screen_active(),"Brak scen. Utwórz scenę w Panelu WWW, w sekcji Sceny (cały dom).")&&find_label(lv_screen_active(),"Otwórz Panel WWW"));screenshot("259-scenes-empty-guided");
  click("Otwórz Panel WWW");CHECK(find_label(lv_screen_active(),"Panel WWW / bezpieczeństwo"));
  tick();tick();CHECK(find_label(lv_screen_active(),"Panel WWW / bezpieczeństwo")&&!find_label(lv_screen_active(),"Otwórz Panel WWW")); /* the actions page no longer redraws over it */
  for(unsigned locale=1;locale<=2;locale++){cJSON *localized=room_document();cJSON_ReplaceItemInObjectCaseSensitive(localized,"language",cJSON_CreateString(locale==1?"en":"de"));publish_document(localized);dsr_ui_actions_open(0);CHECK(dsr_sc_language()==locale);
   lv_obj_t *label=find_label(lv_screen_active(),"Otwórz Panel WWW");CHECK(label&&!strcmp(lv_label_get_text(label),locale==1?"Open Web Panel":"Web-Panel öffnen"));screenshot(locale==1?"260-groups-empty-english":"261-groups-empty-german");tap(20,20);}
  cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(saved,"revision"),dsr_workspace_revision());publish_document(saved);dsr_network_home(NULL);}
 printf("Actions LCD heap %u→%u;40 group/scene/activity cycles, cardRUN/cancel and per-endpoint offline/auth partial results; actual actor with worker/radio/workspace MODEL\n",(unsigned)before.free_size,(unsigned)after.free_size);
}
