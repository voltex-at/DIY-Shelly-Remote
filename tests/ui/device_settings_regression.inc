/* Adapted from PR34/693d869's actual LVGL Settings regression. This runs on
 * the current fleet and workspace/worker MODEL, never a physical Shelly. */
static cJSON *settings_entry(cJSON *root,uint32_t id)
{
 cJSON *d;cJSON_ArrayForEach(d,cJSON_GetObjectItemCaseSensitive(root,"devices"))
  if((uint32_t)cJSON_GetObjectItemCaseSensitive(d,"id")->valueint==id)return d;
 CHECK(!"Settings entry missing");return NULL;
}
static void settings_read(dsr_result_t result,bool metering)
{
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);
 response=(dsr_event_t){.device=submitted.connection.device};
 response.device.online=response.device.has_switch=result==DSR_OK;
 response.device.has_power=metering;response.device.power_w=6.8;
 response.telemetry=(dsr_telemetry_t){.has_voltage=metering,.voltage_v=229.1,.has_current=metering,.current_a=.030};
 deliver(result,DSR_WIFI_IDLE);
}
static void device_settings_regression(void)
{
 CHECK(!wifi.connected&&!in_flight&&!maintenance_ready);
 uint32_t a=review_entry_id("Relay A"),b=review_entry_id("Relay B");
 cJSON *original=room_document(),*r=room_document();
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"rooms"),cJSON_Parse("{\"id\":888,\"name\":\"Settings test room\",\"icon\":0,\"photo\":\"\"}"));publish_document(r);
 dsr_network_device_settings(a);CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS);
 CHECK(find_label(lv_screen_active(),"Wybrane urządzenie")&&find_label(lv_screen_active(),"shelly-A")&&find_label(lv_screen_active(),"Model nieodczytany"));
 CHECK(!find_label(lv_screen_active(),dsr_sc_tr("Historia 1 h / 24 h jest w zakładce Wykresy. Oś czasu UTC.")));
 screenshot("230-settings-bound-offline");
 /* Independent failures remain blocked. An explicit read of A never clears
  * B's auth block, and the in-flight read already satisfies a duplicate. */
 wifi.connected=true;tick();CHECK(in_flight);settings_read(DSR_AUTH_REQUIRED,false);
 if(in_flight)settings_read(DSR_AUTH_REQUIRED,false);
 CHECK(!in_flight);dsr_network_device_settings(a);unsigned before=requests;
 tap(80,376);CHECK(in_flight&&requests==before+1&&!strcmp(submitted.connection.device.device_id,"shelly-A"));
 click("Odśwież status");CHECK(requests==before+1);settings_read(DSR_OK,true);
 CHECK(!in_flight&&dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS&&find_label(lv_screen_active(),"6.8 W • 229.1 V • 0.030 A"));
 screenshot("231-settings-real-status-fixture");
 /* A successful transport completion carrying B's identity cannot replace
  * A's binding or clear B's independent auth block. The explicit retry must
  * still use A's original endpoint, never the mismatched event payload. */
 click("Odśwież status");CHECK(in_flight&&!strcmp(submitted.connection.device.device_id,"shelly-A"));
 response=(dsr_event_t){.device=submitted.connection.device};strcpy(response.device.device_id,"shelly-B");response.device.online=response.device.has_switch=true;response.device.output=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(find_label(lv_screen_active(),"Zmieniona tożsamość")&&find_label(lv_screen_active(),"shelly-A")&&!find_label(lv_screen_active(),"shelly-B"));
 dsr_network_device_settings(b);CHECK(find_label(lv_screen_active(),"Błąd logowania"));dsr_network_device_settings(a);click("Odśwież status");
 CHECK(in_flight&&!strcmp(submitted.connection.device.device_id,"shelly-A")&&!strcmp(submitted.connection.device.host,"192.168.1.60"));settings_read(DSR_OK,true);
 CHECK(!in_flight&&find_label(lv_screen_active(),"6.8 W • 229.1 V • 0.030 A"));
 dsr_network_device_settings(b);CHECK(find_label(lv_screen_active(),"Błąd logowania"));wifi.connected=false;tick();
 dsr_network_device_settings(a);before=requests;tap(236,376);
 CHECK(find_label(lv_screen_active(),"Edytuj kartę urządzenia")&&!dsr_mobile_active());
 unsigned skip=1;CHECK(!nth_type(lv_screen_active(),&lv_textarea_class,&skip));
 room_name("");click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Podaj nazwę urządzenia."));
 room_name("ąąąąąąąąąąąąąąąąąąąąąąąą");click("Zapisz");dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Podaj krótszą nazwę urządzenia."));
 room_name("Światło testowe");tap(100,158);lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb&&!lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));
 key(kb,"q");key(kb,LV_SYMBOL_BACKSPACE);screenshot("232-settings-card-keyboard");key(kb,"Gotowe");CHECK(lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN)&&requests==before);
 room_name("Światło kuchnia");lv_obj_t *room=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(room);lv_dropdown_set_selected(room,1);lv_obj_send_event(room,LV_EVENT_VALUE_CHANGED,NULL);
 test_workspace_defer(true);click("Zapisz");dsr_ui_workspace_tick();click("Wstecz");CHECK(find_label(lv_screen_active(),"Poczekaj na zakończenie zapisu."));
 test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));
 click("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();test_workspace_defer(false);
 CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS&&find_label(lv_screen_active(),"Światło kuchnia")&&requests==before);screenshot("233-settings-after-durable-ACK");
 r=room_document();cJSON *entry=settings_entry(r,a);CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(entry,"host")->valuestring,"192.168.1.60"));CHECK(cJSON_GetObjectItemCaseSensitive(entry,"channel")->valueint==0&&cJSON_GetObjectItemCaseSensitive(entry,"room")->valueint==888);cJSON_Delete(r);
 /* Save A, then edit B: the old ACK cannot navigate or discard the new draft. */
 tap(236,376);room_name("Potwierdzona A");test_workspace_defer(true);click("Zapisz");room_name("Niezapisana B");test_workspace_finish(true);dsr_ui_workspace_tick();
 CHECK(!dsr_mobile_active()&&find_label(lv_screen_active(),"Zapisano wcześniejszą wersję • są niezapisane zmiany"));
 click("Wstecz");CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));click("Zostań");CHECK(!dsr_mobile_active());click("Wstecz");click("Odrzuć");test_workspace_defer(false);
 CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS&&find_label(lv_screen_active(),"Potwierdzona A"));
 /* Web CAS conflict keeps the draft and the exact endpoint identity intact. */
 tap(236,376);room_name("Konflikt LCD");r=room_document();publish_document(r);dsr_ui_workspace_tick();
 CHECK(find_label(lv_screen_active(),"Zmieniono konfigurację. Wróć i otwórz edycję ponownie."));click("Zapisz");click("Wstecz");click("Odrzuć");
 /* Reordering cards while Settings is open cannot change its target. A new
  * card at A's host/channel is still unbound because A reserves its own ID. */
 r=room_document();cJSON *devices=cJSON_GetObjectItemCaseSensitive(r,"devices");entry=cJSON_DetachItemFromArray(devices,0);cJSON_AddItemToArray(devices,entry);
 entry=cJSON_Duplicate(settings_entry(r,a),true);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(entry,"id"),999);cJSON_ReplaceItemInObjectCaseSensitive(entry,"name",cJSON_CreateString("TEST unbound reused host"));cJSON_InsertItemInArray(devices,0,entry);publish_document(r);tick();
 CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS&&find_label(lv_screen_active(),"Potwierdzona A"));tap(236,376);CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Potwierdzona A"));click("Wstecz");tap(20,20);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS&&find_label(lv_screen_active(),"Potwierdzona A"));tap(267,65);
 dsr_network_device_settings(999);before=requests;tap(80,376);CHECK(requests==before&&find_label(lv_screen_active(),"Tożsamość niepotwierdzona")&&find_label(lv_screen_active(),"Brak aktywnego połączenia"));screenshot("234-settings-reused-host-unbound");
 /* Selected definition removal cannot fall through to the new list index. */
 review_remove_layout(999);tick();CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);dsr_network_device_settings(999);CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICES);
 dsr_network_device_settings(a);wifi.connected=true;tick();tap(80,376);CHECK(in_flight&&!strcmp(submitted.connection.device.device_id,"shelly-A"));settings_read(DSR_AUTH_REQUIRED,false);CHECK(find_label(lv_screen_active(),"Błąd logowania")&&!find_label(lv_screen_active(),"6.8 W • 229.1 V • 0.030 A"));screenshot("235-settings-auth-error");
 tap(80,376);settings_read(DSR_OK,false);CHECK(find_label(lv_screen_active(),"Brak obsługiwanych pomiarów"));wifi.connected=false;tick();
 /* A configured locator change disables the old binding; manual refresh does
  * not probe the foreign endpoint or release the private identity reservation. */
 const char *foreign_hosts[]={"192.168.1.199","192.168.1.51"};
 for(unsigned i=0;i<2;i++){
  /* Include B's live locator: matching host/channel alone must not hand A's
   * refresh to B, even before another normal scheduler/config tick. */
  r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(settings_entry(r,a),"host",cJSON_CreateString(foreign_hosts[i]));publish_document(r);before=requests;tap(80,376);CHECK(requests==before&&find_label(lv_screen_active(),"Brak aktywnego połączenia"));
 }
 r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(settings_entry(r,a),"host",cJSON_CreateString("192.168.1.60"));publish_document(r);tick();
 for(unsigned language=1;language<3;language++){
  r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(r);dsr_network_device_settings(a);
  CHECK(!find_label(lv_screen_active(),dsr_sc_tr("Historia 1 h / 24 h jest w zakładce Wykresy. Oś czasu UTC.")));
  screenshot(language==1?"236-settings-english":"238-settings-german");tap(236,376);CHECK(find_label(lv_screen_active(),language==1?"Room":"Raum"));screenshot(language==1?"237-settings-card-english":"239-settings-card-german");click("Wstecz");
 }
 r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString("pl"));publish_document(r);dsr_network_device_settings(a);
 /* A contextual editor return also retains the originating room's stable ID
  * when Web moves that room while Settings is open. Full-width valid names
  * remain bounded in the actual LCD header and editor. */
 dsr_network_room_context(888);click("Potwierdzona A");tap(267,65);
 r=room_document();cJSON_InsertItemInArray(cJSON_GetObjectItemCaseSensitive(r,"rooms"),0,cJSON_Parse("{\"id\":889,\"name\":\"Other settings room\",\"icon\":0,\"photo\":\"\"}"));publish_document(r);tick();
 tap(236,376);room_name("12345678901234567890123456789012345678901234567");click("Zapisz");dsr_ui_workspace_tick();CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS);screenshot("240-settings-long-name");
 tap(20,20);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);tap(20,20);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOM&&find_label(lv_screen_active(),"Settings test room"));dsr_network_device_settings(a);
 /* Existing transactional-open diagnostics handle storage/OOM without a draft. */
 for(unsigned fault=1;fault<=4;fault++){test_workspace_fault(fault);tap(236,376);CHECK(find_label(lv_screen_active(),"Konfiguracja niedostępna")&&!find_label(lv_screen_active(),"Zapisz"));test_workspace_fault(0);dsr_network_device_settings(a);}
 lv_obj_update_layout(lv_screen_active());lv_refr_now(display);lv_mem_monitor_t x,y;
 /* Warm-up: shared LVGL styles (0.7.50) are created once on first use. */
 for(unsigned i=0;i<4;i++){tap(236,376);click("Wstecz");}lv_refr_now(display);lv_mem_monitor(&x);
 /* No growth over 40 edit/back cycles. A transient allocation (timer-driven,
  * freed on a later tick) may coincide with the first sample, so the bound is
  * one-sided: any leak still lowers free memory and fails. */
 for(unsigned i=0;i<40;i++){tap(236,376);click("Wstecz");}lv_refr_now(display);lv_mem_monitor(&y);CHECK(y.free_size>=x.free_size);
 printf("Device Settings heap before=%u after=%u; model=%zu B;40 edit/back; targeted read and durable ACK; SDK/workspace MODEL\n",(unsigned)x.free_size,(unsigned)y.free_size,sizeof(dsr_mobile_model_t));
 /* Restore unchanged A/B fixture for the existing removal/migration corpus. */
 wifi.connected=false;tick();cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(original,"revision"),dsr_workspace_revision());publish_document(original);tick();dsr_network_home(NULL);
 CHECK(dsr_network_layout_reserved(a)&&dsr_network_layout_reserved(b)&&!in_flight&&!maintenance_ready);
}
