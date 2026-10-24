#include "dsr_blu.h"
#include "dsr_ble_shared.h"
#include "dsr_activity.h"
#include "esp_timer.h"
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_sm.h"
#include "host/ble_store.h"
#include "store/config/ble_store_config.h"
#include "os/os_mbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ble_store_config_init(void);
typedef struct {
 dsr_blu_snapshot_t view;
 dsr_ble_device_t candidate;
 ble_addr_t identity;
 uint32_t connection_token,io_token,pin,value,expected,started;
 uint16_t connection,service_first,service_last,handles[DSR_BLU_SETTINGS],key_handle,pending_handle;
 uint8_t data[32],length,pin_action,action,field;
 bool connecting,connected,closing,cancelled,pin_ready,secured,io_pending,io_done,duplicate,have_identity,discard;
 dsr_result_t io_result;
 uint8_t key[16];
 uint32_t raw[DSR_BLU_SETTINGS];
} blu_t;
static blu_t *blu;
static SemaphoreHandle_t mutex;
static uint32_t token_counter,generation_counter;
static uint32_t now_ms(void){return (uint32_t)(esp_timer_get_time()/1000);}
static void wipe(void *p,size_t n){volatile unsigned char *v=p;while(n--)*v++=0;}
static bool lock(void){return mutex&&xSemaphoreTake(mutex,portMAX_DELAY)==pdTRUE;}
static void unlock(void){xSemaphoreGive(mutex);}
static void publish(dsr_blu_phase_t phase,dsr_result_t result)
{if(!lock())return;if(blu){blu->view.phase=phase;blu->view.result=result;blu->view.serial++;}unlock();}
static dsr_result_t status(int rc)
{return rc==0?DSR_OK:rc==BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN)||rc==BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC)?DSR_AUTH_REQUIRED:rc==BLE_HS_ESTORE_CAP?DSR_FULL:DSR_IO;}
static bool next_token(uint32_t *token)
{if(token_counter==UINT32_MAX)return false;*token=++token_counter;return true;}
static bool io_matches(uint16_t handle,void *arg)
{return blu&&blu->view.active&&!blu->closing&&!blu->cancelled&&blu->connected&&blu->connection==handle&&blu->io_pending&&(uint32_t)(uintptr_t)arg==blu->io_token;}
/* A full store fails instead of automatically evicting another Shelly bond. */
static int store_full(struct ble_store_status_event *event,void *arg)
{(void)event;(void)arg;return BLE_HS_ESTORE_CAP;}
static int store_write(int kind,const union ble_store_value *value)
{dsr_activity_data_changed();return ble_store_config_write(kind,value);}
static int store_delete(int kind,const union ble_store_key *key)
{dsr_activity_data_changed();return ble_store_config_delete(kind,key);}
void dsr_blu_host_setup(void)
{
 ble_hs_cfg.sm_io_cap=BLE_HS_IO_KEYBOARD_ONLY;ble_hs_cfg.sm_bonding=1;ble_hs_cfg.sm_mitm=0;ble_hs_cfg.sm_sc=1;
 ble_hs_cfg.sm_our_key_dist=BLE_SM_PAIR_KEY_DIST_ENC|BLE_SM_PAIR_KEY_DIST_ID;ble_hs_cfg.sm_their_key_dist=BLE_SM_PAIR_KEY_DIST_ENC|BLE_SM_PAIR_KEY_DIST_ID;
 ble_hs_cfg.store_status_cb=store_full;ble_store_config_init();
 /* A failed NVS commit can still have changed Flash: attempted writes also
  * invalidate a pre-existing maintenance grant, without exposing key bytes. */
 ble_hs_cfg.store_write_cb=store_write;ble_hs_cfg.store_delete_cb=store_delete;
}
static int gap(struct ble_gap_event *event,void *arg)
{
 if(!lock())return 0;
 if(!blu||!blu->view.active||(uint32_t)(uintptr_t)arg!=blu->connection_token){unlock();return event->type==BLE_GAP_EVENT_REPEAT_PAIRING?BLE_GAP_REPEAT_PAIRING_IGNORE:0;}
 if(event->type==BLE_GAP_EVENT_CONNECT){
  blu->connecting=false;
  if(event->connect.status==0){blu->connected=true;blu->connection=event->connect.conn_handle;}else blu->io_result=DSR_OFFLINE;
 }else if(event->type==BLE_GAP_EVENT_DISCONNECT&&event->disconnect.conn.conn_handle==blu->connection){blu->connected=false;blu->secured=false;blu->io_pending=false;blu->io_done=true;blu->io_result=DSR_OFFLINE;}
 else if(event->type==BLE_GAP_EVENT_ENC_CHANGE&&event->enc_change.conn_handle==blu->connection&&!blu->closing&&!blu->cancelled){
  struct ble_gap_conn_desc d;
  bool ok=event->enc_change.status==0&&ble_gap_conn_find(blu->connection,&d)==0&&d.sec_state.encrypted&&d.sec_state.bonded&&d.sec_state.key_size==16&&(!blu->candidate.encrypted||d.sec_state.authenticated);
  blu->secured=ok;blu->view.bonded=ok;blu->io_result=ok?DSR_OK:DSR_AUTH_REQUIRED;
  if(ok){blu->identity=d.peer_id_addr;blu->have_identity=true;}
  else blu->cancelled=true;
  blu->view.serial++;
 }else if(event->type==BLE_GAP_EVENT_PASSKEY_ACTION&&event->passkey.conn_handle==blu->connection&&!blu->closing&&!blu->cancelled){
  if(event->passkey.params.action==BLE_SM_IOACT_INPUT){blu->pin_action=event->passkey.params.action;blu->view.phase=DSR_BLU_PIN;blu->view.serial++;}
  else {blu->io_result=DSR_AUTH_REQUIRED;blu->cancelled=true;}
 }else if(event->type==BLE_GAP_EVENT_REPEAT_PAIRING){blu->io_result=DSR_AUTH_REQUIRED;blu->cancelled=true;unlock();return BLE_GAP_REPEAT_PAIRING_IGNORE;}
 unlock();return 0;
}
static int service(uint16_t handle,const struct ble_gatt_error *error,const struct ble_gatt_svc *svc,void *arg)
{
 if(!lock())return 0;
 if(io_matches(handle,arg)){
  if(error->status==0&&svc){if(blu->service_first||!svc->start_handle||svc->end_handle<=svc->start_handle)blu->duplicate=true;else{blu->service_first=svc->start_handle;blu->service_last=svc->end_handle;}}
  else{blu->io_result=error->status==BLE_HS_EDONE?(blu->service_first&&!blu->duplicate?DSR_OK:DSR_UNSUPPORTED):status(error->status);blu->io_pending=false;blu->io_done=true;}
 }unlock();return 0;
}
static int characteristic(uint16_t handle,const struct ble_gatt_error *error,const struct ble_gatt_chr *chr,void *arg)
{
 if(!lock())return 0;
 if(io_matches(handle,arg)){
  if(error->status==0&&chr){
   for(unsigned i=0;i<=DSR_BLU_SETTINGS;i++){
    const dsr_blu_setting_t *f=dsr_blu_setting(i);const char *text=i==DSR_BLU_SETTINGS?"eb0fb41b-af4b-4724-a6f9-974f55aba81a":f?f->uuid:NULL;
    if(!text||(f&&f->model&&f->model!=blu->view.model)||(i==0&&blu->view.model==3))continue;
    ble_uuid_any_t uuid;if(ble_uuid_from_str(&uuid,text)||ble_uuid_cmp(&uuid.u,&chr->uuid.u))continue;
    uint16_t *target=i==DSR_BLU_SETTINGS?&blu->key_handle:&blu->handles[i];
    if(*target||chr->val_handle<=blu->service_first||chr->val_handle>blu->service_last||chr->def_handle>=chr->val_handle)blu->duplicate=true;
    else if((chr->properties&BLE_GATT_CHR_F_READ)&&(i==DSR_BLU_SETTINGS||(chr->properties&BLE_GATT_CHR_F_WRITE)))*target=chr->val_handle;
   }
  }else {blu->io_result=error->status==BLE_HS_EDONE?(!blu->duplicate?DSR_OK:DSR_BAD_RESPONSE):status(error->status);blu->io_pending=false;blu->io_done=true;}
 }unlock();return 0;
}
static int attribute(uint16_t handle,const struct ble_gatt_error *error,struct ble_gatt_attr *attr,void *arg)
{
 if(!lock())return 0;
 if(io_matches(handle,arg)){
  blu->io_result=status(error->status);blu->length=0;
  if(blu->io_result==DSR_OK){unsigned n=attr&&attr->om?OS_MBUF_PKTLEN(attr->om):0;
   if(!attr||attr->handle!=blu->pending_handle||n>sizeof(blu->data)||(!attr->om&&n)||(n&&os_mbuf_copydata(attr->om,0,(int)n,blu->data)))blu->io_result=DSR_BAD_RESPONSE;
   else blu->length=(uint8_t)n;
  }
  blu->io_pending=false;blu->io_done=true;
 }unlock();return 0;
}
static bool alive(void)
{if(!lock())return false;bool ok=blu&&blu->view.active&&!blu->cancelled&&!blu->closing&&now_ms()-blu->started<DSR_BLU_OPERATION_MS;unlock();return ok&&dsr_ble_host_healthy();}
static void pin_tick(void)
{
 if(!lock())return;
 bool ready=blu&&blu->pin_ready&&!blu->cancelled&&blu->connected;struct ble_sm_io io={0};uint16_t handle=0;
 if(ready){io.action=blu->pin_action;io.passkey=blu->pin;handle=blu->connection;blu->pin=0;blu->pin_ready=false;blu->pin_action=0;blu->view.phase=DSR_BLU_PAIRING;blu->view.serial++;}unlock();
 if(ready){int rc=ble_sm_inject_io(handle,&io);wipe(&io,sizeof(io));if(rc){lock();blu->io_result=DSR_AUTH_REQUIRED;blu->cancelled=true;unlock();}}
}
static dsr_result_t wait_io(int rc)
{
 if(rc)return status(rc);
 uint32_t started=now_ms();
 while(alive()&&now_ms()-started<10000){lock();bool done=blu->io_done;dsr_result_t result=blu->io_result;unlock();if(done)return result;vTaskDelay(pdMS_TO_TICKS(20));}
 lock();blu->io_pending=false;unlock();return DSR_OFFLINE;
}
static uint32_t begin_io(void)
{
 lock();bool ok=next_token(&blu->io_token);blu->io_pending=ok;blu->io_done=false;blu->io_result=DSR_BUSY;blu->length=0;uint32_t token=ok?blu->io_token:0;unlock();return token;
}
static dsr_result_t discover(void)
{
 ble_uuid_any_t uuid;if(ble_uuid_from_str(&uuid,"de8a5aac-a99b-c315-0c80-60d4cbb51225"))return DSR_IO;
 uint32_t token=begin_io();if(!token)return DSR_IO;
 dsr_result_t r=wait_io(ble_gattc_disc_svc_by_uuid(blu->connection,&uuid.u,service,(void *)(uintptr_t)token));if(r!=DSR_OK)return r;
 token=begin_io();if(!token)return DSR_IO;
 return wait_io(ble_gattc_disc_all_chrs(blu->connection,blu->service_first,blu->service_last,characteristic,(void *)(uintptr_t)token));
}
static dsr_result_t read_handle(uint16_t handle)
{if(!alive())return DSR_OFFLINE;uint32_t token=begin_io();if(!token||!handle)return DSR_UNSUPPORTED;lock();blu->pending_handle=handle;unlock();return wait_io(ble_gattc_read(blu->connection,handle,attribute,(void *)(uintptr_t)token));}
static dsr_result_t read_fields(void)
{
 publish(DSR_BLU_READING,DSR_BUSY);uint32_t available=0;uint32_t values[DSR_BLU_SETTINGS]={0};
 for(unsigned i=0;i<DSR_BLU_SETTINGS;i++)if(blu->handles[i]){
  dsr_result_t r=read_handle(blu->handles[i]);if(r!=DSR_OK)return r;const dsr_blu_setting_t *f=dsr_blu_setting(i);uint32_t value=0;
  if(!f||blu->length!=f->bytes)return DSR_BAD_RESPONSE;
  for(unsigned j=0;j<f->bytes;j++)value|=(uint32_t)blu->data[j]<<(8*j);
  blu->raw[i]=value;if(f->mask)value=(value&f->mask)?1:0;
  if(!dsr_blu_value_valid(i,blu->view.model,value))return DSR_BAD_RESPONSE;
  available|=1u<<i;values[i]=value;
 }
 if(blu->candidate.encrypted){dsr_result_t r=read_handle(blu->key_handle);if(r!=DSR_OK||blu->length!=16)return r==DSR_OK?DSR_BAD_RESPONSE:r;memcpy(blu->key,blu->data,16);}
 lock();blu->view.available=available;memcpy(blu->view.values,values,sizeof(values));blu->view.key_available=blu->candidate.encrypted;unlock();return DSR_OK;
}
static dsr_result_t secure_connect(void)
{
 if(!alive())return DSR_OFFLINE;
 uint8_t own;if(ble_hs_id_infer_auto(0,&own))return DSR_IO;
 lock();blu->connecting=true;blu->io_result=DSR_BUSY;ble_addr_t peer={.type=blu->candidate.address_type};memcpy(peer.val,blu->candidate.address,6);uint32_t token=blu->connection_token;unlock();
 int rc=ble_gap_connect(own,&peer,10000,NULL,gap,(void *)(uintptr_t)token);if(rc){lock();blu->connecting=false;unlock();return status(rc);}
 uint32_t started=now_ms();bool connected=false;
 while(alive()&&now_ms()-started<11000){lock();bool pending=blu->connecting;connected=blu->connected;unlock();if(!pending)break;vTaskDelay(pdMS_TO_TICKS(20));}
 if(!connected)return DSR_OFFLINE;
 /* No automatic peer selection or deletion. The user selected the candidate.
  * New bonding requires its advertised pairing window; stored bonds reconnect. */
 struct ble_gap_conn_desc d;if(ble_gap_conn_find(blu->connection,&d))return DSR_IO;
 ble_addr_t peers[16];int count=0;rc=ble_store_util_bonded_peers(peers,&count,16);bool bonded=false;
 if(rc||count<0||count>16)return DSR_IO;
 for(int i=0;i<count;i++)if(peers[i].type==d.peer_id_addr.type&&!memcmp(peers[i].val,d.peer_id_addr.val,6))bonded=true;
 if(!bonded&&(!blu->candidate.has_flags||!(blu->candidate.flags&DSR_BLE_FLAG_PAIRING)))return DSR_AUTH_REQUIRED;
 if(!bonded&&count==16)return DSR_FULL;
 ble_hs_cfg.sm_mitm=blu->candidate.encrypted?1:0;
 publish(DSR_BLU_PAIRING,DSR_BUSY);rc=ble_gap_security_initiate(blu->connection);if(rc)return status(rc);
 started=now_ms();
 while(alive()&&now_ms()-started<60000){pin_tick();lock();bool secured=blu->secured;dsr_result_t r=blu->io_result;unlock();if(secured)return DSR_OK;if(r!=DSR_BUSY&&r!=DSR_OK)return r;vTaskDelay(pdMS_TO_TICKS(20));}
 return DSR_AUTH_REQUIRED;
}
static bool disconnect_peer(void)
{
 publish(DSR_BLU_CLOSING,DSR_BUSY);lock();blu->closing=true;blu->io_pending=false;bool connecting=blu->connecting;unlock();if(connecting)(void)ble_gap_conn_cancel();
 uint32_t started=now_ms();
 for(;;){lock();bool connected=blu->connected;connecting=blu->connecting;uint16_t handle=blu->connection;unlock();if(!connected&&!connecting)break;
  if(connected&&ble_gap_terminate(handle,BLE_ERR_REM_USER_CONN_TERM)==BLE_HS_ENOTCONN){lock();if(blu->connection==handle)blu->connected=false;unlock();continue;}
  if(now_ms()-started>=DSR_BLE_CLOSE_MS){dsr_ble_manager_quarantine();return false;}
  vTaskDelay(pdMS_TO_TICKS(100));
 }
 lock();blu->closing=false;unlock();return true;
}
static void run(bool host_ready)
{
 dsr_result_t r=host_ready&&alive()?DSR_OK:DSR_OFFLINE;bool closed=true;
 if(r==DSR_OK&&blu->action==2){
  if(!blu->have_identity)r=DSR_INVALID;
  else {r=status(ble_store_util_delete_peer(&blu->identity));if(r==DSR_OK){ble_addr_t peers[16];int count=0;if(ble_store_util_bonded_peers(peers,&count,16))r=DSR_IO;for(int i=0;i<count;i++)if(peers[i].type==blu->identity.type&&!memcmp(peers[i].val,blu->identity.val,6))r=DSR_IO;}}
  lock();if(r==DSR_OK){blu->view.bonded=false;blu->view.key_available=false;wipe(blu->key,sizeof(blu->key));}unlock();
 }else {
  if(r==DSR_OK)r=secure_connect();
  if(r==DSR_OK)r=discover();
  if(r==DSR_OK)r=read_fields();
  if(r==DSR_OK&&blu->action==1){
   unsigned field=blu->field;const dsr_blu_setting_t *f=dsr_blu_setting(field);
   if(!f||!(blu->view.available&(1u<<field)))r=DSR_UNSUPPORTED;
   else if(blu->view.values[field]!=blu->expected)r=DSR_CONFLICT;
   else if(field==8&&blu->value&&(!(blu->view.available&1u)||!blu->view.values[0]))r=DSR_INVALID;
   else if(blu->view.model==8&&((field==25&&(!(blu->view.available&(1u<<26))||blu->value>=blu->view.values[26]))||(field==26&&(!(blu->view.available&(1u<<25))||blu->value<=blu->view.values[25]))))r=DSR_INVALID;
   else {
    publish(DSR_BLU_WRITING,DSR_BUSY);uint32_t raw=f->mask?(blu->raw[field]&~(uint32_t)f->mask)|(blu->value?f->mask:0):blu->value;uint8_t bytes[4]={0};for(unsigned i=0;i<f->bytes;i++)bytes[i]=(uint8_t)(raw>>(8*i));
    uint32_t token=begin_io();lock();blu->pending_handle=blu->handles[field];blu->view.write_possible=true;blu->view.uncertain=true;unlock();
    r=token?wait_io(ble_gattc_write_flat(blu->connection,blu->handles[field],bytes,f->bytes,attribute,(void *)(uintptr_t)token)):DSR_IO;
    lock();bool linked=blu->connected&&blu->secured;unlock();
    if(alive()&&linked){r=read_fields();if(r==DSR_OK&&blu->view.values[field]!=blu->value)r=DSR_BAD_RESPONSE;if(r==DSR_OK){lock();blu->view.uncertain=false;unlock();}}
   }
  }
  closed=disconnect_peer();if(!closed)r=DSR_IO;
 }
 lock();bool cancelled=blu->cancelled;if(cancelled&&blu->io_result==DSR_AUTH_REQUIRED)r=DSR_AUTH_REQUIRED;blu->view.active=false;blu->view.result=r;blu->view.phase=!closed?DSR_BLU_QUARANTINED:r==DSR_OK?DSR_BLU_DONE:cancelled&&r!=DSR_AUTH_REQUIRED?DSR_BLU_CANCELLED:DSR_BLU_FAILED;blu->view.serial++;blu->pin=0;blu->pin_ready=false;wipe(blu->data,sizeof(blu->data));
 if(blu->discard){wipe(blu,sizeof(*blu));free(blu);blu=NULL;}unlock();dsr_activity_end();
}
static dsr_result_t queue_operation(void)
{
 if(!next_token(&blu->connection_token)||!dsr_activity_begin())return DSR_BUSY;
 blu->started=now_ms();blu->view.active=true;blu->view.phase=DSR_BLU_CONNECTING;blu->view.result=DSR_BUSY;blu->view.serial++;blu->cancelled=blu->discard=blu->secured=blu->io_done=blu->io_pending=blu->duplicate=false;blu->connecting=blu->connected=blu->closing=false;blu->connection=UINT16_MAX;blu->service_first=blu->service_last=blu->key_handle=0;memset(blu->handles,0,sizeof(blu->handles));
 unlock();bool queued=dsr_ble_manager_rpc(run);lock();
 if(!queued){blu->view.active=false;blu->view.result=DSR_BUSY;blu->view.phase=DSR_BLU_FAILED;dsr_activity_end();return DSR_BUSY;}return DSR_OK;
}
dsr_result_t dsr_blu_start(const dsr_ble_device_t *candidate)
{
 if(!dsr_blu_supported(candidate)||candidate->address_type>1||now_ms()-candidate->seen_ms>30000)return DSR_INVALID;
 if(!mutex)mutex=xSemaphoreCreateMutex();
 if(!lock())return DSR_IO;
 if(blu&&blu->view.active){unlock();return DSR_BUSY;}
 if(generation_counter==UINT32_MAX){unlock();return DSR_IO;}
 if(blu){wipe(blu,sizeof(*blu));free(blu);blu=NULL;}
#ifdef ESP_PLATFORM
 blu=heap_caps_calloc(1,sizeof(*blu),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
 blu=calloc(1,sizeof(*blu));
#endif
 if(!blu){unlock();return DSR_IO;}blu->candidate=*candidate;blu->view.model=candidate->model_id;blu->view.generation=++generation_counter;
 dsr_result_t r=queue_operation();unlock();return r;
}
dsr_result_t dsr_blu_write(uint32_t generation,unsigned field,uint32_t value)
{
 if(!lock())return DSR_INVALID;
 dsr_result_t r=DSR_INVALID;
 if(blu&&!blu->view.active&&blu->view.generation==generation&&blu->view.bonded&&field<DSR_BLU_SETTINGS&&(blu->view.available&(1u<<field))&&dsr_blu_value_valid(field,blu->view.model,value)){
  blu->action=1;blu->field=(uint8_t)field;blu->value=value;blu->expected=blu->view.values[field];blu->view.write_possible=blu->view.uncertain=false;r=queue_operation();
 }unlock();return r;
}
dsr_result_t dsr_blu_forget(uint32_t generation)
{
 if(!lock())return DSR_INVALID;
 dsr_result_t r=DSR_INVALID;
 if(blu&&!blu->view.active&&blu->view.generation==generation&&blu->have_identity){blu->action=2;r=queue_operation();}unlock();return r;
}
dsr_result_t dsr_blu_pin(uint32_t generation,const char *pin)
{
 if(!pin||strlen(pin)!=6)return DSR_INVALID;
 uint32_t value=0;for(unsigned i=0;i<6;i++){if(pin[i]<'0'||pin[i]>'9')return DSR_INVALID;value=value*10+(unsigned)(pin[i]-'0');}
 if(!lock())return DSR_INVALID;
 dsr_result_t r=DSR_BUSY;if(blu&&blu->view.active&&!blu->cancelled&&blu->view.generation==generation&&blu->view.phase==DSR_BLU_PIN&&blu->pin_action==BLE_SM_IOACT_INPUT&&!blu->pin_ready){blu->pin=value;blu->pin_ready=true;r=DSR_OK;}unlock();return r;
}
void dsr_blu_cancel(uint32_t generation)
{if(!lock())return;if(blu&&blu->view.generation==generation)blu->cancelled=true;unlock();}
void dsr_blu_snapshot(dsr_blu_snapshot_t *out)
{if(!out)return;memset(out,0,sizeof(*out));if(!lock())return;if(blu)*out=blu->view;unlock();}
bool dsr_blu_active(void)
{if(!lock())return false;bool active=blu&&blu->view.active;unlock();return active;}
void dsr_blu_close(void)
{if(!lock())return;if(blu){if(blu->view.active)blu->cancelled=blu->discard=true;else{wipe(blu,sizeof(*blu));free(blu);blu=NULL;}}unlock();}
bool dsr_blu_gateway_key(uint32_t generation,char address[18],char key[33])
{
 if(!address||!key||!lock())return false;
 bool ok=blu&&!blu->view.active&&blu->view.phase==DSR_BLU_DONE&&blu->view.result==DSR_OK&&blu->view.generation==generation&&blu->view.bonded&&blu->have_identity&&(!blu->candidate.encrypted||blu->view.key_available);
 if(ok){const uint8_t *a=blu->identity.val;snprintf(address,18,"%02x:%02x:%02x:%02x:%02x:%02x",a[5],a[4],a[3],a[2],a[1],a[0]);key[0]=0;if(blu->candidate.encrypted)for(unsigned i=0;i<16;i++)snprintf(key+i*2,3,"%02x",blu->key[i]);}unlock();return ok;
}
