/* Groups use confirmed member samples and stable group IDs, never fake device
 * slots. The executor rechecks private identities/capabilities before I/O. */
uint8_t dsr_mobile_control_family(const dsr_mobile_device_t *d)
{
 if(!d||!d->identity[0])return 255;
 if(d->endpoint_kind<=DSR_ENDPOINT_RGB)return d->endpoint_kind;
 return d->endpoint_kind==DSR_ENDPOINT_BLU&&(d->blu.errors&DSR_BLU_TRV)?DSR_ENDPOINT_BLU:255;
}
typedef struct {uint8_t family,value,rgb[3],cover_motion;bool known,mixed,power_known,power_mixed,on,busy;unsigned live,total;} group_state_t;
static group_state_t group_state_of(const dsr_mobile_model_t *view,unsigned index)
{
 group_state_t s={.family=255};if(index>=view->group_count)return s;
 uint16_t mask=view->groups[index].member_mask;bool first=true,unbound=false;
 for(unsigned i=0;i<view->device_count;i++)if(mask&(1u<<i)){
  const dsr_mobile_device_t *d=&view->devices[i];uint8_t family=dsr_mobile_control_family(d);s.total++;s.busy|=d->busy;
  if(family==255){unbound=true;continue;}
  if(s.family==255)s.family=family;else if(s.family!=family)s.family=254;
  bool live=d->online&&d->error==DSR_OK;s.live+=live;
  if(family==DSR_ENDPOINT_COVER)s.cover_motion|=d->cover_state==DSR_COVER_OPENING?1:d->cover_state==DSR_COVER_CLOSING?2:0;
  bool power_known=live&&d->can_switch;
  bool known=live&&(family==DSR_ENDPOINT_SWITCH?d->can_switch:family==DSR_ENDPOINT_COVER?d->has_position:family==DSR_ENDPOINT_LIGHT?(d->light.flags&DSR_LIGHT_BRIGHTNESS)!=0:family==DSR_ENDPOINT_RGB?(d->rgb.flags&DSR_RGB_VALID)!=0:d->has_trv_target);
  double number=family==DSR_ENDPOINT_COVER?d->position_pct:family==DSR_ENDPOINT_LIGHT?d->light.brightness:family==DSR_ENDPOINT_RGB?d->rgb.brightness:family==DSR_ENDPOINT_BLU?d->trv_target_c*2:0;
  bool range=isfinite(number)&&number>=0&&number<=100;if(family==DSR_ENDPOINT_BLU)range=range&&number>=DSR_TRV_MIN_HALF&&number<=DSR_TRV_MAX_HALF;known&=range;uint8_t value=range?(uint8_t)lround(number):0;
  if(first){s.value=value;s.on=d->output;memcpy(s.rgb,&d->rgb.r,3);s.known=known;s.power_known=power_known;first=false;}
  else{s.mixed|=s.value!=value||(family==DSR_ENDPOINT_RGB&&memcmp(s.rgb,&d->rgb.r,3));s.power_mixed|=s.on!=d->output;s.known&=known;s.power_known&=power_known;}
 }
 if(unbound)s.family=255;
 s.known&=!first&&s.total==view->groups[index].members;s.power_known&=!first&&s.total==view->groups[index].members;return s;
}
static group_state_t group_state(unsigned index)
{return group_state_of(&model,index);}
static bool group_ready(group_state_t s)
{return !model.demo&&model.wifi&&actions.set_group&&s.family<=DSR_ENDPOINT_BLU&&s.family!=DSR_ENDPOINT_HT&&s.total&&!s.busy;}
static void group_send(unsigned i,dsr_group_intent_t intent)
{if(i<model.group_count&&group_ready(group_state(i)))actions.set_group(model.groups[i].id,intent);}
static void group_open(lv_event_t *e)
{dsr_mobile_show(DSR_MOBILE_GROUP_CONTROL,(unsigned)(uintptr_t)lv_event_get_user_data(e));}
static void group_power(lv_event_t *e)
{unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(e);group_state_t s=group_state(i);group_send(i,(dsr_group_intent_t){.command=s.power_known&&!s.power_mixed&&s.on?DSR_ACTION_OFF:DSR_ACTION_ON});}
static void group_cover_pair(lv_event_t *e)
{unsigned value=(unsigned)(uintptr_t)lv_event_get_user_data(e);group_send(value/2,(dsr_group_intent_t){.command=value%2?DSR_ACTION_OPEN:DSR_ACTION_CLOSE});}
static void group_stop(lv_event_t *e)
{group_send((unsigned)(uintptr_t)lv_event_get_user_data(e),(dsr_group_intent_t){.command=DSR_ACTION_STOP});}
static void group_target(lv_event_t *e)
{
 unsigned value=(unsigned)(uintptr_t)lv_event_get_user_data(e),i=value/2;group_state_t s=group_state(i);
 if(!s.known||s.mixed){dsr_mobile_show(DSR_MOBILE_GROUP_CONTROL,i);return;}
 int target=(int)s.value+((value&1)?1:-1);if(target>=(int)DSR_TRV_MIN_HALF&&target<=(int)DSR_TRV_MAX_HALF)group_send(i,(dsr_group_intent_t){.command=DSR_ACTION_TARGET,.value={(uint8_t)target}});
}
/* Display-only aggregates have no device ID, host or physical identity. They
 * never enter a connection/worker job; group callbacks address stable group
 * IDs and the actor resolves every real member independently. */
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static dsr_mobile_device_t group_views[8];
#define GROUP_DRAW_CONTEXT 0x100u
static int group_display_index(const dsr_mobile_device_t *d)
{for(unsigned i=0;i<8;i++)if(d==&group_views[i])return (int)i;return -1;}
static const dsr_mobile_device_t *card_draw_device(unsigned context)
{if(context&GROUP_DRAW_CONTEXT){unsigned i=context&~GROUP_DRAW_CONTEXT;return i<model.group_count?&group_views[i]:NULL;}return context<model.device_count?&model.devices[context]:NULL;}
static const char *group_display_state(const dsr_mobile_device_t *d)
{int i=group_display_index(d);if(i<0)return NULL;group_state_t s=group_state((unsigned)i);bool mixed=s.mixed||((s.family==DSR_ENDPOINT_SWITCH||s.family==DSR_ENDPOINT_LIGHT||s.family==DSR_ENDPOINT_RGB)&&s.power_mixed);return s.family==255?"Sprawdź powiązania":s.family==254?"Niezgodny typ":mixed&&s.family!=DSR_ENDPOINT_COVER?"Różne":NULL;}
static dsr_mobile_device_t *group_model(unsigned index)
{
 dsr_mobile_device_t *d=&group_views[index];memset(d,0,sizeof(*d));const dsr_mobile_group_t *g=&model.groups[index];group_state_t s=group_state(index);
 snprintf(d->name,sizeof(d->name),"%s",g->name);d->room=g->room;d->endpoint_kind=s.family<=DSR_ENDPOINT_BLU?s.family:DSR_ENDPOINT_SWITCH;
 d->kind=s.family==DSR_ENDPOINT_COVER?DSR_DEVICE_COVER:s.family==DSR_ENDPOINT_BLU?DSR_DEVICE_SENSOR:s.family==DSR_ENDPOINT_RGB?DSR_DEVICE_RGB:s.family==DSR_ENDPOINT_LIGHT?DSR_DEVICE_LIGHT:DSR_DEVICE_RELAY;
 d->online=model.wifi&&s.live>0;d->error=s.family>=254?DSR_UNSUPPORTED:DSR_OK;d->busy=s.busy;d->output=s.power_known&&!s.power_mixed&&s.on;d->can_switch=group_ready(s);d->can_cover=d->can_stop_cover=d->can_position=group_ready(s);d->can_light=group_ready(s);
 bool all_power=s.total!=0,all_temp=s.total!=0,all_battery=s.total!=0,all_position=s.total!=0,first=true;double temp=0,battery=0;
 for(unsigned i=0;i<model.device_count;i++)if(g->member_mask&(1u<<i)){
  const dsr_mobile_device_t *member=&model.devices[i];bool live=member->online&&member->error==DSR_OK;
  all_power&=live&&member->has_power&&isfinite(member->power_w);if(live&&member->has_power&&isfinite(member->power_w))d->power_w+=member->power_w;
  all_temp&=live&&member->has_temperature&&isfinite(member->temperature_c);all_battery&=live&&member->has_battery&&isfinite(member->battery_pct);all_position&=live&&member->can_position;
  if(first){temp=member->temperature_c;battery=member->battery_pct;if(d->room==DSR_UI_NO_ROOM)d->room=member->room;first=false;}else{all_temp&=fabs(temp-member->temperature_c)<.01;all_battery&=fabs(battery-member->battery_pct)<.01;if(g->room==DSR_UI_NO_ROOM&&d->room!=member->room)d->room=DSR_UI_NO_ROOM;}
  d->can_refresh|=member->can_refresh;d->cover_errors|=member->cover_errors;d->command_pending|=member->command_pending;
 }
 d->has_power=all_power;d->has_temperature=all_temp;d->temperature_c=temp;d->has_battery=all_battery;d->battery_pct=battery;
 if(s.family==DSR_ENDPOINT_COVER){d->has_position=s.known&&!s.mixed;d->position_pct=s.value;d->can_position&=all_position;d->cover_state=s.cover_motion==1?DSR_COVER_OPENING:s.cover_motion==2?DSR_COVER_CLOSING:s.cover_motion?DSR_COVER_UNKNOWN:DSR_COVER_STOPPED;}
 else if(s.family==DSR_ENDPOINT_BLU){d->blu.errors=DSR_BLU_TRV;d->has_trv_target=s.known&&!s.mixed;d->trv_target_c=s.value*.5;}
 else if(s.family==DSR_ENDPOINT_LIGHT)d->light=(dsr_light_status_t){.flags=s.known&&!s.mixed?DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS:0,.brightness=s.value};
 else if(s.family==DSR_ENDPOINT_RGB)d->rgb=(dsr_rgb_status_t){.flags=s.known&&!s.mixed?DSR_RGB_VALID:0,.brightness=s.value,.r=s.rgb[0],.g=s.rgb[1],.b=s.rgb[2]};
 return d;
}
static unsigned group_fields(unsigned index)
{group_state_t s=group_state(index);return s.family==DSR_ENDPOINT_COVER?32:s.family==DSR_ENDPOINT_BLU?8:1;}
static bool group_rgb_draft_stable(const dsr_mobile_model_t *next)
{
 if(!next||selected>=model.group_count||selected>=next->group_count||model.wifi!=next->wifi||model.device_count!=next->device_count)return false;
 const dsr_mobile_group_t *g=&model.groups[selected],*n=&next->groups[selected];if(g->id!=n->id||g->member_mask!=n->member_mask)return false;
 for(unsigned i=0;i<model.device_count;i++)if(g->member_mask&(1u<<i)){
  const dsr_mobile_device_t *a=&model.devices[i],*b=&next->devices[i];
  if(a->id!=b->id||a->generation!=b->generation||a->channel!=b->channel||a->endpoint_kind!=b->endpoint_kind||strcmp(a->identity,b->identity)||strcmp(a->host,b->host)||a->online!=b->online||a->error!=b->error||a->busy!=b->busy||memcmp(&a->rgb,&b->rgb,sizeof(a->rgb)))return false;
 }
 return true;
}

