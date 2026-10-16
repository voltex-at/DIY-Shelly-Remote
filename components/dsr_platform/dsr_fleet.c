#include "dsr_fleet.h"
#include "dsr_activity.h"
#include <string.h>

bool dsr_endpoint_equal(const dsr_device_t *a,const dsr_device_t *b)
{return a&&b&&a->device_id[0]&&b->device_id[0]&&!strcmp(a->device_id,b->device_id)&&a->generation==b->generation&&a->endpoint_kind==b->endpoint_kind&&a->channel==b->channel;}
int dsr_fleet_find(const dsr_fleet_t *f,const dsr_device_t *d)
{for(size_t i=0;i<f->count;i++)if(dsr_endpoint_equal(&f->slots[i].connection.device,d))return (int)i;return -1;}
static void discard(dsr_fleet_slot_t *s)
{if(s->queued){s->queued=false;memset(&s->intent_identity,0,sizeof(s->intent_identity));dsr_activity_end();}}
dsr_result_t dsr_fleet_upsert(dsr_fleet_t *f,const dsr_connection_t *c,uint32_t now)
{
 if(!f||!dsr_connection_valid(c,true))return DSR_INVALID;
 int at=dsr_fleet_find(f,&c->device);if(at<0){if(f->count>=DSR_MAX_DEVICES)return DSR_FULL;at=(int)f->count++;}
 for(size_t i=0;i<f->count;i++)if((int)i!=at&&f->slots[i].connection.device.generation==c->device.generation&&!strcmp(f->slots[i].connection.device.device_id,c->device.device_id)){
  dsr_fleet_slot_t *other=&f->slots[i];
  if(strcmp(other->connection.device.host,c->device.host)){discard(other);other->connection.device.online=false;other->next_poll=now;}
  memcpy(other->connection.device.host,c->device.host,sizeof(c->device.host));other->connection.credentials=c->credentials;
 }
 dsr_fleet_slot_t *s=&f->slots[at];uint32_t command_id=s->command_id;discard(s);memset(s,0,sizeof(*s));s->command_id=command_id;s->connection=*c;s->layout_enabled=c->layout_id!=0||c->legacy_fallback;
 s->sample_valid=c->device.online;s->sample_ms=now;s->next_poll=now+5000;return DSR_OK;
}
dsr_result_t dsr_fleet_request(dsr_fleet_t *f,unsigned index,bool on,bool wifi,uint32_t now)
{
 if(!f||index>=f->count)return DSR_INVALID;
 dsr_fleet_slot_t *s=&f->slots[index];
 if(s->connection.device.endpoint_kind!=DSR_ENDPOINT_SWITCH)return DSR_UNSUPPORTED;
 if(s->queued||(s->in_flight&&s->operation==DSR_JOB_SET_OUTPUT))return DSR_BUSY;
 if(!s->layout_enabled)return DSR_IDENTITY_MISMATCH;
 if(!wifi||!s->connection.device.online||!s->sample_valid||now-s->sample_ms>15000)return DSR_OFFLINE;
 if(s->blocked)return s->result;
 if(!s->connection.device.has_switch)return DSR_UNSUPPORTED;
 if(!dsr_activity_begin())return DSR_BUSY;
 s->queued=true;s->on=on;s->intent_identity=s->connection.device;s->intent_order=++f->order;
 s->result=DSR_OK;s->command_error=false;return DSR_OK;
}
dsr_result_t dsr_fleet_cover_request(dsr_fleet_t *f,unsigned index,dsr_cover_intent_t intent,bool wifi,uint32_t now)
{
 if(!f||index>=f->count||!dsr_cover_intent_valid(intent)||intent.command==DSR_COVER_STOP)return DSR_INVALID;
 dsr_fleet_slot_t *s=&f->slots[index];
 if(s->connection.device.endpoint_kind!=DSR_ENDPOINT_COVER)return DSR_UNSUPPORTED;
 if(s->queued||s->urgent_stop||(s->in_flight&&s->operation==DSR_JOB_COVER))return DSR_BUSY;
 if(!s->layout_enabled)return DSR_IDENTITY_MISMATCH;
 if(!wifi||!s->connection.device.online||!s->sample_valid||now-s->sample_ms>15000)return DSR_OFFLINE;
 if(s->blocked)return s->result;
 if(!dsr_cover_can_move(&s->connection.telemetry.cover,intent))return DSR_UNSUPPORTED;
 if(!dsr_activity_begin())return DSR_BUSY;
 s->queued=true;s->cover=intent;s->intent_identity=s->connection.device;s->intent_order=++f->order;s->result=DSR_OK;s->command_error=false;return DSR_OK;
}
dsr_result_t dsr_fleet_light_request(dsr_fleet_t *f,unsigned index,dsr_light_intent_t intent,bool wifi,uint32_t now)
{
 if(!f||index>=f->count||!dsr_light_intent_valid(intent))return DSR_INVALID;
 dsr_fleet_slot_t *s=&f->slots[index];
 if(s->connection.device.endpoint_kind!=DSR_ENDPOINT_LIGHT)return DSR_UNSUPPORTED;
 if(s->queued||(s->in_flight&&s->operation==DSR_JOB_LIGHT))return DSR_BUSY;
 if(!s->layout_enabled)return DSR_IDENTITY_MISMATCH;
 if(!wifi||!s->connection.device.online||!s->sample_valid||now-s->sample_ms>15000)return DSR_OFFLINE;
 if(s->blocked)return s->result;
 if(!dsr_light_can_set(&s->connection.telemetry.light,intent))return DSR_UNSUPPORTED;
 if(!dsr_activity_begin())return DSR_BUSY;
 s->queued=true;s->light=intent;s->intent_identity=s->connection.device;s->intent_order=++f->order;s->result=DSR_OK;s->command_error=false;return DSR_OK;
}
dsr_result_t dsr_fleet_rgb_request(dsr_fleet_t *f,unsigned index,dsr_rgb_intent_t intent,bool wifi,uint32_t now)
{
 if(!f||index>=f->count||!dsr_rgb_intent_valid(intent))return DSR_INVALID;
 dsr_fleet_slot_t *s=&f->slots[index];
 if(s->connection.device.endpoint_kind!=DSR_ENDPOINT_RGB)return DSR_UNSUPPORTED;
 if(s->queued||(s->in_flight&&s->operation==DSR_JOB_RGB))return DSR_BUSY;
 if(!s->layout_enabled)return DSR_IDENTITY_MISMATCH;
 if(!wifi||!s->connection.device.online||!s->sample_valid||now-s->sample_ms>15000)return DSR_OFFLINE;
 if(s->blocked)return s->result;
 if(!dsr_rgb_can_set(&s->connection.telemetry.rgb,intent))return DSR_UNSUPPORTED;
 if(!dsr_activity_begin())return DSR_BUSY;
 s->queued=true;s->rgb=intent;s->intent_identity=s->connection.device;s->intent_order=++f->order;s->result=DSR_OK;s->command_error=false;return DSR_OK;
}
void dsr_fleet_cancel_queued(dsr_fleet_t *f,unsigned index)
{if(f&&index<f->count)discard(&f->slots[index]);}
bool dsr_fleet_next(dsr_fleet_t *f,uint32_t now,bool allow_poll,dsr_job_t *job,unsigned *index)
{
 if(!f||!job||!index)return false;
 int at=-1;
 for(size_t i=0;i<f->count;i++){
  dsr_fleet_slot_t *s=&f->slots[i];if(!s->queued||s->in_flight)continue;
  if(!s->layout_enabled||now-s->sample_ms>15000||!dsr_endpoint_equal(&s->connection.device,&s->intent_identity)||strcmp(s->connection.device.host,s->intent_identity.host)||!s->connection.device.online||s->blocked){
   s->result=s->blocked?s->result:(!s->connection.device.online||now-s->sample_ms>15000)?DSR_OFFLINE:DSR_IDENTITY_MISMATCH;s->command_error=true;discard(s);continue;
  }
  if(at<0||(int32_t)(s->intent_order-f->slots[at].intent_order)<0)at=(int)i;
 }
 if(at<0&&allow_poll)for(size_t n=0;n<f->count;n++){
  unsigned i=(f->cursor+(unsigned)n)%(unsigned)f->count;dsr_fleet_slot_t *s=&f->slots[i];
  if(s->layout_enabled&&!s->urgent_stop&&!s->in_flight&&!s->blocked&&(int32_t)(now-s->next_poll)>=0){at=(int)i;break;}
 }
 if(at<0)return false;
 dsr_fleet_slot_t *s=&f->slots[at];*job=(dsr_job_t){.kind=s->queued?(s->connection.device.endpoint_kind==DSR_ENDPOINT_COVER?DSR_JOB_COVER:s->connection.device.endpoint_kind==DSR_ENDPOINT_LIGHT?DSR_JOB_LIGHT:s->connection.device.endpoint_kind==DSR_ENDPOINT_RGB?DSR_JOB_RGB:DSR_JOB_SET_OUTPUT):DSR_JOB_PROBE,.connection=s->connection};
 if(s->queued){if(s->connection.device.endpoint_kind==DSR_ENDPOINT_COVER)job->cover=s->cover;else if(s->connection.device.endpoint_kind==DSR_ENDPOINT_LIGHT)job->light=s->light;else if(s->connection.device.endpoint_kind==DSR_ENDPOINT_RGB)job->rgb=s->rgb;else job->on=s->on;}
 *index=(unsigned)at;return true;
}
void dsr_fleet_started(dsr_fleet_t *f,unsigned index,const dsr_job_t *job)
{
 dsr_fleet_slot_t *s=&f->slots[index];s->in_flight=true;s->operation=job->kind;s->request_id=job->id;
 if(job->kind==DSR_JOB_SET_OUTPUT||job->kind==DSR_JOB_COVER||job->kind==DSR_JOB_LIGHT||job->kind==DSR_JOB_RGB)discard(s);
 f->cursor=(index+1)%(unsigned)f->count;
}
bool dsr_fleet_complete(dsr_fleet_t *f,unsigned index,const dsr_event_t *event,uint32_t now)
{
 if(!f||index>=f->count)return false;
 dsr_fleet_slot_t *s=&f->slots[index];
 if(!s->in_flight||s->request_id!=event->id)return false;
 s->in_flight=false;dsr_result_t result=event->result;
 if(!dsr_endpoint_equal(&s->connection.device,&event->device)||strcmp(s->connection.device.host,event->device.host))result=DSR_IDENTITY_MISMATCH;
 if(result==DSR_OK){s->connection.device=event->device;s->connection.telemetry=event->telemetry;s->failures=0;if(event->device.online){s->sample_valid=true;s->sample_ms=now;}}
 else {s->connection.device.online=false;s->connection.device.has_power=false;memset(&s->connection.telemetry,0,sizeof(s->connection.telemetry));if(s->failures<4)s->failures++;}
 if(s->operation==DSR_JOB_SET_OUTPUT||s->operation==DSR_JOB_COVER||s->operation==DSR_JOB_LIGHT||s->operation==DSR_JOB_RGB){s->result=result;s->command_error=result!=DSR_OK;}else if(!s->command_error)s->result=result;
 s->blocked=result==DSR_AUTH_REQUIRED||result==DSR_IDENTITY_MISMATCH||result==DSR_RATE_LIMITED;
 s->next_poll=now+(s->failures?(5000u<<s->failures):5000u);
 if(result!=DSR_OK&&s->queued){s->result=result;s->command_error=true;discard(s);}
 return true;
}
void dsr_fleet_wifi(dsr_fleet_t *f,bool online,uint32_t now)
{for(size_t i=0;i<f->count;i++){dsr_fleet_slot_t *s=&f->slots[i];s->connection.device.online=false;if(!online&&s->queued){discard(s);s->result=DSR_OFFLINE;s->command_error=true;}if(online)s->next_poll=now;}}
void dsr_fleet_refresh(dsr_fleet_t *f,uint32_t now)
{for(size_t i=0;i<f->count;i++){dsr_fleet_slot_t *s=&f->slots[i];s->blocked=false;s->command_error=false;s->result=DSR_OK;s->failures=0;s->next_poll=now;}}
