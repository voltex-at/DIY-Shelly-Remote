void test_timer_boot_offset(uint32_t offset);
static void usage_regression(void)
{
 CHECK(!wifi.connected&&!in_flight&&!maintenance_ready);
 uint32_t a=review_entry_id("Relay A");
 /* "Zużycie" (owner 2026-10-18): Shelly-style gauge instead of line charts. */
 dsr_network_device_control(a);CHECK(!find_label(lv_screen_active(),"Zużycie"));dsr_ui_usage_open(a);CHECK(!dsr_mobile_active()&&find_label(lv_screen_active(),"Zużycie")&&find_label(lv_screen_active(),"Urządzenie offline • brak odczytu mocy")&&!find_type(lv_screen_active(),&lv_chart_class));screenshot("241-usage-offline");
 tap(40,65);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS);wifi.connected=true;tick();CHECK(in_flight);
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_voltage=true,.voltage_v=230,.has_current=true,.current_a=0,.has_energy=true,.energy_wh=1234}};response.device.online=response.device.has_switch=response.device.has_power=true;response.device.power_w=70;deliver(DSR_OK,DSR_WIFI_IDLE);
 if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 dsr_network_device_control(a);dsr_ui_usage_open(a);CHECK(find_label(lv_screen_active(),"70.0")&&find_label(lv_screen_active(),"W")&&find_label(lv_screen_active(),"0")&&find_label(lv_screen_active(),"500"));
 CHECK(find_label(lv_screen_active(),"Ostatnia aktualizacja: teraz")&&find_label(lv_screen_active(),"Licznik Shelly: 1.234 kWh")&&find_type(lv_screen_active(),&lv_line_class)&&find_type(lv_screen_active(),&lv_arc_class));
 lv_obj_t *heading=find_label(lv_screen_active(),"Relay A");CHECK(heading);lv_area_t head_area;lv_obj_get_coords(heading,&head_area);CHECK(head_area.y1>=0&&head_area.y2<=44);screenshot("242-usage-gauge");
 tap(267,65);CHECK(dsr_mobile_page()==DSR_MOBILE_DEVICE_SETTINGS&&find_label(lv_screen_active(),"Relay A"));tap(20,20);
 /* Exact selected ID persists through Web ordering and locale changes. */
 cJSON *original=room_document();for(unsigned language=1;language<=2;language++){cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==1?"en":"de"));publish_document(r);dsr_ui_usage_open(a);CHECK(find_label(lv_screen_active(),"Relay A"));screenshot(language==1?"244-usage-en":"245-usage-de");}
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(original,"revision"),dsr_workspace_revision());publish_document(original);
 /* Older than the 15 s freshness window: treated as offline, never a stale needle. */
 lv_tick_inc(16000);dsr_ui_usage_open(a);CHECK(find_label(lv_screen_active(),"Urządzenie offline • brak odczytu mocy")&&!find_type(lv_screen_active(),&lv_line_class));
 wifi.connected=false;tick();
 /* Energy history removed (0.7.51): no history job is ever submitted, even
  * after the boot offset that used to delay the history loader. */
 test_timer_boot_offset(120000);for(unsigned i=0;i<4;i++){lv_tick_inc(5001);tick();}test_timer_boot_offset(0);CHECK(!in_flight);
 /* The former Energie entry in the bottom bar opens battery diagnostics. */
 dsr_network_home(NULL);tap(160,455);CHECK(find_label(lv_screen_active(),"Bateria i zasilanie")&&find_label(lv_screen_active(),"Zmierz napięcie"));screenshot("246-battery-from-bar");
 /* 0.7.52 diagnostics: live heap/min/stack/BLE counters on the same page. */
 {lv_obj_t *title=find_label(lv_screen_active(),"Pamięć i radio");CHECK(title);lv_obj_t *card=lv_obj_get_parent(title);unsigned bars=0;for(unsigned i=0;i<lv_obj_get_child_count(card);i++)bars+=lv_obj_check_type(lv_obj_get_child(card,(int)i),&lv_bar_class);CHECK(bars==5);
  CHECK(find_label(card,"Wolne 136/320 KiB")&&find_label(card,"Min. 2304/8192 B")&&find_label(card,"wyłączone"));}
 click("Wstecz");CHECK(dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_HOME);
 lv_mem_monitor_t before,after;dsr_ui_usage_open(a);tap(40,65);lv_refr_now(display);lv_mem_monitor(&before);
 for(unsigned i=0;i<40;i++){dsr_ui_usage_open(a);tap(40,65);}lv_refr_now(display);lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);
 printf("Usage UI heap %u→%u;40 actual gauge open/back cycles; no energy history job\n",(unsigned)before.free_size,(unsigned)after.free_size);dsr_network_home(NULL);
}
