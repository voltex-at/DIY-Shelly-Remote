/* Production LVGL editor layouts; storage/radio are the shared native MODEL. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_cyber_icons.h"

static void shot(unsigned number,const char *name,unsigned language)
{char path[96];snprintf(path,sizeof(path),"%u-cyber-%s-%s",number,name,language==0?"pl":language==1?"en":"de");screenshot(path);}
static lv_area_t box(lv_obj_t *o){lv_area_t a;CHECK(o);lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(o,&a);return a;}
static lv_obj_t *front_label(lv_obj_t *root,const char *text)
{
 for(int i=(int)lv_obj_get_child_count(root)-1;i>=0;i--){lv_obj_t *o=front_label(lv_obj_get_child(root,i),text);if(o)return o;}
 return lv_obj_check_type(root,&lv_label_class)&&!strcmp(lv_label_get_text(root),text)?root:NULL;
}
static void visible_press(const char *text)
{
 lv_obj_t *label=front_label(lv_screen_active(),dsr_sc_tr(text));if(!label)fprintf(stderr,"Missing front label %s\n",text);CHECK(label);lv_obj_t *button=lv_obj_get_parent(label);
 lv_obj_scroll_to_view(button,LV_ANIM_OFF);lv_area_t a=box(button);
 CHECK(a.x1>=0&&a.x2<320&&a.y1>=0&&a.y2<480&&lv_obj_get_height(button)>=44);
 for(lv_obj_t *p=button;p;p=lv_obj_get_parent(p))CHECK(!lv_obj_has_flag(p,LV_OBJ_FLAG_HIDDEN));
 tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}
static lv_obj_t *focus_editor(void)
{
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb&&!lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));
 lv_area_t keys=box(kb),input=box(field),view=box(lv_obj_get_parent(field));
 CHECK(keys.y1==192&&keys.y2==415);
 CHECK(input.y1>=view.y1&&input.y2<=view.y2&&input.y2<keys.y1);
 CHECK(lv_obj_has_state(field,LV_STATE_FOCUSED));
 lv_obj_t *root=lv_screen_active();unsigned footer=0;
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *o=lv_obj_get_child(root,(int)i);if(!lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))continue;
  lv_area_t a=box(o);if(a.y1==420){CHECK(a.y2<480&&a.y1>keys.y2);CHECK(lv_obj_get_height(o)>=44&&!lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN));footer++;}
 }
 CHECK(footer==2);return kb;
}
static void no_overlap(lv_obj_t *a,lv_obj_t *b)
{lv_area_t x=box(a),y=box(b);CHECK(x.y2<y.y1||y.y2<x.y1||x.x2<y.x1||y.x2<x.x1);}
static lv_obj_t *prefix_label(lv_obj_t *root,const char *prefix)
{
 lv_obj_t *parent=lv_obj_get_parent(root);
 if(parent&&lv_obj_get_height(parent)==48&&lv_obj_get_width(parent)==284&&lv_obj_check_type(root,&lv_label_class)&&!strncmp(lv_label_get_text(root),prefix,strlen(prefix)))return root;
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=prefix_label(lv_obj_get_child(root,(int)i),prefix);if(o)return o;}return NULL;
}
static void select_long_device(bool scene)
{
 lv_obj_t *search=find_type(lv_screen_active(),&lv_textarea_class);CHECK(search);lv_textarea_set_text(search,"Pralnia");
 lv_obj_t *kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(kb);lv_obj_send_event(kb,LV_EVENT_READY,NULL);
 lv_obj_update_layout(lv_screen_active());
 lv_obj_t *target=scene?lv_obj_get_parent(prefix_label(lv_screen_active(),"Pralnia")):find_type(lv_screen_active(),&lv_checkbox_class);
 CHECK(target);lv_obj_scroll_to_view(target,LV_ANIM_OFF);lv_area_t a=box(target);CHECK(a.y1>=138&&a.y2<398);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
}

static void fixture(unsigned language)
{
 cJSON *doc=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString("cyber_blue"));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));
 cJSON *rooms=cJSON_CreateArray();
 cJSON_AddItemToArray(rooms,cJSON_Parse("{\"id\":1,\"name\":\"Salon na piętrze\",\"icon\":0,\"photo\":\"\"}"));
 cJSON_AddItemToArray(rooms,cJSON_Parse("{\"id\":2,\"name\":\"Pusty pokój\",\"icon\":1,\"photo\":\"\"}"));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"rooms",rooms);
 cJSON *devices=cJSON_CreateArray();
 cJSON_AddItemToArray(devices,cJSON_Parse("{\"id\":1,\"name\":\"Lampa nad stołem w salonie na piętrze\",\"kind\":\"relay\",\"host\":\"192.168.1.20\",\"channel\":0,\"room\":1}"));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"devices",devices);
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"groups",cJSON_CreateArray());cJSON_ReplaceItemInObjectCaseSensitive(doc,"scenes",cJSON_CreateArray());
 cJSON *dashboard=cJSON_GetObjectItemCaseSensitive(doc,"dashboards")->child;
 cJSON_ReplaceItemInObjectCaseSensitive(dashboard,"name",cJSON_CreateString("Mój pulpit"));
 cJSON *tiles=cJSON_CreateArray();
 cJSON_AddItemToArray(tiles,cJSON_Parse("{\"id\":1,\"type\":\"device\",\"ref\":1,\"span\":2,\"height\":1,\"fields\":255}"));
 cJSON_ReplaceItemInObjectCaseSensitive(dashboard,"tiles",tiles);
 publish_document(doc);dsr_network_home(NULL);tick();CHECK(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE);
}
static void editor_checks(unsigned language)
{
 fixture(language);
 p0_generic("Pulpity","Mój pulpit");shot(630,"dashboard-editor",language);
 char tile_title[128];snprintf(tile_title,sizeof(tile_title),"Lampa nad stołem w salonie na piętrze • %s",dsr_sc_tr("standard"));
 lv_obj_t *title=find_label(lv_screen_active(),tile_title);CHECK(title);
 no_overlap(title,lv_obj_get_parent(find_label(lv_screen_active(),"Obiekt")));
 lv_obj_t *kb=focus_editor();key(kb,"123");key(kb,"ąä");key(kb,"ą");key(kb,"ä");
 CHECK(strstr(lv_textarea_get_text(lv_keyboard_get_textarea(kb)),"ąä"));
 lv_textarea_set_text(lv_keyboard_get_textarea(kb),"Pulpit Łódź Küche");visible_press("Zapisz");dsr_ui_workspace_tick();
 cJSON *doc=room_document();CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"dashboards")->child,"name")->valuestring,"Pulpit Łódź Küche"));cJSON_Delete(doc);
 dsr_ui_workspace_hide();dsr_ui_edit_room(1);shot(631,"room-editor",language);
 kb=focus_editor();shot(632,"room-keyboard",language);lv_textarea_set_text(lv_keyboard_get_textarea(kb),"Nie zapisuj");visible_press("Wstecz");
 CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));CHECK(lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));visible_press("Odrzuć");
 dsr_ui_edit_room(1);CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Salon na piętrze"));
 lv_obj_t *choice=find_type(lv_screen_active(),&lv_dropdown_class);CHECK(choice);lv_dropdown_set_selected(choice,2);lv_obj_send_event(choice,LV_EVENT_VALUE_CHANGED,NULL);
 visible_press("Wybierz zdjęcie z pamięci");kb=focus_editor();lv_textarea_set_text(lv_keyboard_get_textarea(kb),"Salon Łódź Küche");
 visible_press("Zapisz");dsr_ui_workspace_tick();doc=room_document();cJSON *room=cJSON_GetObjectItemCaseSensitive(doc,"rooms")->child;
 CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(room,"name")->valuestring,"Salon Łódź Küche"));CHECK(cJSON_GetObjectItemCaseSensitive(room,"icon")->valueint==2);CHECK(strlen(cJSON_GetObjectItemCaseSensitive(room,"photo")->valuestring)==32);cJSON_Delete(doc);
 dsr_ui_edit_room(1);shot(634,"room-photo",language);room_delete_button();CHECK(find_label(lv_screen_active(),"Pomieszczenie jest używane"));visible_press("Wstecz");visible_press("Wstecz");
 dsr_ui_edit_room(2);kb=focus_editor();room_delete_button();CHECK(find_label(lv_screen_active(),"Usunąć pomieszczenie?"));shot(635,"room-delete-confirm",language);visible_press("Anuluj");
 room_delete_button();visible_press("Usuń");doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"rooms"))==2);cJSON_Delete(doc);visible_press("Zapisz");dsr_ui_workspace_tick();
 doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"rooms"))==1);cJSON_Delete(doc);
 dsr_ui_edit_local_device(1);kb=focus_editor();lv_textarea_set_text(lv_keyboard_get_textarea(kb),"Pralnia Łódź Küche — bardzo długa nazwa");shot(636,"device-name-keyboard",language);visible_press("Zapisz");dsr_ui_workspace_tick();
 doc=room_document();CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(doc,"devices")->child,"name")->valuestring,"Pralnia Łódź Küche — bardzo długa nazwa"));cJSON_Delete(doc);
 dsr_ui_edit_action(1,0);kb=focus_editor();lv_textarea_set_text(lv_keyboard_get_textarea(kb),"Wieczór Küche");shot(637,"scene-keyboard",language);visible_press("Dalej");CHECK(find_label(lv_screen_active(),"2/3 • Polecenia"));
 visible_press("+ Dodaj polecenie");select_long_device(true);visible_press("Włącz");visible_press("Dalej");shot(651,"scene-review",language);visible_press("Zapisz scenę");dsr_ui_workspace_tick();
 doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"scenes"))==1);cJSON_Delete(doc);
 dsr_ui_edit_action(0,0);kb=focus_editor();lv_textarea_set_text(lv_keyboard_get_textarea(kb),"Lampy Küche");visible_press("Dalej");select_long_device(false);visible_press("Dalej");shot(652,"group-review",language);visible_press("Zapisz grupę");dsr_ui_workspace_tick();
 doc=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(doc,"groups"))==1);cJSON_Delete(doc);
 dsr_network_home(NULL);dsr_network_device(NULL);choose_method("Dodaj ręcznie (IP)");
 lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field);lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);kb=find_type(lv_screen_active(),&lv_keyboard_class);
 lv_area_t keys=box(kb),input=box(field);CHECK(keys.y1==192&&keys.y2==415&&input.y2<keys.y1);shot(638,"onboarding-keyboard",language);
 lv_obj_t *back=lv_obj_get_parent(find_label(lv_screen_active(),"Anuluj"));lv_area_t a=box(back);CHECK(a.y1==420&&a.y2<480);visible_press("Anuluj");
 dsr_network_home(NULL);
}
static void diagnostics(unsigned language)
{
 fixture(language);dsr_ui_battery_open(NULL);
 dsr_event_t event={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3800}};dsr_ui_battery_result(&event);
 CHECK(find_label(lv_screen_active(),"3.800 V"));lv_obj_t *memory=find_label(lv_screen_active(),"Pamięć i radio");CHECK(memory);
 lv_obj_t *body=lv_obj_get_parent(lv_obj_get_parent(memory));lv_obj_scroll_to_y(body,1000,LV_ANIM_OFF);lv_obj_update_layout(body);
 const char *names[]={"RAM","PSRAM","Grafika","Stos ekranu","BLE"};
 for(unsigned i=0;i<5;i++){lv_obj_t *name=find_label(lv_screen_active(),names[i]);CHECK(name&&lv_obj_get_style_text_font(name,0)==&dsr_roboto_14);}
 CHECK(find_label(lv_screen_active(),"136 KB")&&find_label(lv_screen_active(),"7812 KB")&&find_label(lv_screen_active(),"2304 B"));shot(633,"memory-rows",language);
 lv_obj_scroll_to_y(body,0,LV_ANIM_OFF);
 const dsr_event_t states[]={{.result=DSR_IO},{.result=DSR_OK,.battery={.has_voltage=false}},{.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3800}}};
 const char *status[]={"Pomiar niedostępny. Spróbuj ponownie.","Kalibracja ADC niedostępna. Napięcie nieznane.","Napięcie BAT • dzielnik nominalny 2:1"};
 for(unsigned i=0;i<3;i++){dsr_ui_battery_result(&states[i]);lv_obj_t *s=find_label(lv_screen_active(),status[i]);CHECK(s);CHECK(lv_obj_get_height(s)<=48);}
 lv_tick_inc(31000);dsr_ui_battery_tick();CHECK(find_label(lv_screen_active(),"Pomiar nieaktualny. Zmierz ponownie."));shot(639,"battery-stale",language);
 dsr_ui_open_settings();
}
static void icon_gallery(void)
{
 const dsr_ui_icon_t roles[]={DSR_ICON_SOFA,DSR_ICON_BED,DSR_ICON_KITCHEN,DSR_ICON_BATH,DSR_ICON_FAN,DSR_ICON_BOILER,DSR_ICON_GARAGE,DSR_ICON_FLOOR_LAMP,DSR_ICON_WALL_LAMP,DSR_ICON_EV,DSR_ICON_KETTLE,DSR_ICON_COFFEE,DSR_ICON_LAUNDRY,DSR_ICON_SPEAKER,DSR_ICON_DOOR_CLOSED,DSR_ICON_DOOR_OPEN,DSR_ICON_VALVE};
 const char *names[]={"sofa","bed","kitchen","bathroom","fan","boiler","garage","floor-lamp","wall-lamp","ev-charger","kettle","coffee","washer","speaker","closed","open","valve"};
 lv_obj_t *root=dsr_ui_screen("Cyber Blue • 17 ikon",false);
 for(unsigned i=0;i<17;i++){
  for(unsigned n=24;n<=48;n=n==24?32:48){const lv_image_dsc_t *d=dsr_ui_cyber_icon(roles[i],n);CHECK(d&&d->header.cf==LV_COLOR_FORMAT_A8&&d->header.w==n&&d->data_size==n*n);if(n==48)break;}
  int x=12+(int)(i%4)*76,y=88+(int)(i/4)*64;
  lv_obj_t *card=dsr_sc_panel(root,x,y,68,58,DSR_SC_CARD,8);dsr_sc_icon(card,roles[i],18,6,32,DSR_SC_BLUE);
  lv_obj_t *caption=dsr_sc_text(card,names[i],0,42,68,&dsr_roboto_11,DSR_SC_MUTED);lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
 }
 screenshot("650-cyber-supplementary-icons");
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);
 lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();fixture(0);
 for(unsigned language=0;language<3;language++){editor_checks(language);diagnostics(language);}
 fixture(0);icon_gallery();
 /* Warm the exact open/focus/close sequence before checking retained memory. */
 for(unsigned i=0;i<3;i++){dsr_ui_edit_room(1);focus_editor();dsr_network_home(NULL);}
 lv_refr_now(display);lv_mem_monitor_t before,after;lv_mem_monitor(&before);
 for(unsigned i=0;i<60;i++){dsr_ui_edit_room(1);focus_editor();dsr_network_home(NULL);dsr_ui_battery_open(NULL);dsr_network_home(NULL);}
 lv_refr_now(display);lv_mem_monitor(&after);CHECK(after.free_size>=before.free_size);CHECK(after.free_biggest_size>32000);
 printf("Cyber editors: 60 room/keyboard + battery open/close cycles; LVGL free %lu -> %lu; largest %lu; host MODEL\n",(unsigned long)before.free_size,(unsigned long)after.free_size,(unsigned long)after.free_biggest_size);
 CHECK(!job_counts[DSR_JOB_SET_OUTPUT]&&!job_counts[DSR_JOB_COVER]&&!restart_requests);
 printf("PASS: %u Cyber editor checks; %u native LVGL frames; storage/radio MODEL\n",checks,screenshots);
 lv_deinit();return 0;
}
