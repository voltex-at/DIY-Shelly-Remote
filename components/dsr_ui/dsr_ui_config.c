#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const cJSON *item(const cJSON *o,const char *k){return cJSON_GetObjectItemCaseSensitive(o,k);}
static int integer(const cJSON *o,const char *k){const cJSON *v=item(o,k);return cJSON_IsNumber(v)?v->valueint:0;}
static const char *string(const cJSON *o,const char *k){const cJSON *v=item(o,k);return cJSON_IsString(v)?v->valuestring:"";}
static unsigned object_index(const cJSON *array,int id)
{unsigned i=0;const cJSON *v;cJSON_ArrayForEach(v,array){if(integer(v,"id")==id)return i;i++;}return 255;}
bool dsr_mobile_configure(dsr_mobile_model_t *m,const char *json)
{
 if(!json||!dsr_config_json_depth(json))return false;
 cJSON *r=cJSON_Parse(json);char error[96];
 if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);return false;}
 m->room_count=0;m->device_count=0;m->tile_count=0;m->configured_layout=true;m->accent=(uint32_t)integer(r,"accent");m->midnight=!strcmp(string(r,"theme"),"midnight");
 const cJSON *v;cJSON_ArrayForEach(v,item(r,"rooms")){
  dsr_mobile_room_t *room=&m->rooms[m->room_count++];snprintf(room->name,sizeof(room->name),"%.31s",string(v,"name"));room->icon=(uint8_t)integer(v,"icon");snprintf(room->photo,sizeof(room->photo),"%.32s",string(v,"photo"));
 }
 cJSON_ArrayForEach(v,item(r,"devices")){
  dsr_mobile_device_t *d=&m->devices[m->device_count++];memset(d,0,sizeof(*d));d->id=(uint32_t)integer(v,"id");snprintf(d->name,sizeof(d->name),"%s",string(v,"name"));snprintf(d->host,sizeof(d->host),"%s",string(v,"host"));d->channel=(uint8_t)integer(v,"channel");d->room=(uint8_t)object_index(item(r,"rooms"),integer(v,"room"));
  const char *kind=string(v,"kind");d->kind=!strcmp(kind,"light")?DSR_DEVICE_LIGHT:!strcmp(kind,"cover")?DSR_DEVICE_COVER:!strcmp(kind,"contact")?DSR_DEVICE_CONTACT:!strcmp(kind,"sensor")?DSR_DEVICE_SENSOR:!strcmp(kind,"plug")?DSR_DEVICE_PLUG:DSR_DEVICE_RELAY;d->fields=255;
 }
 const cJSON *dashboard=cJSON_GetArrayItem(item(r,"dashboards"),(int)object_index(item(r,"dashboards"),integer(r,"active_dashboard")));
 snprintf(m->dashboard_name,sizeof(m->dashboard_name),"%s",string(dashboard,"name"));
 cJSON_ArrayForEach(v,item(dashboard,"tiles")){
  dsr_mobile_tile_t *t=&m->tiles[m->tile_count++];const char *type=string(v,"type");t->type=!strcmp(type,"device")?0:!strcmp(type,"room")?1:2;
  t->index=(uint8_t)object_index(item(r,t->type==0?"devices":t->type==1?"rooms":"scenes"),integer(v,"ref"));t->span=(uint8_t)integer(v,"span");t->height=(uint8_t)integer(v,"height");t->fields=(uint8_t)integer(v,"fields");
 }
 cJSON_Delete(r);return true;
}

