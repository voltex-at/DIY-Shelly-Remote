#include "dsr_maintenance_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"stage-model:%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void)
{
 dsr_maint_limits_t l={.total=0x3e0000,.used=65536,.nvs_available=1800,.psram_free=1024*1024,.psram_largest=1024*1024,.internal_free=65536,.internal_largest=32768};
 CHECK(dsr_maint_preflight(&l,65536,16,104)==DSR_OK);l.used+=16*115200;CHECK(dsr_maint_preflight(&l,65536,16,104)==DSR_FULL);l.used=65536;
 l.nvs_available=10;CHECK(dsr_maint_preflight(&l,65536,16,16)==DSR_FULL);l.nvs_available=1800;
 l.psram_largest=4096;CHECK(dsr_maint_preflight(&l,65536,16,16)==DSR_FULL);l.psram_largest=1024*1024;
 l.psram_largest=98304;CHECK(dsr_maint_preflight(&l,65536,16,16)==DSR_FULL);l.psram_largest=1024*1024;
 l.psram_free=230400;CHECK(dsr_maint_preflight(&l,65536,16,16)==DSR_FULL);l.psram_free=1024*1024;
 l.internal_largest=4096;CHECK(dsr_maint_preflight(&l,65536,16,16)==DSR_FULL);l.internal_largest=32768;
 CHECK(dsr_maint_preflight(&l,65537,16,16)==DSR_INVALID&&dsr_maint_preflight(&l,1,17,16)==DSR_INVALID);
 dsr_maint_record_t r={.sequence=UINT64_C(0xfedcba9876543210),.generation=UINT64_C(0x1020304050607080),.candidate_bytes=24+500+16*(56+115200),.metadata_bytes=500,.op=DSR_MAINT_IMPORT_COMMIT,.phase=DSR_MAINT_PREPARING,.photos=16,.connections=16,.rules=8,.bonds=104};
 r.config_revision=901;r.operation_id[0]=1;r.device[1]=2;r.candidate_sha256[2]=3;r.manifest_sha256[3]=4;r.key[4]=5;r.nonce_prefix[5]=6;
 uint8_t wire[DSR_MAINT_JOURNAL_BYTES];CHECK(dsr_maint_record_encode(&r,wire));CHECK(!memcmp(wire,"DSRMNT1",8));
 dsr_maint_record_t decoded;CHECK(dsr_maint_record_decode(wire,sizeof(wire),&decoded)==DSR_OK&&decoded.sequence==r.sequence&&decoded.generation==r.generation&&!memcmp(decoded.key,r.key,32));
 const unsigned positions[]={0,8,12,20,28,76,140,172,180,192,252};
 for(unsigned i=0;i<sizeof(positions)/sizeof(*positions);i++){wire[positions[i]]^=1;memset(&decoded,0xff,sizeof(decoded));CHECK(dsr_maint_record_decode(wire,sizeof(wire),&decoded)!=DSR_OK);dsr_maint_record_t zero={0};CHECK(!memcmp(&zero,&decoded,sizeof(zero)));wire[positions[i]]^=1;}
 CHECK(dsr_maint_record_decode(wire,sizeof(wire)-1,&decoded)==DSR_INVALID);
 r.phase=DSR_MAINT_COMMIT;r.applied_photos=1;CHECK(!dsr_maint_record_valid(&r));r.applied_photos=0;
 r.phase=DSR_MAINT_APPLY;r.applied_photos=0xffff;r.applied_parts=DSR_MAINT_IMPORT_PARTS;CHECK(dsr_maint_record_valid(&r));
 r.phase=DSR_MAINT_RECEIPT;CHECK(!dsr_maint_record_valid(&r));memset(r.key,0,32);memset(r.nonce_prefix,0,8);CHECK(dsr_maint_record_valid(&r));
 r.cancelled=1;CHECK(!dsr_maint_record_valid(&r));r.applied_photos=r.applied_parts=0;CHECK(dsr_maint_record_valid(&r));
 char names[17][32];for(unsigned i=0;i<17;i++){CHECK(dsr_maint_record_name(r.operation_id,i,names[i])&&strlen(names[i])<=30&&!strchr(names[i],'/')&&!strstr(names[i],".."));for(unsigned j=0;j<i;j++)CHECK(strcmp(names[i],names[j]));}CHECK(!dsr_maint_record_name(r.operation_id,17,names[0]));
 printf("PASS: %u journal/budget/name checks; no storage, crypto staging remains separate\n",checks);return 0;
}
