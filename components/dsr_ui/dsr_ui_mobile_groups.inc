static lv_obj_t *group_button(lv_obj_t *p,const char *text,int x,int y,int w,int h,lv_event_cb_t cb,uintptr_t data,bool enabled)
{
 lv_obj_t *b=dsr_sc_button(p,x,y,w,h,DSR_SC_PANEL,10,cb,(void *)data);lv_obj_remove_flag(b,LV_OBJ_FLAG_PRESS_LOCK);
 lv_obj_t *label=dsr_sc_text(b,text,0,0,w-8,!strcmp(text,"+")||!strcmp(text,"-")?&dsr_roboto_22:&dsr_roboto_14,enabled?DSR_SC_TEXT:DSR_SC_OFF);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(label);
 if(!enabled)lv_obj_add_state(b,LV_STATE_DISABLED);
 return b;
}
static int group_card_height(unsigned index)
{return device_card_height(group_model(index),false);}
static lv_obj_t *group_card(unsigned index,int y,int height)
{return device_card_shared(content,index,group_model(index),true,0,y,296,height,group_fields(index));}
static struct {uint32_t id;uint16_t members;uint8_t family,value;bool initialized;lv_obj_t *label;} group_draft;
static void group_draft_label(void)
{
 if(!group_draft.label)return;
 char text[64];
 if(group_draft.family==DSR_ENDPOINT_BLU)snprintf(text,sizeof(text),"%.1f °C",group_draft.value*.5);
 else snprintf(text,sizeof(text),"%u%%",group_draft.value);
 lv_label_set_text(group_draft.label,text);
}
static void group_adjust(lv_event_t *e)
{
 int value=(int)group_draft.value+((uintptr_t)lv_event_get_user_data(e)?1:-1);int min=group_draft.family==DSR_ENDPOINT_BLU?DSR_TRV_MIN_HALF:0,max=group_draft.family==DSR_ENDPOINT_BLU?DSR_TRV_MAX_HALF:100;
 if(value>=min&&value<=max){group_draft.value=(uint8_t)value;group_draft_label();}
}
static void group_slider(lv_event_t *e)
{group_draft.value=(uint8_t)lv_slider_get_value(lv_event_get_target(e));group_draft_label();}
static void group_apply(lv_event_t *e)
{(void)e;group_send(selected,(dsr_group_intent_t){.command=group_draft.family==DSR_ENDPOINT_BLU?DSR_ACTION_TARGET:DSR_ACTION_POSITION,.value={group_draft.value}});}
static void group_details(lv_obj_t *root)
{
 group_draft.label=NULL;if(selected>=model.group_count)return;
 dsr_mobile_group_t *g=&model.groups[selected];group_state_t s=group_state(selected);bool valid=group_ready(s);
 if(s.family==DSR_ENDPOINT_LIGHT||s.family==DSR_ENDPOINT_RGB){if(rgb_color_view)rgb_details(root);else lighting_light_details(root);return;}
 if(!group_draft.initialized||group_draft.id!=g->id||group_draft.members!=g->member_mask||group_draft.family!=s.family){
  group_draft.id=g->id;group_draft.members=g->member_mask;group_draft.family=s.family;group_draft.value=s.known&&!s.mixed?s.value:s.family==DSR_ENDPOINT_BLU?42:50;group_draft.initialized=true;
 }
 lv_obj_t *name=dsr_sc_text(root,dsr_sc_name(g->name),12,54,296,&dsr_roboto_18,DSR_SC_TEXT);truncate(name,42);
 char text[96];snprintf(text,sizeof(text),dsr_sc_tr("Grupa • %u urządzeń"),g->members);dsr_sc_text(root,text,12,103,296,&dsr_roboto_14,DSR_SC_MUTED);
 if(s.family>=254){dsr_sc_text(root,s.family==254?"Niezgodne sterowanie":"Sprawdź powiązania urządzeń",12,160,296,&dsr_roboto_18,dsr_sc_style()==DSR_SC_STYLE_LIGHT?0x956000:0xffc107);return;}
 lv_obj_t *panel=dsr_sc_panel(root,12,136,296,214,DSR_SC_CARD,12);if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_frame(panel,true);
 if(s.family==DSR_ENDPOINT_SWITCH){group_button(panel,"Włącz grupę",16,72,264,48,group_power,selected,valid);if(s.power_known&&!s.power_mixed&&s.on)dsr_sc_label_set_text(lv_obj_get_child(lv_obj_get_child(panel,-1),0),"Wyłącz grupę");}
 else{
  center(dsr_sc_text(panel,s.family==DSR_ENDPOINT_BLU?"Wspólna temperatura":"Wspólna wartość",12,10,272,&dsr_roboto_14,DSR_SC_MUTED));
  group_draft.label=dsr_sc_text(panel,"",76,42,144,&dsr_roboto_28,DSR_SC_TEXT);center(group_draft.label);group_draft_label();
  group_button(panel,"-",16,40,48,48,group_adjust,0,valid);group_button(panel,"+",232,40,48,48,group_adjust,1,valid);
  if(s.family!=DSR_ENDPOINT_BLU){lv_obj_t *slider=lv_slider_create(panel);lv_obj_set_pos(slider,28,110);lv_obj_set_size(slider,240,16);lv_obj_set_ext_click_area(slider,14);lv_slider_set_range(slider,0,100);lv_slider_set_value(slider,group_draft.value,LV_ANIM_OFF);lv_obj_add_event_cb(slider,group_slider,LV_EVENT_VALUE_CHANGED,(void *)3);if(!valid)lv_obj_add_state(slider,LV_STATE_DISABLED);}
  center(dsr_sc_text(panel,s.mixed?"Różne stany":"Każde urządzenie ma własny wynik.",12,160,272,&dsr_roboto_14,DSR_SC_MUTED));
 }
 if(s.family!=DSR_ENDPOINT_SWITCH){lv_obj_t *apply=group_button(root,s.family==DSR_ENDPOINT_BLU?"Ustaw temperaturę":"Ustaw pozycję",12,DSR_SC_NAV_TOP-64,296,48,group_apply,0,valid);if(valid)dsr_ui_primary(apply);}
}
