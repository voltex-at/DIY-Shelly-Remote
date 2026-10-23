/* Execute extracted actual runtime helpers with only their context as MODEL. */
#include "dsr_automation_types.h"
#include "dsr_fleet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {dsr_auto_rules_t public_rules;struct {uint32_t id,until;uint8_t failures;} rate[DSR_AUTO_PARTICIPANTS];} automation_t;
static automation_t *ctx;
#include "automation_rate.inc"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#x);exit(1);}}while(0)
static automation_t fixture;
int main(void){
 ctx=&fixture;
 fixture.public_rules.key_count=2;fixture.public_rules.keys[0].layout_id=10;fixture.public_rules.keys[1].layout_id=90;CHECK(find_key(&fixture.public_rules,90)==1);
 rate_result(10,DSR_RATE_LIMITED,1000);rate_result(90,DSR_RATE_LIMITED,1000);rate_result(10,DSR_OK,61000);
 rate_result(90,DSR_RATE_LIMITED,62000);CHECK(!rate_ready(90,122000));CHECK(rate_ready(90,182000));rate_result(90,DSR_OK,182000);CHECK(rate_ready(90,182000));
 memset(&fixture,0,sizeof(fixture));fixture.public_rules.key_count=16;fixture.public_rules.keys[0].layout_id=90;for(unsigned i=0;i<15;i++)fixture.public_rules.keys[i+1].layout_id=100+i;
 rate_result(90,DSR_RATE_LIMITED,1000);for(unsigned i=0;i<15;i++)rate_result(100+i,DSR_RATE_LIMITED,1000);
 fixture.public_rules.key_count=2;fixture.public_rules.keys[1].layout_id=999;
 rate_result(999,DSR_RATE_LIMITED,2000);CHECK(!rate_ready(999,3000));CHECK(!rate_ready(90,3000));
 printf("automation sparse rate history: %u checks; actual runtime helper\n",checks);return 0;
}
