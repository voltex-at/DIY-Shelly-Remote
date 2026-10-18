/* Genuine BLU central against deterministic NimBLE callbacks. Does not prove
 * RF, SMP cryptography, NVS persistence or operation of a physical sensor. */
#include "dsr_blu.h"
#include "../components/dsr_platform/dsr_ble_shared.h"
#include "../components/dsr_platform/dsr_activity_internal.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "os/os_mbuf.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
struct ble_hs_cfg ble_hs_cfg;
static uint32_t clock_ms;
static bool held,queued,connected,secured,encrypted,has_bond,pin_injected;
static unsigned writes,deletes,mode;
static uint32_t values[DSR_BLU_SETTINGS];
static void (*operation)(bool);
static int (*gap_cb)(struct ble_gap_event *,void *);
static void *gap_arg;
static ble_addr_t identity={.type=0,.val={1,2,3,4,5,6}};
enum {NORMAL,PIN_MODE,CANCEL_PIN,DUPLICATE,WRONG_ATTRIBUTE,WRITE_TIMEOUT,WRONG_READBACK,REPEAT,FULL,BAD_LENGTH,CANCEL_CONNECT,CHANGED_FIELD,OLD_CALLBACK,DISCONNECT_WRITE};
SemaphoreHandle_t xSemaphoreCreateMutex(void){return (void *)1;}
int xSemaphoreTake(SemaphoreHandle_t m,TickType_t t){(void)t;CHECK(m==(void *)1&&!held);held=true;return 1;}
int xSemaphoreGive(SemaphoreHandle_t m){CHECK(m==(void *)1&&held);held=false;return 1;}
int64_t esp_timer_get_time(void){return (int64_t)clock_ms*1000;}
bool dsr_ble_host_healthy(void){return true;}
bool dsr_ble_manager_rpc(void (*fn)(bool)){if(queued)return false;queued=true;operation=fn;return true;}
void ble_store_config_init(void){}
int ble_hs_id_infer_auto(int p,uint8_t *own){CHECK(!p);*own=0;return 0;}
static void gap_event(int type)
{struct ble_gap_event e={.type=type};if(type==BLE_GAP_EVENT_CONNECT){e.connect.conn_handle=7;}else if(type==BLE_GAP_EVENT_DISCONNECT)e.disconnect.conn.conn_handle=7;else if(type==BLE_GAP_EVENT_ENC_CHANGE)e.enc_change.conn_handle=7;else if(type==BLE_GAP_EVENT_PASSKEY_ACTION){e.passkey.conn_handle=7;e.passkey.params.action=BLE_SM_IOACT_INPUT;}int rc=gap_cb(&e,gap_arg);if(type==BLE_GAP_EVENT_REPEAT_PAIRING)CHECK(rc==BLE_GAP_REPEAT_PAIRING_IGNORE);}
int ble_gap_connect(uint8_t own,const ble_addr_t *peer,int32_t timeout,const struct ble_gap_conn_params *params,int (*cb)(struct ble_gap_event *,void *),void *arg)
{CHECK(!own&&timeout==10000&&!params&&!memcmp(peer->val,identity.val,6));gap_cb=cb;gap_arg=arg;if(mode!=CANCEL_CONNECT){connected=true;gap_event(BLE_GAP_EVENT_CONNECT);}return 0;}
int ble_gap_conn_cancel(void){struct ble_gap_event e={.type=BLE_GAP_EVENT_CONNECT};e.connect.status=1;gap_cb(&e,gap_arg);return 0;}
int ble_gap_terminate(uint16_t handle,uint8_t reason){CHECK(handle==7&&reason==BLE_ERR_REM_USER_CONN_TERM);connected=false;secured=false;gap_event(BLE_GAP_EVENT_DISCONNECT);return 0;}
int ble_gap_conn_find(uint16_t handle,struct ble_gap_conn_desc *out)
{CHECK(handle==7);*out=(struct ble_gap_conn_desc){.peer_id_addr=identity,.sec_state={.encrypted=secured,.authenticated=encrypted&&pin_injected,.bonded=has_bond,.key_size=16}};return 0;}
int ble_gap_security_initiate(uint16_t handle)
{CHECK(handle==7);if(mode==REPEAT){gap_event(BLE_GAP_EVENT_REPEAT_PAIRING);return 0;}if(encrypted&&!has_bond){gap_event(BLE_GAP_EVENT_PASSKEY_ACTION);return 0;}secured=true;has_bond=true;gap_event(BLE_GAP_EVENT_ENC_CHANGE);return 0;}
int ble_sm_inject_io(uint16_t handle,struct ble_sm_io *io)
{CHECK(handle==7&&io->action==BLE_SM_IOACT_INPUT&&io->passkey==123456);pin_injected=true;has_bond=secured=true;gap_event(BLE_GAP_EVENT_ENC_CHANGE);return 0;}
int ble_store_util_bonded_peers(ble_addr_t *peers,int *count,int maximum)
{CHECK(maximum==16);*count=mode==FULL?16:has_bond?1:0;for(int i=0;i<*count;i++){peers[i]=identity;if(mode==FULL)peers[i].val[0]=(uint8_t)(20+i);}return 0;}
int ble_store_util_delete_peer(const ble_addr_t *peer){CHECK(peer->type==identity.type&&!memcmp(peer->val,identity.val,6));deletes++;has_bond=false;return 0;}
int ble_uuid_from_str(ble_uuid_any_t *uuid,const char *text){CHECK(strlen(text)==36);uuid->u.type=128;strcpy(uuid->u.text,text);return 0;}
int ble_uuid_cmp(const ble_uuid_t *a,const ble_uuid_t *b){return strcmp(a->text,b->text);}
int ble_gattc_disc_svc_by_uuid(uint16_t handle,const ble_uuid_t *uuid,ble_gatt_disc_svc_fn *cb,void *arg)
{CHECK(handle==7&&!strcmp(uuid->text,"de8a5aac-a99b-c315-0c80-60d4cbb51225"));struct ble_gatt_svc s={.start_handle=1,.end_handle=100};struct ble_gatt_error ok={0},done={.status=BLE_HS_EDONE};cb(handle,&ok,&s,arg);if(mode==DUPLICATE)cb(handle,&ok,&s,arg);cb(handle,&done,NULL,arg);return 0;}
int ble_gattc_disc_all_chrs(uint16_t handle,uint16_t first,uint16_t last,ble_gatt_chr_fn *cb,void *arg)
{CHECK(handle==7&&first==1&&last==100);struct ble_gatt_error ok={0},done={.status=BLE_HS_EDONE};for(unsigned i=0;i<=DSR_BLU_SETTINGS;i++){const dsr_blu_setting_t *f=dsr_blu_setting(i);if(!f&&i!=DSR_BLU_SETTINGS)continue;bool duplicate=false;for(unsigned j=0;f&&j<i;j++){const dsr_blu_setting_t *previous=dsr_blu_setting(j);if(previous&&!strcmp(previous->uuid,f->uuid))duplicate=true;}if(duplicate)continue;struct ble_gatt_chr c={.def_handle=(uint16_t)(i==DSR_BLU_SETTINGS?89:1+2*i),.val_handle=(uint16_t)(i==DSR_BLU_SETTINGS?90:2+2*i),.properties=BLE_GATT_CHR_F_READ|BLE_GATT_CHR_F_WRITE};ble_uuid_from_str(&c.uuid,f?f->uuid:"eb0fb41b-af4b-4724-a6f9-974f55aba81a");cb(handle,&ok,&c,arg);}cb(handle,&done,NULL,arg);return 0;}
static void attribute(ble_gatt_attr_fn *cb,void *arg,uint16_t handle,const uint8_t *bytes,unsigned n)
{struct os_mbuf b={.data=bytes,.length=n};struct ble_gatt_attr a={.handle=mode==WRONG_ATTRIBUTE?99:handle,.om=n?&b:NULL};struct ble_gatt_error ok={0};if(mode==OLD_CALLBACK){void *old=(void *)((uintptr_t)arg-1);cb(7,&ok,&a,old);}cb(7,&ok,&a,arg);}
int ble_gattc_read(uint16_t handle,uint16_t attribute_handle,ble_gatt_attr_fn *cb,void *arg)
{CHECK(handle==7&&connected&&secured);uint8_t bytes[16];if(attribute_handle==90){memset(bytes,0x42,sizeof(bytes));attribute(cb,arg,attribute_handle,bytes,16);}else{unsigned index=(attribute_handle-2)/2;const dsr_blu_setting_t *f=dsr_blu_setting(index);CHECK(f);uint32_t value=values[index];if(mode==CHANGED_FIELD&&index==2)value=90;for(unsigned i=0;i<f->bytes;i++)bytes[i]=(uint8_t)(value>>(8*i));attribute(cb,arg,attribute_handle,bytes,mode==BAD_LENGTH?3:f->bytes);}return 0;}
int ble_gattc_write_flat(uint16_t handle,uint16_t attribute_handle,const void *bytes,uint16_t length,ble_gatt_attr_fn *cb,void *arg)
{CHECK(handle==7&&connected&&secured);writes++;unsigned index=(attribute_handle-2)/2;const dsr_blu_setting_t *f=dsr_blu_setting(index);CHECK(f&&length==f->bytes);uint32_t value=0;for(unsigned i=0;i<length;i++)value|=(uint32_t)((const uint8_t *)bytes)[i]<<(8*i);if(mode!=WRONG_READBACK)values[index]=value;if(mode==DISCONNECT_WRITE){connected=secured=false;gap_event(BLE_GAP_EVENT_DISCONNECT);}else if(mode!=WRITE_TIMEOUT)attribute(cb,arg,attribute_handle,NULL,0);return 0;}
unsigned test_mbuf_length(const struct os_mbuf *b){return b?b->length:0;}
int os_mbuf_copydata(const struct os_mbuf *b,int offset,int length,void *out){CHECK(b&&offset==0&&length>=0&&(unsigned)length<=b->length);memcpy(out,b->data,(size_t)length);return 0;}
void vTaskDelay(TickType_t ticks)
{CHECK(!held);clock_ms+=ticks;dsr_blu_snapshot_t s;dsr_blu_snapshot(&s);CHECK(!dsr_activity_restart_reserve());if(s.phase==DSR_BLU_PIN){if(mode==CANCEL_PIN)dsr_blu_cancel(s.generation);else CHECK(dsr_blu_pin(s.generation,"123456")==DSR_OK);}if(mode==CANCEL_CONNECT)dsr_blu_cancel(s.generation);}
static dsr_blu_snapshot_t run(void)
{CHECK(queued);queued=false;operation(true);dsr_blu_snapshot_t s;dsr_blu_snapshot(&s);CHECK(!s.active&&!connected);return s;}
static dsr_ble_device_t candidate(void)
{return (dsr_ble_device_t){.address={1,2,3,4,5,6},.kind=DSR_BLE_SHELLY_BLU,.evidence=DSR_BLE_MFG|DSR_BLE_BLU_SERVICE,.model_id=5,.has_model=true,.has_flags=true,.flags=DSR_BLE_FLAG_PAIRING,.encrypted=encrypted,.seen_ms=clock_ms};}
static void reset(unsigned test)
{dsr_blu_close();mode=test;writes=deletes=0;encrypted=test==PIN_MODE||test==CANCEL_PIN;has_bond=pin_injected=false;values[0]=0;values[1]=1;values[2]=60;values[3]=2;values[4]=1;values[5]=10;values[6]=0;values[7]=2;values[8]=0;}
int main(void)
{
 dsr_blu_host_setup();CHECK(ble_hs_cfg.sm_bonding&&ble_hs_cfg.sm_sc&&ble_hs_cfg.store_status_cb);struct ble_store_status_event full={0};CHECK(ble_hs_cfg.store_status_cb(&full,NULL)==BLE_HS_ESTORE_CAP);
 for(unsigned test=0;test<=DISCONNECT_WRITE;test++){
  reset(test);dsr_ble_device_t c=candidate();CHECK(dsr_blu_start(&c)==DSR_OK);dsr_blu_snapshot_t s=run();
  bool success=test==NORMAL||test==PIN_MODE||test==WRITE_TIMEOUT||test==WRONG_READBACK||test==CHANGED_FIELD||test==OLD_CALLBACK||test==DISCONNECT_WRITE;
  if(success){CHECK(s.result==DSR_OK&&s.bonded&&(s.available&(1u<<2)));if(test==PIN_MODE){CHECK(pin_injected&&s.key_available);char address[18],key[33];CHECK(dsr_blu_gateway_key(s.generation,address,key));CHECK(strlen(address)==17&&strlen(key)==32);}
   if(test==CHANGED_FIELD){values[2]=60;mode=NORMAL;CHECK(dsr_blu_write(s.generation,2,120)==DSR_OK);s=run();CHECK(s.result==DSR_CONFLICT&&!writes);}
   else {CHECK(dsr_blu_write(s.generation,2,120)==DSR_OK);s=run();CHECK(writes==1);if(test==DISCONNECT_WRITE||test==WRONG_READBACK)CHECK(s.result!=DSR_OK&&s.uncertain);else CHECK(s.result==DSR_OK&&!s.uncertain&&s.values[2]==120);}
  }else CHECK(s.result!=DSR_OK&&!writes);
  CHECK(!deletes);
 }
 reset(NORMAL);dsr_ble_device_t c=candidate();CHECK(dsr_blu_start(&c)==DSR_OK);dsr_blu_snapshot_t s=run();CHECK(dsr_blu_forget(s.generation)==DSR_OK);s=run();CHECK(!s.bonded&&deletes==1&&!has_bond);
 CHECK(dsr_blu_write(s.generation,2,120)==DSR_INVALID);CHECK(dsr_blu_pin(s.generation,"12")==DSR_INVALID);
 reset(NORMAL);c=candidate();c.model_id=8;
 for(unsigned i=9;dsr_blu_setting(i);i++)values[i]=(uint32_t)dsr_blu_setting(i)->minimum;
 values[9]=0xa201;values[23]=(uint8_t)-15;values[26]=300;
 CHECK(dsr_blu_start(&c)==DSR_OK);s=run();CHECK(s.result==DSR_OK&&s.values[9]==1&&s.values[18]==1&&s.values[15]==0);
 CHECK(dsr_blu_write(s.generation,15,1)==DSR_OK);s=run();CHECK(s.result==DSR_OK&&!s.uncertain&&values[9]==0xa241&&writes==1);
 uint32_t raw;char text[24];CHECK(dsr_blu_value_parse(23,8,"-1.5",&raw)&&raw==(uint8_t)-15);dsr_blu_value_format(23,raw,text,sizeof(text));CHECK(!strcmp(text,"-1.5"));
 CHECK(!dsr_blu_value_parse(23,8,"-12.6",&raw));CHECK(!dsr_blu_value_parse(23,8,"1.01",&raw));CHECK(!dsr_blu_value_parse(23,5,"1",&raw));CHECK(!dsr_blu_value_parse(24,8,"nan",&raw));CHECK(dsr_blu_value_parse(24,8,"21.5",&raw)&&raw==215);
 dsr_blu_close();CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: %u actual BLU central checks; callback/security/storage MODEL, no physical BLE claim\n",checks);return 0;
}
