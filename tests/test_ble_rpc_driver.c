/* Actual NimBLE client and protocol against deterministic callback fixtures.
 * Radio, link security, Wi-Fi join and persistence are not physical evidence. */
#include "dsr_ble_rpc.h"
#include "../components/dsr_platform/dsr_ble_shared.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "host/ble_hs.h"
#include "os/os_mbuf.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
enum {HAPPY,CANCEL_CONSENT,CHANGED_ID,LOCKED,ENHANCED,MISSING_CHAR,DUPLICATE_CHAR,AUTH,WRITE_TIMEOUT,STALE_ACK,WRONG_ATTRIBUTE,SECURITY,CANCEL_CONNECT,LATE_CLOSE,LOAD_ERROR,OOM_RESPONSE,REPEAT_SECURITY,PASSKEY_SECURITY,NEVER_CLOSE,NEVER_CANCEL,VANISHED_PEER};
static unsigned mode,sets,info_reads,consents,api_calls,terminated;
static unsigned response_allocations;
static bool watch_request_free,request_wiped,secret_at_free;
void *__real_calloc(size_t,size_t);
void __real_free(void *);
void *__wrap_calloc(size_t count,size_t size)
{
 /* Two four-request read preflights precede the one credential-bearing RPC.
  * Fail its receive allocation after request serialization, before any TX. */
 if(mode==OOM_RESPONSE&&count==8193&&size==1&&++response_allocations==9){watch_request_free=true;return NULL;}
 return __real_calloc(count,size);
}
void __wrap_free(void *p)
{
 /* This is the immediate application-owned serialized request free. Inspect
  * it before releasing memory; never print the fixture credential. */
 if(watch_request_free){
  watch_request_free=false;CHECK(p);
  const size_t n=sizeof("{\"id\":9,\"src\":\"dsr-12345678-00000000\",\"method\":\"WiFi.SetConfig\",\"params\":{\"config\":{\"sta\":{\"enable\":true,\"ssid\":\"Own home\",\"pass\":\"private-home-psk\",\"ipv4mode\":\"dhcp\"}}}}")-1;
  request_wiped=true;const unsigned char *bytes=p;
  for(size_t i=0;i<n;i++)if(bytes[i])request_wiped=false;
  for(size_t i=0;i+sizeof("private-home-psk")-1<=n;i++)if(!memcmp(bytes+i,"private-home-psk",sizeof("private-home-psk")-1))secret_at_free=true;
 }
 __real_free(p);
}
static uint32_t clock_ms;
static bool healthy=true,queued,connected,connecting,refuse_schedule,quarantined;
static unsigned quarantines;
static void (*operation)(bool);
static int (*gap_callback)(struct ble_gap_event *,void *);
static void *gap_arg,*old_gap_arg;
static ble_gatt_attr_fn *old_attr_callback;static void *old_attr_arg;
static char request[1025],response[8193];static size_t expected,used,response_used;
static uint32_t close_started;
static dsr_settings_t saved={.ssid="Own home",.password="private-home-psk",.wifi_security=DSR_WIFI_WPA2};
static dsr_ble_device_t candidate={.address={1,2,3,4,5,6},.address_type=0,.kind=DSR_BLE_SHELLY_RPC,.evidence=DSR_BLE_RPC};
static bool locked_mutex;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return (void *)1;}
int xSemaphoreTake(SemaphoreHandle_t m,TickType_t timeout){(void)timeout;CHECK(m==(void *)1&&!locked_mutex);locked_mutex=true;return 1;}
int xSemaphoreGive(SemaphoreHandle_t m){CHECK(m==(void *)1&&locked_mutex);locked_mutex=false;return 1;}
int64_t esp_timer_get_time(void){return (int64_t)clock_ms*1000;}
uint32_t esp_random(void){return 0x12345678;}
bool dsr_ble_host_healthy(void){return healthy&&!quarantined;}
void dsr_ble_manager_quarantine(void){CHECK(!locked_mutex);quarantined=true;quarantines++;}
bool dsr_ble_manager_rpc(void (*fn)(bool)){if(refuse_schedule||quarantined)return false;CHECK(!queued);queued=true;operation=fn;return true;}
bool dsr_wifi_connected(void){return true;}
bool dsr_wifi_temporary_active(void){return false;}
bool dsr_ap_sensitive(void){return false;}
esp_err_t dsr_settings_load(dsr_settings_t *out){CHECK(!dsr_activity_restart_reserve());if(mode==LOAD_ERROR)return ESP_FAIL;*out=saved;return ESP_OK;}
esp_err_t dsr_settings_save(const dsr_settings_t *settings){(void)settings;CHECK(!"B1 must never save home settings");return ESP_FAIL;}
int ble_hs_id_infer_auto(int privacy,uint8_t *type){CHECK(!privacy);*type=0;return 0;}
static void gap(int event,int status,void *arg)
{struct ble_gap_event e={.type=event};if(event==BLE_GAP_EVENT_CONNECT){e.connect.status=status;e.connect.conn_handle=7;}else if(event==BLE_GAP_EVENT_DISCONNECT)e.disconnect.conn.conn_handle=7;else e.enc_change.conn_handle=7;int result=gap_callback(&e,arg);if(event==BLE_GAP_EVENT_REPEAT_PAIRING)CHECK(result==BLE_GAP_REPEAT_PAIRING_IGNORE);}
int ble_gap_connect(uint8_t own,const ble_addr_t *peer,int32_t timeout,const struct ble_gap_conn_params *params,int (*cb)(struct ble_gap_event *,void *),void *arg)
{CHECK(!own&&timeout==10000&&!params&&!memcmp(peer->val,candidate.address,6));api_calls++;gap_callback=cb;gap_arg=arg;connecting=true;if(old_gap_arg)gap(BLE_GAP_EVENT_CONNECT,0,old_gap_arg);if(mode!=CANCEL_CONNECT&&mode!=NEVER_CANCEL){connecting=false;connected=true;gap(BLE_GAP_EVENT_CONNECT,0,arg);}return 0;}
int ble_gap_conn_cancel(void){CHECK(connecting);if(!close_started)close_started=clock_ms;if(mode==NEVER_CANCEL)return 0;connecting=false;gap(BLE_GAP_EVENT_CONNECT,1,gap_arg);return 0;}
int ble_gap_terminate(uint16_t handle,uint8_t reason){CHECK(handle==7&&reason==BLE_ERR_REM_USER_CONN_TERM);terminated++;if(!close_started)close_started=clock_ms;if(mode==NEVER_CLOSE)return 0;if(mode==VANISHED_PEER){connected=false;return BLE_HS_ENOTCONN;}if(mode!=LATE_CLOSE||clock_ms-close_started>3000){connected=false;gap(BLE_GAP_EVENT_DISCONNECT,0,gap_arg);}return 0;}
uint16_t ble_att_mtu(uint16_t handle){CHECK(handle==7);return 23;}
int ble_uuid_from_str(ble_uuid_any_t *uuid,const char *text){CHECK(strlen(text)==36);uuid->u.type=128;strcpy(uuid->u.text,text);return 0;}
int ble_uuid_cmp(const ble_uuid_t *a,const ble_uuid_t *b){return strcmp(a->text,b->text);}
int ble_gattc_disc_svc_by_uuid(uint16_t handle,const ble_uuid_t *uuid,ble_gatt_disc_svc_fn *cb,void *arg)
{CHECK(handle==7&&!strcmp(uuid->text,"5f6d4f53-5f52-5043-5f53-56435f49445f"));struct ble_gatt_svc service={.start_handle=1,.end_handle=8};struct ble_gatt_error ok={0},done={.status=BLE_HS_EDONE};cb(handle,&ok,&service,arg);cb(handle,&done,NULL,arg);return 0;}
int ble_gattc_disc_all_chrs(uint16_t handle,uint16_t first,uint16_t last,ble_gatt_chr_fn *cb,void *arg)
{CHECK(handle==7&&first==1&&last==8);const char *uuids[]={"5f6d4f53-5f52-5043-5f64-6174615f5f5f","5f6d4f53-5f52-5043-5f74-785f63746c5f","5f6d4f53-5f52-5043-5f72-785f63746c5f"};struct ble_gatt_error ok={0},done={.status=BLE_HS_EDONE};for(unsigned i=0;i<(mode==MISSING_CHAR?2u:3u);i++){struct ble_gatt_chr chr={.def_handle=(uint16_t)(1+2*i),.val_handle=(uint16_t)(2+2*i),.properties=BLE_GATT_CHR_F_READ|BLE_GATT_CHR_F_WRITE};ble_uuid_from_str(&chr.uuid,uuids[i]);cb(handle,&ok,&chr,arg);if(mode==DUPLICATE_CHAR&&i==1)cb(handle,&ok,&chr,arg);}cb(handle,&done,NULL,arg);return 0;}
static void build_response(void)
{
 cJSON *r=cJSON_Parse(request);CHECK(r);const char *method=cJSON_GetObjectItemCaseSensitive(r,"method")->valuestring;const char *src=cJSON_GetObjectItemCaseSensitive(r,"src")->valuestring;int id=cJSON_GetObjectItemCaseSensitive(r,"id")->valueint;const char *payload="{}";
 if(!strcmp(method,"Shelly.GetDeviceInfo")){info_reads++;payload=mode==LOCKED&&info_reads>1?"{\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"AABBCCDDEEFF\",\"gen\":2,\"model\":\"SNSW-001X16EU\",\"auth_en\":false,\"ver\":\"1.7.5\",\"provision\":\"locked\"}":mode==CHANGED_ID&&info_reads>1?"{\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"112233445566\",\"gen\":2,\"model\":\"SNSW-001X16EU\",\"auth_en\":false,\"ver\":\"1.7.5\",\"provision\":\"pending\"}":"{\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"AABBCCDDEEFF\",\"gen\":2,\"model\":\"SNSW-001X16EU\",\"auth_en\":false,\"ver\":\"1.7.5\",\"provision\":\"pending\"}";}
 else if(!strcmp(method,"Sys.GetConfig"))payload=mode==ENHANCED&&info_reads>1?"{\"device\":{\"enhanced_security\":true}}":"{\"device\":{}}";
 else if(!strcmp(method,"Shelly.ListMethods"))payload="{\"methods\":[\"WiFi.SetConfig\",\"WiFi.GetStatus\",\"Shelly.GetDeviceInfo\",\"Sys.GetConfig\",\"Shelly.GetStatus\"]}";
 else if(!strcmp(method,"Shelly.GetStatus"))payload="{\"switch:0\":{\"id\":0,\"output\":false}}";
 else if(!strcmp(method,"WiFi.SetConfig")){CHECK(consents==1&&sets==0&&info_reads==2);sets++;const cJSON *sta=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"params"),"config"),"sta");CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(sta,"ssid")->valuestring,saved.ssid)&&!strcmp(cJSON_GetObjectItemCaseSensitive(sta,"pass")->valuestring,saved.password));payload="{\"restart_required\":false}";}
 else if(!strcmp(method,"WiFi.GetStatus"))payload="{\"status\":\"got ip\",\"ssid\":\"Own home\",\"sta_ip\":\"192.168.1.90\"}";
 else CHECK(!"unexpected RPC method");
 snprintf(response,sizeof(response),"{\"id\":%d,\"src\":\"shellyplus1-aabbccddeeff\",\"dst\":\"%s\",%s:%s}",id,src,mode==AUTH?"\"error\"":"\"result\"",mode==AUTH?"{\"code\":401}":payload);response_used=0;cJSON_Delete(r);
}
static void attribute(ble_gatt_attr_fn *cb,void *arg,uint16_t attribute_handle,const uint8_t *data,size_t length)
{struct os_mbuf second={.data=data?data+length/2:NULL,.length=(unsigned)(length-length/2)},first={.data=data,.length=(unsigned)(length/2),.next=&second};struct ble_gatt_attr attr={.handle=attribute_handle,.om=length?&first:NULL};struct ble_gatt_error ok={0};if(mode==STALE_ACK&&old_attr_callback&&arg!=old_attr_arg)old_attr_callback(7,&ok,&attr,old_attr_arg);cb(7,&ok,&attr,arg);}
int ble_gattc_write_flat(uint16_t handle,uint16_t attribute_handle,const void *bytes,uint16_t length,ble_gatt_attr_fn *cb,void *arg)
{
 CHECK(handle==7&&connected);api_calls++;
 if(attribute_handle==4){CHECK(length==4);const uint8_t *b=bytes;expected=(size_t)b[0]<<24|(size_t)b[1]<<16|(size_t)b[2]<<8|b[3];CHECK(expected<sizeof(request));used=0;if(!old_attr_callback){old_attr_callback=cb;old_attr_arg=arg;}}
 else{CHECK(attribute_handle==2&&length<=20&&length&&used+length<=expected);memcpy(request+used,bytes,length);used+=length;request[used]=0;if(used==expected)build_response();}
 if(mode==WRITE_TIMEOUT&&sets&&attribute_handle==2&&used==expected)return 0;
 attribute(cb,arg,mode==WRONG_ATTRIBUTE?99:attribute_handle,NULL,0);return 0;
}
int ble_gattc_read(uint16_t handle,uint16_t attribute_handle,ble_gatt_attr_fn *cb,void *arg)
{CHECK(handle==7&&connected);api_calls++;if(attribute_handle==6){uint32_t length=(uint32_t)strlen(response);uint8_t bytes[4]={(uint8_t)(length>>24),(uint8_t)(length>>16),(uint8_t)(length>>8),(uint8_t)length};attribute(cb,arg,attribute_handle,bytes,4);}else{CHECK(attribute_handle==2);size_t length=strlen(response)-response_used;if(length>20)length=20;attribute(cb,arg,attribute_handle,(const uint8_t *)response+response_used,length);response_used+=length;}return 0;}
unsigned test_mbuf_length(const struct os_mbuf *b){unsigned n=0;for(;b;b=b->next)n+=b->length;return n;}
int os_mbuf_copydata(const struct os_mbuf *b,int offset,int length,void *out)
{CHECK(offset==0&&length>=0);uint8_t *p=out;for(;b&&length;b=b->next){unsigned n=b->length;if(n>(unsigned)length)n=(unsigned)length;memcpy(p,b->data,n);p+=n;length-=(int)n;}return length?-1:0;}
void vTaskDelay(TickType_t ticks)
{
 CHECK(!locked_mutex);clock_ms+=ticks;CHECK(clock_ms<1000000);dsr_ble_rpc_snapshot_t snapshot;dsr_ble_rpc_snapshot(&snapshot);
 if(snapshot.phase==DSR_BLE_RPC_CONFIRM){CHECK(!sets);if(mode==CANCEL_CONSENT)dsr_ble_rpc_cancel(snapshot.generation);else if(mode==SECURITY||mode==REPEAT_SECURITY||mode==PASSKEY_SECURITY)gap(mode==SECURITY?BLE_GAP_EVENT_ENC_CHANGE:mode==REPEAT_SECURITY?BLE_GAP_EVENT_REPEAT_PAIRING:BLE_GAP_EVENT_PASSKEY_ACTION,0,gap_arg);else{CHECK(dsr_ble_rpc_confirm(snapshot.generation,saved.ssid)==DSR_OK);consents++;CHECK(dsr_ble_rpc_confirm(snapshot.generation,saved.ssid)==DSR_BUSY);}}
 if((mode==CANCEL_CONNECT||mode==NEVER_CANCEL)&&connecting)dsr_ble_rpc_cancel(snapshot.generation);
}
static void scenario(unsigned test)
{
 mode=test;sets=info_reads=consents=api_calls=terminated=quarantines=0;quarantined=connected=connecting=false;response_allocations=0;watch_request_free=request_wiped=secret_at_free=false;close_started=0;candidate.seen_ms=clock_ms;dsr_settings_t before=saved;
 CHECK(dsr_ble_rpc_start(&candidate)==DSR_OK&&queued&&dsr_ble_rpc_active()&&!dsr_activity_restart_reserve());queued=false;operation(healthy);
 dsr_ble_rpc_snapshot_t snapshot;dsr_ble_rpc_snapshot(&snapshot);CHECK(!snapshot.active&&!dsr_ble_rpc_active()&&((!connected&&!connecting)||quarantined));CHECK(!memcmp(&saved,&before,sizeof(saved)));CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();old_gap_arg=gap_arg;
 if(test==NEVER_CLOSE||test==NEVER_CANCEL){
  CHECK(quarantines==1&&snapshot.phase==DSR_BLE_RPC_QUARANTINED&&snapshot.result!=DSR_OK&&sets==(test==NEVER_CLOSE?1u:0u)&&clock_ms-close_started<=DSR_BLE_CLOSE_MS+100);
  dsr_ble_rpc_snapshot_t before_late=snapshot;gap(BLE_GAP_EVENT_DISCONNECT,0,gap_arg);gap(BLE_GAP_EVENT_CONNECT,0,gap_arg);dsr_ble_rpc_snapshot(&snapshot);CHECK(!memcmp(&before_late,&snapshot,sizeof(snapshot)));
  candidate.seen_ms=clock_ms;CHECK(dsr_ble_rpc_start(&candidate)==DSR_BUSY);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 }
 else if(test==HAPPY||test==STALE_ACK||test==LATE_CLOSE||test==VANISHED_PEER){CHECK(snapshot.phase==DSR_BLE_RPC_LAN_READY&&snapshot.result==DSR_OK&&snapshot.write_possible&&sets==1&&!strcmp(snapshot.device.host,"192.168.1.90"));}
 else if(test==WRITE_TIMEOUT){CHECK(snapshot.phase==DSR_BLE_RPC_LAN_READY&&snapshot.result==DSR_OFFLINE&&snapshot.write_possible&&sets==1);}
 else{CHECK(snapshot.phase==DSR_BLE_RPC_ERROR||snapshot.phase==DSR_BLE_RPC_CANCELLED);CHECK(!sets&&!snapshot.write_possible);}
 if(test==SECURITY||test==REPEAT_SECURITY||test==PASSKEY_SECURITY)CHECK(snapshot.result==DSR_AUTH_REQUIRED);
 if(test==OOM_RESPONSE){CHECK(response_allocations==9&&consents==1&&info_reads==2&&!watch_request_free);CHECK(snapshot.result==DSR_IO&&request_wiped&&!secret_at_free&&!sets);}
}
int main(void)
{
 for(unsigned i=0;i<=VANISHED_PEER;i++)scenario(i);
 mode=HAPPY;sets=api_calls=0;candidate.seen_ms=clock_ms;CHECK(dsr_ble_rpc_start(&candidate)==DSR_OK);dsr_ble_rpc_snapshot_t cancelled;dsr_ble_rpc_snapshot(&cancelled);dsr_ble_rpc_cancel(cancelled.generation);queued=false;operation(true);dsr_ble_rpc_snapshot(&cancelled);CHECK(cancelled.phase==DSR_BLE_RPC_CANCELLED&&!api_calls&&!sets&&!cancelled.active);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 refuse_schedule=true;candidate.seen_ms=clock_ms;CHECK(dsr_ble_rpc_start(&candidate)==DSR_BUSY&&!dsr_ble_rpc_active()&&dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: %u actual GATT client checks; both consents/preflight/fragment ACK/no replay/stale callbacks/cleanup; SDK and radio MODEL\n",checks);return 0;
}
