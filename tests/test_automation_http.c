/* Reuse explicit SDK transport providers; execute only automation cases. */
#define main dsr_existing_platform_main
#include "test_platform.c"
#undef main

static void contact_source(void){
 dsr_telemetry_t t; cJSON *j=cJSON_Parse("{\"id\":200,\"battery\":99,\"last_updated_ts\":1800000000}");
 CHECK(dsr_blu_parse_device(j,200,&t)==DSR_OK);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":210,\"value\":true,\"last_update_ts\":1706593991.91}");
 CHECK(dsr_blu_apply_sensor(j,210,0x2d,&t)==DSR_OK&&t.blu_updated_s==1706593991u);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":211,\"value\":100,\"last_updated_ts\":1800000010}");
 CHECK(dsr_blu_apply_sensor(j,211,1,&t)==DSR_OK&&t.blu_updated_s==1706593991u);cJSON_Delete(j);
 const char *unknown[]={"{\"id\":210,\"value\":true}",
  "{\"id\":210,\"value\":true,\"last_update_ts\":null}",
  "{\"id\":210,\"value\":true,\"last_updated_ts\":-1}",
  "{\"id\":210,\"value\":true,\"last_updated_ts\":4294967296}",
  "{\"id\":210,\"value\":true,\"last_updated_ts\":\"now\"}",
  "{\"id\":210,\"value\":true,\"last_update_ts\":1706593991,\"last_updated_ts\":1800000000}"};
 for(unsigned i=0;i<sizeof(unknown)/sizeof(*unknown);i++){
  memset(&t,0,sizeof(t));t.blu_updated_s=1800000010;j=cJSON_Parse(unknown[i]);
  CHECK(dsr_blu_apply_sensor(j,210,0x1a,&t)==DSR_OK&&!t.blu_updated_s&&(t.blu.flags&DSR_BLU_HAS_OPEN));cJSON_Delete(j);
 }
 memset(&t,0,sizeof(t));j=cJSON_Parse("{\"id\":210,\"value\":false,\"last_updated_ts\":1706593991.91}");
 CHECK(dsr_blu_apply_sensor(j,210,0x1a,&t)==DSR_OK&&t.blu_updated_s==1706593991u);cJSON_Delete(j);
 j=cJSON_Parse("{\"id\":211,\"value\":true,\"last_updated_ts\":1800000010}");
 CHECK(dsr_blu_apply_sensor(j,211,0x2d,&t)!=DSR_OK);cJSON_Delete(j);
 dsr_blu_object_t objects[8];unsigned n;
 j=cJSON_Parse("{\"id\":200,\"objects\":[{\"obj_id\":26,\"idx\":0,\"component\":\"bthomesensor:210\"},{\"obj_id\":26,\"idx\":1,\"component\":\"bthomesensor:211\"}]}");
 CHECK(dsr_blu_parse_known(j,200,objects,8,&n)==DSR_OK&&n==2);cJSON_Delete(j);
}
static unsigned set_target_calls,guards;static bool block_read;
static void partial_contact_bundle(void){
 dsr_connection_t c=blu_connection();c.telemetry.blu.errors=DSR_BLU_TRV_CHECKED;
 step_t data[]={
  {"/shelly",blu_gateway_identity,200,false,false},
  {"/rpc/BTHomeDevice.GetConfig?id=201",blu_config,200,false,true},
  {"/rpc/BTHomeDevice.GetStatus?id=201",blu_device_status,200,false,true},
  {"/rpc/BTHomeDevice.GetKnownObjects?id=201","{\"id\":201,\"objects\":[{\"obj_id\":26,\"idx\":0,\"component\":\"bthomesensor:210\"},{\"obj_id\":26,\"idx\":1,\"component\":\"bthomesensor:211\"}]}",200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=210","{\"id\":210,\"value\":null}",200,false,true},
  {"/rpc/BTHomeSensor.GetStatus?id=211","{\"id\":211,\"value\":true,\"last_update_ts\":1706593991.91}",200,false,true}};
 fixture(data,6,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_refresh(&c)==DSR_OK&&used==6);
 CHECK((c.telemetry.blu.flags&DSR_BLU_HAS_OPEN)&&!c.telemetry.blu_updated_s);
}
static void write_attempt(void){set_target_calls++;}
static dsr_result_t read_guard(void){guards++;return block_read?DSR_CONFLICT:DSR_OK;}
static void direct_trv_read(void){
 dsr_connection_t c=connection();c.device.generation=DSR_GEN2_PLUS;c.device.endpoint_kind=DSR_ENDPOINT_BLU;
 strcpy(c.device.device_id,"shellyblugwg3-aabbccddeeff_aabbcc00b567");
 step_t data[]={
  {"/shelly","{\"gen\":3,\"id\":\"shellyblugwg3-aabbccddeeff\",\"model\":\"S3GW-1DBT001\"}",200,false,false},
  {"/rpc/BTHomeDevice.GetConfig?id=200","{\"id\":200,\"addr\":\"aa:bb:cc:00:b5:67\"}",200,false,true},
  {"/rpc/BluTrv.GetConfig?id=200","{\"id\":200,\"trv\":\"bthomedevice:200\"}",200,false,true},
  {"/rpc/BluTrv.Call","{\"id\":0,\"target_C\":21.5,\"current_C\":20.5,\"pos\":30,\"errors\":[]}",200,true,true}};
 cover_payload="{\"id\":200,\"method\":\"TRV.GetStatus\",\"params\":{\"id\":0}}";
 (void)dsr_http_job_write_hook(write_attempt);(void)dsr_http_job_guard(read_guard);
 fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv_read(&c)==DSR_OK&&used==4);
 CHECK(c.telemetry.trv_target_c==21.5f&&c.telemetry.trv_current_c==20.5f&&c.device.online);
 CHECK(set_target_calls==0&&guards>=3);CHECK(!strstr(posted,"SetTarget"));
 const char *bad[]={"null","{\"id\":1,\"target_C\":21.5,\"current_C\":20}",
  "{\"id\":0,\"current_C\":20}","{\"id\":0,\"target_C\":21.5}",
  "{\"id\":0,\"target_C\":20,\"target_C\":21,\"current_C\":20}",
  "{\"error\":{\"code\":-104}}","{broken",NULL};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){
  data[3].body=bad[i];fixture(data,4,HTTP_AUTH_TYPE_DIGEST);
  CHECK(dsr_http_trv_read(&c)!=DSR_OK&&used==4&&!c.device.online&&set_target_calls==0);
 }
 data[3].body="{\"id\":0,\"target_C\":19.75,\"current_C\":20}";
 fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv_read(&c)==DSR_UNSUPPORTED&&used==4&&set_target_calls==0);
 data[3].body="{\"id\":0,\"target_C\":21.5,\"current_C\":20}";data[3].status=429;
 fixture(data,4,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv_read(&c)==DSR_RATE_LIMITED&&set_target_calls==0);
 data[2].body="{\"id\":200,\"trv\":\"bthomedevice:201\"}";fixture(data,3,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_trv_read(&c)==DSR_UNSUPPORTED&&used==3&&set_target_calls==0);
 data[1].body="{\"id\":200,\"addr\":\"aa:bb:cc:00:b5:68\"}";fixture(data,2,HTTP_AUTH_TYPE_DIGEST);
 CHECK(dsr_http_trv_read(&c)==DSR_IDENTITY_MISMATCH&&used==2&&set_target_calls==0);
 block_read=true;fixture(data,0,HTTP_AUTH_TYPE_DIGEST);CHECK(dsr_http_trv_read(&c)==DSR_CONFLICT&&used==0);
 block_read=false;dsr_http_job_guard(NULL);dsr_http_job_write_hook(NULL);cover_payload=NULL;
 CHECK(sizeof(dsr_job_t)==560&&sizeof(dsr_event_t)==1192);
}
int main(void){contact_source();direct_trv_read();partial_contact_bundle();printf("automation HTTP: %u checks passed; no write attempts\n",checks);return 0;}
