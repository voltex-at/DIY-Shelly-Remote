/* Sole LVGL/network owner. Requested values never replace device telemetry. */
typedef struct {dsr_auto_key_t key;uint32_t intent,job;uint8_t half;bool retained,waiting,failed,awaiting;} native_trv_t;
static native_trv_t *native_trvs;
static uint32_t native_trv_serial;
static dsr_auto_key_t native_trv_key(const dsr_connection_t *c)
{
 dsr_auto_key_t k={.layout_id=c->layout_id,.generation=(uint8_t)c->device.generation,.channel=c->device.channel,.kind=DSR_AUTO_TRV};
 memcpy(k.host,c->device.host,16);memcpy(k.device_id,c->device.device_id,64);return k;
}
static bool native_trv_same(const dsr_auto_key_t *k,const dsr_connection_t *c)
{return c->layout_id==k->layout_id&&c->device.generation==k->generation&&c->device.channel==k->channel&&c->device.endpoint_kind==DSR_ENDPOINT_BLU&&!strcmp(c->device.host,k->host)&&!strcmp(c->device.device_id,k->device_id);}
static native_trv_t *native_trv_find(uint32_t id)
{if(native_trvs)for(unsigned i=0;i<DSR_MAX_DEVICES;i++)if(native_trvs[i].retained&&native_trvs[i].key.layout_id==id)return &native_trvs[i];return NULL;}
static bool native_trv_busy(uint32_t id)
{native_trv_t *p=native_trv_find(id);return p&&!p->failed;}
static bool native_trv_waiting(void)
{if(native_trvs)for(unsigned i=0;i<DSR_MAX_DEVICES;i++)if(native_trvs[i].retained&&native_trvs[i].waiting&&!native_trvs[i].failed)return true;return false;}
static dsr_result_t native_trv_request(dsr_fleet_t *f,unsigned index,uint8_t half)
{
 if(!f||index>=f->count||half<DSR_TRV_MIN_HALF||half>DSR_TRV_MAX_HALF)return DSR_INVALID;
 const dsr_fleet_slot_t *s=&f->slots[index];if(!s->layout_enabled||!s->connection.layout_id||s->connection.device.endpoint_kind!=DSR_ENDPOINT_BLU||!(s->connection.telemetry.blu.errors&DSR_BLU_TRV))return DSR_UNSUPPORTED;
 if(!native_trvs)native_trvs=heap_caps_calloc(DSR_MAX_DEVICES,sizeof(*native_trvs),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 if(!native_trvs)return DSR_IO;
 native_trv_t *p=native_trv_find(s->connection.layout_id);
 if(p&&p->job)return DSR_BUSY;
 if(!p)for(unsigned i=0;i<DSR_MAX_DEVICES;i++)if(!native_trvs[i].retained){p=&native_trvs[i];break;}
 if(!p)return DSR_FULL;
 if(!dsr_actions_native_reserve(s->connection.layout_id))return DSR_BUSY;
 if(!++native_trv_serial)++native_trv_serial;
 *p=(native_trv_t){.key=native_trv_key(&s->connection),.intent=native_trv_serial,.half=half,.retained=true,.waiting=true};
 /* Registers the RAM override immediately, before the next auto guard. */
 (void)dsr_automation_manual_ready(p->key.layout_id,p->intent);
 dsr_mobile_trv_request_state(p->key.layout_id,p->half,p->intent,true);return DSR_OK;
}
static void native_trv_tick(dsr_fleet_t *f,uint32_t now,bool wifi)
{
 if(!native_trvs)return;
 for(unsigned i=0;i<DSR_MAX_DEVICES;i++){
  native_trv_t *p=&native_trvs[i];if(!p->waiting&&!p->awaiting&&!p->failed)continue;
  int at=-1;for(unsigned j=0;j<f->count;j++)if(f->slots[j].connection.layout_id==p->key.layout_id){at=(int)j;break;}
  if(at<0||!f->slots[at].layout_enabled||!native_trv_same(&p->key,&f->slots[at].connection)){
   if(p->waiting)dsr_actions_native_release(p->key.layout_id);
   dsr_mobile_trv_request_state(p->key.layout_id,p->half,p->intent,false);memset(p,0,sizeof(*p));continue;
  }
  dsr_fleet_slot_t *s=&f->slots[at];
  if(p->failed){
   if(wifi&&s->result==DSR_OK&&s->connection.device.online&&!s->in_flight&&!s->queued){dsr_mobile_trv_request_state(p->key.layout_id,p->half,p->intent,false);memset(p,0,sizeof(*p));}
   continue;
  }
  if(p->awaiting){
   if(!wifi)continue; /* mode pause is not a failed transport or new write */
   if(s->result!=DSR_OK||!s->trv_followups){dsr_mobile_trv_request_state(p->key.layout_id,p->half,p->intent,false);memset(p,0,sizeof(*p));}
   continue;
  }
  if(!dsr_automation_manual_ready(p->key.layout_id,p->intent)){
   if(dsr_automation_manual_result(p->key.layout_id,p->intent)==DSR_IO){p->waiting=false;p->failed=true;s->result=DSR_IO;s->command_error=true;dsr_actions_native_release(p->key.layout_id);}
   continue;
  }
  if(!wifi||s->in_flight||s->queued||s->mailbox_pending||s->urgent_stop||dsr_actions_reserved(&s->connection.device))continue;
  dsr_result_t result=dsr_fleet_trv_request(f,(unsigned)at,(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=p->half},wifi,now);
  if(result==DSR_BUSY)continue;
  p->waiting=false;if(result!=DSR_OK){p->failed=true;s->result=result;s->command_error=true;dsr_actions_native_release(p->key.layout_id);}
 }
}
static void native_trv_started(const dsr_job_t *j)
{
 if(j->kind!=DSR_JOB_TRV)return;
 native_trv_t *p=native_trv_find(j->connection.layout_id);
 if(p&&!p->waiting&&!p->failed&&!p->job&&p->half==j->trv.half_c&&native_trv_same(&p->key,&j->connection))p->job=j->id;
}
static void native_trv_completed(dsr_fleet_t *f,uint32_t job,dsr_result_t result)
{
 if(!native_trvs)return;
 for(unsigned i=0;i<DSR_MAX_DEVICES;i++){
  native_trv_t *p=&native_trvs[i];if(!p->retained||p->job!=job)continue;p->job=0;
  dsr_actions_native_release(p->key.layout_id);
  if(result==DSR_OK)for(unsigned j=0;j<f->count;j++)if(native_trv_same(&p->key,&f->slots[j].connection)&&f->slots[j].trv_followups){p->awaiting=true;break;}
  if(p->awaiting)continue;
  if(result==DSR_OK){dsr_mobile_trv_request_state(p->key.layout_id,p->half,p->intent,false);memset(p,0,sizeof(*p));}
  else{p->failed=true;p->waiting=false;}
 }
}
