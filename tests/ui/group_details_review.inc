/* Shared Switch/Cover detail screens; transport is an intent capture only. */
static bool gd_group;
static unsigned gd_writes,gd_reads;
static void gd_output(unsigned index,bool on){(void)on;CHECK(!gd_group&&index==0);gd_writes++;}
static void gd_cover(unsigned index,int direction){CHECK(!gd_group&&index==0&&(direction==1||direction==-1));gd_writes++;}
static void gd_stop(unsigned index){CHECK(!gd_group&&index==0);gd_writes++;}
static void gd_position(unsigned index,uint8_t value){CHECK(!gd_group&&index==0&&value<=100);gd_writes++;}
static void gd_refresh(uint32_t id){CHECK(id==101||id==102);gd_reads|=id==101?1u:2u;}
static void gd_point(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(30);lv_indev_read(pointer);lv_timer_handler();}
static void gd_shot(const char *state,unsigned family,unsigned skin,unsigned language)
{if(language==2){char name[96];snprintf(name,sizeof(name),"group-details-%s-kind%u-skin%u",state,family,skin);screenshot(name);}}
static uint32_t gd_visual_hash(void)
{lv_refr_now(display);uint32_t hash=2166136261u;for(unsigned y=180;y<322;y++)for(unsigned x=64;x<240;x++)for(unsigned b=0;b<3;b++)hash=(hash^canvas[(y*320+x)*3+b])*16777619u;return hash;}
static bool gd_has_position(unsigned value)
{char text[48];snprintf(text,sizeof(text),"%s %u%%",dsr_sc_tr("Pozycja"),value);return find_label(lv_screen_active(),text)!=NULL;}
static bool gd_preview(lv_obj_t *o)
{if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o),dsr_sc_tr("Podgląd")))return true;for(unsigned i=0;i<lv_obj_get_child_count(o);i++)if(gd_preview(lv_obj_get_child(o,(int)i)))return true;return false;}
static void group_details_review(void)
{
 for(lv_timer_t *t=lv_timer_get_next(NULL);t;t=lv_timer_get_next(t))if(t!=lv_display_get_refr_timer(display))lv_timer_pause(t);
 const dsr_mobile_actions_t a={.set_group=capture_group,.set_output=gd_output,.set_cover=gd_cover,.stop_cover=gd_stop,.set_position=gd_position,.refresh=gd_refresh,.edit_action=group_edit_fixture};dsr_mobile_bind(&a);
 const unsigned skins[]={0,2,3};
 for(unsigned l=0;l<3;l++)for(unsigned s=0;s<3;s++)for(unsigned family=0;family<2;family++){
  unsigned language=(l+2)%3,skin=skins[s];dsr_mobile_model_t m={.wifi=true,.device_count=2,.group_count=1};m.groups[0]=(dsr_mobile_group_t){.id=31,.members=2,.member_mask=3};snprintf(m.groups[0].name,48,"%s",family?"Rollläden Terrasse":"Licht im Erdgeschoss");
  for(unsigned i=0;i<2;i++)m.devices[i]=(dsr_mobile_device_t){.id=101+i,.name="Einzelgerät Terrasse",.identity="detail-fixture",.generation=DSR_GEN2_PLUS,.channel=(uint8_t)i,.endpoint_kind=(uint8_t)family,.kind=family?DSR_DEVICE_COVER:DSR_DEVICE_RELAY,.online=true,.can_switch=true,.can_cover=true,.can_stop_cover=true,.can_position=true,.can_refresh=true,.has_sample_time=true,.sample_ms=lv_tick_get(),.has_position=true,.position_pct=50,.cover_state=DSR_COVER_STOPPED,.has_power=true,.power_w=10.5,.has_voltage=true,.voltage_v=231.5,.has_current=true,.current_a=.045};
  dsr_sc_appearance(DSR_SC_BLUE,skin,language);dsr_ui_apply_theme();dsr_mobile_update(&m);gd_group=false;dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());
  lv_obj_t *main=group_sized(lv_screen_active(),family?176:86,family?142:86);CHECK(main);lv_area_t single;lv_obj_get_coords(main,&single);gd_shot("single",family,skin,language);
  unsigned physical=gd_writes;if(family){group_press("Otwórz");group_press("Zamknij");group_press("Stop");CHECK(gd_writes==physical+3);}else{group_touch(main);CHECK(gd_writes==physical+1);}physical=gd_writes;
  gd_group=true;dsr_mobile_open_group(31);lv_obj_update_layout(lv_screen_active());main=group_sized(lv_screen_active(),family?176:86,family?142:86);CHECK(main);lv_area_t group;lv_obj_get_coords(main,&group);CHECK(!memcmp(&single,&group,sizeof(group)));gd_shot("group",family,skin,language);
  unsigned before=group_commands;
  if(!family){
   group_touch(main);CHECK(group_commands==before+1&&group_id==31&&group_intent.command==DSR_ACTION_ON);
   m.devices[0].output=m.devices[1].output=true;dsr_mobile_update(&m);group_touch(group_sized(lv_screen_active(),86,86));CHECK(group_intent.command==DSR_ACTION_OFF);
   m.devices[1].output=false;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Różne stany"));gd_shot("mixed",family,skin,language);group_touch(group_sized(lv_screen_active(),86,86));CHECK(group_intent.command==DSR_ACTION_ON);
   gd_reads=0;group_press("Odśwież stan");CHECK(gd_reads==3);
  }else{
   group_press("Otwórz");CHECK(group_commands==before+1&&group_id==31&&group_intent.command==DSR_ACTION_OPEN);group_press("Zamknij");CHECK(group_intent.command==DSR_ACTION_CLOSE);group_press("Stop");CHECK(group_intent.command==DSR_ACTION_STOP);
   before=group_commands;gd_point(152,270,true);gd_point(152,225,true);CHECK(group_commands==before&&gd_has_position(50));gd_point(152,225,false);CHECK(group_commands==before+1&&group_intent.command==DSR_ACTION_POSITION&&group_intent.value[0]<=100&&m.devices[0].position_pct==50);
   m.devices[1].position_pct=80;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Różne stany")&&!gd_has_position(50));gd_shot("mixed",family,skin,language);
   before=group_commands;uint32_t mixed=gd_visual_hash();gd_point(152,270,true);gd_point(152,225,true);CHECK(group_commands==before&&gd_preview(lv_screen_active())&&gd_visual_hash()!=mixed);gd_shot("mixed-preview",family,skin,language);gd_point(152,225,false);CHECK(group_commands==before+1&&group_intent.command==DSR_ACTION_POSITION);
   before=group_commands;gd_point(152,270,true);m.devices[1].channel=7;dsr_mobile_update(&m);gd_point(152,225,false);CHECK(group_commands==before);
   m.devices[1].position_pct=m.devices[0].position_pct=50;dsr_mobile_update(&m);
   before=group_commands;gd_point(152,270,true);gd_point(152,225,true);CHECK(gd_preview(lv_screen_active()));m.devices[1].can_position=false;dsr_mobile_update(&m);gd_point(152,225,false);CHECK(group_commands==before&&!gd_preview(lv_screen_active()));
   m.devices[1].can_position=true;dsr_mobile_update(&m);gd_point(152,270,true);gd_point(152,225,true);m.devices[1].position_pct=80;dsr_mobile_update(&m);gd_point(152,225,false);CHECK(group_commands==before&&!gd_preview(lv_screen_active())&&find_label(lv_screen_active(),"Różne stany"));
   gd_point(152,270,true);gd_point(152,225,true);m.devices[1].position_pct=50;dsr_mobile_update(&m);gd_point(152,225,false);CHECK(group_commands==before&&!gd_preview(lv_screen_active())&&gd_has_position(50));
   for(unsigned i=0;i<2;i++){m.devices[i].position_pct=70;m.devices[i].cover_state=DSR_COVER_OPENING;}dsr_mobile_update(&m);
   CHECK(gd_has_position(70));uint32_t first=gd_visual_hash();lv_tick_inc(400);lv_anim_refr_now();CHECK(gd_visual_hash()!=first);gd_shot("moving",family,skin,language);group_press("Stop");CHECK(group_intent.command==DSR_ACTION_STOP);
  }
  unsigned edits=group_edits;group_press("Ustawienia");CHECK(group_edits==edits+1&&edited_group_id==31&&gd_writes==physical);
  gd_group=false;dsr_mobile_show(DSR_MOBILE_HOME,0);
 }
 printf("PASS: shared Switch/Cover group details; %u checks, %u selected frames; group-only intents, mixed state, position gesture/cancel, animation and stable member refresh; host MODEL\n",checks,screenshots);
}
