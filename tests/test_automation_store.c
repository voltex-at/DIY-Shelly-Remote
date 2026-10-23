#include "dsr_automation_store.h"
#include "dsr_automation_model.h"
#include "automation/nvs_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#x);exit(1);}}while(0)
static dsr_auto_record_t record,loaded;
static void reseal(uint8_t *b,size_t n){
 uint32_t crc=UINT32_MAX;
 for(size_t i=0;i<n-4;i++){crc^=b[i];for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));}
 crc=~crc;for(unsigned i=0;i<4;i++)b[n-4+i]=(uint8_t)(crc>>(8*i));
}
static void make_record(void){
 memset(&record,0,sizeof(record));record.generation=1;record.rules.count=1;record.rules.key_count=2;
 record.rules.items[0]=(dsr_auto_rule_t){.id=1,.signature=7,.sensors=1,.targets=2,.target_half=16,.enabled=1};
 for(unsigned i=0;i<2;i++){dsr_auto_key_t *k=&record.rules.keys[i];k->layout_id=i+1;k->generation=2;k->kind=i?DSR_AUTO_TRV:DSR_AUTO_CONTACT;
  strcpy(k->host,"192.0.2.1");snprintf(k->device_id,sizeof(k->device_id),"gw/blu-%u",i);}
}
int main(void){
 auto_flash_reset();make_record();bool found=true;
 CHECK(dsr_auto_store_load(&loaded,&found)==ESP_OK&&!found);
 CHECK(dsr_auto_store_save(&record)==ESP_OK);CHECK(auto_commits==1&&auto_sets==1&&auto_opens>=3);
 CHECK(auto_lengths[0]&&auto_lengths[0]<=DSR_AUTO_RECORD_MAX);
 CHECK(dsr_auto_store_load(&loaded,&found)==ESP_OK&&found);CHECK(!memcmp(&record,&loaded,sizeof(record)));
 record.generation++;record.cycles[0].cycle=1;record.cycles[0].active=1;
 record.targets[1]=(dsr_auto_target_t){.baseline=40,.desired=16,.phase=DSR_AUTO_PREPARED};
 CHECK(dsr_auto_store_save(&record)==ESP_OK);CHECK(auto_lengths[1]==auto_lengths[0]);
 CHECK(dsr_auto_store_load(&loaded,&found)==ESP_OK&&found&&loaded.generation==2);
 unsigned saves=auto_sets;CHECK(dsr_auto_store_save(&record)==ESP_OK&&auto_sets==saves);
 record.generation=1;CHECK(dsr_auto_store_save(&record)!=ESP_OK);record.generation=2;
 record.targets[1].override=1;CHECK(dsr_auto_store_save(&record)!=ESP_OK);record.generation++;
 CHECK(dsr_auto_store_save(&record)==ESP_OK);CHECK(dsr_auto_store_load(&loaded,&found)==ESP_OK&&loaded.targets[1].override);
 for(unsigned phase=DSR_AUTO_PREPARED;phase<=DSR_AUTO_UNCERTAIN;phase++){
  record.targets[1].phase=(uint8_t)phase;record.generation++;CHECK(dsr_auto_store_save(&record)==ESP_OK);
  CHECK(dsr_auto_store_load(&loaded,&found)==ESP_OK);dsr_auto_model_t model;CHECK(dsr_auto_recover(&model,&loaded));
  CHECK(model.record.targets[1].override&&model.record.targets[1].baseline==40);
 }
 for(unsigned fault=1;fault<=5;fault++){
  auto_flash_reset();make_record();CHECK(dsr_auto_store_save(&record)==ESP_OK);
  record.generation++;record.targets[1]=(dsr_auto_target_t){.baseline=40,.desired=16,.phase=DSR_AUTO_PREPARED,.override=1};
  auto_flash_fault(fault);CHECK(dsr_auto_store_save(&record)!=ESP_OK);auto_flash_fault(0);
  CHECK(dsr_auto_store_load(&loaded,&found)==ESP_OK&&found);
  CHECK(loaded.generation==(fault>=4?2u:1u)); /* readback may fail AFTER durable commit */
 }
 for(unsigned damage=0;damage<4;damage++){
  auto_flash_reset();make_record();CHECK(dsr_auto_store_save(&record)==ESP_OK);record.generation++;
  CHECK(dsr_auto_store_save(&record)==ESP_OK);
  if(damage==0)auto_flash[1][auto_lengths[1]-1]^=1;
  if(damage==1)auto_flash[1][4]++; /* future format, even if other slot is valid */
  if(damage==2)auto_lengths[1]--;
  if(damage==3)auto_flash[1][8]=0; /* sequence/CRC corruption */
  CHECK(dsr_auto_store_load(&loaded,&found)!=ESP_OK);CHECK(!found);
  record.generation++;CHECK(dsr_auto_store_save(&record)!=ESP_OK);
 }
 auto_flash_reset();make_record();CHECK(dsr_auto_store_save(&record)==ESP_OK);record.generation++;
 CHECK(dsr_auto_store_save(&record)==ESP_OK);
 auto_flash[1][16]=9;reseal(auto_flash[1],auto_lengths[1]); /* valid CRC, impossible rule count */
 CHECK(dsr_auto_store_load(&loaded,&found)!=ESP_OK&&!found);record.generation++;
 CHECK(dsr_auto_store_save(&record)!=ESP_OK); /* never overwrite a suspect journal */
 printf("automation store: %u checks passed\n",checks);return 0;
}
