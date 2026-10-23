/* Group TRV uses the physical TRV dial; radio writes remain captured intents. */
static unsigned group_trv_physical_writes;
static void group_trv_physical(unsigned index,uint8_t half)
{CHECK(index==0&&half==44);group_trv_physical_writes++;}
static void group_trv_pointer(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(30);lv_indev_read(pointer);lv_timer_handler();}
static unsigned group_trv_icons(lv_obj_t *o,dsr_ui_icon_t icon)
{
 unsigned count=0;
 if(lv_obj_get_width(o)==24&&lv_obj_get_height(o)==24)
  for(unsigned i=0;i<lv_obj_get_event_count(o);i++)if((uintptr_t)lv_event_dsc_get_user_data(lv_obj_get_event_dsc(o,i))==(uintptr_t)icon)count++;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)count+=group_trv_icons(lv_obj_get_child(o,(int)i),icon);
 return count;
}
static void group_trv_wait(unsigned ms){lv_tick_inc(ms);lv_timer_handler();}
static void group_trv_review(void)
{
 for(lv_timer_t *t=lv_timer_get_next(NULL);t;t=lv_timer_get_next(t))if(t!=lv_display_get_refr_timer(display))lv_timer_pause(t);
 const dsr_mobile_actions_t a={.set_group=capture_group,.set_trv=group_trv_physical,.edit_action=group_edit_fixture};dsr_mobile_bind(&a);
 const unsigned skins[]={0,2,3};
 for(unsigned l=0;l<3;l++)for(unsigned s=0;s<3;s++){
  dsr_mobile_model_t m={.wifi=true,.device_count=2,.group_count=1};m.groups[0]=(dsr_mobile_group_t){.id=31,.members=2,.member_mask=3};strcpy(m.groups[0].name,"Thermostate Schlafzimmer");
  for(unsigned i=0;i<2;i++){dsr_mobile_device_t *d=&m.devices[i];d->id=i+101;d->generation=DSR_GEN2_PLUS;d->endpoint_kind=DSR_ENDPOINT_BLU;d->kind=DSR_DEVICE_SENSOR;snprintf(d->identity,sizeof(d->identity),"group-trv-%u",i);snprintf(d->name,sizeof(d->name),"Thermostat %u",i+1);d->online=d->has_trv_target=d->has_temperature=d->has_battery=true;d->temperature_c=21.5;d->trv_target_c=i?19:21.5;d->battery_pct=i?64:82;d->blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET;d->blu.trv_pos=i?47:28;}
  dsr_sc_appearance(DSR_SC_BLUE,skins[s],(l+2)%3);dsr_ui_apply_theme();dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());
  lv_obj_t *arc=find_type(lv_screen_active(),&lv_arc_class);CHECK(arc);lv_area_t single;lv_obj_get_coords(arc,&single);
  if(l==0){char name[64];snprintf(name,sizeof(name),"groups-trv-single-skin%u",skins[s]);screenshot(name);}
  unsigned physical=group_trv_physical_writes;group_press("+");group_trv_wait(1300);CHECK(group_trv_physical_writes==physical+1);
  dsr_mobile_open_group(31);lv_obj_update_layout(lv_screen_active());arc=find_type(lv_screen_active(),&lv_arc_class);CHECK(arc);lv_area_t group;lv_obj_get_coords(arc,&group);CHECK(!memcmp(&single,&group,sizeof(single)));
  CHECK(group_trv_icons(lv_screen_active(),DSR_ICON_TRV_VALVE)==2&&group_trv_icons(lv_screen_active(),DSR_ICON_BATTERY)==2);
  CHECK(find_label(lv_screen_active(),"82%")&&find_label(lv_screen_active(),"64%")&&find_label(lv_screen_active(),"28%")&&find_label(lv_screen_active(),"47%"));
  CHECK(find_label(lv_screen_active(),"Thermostat 1")&&find_label(lv_screen_active(),"Thermostat 2"));
  if(l==0){char name[64];snprintf(name,sizeof(name),"groups-trv-dial-skin%u",skins[s]);screenshot(name);}
  unsigned before=group_commands;group_press("+");CHECK(group_commands==before);group_trv_wait(1000);CHECK(group_commands==before);group_trv_wait(300);CHECK(group_commands==before+1&&group_id==31&&group_intent.command==DSR_ACTION_TARGET&&group_intent.value[0]==43);
  /* Real pointer drag holds the write until release and a quiet interval. */
  int cx=skins[s]==3?162:160,cy=skins[s]==3?218:222,r=skins[s]==3?96:82;
  group_trv_pointer(cx,cy-r,true);group_trv_pointer(cx+r,cy,true);group_trv_wait(1400);CHECK(group_commands==before+1);
  group_trv_pointer(cx+r,cy,false);group_trv_wait(1000);CHECK(group_commands==before+1);group_trv_wait(300);CHECK(group_commands==before+2&&group_id==31&&group_intent.command==DSR_ACTION_TARGET);
  /* A draft must never follow a changed member identity or a different page. */
  group_press("+");strcpy(m.devices[1].identity,"replacement-trv");dsr_mobile_update(&m);group_trv_wait(1400);CHECK(group_commands==before+2);
  group_press("+");dsr_mobile_show(DSR_MOBILE_HOME,0);group_trv_wait(1400);CHECK(group_commands==before+2&&group_trv_physical_writes==physical+1);
  dsr_mobile_open_group(31);m.devices[1].online=false;m.devices[1].has_battery=false;dsr_mobile_update(&m);CHECK(!find_label(lv_screen_active(),"64%")&&!find_label(lv_screen_active(),"47%"));
  if(l==0&&skins[s]==3){m.devices[0].online=false;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);screenshot("groups-trv-single-offline-skin3");dsr_mobile_open_group(31);screenshot("groups-trv-dial-offline-skin3");}
 }
 printf("PASS: group TRV shared dial, per-member battery/valve, delayed common target and cancelled drafts; %u checks, %u selected frames; host intents\n",checks,screenshots);
}
