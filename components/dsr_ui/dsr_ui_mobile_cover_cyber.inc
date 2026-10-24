/* Production cover view. History belongs only to this visible presentation;
 * it never changes the confirmed model or authorizes a movement command. */
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static struct {
 lv_obj_t *root,*badges[5],*controls[3],*unknown;
 dsr_ui_icon_t icons[5];
 int last_position;
 bool has_position,historical,controls_set,moving,valid,stop_valid;
} cover_view;
static void cover_view_reset(void){memset(&cover_view,0,sizeof(cover_view));}
static void cover_visual_deleted(lv_event_t *event)
{(void)event;cover_visual=NULL;cover_view_reset();}
static void cover_confirmed_text(const dsr_mobile_device_t *d)
{
 if(!cover_position_label)return;
 char text[64];
 int g=group_display_index(d);
 if(cover_view.historical)snprintf(text,sizeof(text),dsr_sc_tr("Ostatnia pozycja: %u%%"),(unsigned)cover_view.last_position);
 else if(g>=0&&group_state((unsigned)g).known&&group_state((unsigned)g).mixed)snprintf(text,sizeof(text),"%s",dsr_sc_tr("Różne stany"));
 else if(cover_position(d))snprintf(text,sizeof(text),"%s %.0f%%",dsr_sc_tr(d->position_pct==100?"Otwarte":d->position_pct==0?"Zamknięte":"Pozycja"),d->position_pct);
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Nieznana pozycja"));
 if(strcmp(lv_label_get_text(cover_position_label),text))lv_label_set_text(cover_position_label,text);
}
/* Only a changed compact badge is replaced. Static artwork, navigation and
 * touch targets survive ordinary telemetry and pending-command updates. */
