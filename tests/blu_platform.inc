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
  {"/rpc/BTHomeSensor.GetStatus?id=206","{\"id\":206,\"value\":true,\"last_updated_ts\":1767225500.25}",200,false,true},
  /* First poll per (gateway, cid) asks once whether it is a BLU TRV; an error answer means no. */
  {"/rpc/BluTrv.GetConfig?id=201","{\"code\":-105,\"message\":\"Argument 'id', value 201 not found!\"}",404,false,true}};
 /* Rotation (obj 63) is known but not shown: no request for bthomesensor:207. */
 fixture(data,8,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_OK&&used==8&&d.online&&!d.has_switch&&!d.has_power&&!d.output);
 CHECK(!strcmp(d.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a")&&(t.blu.flags&DSR_BLU_VALID)&&(t.blu.flags&DSR_BLU_HAS_MOTION)&&(t.blu.flags&DSR_BLU_MOTION)&&!(t.blu.flags&DSR_BLU_HAS_OPEN));
 CHECK(t.has_blu_battery&&t.blu_battery_pct==92&&t.has_illuminance&&t.illuminance_lux==317&&!t.has_blu_temperature&&(t.blu.flags&DSR_BLU_HAS_RSSI)&&t.blu.rssi==-71&&t.blu_updated_s==1767225500u&&t.blu.errors==DSR_BLU_TRV_CHECKED); /* asked once: not a TRV */
 /* LAN wizard bind knows only the gateway id; the probe appends the MAC. */
 dsr_connection_t gateway_only=c;strcpy(gateway_only.device.device_id,"shellyplus1pm-aabbccddeeff");gateway_only.telemetry=t;fixture(data,7,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_probe_telemetry(&gateway_only,&d,&t)==DSR_OK&&used==7&&(t.blu.errors&DSR_BLU_TRV_CHECKED)&& /* the hint rides in the endpoint telemetry */!strcmp(d.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a"));
 /* Another sensor now sits on cid 201, another gateway, or a prefix that only looks alike: mismatch, never its data. */
 data[1].body="{\"id\":201,\"addr\":\"aa:bb:cc:dd:ee:01\",\"name\":null,\"meta\":null}";fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_IDENTITY_MISMATCH&&used==2&&!d.online);data[1].body=blu_config;
 dsr_connection_t foreign=c;strcpy(foreign.device.device_id,"shellyplus1pm-aabbccddeeff_000000000001");fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&foreign,&d,&t)==DSR_IDENTITY_MISMATCH);
 dsr_connection_t prefix=c;strcpy(prefix.device.device_id,"shellyplus1pm-aabbcc");fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&prefix,&d,&t)==DSR_IDENTITY_MISMATCH);
 /* BLU Gateway Gen3 serves BLU endpoints; as a relay it stays unsupported. */
 step_t gw[8];memcpy(gw,data,sizeof(gw));gw[0].body="{\"gen\":3,\"id\":\"shellyblugwg3-aabbccddeeff\",\"model\":\"S3GW-1DBT001\"}";
 dsr_connection_t via_gateway=c;strcpy(via_gateway.device.device_id,"shellyblugwg3-aabbccddeeff");fixture(gw,8,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_probe_telemetry(&via_gateway,&d,&t)==DSR_OK&&used==8&&!strcmp(d.device_id,"shellyblugwg3-aabbccddeeff_3c2ef571d52a"));
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
 /* Separate poll, not a contradictory second object in the same bundle. */
 w=(dsr_telemetry_t){0};j=cJSON_Parse("{\"id\":200,\"value\":true}");CHECK(dsr_blu_apply_sensor(j,200,0x1A,&w)==DSR_OK&&(w.blu.flags&DSR_BLU_OPEN));cJSON_Delete(j);
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

/* Shape observed on a real BLU Gateway Gen3 (2026-10-17, MACs replaced):
 * BTHome devices only in Shelly.GetComponents, paged (total 11, 9 per page),
 * a BLU TRV with two temperatures and a paired sensor with all-null status. */
