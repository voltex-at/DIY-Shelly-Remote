/* Actual parser/serializer and cJSON. Snapshot provider/NVS/HTTP are MODEL. */
#include "dsr_runtime_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,attempts,fail_at,live;
static dsr_runtime_device_t snapshots[DSR_MAX_DEVICES];
static size_t snapshot_count;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static void *tracked_malloc(size_t n){attempts++;if(fail_at&&attempts==fail_at)return NULL;void *p=malloc(n);if(p)live++;return p;}
static void tracked_free(void *p){if(p){CHECK(live>0);live--;free(p);}}
size_t dsr_workspace_runtime_snapshot(dsr_runtime_device_t *out,size_t capacity)
{CHECK(out&&capacity==DSR_MAX_DEVICES&&snapshot_count<=capacity);memcpy(out,snapshots,snapshot_count*sizeof(*out));return snapshot_count;}
static const char *valid="{\"device_id\":\"shellyplus1-aabbccddeeff\",\"generation\":2,\"channel\":7,\"on\":true}";
static void parser(void)
{
 dsr_device_t d={0};bool on=false;CHECK(dsr_output_parse(valid,&d,&on)&&on&&d.generation==DSR_GEN2_PLUS&&d.channel==7&&!strcmp(d.device_id,"shellyplus1-aabbccddeeff"));
 CHECK(!d.host[0]&&!d.name[0]&&!d.online&&!d.has_power);
 CHECK(dsr_output_parse("{\"on\":false,\"channel\":0,\"generation\":1,\"device_id\":\"AABBCCDDEEFF\"}",&d,&on)&&!on&&d.generation==DSR_GEN1);
 const char *bad[]={NULL,"","null","[]","true","1","{}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":0}",
  "{\"device_id\":\"a\",\"device_id\":\"b\",\"generation\":2,\"channel\":0,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"on\":true,\"host\":\"192.0.2.8\"}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"on\":true,\"password\":\"no-credentials-in-api\"}",
  "{\"device_id\":\"a/b\",\"generation\":2,\"channel\":0,\"on\":true}",
  "{\"device_id\":\"shelly-A\\u0000ignored\",\"generation\":2,\"channel\":0,\"on\":true}",
  "{\"device_id\\u0000ignored\":\"a\",\"generation\":2,\"channel\":0,\"on\":true}",
  "{\"device_id\":\"\",\"generation\":2,\"channel\":0,\"on\":true}",
  "{\"device_id\":7,\"generation\":2,\"channel\":0,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":3,\"channel\":0,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":2.1,\"channel\":0,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":\"2\",\"channel\":0,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":-1,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":16,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":0.5,\"on\":true}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"on\":1}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"on\":\"true\"}",
  "{\"device_id\":\"a\",\"generation\":2,\"channel\":0,\"on\":true} {}"};
 for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);i++){memset(&d,0x5a,sizeof(d));dsr_device_t before=d;on=false;CHECK(!dsr_output_parse(bad[i],&d,&on));CHECK(!memcmp(&d,&before,sizeof(d))&&!on);CHECK(live==0);}
 char large[300];memset(large,' ',sizeof(large));large[sizeof(large)-1]=0;CHECK(!dsr_output_parse(large,&d,&on));
 CHECK(!dsr_output_parse(valid,NULL,&on)&&!dsr_output_parse(valid,&d,NULL));
 attempts=0;CHECK(dsr_output_parse(valid,&d,&on));unsigned total=attempts;CHECK(live==0&&total>=8);
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;memset(&d,0x5a,sizeof(d));dsr_device_t before=d;on=false;CHECK(!dsr_output_parse(valid,&d,&on));CHECK(!memcmp(&d,&before,sizeof(d))&&!on&&live==0);}fail_at=0;
}
static void serializer(void)
{
 snapshot_count=2;
 for(unsigned i=0;i<2;i++){
  dsr_runtime_device_t *r=&snapshots[i];r->device.generation=DSR_GEN2_PLUS;r->device.channel=(uint8_t)i;snprintf(r->device.device_id,sizeof(r->device.device_id),"shelly-fixture-%u",i);snprintf(r->device.name,sizeof(r->device.name),"TEST / DEMO %u",i);snprintf(r->device.host,sizeof(r->device.host),"192.0.2.%u",i+8);r->device.online=r->device.has_switch=true;r->device.output=i!=0;r->device.has_power=true;r->device.power_w=(float)i+2.5f;r->telemetry.has_energy=i==0;r->telemetry.energy_wh=0;r->telemetry.has_voltage=true;r->telemetry.voltage_v=230+(float)i;r->pending=i!=0;r->operation_id=37+i;r->sample_age_ms=1000*i;r->has_sample_time=true;
 }
 attempts=0;char *json=dsr_runtime_json(123);CHECK(json);unsigned total=attempts;
 CHECK(!strstr(json,"password")&&!strstr(json,"credentials")&&!strstr(json,"token")&&!strstr(json,"pairing"));
 cJSON *root=cJSON_Parse(json);CHECK(root);cJSON *array=cJSON_GetObjectItemCaseSensitive(root,"devices");CHECK(cJSON_GetObjectItemCaseSensitive(root,"boot_id")->valueint==123&&cJSON_GetArraySize(array)==2);
 for(unsigned i=0;i<2;i++){cJSON *d=cJSON_GetArrayItem(array,(int)i);CHECK(cJSON_GetObjectItemCaseSensitive(d,"channel")->valueint==(int)i);CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(d,"output"))==(i!=0));CHECK(cJSON_GetObjectItemCaseSensitive(d,"operation_id")->valueint==(int)(37+i));CHECK(cJSON_GetObjectItemCaseSensitive(d,"voltage_v")->valuedouble==230+i);CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(d,"has_energy"))==(i==0));CHECK(cJSON_GetObjectItemCaseSensitive(d,"energy_wh")->valuedouble==0);}
 cJSON_Delete(root);cJSON_free(json);CHECK(live==0&&total>=80);
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;json=dsr_runtime_json(123);CHECK(!json&&live==0);}fail_at=0;
 snapshot_count=0;json=dsr_runtime_json(124);CHECK(json);root=cJSON_Parse(json);CHECK(root&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(root,"devices"))==0);cJSON_Delete(root);cJSON_free(json);CHECK(live==0);
}
static void cover_parser(void)
{
 const char *valid_cover="{\"device_id\":\"shellyplus2pm-fixture\",\"generation\":2,\"endpoint_kind\":1,\"channel\":1,\"command\":\"position\",\"position\":67}";
 dsr_device_t d;dsr_cover_intent_t intent;
 attempts=0;CHECK(dsr_cover_parse(valid_cover,&d,&intent)&&d.endpoint_kind==DSR_ENDPOINT_COVER&&d.channel==1&&intent.command==DSR_COVER_GOTO&&intent.position==67&&!d.host[0]);unsigned total=attempts;
 const char *bad[]={"{\"device_id\":\"a\",\"generation\":1,\"endpoint_kind\":1,\"channel\":0,\"command\":\"stop\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":0,\"channel\":0,\"command\":\"open\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"Calibrate\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"stop\",\"position\":0}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"position\",\"position\":100.5}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"position\",\"position\":-1}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"position\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"open\",\"command\":\"stop\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"stop\",\"host\":\"192.0.2.1\"}","{\"device_id\":\"a\\u0000tail\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"stop\"}","null","[]","{} trailing"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){memset(&d,0x5a,sizeof(d));memset(&intent,0x5a,sizeof(intent));dsr_device_t old=d;dsr_cover_intent_t prior=intent;CHECK(!dsr_cover_parse(bad[i],&d,&intent)&&!memcmp(&d,&old,sizeof(d))&&!memcmp(&intent,&prior,sizeof(intent))&&live==0);}
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;CHECK(!dsr_cover_parse(valid_cover,&d,&intent)&&live==0);}fail_at=0;
 CHECK(dsr_cover_parse("{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":15,\"command\":\"stop\"}",&d,&intent)&&intent.command==DSR_COVER_STOP&&intent.position==0);
 snapshot_count=1;snapshots[0]=(dsr_runtime_device_t){.device=d,.can_stop=true,.stop_pending=true,.telemetry={.cover={.state=DSR_COVER_STATE_CALIBRATING,.errors=32}}};
 attempts=0;char *json=dsr_runtime_json(125);CHECK(json&&strstr(json,"\"endpoint_kind\":1")&&strstr(json,"\"stop_pending\":true")&&strstr(json,"\"can_move\":false")&&strstr(json,"\"can_position\":false"));total=attempts;cJSON_free(json);CHECK(live==0);
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;json=dsr_runtime_json(125);CHECK(!json&&live==0);}fail_at=0;
}
static void light_parser(void)
{
 const char *valid_light="{\"device_id\":\"dimmer-test\",\"generation\":2,\"endpoint_kind\":2,\"channel\":1,\"command\":\"brightness\",\"brightness\":0}";
 dsr_device_t d;dsr_light_intent_t intent;attempts=0;CHECK(dsr_light_parse(valid_light,&d,&intent)&&d.endpoint_kind==DSR_ENDPOINT_LIGHT&&intent.command==DSR_LIGHT_SET_BRIGHTNESS&&intent.brightness==0&&!d.host[0]);unsigned total=attempts;
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;memset(&d,0x5a,sizeof(d));memset(&intent,0x5a,sizeof(intent));dsr_device_t old=d;dsr_light_intent_t prior=intent;CHECK(!dsr_light_parse(valid_light,&d,&intent)&&!memcmp(&d,&old,sizeof(d))&&!memcmp(&intent,&prior,sizeof(intent))&&live==0);}fail_at=0;
 const char *bad[]={"null","[]","{} trailing","{\"device_id\":\"a\",\"generation\":1,\"endpoint_kind\":2,\"channel\":0,\"command\":\"on\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":1,\"channel\":0,\"command\":\"on\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"toggle\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"on\",\"brightness\":0}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"brightness\",\"brightness\":101}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"brightness\",\"brightness\":0.5}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"brightness\",\"brightness\":null}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"on\",\"command\":\"off\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"on\",\"host\":\"192.0.2.1\"}","{\"device_id\":\"a\\u0000tail\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"off\"}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){memset(&d,0x5a,sizeof(d));memset(&intent,0x5a,sizeof(intent));dsr_device_t old=d;dsr_light_intent_t prior=intent;CHECK(!dsr_light_parse(bad[i],&d,&intent)&&!memcmp(&d,&old,sizeof(d))&&!memcmp(&intent,&prior,sizeof(intent))&&live==0);}
 CHECK(dsr_light_parse(valid_light,&d,&intent));snapshot_count=1;snapshots[0]=(dsr_runtime_device_t){.device=d,.telemetry={.light={.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS,.brightness=0}}};
 attempts=0;char *json=dsr_runtime_json(126);CHECK(json&&strstr(json,"\"endpoint_kind\":2")&&strstr(json,"\"brightness\":0")&&strstr(json,"\"has_brightness\":true")&&strstr(json,"\"can_light\":true")&&!strstr(json,"cover_state")&&!strstr(json,"credentials"));total=attempts;cJSON_free(json);CHECK(live==0);
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;json=dsr_runtime_json(126);CHECK(!json&&live==0);}fail_at=0;
}
static void rgb_parser(void)
{
 const char *valid_rgb="{\"device_id\":\"rgb-test\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":[255,17,0]}";
 dsr_device_t d;dsr_rgb_intent_t intent;attempts=0;CHECK(dsr_rgb_parse(valid_rgb,&d,&intent)&&d.endpoint_kind==DSR_ENDPOINT_RGB&&intent.command==DSR_RGB_SET_COLOR&&intent.rgb[0]==255&&intent.rgb[1]==17&&intent.rgb[2]==0&&!d.host[0]);unsigned total=attempts;
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;memset(&d,0x5a,sizeof(d));memset(&intent,0x5a,sizeof(intent));dsr_device_t old=d;dsr_rgb_intent_t prior=intent;CHECK(!dsr_rgb_parse(valid_rgb,&d,&intent)&&!memcmp(&d,&old,sizeof(d))&&!memcmp(&intent,&prior,sizeof(intent))&&live==0);}fail_at=0;
 const char *bad[]={"null","[]","{} trailing","{\"device_id\":\"a\",\"generation\":1,\"endpoint_kind\":3,\"channel\":0,\"command\":\"on\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":2,\"channel\":0,\"command\":\"on\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":1,\"command\":\"on\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"toggle\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"on\",\"brightness\":0}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":null}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":[1,2]}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":[1,2,3,4]}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":[1,2,256]}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":[1,2,3.5]}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":[1,2,-1]}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"rgb\",\"rgb\":[1,2,3],\"brightness\":5}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"on\",\"command\":\"off\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"on\",\"host\":\"192.0.2.1\"}","{\"device_id\":\"a\\u0000tail\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0,\"command\":\"off\"}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){memset(&d,0x5a,sizeof(d));memset(&intent,0x5a,sizeof(intent));dsr_device_t old=d;dsr_rgb_intent_t prior=intent;CHECK(!dsr_rgb_parse(bad[i],&d,&intent)&&!memcmp(&d,&old,sizeof(d))&&!memcmp(&intent,&prior,sizeof(intent))&&live==0);}
 CHECK(dsr_rgb_parse(valid_rgb,&d,&intent));snapshot_count=1;snapshots[0]=(dsr_runtime_device_t){.device=d,.telemetry={.rgb={.flags=DSR_RGB_VALID,.r=255,.g=17,.b=0,.brightness=0}}};
 attempts=0;char *json=dsr_runtime_json(127);CHECK(json&&strstr(json,"\"endpoint_kind\":3")&&strstr(json,"\"rgb\":[255,17,0]")&&strstr(json,"\"has_rgb\":true")&&strstr(json,"\"can_rgb\":true")&&!strstr(json,"light_conditions")&&!strstr(json,"credentials"));total=attempts;cJSON_free(json);CHECK(live==0);
 for(unsigned i=1;i<=total;i++){attempts=0;fail_at=i;json=dsr_runtime_json(127);CHECK(!json&&live==0);}fail_at=0;
 CHECK(sizeof(dsr_output_request_t)==160&&sizeof(dsr_connection_t)==296&&sizeof(dsr_telemetry_t)==32);
}
static void ht_parser(void)
{
 const char *good="{\"device_id\":\"shellyhtg3-fixture\",\"generation\":2,\"endpoint_kind\":4,\"channel\":0}";dsr_device_t d;attempts=0;CHECK(dsr_ht_refresh_parse(good,&d)&&d.endpoint_kind==DSR_ENDPOINT_HT&&!d.host[0]);unsigned total=attempts;
 for(unsigned n=1;n<=total;n++){fail_at=n;attempts=0;memset(&d,0xa5,sizeof(d));CHECK(!dsr_ht_refresh_parse(good,&d)&&!live);for(size_t i=0;i<sizeof(d);i++)CHECK(((unsigned char*)&d)[i]==0xa5);}fail_at=0;
 const char *bad[]={"null","[]","{} trailing","{\"device_id\":\"a\",\"generation\":3,\"endpoint_kind\":4,\"channel\":0}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":3,\"channel\":0}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":4,\"channel\":1}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":4,\"channel\":0,\"on\":true}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":4,\"channel\":0,\"host\":\"192.0.2.1\"}","{\"device_id\":\"a\",\"generation\":2,\"endpoint_kind\":4,\"channel\":0,\"channel\":0}","{\"device_id\":\"a\\u0000tail\",\"generation\":2,\"endpoint_kind\":4,\"channel\":0}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){CHECK(!dsr_ht_refresh_parse(bad[i],&d)&&live==0);}
 CHECK(dsr_ht_refresh_parse(good,&d));snapshot_count=1;snapshots[0]=(dsr_runtime_device_t){.device=d,.can_refresh=true,.has_sample_time=true,.sample_age_ms=900000,.telemetry={.has_temperature=true,.temperature_c=-12.5,.has_humidity=true,.humidity_pct=0,.has_battery=true,.battery_pct=0,.ht={.flags=DSR_HT_VALID|DSR_HT_EXTERNAL_KNOWN}}};
 attempts=0;char *json=dsr_runtime_json(129);CHECK(json&&strstr(json,"\"endpoint_kind\":4")&&strstr(json,"\"output\":null")&&strstr(json,"\"temperature_c\":-12.5")&&strstr(json,"\"humidity_pct\":0")&&strstr(json,"\"battery_pct\":0")&&strstr(json,"\"external_present\":false")&&strstr(json,"\"voltage_v\":null")&&strstr(json,"\"energy_wh\":null")&&strstr(json,"\"measurement_time_known\":false")&&!strstr(json,"credentials"));total=attempts;cJSON_free(json);CHECK(live==0);
 for(unsigned n=1;n<=total;n++){fail_at=n;attempts=0;json=dsr_runtime_json(129);CHECK(!json);CHECK(live==0);}fail_at=0;
 snapshots[0].telemetry=(dsr_telemetry_t){.ht={.flags=DSR_HT_VALID}};json=dsr_runtime_json(129);CHECK(json&&strstr(json,"\"temperature_c\":null")&&strstr(json,"\"humidity_pct\":null")&&strstr(json,"\"battery_pct\":null")&&strstr(json,"\"external_present\":null"));cJSON_free(json);CHECK(!live);
}
int main(void)
{
 cJSON_Hooks hooks={tracked_malloc,tracked_free};cJSON_InitHooks(&hooks);parser();serializer();cover_parser();light_parser();rgb_parser();ht_parser();CHECK(live==0);cJSON_InitHooks(NULL);printf("PASS: %u actual runtime parser/serializer checks; exhaustive cJSON allocation failures; snapshot/HTTP MODEL\n",checks);return 0;
}
