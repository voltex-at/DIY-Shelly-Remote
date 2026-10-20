/* Production cover view. The drawing state never changes confirmed model data. */
static void cover_visual_deleted(lv_event_t *event)
{(void)event;cover_visual=NULL;}
static void cover_confirmed_text(const dsr_mobile_device_t *d)
{
 if(!cover_position_label)return;
 char text[48];
 if(cover_position(d))snprintf(text,sizeof(text),"%s %.0f%%",dsr_sc_tr(d->position_pct==100?"Otwarte":d->position_pct==0?"Zamknięte":"Pozycja"),d->position_pct);
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Nieznana pozycja"));
 lv_label_set_text(cover_position_label,text);
}
static void cyber_cover_details(lv_obj_t *root,dsr_mobile_device_t *d)
{
 bool known=cover_position(d),live=d->online&&d->error==DSR_OK;
 const char *state=d->error!=DSR_OK?device_error(d->error):!d->online?"Brak aktualnego odczytu":d->cover_errors?cover_fault(d->cover_errors):d->command_pending?"Wysyłanie polecenia…":d->cover_state==DSR_COVER_OPENING?"Otwieranie":d->cover_state==DSR_COVER_CLOSING?"Zamykanie":d->cover_state==DSR_COVER_CALIBRATING?"Kalibracja":known&&d->position_pct==0?"Zamknięte":known&&d->position_pct==100?"Otwarte":d->cover_state==DSR_COVER_STOPPED?"Zatrzymano":"Stan nieznany";
 dsr_sc_badge(root,live?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,live?"Online":"Offline",12,94,104,live?DSR_SC_GLOW:0xffc65b);
 dsr_sc_badge(root,d->cover_state==DSR_COVER_OPENING?DSR_ICON_UP:d->cover_state==DSR_COVER_CLOSING?DSR_ICON_DOWN:DSR_ICON_COVER,state,124,94,184,live?DSR_SC_TEXT:0xffc65b);
 char text[48];if(known)snprintf(text,sizeof(text),"%.0f%%",d->position_pct);else snprintf(text,sizeof(text),"—");
 dsr_sc_badge(root,DSR_ICON_COVER,dsr_sc_user(text),12,128,92,DSR_SC_TEXT);
 if(live&&d->has_power&&isfinite(d->power_w))snprintf(text,sizeof(text),"%.1f W",d->power_w);else snprintf(text,sizeof(text),"— W");
 dsr_sc_badge(root,DSR_ICON_WATTAGE,dsr_sc_user(text),110,128,96,DSR_SC_TEXT);
 if(live&&d->has_voltage&&isfinite(d->voltage_v))snprintf(text,sizeof(text),"%.1f V",d->voltage_v);else snprintf(text,sizeof(text),"— V");
 dsr_sc_badge(root,DSR_ICON_VOLTAGE,dsr_sc_user(text),212,128,96,DSR_SC_TEXT);
 lv_obj_t *panel=dsr_sc_panel(root,12,162,296,206,DSR_SC_CARD,12);edge(panel,0x00cfff,1);
 lv_obj_t *rail=dsr_sc_panel(root,64,172,176,162,DSR_SC_BG,5);lv_obj_set_style_bg_opa(rail,LV_OPA_TRANSP,0);
 lv_obj_set_style_border_width(rail,0,0);lv_obj_remove_flag(rail,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 lv_obj_t *well=dsr_sc_panel(rail,0,0,176,162,DSR_SC_BG,5);lv_obj_set_style_border_width(well,0,0);
 int pos=known?(int)lround(d->position_pct):0;
 cover_visual=dsr_ui_cyber_cover_transition(well,cover_transition.id==d->id?cover_transition.from:pos,pos,known,cover_transition.id==d->id&&cover_transition.animate);cover_transition.animate=false;
 lv_obj_add_event_cb(well,cover_visual_deleted,LV_EVENT_DELETE,NULL);
 if(position_enabled(d)&&cover_visual){lv_obj_add_flag(rail,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSING,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_RELEASED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESS_LOST,NULL);}
 for(unsigned i=0;i<=10;i++){int y=194+(int)i*128/10;dsr_sc_panel(root,i%5?49:43,y,i%5?5:11,1,DSR_SC_MUTED,0);}
 dsr_sc_text(root,"100%",19,181,40,&dsr_roboto_12,DSR_SC_MUTED);dsr_sc_text(root,"50%",24,250,36,&dsr_roboto_12,DSR_SC_MUTED);dsr_sc_text(root,"0%",31,314,27,&dsr_roboto_12,DSR_SC_MUTED);
 if(!known)center(dsr_sc_text(well,"—",8,74,160,&dsr_roboto_28,DSR_SC_MUTED));
 bool stop_valid=d->can_stop_cover&&(model.demo||actions.stop_cover);
 lv_obj_t *stop=dsr_sc_button(root,250,232,52,64,DSR_SC_PANEL,9,cover_stop,NULL);edge(stop,DSR_SC_GLOW,1);dsr_sc_icon(stop,DSR_ICON_STOP,14,7,24,stop_valid?DSR_SC_GLOW:DSR_SC_OFF);center(dsr_sc_text(stop,"Stop",2,38,46,&dsr_roboto_12,DSR_SC_TEXT));if(!stop_valid)lv_obj_add_state(stop,LV_STATE_DISABLED);
 cover_position_label=dsr_sc_text(root,"",18,339,284,&dsr_roboto_22,known?DSR_SC_GLOW:DSR_SC_MUTED);center(cover_position_label);cover_confirmed_text(d);
 cover_preview_label=dsr_sc_text(root,"",244,182,62,&dsr_roboto_12,0xffc65b);lv_obj_set_style_text_align(cover_preview_label,LV_TEXT_ALIGN_CENTER,0);
 bool valid=d->online&&d->error==DSR_OK&&!d->busy&&d->can_cover&&(model.demo||actions.set_cover);
 for(unsigned i=0;i<2;i++){
  lv_obj_t *button=dsr_sc_button(root,12+(int)i*152,374,144,48,DSR_SC_BLUE,10,cover_control,(void *)(uintptr_t)(selected*2+(i==0)));
  dsr_ui_cyber_panel(button,true);
  dsr_sc_icon(button,i?DSR_ICON_COVER_CLOSE:DSR_ICON_COVER_OPEN,8,7,32,valid?DSR_SC_GLOW:DSR_SC_OFF);
  center(dsr_sc_text(button,i?"Zamknij":"Otwórz",44,13,92,&dsr_roboto_18,DSR_SC_TEXT));if(!valid)lv_obj_add_state(button,LV_STATE_DISABLED);
 }
}
