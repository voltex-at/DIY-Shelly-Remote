static int group_card_height(unsigned index)
{return device_card_height(group_model(index),false);}
static lv_obj_t *group_card(unsigned index,int y,int height)
{return device_card_shared(content,index,group_model(index),true,0,y,296,height,group_fields(index));}
static struct {uint32_t id,due;uint16_t members;uint8_t family,value;bool initialized,dirty,dragging;lv_obj_t *label;} group_draft;
static lv_timer_t *group_trv_timer;
static bool group_trv_selected(void)
{return page==DSR_MOBILE_GROUP_CONTROL&&selected<model.group_count&&group_state(selected).family==DSR_ENDPOINT_BLU;}
static void group_trv_cancel(void)
{group_draft.initialized=group_draft.dirty=group_draft.dragging=false;}
static bool group_trv_draft_stable(const dsr_mobile_model_t *next)
{
 if(!group_trv_selected()||selected>=next->group_count||model.wifi!=next->wifi||model.device_count!=next->device_count)return false;
 const dsr_mobile_group_t *g=&model.groups[selected],*n=&next->groups[selected];if(g->id!=n->id||g->member_mask!=n->member_mask||g->members!=n->members)return false;
 for(unsigned i=0;i<model.device_count;i++)if(g->member_mask&(1u<<i)){
  const dsr_mobile_device_t *a=&model.devices[i],*b=&next->devices[i];
  if(a->id!=b->id||a->generation!=b->generation||a->channel!=b->channel||a->endpoint_kind!=b->endpoint_kind||strcmp(a->identity,b->identity)||strcmp(a->host,b->host)||a->online!=b->online||a->error!=b->error||a->busy!=b->busy||a->has_trv_target!=b->has_trv_target||a->trv_target_c!=b->trv_target_c||a->blu.errors!=b->blu.errors)return false;
 }
 return true;
}
static void group_trv_send(lv_timer_t *timer)
{
 (void)timer;if(!group_draft.dirty||group_draft.dragging||(int32_t)(lv_tick_get()-group_draft.due)<0)return;
 group_draft.dirty=false;
 if(active&&group_trv_selected()&&group_draft.id==model.groups[selected].id&&group_draft.members==model.groups[selected].member_mask)
  group_send(selected,(dsr_group_intent_t){.command=DSR_ACTION_TARGET,.value={group_draft.value}});
}
static void group_trv_change(uint8_t half)
{
 if(!group_trv_selected()||!group_ready(group_state(selected))||half<DSR_TRV_MIN_HALF||half>DSR_TRV_MAX_HALF)return;
 group_draft.value=half;group_draft.dirty=true;group_draft.due=lv_tick_get()+1200u;
 if(!group_trv_timer)group_trv_timer=lv_timer_create(group_trv_send,100,NULL);
 if(group_draft.label){dsr_sc_label_set_text(group_draft.label,"Wysyłanie do termostatu przez bramkę…");lv_obj_set_style_text_color(group_draft.label,dsr_sc_text_color(0xffbf24),0);}
 trv_knob(half);
}
static void group_trv_step(lv_event_t *e)
{
 unsigned value=(unsigned)(uintptr_t)lv_event_get_user_data(e);if(value/2!=selected)return;
 int next=group_draft.value+((value&1)?1:-1);if(next<(int)DSR_TRV_MIN_HALF)next=DSR_TRV_MIN_HALF;if(next>(int)DSR_TRV_MAX_HALF)next=DSR_TRV_MAX_HALF;
 group_trv_change((uint8_t)next);
}
static void group_trv_arc(lv_event_t *e)
{
 if((unsigned)(uintptr_t)lv_event_get_user_data(e)!=selected||!group_ready(group_state(selected)))return;
 switch(lv_event_get_code(e)){
 case LV_EVENT_PRESSED:group_draft.dragging=true;break;
 case LV_EVENT_VALUE_CHANGED:if(group_draft.dragging)group_trv_change((uint8_t)lv_arc_get_value(lv_event_get_target(e)));break;
 case LV_EVENT_RELEASED:group_draft.dragging=false;group_draft.due=lv_tick_get()+1200u;break;
 case LV_EVENT_PRESS_LOST:if(group_draft.dragging){group_trv_cancel();render();}break;
 default:break;
 }
}
/* Same dial as one TRV, with each member's own valve and battery below it.
 * Larger groups scroll this compact strip instead of clipping extra members. */
