/* Genuine LCD controller, with separately labelled RPC/radio provider MODEL. */
unsigned test_rpc_starts(void);
unsigned test_rpc_confirms(void);
unsigned test_rpc_cancels(void);
void test_rpc_update(dsr_ble_rpc_phase_t phase,dsr_result_t result,bool active,const dsr_lan_device_t *identity);
static const dsr_ble_device_t rpc_candidate={.name="Own Shelly RPC",.address={1,2,3,4,5,6},.kind=DSR_BLE_SHELLY_RPC,.evidence=DSR_BLE_RPC};
static const dsr_lan_device_t rpc_identity={.id="shellyplus1-112233445566",.mac="112233445566",.model="SNSW-001X16EU",.generation=2};
static void rpc_intro(void)
{
 wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 wifi.connected=true;dsr_ui_ble_rpc_open(&rpc_candidate);CHECK(dsr_ui_ble_rpc_active()&&dsr_ble_rpc_sensitive());
 unsigned before=requests;CHECK(dsr_network_request_ota(DSR_JOB_OTA_CHECK,0,false)==DSR_BUSY&&requests==before);
}
static void rpc_disclose(void)
{
 click("Sprawdź BLE");CHECK(dsr_ble_rpc_active()&&!dsr_activity_restart_reserve());
 test_rpc_update(DSR_BLE_RPC_CONFIRM,DSR_OK,true,&rpc_identity);dsr_ui_ble_rpc_tick();
 CHECK(find_label(lv_screen_active(),"Przekazać dane Wi-Fi?")&&find_label(lv_screen_active(),rpc_identity.id));
}
static void rpc_finish_cancel(void)
{
 click("Anuluj");test_rpc_update(DSR_BLE_RPC_CANCELLED,DSR_BUSY,false,NULL);dsr_ui_ble_rpc_tick();
 CHECK(!dsr_ui_ble_rpc_active()&&!dsr_ble_rpc_sensitive());
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
}
static void rpc_regression(void)
{
 dsr_sc_appearance(DSR_SC_BLUE,false,0);
 wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}wifi.connected=true;
 unsigned before_start=test_rpc_starts();CHECK(dsr_network_request_ota(DSR_JOB_OTA_CHECK,0,false)==DSR_OK&&in_flight&&submitted.kind==DSR_JOB_OTA_CHECK);
 dsr_ui_ble_rpc_open(&rpc_candidate);click("Sprawdź BLE");CHECK(test_rpc_starts()==before_start&&find_label(lv_screen_active(),"Operacja w toku. Poczekaj."));click("Anuluj");test_native_ota_set(DSR_NATIVE_FAILED,DSR_NATIVE_ERROR_OFFLINE,DSR_MANIFEST_OK);response=(dsr_event_t){0};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);
 unsigned starts=test_rpc_starts(),confirms=test_rpc_confirms();rpc_intro();CHECK(test_rpc_starts()==starts);screenshot("240-ble-first-consent-pl");
 rpc_disclose();CHECK(test_rpc_starts()==starts+1&&test_rpc_confirms()==confirms);screenshot("241-ble-second-consent-pl");rpc_finish_cancel();CHECK(test_rpc_confirms()==confirms);
 /* Hide/reopen cannot replace the candidate, acquire a second lease or display
  * a delayed disclosure from the cancelled session. */
 rpc_intro();click("Sprawdź BLE");unsigned cancelled=test_rpc_cancels();dsr_network_home(NULL);dsr_ui_ble_rpc_open(&rpc_candidate);CHECK(test_rpc_cancels()>cancelled);
 test_rpc_update(DSR_BLE_RPC_CONFIRM,DSR_OK,true,&rpc_identity);dsr_ui_ble_rpc_tick();CHECK(!find_label(lv_screen_active(),"Przekazać dane Wi-Fi?"));CHECK(!dsr_activity_restart_reserve());
 test_rpc_update(DSR_BLE_RPC_CANCELLED,DSR_BUSY,false,NULL);dsr_ui_ble_rpc_tick();CHECK(!dsr_ui_ble_rpc_active());
 /* Only second physical consent calls the transport admission. An uncertain
  * write proceeds to read-only LAN verification, never a second Wi-Fi write. */
 rpc_intro();rpc_disclose();click("Przekaż Wi-Fi");CHECK(test_rpc_confirms()==confirms+1);test_rpc_update(DSR_BLE_RPC_LAN_READY,DSR_OFFLINE,false,&rpc_identity);dsr_ui_ble_rpc_tick();
 CHECK(find_label(lv_screen_active(),"Sprawdź w domowej sieci"));
 test_lan_found("{\"gen\":2,\"id\":\"shellyplus1-112233445566\",\"mac\":\"112233445566\",\"model\":\"SNSW-001X16EU\",\"auth_en\":false}","192.168.1.94");
 next_submit_result=DSR_BUSY;dsr_ui_ble_rpc_tick();CHECK(!in_flight);dsr_ui_ble_rpc_tick();CHECK(in_flight&&submitted.kind==DSR_JOB_LAN_STA_VERIFY&&!strcmp(submitted.ap_identity.id,rpc_identity.id)&&!submitted.ap_identity.ssid[0]);
 response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false}}");dsr_ui_ble_rpc_tick();
 CHECK(find_label(lv_screen_active(),"Nazwa i pomieszczenie")&&!dsr_ui_ble_rpc_active()&&!dsr_ble_rpc_sensitive());CHECK(test_rpc_confirms()==confirms+1);screenshot("242-ble-verified-LAN-bind");
 wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 /* Delayed HTTP verification completion after hide has no access to a deleted
  * screen and cannot adopt or bind. The outer activity lives until its ACK. */
 rpc_intro();rpc_disclose();click("Przekaż Wi-Fi");test_rpc_update(DSR_BLE_RPC_LAN_READY,DSR_OK,false,&rpc_identity);dsr_ui_ble_rpc_tick();
 test_lan_found("{\"gen\":2,\"id\":\"shellyplus1-112233445566\",\"mac\":\"112233445566\",\"model\":\"SNSW-001X16EU\",\"auth_en\":false}","192.168.1.94");dsr_ui_ble_rpc_tick();CHECK(in_flight&&submitted.kind==DSR_JOB_LAN_STA_VERIFY);
 dsr_network_home(NULL);CHECK(!dsr_activity_restart_reserve());response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_CONNECTED);CHECK(!dsr_ui_ble_rpc_active()&&!find_label(lv_screen_active(),"Nazwa i pomieszczenie"));
 wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 for(unsigned language=1;language<=2;language++){
  dsr_sc_appearance(DSR_SC_BLUE,false,language);rpc_intro();screenshot(language==1?"243-ble-first-en":"245-ble-first-de");rpc_disclose();screenshot(language==1?"244-ble-second-en":"246-ble-second-de");
  lv_obj_t *text=find_label(lv_screen_active(),"Przekaż nazwę i hasło zapisanej sieci temu Shelly. Otwarte GATT nie uwierzytelnia kryptograficznie urządzenia; użyj tylko własnego sprzętu.");CHECK(text);bounds(text,330);rpc_finish_cancel();
 }
 dsr_sc_appearance(DSR_SC_BLUE,false,0);rpc_intro();rpc_disclose();test_rpc_update(DSR_BLE_RPC_ERROR,DSR_AUTH_REQUIRED,false,NULL);dsr_ui_ble_rpc_tick();CHECK(find_label(lv_screen_active(),"Bluetooth • wynik"));screenshot("247-ble-security-unsupported");click("Anuluj");
 /* Timers remain valid above 2^31 ms. */
 lv_tick_inc(UINT32_C(0x90001000)-lv_tick_get());rpc_intro();rpc_disclose();lv_tick_inc(120001);dsr_ui_ble_rpc_tick();test_rpc_update(DSR_BLE_RPC_CANCELLED,DSR_BUSY,false,NULL);dsr_ui_ble_rpc_tick();CHECK(!dsr_ui_ble_rpc_active());
 wifi.connected=false;tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 puts("BLE RPC LCD: two physical consents, stale callbacks/hide/reopen/cancel, uncertain write and HTTP identity+STA handoff; transport/radio MODEL");
}
