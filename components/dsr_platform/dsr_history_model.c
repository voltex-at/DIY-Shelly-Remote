#include "dsr_history_private.h"
#include <math.h>
#include <string.h>

static bool key_valid(const dsr_history_key_t *k)
{return k&&memchr(k->device_id,0,sizeof(k->device_id))&&k->device_id[0]&&k->layout_id<=1000000&&k->kind==0&&(k->generation==DSR_GEN1||k->generation==DSR_GEN2_PLUS)&&k->channel<=DSR_MAX_CHANNEL;}
bool dsr_history_key_equal(const dsr_history_key_t *a,const dsr_history_key_t *b)
{return key_valid(a)&&key_valid(b)&&a->kind==b->kind&&a->generation==b->generation&&a->channel==b->channel&&!strcmp(a->device_id,b->device_id);}
int dsr_history_model_binding(dsr_history_model_t *m,const dsr_history_key_t *key,bool active)
{
 if(!m||!key_valid(key))return -1;
 int chosen=-1;
 for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++){
  if(m->slots[i].used&&dsr_history_key_equal(&m->slots[i].key,key)){chosen=(int)i;break;}
  if(!m->slots[i].used)chosen=(int)i;
 }
 if(chosen<0)for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++)if(!m->slots[i].active&&(chosen<0||m->slots[i].last_utc<m->slots[chosen].last_utc))chosen=(int)i;
 if(chosen<0)return -1;
 dsr_history_slot_t *s=&m->slots[chosen];
 if(!s->used||!dsr_history_key_equal(&s->key,key)){
  memset(s,0,sizeof(*s));memset(m->points[chosen],0,sizeof(m->points[chosen]));memset(m->dirty[chosen],0,sizeof(m->dirty[chosen]));s->used=true;
 }
 s->key=*key;s->active=active;return chosen;
}
static unsigned point_index(uint32_t utc){return (utc/DSR_HISTORY_STEP)%DSR_HISTORY_POINTS;}
static void dirty(dsr_history_model_t *m,unsigned index)
{
 dsr_history_slot_t *s=&m->slots[index];if(!s->current_utc||s->sealed)return;
 unsigned at=point_index(s->current_utc);s->sealed=true;
 /* A response with all fields absent is a time gap, not a serializable
  * measurement. Never journal mask0: the strict reader rejects such frames. */
 if(!m->points[index][at].mask){m->dirty[index][at]=0;return;}
 m->dirty[index][at]=++s->version;if(!s->version)m->dirty[index][at]=++s->version;
}
void dsr_history_model_seal(dsr_history_model_t *m,uint32_t utc)
{if(utc<1735689600)return;for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++)if(m->slots[i].used&&m->slots[i].current_utc&&utc/DSR_HISTORY_STEP>m->slots[i].current_utc/DSR_HISTORY_STEP)dirty(m,i);}
void dsr_history_model_record(dsr_history_model_t *m,const dsr_history_key_t *key,
 const dsr_device_t *d,const dsr_telemetry_t *t,uint32_t request,uint32_t utc,uint64_t mono)
{
 if(!m||!d||!t||!d->online||!key||d->endpoint_kind!=key->kind||!key->layout_id)return;
 int index=dsr_history_model_binding(m,key,true);if(index<0)return;
 dsr_history_slot_t *s=&m->slots[index];
 if(s->has_request&&s->last_request==request)return;
 s->last_request=request;s->has_request=true;
 if(utc<1735689600||utc>UINT32_MAX-86400u||(s->last_utc&&utc<s->last_utc)){m->without_time++;s->has_energy=false;return;}
 uint32_t bucket=utc-utc%DSR_HISTORY_STEP;
 if(bucket!=s->current_utc){
  dirty(m,(unsigned)index);s->current_utc=bucket;s->sealed=false;memset(s->sum,0,sizeof(s->sum));memset(s->counts,0,sizeof(s->counts));
  memset(&m->points[index][point_index(bucket)],0,sizeof(dsr_history_point_t));
 }
 if(s->sealed)return;
 dsr_history_point_t *p=&m->points[index][point_index(bucket)];p->utc=bucket;
 if(s->last_mono&&(mono<s->last_mono||mono-s->last_mono>15000u))p->flags|=DSR_HISTORY_GAP;
 const bool has[]={d->has_power,t->has_voltage,t->has_current};const double value[]={d->power_w,t->voltage_v,t->current_a};float *out[]={&p->power,&p->voltage,&p->current};
 for(unsigned i=0;i<3;i++)if(has[i]&&isfinite(value[i])&&fabs(value[i])<=1e9&&(!i||value[i]>=0)){
  if(s->counts[i]<UINT16_MAX){s->sum[i]+=value[i];s->counts[i]++;}
  *out[i]=(float)(s->sum[i]/s->counts[i]);p->count[i]=s->counts[i];p->mask|=(uint8_t)(1u<<i);
 }
 if(t->has_energy&&isfinite(t->energy_wh)&&t->energy_wh>=0&&t->energy_wh<=1e12){
  if(s->has_energy&&t->energy_wh<s->last_energy)p->flags|=DSR_HISTORY_RESET;
  if(s->last_utc&&!s->has_energy)p->flags|=DSR_HISTORY_GAP;
  p->energy_wh=t->energy_wh;p->mask|=DSR_HISTORY_WH;s->last_energy=t->energy_wh;s->has_energy=true;
 }else {if(s->has_energy)p->flags|=DSR_HISTORY_GAP;s->has_energy=false;}
 s->last_utc=utc;s->last_mono=mono;
}
void dsr_history_model_query(const dsr_history_model_t *m,unsigned index,unsigned hours,uint32_t now,dsr_history_series_t *out)
{
 if(!m||!out||index>=DSR_HISTORY_SLOTS||(hours!=1&&hours!=24)||now<1735689600u||now>UINT32_MAX-86400u)return;
 out->count=0;out->key=m->slots[index].key;out->to_utc=now;out->from_utc=now-hours*3600u;
 uint32_t first=out->from_utc-out->from_utc%DSR_HISTORY_STEP,last=now-now%DSR_HISTORY_STEP;
 for(uint32_t t=first;t<=last&&out->count<DSR_HISTORY_POINTS;t+=DSR_HISTORY_STEP){
  const dsr_history_point_t *p=&m->points[index][point_index(t)];if(p->utc==t&&p->mask)out->points[out->count++]=*p;
 }
}
uint32_t dsr_history_crc(const void *data,size_t length)
{const unsigned char *p=data;uint32_t crc=UINT32_MAX;while(length--){crc^=*p++;for(unsigned i=0;i<8;i++)crc=(crc>>1)^(0xedb88320u&-(crc&1u));}return ~crc;}
static void put32(unsigned char *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(x>>(8*i));}
static uint32_t get32(const unsigned char *p){uint32_t x=0;for(unsigned i=0;i<4;i++)x|=(uint32_t)p[i]<<(8*i);return x;}
static void floating(unsigned char *p,const void *value,unsigned bytes)
{uint64_t x=0;if(bytes==4){uint32_t v;memcpy(&v,value,4);x=v;}else memcpy(&x,value,8);for(unsigned i=0;i<bytes;i++)p[i]=(unsigned char)(x>>(8*i));}
static void unfloat(void *value,const unsigned char *p,unsigned bytes)
{uint64_t x=0;for(unsigned i=0;i<bytes;i++)x|=(uint64_t)p[i]<<(8*i);if(bytes==4){uint32_t v=(uint32_t)x;memcpy(value,&v,4);}else memcpy(value,&x,8);}
static size_t frame(unsigned char *out,unsigned kind,unsigned slot,size_t length)
{out[0]=(unsigned char)kind;out[1]=(unsigned char)slot;out[2]=(unsigned char)length;out[3]=0;put32(out+4,0);put32(out+4,dsr_history_crc(out,length));return length;}
size_t dsr_history_encode_key(unsigned char *out,unsigned slot,const dsr_history_key_t *key)
{memset(out,0,80);put32(out+8,key->layout_id);out[12]=key->generation;out[13]=key->channel;out[14]=key->kind;memcpy(out+16,key->device_id,64);return frame(out,1,slot,80);}
size_t dsr_history_encode_point(unsigned char *out,unsigned slot,const dsr_history_point_t *p)
{
 if(!out||!p||!p->mask||(p->mask&~15u))return 0;
 memset(out,0,40);put32(out+8,p->utc);floating(out+12,&p->power,4);floating(out+16,&p->voltage,4);floating(out+20,&p->current,4);floating(out+24,&p->energy_wh,8);
 for(unsigned i=0;i<3;i++){out[32+i*2]=(unsigned char)p->count[i];out[33+i*2]=(unsigned char)(p->count[i]>>8);}out[38]=p->mask;out[39]=p->flags;return frame(out,2,slot,40);
}
bool dsr_history_decode(dsr_history_model_t *m,const unsigned char *data,size_t length)
{
 if(!m||!data||(length!=80&&length!=40)||data[1]>=DSR_HISTORY_SLOTS||data[2]!=length||data[3])return false;
 unsigned char copy[80];memcpy(copy,data,length);uint32_t crc=get32(copy+4);put32(copy+4,0);if(crc!=dsr_history_crc(copy,length))return false;
 unsigned index=data[1];dsr_history_slot_t *s=&m->slots[index];
 if(length==80&&data[0]==1){
  dsr_history_key_t key={.layout_id=get32(data+8),.generation=data[12],.channel=data[13],.kind=data[14]};memcpy(key.device_id,data+16,64);
  if(!key_valid(&key)||data[15])return false;
  if(!s->used||!dsr_history_key_equal(&s->key,&key)){memset(s,0,sizeof(*s));memset(m->points[index],0,sizeof(m->points[index]));}s->used=true;s->key=key;return true;
 }
 if(length!=40||data[0]!=2||!s->used)return false;
 dsr_history_point_t p={.utc=get32(data+8),.mask=data[38],.flags=data[39]};
 unfloat(&p.power,data+12,4);unfloat(&p.voltage,data+16,4);unfloat(&p.current,data+20,4);unfloat(&p.energy_wh,data+24,8);
 for(unsigned i=0;i<3;i++)p.count[i]=(uint16_t)(data[32+i*2]|(unsigned)data[33+i*2]<<8);
 if(p.utc<1735689600||p.utc%DSR_HISTORY_STEP||p.utc>UINT32_MAX-86400u||(p.mask&~15u)||(p.flags&~3u)||!p.mask||
  !isfinite(p.power)||!isfinite(p.voltage)||!isfinite(p.current)||!isfinite(p.energy_wh)||p.energy_wh<0||p.energy_wh>1e12||p.voltage<0||p.current<0||fabs(p.power)>1e9||p.voltage>1e9||p.current>1e9)return false;
 for(unsigned i=0;i<3;i++)if(((p.mask>>i)&1u)!=(p.count[i]!=0))return false;
 unsigned at=point_index(p.utc);if(m->points[index][at].utc>p.utc)return true;
 m->points[index][at]=p;if(s->last_utc<p.utc)s->last_utc=p.utc;return true;
}
size_t dsr_history_checkpoint(const dsr_history_model_t *m,unsigned char *out,size_t capacity,uint32_t generation)
{
 if(!out||capacity<DSR_HISTORY_SNAPSHOT_MAX||!generation)return 0;
 size_t used=32;memset(out,0,32);
 for(unsigned i=0;i<DSR_HISTORY_SLOTS;i++)if(m->slots[i].used){
  used+=dsr_history_encode_key(out+used,i,&m->slots[i].key);
  /* Chronological order is required when a ring index is reused. */
  uint32_t last=m->slots[i].last_utc/DSR_HISTORY_STEP*DSR_HISTORY_STEP,first=last>=86400u?last-86400u:0;
  for(uint32_t t=first;t<=last;t+=DSR_HISTORY_STEP){const dsr_history_point_t *p=&m->points[i][point_index(t)];if(p->utc==t&&p->mask)used+=dsr_history_encode_point(out+used,i,p);}
 }
 put32(out,0x48525344);put32(out+4,1);put32(out+8,generation);put32(out+12,(uint32_t)used);put32(out+16,dsr_history_crc(out+32,used-32));put32(out+20,dsr_history_crc(out,20));return used;
}
