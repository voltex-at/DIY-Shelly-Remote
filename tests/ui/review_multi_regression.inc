/* Reviewer regressions: real LCD/control code, explicit workspace/worker MODEL.
 * Test NVS separately; these cases prove UI transaction ordering and recovery. */
static unsigned review_entry_id(const char *name)
{
 cJSON *r=room_document();const cJSON *d;unsigned id=0;
 cJSON_ArrayForEach(d,cJSON_GetObjectItemCaseSensitive(r,"devices"))if(!strcmp(cJSON_GetObjectItemCaseSensitive(d,"name")->valuestring,name))id=(unsigned)cJSON_GetObjectItemCaseSensitive(d,"id")->valueint;
 cJSON_Delete(r);CHECK(id);return id;
}
static void review_remove_layout(unsigned id);
static void review_preserved_room(void);
static void review_deleted_migration_regression(void)
{
 CHECK(!in_flight&&!maintenance_ready);wifi.connected=true;tick();CHECK(!in_flight);
 dsr_connection_t a={.device={.host="192.168.1.80",.name="Deleted migration A",.device_id="shelly-deleted-migration-A",.generation=DSR_GEN2_PLUS,.online=true,.has_switch=true}};
 dsr_connection_t b=a;strcpy(b.device.host,"192.168.1.81");strcpy(b.device.name,"Preserved migration B");strcpy(b.device.device_id,"shelly-preserved-migration-B");
 CHECK(dsr_network_bind_verified(&a,0)==DSR_OK);response=(dsr_event_t){.device=a.device};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(dsr_network_bind_verified(&b,0)==DSR_OK);response=(dsr_event_t){.device=b.device};deliver(DSR_OK,DSR_WIFI_IDLE);
 a.layout_id=review_entry_id(a.device.name);unsigned bid=review_entry_id(b.device.name);CHECK(dsr_network_layout_reserved(a.layout_id)&&dsr_network_layout_reserved(bid));
 wifi.connected=false;tick();unsigned before=requests;
 /* Existing published A begins a DHCP/layout update. A competing Web save
  * deletes its exact numeric ID before the queued candidate receives ACK. */
 strcpy(a.device.host,"192.168.1.90");test_workspace_defer(true);dsr_ui_workspace_migrate_bound(&a,0,NULL);
 b.layout_id=bid;strcpy(b.device.name,"Retained migration B");dsr_ui_workspace_migrate_bound(&b,0,NULL);tick();
 bool busy;char message[96];dsr_workspace_status(&busy,message);CHECK(busy);
 test_workspace_finish(false);test_workspace_defer(false);review_remove_layout(a.layout_id);tick();
 cJSON *r=room_document(),*devices=cJSON_GetObjectItemCaseSensitive(r,"devices");
 if(cJSON_GetArraySize(devices)!=1){const cJSON *v;fprintf(stderr,"Deleted-ID migration counterexample: expected only preserved ID %u; actual:",bid);cJSON_ArrayForEach(v,devices)fprintf(stderr," [%d %s]",cJSON_GetObjectItemCaseSensitive(v,"id")->valueint,cJSON_GetObjectItemCaseSensitive(v,"name")->valuestring);fputc('\n',stderr);}
 CHECK(cJSON_GetArraySize(devices)==1);
 CHECK((unsigned)cJSON_GetObjectItemCaseSensitive(devices->child,"id")->valueint==bid&&!strcmp(cJSON_GetObjectItemCaseSensitive(devices->child,"name")->valuestring,"Preserved migration B"));cJSON_Delete(r);
 dsr_result_t result;CHECK(!dsr_ui_workspace_migrate_status(&result)&&result==DSR_CONFLICT&&requests==before);
 lv_tick_inc(5100);tick();tick();r=room_document();devices=cJSON_GetObjectItemCaseSensitive(r,"devices");CHECK(cJSON_GetArraySize(devices)==1);
 CHECK((unsigned)cJSON_GetObjectItemCaseSensitive(devices->child,"id")->valueint==bid&&!strcmp(cJSON_GetObjectItemCaseSensitive(devices->child,"name")->valuestring,b.device.name));cJSON_Delete(r);review_preserved_room();
 CHECK(!dsr_ui_workspace_migrate_status(&result)&&result==DSR_OK&&requests==before&&!dsr_network_endpoint_added(a.device.device_id,DSR_GEN2_PLUS,0)&&dsr_network_endpoint_added(b.device.device_id,DSR_GEN2_PLUS,0));
 review_remove_layout(bid);tick();
 printf("PASS: published-ID Web deletion wins over stale DHCP/CAS migration; unrelated migration finishes; no recreated endpoint or tile, no HTTP replay\n");
}
static void review_remove_layout(unsigned id)
{
 cJSON *r=room_document(),*devices=cJSON_GetObjectItemCaseSensitive(r,"devices"),*d,*dashboard;
 int at=0;cJSON_ArrayForEach(d,devices){if((unsigned)cJSON_GetObjectItemCaseSensitive(d,"id")->valueint==id){cJSON_DeleteItemFromArray(devices,at);break;}at++;}
 cJSON_ArrayForEach(dashboard,cJSON_GetObjectItemCaseSensitive(r,"dashboards")){
  cJSON *tiles=cJSON_GetObjectItemCaseSensitive(dashboard,"tiles");for(int i=cJSON_GetArraySize(tiles)-1;i>=0;i--){cJSON *tile=cJSON_GetArrayItem(tiles,i);if(!strcmp(cJSON_GetObjectItemCaseSensitive(tile,"type")->valuestring,"device")&&(unsigned)cJSON_GetObjectItemCaseSensitive(tile,"ref")->valueint==id)cJSON_DeleteItemFromArray(tiles,i);}
 }
 publish_document(r);
}
static void review_preserved_room(void)
{
 cJSON *r=room_document(),*rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms");CHECK(cJSON_GetArraySize(rooms)==1);
 cJSON *room=rooms->child;CHECK(cJSON_GetObjectItemCaseSensitive(room,"id")->valueint==777&&!strcmp(cJSON_GetObjectItemCaseSensitive(room,"photo")->valuestring,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));cJSON_Delete(r);
}
static void review_multi_regression(void)
{
 CHECK(!wifi.connected&&!in_flight&&!maintenance_ready);unsigned unbind_before=unbind_jobs;
 CHECK(dsr_network_endpoint_added("shelly-A",DSR_GEN2_PLUS,0)&&dsr_network_endpoint_added("shelly-B",DSR_GEN2_PLUS,0));
 cJSON *r=room_document();cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"rooms"),cJSON_Parse("{\"id\":777,\"name\":\"Review preserved room\",\"icon\":0,\"photo\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}"));publish_document(r);tick();
 unsigned a=review_entry_id("Relay A"),b=review_entry_id("Relay B");
 /* An unavailable or corrupt snapshot never authorizes deletion. */
 for(unsigned fault=1;fault<=2;fault++){test_workspace_raw_fault(fault);tick();CHECK(unbind_jobs==unbind_before&&dsr_network_endpoint_added("shelly-A",DSR_GEN2_PLUS,0)&&dsr_network_endpoint_added("shelly-B",DSR_GEN2_PLUS,0));}
 test_workspace_raw_fault(0);tick();review_preserved_room();
 /* Changing only a private binding's public locator disables its mapping;
  * it neither changes the private locator nor erases the durable endpoint. */
 r=room_document();cJSON *entry=cJSON_GetObjectItemCaseSensitive(r,"devices")->child;cJSON_ReplaceItemInObjectCaseSensitive(entry,"host",cJSON_CreateString("192.168.1.199"));publish_document(r);tick();
 CHECK(unbind_jobs==unbind_before&&dsr_network_endpoint_added("shelly-A",DSR_GEN2_PLUS,0));
 r=room_document();entry=cJSON_GetObjectItemCaseSensitive(r,"devices")->child;CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(entry,"host")->valuestring,"192.168.1.199"));cJSON_ReplaceItemInObjectCaseSensitive(entry,"host",cJSON_CreateString("192.168.1.60"));publish_document(r);tick();
 /* Targeted UNBIND failure keeps A recoverable, leaves B untouched and backs
  * off rather than repeatedly writing NVS on every UI tick. */
 maintenance_result=DSR_IO;review_remove_layout(a);tick();CHECK(unbind_jobs==unbind_before+1&&dsr_network_endpoint_added("shelly-A",DSR_GEN2_PLUS,0)&&dsr_network_endpoint_added("shelly-B",DSR_GEN2_PLUS,0));
 for(unsigned i=0;i<3;i++){tick();}CHECK(unbind_jobs==unbind_before+1);review_preserved_room();
 maintenance_result=DSR_OK;lv_tick_inc(5001);tick();CHECK(unbind_jobs==unbind_before+2&&!dsr_network_endpoint_added("shelly-A",DSR_GEN2_PLUS,0)&&dsr_network_endpoint_added("shelly-B",DSR_GEN2_PLUS,0));
 review_remove_layout(b);tick();CHECK(!dsr_network_endpoint_added("shelly-B",DSR_GEN2_PLUS,0));review_preserved_room();

 dsr_connection_t c={.device={.host="192.168.1.70",.name="Review C",.device_id="shelly-review-C",.generation=DSR_GEN2_PLUS,.online=true,.has_switch=true},.credentials={.username="admin",.password="review-private-secret"}};
 wifi.connected=true;tick();CHECK(!in_flight);test_workspace_defer(true);unsigned pubs=publish_jobs,network_before=requests;
 CHECK(dsr_network_bind_verified(&c,0)==DSR_OK);response=(dsr_event_t){.device=c.device};deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(dsr_network_busy()&&publish_jobs==pubs&&requests==network_before+1);CHECK(!dsr_activity_restart_reserve());
 bool pending_config;char message[96];dsr_workspace_status(&pending_config,message);CHECK(pending_config);r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"devices"))==0);cJSON_Delete(r);
 /* A failed first-store ACK ends this attempt. The verified private endpoint
  * is recoverable, while the prior workspace and all rooms remain intact. */
 wifi.connected=false;test_workspace_finish(false);tick();CHECK(!dsr_network_busy()&&publish_jobs==pubs&&requests==network_before+1);review_preserved_room();
 test_workspace_defer(false);lv_tick_inc(5001);tick();tick();unsigned cid=review_entry_id("Review C");CHECK(dsr_network_layout_reserved(cid)&&publish_jobs==pubs+1&&requests==network_before+1);review_preserved_room();

 /* Fail the second-store publication independently of a successful config
  * ACK. It must not repeat BIND/HTTP or lose the already published C entry. */
 dsr_connection_t d=c;strcpy(d.device.host,"192.168.1.71");strcpy(d.device.name,"Review D");strcpy(d.device.device_id,"shelly-review-D");
 wifi.connected=true;tick();if(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=false;deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 maintenance_result=DSR_IO;pubs=publish_jobs;network_before=requests;CHECK(dsr_network_bind_verified(&d,0)==DSR_OK);wifi.connected=false;response=(dsr_event_t){.device=d.device};deliver(DSR_OK,DSR_WIFI_IDLE);
 unsigned did=review_entry_id("Review D");CHECK(!dsr_network_busy()&&!dsr_network_layout_reserved(did)&&dsr_network_layout_reserved(cid)&&publish_jobs==pubs+1&&requests==network_before+1);review_preserved_room();
 maintenance_result=DSR_OK;lv_tick_inc(5001);tick();CHECK(dsr_network_layout_reserved(did)&&publish_jobs==pubs+2&&requests==network_before+1);review_preserved_room();
 /* A long read cannot leave an old online LCD card or active power button
  * past the freshness boundary. No network completion triggers this redraw. */
 wifi.connected=true;tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;response.device.output=false;deliver(DSR_OK,DSR_WIFI_IDLE);
 if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 dsr_network_home(NULL);click("Review C");CHECK(find_label(lv_screen_active(),"Wyłączone • Online"));lv_tick_inc(5001);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);unsigned before_stale=requests;
 lv_tick_inc(15001);tick();CHECK(find_label(lv_screen_active(),"Offline • stan niepotwierdzony")&&requests==before_stale);lv_obj_t *power=power_target(lv_screen_active());CHECK(power&&lv_obj_has_state(power,LV_STATE_DISABLED));lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);CHECK(requests==before_stale);
 wifi.connected=false;response=(dsr_event_t){.device=submitted.connection.device};response.device.online=false;deliver(DSR_OFFLINE,DSR_WIFI_IDLE);
 review_remove_layout(cid);tick();review_remove_layout(did);tick();CHECK(!dsr_network_endpoint_added(c.device.device_id,DSR_GEN2_PLUS,0)&&!dsr_network_endpoint_added(d.device.device_id,DSR_GEN2_PLUS,0));review_preserved_room();
 CHECK(!in_flight&&!maintenance_ready&&!dsr_network_busy());CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: reviewer LCD targeted CRUD, unavailable/corrupt-store preservation, independent two-store IO/ACK recovery and restart lease; worker/workspace MODEL\n");
}
