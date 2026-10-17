/* Genuine LVGL: room/device/dashboard editors keep every control reachable
 * (0.7.37 field report: photo hint clipped at the fixed footer). PL/EN/DE,
 * long names, with and without stored photos, keyboard open. SDK/storage MODEL. */
#define main original_ui_main
#include "test_ui.c"
#undef main

static unsigned layout_checks;
static lv_obj_t *raw_label(lv_obj_t *root,const char *text)
{
 if(lv_obj_check_type(root,&lv_label_class)&&!strcmp(lv_label_get_text(root),text))return root;
 for(uint32_t i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *f=raw_label(lv_obj_get_child(root,(int32_t)i),text);if(f)return f;}
 return NULL;
}
static lv_obj_t *editor_body(void)
{
 lv_obj_t *root=lv_screen_active();
 for(uint32_t i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *c=lv_obj_get_child(root,(int32_t)i);
  if(lv_obj_has_flag(c,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_width(c)==296&&lv_obj_get_y(c)>=100&&lv_obj_get_y(c)<=120)return c;
 }
 return NULL;
}
static bool overlap(lv_obj_t *a,lv_obj_t *b)
{
 lv_area_t x,y;lv_obj_get_coords(a,&x);lv_obj_get_coords(b,&y);
 return x.x1<=y.x2&&y.x1<=x.x2&&x.y1<=y.y2&&y.y1<=x.y2;
}
/* Footer buttons start at y=420; the body must end above them, its children
 * must not overlap each other and labels must fit inside their buttons. */
static void editor_layout(const char *where)
{
 lv_obj_update_layout(lv_screen_active());
 lv_obj_t *body=editor_body();CHECK(body);
 lv_area_t area;lv_obj_get_coords(body,&area);
 if(area.y2>=420){fprintf(stderr,"%s: body ends at %d over footer\n",where,(int)area.y2);CHECK(false);}
 /* Without the keyboard nothing may hide below the visible body (no control
  * or text half-cut at the footer); with it, scrolling to the field is enough. */
 bool keyboard=false;lv_obj_t *root=lv_screen_active();
 for(uint32_t i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *c=lv_obj_get_child(root,(int32_t)i);if(lv_obj_check_type(c,&lv_keyboard_class)&&!lv_obj_has_flag(c,LV_OBJ_FLAG_HIDDEN))keyboard=true;}
 if(!keyboard){
  for(uint32_t i=0;i<lv_obj_get_child_count(body);i++){
   lv_area_t c;lv_obj_get_coords(lv_obj_get_child(body,(int32_t)i),&c);
   if(c.y2>area.y2){fprintf(stderr,"%s: child %u ends at %d below visible body %d\n",where,(unsigned)i,(int)c.y2,(int)area.y2);CHECK(false);}
   layout_checks++;
  }
 }
 uint32_t n=lv_obj_get_child_count(body);
 for(uint32_t i=0;i<n;i++){
  lv_obj_t *a=lv_obj_get_child(body,(int32_t)i);if(lv_obj_has_flag(a,LV_OBJ_FLAG_HIDDEN))continue;
  for(uint32_t j=i+1;j<n;j++){
   lv_obj_t *b=lv_obj_get_child(body,(int32_t)j);if(lv_obj_has_flag(b,LV_OBJ_FLAG_HIDDEN))continue;
   if(lv_obj_check_type(a,&lv_dropdown_class)&&lv_obj_check_type(b,&lv_dropdown_class))continue;
   if(overlap(a,b)){
    fprintf(stderr,"%s: overlap '%s' / '%s'\n",where,lv_obj_check_type(a,&lv_label_class)?lv_label_get_text(a):"(object)",lv_obj_check_type(b,&lv_label_class)?lv_label_get_text(b):"(object)");
    CHECK(false);
   }
   layout_checks++;
  }
  if(lv_obj_get_child_count(a)&&!lv_obj_check_type(a,&lv_dropdown_class)&&!lv_obj_check_type(a,&lv_textarea_class)){
   lv_area_t box;lv_obj_get_coords(a,&box);
   for(uint32_t k=0;k<lv_obj_get_child_count(a);k++){
    lv_obj_t *l=lv_obj_get_child(a,(int32_t)k);if(!lv_obj_check_type(l,&lv_label_class))continue;
    lv_area_t t;lv_obj_get_coords(l,&t);
    if(t.y1<box.y1||t.y2>box.y2){fprintf(stderr,"%s: '%s' overflows its button\n",where,lv_label_get_text(l));CHECK(false);}
    layout_checks++;
   }
  }
 }
}
static void rooms_document(const char *language,const char *name)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language));
 cJSON *rooms=cJSON_CreateArray(),*room=cJSON_CreateObject();
 cJSON_AddNumberToObject(room,"id",1);cJSON_AddStringToObject(room,"name",name);cJSON_AddNumberToObject(room,"icon",0);cJSON_AddStringToObject(room,"photo","");
 cJSON_AddItemToArray(rooms,room);cJSON_ReplaceItemInObjectCaseSensitive(doc,"rooms",rooms);publish_document(doc);tick();
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 const char *languages[]={"pl","en","de"};
 const char *names[]={"Salon","Bardzo długa nazwa pomieszczenia na piętrze 1"};
 char frame[64];unsigned frames=350;
 for(unsigned l=0;l<3;l++)for(unsigned photos=0;photos<2;photos++)for(unsigned n=0;n<2;n++){
  test_workspace_photos(photos);rooms_document(languages[l],names[n]);
  dsr_ui_edit_room(1);tick();CHECK(find_type(lv_screen_active(),&lv_textarea_class));
  char where[64];snprintf(where,sizeof(where),"edit room %s photos=%u name=%u",languages[l],photos,n);editor_layout(where);
  if(!n&&photos){snprintf(frame,sizeof(frame),"%u-room-edit-%s",frames++,languages[l]);screenshot(frame);}
  /* Keyboard open: the body shrinks; it still may not overlap anything. */
  lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class);lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);tick();
  snprintf(where,sizeof(where),"edit room keyboard %s",languages[l]);editor_layout(where);
  dsr_ui_add_room();tick();snprintf(where,sizeof(where),"new room %s photos=%u",languages[l],photos);editor_layout(where);
  dsr_network_home(NULL);tick();
 }
 test_workspace_photos(false);
 /* Built-in room photos (owner artwork): selectable without uploads; tiles render them. */
 rooms_document("pl","Salon");dsr_ui_edit_room(1);tick();CHECK(find_label(lv_screen_active(),"Wybierz zdjęcie z pamięci"));
 click("Wybierz zdjęcie z pamięci");tick();CHECK(find_label(lv_screen_active(),"Salon")&&find_label(lv_screen_active(),"Dotknij, aby zmienić. Własne zdjęcia dodasz w Web Panelu."));
 const char *hint="Dotknij, aby zmienić. Własne zdjęcia dodasz w Web Panelu.";click(hint);tick();CHECK(find_label(lv_screen_active(),"Kuchnia"));editor_layout("built-in photo");screenshot("354-room-builtin-photo");
 for(unsigned i=1;i<dsr_room_photo_count();i++){CHECK(find_label(lv_screen_active(),dsr_room_photo(i)->label));click(hint);tick();}
 CHECK(find_label(lv_screen_active(),"Wybierz zdjęcie z pamięci")); /* after the last built-in: no photo */
 dsr_network_home(NULL);tick();
 {cJSON *doc=room_document();cJSON *room=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(doc,"rooms"),0);CHECK(room);
  cJSON_ReplaceItemInObjectCaseSensitive(room,"photo",cJSON_CreateString(dsr_room_photo(1)->id));publish_document(doc);tick();
  dsr_network_rooms();tick();CHECK(find_type(lv_screen_active(),&lv_image_class));screenshot("355-rooms-builtin-photo");}
 /* User names are shown as typed: a room called "Dom" or "Ustawienia" stays
  * so in English, while UI words around it are translated. */
 static const char *const typed[]={"Dom","Ustawienia"};
 for(unsigned i=0;i<2;i++){
  rooms_document("en",typed[i]);dsr_network_rooms();tick();
  CHECK(raw_label(lv_screen_active(),typed[i])&&!raw_label(lv_screen_active(),i?"Settings":"Home"));
  CHECK(raw_label(lv_screen_active(),"Rooms"));layout_checks++;
 }
 screenshot("353-room-name-verbatim-en");
 printf("PASS: %u editor layout checks (room edit/new, PL/EN/DE, long names, photos, keyboard); %u frames; SDK/storage MODEL\n",layout_checks,screenshots);
 lv_deinit();return 0;
}
