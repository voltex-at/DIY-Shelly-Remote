/* Actual UI/fleet, with explicit worker and durable workspace providers. */
static void bind_rebind_regression(void)
{
 CHECK(!wifi.connected&&!in_flight&&!maintenance_ready);
 dsr_lan_device_t gateway={.host="192.168.1.90",.id="shellyblugwg3-aabbccddeeff",.model="S3GW-1DBT001",.generation=3};dsr_lan_capabilities_t none={0};unsigned before_gateway=requests;
 dsr_ui_lan_adopt(&gateway,&none);CHECK(find_label(lv_screen_active(),"Bramka BLU • przekaźnik nieobsługiwany."));lv_obj_t *save=find_label(lv_screen_active(),"Zapisz");CHECK(save&&lv_obj_has_state(lv_obj_get_parent(save),LV_STATE_DISABLED));click("Zapisz");CHECK(requests==before_gateway);screenshot("247-blu-gateway-no-relay");dsr_network_home(NULL);
 cJSON *original=room_document();
 dsr_connection_t sibling={.device={.host="192.168.1.60",.name="Sibling A1",.device_id="shelly-A",.generation=DSR_GEN2_PLUS,.channel=1,.online=true,.has_switch=true},.credentials={.username="admin",.password="first-private"}};
 wifi.connected=true;CHECK(dsr_network_bind_verified(&sibling,0)==DSR_OK);wifi.connected=false;response=(dsr_event_t){.device=sibling.device};deliver(DSR_OK,DSR_WIFI_IDLE);unsigned sibling_id=review_entry_id("Sibling A1");CHECK(dsr_network_layout_reserved(sibling_id));
 unsigned pubs=publish_jobs;
 dsr_connection_t candidate={.device={.host="192.168.1.160",.name="Uncommitted replacement",.device_id="shelly-A",.generation=DSR_GEN2_PLUS,.online=true,.has_switch=true},.credentials={.username="admin",.password="replacement-private"}};
 for(unsigned conflict=0;conflict<2;conflict++){
  test_workspace_defer(true);wifi.connected=true;CHECK(dsr_network_bind_verified(&candidate,0)==DSR_OK);wifi.connected=false;
  response=(dsr_event_t){.device=candidate.device};deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(dsr_network_busy()&&publish_jobs==pubs);
  if(conflict){test_workspace_finish(false);cJSON *web=room_document();cJSON_ReplaceItemInObjectCaseSensitive(web,"accent",cJSON_CreateNumber(0x123456));publish_document(web);tick();CHECK(dsr_network_busy()&&publish_jobs==pubs);}
  test_workspace_finish(false);tick();test_workspace_defer(false);CHECK(!dsr_network_busy()&&publish_jobs==pubs);
  lv_tick_inc(5001);tick();CHECK(publish_jobs==pubs); /* no late private save */
  cJSON *saved=room_document(),*d;unsigned seen=0;
  cJSON_ArrayForEach(d,cJSON_GetObjectItemCaseSensitive(saved,"devices")){
   const char *name=cJSON_GetObjectItemCaseSensitive(d,"name")->valuestring,*host=cJSON_GetObjectItemCaseSensitive(d,"host")->valuestring;
   if(!strcmp(name,"Relay A")){CHECK(!strcmp(host,"192.168.1.60"));seen++;}
   if(!strcmp(name,"Relay B")){CHECK(!strcmp(host,"192.168.1.51"));seen++;}
   if(!strcmp(name,"Sibling A1")){CHECK(!strcmp(host,"192.168.1.60"));seen++;}
  }CHECK(seen==3);cJSON_Delete(saved);
  /* Even after loss of shared storage, fallback reads the previous private
   * bindings, never the uncommitted replacement locator or password. */
  test_workspace_raw_fault(1);wifi.connected=true;seen=0;
  const char *names[]={"Relay A","Sibling A1","Relay B"};
  /* Contextual refresh deliberately targets one stable ID, not the fleet. */
  for(unsigned target=0;target<3;target++){dsr_network_home(NULL);click(names[target]);click("Odśwież stan");
  while(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);const dsr_connection_t *c=&submitted.connection;
   if(!strcmp(c->device.device_id,"shelly-A")){CHECK(!strcmp(c->device.host,"192.168.1.60")&&!strcmp(c->credentials.password,"first-private"));seen|=c->device.channel==1?4:1;}
   else {CHECK(!strcmp(c->device.device_id,"shelly-B")&&!strcmp(c->device.host,"192.168.1.51")&&!strcmp(c->credentials.password,"second-private"));seen|=2;}
   response=(dsr_event_t){.device=c->device};deliver(DSR_AUTH_REQUIRED,DSR_WIFI_IDLE);
  }}CHECK(seen==7&&publish_jobs==pubs);wifi.connected=false;tick();test_workspace_raw_fault(0);dsr_network_home(NULL);
 }
 /* Web removal in either finalization window cannot be resurrected by a
  * late private ACK. The actual worker mapping guard is tested separately. */
 dsr_connection_t late={.device={.host="192.168.1.91",.name="Late bind C",.device_id="shelly-late-C",.generation=DSR_GEN2_PLUS,.online=true,.has_switch=true}};
 for(unsigned after_private=0;after_private<2;after_private++){
  test_workspace_defer(true);hold_maintenance=true;wifi.connected=true;CHECK(dsr_network_bind_verified(&late,0)==DSR_OK);wifi.connected=false;response=(dsr_event_t){.device=late.device};deliver(DSR_OK,DSR_WIFI_IDLE);
  test_workspace_finish(true);test_workspace_defer(false);tick();CHECK(maintenance_ready&&maintenance.kind==DSR_JOB_LAYOUT_PUBLISH&&dsr_network_busy());unsigned removed_id=maintenance.connection.layout_id;CHECK(removed_id);
  review_remove_layout(removed_id);maintenance_result=after_private?DSR_OK:DSR_CONFLICT;hold_maintenance=false;tick();
  CHECK(!dsr_network_busy()&&!dsr_network_endpoint_added(late.device.device_id,DSR_GEN2_PLUS,0)&&!dsr_network_layout_reserved(removed_id));
  test_workspace_raw_fault(1);dsr_network_home(NULL);CHECK(!find_label(lv_screen_active(),"Late bind C"));test_workspace_raw_fault(0);dsr_network_home(NULL);maintenance_result=DSR_OK;
  CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 }
 pubs=publish_jobs;
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(original,"revision"),dsr_workspace_revision());publish_document(original);tick();CHECK(!dsr_network_layout_reserved(sibling_id));
 CHECK(dsr_network_endpoint_added("shelly-A",DSR_GEN2_PLUS,0)&&dsr_network_endpoint_added("shelly-B",DSR_GEN2_PLUS,0)&&publish_jobs==pubs);
 printf("PASS: rebind IO/CAS keeps previous A/B private locators/credentials; no private write or late migration before exact workspace ACK; SDK/workspace MODEL\n");
}
