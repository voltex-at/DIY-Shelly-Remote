/* Contextual scene/group forms share the workspace editor's immutable submit,
 * revision/CAS, exact durable ACK and discard handling. No actor I/O here. */
static void action_form(void);
static void action_leave(lv_event_t *event)
{(void)event;uint8_t kind=action_editor.kind;dsr_ui_workspace_hide();dsr_ui_actions_open(kind);}
static bool action_put(const char *key,cJSON *value)
{
 if(!value)return false;
 if(item(editing,key))return cJSON_ReplaceItemInObjectCaseSensitive(editing,key,value);
 return cJSON_AddItemToObject(editing,key,value);
}
static lv_obj_t *action_pick(const char *label,int y,bool selected,lv_event_cb_t callback,uintptr_t value)
{
 lv_obj_t *b=dsr_ui_button(body,label,0,y,284,callback,(void *)value);
 lv_obj_set_style_bg_color(b,dsr_sc_color(DSR_SC_CARD),0);lv_obj_set_style_radius(b,12,0);
 lv_obj_set_style_border_width(b,selected?2:1,0);lv_obj_set_style_border_color(b,dsr_sc_color(selected?DSR_SC_BLUE:DSR_SC_EDGE),0);
 lv_obj_t *label_obj=lv_obj_get_child(b,0);lv_obj_set_width(label_obj,264);lv_obj_set_height(label_obj,22);lv_label_set_long_mode(label_obj,LV_LABEL_LONG_MODE_DOTS);
 return b;
}
static void action_back(lv_event_t *event)
{
 (void)event;if(own_save){dsr_sc_label_set_text(status_label,"Poczekaj na zakończenie zapisu.");return;}
 if(action_editor.selecting){action_editor.selecting=false;action_form();return;}
 if(!action_editor.deleted&&action_editor.step){action_editor.step--;action_form();return;}
 if(!room_dirty()){action_leave(NULL);return;}
 lv_obj_t *panel=room_modal("Odrzucić zmiany?","Niezapisane zmiany tego formularza zostaną utracone.");
 dsr_ui_button(panel,"Zostań",12,188,130,room_dialog_close,NULL);dsr_ui_button(panel,"Odrzuć",154,188,130,action_leave,NULL);
}
static void action_next(lv_event_t *event)
{(void)event;if(action_editor.step<3){action_editor.step++;action_editor.query[0]=0;action_form();}}
static void action_room(lv_event_t *event)
{action_put("room",cJSON_CreateNumber((double)(uintptr_t)lv_event_get_user_data(event)));action_form();}
static void action_expand(lv_event_t *event)
{uint32_t id=(uint32_t)(uintptr_t)lv_event_get_user_data(event);action_editor.expanded=action_editor.expanded==id?UINT32_MAX:id;action_form();}
static void action_query(lv_event_t *event)
{snprintf(action_editor.query,sizeof(action_editor.query),"%s",lv_textarea_get_text(lv_event_get_target(event)));}
static void action_keyboard_done(lv_event_t *event)
{(void)event;action_form();}
static bool action_device_supported(const cJSON *d)
{const char *k=string(d,"kind");return !strcmp(k,"relay")||!strcmp(k,"plug")||(action_editor.kind&&!strcmp(k,"cover"));}
static bool action_included(uint32_t id)
{const cJSON *v;cJSON_ArrayForEach(v,item(editing,"members"))if((uint32_t)v->valueint==id)return true;return false;}
static void action_member(lv_event_t *event)
{
 uint32_t id=(uint32_t)(uintptr_t)lv_event_get_user_data(event);cJSON *members=cJSON_GetObjectItemCaseSensitive(editing,"members");int at=0;
 for(cJSON *v=members->child;v;v=v->next,at++)if((uint32_t)v->valueint==id){cJSON_DeleteItemFromArray(members,at);goto done;}
 {const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),(int)id));
  if(!action_device_supported(d)){lv_obj_remove_state(lv_event_get_target(event),LV_STATE_CHECKED);return;}
  cJSON *v=cJSON_CreateNumber(id);if(v)cJSON_AddItemToArray(members,v);}
 done:;
 char count[64];snprintf(count,sizeof(count),dsr_sc_tr("Wybrano: %u"),(unsigned)cJSON_GetArraySize(members));lv_label_set_text(action_editor.count,count);if(action_editor.next){if(cJSON_GetArraySize(members))lv_obj_remove_state(action_editor.next,LV_STATE_DISABLED);else lv_obj_add_state(action_editor.next,LV_STATE_DISABLED);}
}
static void action_add_selected(lv_event_t *event)
{
 uint32_t id=(uint32_t)(uintptr_t)lv_event_get_user_data(event);const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),(int)id));
 cJSON *all=cJSON_GetObjectItemCaseSensitive(editing,"actions");if(!action_device_supported(d)||cJSON_GetArraySize(all)>=8)return;
 cJSON *a=cJSON_CreateObject();if(!a)return;
 if(!cJSON_AddNumberToObject(a,"device",id)||!cJSON_AddStringToObject(a,"command",!strcmp(string(d,"kind"),"cover")?"open":"on")){cJSON_Delete(a);return;}
 cJSON_AddItemToArray(all,a);action_editor.selecting=false;action_form();
}
static void action_devices(void)
{
 char count[64];snprintf(count,sizeof(count),dsr_sc_tr("Wybrano: %u"),(unsigned)cJSON_GetArraySize(item(editing,action_editor.kind?"actions":"members")));
 action_editor.count=dsr_sc_text(body,count,2,0,280,&dsr_roboto_16,DSR_SC_TEXT);
 lv_obj_t *search=lv_textarea_create(body);dsr_ui_textarea(search);lv_obj_set_pos(search,0,30);lv_obj_set_size(search,284,44);lv_textarea_set_one_line(search,true);lv_textarea_set_max_length(search,47);lv_textarea_set_placeholder_text(search,dsr_sc_tr("Szukaj"));lv_textarea_set_text(search,action_editor.query);
 lv_obj_add_event_cb(search,focus,LV_EVENT_FOCUSED,NULL);lv_obj_add_event_cb(search,focus,LV_EVENT_CLICKED,NULL);lv_obj_add_event_cb(search,action_query,LV_EVENT_VALUE_CHANGED,NULL);
 int y=86;unsigned shown=0;const cJSON *room,*d;int n=cJSON_GetArraySize(item(draft,"rooms"));
 for(int ri=-1;ri<n;ri++){
  room=ri<0?NULL:cJSON_GetArrayItem(item(draft,"rooms"),ri);uint32_t room_id=(uint32_t)integer(room,"id");unsigned eligible=0;
  cJSON_ArrayForEach(d,item(draft,"devices"))if((uint32_t)integer(d,"room")==room_id&&(action_device_supported(d)||(!action_editor.kind&&action_included((uint32_t)integer(d,"id"))))&&(!*action_editor.query||strstr(display_name(d),action_editor.query)))eligible++;
  if(!eligible)continue;
  bool expanded=action_editor.expanded==room_id||*action_editor.query;char title[96];snprintf(title,sizeof(title),"%s %s",expanded?"-":"+",room?string(room,"name"):dsr_sc_tr("Nieprzypisane"));
  action_pick(dsr_sc_user(title),y,false,action_expand,room_id);y+=56;
  if(!expanded)continue;
  cJSON_ArrayForEach(d,item(draft,"devices")){
   uint32_t id=(uint32_t)integer(d,"id");bool included=action_included(id);
   if((uint32_t)integer(d,"room")!=room_id||(!action_device_supported(d)&&(!included||action_editor.kind))||(*action_editor.query&&!strstr(display_name(d),action_editor.query)))continue;
   if(action_editor.kind)action_pick(dsr_sc_user(display_name(d)),y,false,action_add_selected,id);
   else{lv_obj_t *check=lv_checkbox_create(body);lv_obj_set_pos(check,8,y+8);lv_obj_set_width(check,268);lv_checkbox_set_text(check,dsr_sc_user(display_name(d)));lv_obj_set_style_text_font(check,&dsr_roboto_14,0);lv_obj_set_style_text_color(check,dsr_sc_color(DSR_SC_TEXT),0);if(included)lv_obj_add_state(check,LV_STATE_CHECKED);lv_obj_add_event_cb(check,action_member,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)id);}
   y+=56;shown++;
  }
 }
 if(!shown&&y==86)dsr_sc_text(body,"Brak zgodnych urządzeń. Dodaj je na stronie Urządzenia.",4,y,276,&dsr_roboto_14,DSR_SC_MUTED);
}
static void action_choose_device(lv_event_t *event)
{(void)event;action_editor.selecting=true;action_editor.query[0]=0;action_form();}
static void action_remove_step(lv_event_t *event)
{cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(editing,"actions"),(int)(uintptr_t)lv_event_get_user_data(event));action_form();}
static void action_move_step(lv_event_t *event)
{int at=(int)(uintptr_t)lv_event_get_user_data(event);cJSON *all=cJSON_GetObjectItemCaseSensitive(editing,"actions");if(at>0){cJSON *a=cJSON_DetachItemFromArray(all,at);cJSON_InsertItemInArray(all,at-1,a);}action_form();}
static void action_command(lv_event_t *event)
{
 cJSON *a=lv_event_get_user_data(event);const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),integer(a,"device")));
 const char *switches[]={"on","off"},*covers[]={"open","close","stop"};unsigned at=lv_dropdown_get_selected(lv_event_get_target(event));bool cover=!strcmp(string(d,"kind"),"cover");if(at>=(cover?3u:2u))return;
 cJSON_ReplaceItemInObjectCaseSensitive(a,"command",cJSON_CreateString(cover?covers[at]:switches[at]));
}
static void action_steps(void)
{
 int i=0;const cJSON *a;cJSON_ArrayForEach(a,item(editing,"actions")){
  int y=i*116;const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),integer(a,"device")));bool cover=!strcmp(string(d,"kind"),"cover");
  char title[96];snprintf(title,sizeof(title),"%u. %s",(unsigned)i+1,display_name(d));lv_obj_t *name=dsr_sc_text(body,dsr_sc_user(title),4,y,280,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_height(name,36);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);
  lv_obj_t *choice=lv_dropdown_create(body);lv_obj_set_pos(choice,0,y+40);lv_obj_set_size(choice,156,44);lv_obj_set_style_text_font(choice,&dsr_roboto_14,0);lv_obj_set_style_text_font(lv_dropdown_get_list(choice),&dsr_roboto_14,0);lv_dropdown_set_options(choice,dsr_sc_tr(cover?"Otwórz\nZamknij\nStop":"Włącz\nWyłącz"));const char *cmd=string(a,"command");lv_dropdown_set_selected(choice,!strcmp(cmd,"off")||!strcmp(cmd,"close")?1:!strcmp(cmd,"stop")?2:0);lv_obj_add_event_cb(choice,action_command,LV_EVENT_VALUE_CHANGED,(void *)a);
  lv_obj_t *up=dsr_ui_button(body,"Góra",162,y+40,50,action_move_step,(void *)(uintptr_t)i);if(!i)lv_obj_add_state(up,LV_STATE_DISABLED);
  dsr_ui_button(body,"Usuń",218,y+40,66,action_remove_step,(void *)(uintptr_t)i);i++;
 }
 lv_obj_t *add=dsr_ui_button(body,"+ Działanie",0,i*116,284,action_choose_device,NULL);if(i>=8)lv_obj_add_state(add,LV_STATE_DISABLED);
}
static void action_photo(lv_event_t *event)
{unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);const char *id=i?gallery_id(i-1):"";if(id)action_put("photo",cJSON_CreateString(id));action_form();}
static void action_gallery_page(lv_event_t *event)
{action_editor.gallery_page=(uint8_t)(uintptr_t)lv_event_get_user_data(event);action_form();}
static void action_gallery(void)
{
 add_field("Nazwa","name",0,false);dsr_sc_text(body,"Wybierz zdjęcie",2,80,280,&dsr_roboto_16,DSR_SC_TEXT);
 action_pick("Bez zdjęcia",108,!*string(editing,"photo"),action_photo,0);
 const char *id;unsigned i=(unsigned)action_editor.gallery_page*12,shown=0;
 while(shown<12&&(id=gallery_id(i))){
  lv_obj_t *b=dsr_sc_button(body,(int)(shown%4)*71,174+(int)(shown/4)*72,64,64,DSR_SC_CARD,12,action_photo,(void *)(uintptr_t)(i+1));lv_obj_set_style_clip_corner(b,true,0);
  if(dsr_ui_photo_source(id,&action_editor.photos[shown])){lv_obj_t *image=lv_image_create(b);lv_image_set_src(image,&action_editor.photos[shown]);uint32_t scale=64u*256u/action_editor.photos[shown].header.w,vertical=64u*256u/action_editor.photos[shown].header.h;lv_image_set_scale(image,scale>vertical?scale:vertical);lv_obj_center(image);lv_obj_add_flag(image,LV_OBJ_FLAG_EVENT_BUBBLE);}
  if(!strcmp(id,string(editing,"photo"))){lv_obj_set_style_border_width(b,3,0);lv_obj_set_style_border_color(b,dsr_sc_color(DSR_SC_BLUE),0);dsr_sc_panel(b,44,4,12,12,DSR_SC_BLUE,6);}
  i++;shown++;
 }
 int end=184+(int)((shown+3)/4)*72;
 if(action_editor.gallery_page)dsr_ui_button(body,"Poprzednie zdjęcia",0,end,136,action_gallery_page,(void *)(uintptr_t)(action_editor.gallery_page-1));
 if(gallery_id(i))dsr_ui_button(body,"Następne zdjęcia",148,end,136,action_gallery_page,(void *)(uintptr_t)(action_editor.gallery_page+1));
 dsr_sc_text(body,"Własne zdjęcia dodasz w Web Panelu.",2,end+58,280,&dsr_roboto_12,DSR_SC_MUTED);
 if(!action_editor.fresh)dsr_ui_button(body,"Usuń wpis",0,end+102,284,delete_entity,NULL);
}
static void action_form(void)
{
 lv_obj_t *root=screen(action_editor.kind?(action_editor.fresh?"Nowa scena":"Edytuj scenę"):(action_editor.fresh?"Nowa grupa":"Edytuj grupę"));view_save_target=SAVE_ACTIONS;action_editor.next=NULL;
 lv_obj_set_y(body,138);lv_obj_set_height(body,260);
 for(unsigned i=0;i<4;i++)dsr_sc_panel(root,12+(int)i*76,109,68,4,i==action_editor.step?DSR_SC_BLUE:DSR_SC_EDGE,2);
 lv_obj_add_event_cb(keyboard,action_keyboard_done,LV_EVENT_READY,NULL);lv_obj_add_event_cb(keyboard,action_keyboard_done,LV_EVENT_CANCEL,NULL);
 if(action_editor.deleted){dsr_sc_text(body,"Zapisz, aby usunąć wpis.",4,32,276,&dsr_roboto_18,DSR_SC_TEXT);actions(root,action_back);return;}
 if(action_editor.step==0){
  dsr_sc_text(body,action_editor.kind?"Sceny uruchamiane ręcznie":"Typ grupy",4,0,276,&dsr_roboto_18,DSR_SC_TEXT);
  if(action_editor.kind)dsr_sc_text(body,"Przycisk uruchamia wybrane działania po kolei. Warunki i harmonogramy nie są jeszcze obsługiwane.",4,44,276,&dsr_roboto_16,DSR_SC_MUTED);
  else{action_pick("Przekaźniki i gniazdka",42,true,action_next,0);lv_obj_t *b=action_pick("Światła • wkrótce",102,false,NULL,0);lv_obj_add_state(b,LV_STATE_DISABLED);b=action_pick("Rolety i termostaty • wkrótce",162,false,NULL,0);lv_obj_add_state(b,LV_STATE_DISABLED);dsr_sc_text(body,"Grupa steruje zgodnymi kanałami Switch.",4,224,276,&dsr_roboto_12,DSR_SC_MUTED);}
 }else if(action_editor.step==1){if(!action_editor.kind||action_editor.selecting)action_devices();else action_steps();}
 else if(action_editor.step==2){dsr_sc_text(body,"Wybierz pomieszczenie",2,0,280,&dsr_roboto_18,DSR_SC_TEXT);action_pick("Cały dom",36,integer(editing,"room")==0,action_room,0);const cJSON *room;int y=94;cJSON_ArrayForEach(room,item(draft,"rooms")){action_pick(dsr_sc_user(string(room,"name")),y,integer(editing,"room")==integer(room,"id"),action_room,(uintptr_t)integer(room,"id"));y+=58;}}
 else action_gallery();
 dsr_ui_button(root,"Wstecz",12,420,140,action_back,NULL);
 if(action_editor.step==3){action_editor.next=dsr_ui_button(root,"Zapisz",168,420,140,save,NULL);dsr_ui_primary(action_editor.next);if(!*string(editing,"name")||strlen(string(editing,"name"))>47)lv_obj_add_state(action_editor.next,LV_STATE_DISABLED);}
 else{lv_obj_t *next=action_editor.next=dsr_ui_button(root,"Dalej",168,420,140,action_next,NULL);dsr_ui_primary(next);if(action_editor.selecting||(action_editor.step==1&&!cJSON_GetArraySize(item(editing,action_editor.kind?"actions":"members"))))lv_obj_add_state(next,LV_STATE_DISABLED);}
}
void dsr_ui_edit_action(uint8_t kind,uint32_t id)
{
 dsr_ui_actions_hide();if(!dsr_ui_settings_can_open())return;
 settings_context=appearance_editor=error_return_settings=error_return_rooms=false;
 open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 editor_active=true;action_editor.active=true;action_editor.kind=kind?1:0;action_editor.expanded=UINT32_MAX;action_editor.fresh=!id;
 snprintf(collection_name,sizeof(collection_name),"%s",kind?"scenes":"groups");cJSON *all=cJSON_GetObjectItemCaseSensitive(draft,collection_name);
 if(id){editing=cJSON_GetArrayItem(all,(int)object_index(all,(int)id));if(!editing){action_leave(NULL);return;}}
 else{
  if(cJSON_GetArraySize(all)>=8){lv_obj_t *root=screen(kind?"Sceny":"Grupy");dsr_sc_text(body,"Osiągnięto limit wpisów",4,32,276,&dsr_roboto_18,DSR_SC_TEXT);dsr_ui_button(root,"Wstecz",12,420,296,action_leave,NULL);return;}
  int next=1;while(object_index(all,next)!=255)next++;
  editing=cJSON_CreateObject();if(!editing){config_error(OPEN_OBJECT_MEMORY);return;}
  if(!cJSON_AddNumberToObject(editing,"id",next)||!cJSON_AddStringToObject(editing,"name","")||!cJSON_AddArrayToObject(editing,kind?"actions":"members")||!cJSON_AddStringToObject(editing,"photo",dsr_room_photo(0)->id)){cJSON_Delete(editing);editing=NULL;config_error(OPEN_OBJECT_MEMORY);return;}
  cJSON_AddItemToArray(all,editing);
 }
 action_form();
}
