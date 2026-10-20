/* Presentation uses the existing confirmed data and release/apply callbacks. */
static void cyber_light_step(lv_event_t *event)
{
 if(selected>=model.device_count||!light_enabled(&model.devices[selected]))return;
 int value=brightness(&model.devices[selected])+((uintptr_t)lv_event_get_user_data(event)?5:-5);
 if(value<0)value=0;
 if(value>100)value=100;
 actions.set_brightness(selected,(uint8_t)value);
}
static void cyber_light_status(lv_obj_t *root,const dsr_mobile_device_t *d)
{
 bool live=d->online&&d->error==DSR_OK;
 const char *state=unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):!live?"Brak aktualnego odczytu":d->busy?"Wysyłanie polecenia…":d->output?"Włączone":"Wyłączone";
 dsr_sc_badge(root,live?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,live?"Online":"Offline",12,94,104,live?DSR_SC_GLOW:0xffc65b);
 dsr_sc_badge(root,DSR_ICON_POWER,state,124,94,184,live?DSR_SC_TEXT:0xffc65b);
}
static void cyber_light_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];
 bool readable=d->online&&d->error==DSR_OK;
 bool known=readable&&(d->endpoint_kind==DSR_ENDPOINT_RGB?(d->rgb.flags&DSR_RGB_VALID):(d->light.flags&DSR_LIGHT_BRIGHTNESS));
 char text[80];
 const bool present[]={d->has_power,d->has_voltage,d->has_current};
 const double values[]={d->power_w,d->voltage_v,d->current_a};
 const char *units[]={"W","V","A"};
 const dsr_ui_icon_t icons[]={DSR_ICON_WATTAGE,DSR_ICON_VOLTAGE,DSR_ICON_CURRENT};
 cyber_light_status(root,d);
 for(unsigned i=0;i<3;i++){
  if(readable&&present[i]&&isfinite(values[i]))snprintf(text,sizeof(text),i==2?"%.2f %s":"%.1f %s",values[i],units[i]);else snprintf(text,sizeof(text),"— %s",units[i]);
  dsr_sc_badge(root,icons[i],dsr_sc_user(text),12+(int)i*100,128,96,DSR_SC_TEXT);
 }
 lv_obj_t *panel=dsr_sc_panel(root,12,164,296,183,DSR_SC_CARD,12);
 const uint32_t colors[]={0x004b93,0x007bff,0x00e5ff};
 dsr_sc_gradient_arc(panel,148,80,58,9,135,270,colors,3,18,known);
 dsr_sc_icon(panel,DSR_ICON_LIGHT,132,28,32,known&&d->output?0xffc65b:DSR_SC_OFF);
 if(known)snprintf(text,sizeof(text),"%u%%",brightness(d));else snprintf(text,sizeof(text),"—");
 center(dsr_sc_text(panel,text,72,66,152,&dsr_roboto_28,DSR_SC_TEXT));
 center(dsr_sc_text(panel,"Jasność",72,101,152,&dsr_roboto_14,DSR_SC_MUTED));
 for(unsigned i=0;i<2;i++){
  lv_obj_t *b=dsr_sc_button(panel,i?238:12,60,44,44,DSR_SC_PANEL,LV_RADIUS_CIRCLE,cyber_light_step,(void *)(uintptr_t)i);
  center(dsr_sc_text(b,i?"+":"-",0,8,42,&dsr_roboto_22,DSR_SC_TEXT));if(!light_enabled(d))lv_obj_add_state(b,LV_STATE_DISABLED);
 }
 light_preview_label=dsr_sc_text(panel,"",8,132,280,&dsr_roboto_12,DSR_SC_GLOW);center(light_preview_label);
 light_slider=lv_slider_create(panel);lv_obj_set_pos(light_slider,26,156);lv_obj_set_size(light_slider,244,16);
 lv_slider_set_range(light_slider,0,100);lv_slider_set_value(light_slider,known?brightness(d):0,LV_ANIM_OFF);lv_obj_set_ext_click_area(light_slider,14);
 lv_obj_remove_flag(light_slider,LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_PRESSED,NULL);lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_VALUE_CHANGED,NULL);lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_RELEASED,NULL);lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_PRESS_LOST,NULL);
 if(!light_enabled(d))lv_obj_add_state(light_slider,LV_STATE_DISABLED);
 lv_obj_t *controls=dsr_sc_panel(root,12,355,296,54,DSR_SC_CARD,10);power_button(controls,selected,12,2,48);
 if(d->endpoint_kind==DSR_ENDPOINT_RGB){lv_obj_t *color=dsr_sc_button(controls,68,2,130,48,DSR_SC_PANEL,10,rgb_view,NULL);dsr_sc_icon(color,DSR_ICON_PALETTE,8,12,24,DSR_SC_GLOW);center(dsr_sc_text(color,"Kolor RGB",38,15,88,&dsr_roboto_14,DSR_SC_TEXT));}
 else center(dsr_sc_text(controls,d->output?"Wyłącz":"Włącz",68,22,132,&dsr_roboto_16,DSR_SC_TEXT));
 lv_obj_t *read=dsr_sc_button(controls,206,2,82,48,DSR_SC_PANEL,10,refresh,NULL);dsr_sc_icon(read,DSR_ICON_REFRESH,5,13,22,DSR_SC_GLOW);center(dsr_sc_text(read,"Odśwież stan",29,8,49,&dsr_roboto_12,DSR_SC_TEXT));if(d->busy||!d->can_refresh||!actions.refresh)lv_obj_add_state(read,LV_STATE_DISABLED);
 sample_age=dsr_sc_text(root,"",12,412,296,&dsr_roboto_12,DSR_SC_MUTED);center(sample_age);update_sample_age();
}
static void cyber_rgb_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];bool readable=d->online&&d->error==DSR_OK&&(d->rgb.flags&DSR_RGB_VALID);
 cyber_light_status(root,d);
 lv_obj_t *panel=dsr_sc_panel(root,12,128,296,219,DSR_SC_CARD,12);
 dsr_sc_text(panel,"Kolor RGB",12,10,140,&dsr_roboto_16,DSR_SC_TEXT);
 const uint32_t spectrum[]={0xff354f,0xffcc3b,0x00ee63,0x00dfff,0x2174ff,0xe333ff,0xff354f};
 dsr_sc_gradient_arc(panel,148,55,38,10,0,360,spectrum,7,24,readable);
 rgb_swatch=dsr_sc_panel(panel,126,33,44,44,0,LV_RADIUS_CIRCLE);lv_obj_set_style_bg_grad_dir(rgb_swatch,LV_GRAD_DIR_NONE,0);
 const uint32_t colors[]={0xff3c64,0x21d884,0x08bfff};
 for(unsigned i=0;i<3;i++){
  rgb_values[i]=dsr_sc_text(panel,"",12,104+(int)i*40,66,&dsr_roboto_14,DSR_SC_TEXT);
  lv_obj_t *slider=lv_slider_create(panel);lv_obj_set_pos(slider,88,108+(int)i*40);lv_obj_set_size(slider,178,16);lv_slider_set_range(slider,0,255);lv_slider_set_value(slider,rgb_draft[i],LV_ANIM_OFF);lv_obj_set_ext_click_area(slider,14);
  lv_obj_set_style_bg_color(slider,lv_color_hex(colors[i]),LV_PART_INDICATOR);lv_obj_set_style_bg_color(slider,lv_color_hex(colors[i]),LV_PART_KNOB);
  lv_obj_add_event_cb(slider,rgb_color_event,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)i);lv_obj_add_event_cb(slider,rgb_color_event,LV_EVENT_PRESS_LOST,(void *)(uintptr_t)i);
  if(!rgb_enabled(d))lv_obj_add_state(slider,LV_STATE_DISABLED);
 }
 center(dsr_sc_text(root,readable?((d->rgb.flags&DSR_RGB_TRANSITION)?"Przejście • poczekaj na odczyt":"Podgląd • Zastosuj wysyła kolor"):"Brak aktualnego odczytu",12,412,296,&dsr_roboto_12,DSR_SC_MUTED));
 lv_obj_t *controls=dsr_sc_panel(root,12,355,296,54,DSR_SC_CARD,10);power_button(controls,selected,12,2,48);
 lv_obj_t *level=dsr_sc_button(controls,68,2,130,48,DSR_SC_PANEL,10,rgb_view,NULL);center(dsr_sc_text(level,"Jasność",4,15,122,&dsr_roboto_14,DSR_SC_TEXT));
 rgb_apply_button=dsr_sc_button(controls,206,2,82,48,DSR_SC_BLUE,10,rgb_apply,NULL);center(dsr_sc_text(rgb_apply_button,"Zastosuj",4,15,74,&dsr_roboto_12,DSR_SC_TEXT));rgb_preview();
}
