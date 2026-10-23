#include "dsr_automation_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#c);exit(1);}}while(0)
static dsr_auto_model_t m;
static dsr_auto_rules_t rules;
static uint32_t now=1000,utc=1800000000;
static unsigned writes;
static dsr_auto_effect_t next(void){dsr_auto_effect_t e={0};CHECK(dsr_auto_next(&m,now,utc,&e));return e;}
static void ack(dsr_auto_effect_t e){dsr_auto_complete(&m,&e,DSR_OK);}
static void sample(unsigned i,bool open,double target,uint32_t read_id){
 dsr_auto_observation_t o={.key=rules.keys[i],.sample_ms=now,.source_utc=utc,
  .read_id=read_id,.started_ms=now,.valid=true,.open=open,.target_c=target};dsr_auto_observe(&m,&o);
}
static void read_ack(dsr_auto_effect_t e,double value){
 CHECK(e.kind==DSR_AUTO_EFFECT_READ_TRV);unsigned i=e.layout_id-1;sample(i,false,value,e.id);ack(e);
}
static void init_values(double a,double b){
 dsr_auto_init(&m);memset(&rules,0,sizeof(rules));now=1000;utc=1800000000;writes=0;
 rules.count=1;rules.key_count=4;rules.enroll=true;
 rules.items[0]=(dsr_auto_rule_t){.id=7,.signature=100,.sensors=3,.targets=12,.target_half=16,.enabled=1};
 for(unsigned i=0;i<4;i++){rules.keys[i].layout_id=i+1;rules.keys[i].generation=2;
  rules.keys[i].kind=i<2?DSR_AUTO_CONTACT:DSR_AUTO_TRV;
  snprintf(rules.keys[i].device_id,64,"gateway/blu-%u",i);strcpy(rules.keys[i].host,"192.0.2.1");}
 CHECK(dsr_auto_configure(&m,&rules)==DSR_OK);dsr_auto_pause(&m,now,utc);
 dsr_auto_effect_t e=next();CHECK(e.kind==DSR_AUTO_EFFECT_JOURNAL);CHECK(writes==0);ack(e);
 now+=1000;utc++;sample(0,true,0,0);sample(1,false,0,0);
 read_ack(next(),a);read_ack(next(),b);
}
static void init(void){init_values(20,22);}
static void journal(void){dsr_auto_effect_t e=next();CHECK(e.kind==DSR_AUTO_EFFECT_JOURNAL);ack(e);}
static void set_and_confirm(uint8_t half){
 dsr_auto_effect_t e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV);CHECK(e.target_half==half);writes++;ack(e);
 journal();now+=15001;utc+=15;sample(0,true,0,0);sample(1,false,0,0);
 e=next();read_ack(e,half/2.0);journal();
}
static void restore_requires_new_direct_read(void){
 init();journal();set_and_confirm(16);set_and_confirm(16);sample(0,false,0,0);journal();
 dsr_auto_effect_t e=next();CHECK(e.kind==DSR_AUTO_EFFECT_READ_TRV&&e.layout_id==3);
 read_ack(e,8);e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV&&e.target_half==40);
}
static void first_open_all_closed(void){
 init();journal();CHECK(writes==0);set_and_confirm(16);set_and_confirm(16);
 sample(0,false,0,0);journal();
 read_ack(next(),8);dsr_auto_effect_t e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV);CHECK(e.target_half==40);
 CHECK(e.layout_id==3);ack(e);journal();
 now+=15001;utc+=15;sample(0,false,0,0);sample(1,false,0,0);read_ack(next(),20);journal();
 read_ack(next(),8);e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV);CHECK(e.layout_id==4);CHECK(e.target_half==44);ack(e);journal();
 now+=15001;utc+=15;sample(0,false,0,0);sample(1,false,0,0);read_ack(next(),22);journal();
 journal();CHECK(!m.record.cycles[0].active);
}
static void no_raise_already_lower(void){
 init_values(6,7);journal();CHECK(m.record.targets[2].desired==12);CHECK(m.record.targets[3].desired==14);
 dsr_auto_effect_t e;CHECK(!dsr_auto_next(&m,now,utc,&e));sample(0,false,0,0);journal();
 CHECK(!m.record.cycles[0].active);CHECK(writes==0);
}
static void manual_during_prepared_inflight_uncertain(void){
 for(unsigned phase=0;phase<3;phase++){
  init();dsr_auto_effect_t old=next();CHECK(old.kind==DSR_AUTO_EFFECT_JOURNAL);
  if(phase){ack(old);old=next();CHECK(old.kind==DSR_AUTO_EFFECT_SET_TRV);}
  CHECK(!dsr_auto_manual(&m,3,111));CHECK(!dsr_auto_manual(&m,4,112));
  dsr_auto_complete(&m,&old,phase==2?DSR_IO:DSR_OK);
  CHECK(!dsr_auto_manual(&m,3,111));journal();CHECK(dsr_auto_manual(&m,3,111));CHECK(dsr_auto_manual(&m,4,112));
  sample(0,false,0,0);journal();CHECK(!m.record.cycles[0].active);
  dsr_auto_effect_t e;CHECK(!dsr_auto_next(&m,now,utc,&e));
 }
 /* Failed persistence cannot authorize a same-value manual intent. */
 init();journal();CHECK(!dsr_auto_manual(&m,3,1));dsr_auto_effect_t e=next();
 CHECK(e.kind==DSR_AUTO_EFFECT_JOURNAL);dsr_auto_complete(&m,&e,DSR_IO);
 CHECK(!dsr_auto_manual(&m,3,1));CHECK(!dsr_auto_next(&m,now,utc,&e));
}
static void stale_effect_ack(void){
 init();dsr_auto_effect_t e=next(),stale=e;stale.id++;
 dsr_auto_complete(&m,&stale,DSR_OK);CHECK(m.pending.id==e.id);CHECK(m.durable_generation!=m.record.generation);
 ack(e);e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV);ack(e);
 uint32_t generation=m.record.generation;ack(e);CHECK(m.record.generation==generation);
}
static void fresh_epoch_all_participants(void){
 init();journal();dsr_auto_pause(&m,now,utc);
 dsr_auto_effect_t e=next();read_ack(e,20);read_ack(next(),22);
 sample(0,true,0,0);sample(1,false,0,0);CHECK(!dsr_auto_next(&m,now,utc,&e)); /* same second */
 now+=1000;utc++;sample(0,true,0,0);CHECK(!dsr_auto_next(&m,now,utc,&e));
 sample(1,false,0,0);e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV);
 /* Clock rollback revokes all witnesses and prevents the next write. */
 ack(e);journal();utc-=10;CHECK(!dsr_auto_next(&m,now,utc,&e));CHECK(!m.ready);
}
static void unsupported_baseline(void){
 init();dsr_auto_pause(&m,now,utc);read_ack(next(),19.75);CHECK(m.fault==DSR_UNSUPPORTED);dsr_auto_effect_t e;
 CHECK(!dsr_auto_next(&m,now,utc,&e));CHECK(!m.record.cycles[0].active);
}
static void binding_reuse(void){
 init();journal();dsr_auto_observation_t o={.key=rules.keys[2],.valid=true};strcpy(o.key.device_id,"different");
 dsr_auto_observe(&m,&o);CHECK(m.fault==DSR_IDENTITY_MISMATCH);dsr_auto_effect_t e;CHECK(!dsr_auto_next(&m,now,utc,&e));
 init();journal();rules.keys[2].channel++;CHECK(dsr_auto_configure(&m,&rules)==DSR_CONFLICT);
}
static void recovery_never_replays(void){
 for(unsigned phase=DSR_AUTO_PREPARED;phase<=DSR_AUTO_RESTORE_VERIFY;phase++){
  init();journal();m.record.targets[2].phase=(uint8_t)phase;
  if(phase>=DSR_AUTO_RESTORE_PREPARED)m.record.targets[2].desired=40;
  dsr_auto_record_t record=m.record;CHECK(dsr_auto_recover(&m,&record));
  dsr_auto_pause(&m,now,utc);now+=1000;utc++;sample(0,true,0,0);sample(1,false,0,0);
  for(unsigned i=0;i<2;i++){
   dsr_auto_effect_t e=next();if(e.kind==DSR_AUTO_EFFECT_JOURNAL){ack(e);e=next();}
   read_ack(e,e.layout_id==3?20:22);
  }
  dsr_auto_effect_t e;
  for(unsigned j=0;j<5;j++){
   if(!dsr_auto_next(&m,now,utc,&e))break;
   CHECK(e.kind!=DSR_AUTO_EFFECT_SET_TRV);ack(e);
  }
 }
}
static void external_change_and_partial_close(void){
 init();journal();set_and_confirm(16);set_and_confirm(16);
 sample(1,true,0,0);sample(0,false,0,0);dsr_auto_effect_t e;
 CHECK(!dsr_auto_next(&m,now,utc,&e)||e.kind==DSR_AUTO_EFFECT_READ_TRV);
 if(m.pending.kind)read_ack(m.pending,8);
 now+=15001;utc+=15;sample(0,false,0,0);sample(1,true,0,0);e=next();read_ack(e,25);journal();
 CHECK(m.record.targets[e.layout_id-1].override);
 sample(1,false,0,0);journal();now+=15001;utc+=15;sample(0,false,0,0);sample(1,false,0,0);
 read_ack(next(),8);e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV);
 CHECK(e.layout_id!=3);CHECK(e.target_half==44);
}
static void late_read_after_pause(void){
 init();journal();dsr_auto_pause(&m,now,utc);dsr_auto_effect_t old=next();
 now+=1000;utc++;dsr_auto_pause(&m,now,utc);
 dsr_auto_observation_t o={.key=rules.keys[old.layout_id-1],.valid=true,.read_id=old.id,
  .target_c=20,.started_ms=now-1000,.sample_ms=now};dsr_auto_observe(&m,&o);ack(old);
 CHECK(!m.observations[old.layout_id-1].valid);
}
static void independent_rules(void){
 init();rules.count=2;rules.items[0].sensors=1;rules.items[0].targets=4;
 rules.items[1]=rules.items[0];rules.items[1].id=8;rules.items[1].sensors=2;rules.items[1].targets=8;
 CHECK(dsr_auto_configure(&m,&rules)==DSR_OK);journal();dsr_auto_pause(&m,now,utc);
 now+=1000;utc++;sample(0,true,0,0);read_ack(next(),20);read_ack(next(),22);
 /* Missing contact of rule8 must not starve the complete rule7. */
 journal();dsr_auto_effect_t e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV);CHECK(e.rule_id==7);CHECK(e.layout_id==3);
}
static void durable_manual_is_per_target(void){
 init();journal();CHECK(!dsr_auto_manual(&m,3,71));journal();CHECK(dsr_auto_manual(&m,3,71));
 CHECK(!dsr_auto_manual(&m,4,72));CHECK(dsr_auto_manual(&m,3,71)); /* another dirty target cannot undo this proof */
 dsr_auto_effect_t e=next();CHECK(!dsr_auto_manual(&m,3,73));ack(e);
 CHECK(!dsr_auto_manual(&m,3,73));journal();CHECK(dsr_auto_manual(&m,3,73));
}
static void add_rule_preserves_other_cycle(void){
 init();journal();set_and_confirm(16);set_and_confirm(16);
 rules.count=2;rules.key_count=6;rules.items[1]=(dsr_auto_rule_t){.id=8,.signature=200,.sensors=16,.targets=32,.target_half=16,.enabled=1};
 for(unsigned i=4;i<6;i++){rules.keys[i]=rules.keys[i-4];rules.keys[i].layout_id=i+1;rules.keys[i].kind=i==4?DSR_AUTO_CONTACT:DSR_AUTO_TRV;}
 CHECK(dsr_auto_configure(&m,&rules)==DSR_OK);CHECK(m.record.cycles[0].active==1);
 CHECK(m.record.targets[2].baseline==40&&m.record.targets[3].baseline==44&&m.record.targets[2].phase==DSR_AUTO_OWNED);
}
static void missing_clock_does_not_block_manual_journal(void){
 init();journal();dsr_auto_pause(&m,now,0);CHECK(!dsr_auto_manual(&m,3,99));
 dsr_auto_effect_t e={0};CHECK(dsr_auto_next(&m,now,0,&e)&&e.kind==DSR_AUTO_EFFECT_JOURNAL);ack(e);CHECK(dsr_auto_manual(&m,3,99));
}
static void first_valid_clock_sets_resume_barrier(void){
 init();dsr_auto_pause(&m,now,0);dsr_auto_effect_t e={0};CHECK(!dsr_auto_next(&m,now,0,&e));
 CHECK(!dsr_auto_next(&m,now+1000,utc,&e));CHECK(m.barrier_utc==utc);
}
static void reopen_cancels_unsent_restore_without_losing_baseline(void){
 init();journal();set_and_confirm(16);set_and_confirm(16);sample(0,false,0,0);journal();
 CHECK(m.record.cycles[0].active==3);read_ack(next(),8);dsr_auto_effect_t e=next();CHECK(e.kind==DSR_AUTO_EFFECT_SET_TRV&&e.target_half==40);
 sample(0,true,0,0);CHECK(!dsr_auto_effect_allowed(&m,&e,now,utc));CHECK(dsr_auto_unqueue(&m,&e));
 for(unsigned n=0;n<35&&m.record.cycles[0].cycle<2;n++){
  now+=1000;utc++;sample(0,true,0,0);sample(1,false,0,0);
  if(!dsr_auto_next(&m,now,utc,&e))continue;
  CHECK(e.kind!=DSR_AUTO_EFFECT_SET_TRV);if(e.kind==DSR_AUTO_EFFECT_READ_TRV)read_ack(e,8);else ack(e);
 }
 CHECK(m.record.cycles[0].cycle==2);CHECK(m.record.targets[2].baseline==40&&m.record.targets[3].baseline==44);
 CHECK(m.record.targets[2].phase==DSR_AUTO_OWNED&&m.record.targets[3].phase==DSR_AUTO_OWNED);
}
static void uncertain_restore_at_setback_remains_uncertain(void){
 init();journal();set_and_confirm(16);set_and_confirm(16);sample(0,false,0,0);journal();
 dsr_auto_record_t r=m.record;CHECK(dsr_auto_recover(&m,&r));dsr_auto_pause(&m,now,utc);now+=1000;utc++;
 sample(0,false,0,0);sample(1,false,0,0);dsr_auto_effect_t e=next();read_ack(e,8);
 CHECK(m.record.targets[e.layout_id-1].phase==DSR_AUTO_UNCERTAIN&&!m.record.targets[e.layout_id-1].override);
}
int main(void){
#define RUN(c) do{fprintf(stderr,"case: %s\n",#c);c();}while(0)
 RUN(restore_requires_new_direct_read);
 RUN(first_open_all_closed);RUN(no_raise_already_lower);RUN(manual_during_prepared_inflight_uncertain);RUN(stale_effect_ack);
 RUN(fresh_epoch_all_participants);RUN(unsupported_baseline);RUN(binding_reuse);RUN(recovery_never_replays);
 RUN(external_change_and_partial_close);RUN(late_read_after_pause);
 RUN(independent_rules);
 RUN(durable_manual_is_per_target);
 RUN(add_rule_preserves_other_cycle);
 RUN(missing_clock_does_not_block_manual_journal);RUN(first_valid_clock_sets_resume_barrier);
 RUN(reopen_cancels_unsent_restore_without_losing_baseline);RUN(uncertain_restore_at_setback_remains_uncertain);
 printf("automation model: %u checks passed\n",checks);return 0;
}
