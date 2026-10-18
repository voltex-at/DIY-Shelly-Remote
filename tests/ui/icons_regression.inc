/* 0.7.51 owner requests on the actual LVGL UI: device icon gallery saved in
 * the workspace, status bar clock, visible scroll position indicator. */
static lv_obj_t *icon_choice(unsigned index)
{
 lv_obj_t *caption=find_label(lv_screen_active(),"Ikona");CHECK(caption);lv_obj_t *body=lv_obj_get_parent(caption);
 int32_t at=lv_obj_get_index(caption);CHECK(at>=0&&(uint32_t)at+1+index<lv_obj_get_child_count(body));
 return lv_obj_get_child(body,at+1+(int32_t)index);
}
static bool has_icon(lv_obj_t *o,unsigned kind,int size)
{
 for(uint32_t i=0;i<lv_obj_get_event_count(o);i++){lv_event_dsc_t *e=lv_obj_get_event_dsc(o,i);if(lv_event_dsc_get_user_data(e)==(void *)(uintptr_t)kind&&lv_obj_get_width(o)==size)return true;}
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)if(has_icon(lv_obj_get_child(o,(int)i),kind,size))return true;
 return false;
}
static int workspace_icon(uint32_t id)
{
 char *json=dsr_workspace_snapshot();cJSON *r=json?cJSON_Parse(json):NULL;free(json);CHECK(r);int icon=-1;const cJSON *d;
 cJSON_ArrayForEach(d,cJSON_GetObjectItemCaseSensitive(r,"devices"))if((uint32_t)cJSON_GetObjectItemCaseSensitive(d,"id")->valueint==id){const cJSON *v=cJSON_GetObjectItemCaseSensitive(d,"icon");icon=v?v->valueint:0;}
 cJSON_Delete(r);return icon;
}
static void icons_regression(void)
{
 CHECK(!wifi.connected&&!in_flight);
 uint32_t a=review_entry_id("Relay A");CHECK(workspace_icon(a)==0);
 dsr_network_device_settings(a);click("Edytuj kartę");CHECK(find_label(lv_screen_active(),"Ikona")&&find_label(lv_screen_active(),"Auto"));
 /* 21 round choices (Auto + 20 icons); the thermostat (11) is picked and saved. */
 lv_obj_send_event(icon_choice(11),LV_EVENT_CLICKED,NULL);lv_obj_update_layout(lv_screen_active());
 CHECK(lv_obj_get_style_border_width(icon_choice(11),0)==3&&lv_obj_get_style_border_width(icon_choice(0),0)==1);screenshot("270-device-icon-gallery");
 click("Zapisz");dsr_ui_workspace_tick();tick();CHECK(workspace_icon(a)==11);
 dsr_mobile_device_t d;CHECK(dsr_network_typed_device(a,&d)&&d.icon==11);
 dsr_network_home(NULL);dsr_mobile_show(DSR_MOBILE_DEVICES,0);lv_obj_update_layout(lv_screen_active());CHECK(has_icon(lv_screen_active(),DSR_ICON_THERMOSTAT,30));screenshot("271-device-card-chosen-icon");
 /* Device lists show a scroll position bar on the right. */
 lv_obj_t *list=lv_obj_get_parent(find_label(lv_screen_active(),"Relay A"));while(list&&!lv_obj_has_flag(list,LV_OBJ_FLAG_SCROLLABLE))list=lv_obj_get_parent(list);
 CHECK(list&&lv_obj_get_style_width(list,LV_PART_SCROLLBAR)==4&&lv_obj_get_scrollbar_mode(list)==LV_SCROLLBAR_MODE_AUTO);
 /* "Auto" removes the key: the stored file stays readable by 0.7.50. */
 dsr_network_device_settings(a);click("Edytuj kartę");lv_obj_send_event(icon_choice(0),LV_EVENT_CLICKED,NULL);click("Zapisz");dsr_ui_workspace_tick();tick();
 CHECK(workspace_icon(a)==0&&dsr_network_typed_device(a,&d)&&d.icon==0);
 /* Status bar clock: HH:MM local time next to Wi-Fi and battery, empty without a plausible UTC. */
 dsr_network_home(NULL);dsr_mobile_show(DSR_MOBILE_ROOMS,0);lv_obj_update_layout(lv_screen_active());
 bool clock=false;for(unsigned i=0;i<lv_obj_get_child_count(lv_screen_active());i++){lv_obj_t *o=lv_obj_get_child(lv_screen_active(),(int)i);
  if(lv_obj_check_type(o,&lv_label_class)&&lv_obj_get_x(o)==204){const char *t=lv_label_get_text(o);clock=strlen(t)==5&&t[2]==':'&&t[0]>='0'&&t[0]<='2';}}
 CHECK(clock);
 printf("PASS: device icon gallery (save/Auto/card), list scroll bar, status bar clock\n");
 dsr_network_home(NULL);
}
