#include "dsr_backup_bonds_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"bonds:%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void)
{
 dsr_backup_bond_t original={.family=DSR_BACKUP_BOND_OUR_SEC,.index=1};
 original.value.sec.peer.type=1;for(unsigned i=0;i<6;i++)original.value.sec.peer.bytes[i]=(uint8_t)(i+1);
 original.value.sec.bond_count=3;original.value.sec.key_size=16;original.value.sec.ediv=0x1234;original.value.sec.rand_num=UINT64_C(0xfedcba9876543210);original.value.sec.flags=31;original.value.sec.sign_counter=0x78563412;
 for(unsigned i=0;i<16;i++){original.value.sec.ltk[i]=(uint8_t)i;original.value.sec.irk[i]=(uint8_t)(16+i);original.value.sec.csrk[i]=(uint8_t)(32+i);}
 cJSON *row=NULL;CHECK(dsr_backup_bond_encode(&original,&row)==DSR_OK);CHECK(row);
 const char *wire=cJSON_GetObjectItemCaseSensitive(row,"data")->valuestring;
 CHECK(!strcmp(wire,"0101020304050603001034121032547698badcfe000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f123456781f"));
 dsr_backup_bond_t decoded;CHECK(dsr_backup_bond_decode(row,&decoded)==DSR_OK);CHECK(decoded.value.sec.rand_num==UINT64_C(0xfedcba9876543210)&&decoded.value.sec.sign_counter==0x78563412&&decoded.value.sec.flags==31);
 cJSON *root=cJSON_CreateObject();CHECK(root&&cJSON_AddNumberToObject(root,"version",1));cJSON *records=cJSON_AddArrayToObject(root,"records");CHECK(records&&cJSON_AddItemToArray(records,row));
 CHECK(dsr_backup_bonds_validate(root)==DSR_OK);cJSON *copy=cJSON_Duplicate(row,true);CHECK(copy&&cJSON_AddItemToArray(records,copy));CHECK(dsr_backup_bonds_validate(root)==DSR_INVALID);cJSON_DeleteItemFromArray(records,1);
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"index"),0);CHECK(dsr_backup_bonds_validate(root)==DSR_INVALID);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(row,"index"),1);
 original.value.sec.flags=32;CHECK(dsr_backup_bond_encode(&original,&copy)==DSR_INVALID&&copy==NULL);original.value.sec.flags=31;
 const uint8_t families[]={DSR_BACKUP_BOND_PEER_SEC,DSR_BACKUP_BOND_CCCD,DSR_BACKUP_BOND_CSFC,DSR_BACKUP_BOND_LOCAL_IRK,DSR_BACKUP_BOND_RPA,DSR_BACKUP_BOND_PEER_RECORD};
 for(unsigned i=0;i<sizeof(families);i++){
  dsr_backup_bond_t bond={.family=families[i],.index=1};
  if(bond.family==DSR_BACKUP_BOND_PEER_SEC)bond.value.sec.key_size=16;
  if(bond.family==DSR_BACKUP_BOND_CCCD){bond.value.cccd.handle=0x3344;bond.value.cccd.flags=3;bond.value.cccd.changed=1;}
  if(bond.family==DSR_BACKUP_BOND_CSFC)bond.value.csfc.features=1;
  if(bond.family==DSR_BACKUP_BOND_LOCAL_IRK)bond.value.local_irk.key[15]=77;
  if(bond.family==DSR_BACKUP_BOND_RPA)bond.value.rpa.random.type=1;
  if(bond.family==DSR_BACKUP_BOND_PEER_RECORD){bond.value.peer_record.used=1;bond.value.peer_record.irk_present=1;bond.value.peer_record.irk[0]=99;}
  CHECK(dsr_backup_bond_encode(&bond,&copy)==DSR_OK);CHECK(dsr_backup_bond_decode(copy,&decoded)==DSR_OK);CHECK(decoded.family==bond.family&&decoded.index==1);CHECK(cJSON_AddItemToArray(records,copy));
 }
 CHECK(dsr_backup_bonds_validate(root)==DSR_OK);
 cJSON *unknown=cJSON_Parse("{\"family\":\"ead_sec\",\"index\":1,\"data\":\"00\"}");CHECK(unknown);CHECK(dsr_backup_bond_decode(unknown,&decoded)==DSR_UNSUPPORTED);cJSON_Delete(unknown);
 for(unsigned i=2;i<=16;i++){dsr_backup_bond_t bond={.family=DSR_BACKUP_BOND_OUR_SEC,.index=(uint8_t)i};bond.value.sec.key_size=16;CHECK(dsr_backup_bond_encode(&bond,&copy)==DSR_OK&&cJSON_AddItemToArray(records,copy));}
 CHECK(dsr_backup_bonds_validate(root)==DSR_OK);dsr_backup_bond_t excessive={.family=DSR_BACKUP_BOND_OUR_SEC,.index=17};CHECK(dsr_backup_bond_encode(&excessive,&copy)==DSR_INVALID);
 const char *bad[]={
  "{\"family\":\"our_sec\",\"index\":1,\"data\":\"\"}",
  "{\"family\":\"our_sec\",\"index\":1.5,\"data\":\"00\"}",
  "{\"family\":\"csfc_sec\",\"index\":1,\"data\":\"0400000000000000\"}",
  "{\"family\":\"csfc_sec\",\"index\":1,\"data\":\"00000000000000AB\"}",
  "{\"family\":\"cccd_sec\",\"index\":9,\"data\":\"000000000000000100030001\"}",
  "{\"family\":\"cccd_sec\",\"index\":1,\"data\":\"000000000000000100030002\"}",
  "{\"family\":\"csfc_sec\",\"index\":1,\"data\":\"0000000000000001\",\"extra\":0}"
 };
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){cJSON *v=cJSON_Parse(bad[i]);CHECK(v);memset(&decoded,0xff,sizeof(decoded));CHECK(dsr_backup_bond_decode(v,&decoded)==DSR_INVALID);dsr_backup_bond_t zero={0};CHECK(!memcmp(&zero,&decoded,sizeof(zero)));cJSON_Delete(v);}
 cJSON_Delete(root);printf("PASS: %u canonical NimBLE record checks; no ABI dumps/storage/radio\n",checks);return 0;
}