static bool group_shared_selected(void)
{if(page!=DSR_MOBILE_GROUP_CONTROL||selected>=model.group_count)return false;unsigned f=group_state(selected).family;return f==DSR_ENDPOINT_SWITCH||f==DSR_ENDPOINT_COVER||f==DSR_ENDPOINT_BLU;}
static bool group_cover_selected(void)
{return page==DSR_MOBILE_GROUP_CONTROL&&selected<model.group_count&&group_state(selected).family==DSR_ENDPOINT_COVER;}
static dsr_mobile_device_t *detail_device(void)
{return page==DSR_MOBILE_GROUP_CONTROL?(selected<model.group_count?group_model(selected):NULL):(selected<model.device_count?&model.devices[selected]:NULL);}
static const char *group_relay_status(const dsr_mobile_device_t *d)
{
 int g=group_display_index(d);if(g<0)return NULL;group_state_t s=group_state((unsigned)g);
 return s.family>=254?"Sprawdź powiązania":!s.live?"Brak aktualnego odczytu":s.busy?"Wysyłanie polecenia…":!s.power_known?"Brak potwierdzonego odczytu":s.power_mixed?"Różne stany":s.on?"Włączone":"Wyłączone";
}
static bool group_members_stable(const dsr_mobile_model_t *next)
{
 if(page!=DSR_MOBILE_GROUP_CONTROL||selected>=model.group_count||selected>=next->group_count||model.device_count!=next->device_count||model.wifi!=next->wifi)return false;
 const dsr_mobile_group_t *a=&model.groups[selected],*b=&next->groups[selected];if(a->id!=b->id||a->member_mask!=b->member_mask||a->members!=b->members)return false;
 for(unsigned i=0;i<model.device_count;i++)if(a->member_mask&(1u<<i)){
  const dsr_mobile_device_t *x=&model.devices[i],*y=&next->devices[i];
  if(x->id!=y->id||x->generation!=y->generation||x->channel!=y->channel||x->endpoint_kind!=y->endpoint_kind||strcmp(x->identity,y->identity)||strcmp(x->host,y->host))return false;
 }
 return true;
}
static void group_refresh_members(void)
{
 if(!actions.refresh||model.demo||selected>=model.group_count)return;
 /* Copy stable IDs before the first callback: a read may publish a new model. */
 uint32_t ids[DSR_UI_MAX_DEVICES];unsigned count=0;
 for(unsigned i=0;i<model.device_count;i++)if((model.groups[selected].member_mask&(1u<<i))&&model.devices[i].identity[0]&&model.devices[i].can_refresh)ids[count++]=model.devices[i].id;
 for(unsigned i=0;i<count;i++)actions.refresh(ids[i]);
}