#ifndef DSR_UI_REVIEW_DEMO
static cJSON *draft;
static uint32_t draft_revision;
static char collection_name[16];
static cJSON *editing;
static lv_obj_t *body,*keyboard,*status_label;
static bool editor_active;
static void show_menu(void);
static void show_collection(lv_event_t *event);
static void show_entity(cJSON *entity);
static void close_keyboard(lv_event_t *event){(void)event;if(keyboard)lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);if(body)lv_obj_set_height(body,292);}
static void focus(lv_event_t *event)
{if(!keyboard)return;lv_obj_set_height(body,132);lv_obj_remove_flag(keyboard,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(keyboard);lv_keyboard_set_textarea(keyboard,lv_event_get_target(event));}
static void set_text(lv_event_t *event)
{const char *key=lv_event_get_user_data(event);lv_obj_t *field=lv_event_get_target(event);if(!editing)return;cJSON_ReplaceItemInObjectCaseSensitive(editing,key,cJSON_CreateString(lv_textarea_get_text(field)));}
static void set_number(lv_event_t *event)
{const char *key=lv_event_get_user_data(event);if(!editing)return;cJSON_ReplaceItemInObjectCaseSensitive(editing,key,cJSON_CreateNumber(strtoul(lv_textarea_get_text(lv_event_get_target(event)),NULL,10)));}
static lv_obj_t *screen(const char *title)
{
 dsr_network_reset_view();dsr_mobile_hide();lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);lv_obj_set_style_bg_color(root,lv_color_hex(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);lv_obj_set_style_text_font(root,&dsr_roboto_14,0);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 dsr_sc_text(root,title,14,12,290,&dsr_roboto_18,DSR_SC_TEXT);status_label=dsr_sc_text(root,"Zmiany zostaną zastosowane po zapisaniu",14,42,292,&dsr_roboto_11,DSR_SC_MUTED);
 body=dsr_sc_panel(root,12,74,296,292,DSR_SC_BG,0);lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);
 keyboard=lv_keyboard_create(root);lv_obj_set_size(keyboard,320,210);lv_obj_align(keyboard,LV_ALIGN_BOTTOM_MID,0,0);dsr_ui_keyboard(keyboard);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_READY,NULL);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_CANCEL,NULL);lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
 return root;
}
static void return_home(lv_event_t *e){(void)e;editor_active=false;cJSON_Delete(draft);draft=NULL;editing=NULL;body=keyboard=status_label=NULL;dsr_network_home(NULL);}
static void save(lv_event_t *event)
{
 (void)event;char error[96];if(!dsr_config_validate(draft,error)){lv_label_set_text(status_label,error);return;}
 char *json=cJSON_PrintUnformatted(draft);bool ok=json&&dsr_workspace_submit(json,draft_revision);free(json);lv_label_set_text(status_label,ok?"Zapisywanie…":"Konflikt lub zapis w toku. Wczytaj z pilota.");
}
static void actions(lv_obj_t *root,lv_event_cb_t back)
{dsr_ui_button(root,"Wstecz",12,420,140,back,NULL);dsr_ui_button(root,"Zapisz",168,420,140,save,NULL);}
static void menu_event(lv_event_t *e){(void)e;show_menu();}
static void add_field(const char *caption,const char *key,int y,bool numeric)
{
 dsr_sc_text(body,caption,2,y,280,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *f=lv_textarea_create(body);dsr_ui_textarea(f);lv_obj_set_pos(f,0,y+21);lv_obj_set_size(f,284,44);lv_textarea_set_one_line(f,true);
 lv_textarea_set_max_length(f,!strcmp(key,"name")?47:!strcmp(key,"host")?15:40);
 char text[32];if(numeric){snprintf(text,sizeof(text),"%d",integer(editing,key));lv_textarea_set_accepted_chars(f,"0123456789");lv_textarea_set_text(f,text);}else lv_textarea_set_text(f,string(editing,key));
 lv_obj_add_event_cb(f,focus,LV_EVENT_FOCUSED,NULL);lv_obj_add_event_cb(f,focus,LV_EVENT_CLICKED,NULL);lv_obj_add_event_cb(f,numeric?set_number:set_text,LV_EVENT_VALUE_CHANGED,(void *)key);
}
typedef struct {const char *key,*options;bool numeric;} choice_t;
static const choice_t kind_choice={"kind","light\nrelay\nplug\ncover\ncontact\nsensor",false},icon_choice={"icon","Salon\nKuchnia\nSypialnia\nŁazienka",true};
static void select_changed(lv_event_t *event)
{const choice_t *c=lv_event_get_user_data(event);unsigned selected=lv_dropdown_get_selected(lv_event_get_target(event));if(c->numeric)cJSON_ReplaceItemInObjectCaseSensitive(editing,c->key,cJSON_CreateNumber(selected));else{char value[32];lv_dropdown_get_selected_str(lv_event_get_target(event),value,sizeof(value));cJSON_ReplaceItemInObjectCaseSensitive(editing,c->key,cJSON_CreateString(value));}}
static void add_choice(const char *caption,const choice_t *choice,int y)
{
 dsr_sc_text(body,caption,2,y,284,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *d=lv_dropdown_create(body);lv_obj_set_pos(d,0,y+21);lv_obj_set_size(d,284,44);lv_dropdown_set_options(d,choice->options);lv_obj_set_style_text_font(d,&dsr_roboto_14,0);
 if(choice->numeric)lv_dropdown_set_selected(d,(uint32_t)integer(editing,choice->key));else{char *options=malloc(strlen(choice->options)+1);strcpy(options,choice->options);unsigned at=0;for(char *v=strtok(options,"\n");v;v=strtok(NULL,"\n"),at++)if(!strcmp(v,string(editing,choice->key))){lv_dropdown_set_selected(d,at);break;}free(options);}
 lv_obj_add_event_cb(d,select_changed,LV_EVENT_VALUE_CHANGED,(void *)choice);
}
static void delete_entity(lv_event_t *event)
{(void)event;cJSON *a=cJSON_GetObjectItemCaseSensitive(draft,collection_name);int at=0;for(cJSON *v=a->child;v;v=v->next,at++)if(v==editing){cJSON_DeleteItemFromArray(a,at);break;}editing=NULL;show_collection(NULL);}
static void edit_event(lv_event_t *event){show_entity(lv_event_get_user_data(event));}
static void reorder(lv_event_t *event)
{
 uintptr_t value=(uintptr_t)lv_event_get_user_data(event);int at=(int)(value/2),direction=value%2?1:-1;
 cJSON *a=cJSON_GetObjectItemCaseSensitive(editing,"tiles");int to=at+direction;if(to<0||to>=cJSON_GetArraySize(a))return;cJSON *t=cJSON_DetachItemFromArray(a,at);cJSON_InsertItemInArray(a,to,t);show_entity(editing);
}
static void tile_remove(lv_event_t *event){cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(editing,"tiles"),(int)(uintptr_t)lv_event_get_user_data(event));show_entity(editing);}
static void tile_add(lv_event_t *event)
{
 unsigned type=(unsigned)(uintptr_t)lv_event_get_user_data(event);const char *key=type==0?"devices":type==1?"rooms":"scenes";
 const cJSON *objects=item(draft,key);if(!objects->child){lv_label_set_text(status_label,"Najpierw dodaj obiekt tego typu");return;}
 cJSON *tiles=cJSON_GetObjectItemCaseSensitive(editing,"tiles");if(cJSON_GetArraySize(tiles)>=24)return;int id=1;for(cJSON *t=tiles->child;t;t=t->next)if(integer(t,"id")>=id)id=integer(t,"id")+1;
 cJSON *t=cJSON_CreateObject();cJSON_AddNumberToObject(t,"id",id);cJSON_AddStringToObject(t,"type",type==0?"device":type==1?"room":"scene");cJSON_AddNumberToObject(t,"ref",integer(objects->child,"id"));cJSON_AddNumberToObject(t,"span",type==1?1:2);cJSON_AddNumberToObject(t,"height",1);cJSON_AddNumberToObject(t,"fields",255);cJSON_AddItemToArray(tiles,t);show_entity(editing);
}
static void tile_next(lv_event_t *event)
{
 cJSON *t=lv_event_get_user_data(event);const char *type=string(t,"type");const cJSON *objects=item(draft,!strcmp(type,"device")?"devices":!strcmp(type,"room")?"rooms":"scenes"),*v;
 unsigned i=object_index(objects,integer(t,"ref"));v=cJSON_GetArrayItem(objects,(int)((i+1)%(unsigned)cJSON_GetArraySize(objects)));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"ref"),integer(v,"id"));show_entity(editing);
}
static void tile_size(lv_event_t *event)
{cJSON *t=lv_event_get_user_data(event);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"height"),integer(t,"height")==1?2:1);show_entity(editing);}
static void tile_fields(lv_event_t *event)
{cJSON *t=lv_event_get_user_data(event);int f=integer(t,"fields");cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"fields"),f==255?3:f==3?4:f==4?32:255);show_entity(editing);}
static void show_entity(cJSON *entity)
{
 editing=entity;lv_obj_t *root=screen("Edytuj konfigurację");add_field("Nazwa","name",0,false);
 if(!strcmp(collection_name,"rooms")){add_choice("Ikona",&icon_choice,80);add_field("ID zdjęcia z pamięci Flash (opcjonalne)","photo",160,false);}
 if(!strcmp(collection_name,"devices")){add_choice("Typ",&kind_choice,80);add_field("Adres IPv4 Shelly","host",160,false);add_field("Kanał (0–15)","channel",240,true);add_field("ID pomieszczenia (0 = brak)","room",320,true);}
 if(!strcmp(collection_name,"dashboards")){
  cJSON *tiles=cJSON_GetObjectItemCaseSensitive(entity,"tiles");int i=0;
  for(cJSON *t=tiles->child;t;t=t->next,i++){
   int y=80+i*120;char text[100];snprintf(text,sizeof(text),"%s #%d • %s",string(t,"type"),integer(t,"ref"),integer(t,"height")==2?"duża":"standard");
   dsr_sc_text(body,text,0,y,280,&dsr_roboto_14,DSR_SC_TEXT);
   dsr_ui_button(body,"Obiekt",0,y+22,87,tile_next,t);dsr_ui_button(body,"Rozmiar",96,y+22,87,tile_size,t);dsr_ui_button(body,"Dane",192,y+22,87,tile_fields,t);
   dsr_ui_button(body,"Góra",0,y+74,87,reorder,(void *)(uintptr_t)(i*2));dsr_ui_button(body,"Dół",96,y+74,87,reorder,(void *)(uintptr_t)(i*2+1));dsr_ui_button(body,"Usuń",192,y+74,87,tile_remove,(void *)(uintptr_t)i);
  }
  int y=88+i*120;dsr_ui_button(body,"+ Urządzenie",0,y,284,tile_add,(void *)0);dsr_ui_button(body,"+ Pomieszczenie",0,y+56,284,tile_add,(void *)1);dsr_ui_button(body,"+ Scena",0,y+112,284,tile_add,(void *)2);
 }
 int bottom=lv_obj_get_scroll_bottom(body)+300;if(bottom<400)bottom=400;dsr_ui_button(body,"Usuń wpis",0,bottom,284,delete_entity,NULL);actions(root,show_collection);
}
static void add_entity(lv_event_t *event)
{
 (void)event;cJSON *a=cJSON_GetObjectItemCaseSensitive(draft,collection_name);int id=1;for(cJSON *v=a->child;v;v=v->next)if(integer(v,"id")>=id)id=integer(v,"id")+1;
 cJSON *v=cJSON_CreateObject();cJSON_AddNumberToObject(v,"id",id);cJSON_AddStringToObject(v,"name","Nowy wpis");
 if(!strcmp(collection_name,"dashboards"))cJSON_AddItemToObject(v,"tiles",cJSON_CreateArray());
 if(!strcmp(collection_name,"rooms")){cJSON_AddNumberToObject(v,"icon",0);cJSON_AddStringToObject(v,"photo","");}
 if(!strcmp(collection_name,"devices")){cJSON_AddStringToObject(v,"kind","light");cJSON_AddStringToObject(v,"host","");cJSON_AddNumberToObject(v,"channel",0);cJSON_AddNumberToObject(v,"room",0);}
 if(!strcmp(collection_name,"scenes"))cJSON_AddItemToObject(v,"actions",cJSON_CreateArray());
 cJSON_AddItemToArray(a,v);show_entity(v);
}
static void show_collection(lv_event_t *event)
{
 if(event&&lv_event_get_user_data(event))snprintf(collection_name,sizeof(collection_name),"%s",(const char *)lv_event_get_user_data(event));
 editing=NULL;lv_obj_t *root=screen("Edycja • wspólna konfiguracja");const cJSON *a=item(draft,collection_name);int y=0;const cJSON *v;
 cJSON_ArrayForEach(v,a){dsr_ui_button(body,string(v,"name"),0,y,284,edit_event,(void *)v);y+=58;}dsr_ui_button(body,"+ Dodaj wpis",0,y,284,add_entity,NULL);actions(root,menu_event);
}
static const choice_t language_choice={"language","pl\nen\nde",false},theme_choice={"theme","graphite\nmidnight",false};
static void appearance(lv_event_t *event)
{(void)event;editing=draft;lv_obj_t *root=screen("Wygląd i język");add_choice("Język: Polski / English / Deutsch",&language_choice,0);add_choice("Motyw",&theme_choice,80);add_field("Akcent RGB (wartość dziesiętna)","accent",160,true);add_field("ID pulpitu startowego","active_dashboard",240,true);actions(root,menu_event);}
static void pair_browser(lv_event_t *event)
{
 (void)event;char code[17];dsr_panel_pair_begin(code);lv_obj_t *root=screen("Parowanie Web Panelu");
 dsr_sc_text(body,"http://shelly-remote.local",0,4,284,&dsr_roboto_16,DSR_SC_BLUE);dsr_sc_text(body,"Jednorazowy kod • ważny 2 minuty",0,43,284,&dsr_roboto_14,DSR_SC_TEXT);dsr_sc_text(body,code[0]?code:"Panel niedostępny",0,83,284,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(body,"Wpisz kod w przeglądarce w tej samej sieci Wi-Fi. Nowe parowanie unieważni poprzednią sesję.",0,128,284,&dsr_roboto_14,DSR_SC_MUTED);actions(root,menu_event);
}
static void allow_ota(lv_event_t *event)
{(void)event;dsr_panel_allow_ota();lv_label_set_text(status_label,"OTA dozwolone przez 2 minuty. Wybierz BIN w panelu.");}
static void show_menu(void)
{
 lv_obj_t *root=screen("Edytuj pilot");const char *labels[]={"Pulpity","Pomieszczenia","Urządzenia","Sceny"},*keys[]={"dashboards","rooms","devices","scenes"};
 for(unsigned i=0;i<4;i++)dsr_ui_button(body,labels[i],0,(int)i*56,284,show_collection,(void *)keys[i]);
 dsr_ui_button(body,"Wygląd i język",0,224,284,appearance,NULL);dsr_ui_button(body,"Web Panel • parowanie",0,280,284,pair_browser,NULL);dsr_ui_button(body,"Zezwól na OTA • 2 minuty",0,336,284,allow_ota,NULL);actions(root,return_home);
}
void dsr_ui_edit_dashboard(void)
{
 char *json=dsr_workspace_snapshot();if(!json)return;cJSON_Delete(draft);draft=cJSON_Parse(json);free(json);if(!draft)return;draft_revision=(uint32_t)integer(draft,"revision");editor_active=true;show_menu();
}
void dsr_ui_workspace_tick(void)
{
 if(!editor_active||!draft||!status_label)return;
 bool busy;char message[96];dsr_workspace_status(&busy,message);uint32_t revision=dsr_workspace_revision();
 if(revision!=draft_revision){lv_label_set_text(status_label,"Zmieniono konfigurację. Wróć i otwórz edycję ponownie.");}
 else if(busy||message[0])lv_label_set_text(status_label,message);
}
void dsr_ui_workspace_migrate(const dsr_connection_t *connection)
{
 if(!connection||!connection->device.host[0])return;
 char *json=dsr_workspace_snapshot();if(!json)return;cJSON *r=cJSON_Parse(json);free(json);if(!r)return;
 cJSON *a=cJSON_GetObjectItemCaseSensitive(r,"devices");bool found=false;int id=1;
 for(cJSON *v=a->child;v;v=v->next){if(!strcmp(string(v,"host"),connection->device.host)&&integer(v,"channel")==connection->device.channel)found=true;if(integer(v,"id")>=id)id=integer(v,"id")+1;}
 if(!found&&cJSON_GetArraySize(a)<16){cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"id",id);cJSON_AddStringToObject(d,"name",connection->device.name[0]?connection->device.name:"Shelly");cJSON_AddStringToObject(d,"host",connection->device.host);cJSON_AddNumberToObject(d,"channel",connection->device.channel);cJSON_AddNumberToObject(d,"room",0);cJSON_AddStringToObject(d,"kind","relay");cJSON_AddItemToArray(a,d);
  cJSON *dash=cJSON_GetArrayItem(item(r,"dashboards"),(int)object_index(item(r,"dashboards"),integer(r,"active_dashboard"))),*tiles=cJSON_GetObjectItemCaseSensitive(dash,"tiles"),*t=cJSON_CreateObject();int tile_id=1;for(cJSON *v=tiles->child;v;v=v->next)if(integer(v,"id")>=tile_id)tile_id=integer(v,"id")+1;
  cJSON_AddNumberToObject(t,"id",tile_id);cJSON_AddStringToObject(t,"type","device");cJSON_AddNumberToObject(t,"ref",id);cJSON_AddNumberToObject(t,"span",2);cJSON_AddNumberToObject(t,"height",1);cJSON_AddNumberToObject(t,"fields",255);cJSON_AddItemToArray(tiles,t);
  char *next=cJSON_PrintUnformatted(r);if(next){dsr_workspace_submit(next,(uint32_t)integer(r,"revision"));free(next);}
 }cJSON_Delete(r);
}
#endif
