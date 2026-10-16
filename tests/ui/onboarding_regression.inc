/* Production modal/shell on genuine LVGL; radios remain labelled providers. */
unsigned test_ble_scans(void);
unsigned test_lan_scans(void);
static lv_obj_t *method_button(const char *name)
{lv_obj_t *label=find_label(lv_screen_active(),name);CHECK(label);return lv_obj_get_parent(label);}
static void first_method_fully_visible(void)
{
 lv_obj_t *label=find_label(lv_screen_active(),"Bluetooth");CHECK(label);lv_obj_t *card=lv_obj_get_parent(label),*list=lv_obj_get_parent(card);
 lv_obj_update_layout(lv_screen_active());lv_area_t text,button,viewport;lv_obj_get_coords(label,&text);lv_obj_get_coords(card,&button);lv_obj_get_coords(list,&viewport);
 CHECK(lv_obj_get_scroll_y(list)==0);CHECK(button.y1==viewport.y1&&button.y2<=viewport.y2);
 CHECK(text.y1>=viewport.y1&&text.y2<=viewport.y2&&text.x1>=viewport.x1&&text.x2<=viewport.x2);
 CHECK(text.y1>=button.y1&&text.y2<=button.y2&&lv_obj_get_height(label)>=lv_font_get_line_height(&dsr_roboto_16));
}
static void pointer_drag(int x,int from,int to)
{
 touch=(lv_point_t){x,from};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);
 for(int i=1;i<=12;i++){touch.y=(lv_coord_t)(from+(to-from)*i/12);lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();}
 pressed=false;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
}
static void no_radio(unsigned ble,unsigned lan,unsigned ap,unsigned rpc)
{CHECK(test_ble_scans()==ble&&test_lan_scans()==lan&&job_counts[DSR_JOB_AP_SCAN]==ap&&test_rpc_starts()==rpc);}
static void expect_step(unsigned step)
{char text[40];snprintf(text,sizeof(text),dsr_sc_tr("Krok %u z 3"),step);CHECK(find_label(lv_screen_active(),text));}
static void clean_home(void)
{
 wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}dsr_network_home(NULL);
}
static void onboarding_regression(void)
{
 clean_home();cJSON *locale_config=room_document();cJSON_ReplaceItemInObjectCaseSensitive(locale_config,"language",cJSON_CreateString("pl"));publish_document(locale_config);dsr_network_device(NULL);
 CHECK(dsr_ui_onboarding_active()&&!dsr_mobile_active());first_method_fully_visible();
 unsigned ble=test_ble_scans(),lan=test_lan_scans(),ap=job_counts[DSR_JOB_AP_SCAN],rpc=test_rpc_starts();
 lv_obj_t *forward=method_button("Dalej"),*back=method_button("Anuluj");CHECK(lv_obj_has_state(forward,LV_STATE_DISABLED));
 lv_area_t first,after;lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(forward,&first);CHECK(first.y1==420&&first.y2<480&&lv_obj_get_height(forward)>=44&&lv_obj_get_height(back)>=44);
 CHECK(lv_obj_has_state(method_button("Wi-Fi AP"),LV_STATE_DISABLED)&&lv_obj_has_state(method_button("Sieć lokalna"),LV_STATE_DISABLED));
 screenshot("300-onboarding-none-offline");tap(298,22);CHECK(dsr_ui_onboarding_active()); /* dim HOME cannot receive touch */
 tap(236,444);no_radio(ble,lan,ap,rpc); /* disabled Next is inert */
 tap(144,205);CHECK(!lv_obj_has_state(forward,LV_STATE_DISABLED));CHECK(lv_obj_get_style_border_width(method_button("Bluetooth"),0)==2);no_radio(ble,lan,ap,rpc);screenshot("301-onboarding-BLE-offline");
 tap(144,282);CHECK(lv_obj_get_style_border_width(method_button("Bluetooth"),0)==2);no_radio(ble,lan,ap,rpc); /* offline AP does not replace selection */
 lv_obj_t *list=lv_obj_get_parent(method_button("Bluetooth"));pointer_drag(154,374,194);CHECK(lv_obj_get_scroll_y(list)>0);lv_obj_get_coords(forward,&after);CHECK(!memcmp(&first,&after,sizeof(first)));screenshot("302-onboarding-advanced-scroll");
 lv_obj_t *manual=method_button("Dodaj ręcznie (IP)");lv_obj_get_coords(manual,&after);CHECK(after.y1>=168&&after.y2<408);tap((after.x1+after.x2)/2,(after.y1+after.y2)/2);no_radio(ble,lan,ap,rpc);
 tap(236,444);CHECK(find_type(lv_screen_active(),&lv_textarea_class)&&!dsr_ui_onboarding_active());expect_step(3);no_radio(ble,lan,ap,rpc);screenshot("303-onboarding-manual");click("Anuluj");click("Anuluj");
 /* No hidden request occurs during reconnect or changing a selected card. */
 dsr_network_device(NULL);wifi.connected=true;dsr_ui_onboarding_tick();click("Sieć lokalna");CHECK(lv_obj_get_style_border_width(method_button("Sieć lokalna"),0)==2);no_radio(ble,lan,ap,rpc);screenshot("304-onboarding-LAN-selected");
 click("Wi-Fi AP");no_radio(ble,lan,ap,rpc);screenshot("305-onboarding-AP-selected");if(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OK,DSR_WIFI_IDLE);}click("Dalej");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN&&job_counts[DSR_JOB_AP_SCAN]==ap+1);ap++;expect_step(2);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);screenshot("323-onboarding-empty-AP");click("Anuluj");click("Anuluj");
 dsr_network_device(NULL);choose_method("Bluetooth LE");expect_step(2);CHECK(test_ble_scans()==ble+1);expect_step(2);screenshot("306-onboarding-real-BLE-scan");
 for(unsigned i=0;i<10;i++){
  char name[30];snprintf(name,sizeof(name),i==2?"ShellyPro2Cover-TEST":"Shelly TEST %02u",i);uint8_t packet[31]={0};packet[0]=(uint8_t)(strlen(name)+1);packet[1]=9;memcpy(packet+2,name,strlen(name));test_ble_packet(packet,strlen(name)+2,i+1);
 }
 test_ble_finish(DSR_BLE_DONE);tick();dsr_ble_snapshot_t found;dsr_ble_snapshot(&found);CHECK(found.count==10);screenshot("307-onboarding-ten-advertisements");
 lv_obj_t *unknown=find_label(lv_screen_active(),"Reklama: ShellyPro2Cover-TEST");CHECK(unknown);lv_obj_scroll_to_view(lv_obj_get_parent(unknown),LV_ANIM_OFF);lv_obj_get_coords(lv_obj_get_parent(unknown),&after);tap((after.x1+after.x2)/2,(after.y1+after.y2)/2);
 CHECK(find_label(lv_screen_active(),"Nierozpoznane urządzenie")&&find_label(lv_screen_active(),"Reklama: ShellyPro2Cover-TEST")&&!find_label(lv_screen_active(),"Dodaj przez BLE"));expect_step(2);CHECK(test_rpc_starts()==rpc);screenshot("308-onboarding-unrecognized-advertisement");
 lv_tick_inc(31000);tick();CHECK(test_rpc_starts()==rpc);screenshot("309-onboarding-stale-advertisement");click("Wstecz");click("Ponów");test_ble_finish(DSR_BLE_DONE);tick();screenshot("310-onboarding-empty-BLE");click("Wstecz");click("Anuluj");
 clean_home();wifi.connected=true;dsr_network_device(NULL);lan=test_lan_scans();choose_method("Skanuj sieć Wi-Fi");CHECK(test_lan_scans()==lan+1);expect_step(2);screenshot("311-onboarding-real-LAN-scan");test_lan_found("{\"gen\":2,\"id\":\"shelly-TEST-onboarding\",\"mac\":\"102030405060\",\"model\":\"SNSW-001X16EU\",\"auth_en\":false}","192.0.2.44");tick();CHECK(find_label(lv_screen_active(),"Zweryfikowano API"));screenshot("312-onboarding-verified-LAN");click("Wstecz");click("Anuluj");
 for(unsigned language=0;language<=2;language++){
  clean_home();wifi.connected=true;locale_config=room_document();cJSON_ReplaceItemInObjectCaseSensitive(locale_config,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));publish_document(locale_config);dsr_network_device(NULL);CHECK(dsr_sc_language()==language);CHECK(!strcmp(lv_label_get_text(find_label(lv_screen_active(),"Dodaj urządzenie")),language==0?"Dodaj urządzenie":language==1?"Add device":"Gerät hinzufügen"));first_method_fully_visible();pointer_drag(154,374,194);CHECK(lv_obj_get_scroll_y(lv_obj_get_parent(method_button("Bluetooth")))>0);click("Anuluj");dsr_network_device(NULL);first_method_fully_visible();
  screenshot(language==0?"324-onboarding-reopen-pl":language==1?"313-onboarding-none-en":"317-onboarding-none-de");click("Bluetooth");screenshot(language==0?"325-onboarding-reopen-BLE-pl":language==1?"314-onboarding-BLE-en":"318-onboarding-BLE-de");click("Sieć lokalna");screenshot(language==0?"326-onboarding-reopen-LAN-pl":language==1?"315-onboarding-LAN-en":"319-onboarding-LAN-de");click("Wi-Fi AP");screenshot(language==0?"327-onboarding-reopen-AP-pl":language==1?"316-onboarding-AP-en":"320-onboarding-AP-de");click("Anuluj");
 }
 locale_config=room_document();cJSON_ReplaceItemInObjectCaseSensitive(locale_config,"language",cJSON_CreateString("pl"));publish_document(locale_config);clean_home();
 /* A worker-owned operation only disables admission; selection stays local,
  * and ordinary network ticks cannot replace the modal with HOME. */
 CHECK(dsr_network_request_ota(DSR_JOB_OTA_CHECK,DSR_OTA_STABLE,false)==DSR_OK);test_native_ota_set(DSR_NATIVE_CHECKING,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);dsr_network_device(NULL);click("Bluetooth");forward=method_button("Dalej");CHECK(lv_obj_has_state(forward,LV_STATE_DISABLED));lv_obj_t *modal=lv_obj_get_parent(lv_obj_get_parent(forward));ble=test_ble_scans();lan=test_lan_scans();ap=job_counts[DSR_JOB_AP_SCAN];rpc=test_rpc_starts();
 for(unsigned i=0;i<4;i++){wifi.connected=(i%2)==0;tick();CHECK(dsr_ui_onboarding_active()&&!dsr_mobile_active()&&lv_obj_get_parent(lv_obj_get_parent(forward))==modal);}tap(236,444);no_radio(ble,lan,ap,rpc);screenshot("321-onboarding-operation-busy");
 test_native_ota_set(DSR_NATIVE_IDLE,DSR_NATIVE_ERROR_NONE,DSR_MANIFEST_OK);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);tick();CHECK(!lv_obj_has_state(forward,LV_STATE_DISABLED));click("Dalej");test_ble_finish(DSR_BLE_UNAVAILABLE);tick();expect_step(2);CHECK(find_label(lv_screen_active(),"Bluetooth niedostępny"));screenshot("322-onboarding-scanner-unavailable");click("Wstecz");click("Anuluj");
 for(unsigned warm=0;warm<2;warm++){dsr_network_device(NULL);choose_method("Bluetooth LE");click("Anuluj");tick();click("Wstecz");click("Anuluj");}
 lv_mem_monitor_t before,after_memory;lv_mem_monitor(&before);ble=test_ble_scans();
 for(unsigned i=0;i<10;i++){dsr_network_device(NULL);choose_method("Bluetooth LE");click("Anuluj");tick();click("Wstecz");click("Anuluj");}
 lv_mem_monitor(&after_memory);CHECK(test_ble_scans()==ble+10&&!dsr_ui_ble_active()&&!dsr_ui_onboarding_active()&&after_memory.free_size==before.free_size);
 printf("Unified onboarding: actual pointer hit/drag/sticky CTA, zero transports on select, offline gates, ten unrecognized advertisements, PL/EN/DE; 10 rapid cycles heap%lu→%lu; providers MODEL\n",(unsigned long)before.free_size,(unsigned long)after_memory.free_size);
}
