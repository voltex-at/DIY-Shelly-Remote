/* Focused genuine LVGL controls; command capture is a model, not radio proof. */
static unsigned group_commands;static uint32_t group_id;static dsr_group_intent_t group_intent;
static unsigned group_edits;static uint32_t edited_group_id;
static void capture_group(uint32_t id,dsr_group_intent_t intent)
{group_commands++;group_id=id;group_intent=intent;}
static void group_edit_fixture(uint8_t kind,uint32_t id){CHECK(kind==0);group_edits++;edited_group_id=id;}
static void group_no_physical_output(unsigned index,bool on){(void)index;(void)on;CHECK(false);}
static void group_no_physical_cover(unsigned index,int direction){(void)index;(void)direction;CHECK(false);}
static void group_no_physical_trv(unsigned index,uint8_t half){(void)index;(void)half;CHECK(false);}
static void group_no_physical_usage(uint32_t id){(void)id;CHECK(false);}
static void group_review_shot(const char *state,unsigned family,unsigned theme,unsigned language)
{if(language==2){char name[96];snprintf(name,sizeof(name),"groups-%s-kind%u-skin%u",state,family,theme);screenshot(name);}}
static void group_touch(lv_obj_t *b)
{CHECK(b);lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(b,&a);CHECK(a.x1>=0&&a.x2<320&&a.y1>=0&&a.y2<DSR_SC_NAV_TOP);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);}
static void group_press(const char *text)
{lv_obj_update_layout(lv_screen_active());lv_obj_t *l=find_label(lv_screen_active(),text);CHECK(l);group_touch(lv_obj_get_parent(l));}
static lv_obj_t *group_sized(lv_obj_t *o,int w,int h)
{if(lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))return o;for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=group_sized(lv_obj_get_child(o,(int)i),w,h);if(r)return r;}return NULL;}
static void group_review(unsigned theme,unsigned language,uint8_t family)
{
 dsr_mobile_model_t *m=calloc(1,sizeof(*m));CHECK(m);m->wifi=true;m->configured_layout=true;m->dashboard_id=1;strcpy(m->dashboard_name,"Mein Dashboard");strcpy(m->home_name,"Home");m->room_count=1;strcpy(m->rooms[0].name,"Parterre");m->device_count=2;m->group_count=1;m->groups[0]=(dsr_mobile_group_t){.id=31,.members=2,.member_mask=3};
 const char *names[]={"Licht im Erdgeschoss","Rollläden Terrasse","Dimmer im Wohnzimmer","RGB im Wohnzimmer","Thermostate Schlafzimmer"};snprintf(m->groups[0].name,48,"%s",names[family==DSR_ENDPOINT_BLU?4:family]);m->tile_count=1;m->tiles[0]=(dsr_mobile_tile_t){.type=3,.index=0,.span=2,.height=1};
 for(unsigned i=0;i<2;i++){dsr_mobile_device_t *d=&m->devices[i];d->id=i+1;d->endpoint_kind=family;d->generation=DSR_GEN2_PLUS;snprintf(d->identity,64,"shelly-fixture-%u",i);snprintf(d->name,48,"Member %u",i+1);d->online=d->can_switch=d->has_power=true;d->power_w=10.5;d->kind=family==DSR_ENDPOINT_BLU?DSR_DEVICE_SENSOR:family==DSR_ENDPOINT_COVER?DSR_DEVICE_COVER:DSR_DEVICE_LIGHT;
  if(family==DSR_ENDPOINT_COVER){d->has_position=d->can_cover=true;d->position_pct=50;}
  if(family==DSR_ENDPOINT_LIGHT){d->can_light=true;d->light=(dsr_light_status_t){.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS,.brightness=65};}
  if(family==DSR_ENDPOINT_RGB){d->can_rgb=true;d->rgb=(dsr_rgb_status_t){.flags=DSR_RGB_VALID,.brightness=65,.r=10,.g=120,.b=250};}
  if(family==DSR_ENDPOINT_BLU){d->blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET;d->has_trv_target=d->has_temperature=d->has_battery=true;d->trv_target_c=d->temperature_c=21.5;d->battery_pct=82;}
 }
 dsr_sc_appearance(5020661,theme,language);dsr_ui_apply_theme();dsr_mobile_update(m);dsr_mobile_show(DSR_MOBILE_HOME,0);group_review_shot("dashboard",family,theme,language);
 if(theme==3&&language==2&&(family==0||family==1||family==5)){m->tile_count=2;m->tiles[0]=(dsr_mobile_tile_t){.type=0,.index=0,.span=2,.height=1,.fields=255};m->tiles[1]=(dsr_mobile_tile_t){.type=3,.index=0,.span=2,.height=1};dsr_mobile_update(m);group_review_shot("same-renderer",family,theme,language);m->tile_count=1;m->tiles[0]=(dsr_mobile_tile_t){.type=3,.index=0,.span=2,.height=1};dsr_mobile_update(m);}
 unsigned before=group_commands;
 if(family==DSR_ENDPOINT_SWITCH||family==DSR_ENDPOINT_LIGHT||family==DSR_ENDPOINT_RGB){group_touch(group_sized(lv_screen_active(),58,58));CHECK(group_commands==before+1&&group_id==31&&group_intent.command==DSR_ACTION_ON);}
 else if(family==DSR_ENDPOINT_COVER){lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),m->groups[0].name));lv_obj_t *up=group_sized(card,44,44);CHECK(up);lv_obj_t *pair=lv_obj_get_parent(up);group_touch(up);CHECK(group_intent.command==DSR_ACTION_OPEN);lv_obj_t *down=group_sized(lv_obj_get_child(pair,-1),44,44);CHECK(down);group_touch(down);CHECK(group_intent.command==DSR_ACTION_CLOSE);m->devices[0].cover_state=DSR_COVER_OPENING;dsr_mobile_update(m);group_press("Stop");CHECK(group_intent.command==DSR_ACTION_STOP);m->devices[0].cover_state=DSR_COVER_STOPPED;dsr_mobile_update(m);}
 else{group_press("+");CHECK(group_commands==before+1&&group_intent.command==DSR_ACTION_TARGET&&group_intent.value[0]==44);}
 if(family==DSR_ENDPOINT_SWITCH){m->devices[0].output=m->devices[1].output=true;dsr_mobile_update(m);CHECK(find_label(lv_screen_active(),"Włączone"));group_touch(group_sized(lv_screen_active(),58,58));CHECK(group_intent.command==DSR_ACTION_OFF);m->devices[0].output=false;dsr_mobile_update(m);CHECK(find_label(lv_screen_active(),"Różne"));}
 if(family==DSR_ENDPOINT_LIGHT||family==DSR_ENDPOINT_RGB){m->devices[0].output=m->devices[1].output=true;if(family==DSR_ENDPOINT_LIGHT)m->devices[1].light.brightness=25;else m->devices[1].rgb.r=200;dsr_mobile_update(m);group_touch(group_sized(lv_screen_active(),58,58));CHECK(group_intent.command==DSR_ACTION_OFF);m->devices[0].output=m->devices[1].output=false;if(family==DSR_ENDPOINT_LIGHT)m->devices[1].light.brightness=65;else m->devices[1].rgb.r=10;dsr_mobile_update(m);}
 dsr_mobile_show(DSR_MOBILE_DEVICES,0);CHECK(find_label(lv_screen_active(),m->groups[0].name));group_review_shot("devices",family,theme,language);
 dsr_mobile_open_group(31);CHECK(dsr_mobile_page()==DSR_MOBILE_GROUP_CONTROL);group_review_shot("control",family,theme,language);
 if(family==DSR_ENDPOINT_LIGHT){CHECK(find_type(lv_screen_active(),&lv_arc_class));lv_obj_t *usage=find_label(lv_screen_active(),"Zużycie");CHECK(usage&&lv_obj_has_state(lv_obj_get_parent(usage),LV_STATE_DISABLED));group_press("Zużycie");CHECK(dsr_mobile_page()==DSR_MOBILE_GROUP_CONTROL);group_press("25%");CHECK(group_intent.command==DSR_ACTION_BRIGHTNESS&&group_intent.value[0]==25);unsigned edits=group_edits;group_press("Ustawienia");CHECK(group_edits==edits+1&&edited_group_id==31);}
 else if(family==DSR_ENDPOINT_RGB){group_press("Kolor");lv_obj_t *red=find_type(lv_screen_active(),&lv_slider_class);CHECK(red);lv_slider_set_value(red,200,LV_ANIM_OFF);lv_obj_send_event(red,LV_EVENT_VALUE_CHANGED,NULL);CHECK(find_label(lv_screen_active(),"R  200"));group_review_shot("color",family,theme,language);group_press("Zastosuj");CHECK(group_intent.command==DSR_ACTION_RGB&&group_intent.value[0]==200&&group_intent.value[1]==120&&group_intent.value[2]==250);
  lv_slider_set_value(red,111,LV_ANIM_OFF);lv_obj_send_event(red,LV_EVENT_VALUE_CHANGED,NULL);for(unsigned i=0;i<2;i++){m->devices[i].rgb.g=77;m->devices[i].rgb.b=88;}dsr_mobile_update(m);CHECK(find_label(lv_screen_active(),"G  77")&&find_label(lv_screen_active(),"B  88"));red=find_type(lv_screen_active(),&lv_slider_class);CHECK(red);lv_slider_set_value(red,200,LV_ANIM_OFF);lv_obj_send_event(red,LV_EVENT_VALUE_CHANGED,NULL);group_press("Zastosuj");CHECK(group_intent.value[0]==200&&group_intent.value[1]==77&&group_intent.value[2]==88);group_press("Sterowanie");CHECK(dsr_mobile_page()==DSR_MOBILE_GROUP_CONTROL&&find_label(lv_screen_active(),"Sterowanie RGB"));unsigned edits=group_edits;group_press("Ustawienia");CHECK(group_edits==edits+1&&edited_group_id==31);
 }
 else if(family==DSR_ENDPOINT_COVER){group_press("Otwórz");CHECK(group_intent.command==DSR_ACTION_OPEN);group_press("Stop");CHECK(group_intent.command==DSR_ACTION_STOP);}
 else if(family==DSR_ENDPOINT_BLU){group_press("+");lv_tick_inc(1300);lv_timer_handler();CHECK(group_intent.command==DSR_ACTION_TARGET&&group_intent.value[0]==44);}

 /* Mixed TRV targets open explicit common setpoint, never infer a relative
  * change for each member. Draft changes alone never emit a command. */
 if(family==DSR_ENDPOINT_BLU){m->devices[1].trv_target_c=19;dsr_mobile_show(DSR_MOBILE_HOME,0);dsr_mobile_update(m);before=group_commands;CHECK(find_label(lv_screen_active(),"Różne"));group_press("+");CHECK(group_commands==before&&find_label(lv_screen_active(),"21.0 °C"));group_review_shot("mixed",family,theme,language);group_press("+");lv_tick_inc(1300);lv_timer_handler();CHECK(group_intent.command==DSR_ACTION_TARGET&&group_intent.value[0]==43);}
 dsr_mobile_show(DSR_MOBILE_HOME,0);m->devices[1].endpoint_kind=DSR_ENDPOINT_HT;dsr_mobile_update(m);before=group_commands;CHECK(find_label(lv_screen_active(),"Sprawdź powiązania"));CHECK(lv_obj_has_state(group_sized(lv_screen_active(),58,58),LV_STATE_DISABLED));CHECK(group_commands==before);free(m);
}
static void group_controls_review(void)
{
 /* New group and its active-dashboard tile share the same durable write;
  * failure/retry and later edit cannot duplicate that tile. */
 dsr_ui_edit_action(0,0);action_editor_name("Licht im Erdgeschoss");review_press("Dalej");action_editor_select("Relay A",false);action_editor_select("Relay B",false);review_press("Dalej");test_workspace_defer(true);review_press("Zapisz grupę");test_workspace_finish(false);dsr_ui_workspace_tick();cJSON *doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"groups"))==0);cJSON_Delete(doc);review_press("Zapisz grupę");test_workspace_finish(true);dsr_ui_workspace_tick();test_workspace_defer(false);doc=room_document();cJSON *dashboard=cJSON_GetObjectItemCaseSensitive(doc,"dashboards")->child,*tiles=cJSON_GetObjectItemCaseSensitive(dashboard,"tiles");CHECK(cJSON_GetArraySize(tiles)==1&&!strcmp(cJSON_GetObjectItemCaseSensitive(tiles->child,"type")->valuestring,"group"));uint32_t id=(uint32_t)cJSON_GetObjectItemCaseSensitive(tiles->child,"ref")->valueint;cJSON_Delete(doc);
 dsr_ui_edit_action(0,id);review_press("Dalej");review_press("Dalej");review_press("Zapisz grupę");dsr_ui_workspace_tick();doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"dashboards")->child,"tiles"))==1);dsr_mobile_model_t parsed={0};char *json=cJSON_PrintUnformatted(doc);CHECK(json&&dsr_mobile_configure(&parsed,json)&&parsed.groups[0].member_mask==3);free(json);cJSON_Delete(doc);
 /* Freeze network-provider timers for isolated presentation/intent capture. */
 for(lv_timer_t *t=lv_timer_get_next(NULL);t;t=lv_timer_get_next(t))if(t!=lv_display_get_refr_timer(display))lv_timer_pause(t);
 const dsr_mobile_actions_t actions={.set_group=capture_group,.edit_action=group_edit_fixture,.set_output=group_no_physical_output,.set_cover=group_no_physical_cover,.set_trv=group_no_physical_trv,.open_usage=group_no_physical_usage};dsr_mobile_bind(&actions);const unsigned skins[]={0,2,3};const uint8_t families[]={0,1,2,3,5};for(unsigned l=0;l<3;l++)for(unsigned s=0;s<3;s++)for(unsigned f=0;f<5;f++)group_review(skins[s],(l+2)%3,families[f]);
 printf("PASS: focused group controls, %u checks, %u selected frames; typed intents, Home/Devices, common and mixed TRV, durable tile/retry; radio/storage MODEL\n",checks,screenshots);
}
