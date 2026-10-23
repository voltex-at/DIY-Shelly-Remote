#include "dsr_automation_model.h"
#include <math.h>
#include <string.h>

static bool key_valid(const dsr_auto_key_t *k)
{
 return k&&k->layout_id&&k->layout_id<=1000000&&k->device_id[0]&&k->host[0]&&
  memchr(k->device_id,0,sizeof(k->device_id))&&memchr(k->host,0,sizeof(k->host))&&
  k->generation==2&&k->channel<=DSR_MAX_CHANNEL&&
  (k->kind==DSR_AUTO_CONTACT||k->kind==DSR_AUTO_TRV)&&!k->reserved;
}
bool dsr_auto_key_equal(const dsr_auto_key_t *a,const dsr_auto_key_t *b)
{
 return key_valid(a)&&key_valid(b)&&a->layout_id==b->layout_id&&a->generation==b->generation&&
  a->channel==b->channel&&a->kind==b->kind&&!strcmp(a->device_id,b->device_id)&&!strcmp(a->host,b->host);
}
static int index_of(const dsr_auto_model_t *m,uint32_t id)
{for(unsigned i=0;i<m->record.rules.key_count;i++)if(m->record.rules.keys[i].layout_id==id)return (int)i;return -1;}
static bool rules_valid(const dsr_auto_rules_t *r)
{
 if(!r||r->count>DSR_AUTO_RULES||r->key_count>DSR_AUTO_PARTICIPANTS)return false;
 uint16_t claimed=0;uint32_t mask=(1u<<r->key_count)-1u;
 uint16_t enabled=0;
 for(unsigned i=0;i<r->count;i++)if(r->items[i].enabled)enabled|=(uint16_t)(r->items[i].sensors|r->items[i].targets);
 for(unsigned i=0;i<r->key_count;i++){
  const dsr_auto_key_t *k=&r->keys[i];
  bool unresolved=k->layout_id&&k->layout_id<=1000000&&(k->kind==DSR_AUTO_CONTACT||k->kind==DSR_AUTO_TRV)&&
   !k->device_id[0]&&!k->host[0]&&!k->generation&&!k->reserved;
  if(!key_valid(k)&&((enabled&(1u<<i))||!unresolved))return false;
  for(unsigned j=0;j<i;j++)if(r->keys[i].layout_id==r->keys[j].layout_id)return false;
 }
 for(unsigned i=0;i<r->count;i++){
  const dsr_auto_rule_t *v=&r->items[i];
  if(!v->id||v->id>1000000||!v->signature||!v->sensors||!v->targets||
    ((v->sensors|v->targets)&~mask)||(v->sensors&v->targets)||(claimed&v->targets)||
    v->target_half<DSR_TRV_MIN_HALF||v->target_half>DSR_TRV_MAX_HALF||v->enabled>1||v->reserved)return false;
  for(unsigned j=0;j<i;j++)if(v->id==r->items[j].id)return false;
  for(unsigned j=0;j<r->key_count;j++){
   if((v->sensors&(1u<<j))&&r->keys[j].kind!=DSR_AUTO_CONTACT)return false;
   if((v->targets&(1u<<j))&&r->keys[j].kind!=DSR_AUTO_TRV)return false;
  }
  claimed|=v->targets;
 }
 return true;
}
static bool changed(dsr_auto_model_t *m)
{
 if(m->record.generation==UINT32_MAX){m->fault=DSR_FULL;return false;}
 m->record.generation++;return true;
}
void dsr_auto_init(dsr_auto_model_t *m){if(m)memset(m,0,sizeof(*m));}
bool dsr_auto_record_valid(const dsr_auto_record_t *r)
{
 if(!r||!rules_valid(&r->rules)||!r->generation||r->rules.enroll)return false;
 for(unsigned i=0;i<r->rules.count;i++)if(r->cycles[i].active>DSR_AUTO_CYCLE_RESTORING||
  (r->cycles[i].active&&(!r->rules.items[i].enabled||!r->cycles[i].cycle))||
  r->cycles[i].reserved[0]||r->cycles[i].reserved[1]||r->cycles[i].reserved[2])return false;
 for(unsigned i=0;i<r->rules.key_count;i++){
  const dsr_auto_target_t *t=&r->targets[i];
  if(t->phase>DSR_AUTO_UNCERTAIN||t->override>1||
   (t->phase&&(t->baseline<8||t->baseline>60||t->desired<8||t->desired>60)))return false;
 }
 return true;
}
bool dsr_auto_recover(dsr_auto_model_t *m,const dsr_auto_record_t *r)
{
 if(!m||!dsr_auto_record_valid(r))return false;
 dsr_auto_init(m);m->record=*r;m->record.rules.enroll=false;
 m->loaded=true;m->durable_generation=r->generation;
 /* Prepared may already have reached the radio. Recovery NEVER replays it. */
 for(unsigned i=0;i<r->rules.key_count;i++){
  dsr_auto_target_t *t=&m->record.targets[i];
  if(t->override)m->durable_manual[i]=t->manual_intent;
  if(t->phase==DSR_AUTO_PREPARED||t->phase==DSR_AUTO_VERIFY||
    t->phase==DSR_AUTO_RESTORE_PREPARED||t->phase==DSR_AUTO_RESTORE_VERIFY)t->phase=DSR_AUTO_UNCERTAIN;
 }
 for(unsigned i=0;i<r->rules.count;i++)if(m->record.cycles[i].active==2)m->record.cycles[i].active=0;
 return true;
}
static bool same_members(const dsr_auto_rules_t *a,uint16_t am,const dsr_auto_rules_t *b,uint16_t bm)
{
 unsigned ac=0,bc=0;
 for(unsigned i=0;i<a->key_count;i++)if(am&(1u<<i)){
  ac++;bool found=false;for(unsigned j=0;j<b->key_count;j++)if((bm&(1u<<j))&&dsr_auto_key_equal(&a->keys[i],&b->keys[j]))found=true;
  if(!found)return false;
 }
 for(unsigned j=0;j<b->key_count;j++)if(bm&(1u<<j))bc++;
 return ac==bc;
}
static bool same_rule(const dsr_auto_rules_t *a,const dsr_auto_rule_t *ar,const dsr_auto_rules_t *b,const dsr_auto_rule_t *br)
{return ar->id==br->id&&ar->target_half==br->target_half&&same_members(a,ar->sensors,b,br->sensors)&&same_members(a,ar->targets,b,br->targets);}
dsr_result_t dsr_auto_configure(dsr_auto_model_t *m,const dsr_auto_rules_t *r)
{
 if(!m||!rules_valid(r))return DSR_INVALID;
 dsr_auto_rules_t normalized=*r;normalized.enroll=false;
 if(!memcmp(&normalized,&m->record.rules,sizeof(normalized)))return DSR_OK;
 if(m->pending.kind)return DSR_BUSY;
 for(unsigned i=0;i<m->record.rules.count;i++)if(m->record.cycles[i].active){
  const dsr_auto_rule_t *old=&m->record.rules.items[i];
  for(unsigned j=0;j<r->count;j++)if(r->items[j].id==old->id&&r->items[j].enabled&&
   !same_rule(&m->record.rules,old,r,&r->items[j]))return DSR_CONFLICT;
 }
 bool needs_enroll=false;
 for(unsigned j=0;j<r->count;j++)if(r->items[j].enabled){
  bool match=false;
  for(unsigned i=0;i<m->record.rules.count;i++)if(m->record.rules.items[i].enabled&&
   same_rule(&m->record.rules,&m->record.rules.items[i],r,&r->items[j]))match=true;
  if(!match)needs_enroll=true;
 }
 if(needs_enroll&&!r->enroll){m->fault=DSR_CONFLICT;return DSR_CONFLICT;}
 dsr_auto_cycle_t cycles[DSR_AUTO_RULES]={0};
 for(unsigned j=0;j<r->count;j++)for(unsigned i=0;i<m->record.rules.count;i++)
  if(r->items[j].enabled&&same_rule(&m->record.rules,&m->record.rules.items[i],r,&r->items[j]))
   cycles[j]=m->record.cycles[i];
 /* Remap by exact enrolled identity, never by a mutable participant index. */
 dsr_auto_target_t targets[DSR_AUTO_PARTICIPANTS]={0};
 dsr_auto_observation_t observations[DSR_AUTO_PARTICIPANTS]={0};
 uint32_t times[4][DSR_AUTO_PARTICIPANTS]={{0}};
 for(unsigned j=0;j<r->key_count;j++)for(unsigned i=0;i<m->record.rules.key_count;i++)
  if(dsr_auto_key_equal(&r->keys[j],&m->record.rules.keys[i])){
   targets[j]=m->record.targets[i];observations[j]=m->observations[i];times[0][j]=m->read_ms[i];
   times[1][j]=m->write_ms[i];times[2][j]=m->before_write_read[i];times[3][j]=m->durable_manual[i];
  }
 memcpy(m->record.targets,targets,sizeof(targets));memcpy(m->observations,observations,sizeof(observations));
 memcpy(m->read_ms,times[0],sizeof(m->read_ms));memcpy(m->write_ms,times[1],sizeof(m->write_ms));
 memcpy(m->before_write_read,times[2],sizeof(m->before_write_read));memcpy(m->durable_manual,times[3],sizeof(m->durable_manual));
 memcpy(m->record.cycles,cycles,sizeof(cycles));m->record.rules=normalized;m->fault=DSR_OK;
 m->loaded=true;m->ready_rules=0;changed(m);return DSR_OK;
}
void dsr_auto_pause(dsr_auto_model_t *m,uint32_t ms,uint32_t utc)
{
 if(!m)return;
 m->now_ms=ms;m->barrier_utc=utc;m->last_utc=utc;m->epoch=true;m->ready=false;m->read_received=false;
 if(m->epoch_id==UINT32_MAX)m->fault=DSR_FULL;else m->epoch_id++;
 m->ready_rules=0;for(unsigned r=0;r<DSR_AUTO_RULES;r++)m->rule_barrier_utc[r]=utc;
 for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++){
  m->observations[i].valid=false;m->observations[i].read_id=0;m->read_ms[i]=0;
 }
}
void dsr_auto_observe(dsr_auto_model_t *m,const dsr_auto_observation_t *o)
{
 if(!m||!o)return;
 int i=index_of(m,o->key.layout_id);if(i<0)return;
 if(!dsr_auto_key_equal(&o->key,&m->record.rules.keys[i])){m->fault=DSR_IDENTITY_MISMATCH;return;}
 if(o->key.kind==DSR_AUTO_CONTACT){m->observations[i]=*o;return;}
 /* Mirrored probes are NOT TRV evidence, even if a battery packet is new. */
 if(!o->read_id)return;
 if(m->pending.kind!=DSR_AUTO_EFFECT_READ_TRV||o->read_id!=m->pending.id||
  o->key.layout_id!=m->pending.layout_id||m->pending_epoch!=m->epoch_id)return;
 if(!o->valid||!isfinite(o->target_c)||o->target_c*2.0<8||o->target_c*2.0>60||
  floor(o->target_c*2.0)!=o->target_c*2.0){m->fault=DSR_UNSUPPORTED;return;}
 if((int32_t)(o->started_ms-m->write_ms[i])<0)return;
 m->observations[i]=*o;m->read_received=true;
}
static bool participating(const dsr_auto_model_t *m,unsigned i)
{
 for(unsigned r=0;r<m->record.rules.count;r++){
  if(m->record.rules.items[r].enabled&&((m->record.rules.items[r].sensors|m->record.rules.items[r].targets)&(1u<<i)))return true;
 }
 return false;
}
static int active_rule(const dsr_auto_model_t *m,unsigned i)
{
 for(unsigned r=0;r<m->record.rules.count;r++){
  if(m->record.cycles[r].active&&(m->record.rules.items[r].targets&(1u<<i)))return (int)r;
 }
 return -1;
}
bool dsr_auto_manual(dsr_auto_model_t *m,uint32_t id,uint32_t intent)
{
 if(!m||!intent)return false;
 int i=index_of(m,id);if(i<0||active_rule(m,(unsigned)i)<0)return true;
 dsr_auto_target_t *t=&m->record.targets[i];
 if(!t->override||t->manual_intent!=intent){
  t->override=1;t->manual_intent=intent;
  /* Only a new explicit manual intent retries a failed journal write. */
  if(m->fault==DSR_IO)m->fault=DSR_OK;
  changed(m);
 }
 return m->durable_manual[i]==intent;
}
static bool fresh(const dsr_auto_model_t *m,unsigned i,uint32_t now,uint32_t utc)
{
 const dsr_auto_observation_t *o=&m->observations[i];
 if(!o->valid||(uint32_t)(now-o->sample_ms)>DSR_AUTO_FRESH_MS)return false;
 if(m->record.rules.keys[i].kind==DSR_AUTO_TRV)return o->read_id!=0;
 return o->source_utc>m->barrier_utc&&o->source_utc<=utc&&utc-o->source_utc<=DSR_AUTO_CONTACT_AGE_S;
}
static bool rule_fresh(const dsr_auto_model_t *m,unsigned r,uint32_t now,uint32_t utc)
{
 const dsr_auto_rule_t *v=&m->record.rules.items[r];
 for(unsigned i=0;i<m->record.rules.key_count;i++)if((v->sensors|v->targets)&(1u<<i)){
  if(!fresh(m,i,now,utc))return false;
  if((v->sensors&(1u<<i))&&m->observations[i].source_utc<=m->rule_barrier_utc[r])return false;
 }
 return true;
}
static bool emit(dsr_auto_model_t *m,dsr_auto_effect_t *out,uint8_t kind,unsigned r,unsigned i)
{
 if(m->next_id==UINT32_MAX){m->fault=DSR_FULL;return false;}
 dsr_auto_effect_t e={.id=++m->next_id,.generation=m->record.generation,.kind=kind};
 if(kind!=DSR_AUTO_EFFECT_JOURNAL){
  e.layout_id=m->record.rules.keys[i].layout_id;
  if(r<DSR_AUTO_RULES){e.rule_id=m->record.rules.items[r].id;e.cycle=m->record.cycles[r].cycle;}
  e.target_half=m->record.targets[i].desired;
 }
 if(kind==DSR_AUTO_EFFECT_JOURNAL)for(unsigned i=0;i<DSR_AUTO_PARTICIPANTS;i++)
  m->journal_manual[i]=m->record.targets[i].override?m->record.targets[i].manual_intent:0;
 m->pending=e;m->pending_epoch=m->epoch_id;m->read_received=false;*out=e;return true;
}
static bool reconcile(dsr_auto_model_t *m,unsigned i,uint8_t half)
{
 dsr_auto_target_t *t=&m->record.targets[i];
 if(t->override||active_rule(m,i)<0)return false;
 switch(t->phase){
 case DSR_AUTO_VERIFY:
  if(half==t->desired)t->phase=DSR_AUTO_OWNED;
  else if(half!=t->baseline)t->override=1;
  else t->phase=DSR_AUTO_UNCERTAIN;
  return true;
 case DSR_AUTO_RESTORE_VERIFY:
  if(half==t->desired)t->phase=DSR_AUTO_DONE;
  else t->phase=DSR_AUTO_UNCERTAIN;
  return true;
 case DSR_AUTO_UNCERTAIN:
  if(half==t->desired){t->phase=half==t->baseline?DSR_AUTO_DONE:DSR_AUTO_OWNED;return true;}
  if(t->desired==t->baseline){
   uint8_t limit=m->record.rules.items[active_rule(m,i)].target_half;
   if(half==(t->baseline<limit?t->baseline:limit))return false; /* restore may still arrive: read only */
  }
  if(half!=t->baseline){t->override=1;return true;}
  return false;
 case DSR_AUTO_PREPARED:
  if(half!=t->baseline){t->override=1;return true;}return false;
 case DSR_AUTO_OWNED:case DSR_AUTO_RESTORE_PREPARED:
  if(half!=(t->phase==DSR_AUTO_OWNED?t->desired:(t->baseline<m->record.rules.items[active_rule(m,i)].target_half?
   t->baseline:m->record.rules.items[active_rule(m,i)].target_half))){t->override=1;return true;}return false;
 default:return false;
 }
}
static bool rule_open(const dsr_auto_model_t *m,const dsr_auto_rule_t *v)
{for(unsigned i=0;i<m->record.rules.key_count;i++)if((v->sensors&(1u<<i))&&m->observations[i].open)return true;return false;}
bool dsr_auto_next(dsr_auto_model_t *m,uint32_t now,uint32_t utc,dsr_auto_effect_t *out)
{
 if(!m||!out)return false;
 memset(out,0,sizeof(*out));m->now_ms=now;
 bool reset=!m->epoch||!utc||!m->last_utc||utc<m->last_utc;
 if(reset)dsr_auto_pause(m,now,utc);
 m->last_utc=utc;
 if(m->pending.kind||m->fault!=DSR_OK)return false;
 if(m->record.generation!=m->durable_generation)return emit(m,out,DSR_AUTO_EFFECT_JOURNAL,0,0);
 if(reset||!utc)return false;
 for(unsigned r=0;r<m->record.rules.count;r++){
  if((m->ready_rules&(1u<<r))&&!rule_fresh(m,r,now,utc)){
   m->ready_rules&=(uint16_t)~(1u<<r);m->rule_barrier_utc[r]=utc;
   for(unsigned i=0;i<m->record.rules.key_count;i++)if(m->record.rules.items[r].targets&(1u<<i)){
    m->observations[i].valid=false;m->observations[i].read_id=0;
   }
  }
 }
 /* Pending readback is handled before the next target's SET. */
 for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<m->record.rules.key_count;i++){
  if(!participating(m,i)||m->record.rules.keys[i].kind!=DSR_AUTO_TRV)continue;
  uint8_t phase=m->record.targets[i].phase;
  bool confirming=!m->record.targets[i].override&&(phase==DSR_AUTO_VERIFY||phase==DSR_AUTO_RESTORE_VERIFY||phase==DSR_AUTO_UNCERTAIN);
  if((pass==0)!=confirming)continue;
  bool needs_proof=phase==DSR_AUTO_RESTORE_PREPARED&&m->observations[i].read_id==m->before_write_read[i];
  int owner=active_rule(m,i);
  if(owner>=0&&m->record.cycles[owner].active==DSR_AUTO_CYCLE_RESTORING&&rule_open(m,&m->record.rules.items[owner])&&
   m->observations[i].read_id==m->before_write_read[i])needs_proof=true;
  bool due=!m->observations[i].read_id||!fresh(m,i,now,utc)||confirming||needs_proof;
  if(due&&(!m->read_ms[i]||(uint32_t)(now-m->read_ms[i])>=DSR_AUTO_READ_INTERVAL_MS))
   return emit(m,out,DSR_AUTO_EFFECT_READ_TRV,active_rule(m,i)<0?DSR_AUTO_RULES:(unsigned)active_rule(m,i),i);
 }
 m->ready=false;
 for(unsigned n=0;n<m->record.rules.count;n++){
  unsigned r=(m->cursor+n)%m->record.rules.count;dsr_auto_rule_t *v=&m->record.rules.items[r];
  dsr_auto_cycle_t *c=&m->record.cycles[r];if(!v->enabled||c->active==2)continue;
  if(!rule_fresh(m,r,now,utc))continue;
  m->ready_rules|=(uint16_t)(1u<<r);m->ready=true;
  bool open=rule_open(m,v);
  if(!c->active&&open){
   if(c->cycle==UINT32_MAX){m->fault=DSR_FULL;return false;}c->cycle++;c->active=1;
   for(unsigned i=0;i<m->record.rules.key_count;i++)if(v->targets&(1u<<i)){
    uint8_t baseline=(uint8_t)(m->observations[i].target_c*2.0);
    m->record.targets[i]=(dsr_auto_target_t){.baseline=baseline,.desired=baseline<v->target_half?baseline:v->target_half,
     .phase=baseline<=v->target_half?DSR_AUTO_DONE:DSR_AUTO_PREPARED};
    m->durable_manual[i]=0;
   }
   changed(m);return emit(m,out,DSR_AUTO_EFFECT_JOURNAL,0,0);
  }
  if(!c->active)continue;
  if(open&&c->active==DSR_AUTO_CYCLE_RESTORING){
   bool dirty=false,settled=true,proofs=true;
   for(unsigned i=0;i<m->record.rules.key_count;i++)if(v->targets&(1u<<i)){
    dsr_auto_target_t *t=&m->record.targets[i];
    if(!t->override&&t->phase==DSR_AUTO_RESTORE_PREPARED){t->desired=t->baseline<v->target_half?t->baseline:v->target_half;t->phase=DSR_AUTO_OWNED;dirty=true;}
    if(!t->override&&t->phase!=DSR_AUTO_OWNED&&t->phase!=DSR_AUTO_DONE)settled=false;
    if(m->observations[i].read_id==m->before_write_read[i])proofs=false;
   }
   if(dirty){changed(m);return emit(m,out,DSR_AUTO_EFFECT_JOURNAL,0,0);}
   if(!settled||!proofs)continue;
   if(c->cycle==UINT32_MAX){m->fault=DSR_FULL;return false;}
   c->cycle++;c->active=DSR_AUTO_CYCLE_OPEN;
   for(unsigned i=0;i<m->record.rules.key_count;i++)if(v->targets&(1u<<i)){
    dsr_auto_target_t *t=&m->record.targets[i];uint8_t current=(uint8_t)(m->observations[i].target_c*2.0);
    bool owned=!t->override&&t->phase==DSR_AUTO_OWNED;
    uint8_t baseline=owned?t->baseline:current,desired=current<v->target_half?current:v->target_half;
    *t=(dsr_auto_target_t){.baseline=baseline,.desired=desired,.phase=owned&&current==desired?DSR_AUTO_OWNED:current<=v->target_half?DSR_AUTO_DONE:DSR_AUTO_PREPARED};
    m->durable_manual[i]=0;
   }
   changed(m);return emit(m,out,DSR_AUTO_EFFECT_JOURNAL,0,0);
  }
  if(!open){
   bool dirty=false,done=true;
   if(c->active==DSR_AUTO_CYCLE_OPEN){c->active=DSR_AUTO_CYCLE_RESTORING;dirty=true;
    for(unsigned i=0;i<m->record.rules.key_count;i++)if(v->targets&(1u<<i))m->before_write_read[i]=m->observations[i].read_id;}
   for(unsigned i=0;i<m->record.rules.key_count;i++)if(v->targets&(1u<<i)){
    dsr_auto_target_t *t=&m->record.targets[i];if(t->override||t->phase==DSR_AUTO_DONE)continue;
    if(t->phase==DSR_AUTO_OWNED){t->desired=t->baseline;t->phase=DSR_AUTO_RESTORE_PREPARED;
     m->before_write_read[i]=m->observations[i].read_id;dirty=true;}
    /* A never-dispatched setback can simply be cancelled. */
    if(t->phase==DSR_AUTO_PREPARED){t->phase=DSR_AUTO_DONE;dirty=true;continue;}
    done=false;
   }
   if(done){c->active=2;dirty=true;}
   if(dirty){changed(m);return emit(m,out,DSR_AUTO_EFFECT_JOURNAL,0,0);}
  }
  for(unsigned i=0;i<m->record.rules.key_count;i++)if(v->targets&(1u<<i)){
   dsr_auto_target_t *t=&m->record.targets[i];
   if(t->override)continue;
   if((open&&t->phase==DSR_AUTO_PREPARED)||(!open&&t->phase==DSR_AUTO_RESTORE_PREPARED)){
    if(t->phase==DSR_AUTO_RESTORE_PREPARED&&m->observations[i].read_id==m->before_write_read[i])continue;
    m->cursor=(uint8_t)((r+1)%m->record.rules.count);return emit(m,out,DSR_AUTO_EFFECT_SET_TRV,r,i);
   }
  }
 }
 /* Monitor external changes, at the existing BLU interval. */
 for(unsigned i=0;i<m->record.rules.key_count;i++)if(active_rule(m,i)>=0&&!m->record.targets[i].override&&
  m->record.rules.keys[i].kind==DSR_AUTO_TRV&&(uint32_t)(now-m->read_ms[i])>=DSR_AUTO_READ_INTERVAL_MS)
   return emit(m,out,DSR_AUTO_EFFECT_READ_TRV,(unsigned)active_rule(m,i),i);
 return false;
}
static bool same_effect(const dsr_auto_effect_t *a,const dsr_auto_effect_t *b)
{return a->id==b->id&&a->generation==b->generation&&a->rule_id==b->rule_id&&a->cycle==b->cycle&&
 a->intent_id==b->intent_id&&a->layout_id==b->layout_id&&a->kind==b->kind&&a->target_half==b->target_half;}