static void test_blu_gateway_components(void)
{
 const char *page0="{\"components\":[{\"key\":\"blutrv:200\",\"status\":{\"id\":200,\"target_C\":20.0},\"config\":{\"id\":200,\"addr\":\"aa:bb:cc:00:00:01\",\"trv\":\"bthomedevice:200\"},\"attrs\":{\"model_id\":8}},"
  "{\"key\":\"bthomedevice:200\",\"status\":{\"id\":200,\"rssi\":-51,\"battery\":100},\"config\":{\"id\":200,\"addr\":\"aa:bb:cc:00:00:01\",\"name\":null},\"attrs\":{\"model_id\":8}},"
  "{\"key\":\"bthomedevice:201\",\"status\":{\"id\":201,\"rssi\":null,\"battery\":null,\"packet_id\":null,\"last_updated_ts\":null,\"key\":false,\"paired\":true,\"rpc\":false,\"rsv\":-1,\"fw_ver\":null},\"config\":{\"id\":201,\"addr\":\"aa:bb:cc:00:0b:11\",\"name\":\"Okno \u017Cółte w salonie dużym\"},\"attrs\":{\"model_id\":2}},"
  "{\"key\":\"bthomesensor:200\",\"status\":{\"id\":200,\"value\":100}}],\"cfg_rev\":25461,\"offset\":0,\"total\":5}";
 const char *page1="{\"components\":[{\"key\":\"bthomedevice:230\",\"status\":{\"id\":230}}],\"cfg_rev\":25461,\"offset\":4,\"total\":5}";
 uint16_t blu=0;unsigned next=0,total=0;cJSON *j=cJSON_Parse(page0);
 dsr_blu_labels_t labels={0};CHECK(dsr_lan_parse_components(j,0,&blu,&labels,&next,&total)==DSR_OK&&blu==3&&next==4&&total==5);cJSON_Delete(j);
 CHECK(labels.valid==3&&labels.items[0].model==8&&!labels.items[0].name[0]&&labels.items[0].mac_tail[0]==0x00&&labels.items[0].mac_tail[1]==0x01);
 CHECK(labels.items[1].model==2&&labels.items[1].mac_tail[0]==0x0b&&labels.items[1].mac_tail[1]==0x11&&strlen(labels.items[1].name)<=23&&!strncmp(labels.items[1].name,"Okno ",5));
 {size_t len=strlen(labels.items[1].name);CHECK(len&&((unsigned char)labels.items[1].name[len-1]&0xC0)!=0xC0);} /* never ends mid-character */
 CHECK(!strcmp(dsr_blu_model_name(2),"BLU Door/Window")&&!strcmp(dsr_blu_model_name(8),"BLU TRV")&&!dsr_blu_model_name(5));
 j=cJSON_Parse(page1);CHECK(dsr_lan_parse_components(j,4,&blu,NULL,&next,&total)==DSR_OK&&blu==3&&next==5);cJSON_Delete(j); /* cid 230: outside 16 channels */
 const char *bad[]={"{\"components\":[],\"offset\":0,\"total\":3}","{\"components\":[{\"key\":\"bthomedevice:x\"}],\"offset\":0,\"total\":1}","{\"components\":[{\"key\":1}],\"offset\":0,\"total\":1}","{\"components\":[{\"key\":\"a\"},{\"key\":\"b\"}],\"offset\":0,\"total\":1}"};
 for(unsigned i=0;i<4;i++){uint16_t none=0;j=cJSON_Parse(bad[i]);CHECK(dsr_lan_parse_components(j,0,&none,NULL,&next,&total)==DSR_BAD_RESPONSE&&!none);cJSON_Delete(j);}
 j=cJSON_Parse(page1);CHECK(dsr_lan_parse_components(j,0,&blu,NULL,&next,&total)==DSR_BAD_RESPONSE);cJSON_Delete(j); /* offset not echoed */
 /* Inspection: GetStatus lists no BTHome device, GetComponents pages do. */
 const char *gw_id="{\"name\":null,\"id\":\"shellyblugwg3-aabbccddeeff\",\"mac\":\"AABBCCDDEEFF\",\"model\":\"S3GW-1DBT001\",\"gen\":3,\"fw_id\":\"x\",\"ver\":\"1.7.0\",\"app\":\"BluGatewayG3\",\"auth_en\":true}";
 step_t gw[]={{"/shelly",gw_id,200,false,false},{"/rpc/Shelly.GetStatus","{\"ble\":{},\"blugw\":{},\"sys\":{\"uptime\":5}}",200,false,true},
  {"/rpc/Shelly.GetComponents?dynamic_only=true&offset=0",page0,200,false,true},{"/rpc/Shelly.GetComponents?dynamic_only=true&offset=4",page1,200,false,true}};
 lan_mode=true;dsr_lan_device_t d={0};fixture(gw,1,HTTP_AUTH_TYPE_NONE);CHECK(dsr_http_lan_identity("192.168.1.10",&d)==DSR_OK);
 dsr_connection_t c=connection();dsr_lan_capabilities_t caps;fixture(gw,4,HTTP_AUTH_TYPE_DIGEST);dsr_blu_labels_t got;CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps,&got)==DSR_OK&&used==4&&caps.blu==3&&!caps.switches&&got.valid==3&&got.items[1].model==2);
 /* Odd metadata keeps the channel, drops the label. */
 {const char *odd="{\"components\":[{\"key\":\"bthomedevice:202\",\"config\":{\"id\":202,\"addr\":\"bad\",\"name\":\"a\\u0001b\"},\"attrs\":{\"model_id\":\"x\"}}],\"offset\":0,\"total\":1}";uint16_t b=0;dsr_blu_labels_t l={0};cJSON *o=cJSON_Parse(odd);CHECK(dsr_lan_parse_components(o,0,&b,&l,&next,&total)==DSR_OK&&b==4&&!l.valid&&!l.items[2].name[0]&&!l.items[2].model);cJSON_Delete(o);}
 /* A failing page keeps proven channels and never fails the inspection. */
 gw[3].status=500;fixture(gw,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps,NULL)==DSR_OK&&caps.blu==3);
 gw[2].body="{\"components\":7}";fixture(gw,3,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps,NULL)==DSR_OK&&used==3&&caps.blu==0);
 /* No Bluetooth in the status: no extra requests. */
 gw[1].body="{\"sys\":{\"uptime\":5}}";fixture(gw,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_lan_inspect(&d,&c.credentials,&caps,NULL)==DSR_OK&&used==2&&caps.blu==0);
 lan_mode=false;
 /* BLU TRV GetKnownObjects: two temperatures (idx 0, 1); the first is kept. */
 dsr_blu_object_t objects[8];unsigned n;
 j=cJSON_Parse("{\"id\":200,\"objects\":[{\"obj_id\":1,\"idx\":0,\"component\":\"bthomesensor:200\"},{\"obj_id\":58,\"idx\":0,\"component\":\"bthomesensor:201\"},{\"obj_id\":69,\"idx\":0,\"component\":\"bthomesensor:202\"},{\"obj_id\":69,\"idx\":1,\"component\":\"bthomesensor:203\"}]}");
 CHECK(dsr_blu_parse_known(j,200,objects,8,&n)==DSR_OK&&n==3&&objects[2].sensor==202&&objects[2].obj_id==69);cJSON_Delete(j);
 /* Button with no value yet, sensor never heard: no data, no error. */
 dsr_telemetry_t t={0};j=cJSON_Parse("{\"id\":201,\"last_updated_ts\":1070199072}");CHECK(dsr_blu_apply_sensor(j,201,0x3A,&t)==DSR_OK&&!t.blu.flags);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":201,\"rssi\":null,\"battery\":null,\"packet_id\":null,\"last_updated_ts\":null,\"key\":false,\"paired\":true,\"rpc\":false,\"rsv\":-1,\"fw_ver\":null}");
 CHECK(dsr_blu_parse_device(j,201,&t)==DSR_OK&&!t.has_blu_battery&&!(t.blu.flags&DSR_BLU_HAS_RSSI)&&!t.blu_updated_s);cJSON_Delete(j);
 admission_open();
}

