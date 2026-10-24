#include "dsr_maintenance_store.h"
#include "nvs.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"stage-store:%d %s\n",__LINE__,#x);exit(1);}}while(0)
static dsr_maint_record_t fresh(unsigned id)
{
 dsr_maint_record_t r={.sequence=1,.generation=7,.candidate_bytes=24+500+16*(56+115200),.metadata_bytes=500,.op=DSR_MAINT_IMPORT_COMMIT,.phase=DSR_MAINT_PREPARING,.photos=16,.connections=16,.rules=8,.bonds=16};
 r.operation_id[0]=(uint8_t)id;r.device[1]=2;r.candidate_sha256[2]=3;r.manifest_sha256[3]=4;r.key[4]=5;r.nonce_prefix[5]=6;return r;
}
int main(void)
{
 dsr_maint_record_t r=fresh(1),loaded;bool found=true,pending=true;
 CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&!found&&!pending);CHECK(dsr_maint_store_begin(&r)==DSR_OK&&r.sequence==2);
 CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&found&&pending&&loaded.phase==DSR_MAINT_PREPARING&&loaded.key[4]==5);
 dsr_maint_record_t second=fresh(2);CHECK(dsr_maint_store_begin(&second)==DSR_BUSY);
 r.phase=DSR_MAINT_COMMIT;CHECK(dsr_maint_store_write(&r)==DSR_OK&&r.sequence==3);dsr_maint_record_t stale=r;
 r.phase=DSR_MAINT_APPLY;CHECK(dsr_maint_store_write(&r)==DSR_OK&&r.sequence==4);CHECK(dsr_maint_store_write(&stale)==DSR_CONFLICT);
 r.applied_photos=1;CHECK(dsr_maint_store_write(&r)==DSR_OK);r.applied_photos=0;CHECK(dsr_maint_store_write(&r)==DSR_INVALID);r.applied_photos=1;
 dsr_maint_record_t premature=r;premature.phase=DSR_MAINT_RECEIPT;premature.applied_photos=0xffff;premature.applied_parts=DSR_MAINT_IMPORT_PARTS;memset(premature.key,0,32);memset(premature.nonce_prefix,0,8);CHECK(dsr_maint_store_write(&premature)==DSR_INVALID);
 CHECK(dsr_maint_store_finish(&r,true)==DSR_INVALID);CHECK(dsr_maint_store_finish(&r,false)==DSR_INVALID);
 r.applied_photos=0xffff;r.applied_parts=DSR_MAINT_IMPORT_PARTS;CHECK(dsr_maint_store_write(&r)==DSR_OK);CHECK(dsr_maint_store_finish(&r,false)==DSR_OK);
 CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&found&&!pending&&loaded.phase==DSR_MAINT_RECEIPT);uint8_t zero[32]={0};CHECK(!memcmp(loaded.key,zero,32)&&!memcmp(r.key,zero,32));
 nvs_handle_t receipt;CHECK(nvs_open("dsr_maint",NVS_READONLY,&receipt)==ESP_OK);for(unsigned i=0;i<2;i++){uint8_t raw[256];size_t n=sizeof(raw);CHECK(nvs_get_blob(receipt,i?"b":"a",raw,&n)==ESP_OK&&n==256&&!memcmp(raw+140,zero,32));}nvs_close(receipt);
 CHECK(dsr_maint_store_begin(&second)==DSR_OK);CHECK(dsr_maint_store_finish(&second,true)==DSR_OK);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_OK&&!pending&&loaded.cancelled);
 r=fresh(3);CHECK(dsr_maint_store_begin(&r)==DSR_OK);nvs_handle_t h;CHECK(nvs_open("dsr_maint",NVS_READWRITE,&h)==ESP_OK);CHECK(nvs_erase_key(h,"b")==ESP_OK);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_INVALID&&pending);nvs_close(h);
 test_nvs_reset();r=fresh(4);CHECK(dsr_maint_store_begin(&r)==DSR_OK);CHECK(nvs_open("dsr_maint",NVS_READWRITE,&h)==ESP_OK);uint8_t wire[256];size_t size=sizeof(wire);CHECK(nvs_get_blob(h,"a",wire,&size)==ESP_OK);wire[140]^=1;CHECK(nvs_set_blob(h,"a",wire,size)==ESP_OK);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_INVALID&&pending);nvs_close(h);
 test_nvs_reset();CHECK(nvs_open("dsr_maint",NVS_READWRITE,&h)==ESP_OK&&nvs_set_u8(h,"pending",1)==ESP_OK);CHECK(dsr_maint_store_load(&loaded,&found,&pending)==DSR_INVALID&&pending);nvs_close(h);
 for(int point=0;point<6;point++){
  test_nvs_reset();r=fresh((unsigned)point+10);test_nvs_fail_after=point;CHECK(dsr_maint_store_begin(&r)==DSR_IO);test_nvs_fail_after=-1;
  dsr_result_t result=dsr_maint_store_load(&loaded,&found,&pending);CHECK(result==DSR_OK?(pending||!found):pending);
 }
 CHECK(test_active_writes==0);test_nvs_reset();printf("PASS: %u durable intent checks; real journal/model, NVS model; zero active-store writes\n",checks);return 0;
}