bool dsr_auto_effect_allowed(const dsr_auto_model_t *m,const dsr_auto_effect_t *e,uint32_t now,uint32_t utc)
{
 if(!m||!e||!e->id||!same_effect(e,&m->pending)||m->fault!=DSR_OK||!utc||utc<m->last_utc)return false;
 if(e->kind==DSR_AUTO_EFFECT_READ_TRV)return m->pending_epoch==m->epoch_id;
 if(e->kind!=DSR_AUTO_EFFECT_SET_TRV||e->generation!=m->record.generation||
   m->durable_generation!=m->record.generation)return false;
 int i=index_of(m,e->layout_id);if(i<0)return false;
 for(unsigned r=0;r<m->record.rules.count;r++){
  const dsr_auto_rule_t *v=&m->record.rules.items[r];const dsr_auto_cycle_t *c=&m->record.cycles[r];
  if(v->id!=e->rule_id)continue;
  const dsr_auto_target_t *t=&m->record.targets[i];
  if(!v->enabled||(c->active!=DSR_AUTO_CYCLE_OPEN&&c->active!=DSR_AUTO_CYCLE_RESTORING)||c->cycle!=e->cycle||!(v->targets&(1u<<i))||
    t->override||t->desired!=e->target_half||!rule_fresh(m,r,now,utc))return false;
  bool open=false;for(unsigned j=0;j<m->record.rules.key_count;j++)if((v->sensors&(1u<<j))&&m->observations[j].open)open=true;
  return (open&&t->phase==DSR_AUTO_PREPARED)||(!open&&t->phase==DSR_AUTO_RESTORE_PREPARED&&m->observations[i].read_id!=m->before_write_read[i]);
 }
 return false;
}
bool dsr_auto_unqueue(dsr_auto_model_t *m,const dsr_auto_effect_t *e)
{
 if(!m||!e||!e->id||!same_effect(e,&m->pending)||e->kind==DSR_AUTO_EFFECT_JOURNAL)return false;
 m->pending=(dsr_auto_effect_t){0};m->read_received=false;return true;
}
void dsr_auto_complete(dsr_auto_model_t *m,const dsr_auto_effect_t *e,dsr_result_t result)
{
 if(!m||!e||!e->id||!same_effect(e,&m->pending))return;
 m->pending=(dsr_auto_effect_t){0};
 if(e->kind==DSR_AUTO_EFFECT_JOURNAL){
  if(result!=DSR_OK){m->fault=DSR_IO;return;}
  m->durable_generation=e->generation;
  memcpy(m->durable_manual,m->journal_manual,sizeof(m->durable_manual));
  if(m->durable_generation==m->record.generation)for(unsigned r=0;r<m->record.rules.count;r++)
   if(m->record.cycles[r].active==2)m->record.cycles[r].active=0;
  return;
 }
 int i=index_of(m,e->layout_id);if(i<0)return;
 if(e->kind==DSR_AUTO_EFFECT_READ_TRV){
  m->read_ms[i]=m->now_ms;
  if(result!=DSR_OK||!m->read_received){m->observations[i].valid=false;return;}
  if(reconcile(m,(unsigned)i,(uint8_t)(m->observations[i].target_c*2.0)))changed(m);
 }else if(e->kind==DSR_AUTO_EFFECT_SET_TRV){
  dsr_auto_target_t *t=&m->record.targets[i];m->write_ms[i]=m->now_ms;
  /* Result (including timeout) never authorizes a replay or proves ownership. */
  t->phase=t->phase==DSR_AUTO_RESTORE_PREPARED?DSR_AUTO_RESTORE_VERIFY:DSR_AUTO_VERIFY;
  changed(m);
 }
}
void dsr_auto_model_snapshot(const dsr_auto_model_t *m,dsr_auto_snapshot_t *out)
{
 if(!out)return;
 memset(out,0,sizeof(*out));if(!m)return;out->count=m->record.rules.count;
 for(unsigned r=0;r<out->count;r++){
  const dsr_auto_rule_t *v=&m->record.rules.items[r];dsr_auto_rule_status_t *s=&out->rules[r];
  s->rule_id=v->id;s->cycle=m->record.cycles[r].cycle;s->result=m->fault;
  s->state=!v->enabled?DSR_AUTO_DISABLED:m->fault?DSR_AUTO_ERROR:!rule_fresh(m,r,m->now_ms,m->last_utc)?DSR_AUTO_WAITING:
    m->record.cycles[r].active==DSR_AUTO_CYCLE_RESTORING||m->record.cycles[r].active==DSR_AUTO_CYCLE_CLOSED?DSR_AUTO_CONFIRMING:
    m->record.cycles[r].active?DSR_AUTO_ACTIVE:DSR_AUTO_WAITING;
  for(unsigned i=0;i<m->record.rules.key_count;i++){
   if((v->sensors|v->targets)&(1u<<i))if(!fresh(m,i,m->now_ms,m->last_utc)&&!s->missing_id)s->missing_id=m->record.rules.keys[i].layout_id;
   if(!(v->targets&(1u<<i)))continue;
   const dsr_auto_target_t *t=&m->record.targets[i];
   if(t->override){s->overridden|=(uint16_t)(1u<<i);if(s->state==DSR_AUTO_ACTIVE)s->state=DSR_AUTO_MANUAL;}
   if(t->phase==DSR_AUTO_VERIFY||t->phase==DSR_AUTO_RESTORE_VERIFY||t->phase==DSR_AUTO_UNCERTAIN)
    if(s->state==DSR_AUTO_ACTIVE)s->state=DSR_AUTO_CONFIRMING;
  }
 }
 for(unsigned i=0;i<m->record.rules.key_count;i++)if(m->record.rules.keys[i].kind==DSR_AUTO_TRV){
  unsigned j=out->target_count++;out->targets[j].layout_id=m->record.rules.keys[i].layout_id;
  out->targets[j].baseline=m->record.targets[i].baseline;out->targets[j].requested=m->record.targets[i].desired;
  out->targets[j].read_half=m->observations[i].valid?(uint8_t)(m->observations[i].target_c*2.0):0;
  out->targets[j].phase=m->record.targets[i].phase;
 }
}
