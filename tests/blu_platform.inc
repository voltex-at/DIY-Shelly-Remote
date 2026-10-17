/* Shelly BLU through a Gen2+ gateway: genuine HTTP adapter, parsers, registry
 * and fleet; transport, SDK and Flash MODEL. JSON shapes follow the Gen2 API
 * docs for BTHomeDevice / BTHomeSensor (GetConfig, GetStatus, GetKnownObjects). */
static const char blu_gateway_identity[]="{\"gen\":2,\"id\":\"shellyplus1pm-aabbccddeeff\",\"model\":\"SNSW-001P16EU\"}";
static const char blu_config[]="{\"id\":201,\"addr\":\"3c:2e:f5:71:d5:2a\",\"name\":\"Motion Bad\",\"meta\":null}";
static const char blu_device_status[]="{\"id\":201,\"rssi\":-71,\"battery\":93,\"packet_id\":17,\"last_updated_ts\":1767225500.25,\"key\":false,\"paired\":true,\"rpc\":false,\"rsv\":0,\"fw_ver\":\"1.0.12\"}";
static const char blu_known[]="{\"id\":201,\"objects\":[{\"obj_id\":0,\"idx\":0,\"component\":null},{\"obj_id\":1,\"idx\":0,\"component\":\"bthomesensor:204\"},{\"obj_id\":5,\"idx\":0,\"component\":\"bthomesensor:205\"},{\"obj_id\":33,\"idx\":0,\"component\":\"bthomesensor:206\"},{\"obj_id\":63,\"idx\":0,\"component\":\"bthomesensor:207\"}]}";
static dsr_connection_t blu_connection(void)
{dsr_connection_t c=connection();c.device.generation=DSR_GEN2_PLUS;c.device.endpoint_kind=DSR_ENDPOINT_BLU;c.device.channel=1;strcpy(c.device.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a");return c;}
static void test_blu_protocol(void)
{
 dsr_connection_t c=blu_connection();dsr_device_t d;dsr_telemetry_t t;
 step_t data[]={{"/shelly",blu_gateway_identity,200,false,false},{"/rpc/BTHomeDevice.GetConfig?id=201",blu_config,200,false,true},
  {"/rpc/BTHomeDevice.GetStatus?id=201",blu_device_status,200,false,true},{"/rpc/BTHomeDevice.GetKnownObjects?id=201",blu_known,200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=204","{\"id\":204,\"value\":92,\"last_updated_ts\":1767225500.25}",200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=205","{\"id\":205,\"value\":317,\"last_updated_ts\":1767225500.25}",200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=206","{\"id\":206,\"value\":true,\"last_updated_ts\":1767225500.25}",200,false,true}};
 /* Rotation (obj 63) is known but not shown: no request for bthomesensor:207. */
 fixture(data,7,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_OK&&used==7&&d.online&&!d.has_switch&&!d.has_power&&!d.output);
 CHECK(!strcmp(d.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a")&&(t.blu.flags&DSR_BLU_VALID)&&(t.blu.flags&DSR_BLU_HAS_MOTION)&&(t.blu.flags&DSR_BLU_MOTION)&&!(t.blu.flags&DSR_BLU_HAS_OPEN));
 CHECK(t.has_blu_battery&&t.blu_battery_pct==92&&t.has_illuminance&&t.illuminance_lux==317&&!t.has_blu_temperature&&(t.blu.flags&DSR_BLU_HAS_RSSI)&&t.blu.rssi==-71&&t.blu_updated_s==1767225500u&&!t.blu.errors);
 /* LAN wizard bind knows only the gateway id; the probe appends the MAC. */
 dsr_connection_t gateway_only=c;strcpy(gateway_only.device.device_id,"shellyplus1pm-aabbccddeeff");fixture(data,7,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_probe_telemetry(&gateway_only,&d,&t)==DSR_OK&&!strcmp(d.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a"));
 /* Another sensor now sits on cid 201, another gateway, or a prefix that only looks alike: mismatch, never its data. */
 data[1].body="{\"id\":201,\"addr\":\"aa:bb:cc:dd:ee:01\",\"name\":null,\"meta\":null}";fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_IDENTITY_MISMATCH&&used==2&&!d.online);data[1].body=blu_config;
 dsr_connection_t foreign=c;strcpy(foreign.device.device_id,"shellyplus1pm-aabbccddeeff_000000000001");fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&foreign,&d,&t)==DSR_IDENTITY_MISMATCH);
 dsr_connection_t prefix=c;strcpy(prefix.device.device_id,"shellyplus1pm-aabbcc");fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&prefix,&d,&t)==DSR_IDENTITY_MISMATCH);
 /* BLU Gateway Gen3 serves BLU endpoints; as a relay it stays unsupported. */
 step_t gw[7];memcpy(gw,data,sizeof(gw));gw[0].body="{\"gen\":3,\"id\":\"shellyblugwg3-aabbccddeeff\",\"model\":\"S3GW-1DBT001\"}";
 dsr_connection_t via_gateway=c;strcpy(via_gateway.device.device_id,"shellyblugwg3-aabbccddeeff");fixture(gw,7,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_probe_telemetry(&via_gateway,&d,&t)==DSR_OK&&used==7&&!strcmp(d.device_id,"shellyblugwg3-aabbccddeeff_3c2ef571d52a"));
 dsr_connection_t relay=connection();relay.device.generation=DSR_GEN2_PLUS;strcpy(relay.device.device_id,"shellyblugwg3-aabbccddeeff");fixture(gw,1,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_probe_telemetry(&relay,&d,&t)!=DSR_OK&&used==1&&!d.online);
 /* Read-only: never a Switch.Set. */
 fixture(data,0,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_set_output(&c,true)==DSR_UNSUPPORTED&&used==0);
 /* Malformed answers at each stage: no partial telemetry, offline. */
 const char *bad_status[]={"null","{\"id\":202,\"battery\":93}","{\"id\":201,\"battery\":101}","{\"id\":201,\"battery\":93,\"battery\":94}","{\"id\":201,\"errors\":\"decrypt_failed\"}"};
 for(unsigned i=0;i<5;i++){data[2].body=bad_status[i];fixture(data,3,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)!=DSR_OK&&!d.online&&!(t.blu.flags&DSR_BLU_VALID)&&!t.has_blu_battery);}data[2].body=blu_device_status;
 const char *bad_known[]={"{\"id\":201}","{\"id\":201,\"objects\":[{\"obj_id\":33,\"idx\":0,\"component\":\"switch:0\"}]}","{\"id\":201,\"objects\":[{\"obj_id\":300,\"idx\":0,\"component\":null}]}","{\"id\":201,\"objects\":[{\"obj_id\":33,\"idx\":0,\"component\":\"bthomesensor:x\"}]}"};
 for(unsigned i=0;i<4;i++){data[3].body=bad_known[i];fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)!=DSR_OK&&!d.online&&!(t.blu.flags&DSR_BLU_VALID));}data[3].body=blu_known;
 const char *bad_sensor[]={"{\"id\":204,\"value\":\"92\"}","{\"id\":209,\"value\":92}","{\"id\":204,\"value\":-1}"};
 for(unsigned i=0;i<3;i++){data[4].body=bad_sensor[i];fixture(data,5,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)!=DSR_OK&&!d.online&&!t.has_illuminance);}data[4].body="{\"id\":204,\"value\":92,\"last_updated_ts\":1767225500.25}";
 /* Parsers directly. */
 cJSON *j=cJSON_Parse("{\"id\":200,\"value\":false,\"last_updated_ts\":1}");dsr_telemetry_t w={0};CHECK(dsr_blu_apply_sensor(j,200,0x2D,&w)==DSR_OK&&(w.blu.flags&DSR_BLU_HAS_OPEN)&&!(w.blu.flags&DSR_BLU_OPEN));cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"value\":true}");CHECK(dsr_blu_apply_sensor(j,200,0x1A,&w)==DSR_OK&&(w.blu.flags&DSR_BLU_OPEN));cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"value\":21.37}");CHECK(dsr_blu_apply_sensor(j,200,0x45,&w)==DSR_OK&&w.has_blu_temperature&&w.blu_temperature_c>21.36f&&w.blu_temperature_c<21.38f);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"value\":54.6}");CHECK(dsr_blu_apply_sensor(j,200,0x2E,&w)==DSR_OK&&(w.blu.flags&DSR_BLU_HAS_HUMIDITY)&&w.blu.humidity==55);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"value\":4}");CHECK(dsr_blu_apply_sensor(j,200,0x3A,&w)==DSR_OK&&(w.blu.flags&DSR_BLU_HAS_BUTTON)&&w.blu.button==4);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"value\":null}");dsr_telemetry_t empty={0};CHECK(dsr_blu_apply_sensor(j,200,0x21,&empty)==DSR_OK&&!empty.blu.flags);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"value\":121}");CHECK(dsr_blu_apply_sensor(j,200,0x02,&w)==DSR_BAD_RESPONSE);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":201,\"errors\":[\"key_missing_or_bad\",\"unencrypted_data\"],\"last_update_ts\":5}");CHECK(dsr_blu_parse_device(j,201,&w)==DSR_OK&&w.blu.errors==(DSR_BLU_ERROR_KEY|DSR_BLU_ERROR_UNENCRYPTED)&&w.blu_updated_s==5&&!w.has_blu_battery);cJSON_Delete(j);
 char mac[13];const char *bad_addr[]={"{\"id\":201,\"addr\":\"3c2ef571d52a\"}","{\"id\":201,\"addr\":\"00:00:00:00:00:00\"}","{\"id\":201,\"addr\":\"3c:2e:f5:71:d5:2g\"}","{\"id\":200,\"addr\":\"3c:2e:f5:71:d5:2a\"}"};
 for(unsigned i=0;i<4;i++){j=cJSON_Parse(bad_addr[i]);CHECK(dsr_blu_parse_address(j,201,mac)==DSR_BAD_RESPONSE&&!mac[0]);cJSON_Delete(j);}
 dsr_blu_object_t objects[2];unsigned n;j=cJSON_Parse(blu_known);CHECK(dsr_blu_parse_known(j,201,objects,2,&n)==DSR_OK&&n==2&&objects[0].sensor==204&&objects[0].obj_id==1&&objects[1].sensor==205);cJSON_Delete(j);
 /* Gateway capability: BTHomeDevice cids 200..215 become BLU channels. */
 j=cJSON_Parse("{\"switch:0\":{\"id\":0,\"output\":false},\"bthomedevice:200\":{\"id\":200},\"bthomedevice:203\":{\"id\":203},\"bthomedevice:230\":{\"id\":230},\"bthomesensor:200\":{\"id\":200,\"value\":true}}");
 dsr_lan_capabilities_t caps;CHECK(dsr_lan_parse_capabilities(j,2,&caps)==DSR_OK&&caps.switches==1&&caps.blu==((1u<<0)|(1u<<3)));cJSON_Delete(j);
 j=cJSON_Parse("{\"bthomedevice:201\":{\"id\":202}}");CHECK(dsr_lan_parse_capabilities(j,2,&caps)==DSR_BAD_RESPONSE);cJSON_Delete(j);
 admission_open();
}
static void test_blu_registry_fleet(void)
{
 memset(sizes,0,sizeof(sizes));memset(schemas,0,sizeof(schemas));
 dsr_connection_t c=blu_connection();c.layout_id=91;CHECK(dsr_connection_valid(&c,true)&&dsr_connection_save(&c)==ESP_OK&&((uint32_t*)blobs[2])[0]==7);
 dsr_connection_t loaded[16];size_t n;CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==1&&loaded[0].device.endpoint_kind==DSR_ENDPOINT_BLU&&loaded[0].device.channel==1);
 dsr_connection_t gen1=c;gen1.device.generation=DSR_GEN1;CHECK(!dsr_connection_valid(&gen1,true));
 dsr_connection_t high=c;high.device.channel=16;CHECK(!dsr_connection_valid(&high,true));
 dsr_fleet_t f={0};c.device.online=true;CHECK(dsr_fleet_upsert(&f,&c,100)==DSR_OK&&f.slots[0].next_poll==15100);
 CHECK(dsr_fleet_request(&f,0,true,true,100)==DSR_UNSUPPORTED);
 CHECK(dsr_fleet_ht_refresh(&f,0,true,200)==DSR_OK);dsr_job_t job;unsigned at;CHECK(dsr_fleet_next(&f,200,false,&job,&at)&&job.kind==DSR_JOB_PROBE);
 job.id=700;dsr_fleet_started(&f,at,&job);dsr_event_t event={.id=700,.device=c.device,.result=DSR_OFFLINE};CHECK(dsr_fleet_complete(&f,at,&event,300));
 CHECK(f.slots[0].next_poll==300+30000);admission_open();
 c.layout_id=0;CHECK(dsr_connection_remove(&c)==ESP_OK);
}