static void group_trv_members(lv_obj_t *root)
{
 lv_obj_t *strip=dsr_sc_panel(root,8,336,304,64,DSR_SC_BG,0);lv_obj_set_style_bg_opa(strip,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(strip,0,0);
 lv_obj_add_flag(strip,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(strip,LV_DIR_HOR);lv_obj_set_scrollbar_mode(strip,LV_SCROLLBAR_MODE_OFF);
 unsigned at=0;for(unsigned i=0;i<model.device_count;i++)if(model.groups[selected].member_mask&(1u<<i)){
  const dsr_mobile_device_t *d=&model.devices[i];bool live=d->online&&d->error==DSR_OK;
  bool valve=live&&trv_known(d)&&d->blu.trv_pos<=100,battery=live&&d->has_battery&&isfinite(d->battery_pct)&&d->battery_pct>=0&&d->battery_pct<=100;
  lv_obj_t *card=dsr_sc_panel(strip,(int)at++*156,0,148,64,DSR_SC_CARD,10);if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_frame(card,true);
  lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(d->name),8,5,128,&dsr_roboto_11,DSR_SC_MUTED);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
  dsr_sc_icon(card,DSR_ICON_TRV_VALVE,12,28,24,valve?DSR_SC_GLOW:DSR_SC_OFF);dsr_sc_icon(card,DSR_ICON_BATTERY,77,28,24,battery?DSR_SC_GLOW:DSR_SC_OFF);
  char text[16];if(valve)snprintf(text,sizeof(text),"%u%%",d->blu.trv_pos);else snprintf(text,sizeof(text),"—");dsr_sc_text(card,text,40,32,34,&dsr_roboto_14,DSR_SC_TEXT);
  if(battery)snprintf(text,sizeof(text),"%.0f%%",d->battery_pct);else snprintf(text,sizeof(text),"—");dsr_sc_text(card,text,103,32,38,&dsr_roboto_14,battery&&d->battery_pct<=20?0xffbf24:DSR_SC_TEXT);
 }
}
static void group_trv_note(lv_obj_t *root)
{
 group_state_t s=group_state(selected);const char *note=group_draft.dirty||s.busy?"Wysyłanie do termostatu przez bramkę…":s.mixed?"Różne stany":"Każde urządzenie ma własny wynik.";
 lv_obj_t *l=group_draft.label=dsr_sc_text(root,note,12,403,296,&dsr_roboto_12,group_draft.dirty||s.busy?0xffbf24:DSR_SC_MUTED);center(l);truncate(l,20);
}
static void group_details(lv_obj_t *root)
{
 group_draft.label=NULL;if(selected>=model.group_count)return;
 dsr_mobile_group_t *g=&model.groups[selected];group_state_t s=group_state(selected);
 if(s.family==DSR_ENDPOINT_SWITCH){relay_details(root,group_model(selected));return;}
 if(s.family==DSR_ENDPOINT_COVER){shared_cover_details(root,group_model(selected));return;}
 if(s.family==DSR_ENDPOINT_LIGHT||s.family==DSR_ENDPOINT_RGB){if(rgb_color_view)rgb_details(root);else lighting_light_details(root);return;}
 if(s.family==DSR_ENDPOINT_BLU){
  if(!group_draft.initialized||group_draft.id!=g->id||group_draft.members!=g->member_mask||group_draft.family!=s.family){
   group_draft.id=g->id;group_draft.members=g->member_mask;group_draft.family=s.family;group_draft.value=s.known&&!s.mixed?s.value:42;group_draft.initialized=true;
  }
  trv_details_view(root,group_model(selected),true);return;
 }
 lv_obj_t *name=dsr_sc_text(root,dsr_sc_name(g->name),12,54,296,&dsr_roboto_18,DSR_SC_TEXT);truncate(name,42);
 dsr_sc_text(root,s.family==254?"Niezgodne sterowanie":"Sprawdź powiązania urządzeń",12,160,296,&dsr_roboto_18,dsr_sc_style()==DSR_SC_STYLE_LIGHT?0x956000:0xffc107);
}
