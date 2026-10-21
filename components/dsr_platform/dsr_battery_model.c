#include "dsr_battery.h"
#include <string.h>

void dsr_battery_profile_defaults(dsr_battery_profile_t *p)
{
 if(p)*p=(dsr_battery_profile_t){.nominal_mv=3700,.capacity_mah=1000,
  .empty_mv=3300,.mid_mv=3800,.full_mv=4200,.gain_permille=1000,.trend_enabled=1};
}
bool dsr_battery_profile_valid(const dsr_battery_profile_t *p)
{
 return p&&p->kind<=DSR_BATTERY_LIION_1S&&
  p->trend_enabled<=1&&!p->reserved[0]&&!p->reserved[1]&&
  (p->nominal_mv==3600||p->nominal_mv==3700)&&
  p->capacity_mah>=1&&p->capacity_mah<=20000&&
  p->empty_mv>=2800&&p->empty_mv<=3600&&
  p->mid_mv>=3500&&p->mid_mv<=4100&&
  p->full_mv>=4000&&p->full_mv<=4250&&
  p->mid_mv>=p->empty_mv+100&&p->full_mv>=p->mid_mv+100&&
  p->gain_permille>=900&&p->gain_permille<=1100;
}
bool dsr_battery_profile_equal(const dsr_battery_profile_t *a,const dsr_battery_profile_t *b)
{return a&&b&&!memcmp(a,b,sizeof(*a));}
bool dsr_battery_parse_number(const char *text,uint16_t *out)
{
 if(!text||!out||!*text)return false;
 uint32_t n=0;unsigned count=0;
 for(const char *c=text;*c;c++){
  if(*c<'0'||*c>'9'||++count>5)return false;
  n=n*10+(unsigned)(*c-'0');if(n>UINT16_MAX)return false;
 }
 *out=(uint16_t)n;return true;
}
void dsr_battery_apply_profile(const dsr_battery_profile_t *p,dsr_battery_sample_t *s)
{
 if(!s)return;
 /* A real gauge, if ever supplied, retains its own trusted capabilities. */
 if(s->has_percent&&!s->percent_estimated)return;
 s->has_percent=false;s->percent_estimated=false;s->percent=0;
 if(!dsr_battery_profile_valid(p)||p->kind==DSR_BATTERY_NONE||!s->has_voltage||
    s->calibration_error!=ESP_OK||!s->adc_mv)return;
 uint32_t mv=((uint32_t)s->adc_mv*2*p->gain_permille+500)/1000;
 if(mv>UINT16_MAX)return;
 s->bat_mv=(uint16_t)mv;
 /* Outside a plausible 1S BAT range, or very noisy ADC, show voltage only.
  * No attached cell may also leave BAT high; this is not presence detection. */
 if(mv<2500||mv>4350||s->spread>120)return;
 unsigned percent;
 if(mv<=p->empty_mv)percent=0;
 else if(mv>=p->full_mv)percent=100;
 else if(mv<=p->mid_mv)percent=((mv-p->empty_mv)*50+(p->mid_mv-p->empty_mv)/2)/(p->mid_mv-p->empty_mv);
 else percent=50+((mv-p->mid_mv)*50+(p->full_mv-p->mid_mv)/2)/(p->full_mv-p->mid_mv);
 s->percent=(uint8_t)percent;s->has_percent=true;s->percent_estimated=true;
}
bool dsr_battery_reference_gain(const dsr_battery_sample_t *s,uint16_t reference,uint16_t *gain)
{
 if(!s||!gain||!s->has_voltage||s->calibration_error!=ESP_OK||s->adc_mv<1250||
    s->adc_mv>2175||s->spread>120||reference<2500||reference>4350)return false;
 uint32_t value=((uint32_t)reference*1000+s->adc_mv)/(2*s->adc_mv);
 if(value<900||value>1100)return false;
 *gain=(uint16_t)value;return true;
}
void dsr_battery_trend_reset(dsr_battery_trend_t *t)
{if(t)memset(t,0,sizeof(*t));}
static void trend_start(dsr_battery_trend_t *t,uint32_t now,uint16_t mv)
{*t=(dsr_battery_trend_t){.started=now,.last_at=now,.last_rise=now,.base_mv=mv,.peak_mv=mv,.tracking=true};}
void dsr_battery_trend_update(dsr_battery_trend_t *t,const dsr_battery_sample_t *s,uint32_t now,bool enabled)
{
 if(!t)return;
 if(!enabled||!s||!s->has_voltage||!s->has_percent||!s->percent_estimated||s->has_charge_state||s->spread>120){dsr_battery_trend_reset(t);return;}
 uint16_t mv=s->bat_mv;
 if(!t->tracking||(uint32_t)(now-t->last_at)>2500||mv+5u<t->peak_mv||
    (uint32_t)(now-t->last_rise)>3000){trend_start(t,now,mv);return;}
 /* The same timestamp cannot count as another independent observation. */
 if(now==t->last_at)return;
 t->last_at=now;
 if(mv>=t->peak_mv+2u){t->peak_mv=mv;t->last_rise=now;if(t->rises<255)t->rises++;}
 t->likely=(uint32_t)(now-t->started)>=10000&&mv>=t->base_mv+20u&&t->rises>=5;
}
bool dsr_battery_trend_likely(const dsr_battery_trend_t *t,uint32_t now)
{return t&&t->tracking&&t->likely&&(uint32_t)(now-t->last_at)<=2500&&(uint32_t)(now-t->last_rise)<=3000;}
