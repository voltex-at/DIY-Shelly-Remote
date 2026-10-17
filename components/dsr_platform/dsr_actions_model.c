#include "dsr_actions_private.h"
#include <string.h>
static bool key_valid(const dsr_action_key_t *key)
{
 return key&&key->layout_id&&key->layout_id<=1000000&&key->device_id[0]&&memchr(key->device_id,0,64)&&
  memchr(key->host,0,16)&&dsr_ipv4_valid(key->host)&&(key->generation==1||key->generation==2)&&key->kind<=1&&(key->kind==0||key->generation==2)&&key->channel<=15;
}
bool dsr_action_key_equal(const dsr_action_key_t *a,const dsr_action_key_t *b)
{return key_valid(a)&&key_valid(b)&&a->layout_id==b->layout_id&&a->generation==b->generation&&a->kind==b->kind&&a->channel==b->channel&&!strcmp(a->device_id,b->device_id)&&!strcmp(a->host,b->host);}
static bool nonce_valid(const char *nonce)
{if(!nonce)return false;for(unsigned i=0;i<16;i++)if(!((nonce[i]>='0'&&nonce[i]<='9')||(nonce[i]>='a'&&nonce[i]<='f')))return false;return nonce[16]==0;}
bool dsr_actions_model_duplicate(const dsr_actions_model_t *m,uint32_t boot_id,const char *nonce)
{return m&&m->run_id&&m->boot_id==boot_id&&nonce_valid(nonce)&&!strcmp(m->nonce,nonce);}
static void done(dsr_action_step_t *s,dsr_action_outcome_t outcome,dsr_result_t result)
{s->phase=DSR_ACTION_DONE;s->outcome=outcome;s->result=result;}
bool dsr_actions_model_begin(dsr_actions_model_t *m,const dsr_action_step_t *steps,size_t count,
 uint32_t run_id,uint32_t expected_generation,uint32_t boot_id,uint32_t object_id,uint32_t revision,const char nonce[17],uint8_t source,uint8_t kind,uint32_t now)
{
 if(!m||m->active||(m->boot_id&&m->boot_id!=boot_id)||m->generation==UINT32_MAX||m->generation!=expected_generation||!steps||!count||count>DSR_ACTION_STEPS||!run_id||!boot_id||!object_id||object_id>1000000||!revision||!nonce_valid(nonce)||source>1||kind>1||dsr_actions_model_duplicate(m,boot_id,nonce))return false;
 for(size_t i=0;i<count;i++)if(!steps[i].key.layout_id||steps[i].key.layout_id>1000000||steps[i].command<DSR_ACTION_ON||steps[i].command>DSR_ACTION_STOP)return false;
 uint32_t generation=m->generation+1u;memset(m,0,sizeof(*m));m->generation=generation;m->run_id=run_id;m->boot_id=boot_id;m->object_id=object_id;m->revision=revision;m->admitted_ms=now;m->source=source;m->kind=kind;m->count=(uint8_t)count;m->active=true;memcpy(m->nonce,nonce,17);
 for(size_t i=0;i<count;i++){
  m->steps[i].key=steps[i].key;m->steps[i].command=steps[i].command;
  if(!key_valid(&steps[i].key))done(&m->steps[i],DSR_ACTION_FAILED,DSR_IDENTITY_MISMATCH);
  else if((steps[i].key.kind==0)!=(steps[i].command<=DSR_ACTION_OFF))done(&m->steps[i],DSR_ACTION_FAILED,DSR_UNSUPPORTED);
 }
 return true;
}
void dsr_actions_model_cancel(dsr_actions_model_t *m){if(m&&m->active)m->cancel=true;}
static bool expired(dsr_actions_model_t *m,uint32_t now)
{if((int32_t)(now-m->admitted_ms)>=(int32_t)DSR_ACTION_DEADLINE_MS)m->cancel=true;return m->cancel;}
static bool pending(const dsr_action_step_t *s)
{return s->phase==DSR_ACTION_PROBE_QUEUED||s->phase==DSR_ACTION_PROBE_SENT||s->phase==DSR_ACTION_WRITE_QUEUED||s->phase==DSR_ACTION_WRITE_SENT;}
int dsr_actions_model_current(dsr_actions_model_t *m,uint32_t now)
{
 if(!m||!m->active)return -1;
 if(expired(m,now))for(unsigned i=m->current;i<m->count;i++)if(m->steps[i].phase!=DSR_ACTION_DONE&&!pending(&m->steps[i]))done(&m->steps[i],DSR_ACTION_CANCELLED,DSR_BUSY);
 while(m->current<m->count&&m->steps[m->current].phase==DSR_ACTION_DONE)m->current++;
 if(m->current<m->count)return (int)m->current;
 m->active=false;
 /* Terminal public summary keeps numeric IDs/results; private locators and
  * physical identities are wiped after the final exact acknowledgement. */
 for(unsigned i=0;i<m->count;i++){memset(m->steps[i].key.device_id,0,64);memset(m->steps[i].key.host,0,16);}
 return -1;
}
bool dsr_actions_model_check(dsr_actions_model_t *m,unsigned index,const dsr_action_key_t *actual,
 dsr_result_t gate,bool fresh,bool capable,uint32_t now)
{
 if(!m||!m->active||dsr_actions_model_current(m,now)!=(int)index)return false;
 dsr_action_step_t *s=&m->steps[index];
 if(s->phase!=DSR_ACTION_RESOLVE&&s->phase!=DSR_ACTION_PROBE_READY&&s->phase!=DSR_ACTION_WRITE_READY)return false;
 if(!dsr_action_key_equal(&s->key,actual)){done(s,DSR_ACTION_FAILED,DSR_IDENTITY_MISMATCH);return false;}
 if(gate!=DSR_OK){done(s,DSR_ACTION_FAILED,gate);return false;}
 if(!fresh){if(s->probed){done(s,DSR_ACTION_FAILED,DSR_OFFLINE);return false;}s->phase=DSR_ACTION_PROBE_READY;return true;}
 if(!capable){done(s,DSR_ACTION_FAILED,DSR_UNSUPPORTED);return false;}
 s->phase=DSR_ACTION_WRITE_READY;return true;
}
bool dsr_actions_model_queued(dsr_actions_model_t *m,unsigned index,uint32_t request,uint32_t now)
{
 if(!request||!m||!m->active||dsr_actions_model_current(m,now)!=(int)index)return false;
 dsr_action_step_t *s=&m->steps[index];if(s->phase!=DSR_ACTION_PROBE_READY&&s->phase!=DSR_ACTION_WRITE_READY)return false;
 s->request_id=request;s->phase=s->phase==DSR_ACTION_PROBE_READY?DSR_ACTION_PROBE_QUEUED:DSR_ACTION_WRITE_QUEUED;return true;
}
void dsr_actions_model_unqueue(dsr_actions_model_t *m,uint32_t request)
{
 if(!m||!m->active||m->current>=m->count)return;
 dsr_action_step_t *s=&m->steps[m->current];if(s->request_id!=request)return;
 if(s->phase==DSR_ACTION_PROBE_QUEUED)s->phase=DSR_ACTION_PROBE_READY;else if(s->phase==DSR_ACTION_WRITE_QUEUED)s->phase=DSR_ACTION_WRITE_READY;else return;
 s->request_id=0;
}
bool dsr_actions_model_worker_allowed(dsr_actions_model_t *m,uint32_t request,const dsr_action_key_t *actual,uint32_t now)
{
 if(!m||!m->active||m->current>=m->count)return false;
 dsr_action_step_t *s=&m->steps[m->current];if(!request||request!=s->request_id||!pending(s))return false;
 if(!dsr_action_key_equal(&s->key,actual))s->rejected=DSR_IDENTITY_MISMATCH;
 if(expired(m,now)&&s->rejected==DSR_OK)s->rejected=DSR_BUSY;
 if(s->rejected!=DSR_OK)return false;
 if(s->phase==DSR_ACTION_PROBE_QUEUED){s->phase=DSR_ACTION_PROBE_SENT;s->probed=true;}
 else if(s->phase==DSR_ACTION_WRITE_QUEUED)s->phase=DSR_ACTION_WRITE_SENT;
 return true;
}
bool dsr_actions_model_write_attempted(dsr_actions_model_t *m,uint32_t request)
{
 if(!m||!m->active||m->current>=m->count)return false;
 dsr_action_step_t *s=&m->steps[m->current];if(!request||s->request_id!=request||s->phase!=DSR_ACTION_WRITE_SENT)return false;
 s->write_attempted=true;return true;
}
bool dsr_actions_model_complete(dsr_actions_model_t *m,uint32_t request,dsr_result_t result,uint32_t now)
{
 if(!m||!m->active||m->current>=m->count)return false;
 dsr_action_step_t *s=&m->steps[m->current];if(!request||s->request_id!=request||!pending(s))return false;
 (void)expired(m,now);bool sent=s->phase==DSR_ACTION_PROBE_SENT||s->phase==DSR_ACTION_WRITE_SENT;
 bool write=s->phase==DSR_ACTION_WRITE_QUEUED||s->phase==DSR_ACTION_WRITE_SENT;
 if(!sent){done(s,m->cancel?DSR_ACTION_CANCELLED:DSR_ACTION_FAILED,s->rejected!=DSR_OK?s->rejected:result==DSR_OK?DSR_BAD_RESPONSE:result);return true;}
 if(!write){
  if(m->cancel)done(s,DSR_ACTION_CANCELLED,DSR_BUSY);
  else if(result!=DSR_OK)done(s,DSR_ACTION_FAILED,result);
  else{s->phase=DSR_ACTION_RESOLVE;s->request_id=0;}
 }else if(result==DSR_OK)done(s,DSR_ACTION_SUCCESS,DSR_OK);
 else if(!s->write_attempted&&(result==DSR_AUTH_REQUIRED||result==DSR_IDENTITY_MISMATCH||result==DSR_RATE_LIMITED||result==DSR_UNSUPPORTED||result==DSR_INVALID||result==DSR_CONFLICT))done(s,DSR_ACTION_FAILED,result);
 else {
  done(s,DSR_ACTION_UNCERTAIN,result);
  /* A later scene entry for the same frozen endpoint must not become an
   * implicit retry after a lost write/readback. Other endpoints can finish. */
  for(unsigned i=m->current+1;i<m->count;i++)if(dsr_action_key_equal(&s->key,&m->steps[i].key))done(&m->steps[i],DSR_ACTION_CANCELLED,DSR_BUSY);
 }
 return true;
}
void dsr_action_log_add(dsr_action_log_model_t *log,const dsr_action_log_entry_t *entry)
{
 if(!log||!entry)return;
 if(!log->epoch)log->epoch=1;
 dsr_action_log_entry_t value=*entry;value.sequence=++log->serial;if(!value.sequence)value.sequence=++log->serial;
 if(value.utc<1735689600u)value.utc=0;
 value.reserved=0;log->entries[log->next]=value;log->next=(uint16_t)((log->next+1)%DSR_ACTION_LOG_COUNT);if(log->count<DSR_ACTION_LOG_COUNT)log->count++;
}
void dsr_action_log_clear(dsr_action_log_model_t *log)
{if(!log)return;uint32_t epoch=log->epoch+1u;if(!epoch)epoch=1;memset(log,0,sizeof(*log));log->epoch=epoch;}
bool dsr_action_log_page(const dsr_action_log_model_t *log,uint32_t expected_epoch,unsigned offset,unsigned limit,dsr_action_log_page_t *page)
{
 if(!log||!page||!limit||limit>DSR_ACTION_LOG_PAGE||offset>DSR_ACTION_LOG_COUNT||(expected_epoch&&expected_epoch!=log->epoch))return false;
 memset(page,0,sizeof(*page));page->epoch=log->epoch;page->newest=log->serial;page->total=log->count;
 for(unsigned i=offset;i<log->count&&page->count<limit;i++){unsigned at=(log->next+DSR_ACTION_LOG_COUNT-1-i)%DSR_ACTION_LOG_COUNT;page->entries[page->count++]=log->entries[at];}return true;
}
