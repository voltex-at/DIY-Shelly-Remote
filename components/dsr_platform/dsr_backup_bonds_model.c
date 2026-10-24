#include "dsr_backup_bonds_model.h"
#include "dsr_backup_format.h"
#include "dsr_backup_json.h"
#include <stdbool.h>
#include <string.h>

typedef struct { uint8_t family,length,limit; const char *name; } family_t;
static const family_t families[]={
 {DSR_BACKUP_BOND_OUR_SEC,73,16,"our_sec"},
 {DSR_BACKUP_BOND_PEER_SEC,73,16,"peer_sec"},
 {DSR_BACKUP_BOND_CCCD,12,8,"cccd_sec"},
 {DSR_BACKUP_BOND_PEER_RECORD,44,16,"p_dev_rec"},
 {DSR_BACKUP_BOND_RPA,14,16,"rpa_rec"},
 {DSR_BACKUP_BOND_LOCAL_IRK,23,16,"local_irk"},
 {DSR_BACKUP_BOND_CSFC,8,16,"csfc_sec"}
};
static const family_t *by_id(unsigned id)
{for(unsigned i=0;i<sizeof(families)/sizeof(*families);i++)if(families[i].family==id)return &families[i];return NULL;}
static const family_t *by_name(const char *name)
{for(unsigned i=0;i<sizeof(families)/sizeof(*families);i++)if(!strcmp(families[i].name,name))return &families[i];return NULL;}
static bool address_valid(const dsr_backup_addr_t *a){return a->type<=3;}
static bool valid(const dsr_backup_bond_t *b,const family_t *f)
{
 if(!b->index||b->index>f->limit)return false;
 switch(b->family){
 case DSR_BACKUP_BOND_OUR_SEC:case DSR_BACKUP_BOND_PEER_SEC:
  return address_valid(&b->value.sec.peer)&&b->value.sec.key_size<=16&&
   (!(b->value.sec.flags&1)||b->value.sec.key_size>=7)&&b->value.sec.flags<=31;
 case DSR_BACKUP_BOND_CCCD:return address_valid(&b->value.cccd.peer)&&b->value.cccd.handle&&b->value.cccd.flags<=3&&b->value.cccd.changed<=1;
 case DSR_BACKUP_BOND_CSFC:return address_valid(&b->value.csfc.peer);
 case DSR_BACKUP_BOND_LOCAL_IRK:return address_valid(&b->value.local_irk.addr);
 case DSR_BACKUP_BOND_RPA:return address_valid(&b->value.rpa.random)&&address_valid(&b->value.rpa.peer);
 case DSR_BACKUP_BOND_PEER_RECORD:return b->value.peer_record.used<=1&&b->value.peer_record.rand_addr_type<=3&&address_valid(&b->value.peer_record.peer)&&b->value.peer_record.irk_present<=1;
 default:return false;
 }
}
dsr_result_t dsr_backup_bond_check(const dsr_backup_bond_t *b)
{if(!b)return DSR_INVALID;const family_t *f=by_id(b->family);return !f?DSR_UNSUPPORTED:valid(b,f)?DSR_OK:DSR_INVALID;}
static void put(uint8_t **p,uint64_t v,unsigned n){for(unsigned i=0;i<n;i++){*(*p)++=(uint8_t)v;v>>=8;}}
static uint64_t get(const uint8_t **p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;i++)v|=(uint64_t)*(*p)++<<(i*8);return v;}
static void put_bytes(uint8_t **p,const uint8_t *b,size_t n){memcpy(*p,b,n);*p+=n;}
static void get_bytes(const uint8_t **p,uint8_t *b,size_t n){memcpy(b,*p,n);*p+=n;}
static void put_addr(uint8_t **p,const dsr_backup_addr_t *a){put(p,a->type,1);put_bytes(p,a->bytes,6);}
static void get_addr(const uint8_t **p,dsr_backup_addr_t *a){a->type=(uint8_t)get(p,1);get_bytes(p,a->bytes,6);}
static void pack(const dsr_backup_bond_t *b,uint8_t *bytes)
{
 uint8_t *p=bytes;
 switch(b->family){
 case DSR_BACKUP_BOND_OUR_SEC:case DSR_BACKUP_BOND_PEER_SEC:
  put_addr(&p,&b->value.sec.peer);put(&p,b->value.sec.bond_count,2);put(&p,b->value.sec.key_size,1);
  put(&p,b->value.sec.ediv,2);put(&p,b->value.sec.rand_num,8);
  put_bytes(&p,b->value.sec.ltk,16);put_bytes(&p,b->value.sec.irk,16);put_bytes(&p,b->value.sec.csrk,16);
  put(&p,b->value.sec.sign_counter,4);put(&p,b->value.sec.flags,1);break;
 case DSR_BACKUP_BOND_CCCD:put_addr(&p,&b->value.cccd.peer);put(&p,b->value.cccd.handle,2);put(&p,b->value.cccd.flags,2);put(&p,b->value.cccd.changed,1);break;
 case DSR_BACKUP_BOND_CSFC:put_addr(&p,&b->value.csfc.peer);put(&p,b->value.csfc.features,1);break;
 case DSR_BACKUP_BOND_LOCAL_IRK:put_addr(&p,&b->value.local_irk.addr);put_bytes(&p,b->value.local_irk.key,16);break;
 case DSR_BACKUP_BOND_RPA:put_addr(&p,&b->value.rpa.random);put_addr(&p,&b->value.rpa.peer);break;
 case DSR_BACKUP_BOND_PEER_RECORD:
  put(&p,b->value.peer_record.used,1);put(&p,b->value.peer_record.rand_addr_type,1);
  put_bytes(&p,b->value.peer_record.pseudo,6);put_bytes(&p,b->value.peer_record.random,6);put_bytes(&p,b->value.peer_record.identity,6);
  put_addr(&p,&b->value.peer_record.peer);put_bytes(&p,b->value.peer_record.irk,16);put(&p,b->value.peer_record.irk_present,1);break;
 }
}
static void unpack(const uint8_t *bytes,dsr_backup_bond_t *b)
{
 const uint8_t *p=bytes;
 switch(b->family){
 case DSR_BACKUP_BOND_OUR_SEC:case DSR_BACKUP_BOND_PEER_SEC:
  get_addr(&p,&b->value.sec.peer);b->value.sec.bond_count=(uint16_t)get(&p,2);b->value.sec.key_size=(uint8_t)get(&p,1);
  b->value.sec.ediv=(uint16_t)get(&p,2);b->value.sec.rand_num=get(&p,8);
  get_bytes(&p,b->value.sec.ltk,16);get_bytes(&p,b->value.sec.irk,16);get_bytes(&p,b->value.sec.csrk,16);
  b->value.sec.sign_counter=(uint32_t)get(&p,4);b->value.sec.flags=(uint8_t)get(&p,1);break;
 case DSR_BACKUP_BOND_CCCD:get_addr(&p,&b->value.cccd.peer);b->value.cccd.handle=(uint16_t)get(&p,2);b->value.cccd.flags=(uint16_t)get(&p,2);b->value.cccd.changed=(uint8_t)get(&p,1);break;
 case DSR_BACKUP_BOND_CSFC:get_addr(&p,&b->value.csfc.peer);b->value.csfc.features=(uint8_t)get(&p,1);break;
 case DSR_BACKUP_BOND_LOCAL_IRK:get_addr(&p,&b->value.local_irk.addr);get_bytes(&p,b->value.local_irk.key,16);break;
 case DSR_BACKUP_BOND_RPA:get_addr(&p,&b->value.rpa.random);get_addr(&p,&b->value.rpa.peer);break;
 case DSR_BACKUP_BOND_PEER_RECORD:
  b->value.peer_record.used=(uint8_t)get(&p,1);b->value.peer_record.rand_addr_type=(uint8_t)get(&p,1);
  get_bytes(&p,b->value.peer_record.pseudo,6);get_bytes(&p,b->value.peer_record.random,6);get_bytes(&p,b->value.peer_record.identity,6);
  get_addr(&p,&b->value.peer_record.peer);get_bytes(&p,b->value.peer_record.irk,16);b->value.peer_record.irk_present=(uint8_t)get(&p,1);break;
 }
}
dsr_result_t dsr_backup_bond_encode(const dsr_backup_bond_t *b,cJSON **out)
{
 if(!out)return DSR_INVALID;
 *out=NULL;if(!b)return DSR_INVALID;
 const family_t *f=by_id(b->family);if(!f)return DSR_UNSUPPORTED;if(!valid(b,f))return DSR_INVALID;
 uint8_t bytes[73]={0};char hex[147];static const char digits[]="0123456789abcdef";pack(b,bytes);
 for(unsigned i=0;i<f->length;i++){hex[2*i]=digits[bytes[i]>>4];hex[2*i+1]=digits[bytes[i]&15];}hex[2*f->length]=0;
 cJSON *o=cJSON_CreateObject();bool ok=o&&cJSON_AddStringToObject(o,"family",f->name)&&cJSON_AddNumberToObject(o,"index",b->index)&&cJSON_AddStringToObject(o,"data",hex);
 dsr_backup_wipe(bytes,sizeof(bytes));dsr_backup_wipe(hex,sizeof(hex));if(!ok){dsr_backup_json_discard(o);return DSR_FULL;}*out=o;return DSR_OK;
}
static int digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
dsr_result_t dsr_backup_bond_decode(const cJSON *o,dsr_backup_bond_t *out)
{
 if(!out)return DSR_INVALID;
 memset(out,0,sizeof(*out));static const char *const fields[]={"family","index","data"};
 if(!dsr_backup_json_keys(o,fields,3))return DSR_INVALID;
 const cJSON *name=cJSON_GetObjectItemCaseSensitive(o,"family"),*data=cJSON_GetObjectItemCaseSensitive(o,"data");uint32_t index;
 if(!cJSON_IsString(name)||!cJSON_IsString(data)||!dsr_backup_json_number(o,"index",16,&index)||!index)return DSR_INVALID;
 const family_t *f=by_name(name->valuestring);if(!f)return DSR_UNSUPPORTED;
 if(index>f->limit||strlen(data->valuestring)!=f->length*2u)return DSR_INVALID;
 uint8_t bytes[73]={0};
 for(unsigned i=0;i<f->length;i++){int a=digit(data->valuestring[2*i]),b=digit(data->valuestring[2*i+1]);if(a<0||b<0){dsr_backup_wipe(bytes,sizeof(bytes));return DSR_INVALID;}bytes[i]=(uint8_t)(a*16+b);}
 out->family=f->family;out->index=(uint8_t)index;unpack(bytes,out);dsr_backup_wipe(bytes,sizeof(bytes));
 if(!valid(out,f)){dsr_backup_wipe(out,sizeof(*out));return DSR_INVALID;}return DSR_OK;
}
dsr_result_t dsr_backup_bonds_validate(const cJSON *o)
{
 static const char *const fields[]={"version","records"};uint32_t version;
 if(!dsr_backup_json_keys(o,fields,2)||!dsr_backup_json_number(o,"version",UINT32_MAX,&version))return DSR_INVALID;
 if(version!=1)return DSR_UNSUPPORTED;
 const cJSON *records=cJSON_GetObjectItemCaseSensitive(o,"records");if(!cJSON_IsArray(records)||cJSON_GetArraySize(records)>104)return DSR_INVALID;
 uint16_t seen[9]={0};const cJSON *row;
 cJSON_ArrayForEach(row,records){dsr_backup_bond_t b;dsr_result_t r=dsr_backup_bond_decode(row,&b);if(r!=DSR_OK)return r;
  uint16_t bit=(uint16_t)(1u<<(b.index-1));bool duplicate=(seen[b.family]&bit)!=0;seen[b.family]|=bit;dsr_backup_wipe(&b,sizeof(b));if(duplicate)return DSR_INVALID;
 }return DSR_OK;
}
