#include "dsr_backup_bonds_sdk.h"
#include "dsr_backup_format.h"
#include <stdbool.h>
#include "host/ble_store.h"
#include "ble_hs_resolv_priv.h"
#include <stdio.h>
#include <string.h>

_Static_assert(MYNEWT_VAL(BLE_STORE_MAX_BONDS)==16,"review backup limits when changing SDK bonds");
_Static_assert(MYNEWT_VAL(BLE_STORE_MAX_CCCDS)<=8,"review backup CCCD limits");
_Static_assert(MYNEWT_VAL(BLE_GATT_CSFC_SIZE)==1,"review backup CSFC encoding");
typedef union {
 union ble_store_value store;
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
 struct ble_hs_dev_records peer;
#endif
} sdk_value_t;
static const struct {uint8_t id;const char *prefix;size_t size;} types[]={
 {DSR_BACKUP_BOND_OUR_SEC,"our_sec",sizeof(struct ble_store_value_sec)},
 {DSR_BACKUP_BOND_PEER_SEC,"peer_sec",sizeof(struct ble_store_value_sec)},
 {DSR_BACKUP_BOND_CCCD,"cccd_sec",sizeof(struct ble_store_value_cccd)},
 {DSR_BACKUP_BOND_CSFC,"csfc_sec",sizeof(struct ble_store_value_csfc)},
 {DSR_BACKUP_BOND_LOCAL_IRK,"local_irk",sizeof(struct ble_store_value_local_irk)},
 {DSR_BACKUP_BOND_RPA,"rpa_rec",sizeof(struct ble_store_value_rpa_rec)},
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
 {DSR_BACKUP_BOND_PEER_RECORD,"p_dev_rec",sizeof(struct ble_hs_dev_records)},
#endif
};
static int type_index(unsigned id){for(unsigned i=0;i<sizeof(types)/sizeof(*types);i++)if(types[i].id==id)return (int)i;return -1;}
static dsr_backup_addr_t logical(ble_addr_t a){dsr_backup_addr_t b={.type=a.type};memcpy(b.bytes,a.val,6);return b;}
static ble_addr_t native(dsr_backup_addr_t a){ble_addr_t b={.type=a.type};memcpy(b.val,a.bytes,6);return b;}
dsr_result_t dsr_backup_bond_from_sdk(const char *key,const void *value,size_t size,dsr_backup_bond_t *out)
{
 if(!out)return DSR_INVALID;
 memset(out,0,sizeof(*out));if(!key||!value)return DSR_INVALID;
 int at=-1;unsigned index=0;
 for(unsigned i=0;i<sizeof(types)/sizeof(*types);i++){
  size_t n=strlen(types[i].prefix);if(strncmp(key,types[i].prefix,n)||key[n]!='_')continue;
  const char *p=key+n+1;if(*p<'1'||*p>'9')continue;
  while(*p>='0'&&*p<='9'&&index<=16){index=index*10+(unsigned)(*p++-'0');}
  if(*p||index>16)return DSR_UNSUPPORTED;
  at=(int)i;break;
 }
 if(at<0||size!=types[at].size)return DSR_UNSUPPORTED;
 sdk_value_t raw={0};memcpy(&raw,value,size);out->family=types[at].id;out->index=(uint8_t)index;
 switch(out->family){
 case DSR_BACKUP_BOND_OUR_SEC:case DSR_BACKUP_BOND_PEER_SEC:{
  const struct ble_store_value_sec *s=&raw.store.sec;
  out->value.sec.peer=logical(s->peer_addr);out->value.sec.bond_count=s->bond_count;
  out->value.sec.key_size=s->key_size;out->value.sec.ediv=s->ediv;out->value.sec.rand_num=s->rand_num;
  memcpy(out->value.sec.ltk,s->ltk,16);memcpy(out->value.sec.irk,s->irk,16);memcpy(out->value.sec.csrk,s->csrk,16);
  out->value.sec.sign_counter=s->sign_counter;
  out->value.sec.flags=(s->ltk_present?1:0)|(s->irk_present?2:0)|(s->csrk_present?4:0)|(s->authenticated?8:0)|(s->sc?16:0);break;}
 case DSR_BACKUP_BOND_CCCD:
  out->value.cccd.peer=logical(raw.store.cccd.peer_addr);out->value.cccd.handle=raw.store.cccd.chr_val_handle;
  out->value.cccd.flags=raw.store.cccd.flags;out->value.cccd.changed=raw.store.cccd.value_changed;break;
 case DSR_BACKUP_BOND_CSFC:out->value.csfc.peer=logical(raw.store.csfc.peer_addr);out->value.csfc.features=raw.store.csfc.csfc[0];break;
 case DSR_BACKUP_BOND_LOCAL_IRK:out->value.local_irk.addr=logical(raw.store.local_irk.addr);memcpy(out->value.local_irk.key,raw.store.local_irk.irk,16);break;
 case DSR_BACKUP_BOND_RPA:out->value.rpa.random=logical(raw.store.rpa_rec.peer_rpa_addr);out->value.rpa.peer=logical(raw.store.rpa_rec.peer_addr);break;
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
 case DSR_BACKUP_BOND_PEER_RECORD:
  out->value.peer_record.used=raw.peer.rec_used;out->value.peer_record.rand_addr_type=raw.peer.rand_addr_type;
  memcpy(out->value.peer_record.pseudo,raw.peer.pseudo_addr,6);memcpy(out->value.peer_record.random,raw.peer.rand_addr,6);memcpy(out->value.peer_record.identity,raw.peer.identity_addr,6);
  out->value.peer_record.peer=logical(raw.peer.peer_sec.peer_addr);memcpy(out->value.peer_record.irk,raw.peer.peer_sec.irk,16);out->value.peer_record.irk_present=raw.peer.peer_sec.irk_present;break;
#endif
 }
 dsr_backup_wipe(&raw,sizeof(raw));dsr_result_t r=dsr_backup_bond_check(out);
 if(r!=DSR_OK)dsr_backup_wipe(out,sizeof(*out));
 return r;
}
dsr_result_t dsr_backup_bond_to_sdk(const dsr_backup_bond_t *b,char key[16],void *out,size_t capacity,size_t *size)
{
 if(!key||!out||!size)return DSR_INVALID;
 *size=0;key[0]=0;dsr_result_t r=dsr_backup_bond_check(b);if(r!=DSR_OK)return r;
 int at=type_index(b->family);if(at<0)return DSR_UNSUPPORTED;
 if(capacity<types[at].size)return DSR_FULL;
 sdk_value_t raw={0};
 switch(b->family){
 case DSR_BACKUP_BOND_OUR_SEC:case DSR_BACKUP_BOND_PEER_SEC:{
  struct ble_store_value_sec *s=&raw.store.sec;s->peer_addr=native(b->value.sec.peer);s->bond_count=b->value.sec.bond_count;
  s->key_size=b->value.sec.key_size;s->ediv=b->value.sec.ediv;s->rand_num=b->value.sec.rand_num;
  memcpy(s->ltk,b->value.sec.ltk,16);memcpy(s->irk,b->value.sec.irk,16);memcpy(s->csrk,b->value.sec.csrk,16);
  s->sign_counter=b->value.sec.sign_counter;s->ltk_present=(b->value.sec.flags&1)!=0;s->irk_present=(b->value.sec.flags&2)!=0;
  s->csrk_present=(b->value.sec.flags&4)!=0;s->authenticated=(b->value.sec.flags&8)!=0;s->sc=(b->value.sec.flags&16)!=0;break;}
 case DSR_BACKUP_BOND_CCCD:raw.store.cccd.peer_addr=native(b->value.cccd.peer);raw.store.cccd.chr_val_handle=b->value.cccd.handle;raw.store.cccd.flags=b->value.cccd.flags;raw.store.cccd.value_changed=b->value.cccd.changed;break;
 case DSR_BACKUP_BOND_CSFC:raw.store.csfc.peer_addr=native(b->value.csfc.peer);raw.store.csfc.csfc[0]=b->value.csfc.features;break;
 case DSR_BACKUP_BOND_LOCAL_IRK:raw.store.local_irk.addr=native(b->value.local_irk.addr);memcpy(raw.store.local_irk.irk,b->value.local_irk.key,16);break;
 case DSR_BACKUP_BOND_RPA:raw.store.rpa_rec.peer_rpa_addr=native(b->value.rpa.random);raw.store.rpa_rec.peer_addr=native(b->value.rpa.peer);break;
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
 case DSR_BACKUP_BOND_PEER_RECORD:raw.peer.rec_used=b->value.peer_record.used;raw.peer.rand_addr_type=b->value.peer_record.rand_addr_type;
  memcpy(raw.peer.pseudo_addr,b->value.peer_record.pseudo,6);memcpy(raw.peer.rand_addr,b->value.peer_record.random,6);memcpy(raw.peer.identity_addr,b->value.peer_record.identity,6);
  raw.peer.peer_sec.peer_addr=native(b->value.peer_record.peer);memcpy(raw.peer.peer_sec.irk,b->value.peer_record.irk,16);raw.peer.peer_sec.irk_present=b->value.peer_record.irk_present;break;
#endif
 }
 *size=types[at].size;snprintf(key,16,"%s_%u",types[at].prefix,b->index);memcpy(out,&raw,*size);dsr_backup_wipe(&raw,sizeof(raw));return DSR_OK;
}
