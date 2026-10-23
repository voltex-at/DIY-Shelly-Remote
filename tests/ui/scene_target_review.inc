/* Actual typed binding + scene draft/persistence; worker/radio/storage MODEL. */
static void scene_target_review(void)
{
 dsr_connection_t trv={.device={.host="192.168.1.70",.name="Thermostat Schlafzimmer",.device_id="shellyplus1pm-aabbccddeeff",.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_BLU,.channel=3}};
 wifi.connected=true;CHECK(dsr_network_bind_verified(&trv,0)==DSR_OK);wifi.connected=false;
 response=(dsr_event_t){.device=trv.device,.telemetry={.has_blu_battery=true,.blu_battery_pct=82,.trv_target_c=21.5f,.trv_current_c=20.5f,.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET,.trv_pos=28}}};
 strcpy(response.device.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a");response.device.online=true;deliver(DSR_OK,DSR_WIFI_IDLE);tick();CHECK(!dsr_network_busy());
 cJSON *doc=room_document();uint32_t id=0;const cJSON *d;cJSON_ArrayForEach(d,cJSON_GetObjectItemCaseSensitive(doc,"devices"))if(!strcmp(cJSON_GetObjectItemCaseSensitive(d,"name")->valuestring,trv.device.name))id=(uint32_t)cJSON_GetObjectItemCaseSensitive(d,"id")->valueint;cJSON_Delete(doc);
 dsr_mobile_device_t actual;CHECK(id&&dsr_network_typed_device(id,&actual)&&dsr_mobile_control_family(&actual)==DSR_ENDPOINT_BLU);
 unsigned writes=job_counts[DSR_JOB_TRV];const unsigned skins[]={0,2,3};const char *locales[]={"pl","en","de"};
 for(unsigned language=0;language<3;language++)for(unsigned skin=0;skin<3;skin++){
  dsr_ui_workspace_hide();doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(skins[skin]==3?"cyber_blue":skins[skin]==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(locales[language]));cJSON_ReplaceItemInObjectCaseSensitive(doc,"scenes",cJSON_CreateArray());publish_document(doc);dsr_network_home(NULL);tick();
  dsr_ui_edit_action(1,0);action_editor_name("Abendtemperatur");review_press("Dalej");review_press("+ Dodaj polecenie");review_select(trv.device.name,true);
  CHECK(find_label(lv_screen_active(),"21.5 °C"));review_press("+");CHECK(find_label(lv_screen_active(),"22.0 °C"));
  lv_obj_update_layout(lv_screen_active());lv_area_t change;lv_obj_get_coords(lv_obj_get_parent(find_label(lv_screen_active(),"Zmień urządzenie")),&change);CHECK(change.y2<398&&change.y2-change.y1+1>=44);
  if(language==2){char path[80];snprintf(path,sizeof(path),"scene-target-pick-skin%u",skins[skin]);screenshot(path);}
  review_press("Ustaw temperaturę");CHECK(find_label(lv_screen_active(),"22.0 °C"));review_press("Dalej");
  if(language==2){char path[80];snprintf(path,sizeof(path),"scene-target-summary-skin%u",skins[skin]);screenshot(path);}
  test_workspace_defer(true);review_press("Zapisz scenę");test_workspace_finish(false);dsr_ui_workspace_tick();doc=room_document();CHECK(!cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"scenes")));cJSON_Delete(doc);
  review_press("Zapisz scenę");test_workspace_finish(true);dsr_ui_workspace_tick();test_workspace_defer(false);
  doc=room_document();const cJSON *scene=cJSON_GetObjectItemCaseSensitive(doc,"scenes")->child,*a=cJSON_GetObjectItemCaseSensitive(scene,"actions")->child;CHECK(a&&cJSON_GetObjectItemCaseSensitive(a,"device")->valueint==(int)id&&!strcmp(cJSON_GetObjectItemCaseSensitive(a,"command")->valuestring,"target")&&cJSON_GetObjectItemCaseSensitive(a,"value")->valueint==44);unsigned scene_id=(unsigned)cJSON_GetObjectItemCaseSensitive(scene,"id")->valueint;cJSON_Delete(doc);
  dsr_ui_edit_action(1,scene_id);review_press("Dalej");review_press("22.0 °C");CHECK(find_label(lv_screen_active(),"22.0 °C"));review_press("-");CHECK(find_label(lv_screen_active(),"21.5 °C"));review_press("Wstecz");dsr_ui_workspace_hide();
  CHECK(job_counts[DSR_JOB_TRV]==writes); /* Editing a scene never controls the TRV. */
 }
 /* The older settings registry uses the same target selector without losing
  * its wider draft; changing device must remove the old target payload. */
 dsr_ui_config_registry(NULL);review_press("Sceny");review_press("Abendtemperatur");action_editor_name("Registry draft");
 char label[64];snprintf(label,sizeof(label),"%s 22.0 °C",dsr_sc_tr("Nastawa"));review_press(label);review_press("+");review_press("Wstecz");CHECK(find_label(lv_screen_active(),label));
 review_press(label);review_press("+");review_press("Ustaw temperaturę");review_press("Zapisz");dsr_ui_workspace_tick();
 doc=room_document();const cJSON *scene=cJSON_GetObjectItemCaseSensitive(doc,"scenes")->child,*a=cJSON_GetObjectItemCaseSensitive(scene,"actions")->child;CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(scene,"name")->valuestring,"Registry draft")&&cJSON_GetObjectItemCaseSensitive(a,"value")->valueint==45);cJSON_Delete(doc);
 review_press("Registry draft");review_press(trv.device.name);review_press("Zapisz");dsr_ui_workspace_tick();doc=room_document();a=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"scenes")->child,"actions")->child;CHECK(!cJSON_GetObjectItemCaseSensitive(a,"value")&&!strcmp(cJSON_GetObjectItemCaseSensitive(a,"command")->valuestring,"on"));cJSON_Delete(doc);
 CHECK(job_counts[DSR_JOB_TRV]==writes);
 printf("PASS: focused scene TRV editor; %u checks, %u selected frames; typed device, 3 skins/PL/EN/DE, saved target, failure/retry, registry draft/edit/cancel without radio writes\n",checks,screenshots);
}
