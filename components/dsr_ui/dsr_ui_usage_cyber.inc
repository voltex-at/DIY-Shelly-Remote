/* Live measurements only. The scale expresses range, never alarm severity. */
static void cyber_usage_value(lv_obj_t *parent,dsr_ui_icon_t icon,const char *title,const char *value,int x,int y,int w)
{
 lv_obj_t *card=dsr_sc_panel(parent,x,y,w,52,DSR_SC_CARD,9);
 dsr_sc_icon(card,icon,10,14,24,DSR_SC_GLOW);
 dsr_sc_text(card,title,44,5,w-54,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(card,dsr_sc_user(value),44,24,w-54,&dsr_roboto_18,DSR_SC_TEXT);
}
static void cyber_usage(lv_obj_t *root,const dsr_mobile_device_t *d,bool exists)
{
 button(root,"Sterowanie",0,44,104,back,0,false);
 button(root,"Zużycie",108,44,104,NULL,0,true);
 button(root,"Ustawienia",216,44,104,settings,0,false);
 bool fresh=exists&&d->online&&d->error==DSR_OK&&d->has_power&&isfinite(d->power_w)&&d->has_sample_time&&lv_tick_get()-d->sample_ms<=15000;
 lv_obj_t *card=dsr_sc_panel(root,12,96,296,222,DSR_SC_CARD,12);
 dsr_sc_icon(card,DSR_ICON_WATTAGE,12,10,24,DSR_SC_GLOW);
 dsr_sc_text(card,"Moc chwilowa",44,13,236,&dsr_roboto_16,DSR_SC_TEXT);
 const double scales[]={500,1000,2000,3500,5000,10000,25000};
 double watts=fresh&&d->power_w>0?d->power_w:0,scale=500;
 for(unsigned i=0;i<sizeof(scales)/sizeof(scales[0]);i++){scale=scales[i];if(watts<=scale*.95)break;}
 const uint32_t colors[]={0x004b93,0x007bff,0x00e5ff};
 dsr_sc_gradient_arc(card,148,113,76,10,135,270,colors,3,18,fresh);
 for(unsigned i=0;i<=10;i++){
  double a=(135.0+270.0*i/10)*DSR_PI/180;
  const lv_point_precise_t tick[2]={{160+62*cos(a),210+62*sin(a)},{160+67*cos(a),210+67*sin(a)}};
  gauge_line(root,tick,1,DSR_SC_MUTED,LV_OPA_60);
 }
 if(fresh){
  double fraction=watts/scale;if(fraction>1)fraction=1;
  double a=(135+270*fraction)*DSR_PI/180;
  const lv_point_precise_t needle[2]={{160,210},{160+49*cos(a),210+49*sin(a)}};
  gauge_line(root,needle,3,DSR_SC_GLOW,LV_OPA_COVER);
 }
 char value[48],unit[8],top[16];gauge_value(watts,scale,value,sizeof(value),unit,sizeof(unit),top,sizeof(top));
 lv_obj_t *v=dsr_sc_text(card,fresh?value:"—",48,131,200,&dsr_roboto_28,DSR_SC_TEXT);lv_obj_set_style_text_align(v,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_t *u=dsr_sc_text(card,unit,108,166,80,&dsr_roboto_14,DSR_SC_MUTED);lv_obj_set_style_text_align(u,LV_TEXT_ALIGN_CENTER,0);
 dsr_sc_text(card,"0",40,181,40,&dsr_roboto_12,DSR_SC_MUTED);dsr_sc_text(card,top,218,181,66,&dsr_roboto_12,DSR_SC_MUTED);
 if(fresh&&d->has_energy&&isfinite(d->energy_wh))snprintf(value,sizeof(value),"%.3f kWh",d->energy_wh/1000);else snprintf(value,sizeof(value),"— kWh");
 cyber_usage_value(root,DSR_ICON_ENERGY,"Licznik energii",value,12,326,296);
 if(fresh&&d->has_voltage&&isfinite(d->voltage_v))snprintf(value,sizeof(value),"%.1f V",d->voltage_v);else snprintf(value,sizeof(value),"— V");
 cyber_usage_value(root,DSR_ICON_VOLTAGE,"Napięcie",value,12,386,144);
 if(fresh&&d->has_current&&isfinite(d->current_a))snprintf(value,sizeof(value),"%.3f A",d->current_a);else snprintf(value,sizeof(value),"— A");
 cyber_usage_value(root,DSR_ICON_CURRENT,"Natężenie",value,164,386,144);
 usage_age=dsr_sc_text(root,"",12,446,296,&dsr_roboto_12,fresh?DSR_SC_MUTED:0xffc18c);lv_obj_set_style_text_align(usage_age,LV_TEXT_ALIGN_CENTER,0);usage_age_update(d,exists,fresh);
}
