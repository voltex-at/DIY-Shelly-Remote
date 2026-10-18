/* Genuine protocol/persistence/fleet; HTTP, NVS and radio are explicit MODEL. */
static const char cover_ready[]="{\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":38,\"target_pos\":null,\"apower\":3.2,\"voltage\":230,\"current\":0.014}";
static unsigned urgent_calls,urgent_trigger;
static bool cover_interrupt(void){return ++urgent_calls==urgent_trigger;}
static void test_cover_protocol(void)
{
 dsr_connection_t c=connection();c.device.generation=DSR_GEN2_PLUS;c.device.endpoint_kind=DSR_ENDPOINT_COVER;strcpy(c.device.device_id,"shellyplus1-AABBCCDDEEFF");
 dsr_device_t d;dsr_telemetry_t t;
 step_t probe[]={{"/shelly",gen2,200,false,false},{"/rpc/Cover.GetStatus?id=0",cover_ready,200,false,true}};
 fixture(probe,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_probe_telemetry(&c,&d,&t)==DSR_OK&&used==2&&d.online&&!d.has_switch&&d.endpoint_kind==DSR_ENDPOINT_COVER&&d.has_power&&d.power_w==3.2&&t.cover.current_pos==38&&t.cover.target_pos==0&&(t.cover.flags&DSR_COVER_POSITION));
 const char *paths[]={"/rpc/Cover.Open","/rpc/Cover.Close","/rpc/Cover.Stop","/rpc/Cover.GoToPosition"};
 for(unsigned command=DSR_COVER_OPEN;command<=DSR_COVER_GOTO;command++){
  dsr_cover_intent_t intent={.command=(uint8_t)command,.position=command==DSR_COVER_GOTO?67:0};cover_payload=command==DSR_COVER_GOTO?"{\"id\":0,\"pos\":67}":"{\"id\":0}";
  step_t move[]={{"/shelly",gen2,200,false,false},{"/rpc/Cover.GetStatus?id=0",cover_ready,200,false,false},{paths[command-1],"null",200,true,false},{"/rpc/Cover.GetStatus?id=0",cover_ready,200,false,false}};
  if(command==DSR_COVER_STOP){move[1]=move[2];move[2]=move[3];}
  unsigned length=command==DSR_COVER_STOP?3:4;
  fixture(move,length,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_cover(&c,intent)==DSR_OK&&used==length&&c.device.online&&c.telemetry.cover.current_pos==38); /* ACK never invents 67%. */
  unsigned post=command==DSR_COVER_STOP?1:2;move[post].body=NULL;move[post].status=0;fixture(move,post+1,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_cover(&c,intent)==DSR_OFFLINE&&used==post+1&&!c.device.online); /* no replay/readback on lost ACK */
  move[post].body="{}";move[post].status=200;fixture(move,post+1,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_cover(&c,intent)==DSR_BAD_RESPONSE&&used==post+1&&!c.device.online);
 }
 cover_payload=NULL;
 for(unsigned boundary=1;boundary<=3;boundary++){
  cover_payload="{\"id\":0,\"pos\":67}";step_t move[]={{"/shelly",gen2,200,false,false},{"/rpc/Cover.GetStatus?id=0",cover_ready,200,false,false},{"/rpc/Cover.GoToPosition","null",200,true,false}};
  urgent_calls=0;urgent_trigger=boundary;dsr_http_urgent_hook(cover_interrupt);fixture(move,boundary,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_cover(&c,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=67})==(boundary<3?DSR_BUSY:DSR_BAD_RESPONSE)&&used==boundary);dsr_http_urgent_hook(NULL);
 }
 cover_payload="{\"id\":0}";step_t stop_cal[]={{"/shelly",gen2,200,false,false},{"/rpc/Cover.Stop","null",200,true,false},{"/rpc/Cover.GetStatus?id=0","{\"id\":0,\"state\":\"calibrating\",\"pos_control\":false,\"current_pos\":null}",200,false,false}};
 fixture(stop_cal,3,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_cover(&c,(dsr_cover_intent_t){.command=DSR_COVER_STOP})==DSR_OK&&used==3&&c.telemetry.cover.state==DSR_COVER_STATE_CALIBRATING&&!(c.telemetry.cover.flags&DSR_COVER_POSITION));cover_payload=NULL;
 const char *blocked[]={"{\"id\":1,\"state\":\"stopped\",\"pos_control\":true}","{\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":101}","{\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":38.5}","{\"id\":0,\"state\":\"calibrating\",\"pos_control\":true,\"current_pos\":38}","{\"id\":0,\"state\":\"stopped\",\"pos_control\":false,\"current_pos\":null}","{\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":38,\"errors\":[\"obstruction\"]}","{\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":38,\"errors\":[\"future_error\"]}","{\"id\":0,\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":38}"};
 for(unsigned i=0;i<sizeof(blocked)/sizeof(*blocked);i++){probe[1].body=blocked[i];fixture(probe,2,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_cover(&c,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=50})!=DSR_OK&&used==2);}
 probe[0].body="{\"gen\":2,\"id\":\"foreign\"}";fixture(probe,1,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_cover(&c,(dsr_cover_intent_t){.command=DSR_COVER_STOP})==DSR_IDENTITY_MISMATCH&&used==1);
 c.device.generation=DSR_GEN1;CHECK(dsr_http_cover(&c,(dsr_cover_intent_t){.command=DSR_COVER_STOP})==DSR_INVALID);c.device.generation=DSR_GEN2_PLUS;
 c.device.endpoint_kind=DSR_ENDPOINT_SWITCH;CHECK(dsr_http_cover(&c,(dsr_cover_intent_t){.command=DSR_COVER_STOP})==DSR_INVALID);
}
static void test_cover_persistence(void)
{
 memset(sizes,0,sizeof(sizes));memset(schemas,0,sizeof(schemas));
 /* Independent installed bytes: kind's new byte137 was arbitrary padding. */
 unsigned char legacy[256]={0};memcpy(legacy,"192.168.1.10",13);memcpy(legacy+64,"installed",10);uint32_t generation=2;memcpy(legacy+128,&generation,4);legacy[137]=0xa5;memcpy(legacy+152,"admin",6);memcpy(legacy+185,"legacy-private",15);
 memcpy(blobs[1],legacy,sizeof(legacy));sizes[1]=sizeof(legacy);schemas[1]=1;
 dsr_connection_t loaded[16];size_t n;CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==1&&loaded[0].device.endpoint_kind==DSR_ENDPOINT_SWITCH&&loaded[0].legacy_fallback&&!sizes[2]);
 /* Exact v2 4104-byte fixture, including poisoned unused runtime flags. */
 memset(blobs[2],0,4104);uint32_t schema=2,one=1,id=42;memcpy(blobs[2],&schema,4);memcpy(blobs[2]+4,&one,4);memcpy(blobs[2]+8,legacy,256);memcpy(blobs[2]+8+252,&id,4);sizes[2]=4104;
 CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==1&&!loaded[0].legacy_fallback&&loaded[0].device.endpoint_kind==DSR_ENDPOINT_SWITCH&&loaded[0].layout_id==42&&((uint32_t*)blobs[2])[0]==2);
 dsr_connection_t cover=loaded[0];cover.device.endpoint_kind=DSR_ENDPOINT_COVER;cover.layout_id=43;CHECK(dsr_connection_save(&cover)==ESP_OK&&sizes[2]==4104&&((uint32_t*)blobs[2])[0]==6);
 CHECK(dsr_connections_load(loaded,16,&n)==ESP_OK&&n==2&&loaded[0].device.endpoint_kind==DSR_ENDPOINT_SWITCH&&loaded[1].device.endpoint_kind==DSR_ENDPOINT_COVER&&loaded[0].layout_id==42&&loaded[1].layout_id==43);
 unsigned before=write_opens;((uint32_t*)blobs[2])[0]=8;CHECK(dsr_connections_load(loaded,16,&n)!=ESP_OK&&!n&&dsr_connection_save(&cover)!=ESP_OK&&write_opens==before);((uint32_t*)blobs[2])[0]=6;
 /* BLU (kind5): schema 7 only while a BLU entry exists; schema 6 cannot hold kind5. */
 dsr_connection_t blu=loaded[0];blu.device.endpoint_kind=DSR_ENDPOINT_BLU;blu.device.channel=3;strcpy(blu.device.device_id,"shellyplus1pm-aabbccddeeff_3c2ef571d52a");blu.layout_id=44;
 CHECK(dsr_connection_save(&blu)==ESP_OK&&((uint32_t*)blobs[2])[0]==7&&dsr_connections_load(loaded,16,&n)==ESP_OK&&n==3&&loaded[2].device.endpoint_kind==DSR_ENDPOINT_BLU&&loaded[2].device.channel==3);
 ((uint32_t*)blobs[2])[0]=6;CHECK(dsr_connections_load(loaded,16,&n)!=ESP_OK&&!n);((uint32_t*)blobs[2])[0]=7;
 CHECK(dsr_connection_remove(&blu)==ESP_OK&&((uint32_t*)blobs[2])[0]==6&&dsr_connections_load(loaded,16,&n)==ESP_OK&&n==2);((uint32_t*)blobs[2])[0]=3;
 CHECK(dsr_connection_remove(&cover)==ESP_OK&&dsr_connections_load(loaded,16,&n)==ESP_OK&&n==1&&loaded[0].device.endpoint_kind==DSR_ENDPOINT_SWITCH);CHECK(!memcmp(blobs[1],legacy,256));admission_open();
}
static void test_cover_scheduler(void)
{
 dsr_fleet_t f={0};dsr_connection_t c=connection();c.device.generation=DSR_GEN2_PLUS;c.device.online=true;c.device.has_switch=true;c.layout_id=41;strcpy(c.device.device_id,"same-physical");CHECK(dsr_fleet_upsert(&f,&c,0)==DSR_OK);
 c.device.endpoint_kind=DSR_ENDPOINT_COVER;c.device.has_switch=false;c.layout_id=42;c.telemetry.cover=(dsr_cover_status_t){.state=DSR_COVER_STATE_STOPPED,.flags=DSR_COVER_VALID|DSR_COVER_POSITION|DSR_COVER_POSITION_CONTROL,.current_pos=30};CHECK(dsr_fleet_upsert(&f,&c,0)==DSR_OK&&f.count==2);
 CHECK(dsr_fleet_request(&f,1,true,true,0)==DSR_UNSUPPORTED&&dsr_fleet_cover_request(&f,0,(dsr_cover_intent_t){.command=DSR_COVER_OPEN},true,0)==DSR_UNSUPPORTED);
 CHECK(dsr_fleet_cover_request(&f,1,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=61},true,0)==DSR_OK);dsr_job_t job;unsigned index;
 CHECK(dsr_fleet_next(&f,0,false,&job,&index)&&index==1&&job.kind==DSR_JOB_COVER&&job.cover.position==61);job.id=77;dsr_fleet_started(&f,index,&job);
 dsr_event_t e={.id=77,.device=c.device,.result=DSR_OK,.telemetry=c.telemetry};e.device.endpoint_kind=DSR_ENDPOINT_SWITCH;CHECK(dsr_fleet_complete(&f,index,&e,1)&&f.slots[1].blocked);CHECK(!f.slots[1].connection.device.online);admission_open();
 CHECK(dsr_fleet_upsert(&f,&c,2)==DSR_OK);CHECK(dsr_fleet_cover_request(&f,1,(dsr_cover_intent_t){.command=DSR_COVER_CLOSE},true,16000)==DSR_OFFLINE);
 CHECK(dsr_fleet_cover_request(&f,1,(dsr_cover_intent_t){.command=DSR_COVER_STOP},true,2)==DSR_INVALID); /* only independent urgent lane */
 /* Lost command ACK/readback: fast GET-only recovery, never replay GOTO.
  * Even when another device is overdue, its routine poll waits for the cover. */
 CHECK(dsr_fleet_cover_request(&f,1,(dsr_cover_intent_t){.command=DSR_COVER_GOTO,.position=61},true,2)==DSR_OK);
 CHECK(dsr_fleet_next(&f,2,false,&job,&index)&&job.kind==DSR_JOB_COVER);job.id=78;dsr_fleet_started(&f,index,&job);
 e=(dsr_event_t){.id=78,.device=c.device,.result=DSR_OFFLINE};CHECK(dsr_fleet_complete(&f,index,&e,3)&&f.slots[1].next_poll==1003&&!f.slots[1].connection.device.online&&!f.slots[1].queued);
 f.slots[0].next_poll=0;CHECK(dsr_fleet_next(&f,1003,true,&job,&index)&&index==1&&job.kind==DSR_JOB_PROBE);job.id=79;dsr_fleet_started(&f,index,&job);
 e=(dsr_event_t){.id=79,.device=c.device,.result=DSR_OK,.telemetry=c.telemetry};e.telemetry.cover.state=DSR_COVER_STATE_OPENING;e.telemetry.cover.flags|=DSR_COVER_TARGET;e.telemetry.cover.target_pos=61;e.telemetry.cover.current_pos=45;
 CHECK(dsr_fleet_complete(&f,index,&e,1004)&&f.slots[1].next_poll==2004&&f.slots[1].connection.telemetry.cover.current_pos==45&&f.slots[1].result==DSR_OK&&!f.slots[1].command_error);
 CHECK(dsr_fleet_next(&f,2004,true,&job,&index)&&index==1&&job.kind==DSR_JOB_PROBE);job.id=80;dsr_fleet_started(&f,index,&job);e.id=80;e.telemetry.cover.current_pos=61;e.telemetry.cover.state=DSR_COVER_STATE_STOPPED;
 CHECK(dsr_fleet_complete(&f,index,&e,2005)&&f.slots[1].next_poll==7005);
 job.id=81;dsr_fleet_started(&f,1,&job);e.id=81;e.result=DSR_AUTH_REQUIRED;CHECK(dsr_fleet_complete(&f,1,&e,7005)&&f.slots[1].blocked);f.slots[0].next_poll=9000;CHECK(!dsr_fleet_next(&f,8005,true,&job,&index));

}
