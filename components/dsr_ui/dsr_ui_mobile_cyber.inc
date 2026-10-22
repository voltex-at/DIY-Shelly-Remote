/* Included by dsr_ui_mobile.c. No transport, sample data or extra timers. */
static void cyber_text(lv_obj_t *parent,const char *text,int x,int y,int width,int height,const lv_font_t *font,uint32_t color)
{lv_obj_t *t=dsr_sc_text(parent,text,x,y,width,font,color);lv_obj_set_height(t,height);lv_label_set_long_mode(t,LV_LABEL_LONG_MODE_DOTS);}
static void cyber_relay_details(lv_obj_t *root,const dsr_mobile_device_t *d)
{
 bool valid=d->online&&d->error==DSR_OK&&!unbound(d)&&d->has_sample_time;
 bool online=d->online&&d->error==DSR_OK&&!unbound(d);
 dsr_sc_badge(root,online?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,online?"Online":"Offline",8,94,106,online?DSR_SC_GLOW:0xffc65b);
 const char *status=unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):!d->online?"Brak aktualnego odczytu":!valid?"Brak potwierdzonego odczytu":d->busy?(d->command_pending?"Wysyłanie polecenia…":"Odczyt stanu…"):d->output?"Włączone":"Wyłączone";
 dsr_sc_badge(root,valid?DSR_ICON_POWER:DSR_ICON_WARNING,status,122,94,190,valid?DSR_SC_TEXT:0xffc65b);
 lv_obj_t *panel=dsr_sc_panel(root,8,130,304,206,DSR_SC_CARD,12);edge(panel,0x00cfff,1);
 lv_obj_t *power=dsr_sc_panel(panel,8,8,288,70,DSR_SC_PANEL,8);edge(power,0x00cfff,1);
 dsr_sc_icon(power,DSR_ICON_WATTAGE,12,9,24,DSR_SC_GLOW);
 dsr_sc_text(power,"Moc chwilowa",44,8,230,&dsr_roboto_14,DSR_SC_MUTED);
 char text[40];if(valid&&d->has_power&&isfinite(d->power_w))snprintf(text,sizeof(text),"%.1f W",d->power_w);else snprintf(text,sizeof(text),"— W");
 center(dsr_sc_text(power,text,8,31,270,&dsr_roboto_28,DSR_SC_TEXT));
 bool enabled=valid&&!d->busy&&d->can_switch&&(model.demo||actions.set_output);
 dsr_sc_power_button(root,117,220,86,d->output,enabled,DSR_ICON_POWER,DSR_SC_BLUE,control,(void *)(uintptr_t)selected);
 lv_obj_t *update=dsr_sc_button(root,217,233,84,64,DSR_SC_PANEL,10,refresh,NULL);edge(update,0x00cfff,1);
 dsr_sc_icon(update,DSR_ICON_REFRESH,28,4,28,DSR_SC_GLOW);center(dsr_sc_text(update,"Odśwież stan",2,33,78,&dsr_roboto_12,DSR_SC_TEXT));
 if(!d->can_refresh||d->busy||!actions.refresh)lv_obj_add_state(update,LV_STATE_DISABLED);
 if(valid&&!d->busy)center(dsr_sc_text(root,d->output?"Wyłącz":"Włącz",90,310,140,&dsr_roboto_14,DSR_SC_GLOW));
 lv_obj_t *age=dsr_sc_panel(root,8,342,304,28,DSR_SC_PANEL,7);dsr_sc_icon(age,DSR_ICON_CLOCK,7,5,18,DSR_SC_MUTED);
 sample_age=dsr_sc_text(age,"",33,7,262,&dsr_roboto_12,DSR_SC_MUTED);update_sample_age();
 const char *labels[]={"Napięcie","Natężenie"};const dsr_ui_icon_t icons[]={DSR_ICON_VOLTAGE,DSR_ICON_CURRENT};
 for(unsigned i=0;i<2;i++){
  lv_obj_t *card=dsr_sc_panel(root,i?164:8,378,148,48,DSR_SC_CARD,9);edge(card,0x00cfff,1);
  dsr_sc_icon(card,icons[i],10,4,18,DSR_SC_GLOW);dsr_sc_text(card,labels[i],34,5,108,&dsr_roboto_12,DSR_SC_MUTED);
  if(valid&&(i?d->has_current:d->has_voltage)&&isfinite(i?d->current_a:d->voltage_v))snprintf(text,sizeof(text),i?"%.3f A":"%.1f V",i?d->current_a:d->voltage_v);else snprintf(text,sizeof(text),i?"— A":"— V");
  center(dsr_sc_text(card,text,4,20,138,&dsr_roboto_18,DSR_SC_TEXT));
 }
}
static void cyber_overview_row(lv_obj_t *panel,dsr_ui_icon_t icon,const char *label,const char *value,int y,int height,bool wrap)
{
 dsr_sc_icon(panel,icon,14,y+2,22,DSR_SC_GLOW);
 cyber_text(panel,label,44,y+4,112,18,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *v=dsr_sc_text(panel,dsr_sc_user(value),158,y+2,130,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_height(v,height-2);lv_label_set_long_mode(v,wrap?LV_LABEL_LONG_MODE_WRAP:LV_LABEL_LONG_MODE_DOTS);
 dsr_sc_panel(panel,14,y+height,272,1,DSR_SC_EDGE,0);
}
static void cyber_action_caption(lv_obj_t *button,const char *text)
{
 /* Medium DE captions need two lines in the fixed 148 px action module. */
 lv_obj_t *label=dsr_sc_text(button,text,36,0,104,&dsr_roboto_14,DSR_SC_TEXT);
 lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);lv_obj_align(label,LV_ALIGN_RIGHT_MID,-4,0);
}
static void cyber_device_settings(lv_obj_t *root,const dsr_mobile_device_t *d)
{
 const char *state=unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):d->busy?"Odczyt stanu…":d->online?"Online":not_online(d);
 bool online=d->online&&d->error==DSR_OK&&!unbound(d);bool extra=strcmp(state,"Online")!=0;int shift=extra?24:0;
 content=dsr_sc_panel(root,8,94,304,256,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);dsr_sc_scrollbar(content);
 lv_obj_t *panel=dsr_sc_panel(content,0,0,304,256+shift,DSR_SC_CARD,12);edge(panel,0x00cfff,1);
 dsr_sc_icon(panel,DSR_ICON_SETTINGS,16,10,24,DSR_SC_GLOW);cyber_text(panel,"Ustawienia Shelly",46,11,184,23,&dsr_roboto_18,DSR_SC_TEXT);
 lv_obj_t *s=dsr_sc_text(panel,state,extra?12:236,extra?34:14,extra?280:56,&dsr_roboto_12,online?DSR_SC_GLOW:0xffbf24);lv_obj_set_style_text_align(s,LV_TEXT_ALIGN_RIGHT,0);lv_obj_set_height(s,18);lv_label_set_long_mode(s,LV_LABEL_LONG_MODE_DOTS);
 lv_obj_t *table=dsr_sc_panel(panel,8,39+shift,288,164,DSR_SC_PANEL,6);(void)table;
 cyber_overview_row(panel,DSR_ICON_TV,"Nazwa urządzenia",d->name[0]?d->name:"—",40+shift,23,false);
 /* Identity, not a constant from the approval fixture. */
 cyber_overview_row(panel,DSR_ICON_CPU,"Model",d->identity[0]?d->identity:d->model[0]?d->model:"—",64+shift,39,true);
 cyber_overview_row(panel,DSR_ICON_FIRMWARE,"Wersja firmware",d->fw[0]?d->fw:"—",104+shift,23,false);
 cyber_overview_row(panel,DSR_ICON_SIGNAL,"Adres IP",d->host[0]?d->host:"—",128+shift,23,false);
 char value[48];snprintf(value,sizeof(value),"%u",d->channel);cyber_overview_row(panel,DSR_ICON_GIT_BRANCH,"Kanał",value,152+shift,23,false);
 if(d->physical_generation)snprintf(value,sizeof(value),"Gen %u",d->physical_generation);else snprintf(value,sizeof(value),"%s",d->identity[0]?(d->generation==DSR_GEN1?"Gen1":d->generation==DSR_GEN2_PLUS?"Gen2+":"—"):"—");
 cyber_overview_row(panel,DSR_ICON_INFO,"Zapisane ID",value,176+shift,23,false);
 lv_obj_t *b=dsr_sc_button(panel,8,209+shift,288,43,DSR_SC_BLUE,8,open_device_settings,NULL);dsr_ui_cyber_panel(b,true);dsr_sc_icon(b,DSR_ICON_SETTINGS,10,8,24,dsr_sc_icon_accent());cyber_text(b,"Otwórz ustawienia Shelly",42,12,210,21,&dsr_roboto_14,DSR_SC_TEXT);dsr_sc_icon(b,DSR_ICON_NEXT,258,9,24,dsr_sc_icon_accent());
 if(!actions.device_settings||model.demo||!d->id||d->generation!=DSR_GEN2_PLUS||unbound(d))lv_obj_add_state(b,LV_STATE_DISABLED);
 lv_obj_t *update=dsr_sc_button(root,8,354,148,48,DSR_SC_CARD,10,refresh,NULL);dsr_sc_icon(update,DSR_ICON_REFRESH,10,11,24,DSR_SC_GLOW);cyber_action_caption(update,"Odśwież status");
 if(!d->can_refresh||!actions.refresh||d->busy)lv_obj_add_state(update,LV_STATE_DISABLED);
 lv_obj_t *edit=dsr_sc_button(root,164,354,148,48,DSR_SC_CARD,10,edit_device,NULL);dsr_sc_icon(edit,DSR_ICON_EDIT,10,11,24,DSR_SC_GLOW);cyber_action_caption(edit,"Edytuj kartę");
 if(model.demo||!d->id||!actions.edit_device)lv_obj_add_state(edit,LV_STATE_DISABLED);
 cyber_text(root,"Ustawienia karty • lokalnie",12,410,296,16,&dsr_roboto_12,DSR_SC_MUTED);
}
