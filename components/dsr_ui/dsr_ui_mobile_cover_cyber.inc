/* Production cover view. The drawing state never changes confirmed model data. */
static void cover_visual_deleted(lv_event_t *event)
{(void)event;cover_visual=NULL;}
static void cover_confirmed_text(const dsr_mobile_device_t *d)
{
 if(!cover_position_label)return;
 char text[48];
 int g=group_display_index(d);
 if(g>=0&&group_state((unsigned)g).known&&group_state((unsigned)g).mixed)snprintf(text,sizeof(text),"%s",dsr_sc_tr("Różne stany"));
 else if(cover_position(d))snprintf(text,sizeof(text),"%s %.0f%%",dsr_sc_tr(d->position_pct==100?"Otwarte":d->position_pct==0?"Zamknięte":"Pozycja"),d->position_pct);
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Nieznana pozycja"));
 lv_label_set_text(cover_position_label,text);
}
static void shared_cover_details(lv_obj_t *root,dsr_mobile_device_t *d)
{
 bool group=group_display_index(d)>=0,hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE,known=cover_position(d),live=d->online&&d->error==DSR_OK;
 const char *state=d->error!=DSR_OK?device_error(d->error):!d->online?"Brak aktualnego odczytu":!d->can_cover?"Brak obsługi Cover RPC":d->cover_errors?cover_fault(d->cover_errors):d->command_pending?"Wysyłanie polecenia…":d->cover_state==DSR_COVER_OPENING?"Otwieranie":d->cover_state==DSR_COVER_CLOSING?"Zamykanie":d->cover_state==DSR_COVER_CALIBRATING?"Kalibracja":known&&d->position_pct==0?"Zamknięte":known&&d->position_pct==100?"Otwarte":d->cover_state==DSR_COVER_STOPPED?"Zatrzymano":"Stan nieznany";
 if(group&&live&&!d->cover_errors&&group_state(selected).mixed)state="Różne stany";
 dsr_sc_badge(root,live?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,live?"Online":"Offline",12,102,104,live?DSR_SC_GLOW:0xffc65b);
 char state_text[112];if(live&&d->has_frequency&&isfinite(d->frequency_hz))snprintf(state_text,sizeof(state_text),"%s • %.1f Hz",dsr_sc_tr(state),d->frequency_hz);else snprintf(state_text,sizeof(state_text),"%s",dsr_sc_tr(state));
 dsr_sc_badge(root,d->cover_state==DSR_COVER_OPENING?DSR_ICON_UP:d->cover_state==DSR_COVER_CLOSING?DSR_ICON_DOWN:DSR_ICON_COVER,dsr_sc_user(state_text),124,102,184,live?DSR_SC_TEXT:0xffc65b);
 char text[48];if(known)snprintf(text,sizeof(text),"%.0f%%",d->position_pct);else snprintf(text,sizeof(text),"—");
 dsr_sc_badge(root,DSR_ICON_COVER,dsr_sc_user(text),12,136,92,DSR_SC_TEXT);
 if(live&&d->has_power&&isfinite(d->power_w))snprintf(text,sizeof(text),"%.1f W",d->power_w);else snprintf(text,sizeof(text),"— W");
 dsr_sc_badge(root,DSR_ICON_WATTAGE,dsr_sc_user(text),110,136,96,DSR_SC_TEXT);
 if(live&&d->has_voltage&&isfinite(d->voltage_v))snprintf(text,sizeof(text),"%.1f V",d->voltage_v);else snprintf(text,sizeof(text),"— V");
 dsr_sc_badge(root,DSR_ICON_VOLTAGE,dsr_sc_user(text),212,136,96,DSR_SC_TEXT);
 lv_obj_t *panel=dsr_sc_panel(root,12,170,296,186,DSR_SC_CARD,12);edge(panel,DSR_SC_BLUE,1);
 lv_obj_t *rail=dsr_sc_panel(root,64,180,176,142,DSR_SC_BG,5);lv_obj_set_style_bg_opa(rail,LV_OPA_TRANSP,0);
 dsr_sc_border_width(rail,0,0);lv_obj_remove_flag(rail,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 lv_obj_t *well=dsr_sc_panel(rail,0,0,176,142,DSR_SC_BG,5);dsr_sc_border_width(well,0,0);
 int pos=known?(int)lround(d->position_pct):0;
 uint32_t id=group?model.groups[selected].id:d->id;bool same=cover_transition.id==id&&cover_transition.group==group;
 cover_visual=dsr_ui_cyber_cover_transition(well,same?cover_transition.from:pos,pos,known,same&&cover_transition.animate);cover_transition.animate=false;
 lv_obj_add_event_cb(well,cover_visual_deleted,LV_EVENT_DELETE,NULL);
 if(position_enabled(d)&&cover_visual){lv_obj_add_flag(rail,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSING,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_RELEASED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESS_LOST,NULL);}
 for(unsigned i=0;i<=10;i++){int y=202+(int)i*108/10;dsr_sc_panel(root,i%5?60:56,y,i%5?3:7,1,DSR_SC_MUTED,0);}
 const char *levels[]={"100%","50%","0%"};const int levels_y[]={192,246,300};for(unsigned i=0;i<3;i++){lv_obj_t *level=dsr_sc_text(root,levels[i],16,levels_y[i],36,&dsr_roboto_14,DSR_SC_MUTED);lv_obj_set_style_text_align(level,LV_TEXT_ALIGN_RIGHT,0);}
 if(!known)center(dsr_sc_text(well,"—",8,74,160,&dsr_roboto_28,DSR_SC_MUTED));
 bool stop_valid=d->can_stop_cover&&(group?actions.set_group!=NULL:model.demo||actions.stop_cover),moving=cover_moving(d);
 if(!moving){lv_obj_t *stop=dsr_sc_button(root,250,232,52,64,DSR_SC_PANEL,9,cover_stop,NULL);edge(stop,DSR_SC_GLOW,1);dsr_sc_icon(stop,DSR_ICON_STOP,14,7,24,stop_valid?DSR_SC_GLOW:DSR_SC_OFF);center(dsr_sc_text(stop,"Stop",2,38,46,&dsr_roboto_12,DSR_SC_TEXT));if(!stop_valid)lv_obj_add_state(stop,LV_STATE_DISABLED);}
 cover_position_label=dsr_sc_text(root,"",18,327,284,&dsr_roboto_22,known?DSR_SC_GLOW:DSR_SC_MUTED);center(cover_position_label);cover_confirmed_text(d);
 cover_preview_label=dsr_sc_text(root,"",244,190,62,&dsr_roboto_12,0xffc65b);lv_obj_set_style_text_align(cover_preview_label,LV_TEXT_ALIGN_CENTER,0);
 bool valid=d->online&&d->error==DSR_OK&&!d->busy&&d->can_cover&&(group?actions.set_group!=NULL:model.demo||actions.set_cover);
 if(moving){cover_compact(root,12,364,88,48,0,valid);stop_button(root,106,364,108,48,stop_valid,cover_stop,NULL);cover_compact(root,220,364,88,48,1,valid);return;}
 for(unsigned i=0;i<2;i++){
  lv_obj_t *button=dsr_sc_button(root,12+(int)i*152,364,144,48,hud?DSR_SC_BLUE:DSR_SC_CARD,10,group?group_cover_pair:cover_control,(void *)(uintptr_t)(selected*2+(i==0)));
  if(hud)dsr_ui_cyber_panel(button,true);
  dsr_sc_icon(button,i?DSR_ICON_COVER_CLOSE:DSR_ICON_COVER_OPEN,8,7,32,valid?DSR_SC_GLOW:DSR_SC_OFF);
  center(dsr_sc_text(button,i?"Zamknij":"Otwórz",44,13,92,&dsr_roboto_18,DSR_SC_TEXT));if(!valid)lv_obj_add_state(button,LV_STATE_DISABLED);
 }
}