static void cover_badge(unsigned index,dsr_ui_icon_t icon,const char *text,int x,int y,int width,uint32_t color)
{
 lv_obj_t *badge=cover_view.badges[index],*label=badge?lv_obj_get_child(badge,1):NULL;
 if(label&&cover_view.icons[index]==icon&&!strcmp(lv_label_get_text(label),text)&&lv_color_eq(lv_obj_get_style_text_color(label,0),dsr_sc_text_color(color)))return;
 if(badge)lv_obj_delete(badge);
 cover_view.badges[index]=dsr_sc_badge(cover_view.root,icon,dsr_sc_user(text),x,y,width,color);cover_view.icons[index]=icon;
}
static void cover_controls(const dsr_mobile_device_t *d)
{
 bool group=group_display_index(d)>=0,hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 bool stop_valid=d->can_stop_cover&&(group?actions.set_group!=NULL:model.demo||actions.stop_cover),moving=cover_moving(d);
 bool valid=d->online&&d->error==DSR_OK&&!d->busy&&d->can_cover&&(group?actions.set_group!=NULL:model.demo||actions.set_cover);
 if(cover_view.controls_set&&cover_view.moving==moving&&cover_view.valid==valid&&cover_view.stop_valid==stop_valid)return;
 for(unsigned i=0;i<3;i++){if(cover_view.controls[i])lv_obj_delete(cover_view.controls[i]);cover_view.controls[i]=NULL;}
 cover_view.controls_set=true;cover_view.moving=moving;cover_view.valid=valid;cover_view.stop_valid=stop_valid;
 lv_obj_t *root=cover_view.root;
 if(moving){cover_view.controls[0]=cover_compact(root,12,364,88,48,0,valid);cover_view.controls[1]=stop_button(root,106,364,108,48,stop_valid,cover_stop,NULL);cover_view.controls[2]=cover_compact(root,220,364,88,48,1,valid);return;}
 lv_obj_t *stop=dsr_sc_button(root,250,232,52,64,DSR_SC_PANEL,9,cover_stop,NULL);cover_view.controls[1]=stop;edge(stop,DSR_SC_GLOW,1);dsr_sc_icon(stop,DSR_ICON_STOP,14,7,24,stop_valid?DSR_SC_GLOW:DSR_SC_OFF);center(dsr_sc_text(stop,"Stop",2,38,46,&dsr_roboto_12,DSR_SC_TEXT));if(!stop_valid)lv_obj_add_state(stop,LV_STATE_DISABLED);
 for(unsigned i=0;i<2;i++){
  lv_obj_t *button=dsr_sc_button(root,12+(int)i*152,364,144,48,hud?DSR_SC_BLUE:DSR_SC_CARD,10,group?group_cover_pair:cover_control,(void *)(uintptr_t)(selected*2+(i==0)));cover_view.controls[i?2:0]=button;
  if(hud)dsr_ui_cyber_panel(button,true);
  dsr_sc_icon(button,i?DSR_ICON_COVER_CLOSE:DSR_ICON_COVER_OPEN,8,7,32,valid?DSR_SC_GLOW:DSR_SC_OFF);
  center(dsr_sc_text(button,i?"Zamknij":"Otwórz",44,13,92,&dsr_roboto_18,DSR_SC_TEXT));if(!valid)lv_obj_add_state(button,LV_STATE_DISABLED);
 }
}
static void cover_values(const dsr_mobile_device_t *d)
{
 bool group=group_display_index(d)>=0,known=cover_position(d),live=d->online&&d->error==DSR_OK;
 uint32_t warning=dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc65b;
 bool moving=known&&!d->cover_errors&&cover_view.has_position&&((d->cover_state==DSR_COVER_OPENING&&d->position_pct>=cover_view.last_position)||(d->cover_state==DSR_COVER_CLOSING&&d->position_pct<=cover_view.last_position));
 /* A timeout retains the picture with an explicit historical caption. No
  * cache survives calibration, invalid data, auth failure, rebind/navigation. */
 if(known){cover_view.last_position=(int)lround(d->position_pct);cover_view.has_position=true;}
 else if(group||d->cover_errors||(d->error!=DSR_OFFLINE&&d->error!=DSR_IO&&d->error!=DSR_RATE_LIMITED))cover_view.has_position=false;
 cover_view.historical=!known&&cover_view.has_position;
 dsr_ui_cyber_cover_update(cover_visual,cover_view.has_position?cover_view.last_position:0,known||cover_view.historical,moving);
 if(known||cover_view.historical)lv_obj_add_flag(cover_view.unknown,LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(cover_view.unknown,LV_OBJ_FLAG_HIDDEN);
 cover_confirmed_text(d);
 lv_color_t caption=dsr_sc_text_color(cover_view.historical?warning:known?DSR_SC_GLOW:DSR_SC_MUTED);
 if(!lv_color_eq(lv_obj_get_style_text_color(cover_position_label,0),caption))lv_obj_set_style_text_color(cover_position_label,caption,0);
 const char *state=d->error!=DSR_OK?device_error(d->error):!d->online?"Brak aktualnego odczytu":!d->can_cover?"Brak obsługi Cover RPC":d->cover_errors?cover_fault(d->cover_errors):d->command_pending?"Wysyłanie polecenia…":d->cover_state==DSR_COVER_OPENING?"Otwieranie":d->cover_state==DSR_COVER_CLOSING?"Zamykanie":d->cover_state==DSR_COVER_CALIBRATING?"Kalibracja":known&&d->position_pct==0?"Zamknięte":known&&d->position_pct==100?"Otwarte":d->cover_state==DSR_COVER_STOPPED?"Zatrzymano":"Stan nieznany";
 if(group&&live&&!d->cover_errors&&group_state(selected).mixed)state="Różne stany";
 cover_badge(0,live?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,dsr_sc_tr(live?"Online":"Offline"),12,102,104,live?DSR_SC_GLOW:warning);
 char state_text[112];if(live&&d->has_frequency&&isfinite(d->frequency_hz))snprintf(state_text,sizeof(state_text),"%s • %.1f Hz",dsr_sc_tr(state),d->frequency_hz);else snprintf(state_text,sizeof(state_text),"%s",dsr_sc_tr(state));
 cover_badge(1,d->cover_state==DSR_COVER_OPENING?DSR_ICON_UP:d->cover_state==DSR_COVER_CLOSING?DSR_ICON_DOWN:DSR_ICON_COVER,state_text,124,102,184,live?DSR_SC_TEXT:warning);
 char text[48];if(known||cover_view.historical)snprintf(text,sizeof(text),"%u%%",(unsigned)cover_view.last_position);else snprintf(text,sizeof(text),"—");
 cover_badge(2,cover_view.historical?DSR_ICON_CLOCK:DSR_ICON_COVER,text,12,136,92,cover_view.historical?warning:DSR_SC_TEXT);
 if(live&&d->has_power&&isfinite(d->power_w))snprintf(text,sizeof(text),"%.1f W",d->power_w);else snprintf(text,sizeof(text),"— W");
 cover_badge(3,DSR_ICON_WATTAGE,text,110,136,96,DSR_SC_TEXT);
 if(live&&d->has_voltage&&isfinite(d->voltage_v))snprintf(text,sizeof(text),"%.1f V",d->voltage_v);else snprintf(text,sizeof(text),"— V");
 cover_badge(4,DSR_ICON_VOLTAGE,text,212,136,96,DSR_SC_TEXT);
 cover_controls(d);
}
static void shared_cover_details(lv_obj_t *root,dsr_mobile_device_t *d)
{
 bool group=group_display_index(d)>=0,known=cover_position(d);
 cover_view.root=root;
 lv_obj_t *panel=dsr_sc_panel(root,12,170,296,186,DSR_SC_CARD,12);edge(panel,DSR_SC_BLUE,1);
 lv_obj_t *rail=dsr_sc_panel(root,64,180,176,142,DSR_SC_BG,5);lv_obj_set_style_bg_opa(rail,LV_OPA_TRANSP,0);
 dsr_sc_border_width(rail,0,0);lv_obj_remove_flag(rail,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 lv_obj_t *well=dsr_sc_panel(rail,0,0,176,142,DSR_SC_BG,5);dsr_sc_border_width(well,0,0);
 int pos=known?(int)lround(d->position_pct):0;
 uint32_t id=group?model.groups[selected].id:d->id;bool same=cover_transition.id==id&&cover_transition.group==group;
 cover_view.has_position=known;cover_view.last_position=same&&cover_transition.animate?cover_transition.from:pos;
 cover_visual=dsr_ui_cyber_cover_transition(well,same?cover_transition.from:pos,pos,known,same&&cover_transition.animate);cover_transition.animate=false;
 lv_obj_add_event_cb(well,cover_visual_deleted,LV_EVENT_DELETE,NULL);
 /* Capability is checked at event time, including after a failed readback. */
 lv_obj_add_flag(rail,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSING,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_RELEASED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESS_LOST,NULL);
 for(unsigned i=0;i<=10;i++){int y=202+(int)i*108/10;dsr_sc_panel(root,i%5?60:56,y,i%5?3:7,1,DSR_SC_MUTED,0);}
 const char *levels[]={"100%","50%","0%"};const int levels_y[]={192,246,300};for(unsigned i=0;i<3;i++){lv_obj_t *level=dsr_sc_text(root,levels[i],16,levels_y[i],36,&dsr_roboto_14,DSR_SC_MUTED);lv_obj_set_style_text_align(level,LV_TEXT_ALIGN_RIGHT,0);}
 cover_view.unknown=dsr_sc_text(well,"—",8,74,160,&dsr_roboto_28,DSR_SC_MUTED);center(cover_view.unknown);
 cover_position_label=dsr_sc_text(root,"",18,327,284,&dsr_roboto_22,known?DSR_SC_GLOW:DSR_SC_MUTED);center(cover_position_label);
 cover_preview_label=dsr_sc_text(root,"",244,190,62,&dsr_roboto_12,0xffc65b);lv_obj_set_style_text_align(cover_preview_label,LV_TEXT_ALIGN_CENTER,0);
 cover_values(d);
}
static bool refresh_cover_details(const dsr_mobile_model_t *m)
{
 if(page!=DSR_MOBILE_DETAILS||selected>=model.device_count||!cover_view.root||!cover_visual||info_open||cover_drag||m->wifi!=model.wifi||m->demo!=model.demo)return false;
 const dsr_mobile_device_t *old=&model.devices[selected],*next=&m->devices[selected];
 if(old->kind!=DSR_DEVICE_COVER||next->kind!=DSR_DEVICE_COVER||strcmp(old->name,next->name))return false;
 /* The caller has already verified stable endpoint identities and structure. */
 model=*m;cover_values(&model.devices[selected]);return true;
}
