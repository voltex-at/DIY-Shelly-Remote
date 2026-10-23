/* A single drawing callback replaces dozens of decorative arc objects. */
static void trv_ring_draw(lv_event_t *event)
{
 lv_area_t a;lv_obj_get_coords(lv_event_get_target(event),&a);lv_layer_t *layer=lv_event_get_layer(event);
 bool live=(uintptr_t)lv_event_get_user_data(event)!=0;int cx=a.x1+154,cy=a.y1+124;
 const uint32_t stops[]={0x007bff,0x00cfff,0x00d5ff,0xffbd36,0xff3c35};
 lv_draw_arc_dsc_t arc;lv_draw_arc_dsc_init(&arc);arc.center=(lv_point_t){cx,cy};arc.radius=99;arc.width=11;
 for(unsigned i=0;i<42;i++){
  double at=i*4.0/41;unsigned k=(unsigned)at;if(k>3)k=3;double f=at-k;uint32_t c=0;
  for(int bit=16;bit>=0;bit-=8){int v=(int)((stops[k]>>bit)&255),w=(int)((stops[k+1]>>bit)&255);c=(c<<8)|(uint32_t)(v+(w-v)*f);}
  arc.color=dsr_sc_color(live?c:DSR_SC_OFF);arc.start_angle=135+270.0*i/42;arc.end_angle=135+270.0*(i+1)/42-1.2;lv_draw_arc(layer,&arc);
 }
 arc.radius=108;arc.width=1;arc.color=dsr_sc_color(live?0x2785ac:DSR_SC_OFF);arc.start_angle=135;arc.end_angle=405;lv_draw_arc(layer,&arc);
 for(unsigned i=0;i<19;i++){
  double angle=(135+15.0*i)*3.14159265358979/180;
  lv_draw_line_dsc_t l;lv_draw_line_dsc_init(&l);l.color=dsr_sc_color(DSR_SC_MUTED);l.width=1;
  l.p1=(lv_point_precise_t){cx+110*cos(angle),cy+110*sin(angle)};l.p2=(lv_point_precise_t){cx+(i%3?114:117)*cos(angle),cy+(i%3?114:117)*sin(angle)};lv_draw_line(layer,&l);
 }
}
static void trv_percent_card(lv_obj_t *root,int x,const char *title,dsr_ui_icon_t icon,bool known,unsigned value,bool warning)
{
 lv_obj_t *card=dsr_sc_panel(root,x,336,148,64,DSR_SC_CARD,10);edge(card,0x00cfff,1);
 dsr_sc_icon(card,icon,10,10,32,known?DSR_SC_GLOW:DSR_SC_OFF);
 dsr_sc_text(card,title,48,5,96,&dsr_roboto_16,DSR_SC_TEXT);
 char text[16];if(known)snprintf(text,sizeof(text),"%u%%",value);else snprintf(text,sizeof(text),"—");
 dsr_sc_text(card,text,48,25,96,&dsr_roboto_18,known&&warning?0xffbf24:DSR_SC_TEXT);
 lv_obj_t *track=dsr_sc_panel(card,8,48,132,10,DSR_SC_PANEL,2);edge(track,DSR_SC_EDGE,1);
 if(known&&value){int width=130*(int)(value>100?100:value)/100;if(width<1)width=1;dsr_sc_panel(track,0,0,width,8,warning?0xffbf24:DSR_SC_GLOW,1);}
}
static void cyber_trv_details(lv_obj_t *root,const dsr_mobile_device_t *d,bool group)
{
 bool live=d->online&&d->error==DSR_OK;
 bool known=group||trv_known(d),can=group?group_ready(group_state(selected)):trv_can(d);uint8_t half=group?group_draft.value:trv_shown(d);char text[48];
 lv_obj_t *panel=dsr_sc_panel(root,8,94,304,236,DSR_SC_CARD,10);edge(panel,0x00cfff,1);lv_obj_add_event_cb(panel,trv_ring_draw,LV_EVENT_DRAW_MAIN_END,(void *)(uintptr_t)live);
 dsr_sc_icon(root,live?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,22,104,24,live?DSR_SC_GLOW:0xffc65b);
 dsr_sc_text(root,live?"Online":"Offline",52,108,88,&dsr_roboto_12,live?DSR_SC_GLOW:0xffc65b);
 trv_dial.arc=lv_arc_create(root);lv_obj_remove_style_all(trv_dial.arc);lv_obj_set_pos(trv_dial.arc,54,110);lv_obj_set_size(trv_dial.arc,216,216);
 lv_arc_set_range(trv_dial.arc,DSR_TRV_MIN_HALF,DSR_TRV_MAX_HALF);lv_arc_set_bg_angles(trv_dial.arc,135,45);lv_arc_set_value(trv_dial.arc,half);lv_arc_set_change_rate(trv_dial.arc,720);
 lv_obj_set_style_arc_width(trv_dial.arc,32,LV_PART_MAIN);lv_obj_set_style_arc_width(trv_dial.arc,32,LV_PART_INDICATOR);lv_obj_set_style_arc_opa(trv_dial.arc,LV_OPA_TRANSP,LV_PART_MAIN);lv_obj_set_style_arc_opa(trv_dial.arc,LV_OPA_TRANSP,LV_PART_INDICATOR);
 lv_obj_add_flag(trv_dial.arc,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_remove_flag(trv_dial.arc,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_event_cb(trv_dial.arc,trv_arc_event,LV_EVENT_ALL,(void *)(uintptr_t)selected);if(!can)lv_obj_add_state(trv_dial.arc,LV_STATE_DISABLED);
 trv_dial.knob=dsr_sc_panel(root,0,0,26,26,DSR_SC_GLOW,LV_RADIUS_CIRCLE);edge(trv_dial.knob,DSR_SC_TEXT,2);lv_obj_set_style_outline_width(trv_dial.knob,1,0);lv_obj_set_style_outline_pad(trv_dial.knob,3,0);lv_obj_set_style_outline_color(trv_dial.knob,dsr_sc_color(DSR_SC_GLOW),0);
 if(!known)lv_obj_add_flag(trv_dial.knob,LV_OBJ_FLAG_HIDDEN);
 dsr_sc_icon(root,DSR_ICON_THERMOSTAT,150,133,24,live?DSR_SC_GLOW:DSR_SC_OFF);
 if(live&&d->has_temperature&&isfinite(d->temperature_c))snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"— °C");
 center(dsr_sc_text(root,text,102,162,120,&dsr_roboto_22,DSR_SC_TEXT));
 center(dsr_sc_text(root,"Temperatura zmierzona",72,191,180,&dsr_roboto_12,DSR_SC_MUTED));
 dsr_sc_panel(root,104,210,116,1,DSR_SC_EDGE,0);
 trv_dial.label=dsr_sc_text(root,"",82,217,160,&dsr_roboto_28,DSR_SC_TEXT);center(trv_dial.label);
 trv_dial.caption=dsr_sc_text(root,"Nastawa temperatury",62,252,200,&dsr_roboto_12,DSR_SC_MUTED);center(trv_dial.caption);trv_knob(half);if(!known)lv_label_set_text(trv_dial.label,"— °C");
 trv_round_action(root,selected,98,276,48,false,can,group?group_trv_step:trv_step);trv_round_action(root,selected,174,276,48,true,can,group?group_trv_step:trv_step);
 half_text(text,sizeof(text),DSR_TRV_MIN_HALF);dsr_sc_text(root,dsr_sc_user(text),32,292,56,&dsr_roboto_12,DSR_SC_MUTED);dsr_sc_text(root,"Min.",42,305,44,&dsr_roboto_11,DSR_SC_MUTED);
 half_text(text,sizeof(text),DSR_TRV_MAX_HALF);dsr_sc_text(root,dsr_sc_user(text),252,292,60,&dsr_roboto_12,DSR_SC_MUTED);dsr_sc_text(root,"Max.",264,305,44,&dsr_roboto_11,DSR_SC_MUTED);
 if(group){group_trv_members(root);group_trv_note(root);return;}
 trv_percent_card(root,8,"Zawór",DSR_ICON_TRV_VALVE,known&&d->blu.trv_pos<=100,d->blu.trv_pos,false);
 trv_percent_card(root,164,"Bateria",DSR_ICON_BATTERY,live&&d->has_battery&&isfinite(d->battery_pct)&&d->battery_pct>=0&&d->battery_pct<=100,(unsigned)(live&&d->has_battery&&isfinite(d->battery_pct)&&d->battery_pct>=0&&d->battery_pct<=100?d->battery_pct:0),d->battery_pct<=20);
 const char *warning=!live?"Bramka nie odpowiada":(d->blu.errors&DSR_BLU_TRV_FAULT)?"Termostat zgłasza błąd (np. brak kalibracji)":(trv_slot(d->id)>=0||d->command_pending)?"Wysyłanie do termostatu przez bramkę…":NULL;
 if(warning){lv_obj_t *l=dsr_sc_text(root,warning,12,403,296,&dsr_roboto_12,0xffbf24);center(l);truncate(l,20);}
 else{dsr_sc_icon(root,DSR_ICON_CLOCK,18,402,22,DSR_SC_MUTED);sample_age=dsr_sc_text(root,"",47,404,260,&dsr_roboto_12,DSR_SC_MUTED);update_sample_age();}
}
