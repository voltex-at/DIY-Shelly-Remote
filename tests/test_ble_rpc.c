/* Actual framing, correlation and setup policy. No radio, GATT security or
 * physical provisioning is inferred from these protocol fixtures. */
#include "../components/dsr_platform/dsr_ble_rpc_protocol.h"
#include "dsr_ble.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static const char *client="dsr-test-001",*device="shellyplus1-aabbccddeeff";
static dsr_settings_t home={.ssid="Own & WiFi-é",.password="a&+?=private-psk",.wifi_security=DSR_WIFI_WPA2};
static void complete(dsr_rpc_stream_t *s,const dsr_rpc_action_t *a,const uint8_t *bytes,size_t length)
{CHECK(dsr_rpc_stream_complete(s,a->session,a->sequence,a->connection,DSR_OK,bytes,length,100));}
static void streams(void)
{
 uint8_t request[DSR_BLE_RPC_TX_MAX],response[DSR_BLE_RPC_RX_MAX+1];for(unsigned i=0;i<sizeof(request);i++)request[i]=(uint8_t)i;
 for(unsigned mtu_index=0;mtu_index<3;mtu_index++)for(size_t size=1;size<=DSR_BLE_RPC_TX_MAX;size++){
  uint16_t mtu=mtu_index==0?23:mtu_index==1?247:517;dsr_rpc_stream_t s={0};dsr_rpc_action_t a;
  CHECK(dsr_rpc_stream_begin(&s,10,4,mtu,request,size,response,true,0)==DSR_OK);
  CHECK(dsr_rpc_stream_next(&s,1,&a)&&a.kind==DSR_RPC_TX_CTL&&a.length==4);uint32_t announced=0;for(unsigned i=0;i<4;i++)announced=announced<<8|a.bytes[i];CHECK(announced==size&&!s.write_possible);
  CHECK(!dsr_rpc_stream_next(&s,1,&a));CHECK(!dsr_rpc_stream_complete(&s,11,a.sequence,4,DSR_OK,NULL,0,1)&&s.pending);
  CHECK(!dsr_rpc_stream_complete(&s,10,a.sequence+1,4,DSR_OK,NULL,0,1)&&s.pending);CHECK(!dsr_rpc_stream_complete(&s,10,a.sequence,5,DSR_OK,NULL,0,1)&&s.pending);complete(&s,&a,NULL,0);
  size_t offset=0;while(s.step==DSR_RPC_TX_DATA){CHECK(dsr_rpc_stream_next(&s,2,&a)&&a.kind==DSR_RPC_TX_DATA&&a.length>0&&a.length<=mtu-3u&&s.write_possible);CHECK(!memcmp(a.bytes,request+offset,a.length));offset+=a.length;complete(&s,&a,NULL,0);}CHECK(offset==size&&s.tx_used==size);
  CHECK(dsr_rpc_stream_next(&s,3,&a)&&a.kind==DSR_RPC_RX_CTL);uint8_t zero[4]={0};complete(&s,&a,zero,4);CHECK(s.step==DSR_RPC_RX_CTL&&!s.pending&&s.result==DSR_BUSY);
  CHECK(dsr_rpc_stream_next(&s,4,&a));uint8_t length[4]={0,0,0,61};complete(&s,&a,length,4);
  size_t received=0;uint8_t data[516];memset(data,'x',sizeof(data));while(s.step==DSR_RPC_RX_DATA){CHECK(dsr_rpc_stream_next(&s,5,&a)&&a.kind==DSR_RPC_RX_DATA);size_t n=61-received;if(n>mtu-1u)n=mtu-1u;complete(&s,&a,data,n);received+=n;}
  CHECK(s.step==DSR_RPC_DONE&&s.result==DSR_OK&&s.rx_used==61&&response[61]==0);CHECK(!dsr_rpc_stream_next(&s,6,&a));
 }
 for(unsigned variant=0;variant<7;variant++){
  dsr_rpc_stream_t s={0};dsr_rpc_action_t a;CHECK(dsr_rpc_stream_begin(&s,1,2,23,request,1,response,true,0)==DSR_OK);CHECK(dsr_rpc_stream_next(&s,1,&a));complete(&s,&a,NULL,0);CHECK(dsr_rpc_stream_next(&s,1,&a));
  if(variant==0){dsr_rpc_stream_cancel(&s);CHECK(s.write_possible&&s.step==DSR_RPC_FAILED&&!dsr_rpc_stream_next(&s,1,&a));CHECK(!dsr_rpc_stream_complete(&s,1,a.sequence,2,DSR_OK,NULL,0,1));continue;}
  if(variant==1){CHECK(dsr_rpc_stream_complete(&s,1,a.sequence,2,DSR_OFFLINE,NULL,0,1));CHECK(s.step==DSR_RPC_FAILED&&s.write_possible&&!dsr_rpc_stream_next(&s,2,&a));continue;}
  complete(&s,&a,NULL,0);CHECK(dsr_rpc_stream_next(&s,1,&a));uint8_t length[4]={0,0,0,3};
  if(variant==2){complete(&s,&a,length,3);CHECK(s.step==DSR_RPC_FAILED);continue;}
  if(variant==3){length[2]=32;length[3]=1;complete(&s,&a,length,4);CHECK(s.step==DSR_RPC_FAILED);continue;}
  complete(&s,&a,length,4);CHECK(dsr_rpc_stream_next(&s,1,&a));uint8_t data[23]={0};complete(&s,&a,data,variant==4?0:variant==5?4:23);CHECK(s.step==DSR_RPC_FAILED&&s.result==DSR_BAD_RESPONSE);
 }
 dsr_rpc_stream_t s={0};dsr_rpc_action_t a;CHECK(dsr_rpc_stream_begin(&s,1,2,23,request,1,response,true,UINT32_MAX-100)==DSR_OK);CHECK(!dsr_rpc_stream_next(&s,DSR_BLE_RPC_TIMEOUT_MS,&a)&&s.result==DSR_OFFLINE);
 CHECK(dsr_rpc_stream_begin(&s,1,2,23,request,1,response,true,0)==DSR_OK);s.sequence=UINT32_MAX;CHECK(!dsr_rpc_stream_next(&s,1,&a)&&s.result==DSR_IO);
 CHECK(dsr_rpc_stream_begin(&s,1,2,22,request,1,response,true,0)==DSR_INVALID);CHECK(dsr_rpc_stream_begin(&s,1,2,518,request,1,response,true,0)==DSR_INVALID);CHECK(dsr_rpc_stream_begin(&s,1,UINT16_MAX,23,request,1,response,true,0)==DSR_INVALID);CHECK(dsr_rpc_stream_begin(&s,0,2,23,request,1,response,true,0)==DSR_INVALID);
}
static dsr_result_t parse(const char *text,const char *expected,cJSON **out,bool *notification)
{return dsr_rpc_response((const uint8_t *)text,strlen(text),17,client,expected,out,notification);}
static const char *good="{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":{\"restart_required\":false}}";
static void envelopes(void)
{
 cJSON *out=NULL;bool notification;
 CHECK(parse(good,device,&out,&notification)==DSR_OK&&out&&!notification);cJSON_Delete(out);
 const char *bad[]={"{}","[]","{broken","{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":{},\"error\":{}}","{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":null}","{\"id\":17,\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":{}}","{\"id\":16,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":{}}","{\"id\":\"17\",\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":{}}","{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"other-client\",\"result\":{}}","{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":{},\"extra\":true}","{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\\u0000suffix\",\"dst\":\"dsr-test-001\",\"result\":{}}","{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"result\":{\"ssid\":\"a\",\"ssid\":\"b\"}}","{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"jsonrpc\":\"1.0\",\"result\":{}}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){out=(cJSON *)1;CHECK(parse(bad[i],device,&out,&notification)!=DSR_OK&&out==NULL&&!notification);}
 CHECK(parse("{\"id\":17,\"src\":\"shelly-other\",\"dst\":\"dsr-test-001\",\"result\":{}}",device,&out,&notification)==DSR_IDENTITY_MISMATCH);
 CHECK(parse("{\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"method\":\"NotifyStatus\",\"params\":{}}",device,&out,&notification)==DSR_BUSY&&notification&&!out);
 CHECK(parse("{\"id\":17,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"dsr-test-001\",\"error\":{\"code\":401}}",device,&out,&notification)==DSR_AUTH_REQUIRED&&!out);
 CHECK(parse(good,NULL,&out,&notification)==DSR_IDENTITY_MISMATCH); /* first response src must match actual GetDeviceInfo id */
}
static void policies(void)
{
 const char *info="{\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"gen\":2,\"model\":\"SNSW-001X16EU\",\"auth_en\":false,\"ver\":\"1.7.5\",\"provision\":\"pending\"}";
 cJSON *r=cJSON_Parse(info);dsr_rpc_identity_t identity;CHECK(dsr_rpc_identity(r,&identity)==DSR_OK&&!identity.device.host[0]&&!strcmp(identity.device.mac,"AABBCCDDEEFF"));
 const char *states[]={"complete","locked","unknown","confirmed"};for(unsigned i=0;i<4;i++){cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(r,"provision"),states[i]);CHECK(dsr_rpc_identity(r,&identity)==(i==3?DSR_OK:DSR_UNSUPPORTED));}
 cJSON_DeleteItemFromObjectCaseSensitive(r,"provision");CHECK(dsr_rpc_identity(r,&identity)==DSR_UNSUPPORTED);cJSON_Delete(r);
 r=cJSON_Parse(info);cJSON_ReplaceItemInObjectCaseSensitive(r,"auth_en",cJSON_CreateTrue());CHECK(dsr_rpc_identity(r,&identity)==DSR_AUTH_REQUIRED);cJSON_Delete(r);
 r=cJSON_Parse(info);CHECK(dsr_rpc_identity(r,&identity)==DSR_OK);cJSON_Delete(r);
 cJSON *system=cJSON_Parse("{\"device\":{}}"),*methods=cJSON_Parse("{\"methods\":[\"WiFi.SetConfig\",\"WiFi.GetStatus\",\"Sys.GetConfig\",\"Shelly.GetDeviceInfo\",\"Shelly.GetStatus\"]}"),*status=cJSON_Parse("{\"switch:0\":{\"id\":0,\"output\":false}}");
 CHECK(dsr_rpc_setup_policy(system,methods,status,&identity)==DSR_OK);strcpy(identity.firmware,"2.0.0");CHECK(dsr_rpc_setup_policy(system,methods,status,&identity)==DSR_UNSUPPORTED);
 cJSON *cfg=cJSON_GetObjectItemCaseSensitive(system,"device");cJSON_AddBoolToObject(cfg,"enhanced_security",false);CHECK(dsr_rpc_setup_policy(system,methods,status,&identity)==DSR_OK);cJSON_ReplaceItemInObjectCaseSensitive(cfg,"enhanced_security",cJSON_CreateTrue());CHECK(dsr_rpc_setup_policy(system,methods,status,&identity)==DSR_UNSUPPORTED);cJSON_ReplaceItemInObjectCaseSensitive(cfg,"enhanced_security",cJSON_CreateFalse());
 cJSON_DeleteItemFromObjectCaseSensitive(status,"switch:0");CHECK(dsr_rpc_setup_policy(system,methods,status,&identity)==DSR_UNSUPPORTED);/* Cover-only Plus2PM is the same HTTP-capable mains setup class, never a
  * synthetic Switch. Existing enhanced-security/method/battery gates remain. */
 cJSON *cover_info=cJSON_Parse(info);cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(cover_info,"id"),"shellyplus2pm-aabbccddeeff");cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(cover_info,"model"),"SNSW-102P16EU");dsr_rpc_identity_t cover_identity;CHECK(dsr_rpc_identity(cover_info,&cover_identity)==DSR_OK);cJSON_Delete(cover_info);
 cJSON_AddItemToObject(status,"cover:0",cJSON_Parse("{\"id\":0,\"state\":\"stopped\",\"pos_control\":true,\"current_pos\":38}"));CHECK(dsr_rpc_setup_policy(system,methods,status,&cover_identity)==DSR_OK);
 cJSON_ReplaceItemInObjectCaseSensitive(cfg,"enhanced_security",cJSON_CreateTrue());CHECK(dsr_rpc_setup_policy(system,methods,status,&cover_identity)==DSR_UNSUPPORTED);cJSON_ReplaceItemInObjectCaseSensitive(cfg,"enhanced_security",cJSON_CreateFalse());
 cJSON_AddItemToObject(status,"devicepower:0",cJSON_Parse("{\"battery\":{\"percent\":80}}"));CHECK(dsr_rpc_setup_policy(system,methods,status,&cover_identity)==DSR_UNSUPPORTED);cJSON_DeleteItemFromObjectCaseSensitive(status,"devicepower:0");
 cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(methods,"methods"),0);CHECK(dsr_rpc_setup_policy(system,methods,status,&cover_identity)==DSR_UNSUPPORTED);cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(methods,"methods"),cJSON_CreateString("WiFi.SetConfig"));cJSON_DeleteItemFromObjectCaseSensitive(status,"cover:0");
 cJSON_AddItemToObject(status,"switch:0",cJSON_Parse("{\"id\":0,\"output\":false}"));cJSON_AddItemToObject(status,"devicepower:0",cJSON_Parse("{\"battery\":{\"percent\":80}}"));CHECK(dsr_rpc_setup_policy(system,methods,status,&identity)==DSR_UNSUPPORTED);
 cJSON_DeleteItemFromObjectCaseSensitive(status,"devicepower:0");cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(methods,"methods"),0);CHECK(dsr_rpc_setup_policy(system,methods,status,&identity)==DSR_UNSUPPORTED);cJSON_Delete(system);cJSON_Delete(methods);cJSON_Delete(status);
 char host[16];r=cJSON_Parse("{\"status\":\"got ip\",\"ssid\":\"Own & WiFi-é\",\"sta_ip\":\"192.168.1.50\"}");CHECK(dsr_rpc_station(r,&home,host)==DSR_OK&&!strcmp(host,"192.168.1.50"));cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(r,"ssid"),"Other");CHECK(dsr_rpc_station(r,&home,host)==DSR_BAD_RESPONSE);cJSON_Delete(r);
}
static size_t allocations,frees,fail_at;
static void *alloc(size_t n){allocations++;return allocations==fail_at?NULL:malloc(n);}
static void dealloc(void *p){if(p){frees++;free(p);}}
static void allocations_test(void)
{
 cJSON_Hooks hooks={.malloc_fn=alloc,.free_fn=dealloc};cJSON_InitHooks(&hooks);
 for(unsigned method=0;method<DSR_RPC_METHOD_COUNT;method++){
  allocations=frees=0;fail_at=0;char *json=NULL;CHECK(dsr_rpc_request(method,17,client,&home,&json)==DSR_OK);size_t n=allocations;CHECK(json&&strlen(json)<DSR_BLE_RPC_TX_MAX);cJSON_free(json);CHECK(allocations==frees);
  for(size_t i=1;i<=n;i++){allocations=frees=0;fail_at=i;json=(char *)1;CHECK(dsr_rpc_request(method,17,client,&home,&json)==DSR_IO&&json==NULL);CHECK(allocations==frees+1);}
 }
 fail_at=0;allocations=frees=0;cJSON *out=NULL;bool notification;CHECK(parse(good,device,&out,&notification)==DSR_OK);size_t n=allocations;cJSON_Delete(out);CHECK(allocations==frees);
 for(size_t i=1;i<=n;i++){allocations=frees=0;fail_at=i;CHECK(parse(good,device,&out,&notification)!=DSR_OK&&!out);CHECK(allocations==frees+1);}
 cJSON_InitHooks(NULL);char *json=NULL;CHECK(dsr_rpc_request(DSR_RPC_WIFI_SET,17,client,&home,&json)==DSR_OK);cJSON *r=cJSON_Parse(json),*sta=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"params"),"config"),"sta");CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(sta,"pass")->valuestring,home.password)&&!strcmp(cJSON_GetObjectItemCaseSensitive(sta,"ssid")->valuestring,home.ssid));cJSON_Delete(r);free(json);
}
static void handoff_parser(void)
{
 uint32_t serial=0;unsigned index=99;
 CHECK(dsr_ble_handoff_parse("{\"serial\":4294967295,\"index\":15}",&serial,&index)&&serial==UINT32_MAX&&index==15);
 const char *bad[]={"{}","null","[]","{\"serial\":1,\"index\":16}","{\"serial\":-1,\"index\":0}","{\"serial\":1.5,\"index\":0}","{\"serial\":1,\"index\":0.5}","{\"serial\":1,\"index\":0,\"mac\":\"AABBCCDDEEFF\"}","{\"serial\":1,\"serial\":1}","{\"serial\":1,\"index\":0}{}","{\"serial\\u0000\":1,\"index\":0}","{\"serial\":4294967296,\"index\":0}"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++)CHECK(!dsr_ble_handoff_parse(bad[i],&serial,&index));
}
int main(void)
{
 handoff_parser();streams();envelopes();policies();allocations_test();printf("PASS: %u actual BLE RPC protocol checks; framing/correlation/setup/OOM, no RF/security/persistence proof\n",checks);return 0;}