/* BLU TRV through BluTrv (docs: BLU Gateway Gen3; shape seen on the owner's gateway). */
static void test_blu_trv(void)
{
 dsr_connection_t c=connection();c.device.generation=DSR_GEN2_PLUS;c.device.endpoint_kind=DSR_ENDPOINT_BLU;c.device.channel=0;strcpy(c.device.device_id,"shellyblugwg3-aabbccddeeff_aabbcc00b567");
 static const char *gw="{\"gen\":3,\"id\":\"shellyblugwg3-aabbccddeeff\",\"model\":\"S3GW-1DBT001\"}";
 static const char *known="{\"id\":200,\"objects\":[{\"obj_id\":1,\"idx\":0,\"component\":\"bthomesensor:200\"},{\"obj_id\":58,\"idx\":0,\"component\":\"bthomesensor:201\"},{\"obj_id\":69,\"idx\":0,\"component\":\"bthomesensor:202\"},{\"obj_id\":69,\"idx\":1,\"component\":\"bthomesensor:203\"}]}";
 step_t read[]={{"/rpc/BTHomeDevice.GetStatus?id=200","{\"id\":200,\"rssi\":-51,\"battery\":100,\"packet_id\":189,\"last_updated_ts\":1792272402,\"key\":false,\"paired\":true,\"rpc\":true,\"rsv\":50,\"fw_ver\":\"v1.3.2\"}",200,false,true},
  {"/rpc/BTHomeDevice.GetKnownObjects?id=200",known,200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=200","{\"id\":200,\"value\":100,\"last_updated_ts\":1792272402}",200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=201","{\"id\":201,\"last_updated_ts\":1070199072}",200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=202","{\"id\":202,\"value\":20.0,\"last_updated_ts\":1792272402}",200,false,true}};
 step_t ident[]={{"/shelly",gw,200,false,false},{"/rpc/BTHomeDevice.GetConfig?id=200","{\"id\":200,\"addr\":\"aa:bb:cc:00:b5:67\",\"name\":null,\"meta\":null}",200,false,true}};
 step_t linked={"/rpc/BluTrv.GetConfig?id=200","{\"id\":200,\"addr\":\"aa:bb:cc:00:b5:67\",\"name\":null,\"trv\":\"bthomedevice:200\",\"temp_sensors\":[],\"dw_sensors\":[\"bthomesensor:206\"],\"override_delay\":30,\"meta\":null}",200,false,true};
 step_t trv_status={"/rpc/BluTrv.GetStatus?id=200","{\"id\":200,\"target_C\":20.0,\"current_C\":21.5,\"pos\":28,\"connected\":true,\"rssi\":-51,\"battery\":100,\"packet_id\":212,\"last_updated_ts\":1792274671,\"key\":false,\"paired\":true,\"rpc\":true,\"rsv\":4,\"fw_ver\":\"v1.3.2\"}",200,false,true};
 step_t s[24];unsigned n=0;
 memcpy(s+n,ident,sizeof(ident));n+=2;memcpy(s+n,read,sizeof(read));n+=5;s[n++]=linked;s[n++]=trv_status;
 dsr_device_t d;dsr_telemetry_t t;fixture(s,n,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_OK&&used==9&&(t.blu.errors&DSR_BLU_TRV)&&(t.blu.errors&DSR_BLU_TRV_TARGET)&&(t.blu.errors&DSR_BLU_TRV_CHECKED)&&!(t.blu.errors&DSR_BLU_TRV_FAULT)&&!(t.blu.errors&DSR_BLU_ERROR_MASK));
 c.telemetry=t; /* later calls carry what the first one learned */
 /* A previously identified TRV provides the displayed readings in one typed
  * status; do not rediscover BTHome objects for each active-screen refresh. */
 step_t quick[]={ident[0],ident[1],trv_status};dsr_connection_t quick_connection=c;
 fixture(quick,3,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_refresh(&quick_connection)==DSR_OK&&used==3&&quick_connection.telemetry.trv_target_c==20.0f&&quick_connection.telemetry.has_blu_battery&&quick_connection.telemetry.blu_battery_pct==100);
 CHECK(t.trv_target_c==20.0f&&t.trv_current_c==21.5f&&t.blu.trv_pos==28&&!t.has_illuminance&&!(t.blu.flags&DSR_BLU_HAS_HUMIDITY)&&t.has_blu_battery);
 /* Known TRV: identity and one complete typed status, no object rediscovery. */
 n=0;memcpy(s+n,ident,sizeof(ident));n+=2;s[n++]=trv_status;
 fixture(s,n,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_OK&&used==3&&t.trv_current_c==21.5f);
 /* SetTarget 21.5: identity, read, POST BluTrv.Call (null ack), read back. */
 step_t after=trv_status;after.body="{\"id\":200,\"target_C\":21.5,\"current_C\":21.5,\"pos\":40,\"errors\":[\"not_calibrated\"]}";
 n=0;memcpy(s+n,ident,sizeof(ident));n+=2;s[n++]=trv_status;
 s[n++]=(step_t){"/rpc/BluTrv.Call","null",200,true,true};s[n++]=after;
 const char *set215="{\"id\":200,\"method\":\"TRV.SetTarget\",\"params\":{\"id\":0,\"target_C\":21.5}}";cover_payload=set215;
 dsr_connection_t w=c;posted[0]=0;fixture(s,n,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=43})==DSR_OK&&used==n&&w.device.online);
 CHECK(!strcmp(posted,"{\"id\":200,\"method\":\"TRV.SetTarget\",\"params\":{\"id\":0,\"target_C\":21.5}}"));
 CHECK(w.telemetry.trv_target_c==21.5f&&w.telemetry.blu.trv_pos==40&&(w.telemetry.blu.errors&DSR_BLU_TRV_FAULT));
 CHECK(!w.telemetry.has_blu_battery&&!w.telemetry.blu_updated_s); /* omitted fields are unknown */
 /* Accepted SetTarget, first GET lost: one GET-only recovery, no second POST. */
 s[4]=trv_status;s[4].body=NULL;s[5]=after;
 w=c;fixture(s,6,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=43})==DSR_OK&&used==6&&w.telemetry.trv_target_c==21.5f);
 s[5].body=NULL;w=c;fixture(s,6,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=43})==DSR_OFFLINE&&used==6&&!w.device.online);
 const unsigned terminal[]={401,429,200};const dsr_result_t errors[]={DSR_AUTH_REQUIRED,DSR_RATE_LIMITED,DSR_BAD_RESPONSE};
 for(unsigned k=0;k<3;k++){s[4]=trv_status;s[4].status=terminal[k];s[4].body="{}";w=c;fixture(s,5,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=43})==errors[k]&&used==5);}
 /* Lower bound 4.0 C is written with one decimal. */
 n=0;memcpy(s+n,ident,sizeof(ident));n+=2;s[n++]=trv_status;s[n++]=(step_t){"/rpc/BluTrv.Call","null",200,true,true};s[n++]=trv_status;
 cover_payload="{\"id\":200,\"method\":\"TRV.SetTarget\",\"params\":{\"id\":0,\"target_C\":4.0}}";w=c;fixture(s,n,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=8})==DSR_OK&&strstr(posted,"\"target_C\":4.0}"));
 /* Out of range or unknown command: refused before any request. */
 w=c;fixture(s,0,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=61})==DSR_INVALID&&used==0);
 CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=2,.half_c=40})==DSR_INVALID&&used==0);
 /* An uncertain write is never replayed. */
 n=0;memcpy(s+n,ident,sizeof(ident));n+=2;s[n++]=trv_status;s[n++]=(step_t){"/rpc/BluTrv.Call","{\"code\":-1}",500,true,true};
 cover_payload=set215;w=c;fixture(s,n,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=43})==DSR_BAD_RESPONSE&&used==n&&!w.device.online);
 /* Not a TRV: GetConfig error remembered; a TRV command is unsupported after the read. */
 c.telemetry=(dsr_telemetry_t){0};step_t none={"/rpc/BluTrv.GetConfig?id=200","{\"code\":-105}",404,false,true};
 n=0;memcpy(s+n,ident,sizeof(ident));n+=2;memcpy(s+n,read,sizeof(read));n+=5;s[n++]=none;
 w=c;fixture(s,n,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv(&w,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=43})==DSR_UNSUPPORTED&&used==n&&!strstr(posted,"200,\"method\":\"TRV.SetTarget\",\"params\":{\"id\":0,\"target_C\":21.5}}x"));
 cover_payload=NULL;
 /* Linked to another device: not this channel's TRV. */
 cJSON *j=cJSON_Parse("{\"id\":200,\"trv\":\"bthomedevice:201\"}");CHECK(!dsr_blu_trv_linked(j,200,200));cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"target_C\":\"20\"}");dsr_telemetry_t bad={0};CHECK(dsr_blu_parse_trv(j,200,&bad)==DSR_BAD_RESPONSE);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":200,\"target_C\":null,\"current_C\":null,\"pos\":null}");CHECK(dsr_blu_parse_trv(j,200,&bad)==DSR_OK&&(bad.blu.errors&DSR_BLU_TRV)&&!(bad.blu.errors&DSR_BLU_TRV_TARGET));cJSON_Delete(j);
 /* Fleet: TRV target needs a fresh TRV sample. */
 dsr_fleet_t f={0};dsr_connection_t fc=c;fc.layout_id=92;fc.device.online=true;CHECK(dsr_fleet_upsert(&f,&fc,100)==DSR_OK);
 dsr_trv_intent_t i={.command=DSR_TRV_SET_TARGET,.half_c=43};CHECK(dsr_fleet_trv_request(&f,0,i,true,200)==DSR_UNSUPPORTED); /* no TRV proven yet */
 f.slots[0].connection.telemetry.blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET;f.slots[0].connection.telemetry.blu.flags=DSR_BLU_VALID;
 CHECK(dsr_fleet_trv_request(&f,0,i,true,100+DSR_FLEET_BLU_FRESH_MS+1)==DSR_OFFLINE);CHECK(dsr_fleet_trv_request(&f,0,i,false,200)==DSR_OFFLINE);
 CHECK(dsr_fleet_trv_request(&f,0,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=7},true,200)==DSR_INVALID);
 CHECK(dsr_fleet_trv_request(&f,0,i,true,200)==DSR_OK);CHECK(dsr_fleet_trv_request(&f,0,i,true,200)==DSR_BUSY);
 dsr_job_t job;unsigned at;CHECK(dsr_fleet_next(&f,200,true,&job,&at)&&job.kind==DSR_JOB_TRV&&job.trv.half_c==43);
 job.id=901;f.slots[at].trv.half_c=40;dsr_fleet_started(&f,at,&job);CHECK(f.slots[at].trv.half_c==43); /* actor jobs also own their current target */
 dsr_event_t ev={.id=901,.device=f.slots[0].connection.device,.result=DSR_BAD_RESPONSE};CHECK(dsr_fleet_complete(&f,at,&ev,300)&&f.slots[0].command_error);
 /* An uncertain write gets one early GET, not the ordinary 30 s failure
  * delay and never another TRV.SetTarget. Failed recovery restores backoff. */
 fc.telemetry.blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET;fc.telemetry.blu.flags=DSR_BLU_VALID;
 CHECK(dsr_fleet_upsert(&f,&fc,400)==DSR_OK&&dsr_fleet_trv_request(&f,0,i,true,400)==DSR_OK);
 CHECK(dsr_fleet_next(&f,400,false,&job,&at)&&job.kind==DSR_JOB_TRV);job.id=903;dsr_fleet_started(&f,at,&job);
 ev=(dsr_event_t){.id=903,.device=fc.device,.result=DSR_OFFLINE,.command_phase=DSR_COMMAND_WRITE};
 CHECK(dsr_fleet_complete(&f,at,&ev,401)&&f.slots[0].command_phase==DSR_COMMAND_WRITE&&f.slots[0].next_poll==1401&&!f.slots[0].connection.device.online);
 CHECK(dsr_fleet_next_poll(&f,1401,1,&job,&at)&&job.kind==DSR_JOB_PROBE);job.id=904;dsr_fleet_started(&f,at,&job);
 ev.id=904;ev.command_phase=DSR_COMMAND_NONE;CHECK(dsr_fleet_complete(&f,at,&ev,1402)&&f.slots[0].command_phase==DSR_COMMAND_NONE&&f.slots[0].next_poll>2402);
 /* A plain refresh after a TRV request never turns into a TRV write. */
 f.slots[0].connection.device.online=true;f.slots[0].sample_valid=true;f.slots[0].sample_ms=300;f.slots[0].command_error=false;f.slots[0].blocked=false;
 CHECK(dsr_fleet_ht_refresh(&f,0,true,300)==DSR_OK);CHECK(dsr_fleet_next(&f,300,true,&job,&at)&&job.kind==DSR_JOB_PROBE);job.id=902;dsr_fleet_started(&f,at,&job); /* releases the queued activity */
 admission_open();
}
