/* Genuine RGB parser/HTTP/NVS/fleet. SDK/Flash/radio remain MODEL. */
static const char rgb_identity[]="{\"gen\":2,\"id\":\"shellyplusrgbwpm-AABBCCDDEEFF\",\"model\":\"SNDC-0D4P10WW\",\"profile\":\"rgb\"}";
static const char rgb_ready[]="{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[12,34,56],\"apower\":0,\"voltage\":null,\"current\":0,\"aenergy\":{\"total\":0}}";
static dsr_connection_t rgb_connection(void)
{dsr_connection_t c=connection();c.device.generation=DSR_GEN2_PLUS;c.device.endpoint_kind=DSR_ENDPOINT_RGB;strcpy(c.device.device_id,"shellyplusrgbwpm-AABBCCDDEEFF");return c;}
static void test_rgb_protocol(void)
{
 dsr_connection_t c=rgb_connection();dsr_device_t d;dsr_telemetry_t t;
 step_t data[]={{"/shelly",rgb_identity,200,false,false},{"/rpc/RGB.GetStatus?id=0",rgb_ready,200,false,true},{"/rpc/RGB.Set","null",200,true,false},{"/rpc/RGB.GetStatus?id=0",rgb_ready,200,false,false}};
 fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_OK&&d.online&&!d.has_switch&&d.output&&t.rgb.r==12&&t.rgb.g==34&&t.rgb.b==56&&t.rgb.brightness==38&&!t.has_voltage&&t.has_current&&t.current_a==0&&t.has_energy&&t.energy_wh==0);
 const char *bad_identity[]={"{\"gen\":2,\"id\":\"shellyplusrgbwpm-AABBCCDDEEFF\",\"model\":\"SNDC-0D4P10WW\",\"profile\":\"rgbw\"}","{\"gen\":2,\"id\":\"shellyplusrgbwpm-AABBCCDDEEFF\",\"model\":\"SNDC-0D4P10WW\",\"profile\":\"light\"}","{\"gen\":2,\"id\":\"shellyplusrgbwpm-AABBCCDDEEFF\",\"model\":\"other\",\"profile\":\"rgb\"}","{\"gen\":2,\"id\":\"shellyplusrgbwpm-AABBCCDDEEFF\",\"model\":\"SNDC-0D4P10WW\"}","{\"gen\":2,\"id\":\"foreign\",\"model\":\"SNDC-0D4P10WW\",\"profile\":\"rgb\"}"};
 for(unsigned i=0;i<sizeof(bad_identity)/sizeof(*bad_identity);i++){data[0].body=bad_identity[i];fixture(data,1,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_rgb(&c,(dsr_rgb_intent_t){.command=DSR_RGB_ON})!=DSR_OK&&used==1);}
 data[0].body=rgb_identity;
 /* Full one-byte range in every color channel, including zeros. The only
  * payload is RGB.Set; color never changes on/off or brightness implicitly. */
 for(unsigned channel=0;channel<3;channel++)for(unsigned value=0;value<=255;value++){
  dsr_rgb_intent_t intent={.command=DSR_RGB_SET_COLOR,.rgb={12,34,56}};intent.rgb[channel]=(uint8_t)value;
  char payload[64],readback[120];snprintf(payload,sizeof(payload),"{\"id\":0,\"rgb\":[%u,%u,%u]}",intent.rgb[0],intent.rgb[1],intent.rgb[2]);snprintf(readback,sizeof(readback),"{\"id\":0,\"output\":false,\"brightness\":0,\"rgb\":[%u,%u,%u]}",intent.rgb[0],intent.rgb[1],intent.rgb[2]);cover_payload=payload;data[3].body=readback;
  fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_rgb(&c,intent)==DSR_OK&&used==4&&c.device.online&&!c.device.output&&c.telemetry.rgb.brightness==0&&c.telemetry.rgb.r==intent.rgb[0]&&c.telemetry.rgb.g==intent.rgb[1]&&c.telemetry.rgb.b==intent.rgb[2]);
 }
 for(unsigned value=0;value<=100;value++){
  char payload[64],readback[120];snprintf(payload,sizeof(payload),"{\"id\":0,\"brightness\":%u}",value);snprintf(readback,sizeof(readback),"{\"id\":0,\"output\":true,\"brightness\":%u,\"rgb\":[0,0,0]}",value);cover_payload=payload;data[3].body=readback;
  fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_rgb(&c,(dsr_rgb_intent_t){.command=DSR_RGB_SET_BRIGHTNESS,.brightness=(uint8_t)value})==DSR_OK&&used==4&&c.device.output&&c.telemetry.rgb.brightness==value); /* zero is not OFF */
 }
 /* Transition target always decides; current may be old or deceptively match. */
 for(unsigned command=DSR_RGB_ON;command<=DSR_RGB_SET_COLOR;command++)for(unsigned correct=0;correct<2;correct++){
  dsr_rgb_intent_t intent={.command=(uint8_t)command};if(command==DSR_RGB_SET_COLOR){intent.rgb[0]=12;intent.rgb[1]=34;intent.rgb[2]=56;}else if(command==DSR_RGB_SET_BRIGHTNESS)intent.brightness=38;
  dsr_request_t request;CHECK(dsr_rgb_request_build(0,intent,&request)==DSR_OK);cover_payload=request.body;
  bool target_on=command==DSR_RGB_ON?correct!=0:command==DSR_RGB_OFF?correct==0:true;
  char readback[256];snprintf(readback,sizeof(readback),"{\"id\":0,\"output\":%s,\"brightness\":38,\"rgb\":[12,34,56],\"transition\":{\"target\":{\"output\":%s,\"brightness\":%u,\"rgb\":[12,34,%u]}}}",command==DSR_RGB_OFF?"false":"true",target_on?"true":"false",command==DSR_RGB_SET_BRIGHTNESS&&!correct?39:38,command==DSR_RGB_SET_COLOR&&!correct?57:56);data[3].body=readback;
  fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_rgb(&c,intent)==(correct?DSR_OK:DSR_BAD_RESPONSE)&&used==4&&c.device.online==(bool)correct);if(correct)CHECK(c.telemetry.rgb.brightness==38&&c.telemetry.rgb.b==56&&(c.telemetry.rgb.flags&DSR_RGB_TRANSITION));
 }
 cover_payload="{\"id\":0,\"on\":true}";
 const char *bad_status[]={"null","{}","{\"id\":0,\"output\":true,\"brightness\":null,\"rgb\":[1,2,3]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":null}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,3,4]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,256]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,-1]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,1.5]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,true]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,3],\"rgb\":[1,2,3]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,3],\"transition\":null}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,3],\"transition\":{\"target\":{\"output\":true,\"brightness\":38}}}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,3],\"errors\":null}"};
 for(unsigned i=0;i<sizeof(bad_status)/sizeof(*bad_status);i++){data[1].body=bad_status[i];fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_rgb(&c,(dsr_rgb_intent_t){.command=DSR_RGB_ON})!=DSR_OK&&used==2&&!c.device.online);}
 const char *blocked[]={"{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,3],\"errors\":[\"future_fault\"]}","{\"id\":0,\"output\":true,\"brightness\":38,\"rgb\":[1,2,3],\"transition\":{\"target\":{\"output\":true,\"brightness\":38,\"rgb\":[1,2,3]}}}"};
 for(unsigned i=0;i<2;i++){data[1].body=blocked[i];fixture(data,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_rgb(&c,(dsr_rgb_intent_t){.command=DSR_RGB_ON})==DSR_UNSUPPORTED&&used==2);}
 data[1].body=rgb_ready;data[3].body=rgb_ready;cover_payload="{\"id\":0,\"on\":false}";
 for(unsigned failure=0;failure<5;failure++){data[2].body=failure==0?NULL:failure==1?"{}":"null";data[2].status=failure==2?401:failure==3?429:200;fixture(data,failure==4?4:3,HTTP_AUTH_TYPE_DIGEST);dsr_result_t result=dsr_http_rgb(&c,(dsr_rgb_intent_t){.command=DSR_RGB_OFF});CHECK(result!=DSR_OK&&used==(failure==4?4:3)&&!c.device.online);}
 cover_payload=NULL;admission_open();
}
static void test_rgb_capabilities(void)
{
 dsr_lan_capabilities_t caps;const char *good="{\"rgb:0\":{\"id\":0,\"output\":true,\"brightness\":0,\"rgb\":[0,255,17]},\"rgbw:0\":{\"id\":0}}";cJSON *r=cJSON_Parse(good);CHECK(dsr_lan_parse_capabilities(r,2,&caps)==DSR_OK&&caps.rgbs==1&&!caps.lights&&!caps.switches&&!caps.covers);cJSON_Delete(r);
 const char *bad[]={"{\"rgb:0\":{\"id\":0,\"output\":true,\"brightness\":0,\"rgb\":[0,255.5,17]}}","{\"rgb:0\":{\"id\":0,\"output\":true,\"brightness\":0,\"rgb\":null}}","{\"rgb:0\":{\"id\":1,\"output\":true,\"brightness\":0,\"rgb\":[0,255,17]}}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){r=cJSON_Parse(bad[i]);CHECK(dsr_lan_parse_capabilities(r,2,&caps)==DSR_BAD_RESPONSE);cJSON_Delete(r);}
 r=cJSON_Parse("{\"rgbw:0\":{\"id\":0,\"output\":true,\"brightness\":0,\"rgb\":[0,255,17],\"white\":8}}");CHECK(dsr_lan_parse_capabilities(r,2,&caps)==DSR_OK&&!caps.rgbs&&!caps.lights);cJSON_Delete(r);
}
static void test_rgb_registry_scheduler(void)
{
 memset(sizes,0,sizeof(sizes));memset(schemas,0,sizeof(schemas));dsr_connection_t c=rgb_connection();c.layout_id=44;
 CHECK(dsr_connection_save(&c)==ESP_OK&&sizes[2]==4104&&((uint32_t*)blobs[2])[0]==6);unsigned writes=write_opens;dsr_connection_t loaded[16];size_t count;
 CHECK(dsr_connections_load(loaded,16,&count)==ESP_OK&&count==1&&loaded[0].device.endpoint_kind==DSR_ENDPOINT_RGB&&!loaded[0].device.online&&write_opens==writes);
 for(unsigned version=3;version<=7;version++)if(version!=5&&version!=6){((uint32_t*)blobs[2])[0]=version;CHECK(dsr_connections_load(loaded,16,&count)!=ESP_OK&&!count&&dsr_connection_save(&c)!=ESP_OK&&write_opens==writes);}
 ((uint32_t*)blobs[2])[0]=5;c.layout_id=0;CHECK(dsr_connection_remove(&c)==ESP_OK&&dsr_connection_save(&c)==ESP_OK);CHECK(dsr_connections_load(loaded,16,&count)==ESP_OK&&count==1&&!loaded[0].legacy_fallback&&!loaded[0].layout_id);
 dsr_fleet_t f={0};CHECK(dsr_fleet_upsert(&f,&loaded[0],0)==DSR_OK&&!f.slots[0].layout_enabled&&dsr_fleet_rgb_request(&f,0,(dsr_rgb_intent_t){.command=DSR_RGB_ON},true,0)==DSR_IDENTITY_MISMATCH);
 c.layout_id=44;c.device.online=true;c.telemetry.rgb=(dsr_rgb_status_t){.r=12,.g=34,.b=56,.brightness=38,.flags=DSR_RGB_VALID};CHECK(dsr_fleet_upsert(&f,&c,1)==DSR_OK);
 CHECK(dsr_fleet_light_request(&f,0,(dsr_light_intent_t){.command=DSR_LIGHT_ON},true,1)==DSR_UNSUPPORTED&&dsr_fleet_request(&f,0,true,true,1)==DSR_UNSUPPORTED);
 CHECK(dsr_fleet_rgb_request(&f,0,(dsr_rgb_intent_t){.command=DSR_RGB_SET_COLOR,.rgb={255,17,0}},true,1)==DSR_OK);CHECK(dsr_fleet_rgb_request(&f,0,(dsr_rgb_intent_t){.command=DSR_RGB_OFF},true,1)==DSR_BUSY);
 dsr_job_t job;unsigned at;CHECK(dsr_fleet_next(&f,1,false,&job,&at)&&at==0&&job.kind==DSR_JOB_RGB&&job.rgb.command==DSR_RGB_SET_COLOR&&job.rgb.rgb[0]==255&&job.rgb.rgb[1]==17&&job.rgb.rgb[2]==0);job.id=303;dsr_fleet_started(&f,at,&job);dsr_event_t event={.id=303,.device=c.device,.result=DSR_OFFLINE};CHECK(dsr_fleet_complete(&f,at,&event,2)&&!dsr_fleet_next(&f,2,false,&job,&at));admission_open();
 CHECK(dsr_fleet_upsert(&f,&c,3)==DSR_OK&&dsr_fleet_rgb_request(&f,0,(dsr_rgb_intent_t){.command=DSR_RGB_ON},true,16004)==DSR_OFFLINE);
 c.device.channel=1;CHECK(!dsr_connection_valid(&c,true)&&dsr_connection_save(&c)==ESP_ERR_INVALID_ARG);admission_open();
 CHECK(sizeof(dsr_job_t)==560&&sizeof(dsr_event_t)==1192&&sizeof(dsr_connection_t)==296&&sizeof(dsr_telemetry_t)==32&&sizeof(dsr_fleet_t)==8080);
}
