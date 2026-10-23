/* Local scene/group drafts use the unchanged workspace CAS and durable ACK. */
static void action_form(void);
static void action_leave(lv_event_t *event)
{(void)event;uint8_t kind=action_editor.kind;dsr_ui_workspace_hide();dsr_ui_actions_open(kind);}
static bool action_put(const char *key,cJSON *value)
{if(!value)return false;if(item(editing,key))return cJSON_ReplaceItemInObjectCaseSensitive(editing,key,value);return cJSON_AddItemToObject(editing,key,value);}
static bool action_named(void)
{return action_name_valid(string(editing,"name"));}
static lv_obj_t *action_pick(const char *label,int y,bool selected,lv_event_cb_t callback,uintptr_t value)
{
 lv_obj_t *b=dsr_ui_button(body,label,0,y,284,callback,(void *)value);lv_obj_set_style_bg_color(b,dsr_sc_color(DSR_SC_CARD),0);lv_obj_set_style_radius(b,12,0);dsr_sc_border_width(b,selected?2:1,0);dsr_sc_border_color(b,dsr_sc_color(selected?DSR_SC_BLUE:DSR_SC_EDGE),0);
 lv_obj_t *label_obj=lv_obj_get_child(b,0);lv_obj_set_width(label_obj,264);lv_obj_set_height(label_obj,22);lv_label_set_long_mode(label_obj,LV_LABEL_LONG_MODE_DOTS);return b;
}
static void action_back(lv_event_t *event)
{
 (void)event;if(own_save){config_status("Poczekaj na zakończenie zapisu.",true);return;}
 if(action_editor.pending_device){action_editor.pending_device=0;if(action_editor.edit_index)action_editor.selecting=false;action_editor.edit_index=0;action_form();return;}
 if(action_editor.selecting){action_editor.selecting=false;action_form();return;}
 if(action_editor.step==3){action_editor.step=action_editor.return_step;action_form();return;}
 if(!action_editor.deleted&&action_editor.step){action_editor.step--;action_form();return;}
 if(!room_dirty()){action_leave(NULL);return;}
 lv_obj_t *panel=room_modal("Odrzucić zmiany?","Niezapisane zmiany tego formularza zostaną utracone.");dsr_ui_button(panel,"Zostań",12,188,130,room_dialog_close,NULL);dsr_ui_button(panel,"Odrzuć",154,188,130,action_leave,NULL);
}
static void action_next(lv_event_t *event)
{(void)event;if(action_editor.step==0&&!action_named())return;if(action_editor.step==1&&!cJSON_GetArraySize(item(editing,action_editor.kind?"actions":"members")))return;if(action_editor.step<2){action_editor.step++;action_editor.query[0]=0;action_form();}}
static void action_details(lv_event_t *event)
{(void)event;action_editor.return_step=action_editor.step;action_editor.step=3;action_form();}
static void action_edit_name(lv_event_t *event)
{(void)event;action_editor.step=0;action_form();}
static void action_room(lv_event_t *event)
{action_put("room",cJSON_CreateNumber(lv_dropdown_get_selected(lv_event_get_target(event))?integer(cJSON_GetArrayItem(item(draft,"rooms"),(int)lv_dropdown_get_selected(lv_event_get_target(event))-1),"id"):0));}
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
 {const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),(int)id));if(!action_device_supported(d)){lv_obj_remove_state(lv_event_get_target(event),LV_STATE_CHECKED);return;}cJSON *v=cJSON_CreateNumber(id);if(v)cJSON_AddItemToArray(members,v);}
 done:;
 char count[64];snprintf(count,sizeof(count),dsr_sc_tr("Wybrano: %u"),(unsigned)cJSON_GetArraySize(members));lv_label_set_text(action_editor.count,count);if(action_editor.next){if(cJSON_GetArraySize(members))lv_obj_remove_state(action_editor.next,LV_STATE_DISABLED);else lv_obj_add_state(action_editor.next,LV_STATE_DISABLED);}
}
static void action_select_device(lv_event_t *event)
{action_editor.pending_device=(uint32_t)(uintptr_t)lv_event_get_user_data(event);action_form();}
static const char *action_command_name(const char *cmd)
{return !strcmp(cmd,"on")?"Włącz":!strcmp(cmd,"off")?"Wyłącz":!strcmp(cmd,"open")?"Otwórz":!strcmp(cmd,"close")?"Zamknij":"Stop";}
static void action_set_command(lv_event_t *event)
{
 const char *cmd=lv_event_get_user_data(event);const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),(int)action_editor.pending_device));bool cover=!strcmp(string(d,"kind"),"cover");
 if(!action_device_supported(d)||(cover?strcmp(cmd,"open")&&strcmp(cmd,"close")&&strcmp(cmd,"stop"):strcmp(cmd,"on")&&strcmp(cmd,"off")))return;
 cJSON *all=cJSON_GetObjectItemCaseSensitive(editing,"actions"),*a=cJSON_CreateObject();if(!a)return;
 if(!cJSON_AddNumberToObject(a,"device",action_editor.pending_device)||!cJSON_AddStringToObject(a,"command",cmd)){cJSON_Delete(a);return;}
 if(action_editor.edit_index){if(!cJSON_ReplaceItemInArray(all,action_editor.edit_index-1,a)){cJSON_Delete(a);return;}}
 else if(cJSON_GetArraySize(all)>=8||!cJSON_AddItemToArray(all,a)){cJSON_Delete(a);return;}
 action_editor.pending_device=0;action_editor.edit_index=0;action_editor.selecting=false;action_form();
}
static void action_devices(void)
{
 char count[64];if(action_editor.kind)snprintf(count,sizeof(count),"%s",dsr_sc_tr("Wybierz urządzenie"));else snprintf(count,sizeof(count),dsr_sc_tr("Wybrano: %u"),(unsigned)cJSON_GetArraySize(item(editing,"members")));action_editor.count=dsr_sc_text(body,count,2,0,280,&dsr_roboto_16,DSR_SC_TEXT);
 lv_obj_t *search=lv_textarea_create(body);dsr_ui_textarea(search);lv_obj_set_pos(search,0,30);lv_obj_set_size(search,284,44);lv_textarea_set_one_line(search,true);lv_textarea_set_max_length(search,47);lv_textarea_set_placeholder_text(search,dsr_sc_tr("Szukaj"));lv_textarea_set_text(search,action_editor.query);lv_obj_add_event_cb(search,focus,LV_EVENT_FOCUSED,NULL);lv_obj_add_event_cb(search,focus,LV_EVENT_CLICKED,NULL);lv_obj_add_event_cb(search,action_query,LV_EVENT_VALUE_CHANGED,NULL);
 int y=86;unsigned shown=0;const cJSON *room,*d;int n=cJSON_GetArraySize(item(draft,"rooms"));
 for(int ri=-1;ri<n;ri++){
  room=ri<0?NULL:cJSON_GetArrayItem(item(draft,"rooms"),ri);uint32_t room_id=(uint32_t)integer(room,"id");bool heading=false;
  cJSON_ArrayForEach(d,item(draft,"devices")){
   uint32_t id=(uint32_t)integer(d,"id");bool included=!action_editor.kind&&action_included(id);
   if((uint32_t)integer(d,"room")!=room_id||(!action_device_supported(d)&&!included)||!dsr_ui_search_match(display_name(d),action_editor.query))continue;
   if(!heading){dsr_sc_text(body,room?dsr_sc_user(string(room,"name")):"Nieprzypisane",6,y,272,&dsr_roboto_12,DSR_SC_MUTED);heading=true;y+=26;}
   if(action_editor.kind)action_pick(dsr_sc_user(display_name(d)),y,false,action_select_device,id);
   else{lv_obj_t *check=lv_checkbox_create(body);lv_obj_set_pos(check,0,y);lv_obj_set_size(check,284,64);lv_checkbox_set_text(check,dsr_sc_user(display_name(d)));lv_obj_set_style_text_font(check,&dsr_roboto_14,0);lv_obj_set_style_text_color(check,dsr_sc_color(DSR_SC_TEXT),0);lv_obj_set_style_bg_color(check,dsr_sc_color(DSR_SC_CARD),0);lv_obj_set_style_bg_opa(check,LV_OPA_COVER,0);lv_obj_set_style_radius(check,10,0);lv_obj_set_style_pad_all(check,12,0);if(included)lv_obj_add_state(check,LV_STATE_CHECKED);lv_obj_t *label=lv_obj_get_child(check,0);if(label){lv_obj_set_width(label,230);lv_obj_set_height(label,22);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);}lv_obj_add_event_cb(check,action_member,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)id);}
   y+=action_editor.kind?56:72;shown++;
  }
 }
 if(!shown)dsr_sc_text(body,*action_editor.query?"Brak wyników. Zmień wyszukiwanie.":"Brak zgodnych urządzeń. Dodaj je na stronie Urządzenia.",4,y,276,&dsr_roboto_14,DSR_SC_MUTED);
}
static void action_choose_device(lv_event_t *event)
{(void)event;action_editor.selecting=true;action_editor.pending_device=0;action_editor.edit_index=0;action_editor.query[0]=0;action_form();}
static void action_change_device(lv_event_t *event)
{(void)event;action_editor.pending_device=0;action_editor.query[0]=0;action_form();}
static void action_edit_step(lv_event_t *event)
{unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);const cJSON *a=cJSON_GetArrayItem(item(editing,"actions"),(int)i);action_editor.edit_index=(uint8_t)(i+1);action_editor.pending_device=(uint32_t)integer(a,"device");action_editor.selecting=true;action_form();}
static void action_remove_step(lv_event_t *event)
{cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(editing,"actions"),(int)(uintptr_t)lv_event_get_user_data(event));action_form();}
static void action_move_step(lv_event_t *event)
{unsigned value=(unsigned)(uintptr_t)lv_event_get_user_data(event);int at=(int)(value>>1),to=at+((value&1)?1:-1);cJSON *all=cJSON_GetObjectItemCaseSensitive(editing,"actions");if(to>=0&&to<cJSON_GetArraySize(all)){cJSON *a=cJSON_DetachItemFromArray(all,at);cJSON_InsertItemInArray(all,to,a);}action_form();}
static lv_obj_t *action_order(int x,int y,dsr_ui_icon_t icon,lv_event_cb_t callback,uintptr_t data)
{lv_obj_t *b=dsr_sc_button(body,x,y,44,44,DSR_SC_CARD,10,callback,(void *)data);dsr_sc_icon_centered(b,icon,22,dsr_sc_icon_accent());return b;}
static void action_steps(void)
{
 int i=0,total=cJSON_GetArraySize(item(editing,"actions"));const cJSON *a;
 if(!total){dsr_sc_text(body,"Co ma zrobić scena?",4,8,276,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(body,"Dodaj urządzenie i wybierz dla niego polecenie.",4,46,276,&dsr_roboto_14,DSR_SC_MUTED);}
 cJSON_ArrayForEach(a,item(editing,"actions")){
  int y=i*120;const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),integer(a,"device")));char title[96];snprintf(title,sizeof(title),"%u. %s",(unsigned)i+1,display_name(d));lv_obj_t *name=dsr_sc_text(body,dsr_sc_user(title),4,y,280,&dsr_roboto_16,DSR_SC_TEXT);lv_obj_set_height(name,40);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);
  lv_obj_t *cmd=dsr_ui_button(body,action_command_name(string(a,"command")),0,y+48,110,action_edit_step,(void *)(uintptr_t)i);lv_obj_set_height(cmd,44);
  lv_obj_t *up=action_order(118,y+48,DSR_ICON_UP,action_move_step,(uintptr_t)i*2);if(!i)lv_obj_add_state(up,LV_STATE_DISABLED);
  lv_obj_t *down=action_order(170,y+48,DSR_ICON_DOWN,action_move_step,(uintptr_t)i*2+1);if(i==total-1)lv_obj_add_state(down,LV_STATE_DISABLED);
  lv_obj_t *remove=dsr_ui_button(body,"Usuń",222,y+48,62,action_remove_step,(void *)(uintptr_t)i);lv_obj_set_height(remove,44);lv_obj_t *label=lv_obj_get_child(remove,0);lv_obj_set_style_text_font(label,&dsr_roboto_12,0);lv_obj_set_width(label,58);lv_obj_center(label);
  i++;
 }
 lv_obj_t *add=action_pick("+ Dodaj polecenie",i?i*120:108,false,action_choose_device,0);if(i>=8)lv_obj_add_state(add,LV_STATE_DISABLED);
}
static void action_commands(void)
{
 const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),(int)action_editor.pending_device));lv_obj_t *name=dsr_sc_text(body,dsr_sc_user(display_name(d)),4,0,276,&dsr_roboto_18,DSR_SC_TEXT);lv_obj_set_height(name,44);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);dsr_sc_text(body,"Wybierz polecenie",4,50,276,&dsr_roboto_14,DSR_SC_MUTED);
 if(!action_device_supported(d)){dsr_sc_text(body,"Nieobsługiwany kanał",4,82,276,&dsr_roboto_14,DSR_SC_MUTED);action_pick("Zmień urządzenie",140,false,action_change_device,0);return;}
 bool cover=!strcmp(string(d,"kind"),"cover");const char *commands[]={cover?"open":"on",cover?"close":"off","stop"};for(unsigned i=0;i<(cover?3u:2u);i++){lv_obj_t *b=action_pick(action_command_name(commands[i]),78+(int)i*56,false,action_set_command,(uintptr_t)commands[i]);dsr_sc_border_color(b,dsr_sc_color(DSR_SC_BLUE),0);}action_pick("Zmień urządzenie",cover?252:196,false,action_change_device,0);
}
static void action_photo(lv_event_t *event)
{unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);const char *id=i?gallery_id(i-1):"";if(id)action_put("photo",cJSON_CreateString(id));action_form();}
static void action_gallery_page(lv_event_t *event)
{action_editor.gallery_page=(uint8_t)(uintptr_t)lv_event_get_user_data(event);action_form();}
static void action_gallery(void)
{
 dsr_sc_text(body,"Pomieszczenie (opcjonalnie)",2,0,280,&dsr_roboto_14,DSR_SC_MUTED);char rooms[512];snprintf(rooms,sizeof(rooms),"%s",dsr_sc_tr("Cały dom"));unsigned selected=0,i=0;const cJSON *room;cJSON_ArrayForEach(room,item(draft,"rooms")){i++;strncat(rooms,"\n",sizeof(rooms)-strlen(rooms)-1);strncat(rooms,string(room,"name"),sizeof(rooms)-strlen(rooms)-1);if(integer(room,"id")==integer(editing,"room"))selected=i;}
 lv_obj_t *choice=lv_dropdown_create(body);lv_obj_set_pos(choice,0,24);lv_obj_set_size(choice,284,44);lv_obj_set_style_text_font(choice,&dsr_roboto_14,0);lv_obj_set_style_text_font(lv_dropdown_get_list(choice),&dsr_roboto_14,0);lv_dropdown_set_options(choice,rooms);lv_dropdown_set_selected(choice,selected);lv_obj_add_event_cb(choice,action_room,LV_EVENT_VALUE_CHANGED,NULL);
 dsr_sc_text(body,"Zdjęcie (opcjonalnie)",2,88,280,&dsr_roboto_16,DSR_SC_TEXT);action_pick("Bez zdjęcia",116,!*string(editing,"photo"),action_photo,0);
 const char *id;i=(unsigned)action_editor.gallery_page*12;unsigned shown=0;
 while(shown<12&&(id=gallery_id(i))){lv_obj_t *b=dsr_sc_button(body,(int)(shown%4)*71,180+(int)(shown/4)*72,64,64,DSR_SC_CARD,12,action_photo,(void *)(uintptr_t)(i+1));lv_obj_set_style_clip_corner(b,true,0);if(dsr_ui_photo_source(id,&action_editor.photos[shown])){lv_obj_t *image=lv_image_create(b);lv_image_set_src(image,&action_editor.photos[shown]);uint32_t scale=64u*256u/action_editor.photos[shown].header.w,vertical=64u*256u/action_editor.photos[shown].header.h;lv_image_set_scale(image,scale>vertical?scale:vertical);lv_obj_center(image);lv_obj_add_flag(image,LV_OBJ_FLAG_EVENT_BUBBLE);}if(!strcmp(id,string(editing,"photo"))){dsr_sc_border_width(b,3,0);dsr_sc_border_color(b,dsr_sc_color(DSR_SC_BLUE),0);}i++;shown++;}
 int end=190+(int)((shown+3)/4)*72;if(action_editor.gallery_page)dsr_ui_button(body,"Poprzednie zdjęcia",0,end,136,action_gallery_page,(void *)(uintptr_t)(action_editor.gallery_page-1));if(gallery_id(i))dsr_ui_button(body,"Następne zdjęcia",148,end,136,action_gallery_page,(void *)(uintptr_t)(action_editor.gallery_page+1));dsr_sc_text(body,"Własne zdjęcia dodasz w Web Panelu.",2,end+58,280,&dsr_roboto_12,DSR_SC_MUTED);if(!action_editor.fresh)dsr_ui_button(body,"Usuń wpis",0,end+102,284,delete_entity,NULL);
}
static void action_summary(void)
{
 lv_obj_t *name=dsr_sc_text(body,dsr_sc_user(string(editing,"name")),4,0,276,&dsr_roboto_18,DSR_SC_TEXT);lv_obj_set_height(name,44);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);action_pick("Zmień nazwę i opcje",52,false,action_edit_name,0);
 const cJSON *room=cJSON_GetArrayItem(item(draft,"rooms"),(int)object_index(item(draft,"rooms"),integer(editing,"room")));dsr_sc_text(body,room?dsr_sc_user(string(room,"name")):"Cały dom",4,112,276,&dsr_roboto_12,DSR_SC_MUTED);
 int y=144;unsigned order=0;const cJSON *v;cJSON_ArrayForEach(v,item(editing,action_editor.kind?"actions":"members")){unsigned id=action_editor.kind?(unsigned)integer(v,"device"):(unsigned)v->valueint;const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),(int)id));char title[96];if(action_editor.kind)snprintf(title,sizeof(title),"%u. %s",++order,display_name(d));else snprintf(title,sizeof(title),"%s",display_name(d));lv_obj_t *label=dsr_sc_text(body,dsr_sc_user(title),4,y,276,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_height(label,22);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);if(action_editor.kind)dsr_sc_text(body,action_command_name(string(v,"command")),12,y+24,264,&dsr_roboto_14,DSR_SC_BLUE);y+=action_editor.kind?62:34;}
}
static void action_form(void)
{
 lv_obj_t *root=screen(action_editor.kind?(action_editor.fresh?"Nowa scena":"Edytuj scenę"):(action_editor.fresh?"Nowa grupa":"Edytuj grupę"));view_save_target=SAVE_ACTIONS;action_editor.next=action_editor.back=NULL;lv_obj_set_y(body,138);lv_obj_set_height(body,260);
 unsigned progress=action_editor.step==3?action_editor.return_step:action_editor.step;for(unsigned i=0;i<3;i++)dsr_sc_panel(root,12+(int)i*102,118,94,4,i<=progress?DSR_SC_BLUE:DSR_SC_EDGE,2);
 const char *title=action_editor.step==3?"Opcjonalne ustawienia":action_editor.step==0?"1/3 • Nazwa":action_editor.step==1?(action_editor.kind?"2/3 • Polecenia":"2/3 • Urządzenia"):"3/3 • Podsumowanie";config_status(title,false);
 lv_obj_add_event_cb(keyboard,action_keyboard_done,LV_EVENT_READY,NULL);lv_obj_add_event_cb(keyboard,action_keyboard_done,LV_EVENT_CANCEL,NULL);
 if(action_editor.deleted){dsr_sc_text(body,"Zapisz, aby usunąć wpis.",4,32,276,&dsr_roboto_18,DSR_SC_TEXT);actions(root,action_back);return;}
 if(action_editor.step==0){add_field(action_editor.kind?"Nazwa sceny":"Nazwa grupy","name",0,false);lv_textarea_set_placeholder_text(lv_obj_get_child(body,-1),dsr_sc_tr(action_editor.kind?"np. Dobranoc":"np. Lampy na parterze"));dsr_sc_text(body,action_editor.kind?"Jedno dotknięcie uruchomi wybrane polecenia.":"Wspólne włączanie i wyłączanie wybranych przekaźników i gniazdek.",4,82,276,&dsr_roboto_14,DSR_SC_MUTED);action_pick("Pokój i zdjęcie (opcjonalnie)",164,false,action_details,0);}
 else if(action_editor.step==1){if(action_editor.pending_device)action_commands();else if(!action_editor.kind||action_editor.selecting)action_devices();else action_steps();}
 else if(action_editor.step==2)action_summary();else action_gallery();
 action_editor.back=dsr_ui_button(root,action_editor.step==0?"Anuluj":"Wstecz",12,420,140,action_back,NULL);
 if(action_editor.step==3)action_editor.next=dsr_ui_button(root,"Gotowe",168,420,140,action_back,NULL);
 else if(action_editor.step==2){action_editor.next=dsr_ui_button(root,action_editor.kind?"Zapisz scenę":"Zapisz grupę",168,420,140,save,NULL);if(!action_named()||!cJSON_GetArraySize(item(editing,action_editor.kind?"actions":"members")))lv_obj_add_state(action_editor.next,LV_STATE_DISABLED);}
 else{action_editor.next=dsr_ui_button(root,"Dalej",168,420,140,action_next,NULL);if((action_editor.step==0&&!action_named())||action_editor.selecting||(action_editor.step==1&&!cJSON_GetArraySize(item(editing,action_editor.kind?"actions":"members"))))lv_obj_add_state(action_editor.next,LV_STATE_DISABLED);}
 dsr_ui_primary(action_editor.next);
}
void dsr_ui_edit_action(uint8_t kind,uint32_t id)
{
 dsr_ui_actions_hide();if(!dsr_ui_settings_can_open())return;settings_context=appearance_editor=error_return_settings=error_return_rooms=false;open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 editor_active=true;action_editor.active=true;action_editor.kind=kind?1:0;action_editor.fresh=!id;snprintf(collection_name,sizeof(collection_name),"%s",kind?"scenes":"groups");cJSON *all=cJSON_GetObjectItemCaseSensitive(draft,collection_name);
 if(id){editing=cJSON_GetArrayItem(all,(int)object_index(all,(int)id));if(!editing){action_leave(NULL);return;}}
 else{if(cJSON_GetArraySize(all)>=8){screen(kind?"Sceny":"Grupy");dsr_sc_text(body,"Osiągnięto limit wpisów",4,32,276,&dsr_roboto_18,DSR_SC_TEXT);dsr_ui_button(lv_screen_active(),"Wstecz",12,420,296,action_leave,NULL);return;}int next=1;while(object_index(all,next)!=255)next++;editing=cJSON_CreateObject();if(!editing){config_error(OPEN_OBJECT_MEMORY);return;}if(!cJSON_AddNumberToObject(editing,"id",next)||!cJSON_AddStringToObject(editing,"name","")||!cJSON_AddArrayToObject(editing,kind?"actions":"members")||!cJSON_AddStringToObject(editing,"photo","")){cJSON_Delete(editing);editing=NULL;config_error(OPEN_OBJECT_MEMORY);return;}cJSON_AddItemToArray(all,editing);}
 action_form();
}
