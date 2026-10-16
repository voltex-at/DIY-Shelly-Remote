/* Genuine Light protocol/NVS/fleet. SDK HTTP/NVS/radio remain MODEL. */
static const char light_ready[]="{\"id\":0,\"output\":true,\"brightness\":38,\"apower\":0,\"voltage\":230,\"current\":0,\"aenergy\":{\"total\":0}}";
static dsr_connection_t light_connection(void)
{dsr_connection_t c=connection();c.device.generation=DSR_GEN2_PLUS;c.device.endpoint_kind=DSR_ENDPOINT_LIGHT;strcpy(c.device.device_id,"shellyplus1-AABBCCDDEEFF");return c;}
static void test_light_protocol(void)
{
 dsr_connection_t c=light_connection();dsr_device_t d;dsr_telemetry_t telemetry;
 step_t data[]={{"/shelly",gen2,200,false,false},{"/rpc/Light.GetStatus?id=0",light_ready,200,false,true},{"/rpc/Light.Set","null",200,true,false},{"/rpc/Light.GetStatus?id=0",light_ready,200,false,false}};
 fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&telemetry)==DSR_OK&&used==2&&d.online&&!d.has_switch&&d.output&&d.endpoint_kind==DSR_ENDPOINT_LIGHT&&telemetry.light.brightness==38&&d.has_power&&d.power_w==0&&telemetry.has_energy&&telemetry.energy_wh==0);
 data[1].body="{\"id\":0,\"output\":true,\"brightness\":0,\"apower\":null,\"voltage\":null,\"current\":null,\"aenergy\":null}";fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&telemetry)==DSR_OK&&d.output&&!d.has_power&&!telemetry.has_voltage&&!telemetry.has_current&&!telemetry.has_energy&&telemetry.light.brightness==0);data[1].body=light_ready;
 for(unsigned value=0;value<=100;value++){
  char body[64],readback[96];snprintf(body,sizeof(body),"{\"id\":0,\"brightness\":%u}",value);snprintf(readback,sizeof(readback),"{\"id\":0,\"output\":false,\"brightness\":%u}",value);cover_payload=body;data[3].body=readback;
  fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_SET_BRIGHTNESS,.brightness=(uint8_t)value})==DSR_OK&&used==4&&c.device.online&&!c.device.output&&c.telemetry.light.brightness==value);
 }
 for(unsigned on=0;on<2;on++){
  cover_payload=on?"{\"id\":0,\"on\":true}":"{\"id\":0,\"on\":false}";
  data[3].body=on?"{\"id\":0,\"output\":true,\"brightness\":0}":"{\"id\":0,\"output\":false,\"brightness\":67}";
  fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=on?DSR_LIGHT_ON:DSR_LIGHT_OFF})==DSR_OK&&used==4&&c.device.output==(bool)on); /* 0 brightness is not OFF. */
  data[3].body=on?"{\"id\":0,\"output\":false,\"brightness\":38,\"transition\":{\"target\":{\"output\":true,\"brightness\":50}}}":"{\"id\":0,\"output\":true,\"brightness\":38,\"transition\":{\"target\":{\"output\":false,\"brightness\":0}}}";
  fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=on?DSR_LIGHT_ON:DSR_LIGHT_OFF})==DSR_OK&&used==4&&c.device.output!=(bool)on&&(c.telemetry.light.flags&DSR_LIGHT_TRANSITION)); /* Correct target; current remains actual readback. */
 }
 cover_payload="{\"id\":0,\"brightness\":67}";
 data[3].body="{\"id\":0,\"output\":true,\"brightness\":38,\"transition\":{\"target\":{\"output\":true,\"brightness\":67},\"duration\":3,\"started_at\":123}}";
 fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_SET_BRIGHTNESS,.brightness=67})==DSR_OK&&used==4&&c.telemetry.light.brightness==38&&c.telemetry.light.target==67&&(c.telemetry.light.flags&DSR_LIGHT_TRANSITION));
 data[3].body=light_ready;fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_SET_BRIGHTNESS,.brightness=67})==DSR_BAD_RESPONSE&&used==4&&!c.device.online);
 for(unsigned failure=0;failure<4;failure++){
  data[2].body=failure==0?NULL:failure==1?"{}":failure==2?"{\"error\":{\"code\":-114}}":"null";data[2].status=failure==3?401:failure==0?0:200;
  fixture(data,3,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_SET_BRIGHTNESS,.brightness=67})!=DSR_OK&&used==3&&!c.device.online);
 }
 data[2].body="null";data[2].status=200;
 const char *malformed[]={"{\"id\":1,\"output\":true,\"brightness\":38}","{\"id\":0,\"output\":true}","{\"id\":0,\"output\":true,\"brightness\":null}","{\"id\":0,\"output\":true,\"brightness\":101}","{\"id\":0,\"output\":true,\"brightness\":-1}","{\"id\":0,\"output\":true,\"brightness\":38.5}","{\"id\":0,\"output\":true,\"brightness\":\"38\"}","{\"id\":0,\"output\":1,\"brightness\":38}","{\"id\":0,\"output\":true,\"brightness\":38,\"brightness\":39}","{\"id\":0,\"output\":true,\"brightness\":38,\"transition\":{\"target\":{\"output\":true,\"brightness\":101}}}","{\"id\":0,\"output\":true,\"brightness\":38,\"errors\":null}","{\"id\":0,\"output\":true,\"brightness\":38,\"flags\":[1]}"};
 for(unsigned i=0;i<sizeof(malformed)/sizeof(*malformed);i++){data[1].body=malformed[i];fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_ON})!=DSR_OK&&used==2);}
 const char *conditions[]={"\"errors\":[\"overtemp\"]","\"errors\":[\"unknown_future_error\"]","\"flags\":[\"uncalibrated\"]","\"flags\":[\"no_load\"]","\"flags\":[\"future_flag\"]","\"calibration\":{\"progess\":50}"};
 for(unsigned i=0;i<sizeof(conditions)/sizeof(*conditions);i++){char body[160];snprintf(body,sizeof(body),"{\"id\":0,\"output\":true,\"brightness\":38,%s}",conditions[i]);data[1].body=body;fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_ON})==DSR_UNSUPPORTED&&used==2);}
 data[0].body="{\"gen\":2,\"id\":\"foreign\"}";fixture(data,1,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_OFF})==DSR_IDENTITY_MISMATCH&&used==1);
 c.device.endpoint_kind=DSR_ENDPOINT_SWITCH;fixture(data,0,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_ON})==DSR_INVALID&&used==0);
 c=light_connection();c.device.generation=DSR_GEN1;CHECK(dsr_http_light(&c,(dsr_light_intent_t){.command=DSR_LIGHT_ON})==DSR_INVALID&&used==0);
 cover_payload=NULL;admission_open();
}
static void test_light_opposite_transition(void)
{
 unsigned wrong=0;
 for(unsigned on=0;on<2;on++){
  dsr_connection_t c=light_connection();
  step_t data[]={{"/shelly",gen2,200,false,false},{"/rpc/Light.GetStatus?id=0",light_ready,200,false,false},
   {"/rpc/Light.Set","null",200,true,false},{"/rpc/Light.GetStatus?id=0",NULL,200,false,false}};
  char readback[192];snprintf(readback,sizeof(readback),"{\"id\":0,\"output\":%s,\"brightness\":38,\"transition\":{\"target\":{\"output\":%s,\"brightness\":38},\"duration\":3,\"started_at\":123}}",on?"true":"false",on?"false":"true");
  data[3].body=readback;cover_payload=on?"{\"id\":0,\"on\":true}":"{\"id\":0,\"on\":false}";
  fixture(data,4,HTTP_AUTH_TYPE_DIGEST);dsr_result_t result=dsr_http_light(&c,(dsr_light_intent_t){.command=on?DSR_LIGHT_ON:DSR_LIGHT_OFF});
  CHECK(used==4);fprintf(stderr,"Light.%s ACK then current matches but transition.target.output opposes: result=%u online=%u\n",on?"ON":"OFF",result,c.device.online);
  if(result==DSR_OK||c.device.online)wrong++;
 }
 cover_payload=NULL;CHECK(wrong==0);
}
static void test_light_persistence_scheduler(void)
{
 memset(sizes,0,sizeof(sizes));memset(schemas,0,sizeof(schemas));
 dsr_connection_t c=light_connection();c.layout_id=43;CHECK(dsr_connection_save(&c)==ESP_OK&&sizes[2]==4104&&((uint32_t*)blobs[2])[0]==4);
 dsr_connection_t loaded[16];size_t count;unsigned writes=write_opens;
 CHECK(dsr_connections_load(loaded,16,&count)==ESP_OK&&count==1&&loaded[0].device.endpoint_kind==DSR_ENDPOINT_LIGHT&&loaded[0].layout_id==43&&!loaded[0].legacy_fallback&&write_opens==writes&&!loaded[0].device.online);
 ((uint32_t*)blobs[2])[0]=3;CHECK(dsr_connections_load(loaded,16,&count)!=ESP_OK&&!count&&dsr_connection_save(&c)!=ESP_OK&&write_opens==writes); /* v3 kind2 is corrupt, not an extension. */
 ((uint32_t*)blobs[2])[0]=5;CHECK(dsr_connections_load(loaded,16,&count)!=ESP_OK&&!count&&dsr_connection_remove(&c)!=ESP_OK&&write_opens==writes);
 ((uint32_t*)blobs[2])[0]=4;
 c.layout_id=0;CHECK(dsr_connection_remove(&c)==ESP_OK&&dsr_connection_save(&c)==ESP_OK);writes=write_opens;CHECK(dsr_connections_load(loaded,16,&count)==ESP_OK&&count==1&&!loaded[0].layout_id&&!loaded[0].legacy_fallback&&write_opens==writes);dsr_fleet_t quarantined={0};CHECK(dsr_fleet_upsert(&quarantined,&loaded[0],0)==DSR_OK&&!quarantined.slots[0].layout_enabled&&dsr_fleet_light_request(&quarantined,0,(dsr_light_intent_t){.command=DSR_LIGHT_ON},true,0)==DSR_IDENTITY_MISMATCH);c.layout_id=43;
 c.device.online=true;c.telemetry.light=(dsr_light_status_t){.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS,.brightness=38};dsr_fleet_t f={0};CHECK(dsr_fleet_upsert(&f,&c,0)==DSR_OK);
 CHECK(dsr_fleet_request(&f,0,true,true,0)==DSR_UNSUPPORTED&&dsr_fleet_cover_request(&f,0,(dsr_cover_intent_t){.command=DSR_COVER_OPEN},true,0)==DSR_UNSUPPORTED);
 CHECK(dsr_fleet_light_request(&f,0,(dsr_light_intent_t){.command=DSR_LIGHT_SET_BRIGHTNESS,.brightness=0},true,0)==DSR_OK);
 dsr_job_t job;unsigned index;CHECK(dsr_fleet_next(&f,0,false,&job,&index)&&index==0&&job.kind==DSR_JOB_LIGHT&&job.light.command==DSR_LIGHT_SET_BRIGHTNESS&&job.light.brightness==0);
 job.id=101;dsr_fleet_started(&f,0,&job);dsr_event_t e={.id=101,.device=c.device,.telemetry=c.telemetry,.result=DSR_OFFLINE};CHECK(dsr_fleet_complete(&f,0,&e,1));CHECK(!dsr_fleet_next(&f,2,false,&job,&index)); /* no SET replay */
 CHECK(dsr_fleet_upsert(&f,&c,3)==DSR_OK);CHECK(dsr_fleet_light_request(&f,0,(dsr_light_intent_t){.command=DSR_LIGHT_ON},true,3)==DSR_OK);f.slots[0].layout_enabled=false;CHECK(!dsr_fleet_next(&f,3,false,&job,&index)&&!f.slots[0].queued);
 CHECK(dsr_fleet_upsert(&f,&c,4)==DSR_OK);CHECK(dsr_fleet_light_request(&f,0,(dsr_light_intent_t){.command=DSR_LIGHT_ON},true,16000)==DSR_OFFLINE);admission_open();
}
