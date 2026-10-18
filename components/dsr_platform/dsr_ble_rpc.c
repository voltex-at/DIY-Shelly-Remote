#include "dsr_ble_rpc.h"
#include "dsr_ble_rpc_protocol.h"
#include "dsr_ble_shared.h"
#include "dsr_activity.h"
#include "sdkconfig.h"
/* The shared host supports BLU security. This open-setup RPC client still
 * refuses SMP events; its consent and setup policy are unchanged. */
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_att.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SemaphoreHandle_t mutex;
static atomic_bool active,visible,cancelled,security_refused;
static dsr_ble_rpc_snapshot_t state;
static dsr_ble_device_t peer;
static char requested_home[33];
static bool confirmed,connecting,connected,io_pending,io_done,closing;
static uint16_t connection=UINT16_MAX,service_first,service_last,data_handle,tx_handle,rx_handle,pending_handle;
static uint32_t token_counter,pending_token,pending_sequence,pending_session;
static uint32_t boot_nonce;
static dsr_result_t io_result;
static dsr_rpc_stream_t stream;
static uint8_t io_bytes[516];
static bool duplicated;
static uint32_t now_ms(void){return (uint32_t)(esp_timer_get_time()/1000);}
static void lock(void){xSemaphoreTake(mutex,portMAX_DELAY);}
static void unlock(void){xSemaphoreGive(mutex);}
static void publish(dsr_ble_rpc_phase_t phase,dsr_result_t result)
{lock();state.phase=phase;state.result=result;state.serial++;unlock();}
bool dsr_ble_rpc_active(void){return atomic_load(&active);}
bool dsr_ble_rpc_sensitive(void){return atomic_load(&active)||atomic_load(&visible);}
void dsr_ble_rpc_visibility(bool value){atomic_store(&visible,value);}
void dsr_ble_rpc_snapshot(dsr_ble_rpc_snapshot_t *out)
{if(!out)return;if(!mutex){memset(out,0,sizeof(*out));return;}lock();*out=state;out->active=atomic_load(&active);unlock();}
void dsr_ble_rpc_cancel(uint32_t generation)
{if(!mutex)return;lock();if(state.generation==generation&&atomic_load(&active))atomic_store(&cancelled,true);unlock();}
dsr_result_t dsr_ble_rpc_confirm(uint32_t generation,const char *ssid)
{
 if(!mutex||!ssid||!ssid[0])return DSR_INVALID;
 size_t length=0;while(length<33&&ssid[length])length++;if(length>32)return DSR_INVALID;
 lock();dsr_result_t result=state.generation==generation&&state.phase==DSR_BLE_RPC_CONFIRM&&!confirmed&&!atomic_load(&cancelled)&&atomic_load(&active)?DSR_OK:DSR_BUSY;
 if(result==DSR_OK){strcpy(requested_home,ssid);confirmed=true;state.phase=DSR_BLE_RPC_WRITING;state.serial++;}unlock();return result;
}
static bool callback_matches(uint16_t handle,void *arg)
{return io_pending&&handle==connection&&(uint32_t)(uintptr_t)arg==pending_token;}
static dsr_result_t sdk_result(int code)
{return code==0?DSR_OK:code==BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN)||code==BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC)?DSR_AUTH_REQUIRED:DSR_OFFLINE;}
static int service_cb(uint16_t handle,const struct ble_gatt_error *error,const struct ble_gatt_svc *service,void *arg)
{
 lock();if(callback_matches(handle,arg)){
  if(error->status==0&&service){if(service_first||!service->start_handle||service->start_handle>=service->end_handle)duplicated=true;else{service_first=service->start_handle;service_last=service->end_handle;}}
  else{io_result=error->status==BLE_HS_EDONE?(service_first&&!duplicated?DSR_OK:DSR_UNSUPPORTED):sdk_result(error->status);io_pending=false;io_done=true;}
 }unlock();return 0;
}
static int characteristic_cb(uint16_t handle,const struct ble_gatt_error *error,const struct ble_gatt_chr *chr,void *arg)
{
 static const char *uuids[]={"5f6d4f53-5f52-5043-5f64-6174615f5f5f","5f6d4f53-5f52-5043-5f74-785f63746c5f","5f6d4f53-5f52-5043-5f72-785f63746c5f"};
 lock();if(callback_matches(handle,arg)){
  if(error->status==0&&chr){for(unsigned i=0;i<3;i++){ble_uuid_any_t uuid;if(ble_uuid_from_str(&uuid,uuids[i])!=0)continue;if(ble_uuid_cmp(&uuid.u,&chr->uuid.u))continue;uint16_t *target=i==0?&data_handle:i==1?&tx_handle:&rx_handle;unsigned properties=i==0?BLE_GATT_CHR_F_READ|BLE_GATT_CHR_F_WRITE:i==1?BLE_GATT_CHR_F_WRITE:BLE_GATT_CHR_F_READ;if(*target||chr->val_handle<=service_first||chr->val_handle>service_last||chr->def_handle>=chr->val_handle||(chr->properties&properties)!=properties)duplicated=true;else *target=chr->val_handle;}}
  else{io_result=error->status==BLE_HS_EDONE?(data_handle&&tx_handle&&rx_handle&&data_handle!=tx_handle&&data_handle!=rx_handle&&tx_handle!=rx_handle&&!duplicated?DSR_OK:DSR_UNSUPPORTED):sdk_result(error->status);io_pending=false;io_done=true;}
 }unlock();return 0;
}
static int attribute_cb(uint16_t handle,const struct ble_gatt_error *error,struct ble_gatt_attr *attr,void *arg)
{
 lock();if(callback_matches(handle,arg)){
  dsr_result_t result=sdk_result(error->status);size_t length=0;
  if(result==DSR_OK&&(!attr||attr->handle!=pending_handle))result=DSR_BAD_RESPONSE;
  if(result==DSR_OK&&(stream.step==DSR_RPC_RX_CTL||stream.step==DSR_RPC_RX_DATA)){
   length=attr->om?OS_MBUF_PKTLEN(attr->om):0;if(length>sizeof(io_bytes)||(length&&os_mbuf_copydata(attr->om,0,(int)length,io_bytes)!=0))result=DSR_BAD_RESPONSE;
  }
  (void)dsr_rpc_stream_complete(&stream,pending_session,pending_sequence,handle,result,io_bytes,length,now_ms());
  io_result=result;io_pending=false;io_done=true;
 }unlock();return 0;
}
static int gap_cb(struct ble_gap_event *event,void *arg)
{
 uint32_t generation=(uint32_t)(uintptr_t)arg;lock();
 if(generation!=state.generation||!atomic_load(&active)){unlock();return 0;}
 if(event->type==BLE_GAP_EVENT_CONNECT){connecting=false;if(event->connect.status==0){connection=event->connect.conn_handle;connected=true;}else io_result=DSR_OFFLINE;}
 else if(event->type==BLE_GAP_EVENT_DISCONNECT&&event->disconnect.conn.conn_handle==connection){connected=false;io_pending=false;io_done=true;io_result=DSR_OFFLINE;if(stream.step<DSR_RPC_DONE)dsr_rpc_stream_cancel(&stream);}
 else if(event->type==BLE_GAP_EVENT_ENC_CHANGE&&event->enc_change.conn_handle==connection){/* This B1 deliberately supports only the open setup window. */atomic_store(&security_refused,true);atomic_store(&cancelled,true);io_result=DSR_AUTH_REQUIRED;}
 else if(event->type==BLE_GAP_EVENT_REPEAT_PAIRING||event->type==BLE_GAP_EVENT_PASSKEY_ACTION){atomic_store(&security_refused,true);atomic_store(&cancelled,true);io_result=DSR_AUTH_REQUIRED;unlock();return event->type==BLE_GAP_EVENT_REPEAT_PAIRING?BLE_GAP_REPEAT_PAIRING_IGNORE:0;}
 unlock();return 0;
}
static bool next_token(void)
{if(token_counter==UINT32_MAX)return false;pending_token=++token_counter;io_pending=true;io_done=false;io_result=DSR_BUSY;return true;}
static bool wait_io(uint32_t started)
{
 for(;;){lock();bool done=io_done;unlock();if(done)return true;if(atomic_load(&cancelled)||!dsr_ble_host_healthy()||now_ms()-started>=DSR_BLE_RPC_TIMEOUT_MS)return false;vTaskDelay(pdMS_TO_TICKS(20));}
}
static dsr_result_t connect_peer(uint32_t generation)
{
 if(atomic_load(&cancelled))return DSR_BUSY;
 uint8_t own;if(ble_hs_id_infer_auto(0,&own)!=0)return DSR_OFFLINE;
 ble_addr_t address={.type=peer.address_type};memcpy(address.val,peer.address,6);
 lock();connecting=true;connected=false;closing=false;io_result=DSR_BUSY;connection=UINT16_MAX;unlock();
 int rc=ble_gap_connect(own,&address,10000,NULL,gap_cb,(void *)(uintptr_t)generation);
 if(rc){lock();connecting=false;unlock();return sdk_result(rc);}
 uint32_t started=now_ms();for(;;){lock();bool pending=connecting,ok=connected;dsr_result_t result=io_result;unlock();if(!pending)return ok?DSR_OK:result;if(atomic_load(&cancelled)||!dsr_ble_host_healthy()||now_ms()-started>=12000)return DSR_OFFLINE;vTaskDelay(pdMS_TO_TICKS(20));}
}
static dsr_result_t discover(void)
{
 if(atomic_load(&cancelled))return DSR_BUSY;
 ble_uuid_any_t service;if(ble_uuid_from_str(&service,"5f6d4f53-5f52-5043-5f53-56435f49445f")!=0)return DSR_IO;
 lock();service_first=service_last=data_handle=tx_handle=rx_handle=0;duplicated=false;bool token=next_token();uint32_t id=pending_token;unlock();if(!token)return DSR_IO;
 int rc=ble_gattc_disc_svc_by_uuid(connection,&service.u,service_cb,(void *)(uintptr_t)id);
 if(rc||!wait_io(now_ms()))return rc?sdk_result(rc):DSR_OFFLINE;
 lock();dsr_result_t result=io_result;if(result==DSR_OK){duplicated=false;token=next_token();id=pending_token;}unlock();if(result!=DSR_OK||!token)return result!=DSR_OK?result:DSR_IO;
 rc=ble_gattc_disc_all_chrs(connection,service_first,service_last,characteristic_cb,(void *)(uintptr_t)id);
 if(rc||!wait_io(now_ms()))return rc?sdk_result(rc):DSR_OFFLINE;
 lock();result=io_result;unlock();return result;
}
static dsr_result_t transact(dsr_rpc_method_t method,uint32_t request_id,const char *client,const char *identity,const dsr_settings_t *home,cJSON **result)
{
 *result=NULL;if(atomic_load(&cancelled))return DSR_BUSY;
 char *request=NULL;dsr_result_t status=dsr_rpc_request(method,request_id,client,home,&request);if(status!=DSR_OK)return status;
 uint8_t *response=calloc(DSR_BLE_RPC_RX_MAX+1,1);if(!response){volatile char *p=request;size_t n=strlen(request);while(n--)*p++=0;free(request);return DSR_IO;}
 lock();uint16_t handle=connection;unlock();uint16_t mtu=ble_att_mtu(handle);
 lock();status=connected&&connection==handle?dsr_rpc_stream_begin(&stream,state.generation,handle,mtu,(const uint8_t *)request,strlen(request),response,method==DSR_RPC_WIFI_SET,now_ms()):DSR_OFFLINE;unlock();
 unsigned notifications=0;
 while(status==DSR_OK){
  if(atomic_load(&cancelled)||!dsr_ble_host_healthy()){status=DSR_BUSY;break;}
  dsr_rpc_action_t action;lock();bool next=dsr_rpc_stream_next(&stream,now_ms(),&action);dsr_rpc_step_t step=stream.step;dsr_result_t progress=stream.result;bool token=next&&next_token();uint32_t id=pending_token;
  if(next&&token){pending_sequence=action.sequence;pending_session=action.session;pending_handle=action.kind==DSR_RPC_TX_CTL?tx_handle:action.kind==DSR_RPC_RX_CTL?rx_handle:data_handle;if(stream.write_possible){state.write_possible=true;state.serial++;}}
  unlock();
  if(step==DSR_RPC_DONE){bool notification=false;status=dsr_rpc_response(response,stream.rx_used,request_id,client,identity,result,&notification);if(notification&&++notifications<=4){lock();stream.step=DSR_RPC_RX_CTL;stream.rx_used=stream.rx_length=0;unlock();status=DSR_OK;continue;}break;}
  if(step==DSR_RPC_FAILED){status=progress;break;}
  if(!next||!token){status=DSR_IO;break;}
  int rc=action.kind==DSR_RPC_TX_CTL||action.kind==DSR_RPC_TX_DATA?ble_gattc_write_flat(action.connection,pending_handle,action.bytes,(uint16_t)action.length,attribute_cb,(void *)(uintptr_t)id):ble_gattc_read(action.connection,pending_handle,attribute_cb,(void *)(uintptr_t)id);
  if(rc){status=sdk_result(rc);break;}
  if(!wait_io(stream.started_ms)){status=atomic_load(&cancelled)?DSR_BUSY:DSR_OFFLINE;break;}
  lock();progress=io_result;step=stream.step;unlock();if(progress!=DSR_OK){status=progress;break;}
  if(step==DSR_RPC_RX_CTL)vTaskDelay(pdMS_TO_TICKS(20));
 }
 /* SDK write_flat owns its copied mbuf. Callback arguments are numeric tokens,
  * never pointers to these buffers; cancelled late callbacks are sealed below. */
 lock();dsr_rpc_stream_cancel(&stream);io_pending=false;stream.tx=NULL;stream.rx=NULL;unlock();
 volatile char *p=request;size_t size=strlen(request);while(size--)*p++=0;free(request);volatile uint8_t *b=response;for(size_t i=0;i<DSR_BLE_RPC_RX_MAX+1;i++)b[i]=0;free(response);return status;
}
static dsr_result_t inspect(uint32_t *id,const char *client,dsr_rpc_identity_t *identity)
{
 cJSON *info=NULL,*system=NULL,*methods=NULL,*status=NULL;const char *expected=identity->device.id[0]?identity->device.id:NULL;
 dsr_result_t result=transact(DSR_RPC_INFO,++*id,client,expected,NULL,&info);dsr_rpc_identity_t observed;
 if(result==DSR_OK)result=dsr_rpc_identity(info,&observed);
 if(result==DSR_OK&&expected&&(strcmp(identity->device.id,observed.device.id)||strcmp(identity->device.mac,observed.device.mac)||identity->device.generation!=observed.device.generation))result=DSR_IDENTITY_MISMATCH;
 if(result==DSR_OK)result=transact(DSR_RPC_SYS_CONFIG,++*id,client,observed.device.id,NULL,&system);
 if(result==DSR_OK)result=transact(DSR_RPC_METHODS,++*id,client,observed.device.id,NULL,&methods);
 if(result==DSR_OK)result=transact(DSR_RPC_STATUS,++*id,client,observed.device.id,NULL,&status);
 if(result==DSR_OK)result=dsr_rpc_setup_policy(system,methods,status,&observed);
 if(result==DSR_OK)*identity=observed;
 cJSON_Delete(info);cJSON_Delete(system);cJSON_Delete(methods);cJSON_Delete(status);return result;
}
static void disconnect_peer(void)
{
 publish(DSR_BLE_RPC_CLOSING,DSR_BUSY);lock();closing=true;bool pending=connecting,has_connection=connected;uint16_t handle=connection;unlock();
 if(pending)(void)ble_gap_conn_cancel();
 if(has_connection)(void)ble_gap_terminate(handle,BLE_ERR_REM_USER_CONN_TERM);
 uint32_t started=now_ms();bool quarantined=false;
 for(;;){lock();pending=connecting;has_connection=connected;handle=connection;unlock();if(!pending&&!has_connection)break;
  if(has_connection){(void)ble_gap_terminate(handle,BLE_ERR_REM_USER_CONN_TERM);}
  if(!quarantined&&now_ms()-started>=4000){quarantined=true;publish(DSR_BLE_RPC_QUARANTINED,DSR_IO);}
  vTaskDelay(pdMS_TO_TICKS(100));
 }
 lock();closing=false;io_pending=false;memset(&stream,0,sizeof(stream));unlock();
}
static void run(bool host_ready)
{
 lock();uint32_t generation=state.generation;unlock();uint32_t request_id=0;char client[32];snprintf(client,sizeof(client),"dsr-%08lx-%08lx",(unsigned long)boot_nonce,(unsigned long)generation);dsr_rpc_identity_t identity={0};
 dsr_result_t result=atomic_load(&cancelled)?DSR_BUSY:host_ready?connect_peer(generation):DSR_OFFLINE;
 if(result==DSR_OK){publish(DSR_BLE_RPC_INSPECTING,DSR_BUSY);result=discover();}
 if(result==DSR_OK)result=inspect(&request_id,client,&identity);
 if(result==DSR_OK){lock();state.device=identity.device;strcpy(state.firmware,identity.firmware);state.has_identity=true;state.phase=DSR_BLE_RPC_CONFIRM;state.serial++;unlock();
  uint32_t started=now_ms();for(;;){lock();bool accept=confirmed;unlock();if(accept)break;if(atomic_load(&cancelled)||!dsr_ble_host_healthy()||now_ms()-started>=120000){result=DSR_BUSY;break;}vTaskDelay(pdMS_TO_TICKS(20));}
 }
 dsr_settings_t home={0};
 if(result==DSR_OK){result=dsr_settings_load(&home)==ESP_OK&&dsr_wifi_settings_valid(&home)&&dsr_wifi_connected()&&!strcmp(home.ssid,requested_home)?DSR_OK:DSR_OFFLINE;}
 if(result==DSR_OK)result=inspect(&request_id,client,&identity); /* setup/security rechecked immediately before secrets */
 if(result==DSR_OK){cJSON *written=NULL;result=transact(DSR_RPC_WIFI_SET,++request_id,client,identity.device.id,&home,&written);if(result==DSR_OK){const cJSON *restart=cJSON_GetObjectItemCaseSensitive(written,"restart_required");if(!cJSON_IsFalse(restart))result=DSR_UNSUPPORTED;}cJSON_Delete(written);}
 if(result==DSR_OK){publish(DSR_BLE_RPC_VERIFYING,DSR_BUSY);uint32_t started=now_ms();do{cJSON *station=NULL;result=transact(DSR_RPC_WIFI_STATUS,++request_id,client,identity.device.id,NULL,&station);if(result==DSR_OK)result=dsr_rpc_station(station,&home,identity.device.host);cJSON_Delete(station);if(result==DSR_OK||atomic_load(&cancelled))break;if(result==DSR_BAD_RESPONSE)vTaskDelay(pdMS_TO_TICKS(1000));else break;}while(now_ms()-started<45000);}
 volatile unsigned char *secret=(volatile unsigned char *)&home;for(size_t i=0;i<sizeof(home);i++)secret[i]=0;
 disconnect_peer();if(atomic_load(&security_refused))result=DSR_AUTH_REQUIRED;lock();bool uncertain=state.write_possible&&(result==DSR_OFFLINE||result==DSR_BAD_RESPONSE);if(result==DSR_OK||uncertain)state.device=identity.device;state.result=result;state.phase=atomic_load(&security_refused)?DSR_BLE_RPC_ERROR:!atomic_load(&cancelled)&&(result==DSR_OK||uncertain)?DSR_BLE_RPC_LAN_READY:atomic_load(&cancelled)||result==DSR_BUSY?DSR_BLE_RPC_CANCELLED:DSR_BLE_RPC_ERROR;state.serial++;atomic_store(&active,false);unlock();dsr_activity_end();
}
dsr_result_t dsr_ble_rpc_start(const dsr_ble_device_t *candidate)
{
 if(!candidate||candidate->kind!=DSR_BLE_SHELLY_RPC||!(candidate->evidence&DSR_BLE_RPC_HINTS)||candidate->address_type>1||now_ms()-candidate->seen_ms>30000)return DSR_INVALID;
 if(dsr_ap_sensitive()||dsr_wifi_temporary_active()||!dsr_wifi_connected())return DSR_BUSY;
 if(!mutex){mutex=xSemaphoreCreateMutex();if(!mutex)return DSR_IO;}
 lock();if(atomic_load(&active)||state.generation==UINT32_MAX){unlock();return DSR_BUSY;}
 if(!dsr_activity_begin()){unlock();return DSR_BUSY;}
 if(!boot_nonce){boot_nonce=esp_random();if(!boot_nonce)boot_nonce=1;}
 uint32_t generation=state.generation+1;state=(dsr_ble_rpc_snapshot_t){.generation=generation,.phase=DSR_BLE_RPC_CONNECTING,.result=DSR_BUSY,.serial=state.serial+1};peer=*candidate;confirmed=false;requested_home[0]=0;atomic_store(&cancelled,false);atomic_store(&security_refused,false);atomic_store(&active,true);unlock();
 if(!dsr_ble_manager_rpc(run)){lock();atomic_store(&active,false);state.phase=DSR_BLE_RPC_ERROR;state.result=DSR_BUSY;state.serial++;unlock();dsr_activity_end();return DSR_BUSY;}return DSR_OK;
}
