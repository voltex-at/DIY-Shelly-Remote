#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_heap_caps.h"
#endif

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
 const char *locale=string(r,"language");dsr_sc_appearance(m->accent,m->midnight,!strcmp(locale,"en")?1:!strcmp(locale,"de")?2:0);
 snprintf(m->home_name,sizeof(m->home_name),"%s",dsr_sc_tr("Mój dom"));
 const cJSON *v;cJSON_ArrayForEach(v,item(r,"rooms")){
  dsr_mobile_room_t *room=&m->rooms[m->room_count++];snprintf(room->name,sizeof(room->name),"%s",string(v,"name"));room->icon=(uint8_t)integer(v,"icon");snprintf(room->photo,sizeof(room->photo),"%.32s",string(v,"photo"));room->id=(uint32_t)integer(v,"id");
 }
 m->scene_count=0;cJSON_ArrayForEach(v,item(r,"scenes")){dsr_mobile_scene_t *s=&m->scenes[m->scene_count++];snprintf(s->name,sizeof(s->name),"%s",string(v,"name"));s->actions=(uint8_t)cJSON_GetArraySize(item(v,"actions"));}
 cJSON_ArrayForEach(v,item(r,"devices")){
  dsr_mobile_device_t *d=&m->devices[m->device_count++];memset(d,0,sizeof(*d));d->id=(uint32_t)integer(v,"id");snprintf(d->name,sizeof(d->name),"%s",string(v,"name"));snprintf(d->host,sizeof(d->host),"%s",string(v,"host"));d->channel=(uint8_t)integer(v,"channel");d->room=(uint8_t)object_index(item(r,"rooms"),integer(v,"room"));
  const char *kind=string(v,"kind");d->kind=!strcmp(kind,"light")?DSR_DEVICE_LIGHT:!strcmp(kind,"cover")?DSR_DEVICE_COVER:!strcmp(kind,"contact")?DSR_DEVICE_CONTACT:!strcmp(kind,"sensor")?DSR_DEVICE_SENSOR:!strcmp(kind,"plug")?DSR_DEVICE_PLUG:DSR_DEVICE_RELAY;d->fields=255;
 }
 const cJSON *dashboard=cJSON_GetArrayItem(item(r,"dashboards"),(int)object_index(item(r,"dashboards"),integer(r,"active_dashboard")));
 m->dashboard_id=(uint32_t)integer(dashboard,"id");
 snprintf(m->dashboard_name,sizeof(m->dashboard_name),"%s",string(dashboard,"name"));
 cJSON_ArrayForEach(v,item(dashboard,"tiles")){
  dsr_mobile_tile_t *t=&m->tiles[m->tile_count++];const char *type=string(v,"type");t->type=!strcmp(type,"device")?0:!strcmp(type,"room")?1:2;
  t->index=(uint8_t)object_index(item(r,t->type==0?"devices":t->type==1?"rooms":"scenes"),integer(v,"ref"));t->span=(uint8_t)integer(v,"span");t->height=(uint8_t)integer(v,"height");t->fields=(uint8_t)integer(v,"fields");
 }
 cJSON_Delete(r);return true;
}

#ifndef DSR_UI_REVIEW_DEMO
static cJSON *draft;
/* A pending write owns an immutable snapshot; later edits remain in draft. */
static cJSON *submitted_draft,*saved_baseline;
static bool save_failed,save_confirmed;
static uint32_t draft_revision;
static char collection_name[16];
static cJSON *editing;
static lv_obj_t *body,*keyboard,*status_label;
static bool editor_active;
static bool own_save;
static bool dashboard_picker;
static bool room_editor,room_new,room_deleted;
static lv_obj_t *room_dialog,*settings_dialog;
static bool settings_context,appearance_editor;
static unsigned tile_page;
static cJSON *fields_tile;
typedef enum {SAVE_STAY,SAVE_HOME,SAVE_ROOMS,SAVE_MENU,SAVE_COLLECTION,SAVE_SETTINGS,SAVE_ENTITY} save_target_t;
static save_target_t view_save_target,submitted_save_target;
static uint32_t view_generation,submitted_generation;
static bool room_return_detail;
static uint32_t room_return_id;
typedef enum {OPEN_OK,OPEN_STORAGE,OPEN_SNAPSHOT_MEMORY,OPEN_PARSE,OPEN_INVALID,OPEN_BASELINE_MEMORY,OPEN_OBJECT_MEMORY,OPEN_PANEL} open_result_t;
static open_result_t open_error;
static bool error_return_rooms,error_return_settings;
static void config_log(const char *stage,int code)
{
#ifdef ESP_PLATFORM
 ESP_LOGI("dsr_config_ui","stage=%s code=%d internal_free=%u largest=%u psram_free=%u",stage,code,
  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
  (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#else
 (void)stage;(void)code;
#endif
}
static void clear_draft(void)
{cJSON_Delete(draft);cJSON_Delete(submitted_draft);cJSON_Delete(saved_baseline);draft=submitted_draft=saved_baseline=editing=NULL;own_save=false;save_failed=save_confirmed=false;submitted_save_target=view_save_target=SAVE_STAY;}
void dsr_ui_workspace_hide(void){editor_active=false;room_editor=room_new=room_deleted=false;room_return_detail=false;room_return_id=0;room_dialog=settings_dialog=NULL;body=keyboard=status_label=NULL;settings_context=appearance_editor=false;fields_tile=NULL;clear_draft();}
static void settings_stay(lv_event_t *event)
{(void)event;if(settings_dialog){lv_obj_delete(settings_dialog);settings_dialog=NULL;}}
static void settings_discard(lv_event_t *event)
{(void)event;dsr_ui_workspace_hide();dsr_ui_open_settings();}
bool dsr_ui_settings_can_open(void)
{
 if(own_save){if(status_label)dsr_sc_label_set_text(status_label,"Poczekaj na zakończenie zapisu.");return false;}
 if(!editor_active||!draft||(saved_baseline&&cJSON_Compare(draft,saved_baseline,true)))return true;
 if(settings_dialog)return false;
 if(keyboard)lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
 if(body)lv_obj_set_height(body,292);
 settings_dialog=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);
 lv_obj_set_style_bg_opa(settings_dialog,180,0);lv_obj_add_flag(settings_dialog,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *panel=dsr_sc_panel(settings_dialog,12,117,296,246,DSR_SC_PANEL,14);
 dsr_sc_text(panel,"Odrzucić zmiany?",14,16,268,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(panel,"Niezapisane zmiany tego formularza zostaną utracone.",14,53,268,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(panel,"Zostań",12,188,130,settings_stay,NULL);
 dsr_ui_button(panel,"Odrzuć",154,188,130,settings_discard,NULL);return false;
}
static void return_settings(lv_event_t *event)
{(void)event;if(dsr_ui_settings_can_open()){dsr_ui_workspace_hide();dsr_ui_open_settings();}}
static void show_menu(void);
static void show_collection(lv_event_t *event);
static void show_entity(cJSON *entity);
static void show_room_form(void);
static void close_keyboard(lv_event_t *event){(void)event;if(keyboard)lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);if(body)lv_obj_set_height(body,292);}
static void focus(lv_event_t *event)
{if(!keyboard)return;lv_obj_set_height(body,130);dsr_ui_keyboard_focus(keyboard,lv_event_get_target(event));lv_obj_scroll_to_view(lv_event_get_target(event),LV_ANIM_OFF);}
static void set_text(lv_event_t *event)
{const char *key=lv_event_get_user_data(event);lv_obj_t *field=lv_event_get_target(event);if(!editing)return;cJSON_ReplaceItemInObjectCaseSensitive(editing,key,cJSON_CreateString(lv_textarea_get_text(field)));}
static void set_number(lv_event_t *event)
{const char *key=lv_event_get_user_data(event);if(!editing)return;cJSON_ReplaceItemInObjectCaseSensitive(editing,key,cJSON_CreateNumber(strtoul(lv_textarea_get_text(lv_event_get_target(event)),NULL,10)));}
static lv_obj_t *screen(const char *title)
{
 view_generation++;view_save_target=SAVE_STAY;room_dialog=settings_dialog=NULL;
 dsr_network_reset_view();dsr_mobile_hide();lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);lv_obj_set_style_bg_color(root,lv_color_hex(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);lv_obj_set_style_text_font(root,&dsr_roboto_14,0);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 dsr_sc_header(root,"Mój dom",false,dsr_wifi_connected(),NULL,NULL);dsr_sc_text(root,title,12,46,296,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_panel(root,12,75,296,1,DSR_SC_BLUE,0);
 status_label=dsr_sc_text(root,"Zmiany zostaną zastosowane po zapisaniu",14,82,292,&dsr_roboto_11,DSR_SC_MUTED);
 body=dsr_sc_panel(root,12,106,296,292,DSR_SC_BG,0);lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);
 keyboard=lv_keyboard_create(root);dsr_ui_keyboard(keyboard);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_READY,NULL);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_CANCEL,NULL);lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
 return root;
}
static void return_home(lv_event_t *e){(void)e;dsr_ui_workspace_hide();dsr_network_home(NULL);}
static void error_back(lv_event_t *e){(void)e;bool settings=error_return_settings,rooms=error_return_rooms;dsr_ui_workspace_hide();if(settings)dsr_ui_open_settings();else if(rooms)dsr_network_rooms();else dsr_network_home(NULL);}
static void config_error(open_result_t result);
static void diagnostics(lv_event_t *event)
{
 (void)event;dsr_config_health_t h;dsr_config_health_snapshot(&h);
 lv_obj_t *root=screen("Diagnostyka konfiguracji");dsr_sc_label_set_text(status_label,"Tylko odczyt • dane zachowane");
 const char *storage=h.storage==DSR_STORAGE_READY?"SPIFFS: zamontowany":h.storage==DSR_STORAGE_NONEMPTY?"SPIFFS: niepusty, niezamontowany":h.storage==DSR_STORAGE_READ_ERROR?"SPIFFS: błąd odczytu":h.storage==DSR_STORAGE_PROVISION_ERROR?"SPIFFS: błąd inicjalizacji":"SPIFFS: niedostępny";
 dsr_sc_text(body,storage,4,8,284,&dsr_roboto_16,DSR_SC_TEXT);
 char codes[96];snprintf(codes,sizeof(codes),"Storage: %d / 0x%x\nWorkspace: %d / 0x%x\nHTTPS: %d / 0x%x",(int)h.storage,(unsigned)h.storage_error,(int)h.workspace,(unsigned)h.workspace_error,(int)h.panel,(unsigned)h.panel_error);dsr_sc_text(body,codes,4,50,284,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_sc_text(body,h.panel==DSR_PANEL_READY?"HTTPS uruchomiony":h.panel==DSR_PANEL_FAILED?"HTTPS: błąd uruchomienia":"HTTPS nie został uruchomiony",4,123,284,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_text(body,"USB 115200 • zachowaj kopię partycji przed naprawą.",4,156,284,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_sc_text(body,"dsrdata: 0xC20000 + 0x3E0000\nNVS: 0x9000 + 0x10000",4,207,284,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(body,"Nie kasuj i nie formatuj pamięci.",4,251,284,&dsr_roboto_14,0xffc18c);
 dsr_ui_button(root,"Wstecz",12,420,296,error_back,NULL);config_log("diagnostics",open_error);
}
void dsr_ui_config_diagnostics(lv_event_t *event)
{if(!dsr_ui_settings_can_open())return;dsr_ui_workspace_hide();error_return_rooms=false;error_return_settings=true;open_error=OPEN_OK;diagnostics(event);}
static void config_error(open_result_t result)
{
 clear_draft();editor_active=false;room_editor=room_new=room_deleted=false;room_dialog=settings_dialog=NULL;open_error=result;
 lv_obj_t *root=screen("Konfiguracja niedostępna");dsr_sc_label_set_text(status_label,"Nie można otworzyć edytora");
 dsr_config_health_t h;dsr_config_health_snapshot(&h);
 const char *reason=result==OPEN_PANEL?"Serwer HTTPS jest niedostępny. Kod parowania nie został utworzony.":result==OPEN_STORAGE?(h.storage==DSR_STORAGE_NONEMPTY?"Magazyn konfiguracji niedostępny: niepusta partycja SPIFFS nie została zamontowana.":"Magazyn konfiguracji jest niedostępny."):result==OPEN_PARSE||result==OPEN_INVALID?"Nie można odczytać poprawnej konfiguracji. Sprawdź pamięć i kopię danych.":"Brak pamięci do otwarcia edytora. Ustawienia pozostają zachowane.";
 dsr_sc_text(body,reason,4,16,284,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(body,"Dane nie zostały skasowane. Szczegóły są w diagnostyce i logu USB.",4,137,284,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(root,"Wstecz",12,420,140,error_back,NULL);dsr_ui_button(root,"Diagnostyka",168,420,140,diagnostics,NULL);config_log("open_failed",result);
}
/* Transactional open: no expired/partial draft survives an error. */
static open_result_t prepare_draft(void)
{
 char *json=NULL;dsr_snapshot_result_t result=dsr_workspace_snapshot_checked(&json);config_log("snapshot",result);
 if(result!=DSR_SNAPSHOT_OK)return result==DSR_SNAPSHOT_NO_MEMORY?OPEN_SNAPSHOT_MEMORY:OPEN_STORAGE;
 if(!dsr_config_json_depth(json)){free(json);return OPEN_PARSE;}
 cJSON *next=cJSON_Parse(json);free(json);config_log("parse",next?0:OPEN_PARSE);if(!next)return OPEN_PARSE;
 char error[96];if(!dsr_config_validate(next,error)){cJSON_Delete(next);return OPEN_INVALID;}
 cJSON *baseline=cJSON_Duplicate(next,true);config_log("baseline",baseline?0:OPEN_BASELINE_MEMORY);if(!baseline){cJSON_Delete(next);return OPEN_BASELINE_MEMORY;}
 clear_draft();draft=next;saved_baseline=baseline;draft_revision=(uint32_t)integer(draft,"revision");return OPEN_OK;
}
static void save(lv_event_t *event)
{
 (void)event;if(own_save){dsr_sc_label_set_text(status_label,"Zapisywanie…");return;}
 if(room_editor&&editing&&(!string(editing,"name")[0]||strlen(string(editing,"name"))>47)){
  dsr_sc_label_set_text(status_label,!string(editing,"name")[0]?"Podaj nazwę pomieszczenia.":"Podaj krótszą nazwę pomieszczenia.");return;
 }
 char error[96];if(!dsr_config_validate(draft,error)){dsr_sc_label_set_text(status_label,"Nieprawidłowa konfiguracja. Sprawdź pola i powiązania.");return;}
 cJSON *snapshot=cJSON_Duplicate(draft,true);char *json=snapshot?cJSON_PrintUnformatted(snapshot):NULL;
 bool ok=json&&dsr_workspace_submit(json,draft_revision);free(json);
 if(ok){cJSON_Delete(submitted_draft);submitted_draft=snapshot;own_save=true;save_failed=false;submitted_save_target=view_save_target;submitted_generation=view_generation;}else{cJSON_Delete(snapshot);save_failed=true;}
 dsr_sc_label_set_text(status_label,ok?"Zapisywanie…":"Konflikt lub zapis w toku. Wczytaj z pilota.");
}
static void actions(lv_obj_t *root,lv_event_cb_t back)
{dsr_ui_button(root,"Wstecz",12,420,140,back,NULL);dsr_ui_button(root,"Zapisz",168,420,140,save,NULL);}
static void menu_event(lv_event_t *e){(void)e;show_menu();}
static void add_field(const char *caption,const char *key,int y,bool numeric)
{
 dsr_sc_text(body,caption,2,y,280,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *f=lv_textarea_create(body);dsr_ui_textarea(f);lv_obj_set_pos(f,0,y+21);lv_obj_set_size(f,284,44);lv_textarea_set_one_line(f,true);
 lv_textarea_set_max_length(f,!strcmp(key,"name")?47:!strcmp(key,"host")?15:40);
 char text[32];if(numeric){snprintf(text,sizeof(text),"%d",integer(editing,key));lv_textarea_set_accepted_chars(f,"0123456789");lv_textarea_set_text(f,text);}else lv_textarea_set_text(f,string(editing,key));
 if(!strcmp(key,"host"))lv_textarea_set_accepted_chars(f,"0123456789.");
 lv_obj_add_event_cb(f,focus,LV_EVENT_FOCUSED,NULL);lv_obj_add_event_cb(f,focus,LV_EVENT_CLICKED,NULL);lv_obj_add_event_cb(f,numeric?set_number:set_text,LV_EVENT_VALUE_CHANGED,(void *)key);
}
typedef struct {const char *key,*options;bool numeric;} choice_t;
static const choice_t kind_choice={"kind","light\nrelay\nplug\ncover\ncontact\nsensor",false},icon_choice={"icon","Salon\nKuchnia\nSypialnia\nŁazienka",true};
static void select_changed(lv_event_t *event)
{
 const choice_t *c=lv_event_get_user_data(event);unsigned selected=lv_dropdown_get_selected(lv_event_get_target(event));
 if(c->numeric)cJSON_ReplaceItemInObjectCaseSensitive(editing,c->key,cJSON_CreateNumber(selected));
 else{const char *value=c->options;while(selected--){value=strchr(value,'\n');if(!value)return;value++;}size_t length=strcspn(value,"\n");char key[32];if(length>=sizeof(key))return;memcpy(key,value,length);key[length]=0;cJSON_ReplaceItemInObjectCaseSensitive(editing,c->key,cJSON_CreateString(key));}
}
static void add_choice(const char *caption,const choice_t *choice,int y)
{
 dsr_sc_text(body,caption,2,y,284,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *d=lv_dropdown_create(body);lv_obj_set_pos(d,0,y+21);lv_obj_set_size(d,284,44);
 const char *options=!strcmp(choice->key,"kind")?"Oświetlenie\nPrzekaźnik\nGniazdko\nRoleta\nCzujnik otwarcia\nCzujnik":!strcmp(choice->key,"theme")?"Grafitowy\nNocny":!strcmp(choice->key,"language")?"Polski\nEnglish\nDeutsch":choice->options;
 lv_dropdown_set_options(d,dsr_sc_tr(options));lv_obj_set_style_text_font(d,&dsr_roboto_14,0);
 if(choice->numeric)lv_dropdown_set_selected(d,(uint32_t)integer(editing,choice->key));else{char *options=malloc(strlen(choice->options)+1);strcpy(options,choice->options);unsigned at=0;for(char *v=strtok(options,"\n");v;v=strtok(NULL,"\n"),at++)if(!strcmp(v,string(editing,choice->key))){lv_dropdown_set_selected(d,at);break;}free(options);}
 lv_obj_add_event_cb(d,select_changed,LV_EVENT_VALUE_CHANGED,(void *)choice);
}
static void object_selected(lv_event_t *event)
{
 const char *key=lv_event_get_user_data(event);unsigned index=lv_dropdown_get_selected(lv_event_get_target(event));
 const cJSON *objects=item(draft,!strcmp(key,"room")?"rooms":"dashboards");int id=0;
 if(strcmp(key,"room")||index){if(!strcmp(key,"room"))index--;id=integer(cJSON_GetArrayItem(objects,(int)index),"id");}
 cJSON_ReplaceItemInObjectCaseSensitive(editing,key,cJSON_CreateNumber(id));
}
static void object_choice(const char *caption,const char *key,int y)
{
 const cJSON *a=item(draft,!strcmp(key,"room")?"rooms":"dashboards"),*v;char options[1024]={0};unsigned selected=0,index=0;
 if(!strcmp(key,"room")){snprintf(options,sizeof(options),"%s",dsr_sc_tr("Nieprzypisane"));index=1;}
 cJSON_ArrayForEach(v,a){if(options[0])strncat(options,"\n",sizeof(options)-strlen(options)-1);strncat(options,string(v,"name"),sizeof(options)-strlen(options)-1);if(integer(v,"id")==integer(editing,key))selected=index;index++;}
 dsr_sc_text(body,caption,2,y,284,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *d=lv_dropdown_create(body);lv_obj_set_pos(d,0,y+21);lv_obj_set_size(d,284,44);lv_dropdown_set_options(d,options);lv_dropdown_set_selected(d,selected);lv_obj_add_event_cb(d,object_selected,LV_EVENT_VALUE_CHANGED,(void *)key);
}
static void choose_photo(lv_event_t *event)
{
 (void)event;unsigned index=0;const char *id;
 while((id=dsr_photo_id(index))){if(!strcmp(id,string(editing,"photo"))){index++;break;}index++;}
 if(!string(editing,"photo")[0])index=0;
 id=dsr_photo_id(index);
 cJSON_ReplaceItemInObjectCaseSensitive(editing,"photo",cJSON_CreateString(id?id:""));show_entity(editing);
}
/* Contextual rooms use the same draft, CAS submission and durable acknowledgement
 * as the full editor. Only their entry points and navigation differ. */
static bool room_dirty(void){return draft&&(!saved_baseline||!cJSON_Compare(draft,saved_baseline,true));}
static void room_leave(lv_event_t *event)
{(void)event;bool detail=room_return_detail;uint32_t id=room_return_id;if(detail)dsr_network_room_context(id);else dsr_network_rooms();}
static void room_dialog_close(lv_event_t *event)
{(void)event;if(room_dialog){lv_obj_delete(room_dialog);room_dialog=NULL;}}
static lv_obj_t *room_modal(const char *title,const char *text)
{
 close_keyboard(NULL);room_dialog_close(NULL);
 room_dialog=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);lv_obj_set_style_bg_opa(room_dialog,180,0);lv_obj_add_flag(room_dialog,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *panel=dsr_sc_panel(room_dialog,12,117,296,246,DSR_SC_PANEL,14);
 dsr_sc_text(panel,title,14,16,268,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(panel,text,14,53,268,&dsr_roboto_14,DSR_SC_MUTED);return panel;
}
static void room_back(lv_event_t *event)
{
 (void)event;if(own_save){dsr_sc_label_set_text(status_label,"Poczekaj na zakończenie zapisu.");return;}
 if(!room_dirty()){room_leave(NULL);return;}
 lv_obj_t *panel=room_modal("Odrzucić zmiany?","Niezapisane zmiany tego formularza zostaną utracone.");
 dsr_ui_button(panel,"Zostań",12,188,130,room_dialog_close,NULL);dsr_ui_button(panel,"Odrzuć",154,188,130,room_leave,NULL);
}
static void room_delete_confirm(lv_event_t *event)
{
 (void)event;room_dialog_close(NULL);
 cJSON *rooms=cJSON_GetObjectItemCaseSensitive(draft,"rooms");
 int at=0;for(cJSON *v=rooms->child;v;v=v->next,at++)if(v==editing){cJSON_DeleteItemFromArray(rooms,at);break;}
 editing=NULL;room_deleted=true;show_room_form(); /* No deletion is published until Save succeeds. */
}
static void room_delete(lv_event_t *event)
{
 (void)event;if(own_save){dsr_sc_label_set_text(status_label,"Poczekaj na zakończenie zapisu.");return;}
 unsigned devices=0,tiles=0;const cJSON *d,*t;int id=integer(editing,"id");
 cJSON_ArrayForEach(d,item(draft,"devices"))if(integer(d,"room")==id)devices++;
 cJSON_ArrayForEach(d,item(draft,"dashboards"))cJSON_ArrayForEach(t,item(d,"tiles"))if(!strcmp(string(t,"type"),"room")&&integer(t,"ref")==id)tiles++;
 if(devices||tiles){
  char text[224];snprintf(text,sizeof(text),dsr_sc_tr("Urządzenia: %u • karty pomieszczenia: %u. Przenieś urządzenia i usuń te karty z pulpitów. Sceny i urządzenia pozostaną zachowane."),devices,tiles);
  lv_obj_t *panel=room_modal("Pomieszczenie jest używane",text);dsr_ui_button(panel,"Wstecz",12,188,272,room_dialog_close,NULL);return;
 }
 lv_obj_t *panel=room_modal("Usunąć pomieszczenie?","Po potwierdzeniu wybierz Zapisz. Zdjęcie może być używane przez inne pokoje.");
 dsr_ui_button(panel,"Anuluj",12,188,130,room_dialog_close,NULL);dsr_ui_button(panel,"Usuń",154,188,130,room_delete_confirm,NULL);
}
static void room_photo(void)
{
 lv_obj_t *button=dsr_sc_button(body,0,160,284,64,DSR_SC_CARD,12,choose_photo,NULL);
 lv_obj_t *thumb=dsr_sc_panel(button,8,8,64,48,DSR_SC_PANEL,6);lv_obj_set_style_clip_corner(thumb,true,0);
 const void *pixels=dsr_photo_pixels(string(editing,"photo"));
 if(pixels){
  static lv_image_dsc_t photo;photo=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=DSR_PHOTO_WIDTH,.h=DSR_PHOTO_HEIGHT,.stride=DSR_PHOTO_WIDTH*2},.data_size=DSR_PHOTO_BYTES,.data=pixels};
  lv_obj_t *image=lv_image_create(thumb);lv_image_set_src(image,&photo);lv_image_set_scale(image,48*256/DSR_PHOTO_HEIGHT);lv_image_set_antialias(image,true);lv_obj_center(image);
 }else dsr_sc_icon_centered(thumb,DSR_ICON_HOME,32,DSR_SC_MUTED);
 dsr_sc_text(button,string(editing,"photo")[0]?"Zmień zdjęcie z pamięci":"Wybierz zdjęcie z pamięci",84,15,188,&dsr_roboto_14,DSR_SC_TEXT);
}
static void show_room_form(void)
{
 bool deleted_saved=room_deleted&&save_confirmed&&!room_dirty();
 lv_obj_t *root=screen(deleted_saved?"Pomieszczenie usunięte":room_deleted?"Usunięcie pomieszczenia":room_new?"Nowe pomieszczenie":"Edytuj pomieszczenie");room_dialog=NULL;
 view_save_target=SAVE_ROOMS;
 lv_obj_set_y(body,118);lv_obj_set_height(body,280);
 if(room_deleted){
  dsr_sc_icon(body,DSR_ICON_HOME,118,18,52,DSR_SC_MUTED);
  dsr_sc_text(body,deleted_saved?"Zapisano • zsynchronizowano":"Zapisz, aby trwale usunąć pomieszczenie. Wstecz pozwala odrzucić zmianę.",6,94,284,&dsr_roboto_16,DSR_SC_TEXT);
 }else{
  add_field("Nazwa","name",0,false);add_choice("Ikona",&icon_choice,80);
  room_photo();
  if(!room_new)dsr_ui_button(body,"Usuń pomieszczenie",0,232,284,room_delete,NULL);
  dsr_sc_text(body,dsr_photo_id(0)?"Własne zdjęcia dodaj w lokalnym Web Panelu. Zostaną zapisane w pilocie.":"Brak zdjęć w pamięci. Dodaj zdjęcia w Web Panelu.",2,room_new?236:292,280,&dsr_roboto_12,DSR_SC_MUTED);
 }
 if(deleted_saved)dsr_ui_button(root,"Wstecz",12,420,296,room_back,NULL);else actions(root,room_back);
}
static void scene_action_cycle(lv_event_t *event)
{
 cJSON *a=lv_event_get_user_data(event);const char *commands[]={"on","off","open","close","stop"};unsigned at=0;
 while(at<5&&strcmp(string(a,"command"),commands[at]))at++;
 cJSON_ReplaceItemInObjectCaseSensitive(a,"command",cJSON_CreateString(commands[(at+1)%5]));show_entity(editing);
}
static void scene_device_cycle(lv_event_t *event)
{
 cJSON *a=lv_event_get_user_data(event);const cJSON *devices=item(draft,"devices");int count=cJSON_GetArraySize(devices);if(!count)return;unsigned at=object_index(devices,integer(a,"device"));const cJSON *d=cJSON_GetArrayItem(devices,(int)((at+1)%(unsigned)count));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(a,"device"),integer(d,"id"));show_entity(editing);
}
static void scene_action_remove(lv_event_t *event){cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(editing,"actions"),(int)(uintptr_t)lv_event_get_user_data(event));show_entity(editing);}
static void scene_action_add(lv_event_t *event)
{
 (void)event;cJSON *actions=cJSON_GetObjectItemCaseSensitive(editing,"actions");const cJSON *devices=item(draft,"devices");if(!devices->child||cJSON_GetArraySize(actions)>=8){dsr_sc_label_set_text(status_label,"Dodaj urządzenie; limit sceny: 8 działań");return;}
 cJSON *a=cJSON_CreateObject();cJSON_AddNumberToObject(a,"device",integer(devices->child,"id"));cJSON_AddStringToObject(a,"command","on");cJSON_AddItemToArray(actions,a);show_entity(editing);
}
static bool entity_deletable(void)
{
 if(own_save){dsr_sc_label_set_text(status_label,"Poczekaj na zakończenie zapisu.");return false;}
 if(!draft||!editing)return false;
 const cJSON *a=item(draft,collection_name),*member;bool present=false;
 cJSON_ArrayForEach(member,a)if(member==editing)present=true;
 if(!present)return false;
 int id=integer(editing,"id");const cJSON *v,*t;bool referenced=false;
 const char *type=!strcmp(collection_name,"devices")?"device":!strcmp(collection_name,"rooms")?"room":"scene";
 cJSON_ArrayForEach(v,item(draft,"dashboards"))cJSON_ArrayForEach(t,item(v,"tiles"))if(!strcmp(string(t,"type"),type)&&integer(t,"ref")==id)referenced=true;
 if(!strcmp(collection_name,"devices"))cJSON_ArrayForEach(v,item(draft,"scenes"))cJSON_ArrayForEach(t,item(v,"actions"))if(integer(t,"device")==id)referenced=true;
 if(!strcmp(collection_name,"rooms"))cJSON_ArrayForEach(v,item(draft,"devices"))if(integer(v,"room")==id)referenced=true;
 if(strcmp(collection_name,"dashboards")&&referenced){dsr_sc_label_set_text(status_label,"Najpierw usuń powiązane karty lub działania.");return false;}
 if(!strcmp(collection_name,"dashboards")&&cJSON_GetArraySize(a)<=1){dsr_sc_label_set_text(status_label,"Zachowaj przynajmniej jeden pulpit.");return false;}
 return true;
}
static void delete_entity_confirm(lv_event_t *event)
{
 (void)event;room_dialog_close(NULL);if(!entity_deletable())return;
 cJSON *a=cJSON_GetObjectItemCaseSensitive(draft,collection_name);int id=integer(editing,"id"),at=0;
 for(cJSON *entry=a->child;entry;entry=entry->next,at++)if(entry==editing){cJSON_DeleteItemFromArray(a,at);break;}
 if(!strcmp(collection_name,"dashboards")&&integer(draft,"active_dashboard")==id)cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"active_dashboard"),integer(a->child,"id"));
 editing=NULL;show_collection(NULL);view_save_target=SAVE_COLLECTION;
}
static void delete_entity(lv_event_t *event)
{
 (void)event;if(!entity_deletable())return;
 char question[160];snprintf(question,sizeof(question),dsr_sc_tr("Usunąć \"%s\"?"),string(editing,"name"));
 lv_obj_t *panel=room_modal(question,"Po potwierdzeniu wybierz Zapisz. Dane zostaną zmienione dopiero po udanym zapisie.");
 /* A maximum-width name can wrap beyond two lines. Bound this named heading
  * and move only this modal's description, leaving contextual dialogs intact. */
 lv_obj_t *heading=lv_obj_get_child(panel,0);lv_obj_set_height(heading,44);lv_label_set_long_mode(heading,LV_LABEL_LONG_MODE_DOTS);
 lv_obj_set_y(lv_obj_get_child(panel,1),72);
 dsr_ui_button(panel,"Anuluj",12,188,130,room_dialog_close,NULL);
 dsr_ui_button(panel,"Usuń",154,188,130,delete_entity_confirm,NULL);
}
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
 const cJSON *objects=item(draft,key);if(!objects->child){dsr_sc_label_set_text(status_label,"Najpierw dodaj obiekt tego typu");return;}
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
static void tile_span(lv_event_t *event)
{cJSON *t=lv_event_get_user_data(event);if(strcmp(string(t,"type"),"room")){dsr_sc_label_set_text(status_label,"Urządzenia i sceny używają pełnej szerokości.");return;}cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"span"),integer(t,"span")==1?2:1);show_entity(editing);}
static void tile_type(lv_event_t *event)
{
 cJSON *t=lv_event_get_user_data(event);const char *types[]={"device","room","scene"},*keys[]={"devices","rooms","scenes"};unsigned current=!strcmp(string(t,"type"),"device")?0:!strcmp(string(t,"type"),"room")?1:2;
 for(unsigned n=1;n<=3;n++){unsigned to=(current+n)%3;const cJSON *a=item(draft,keys[to]);if(a->child){cJSON_ReplaceItemInObjectCaseSensitive(t,"type",cJSON_CreateString(types[to]));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"ref"),integer(a->child,"id"));if(to!=1)cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"span"),2);break;}}show_entity(editing);
}
static void field_changed(lv_event_t *event)
{unsigned bit=(unsigned)(uintptr_t)lv_event_get_user_data(event);int f=integer(fields_tile,"fields");if(lv_obj_has_state(lv_event_get_target(event),LV_STATE_CHECKED))f|=(int)bit;else f&=~(int)bit;cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(fields_tile,"fields"),f);}
static void fields_back(lv_event_t *event){(void)event;show_entity(editing);}
static void tile_fields(lv_event_t *event)
{
 fields_tile=lv_event_get_user_data(event);lv_obj_t *root=screen("Dane wyświetlane na karcie");const char *labels[]={"Moc (W)","Napięcie (V)","Bateria","Temperatura","Wilgotność","Pozycja","Stan otwarcia","Dodatkowe parametry"};
 view_save_target=SAVE_ENTITY;
 for(unsigned i=0;i<8;i++){lv_obj_t *c=lv_checkbox_create(body);lv_obj_set_pos(c,6,(int)i*44);lv_checkbox_set_text(c,dsr_sc_tr(labels[i]));lv_obj_set_style_text_color(c,lv_color_hex(DSR_SC_TEXT),0);if(integer(fields_tile,"fields")&(1<<i))lv_obj_add_state(c,LV_STATE_CHECKED);lv_obj_add_event_cb(c,field_changed,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)(1u<<i));}actions(root,fields_back);
}
static void tiles_page(lv_event_t *event){tile_page=(unsigned)(uintptr_t)lv_event_get_user_data(event);show_entity(editing);}
static void show_entity(cJSON *entity)
{
 if(room_editor){editing=entity;show_room_form();return;}
 editing=entity;lv_obj_t *root=screen("Edytuj konfigurację");add_field("Nazwa","name",0,false);
 view_save_target=SAVE_COLLECTION;
 bool room=!strcmp(collection_name,"rooms"),dashboard=!strcmp(collection_name,"dashboards");
 if(room||dashboard)dsr_ui_button(body,room?"Usuń pomieszczenie":"Usuń pulpit",0,76,284,delete_entity,NULL);
 if(room){add_choice("Ikona",&icon_choice,140);dsr_ui_button(body,string(editing,"photo")[0]?"Zmień zdjęcie z pamięci":"Wybierz zdjęcie z pamięci",0,228,284,choose_photo,NULL);dsr_sc_text(body,"Własne zdjęcia dodaj w lokalnym Web Panelu. Zostaną zapisane w pilocie.",0,288,284,&dsr_roboto_12,DSR_SC_MUTED);}
 if(!strcmp(collection_name,"devices")){add_choice("Typ",&kind_choice,80);add_field("Adres IPv4 Shelly","host",160,false);add_field("Kanał (0–15)","channel",240,true);object_choice("Pomieszczenie","room",320);}
 if(!strcmp(collection_name,"scenes")){
  const cJSON *actions_list=item(editing,"actions"),*a;int i=0;
  cJSON_ArrayForEach(a,actions_list){int y=80+i*112;const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),integer(a,"device")));dsr_ui_button(body,string(d,"name"),0,y,284,scene_device_cycle,(void *)a);dsr_ui_button(body,string(a,"command"),0,y+52,180,scene_action_cycle,(void *)a);dsr_ui_button(body,"Usuń",190,y+52,94,scene_action_remove,(void *)(uintptr_t)i);i++;}dsr_ui_button(body,"+ Działanie",0,88+i*112,284,scene_action_add,NULL);
 }
 if(!strcmp(collection_name,"dashboards")){
  cJSON *tiles=cJSON_GetObjectItemCaseSensitive(entity,"tiles");int i=0,visible=0;
  unsigned pages=((unsigned)cJSON_GetArraySize(tiles)+2)/3;if(tile_page>=pages)tile_page=pages?pages-1:0;
  for(cJSON *t=tiles->child;t;t=t->next,i++){
   if((unsigned)i/3!=tile_page)continue;
   const char *type=string(t,"type");const cJSON *a=item(draft,!strcmp(type,"device")?"devices":!strcmp(type,"room")?"rooms":"scenes"),*target=cJSON_GetArrayItem(a,(int)object_index(a,integer(t,"ref")));
   int y=140+visible++*182;char text[100];snprintf(text,sizeof(text),"%s • %s",string(target,"name"),dsr_sc_tr(integer(t,"height")==2?"duża":"standard"));
   dsr_sc_text(body,text,0,y,280,&dsr_roboto_14,DSR_SC_TEXT);
   dsr_ui_button(body,"Obiekt",0,y+22,87,tile_next,t);dsr_ui_button(body,"Układ",96,y+22,87,tile_span,t);dsr_ui_button(body,"Typ",192,y+22,87,tile_type,t);
   dsr_ui_button(body,"Wys.",0,y+74,87,tile_size,t);dsr_ui_button(body,"Dane",96,y+74,87,tile_fields,t);dsr_ui_button(body,"Usuń",192,y+74,87,tile_remove,(void *)(uintptr_t)i);
   dsr_ui_button(body,"Góra",0,y+126,136,reorder,(void *)(uintptr_t)(i*2));dsr_ui_button(body,"Dół",148,y+126,136,reorder,(void *)(uintptr_t)(i*2+1));
  }
  int y=148+visible*182;if(tile_page)dsr_ui_button(body,"Poprzednie karty",0,y,284,tiles_page,(void *)(uintptr_t)(tile_page-1));if(tile_page+1<pages)dsr_ui_button(body,"Następne karty",0,y+56,284,tiles_page,(void *)(uintptr_t)(tile_page+1));
  dsr_ui_button(body,"+ Urządzenie",0,y+112,284,tile_add,(void *)0);dsr_ui_button(body,"+ Pomieszczenie",0,y+168,284,tile_add,(void *)1);dsr_ui_button(body,"+ Scena",0,y+224,284,tile_add,(void *)2);
 }
 if(!room&&!dashboard){int bottom=!strcmp(collection_name,"scenes")?160+cJSON_GetArraySize(item(entity,"actions"))*112:420;dsr_ui_button(body,"Usuń wpis",0,bottom,284,delete_entity,NULL);}actions(root,show_collection);
}
static void add_entity(lv_event_t *event)
{
 (void)event;cJSON *a=cJSON_GetObjectItemCaseSensitive(draft,collection_name);int id=1;for(cJSON *v=a->child;v;v=v->next)if(integer(v,"id")>=id)id=integer(v,"id")+1;
 int limit=!strcmp(collection_name,"dashboards")?4:!strcmp(collection_name,"devices")?16:8;if(cJSON_GetArraySize(a)>=limit){dsr_sc_label_set_text(status_label,"Osiągnięto limit wpisów");return;}
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
 view_save_target=SAVE_MENU;
 cJSON_ArrayForEach(v,a){dsr_ui_button(body,string(v,"name"),0,y,284,edit_event,(void *)v);y+=58;}dsr_ui_button(body,"+ Dodaj wpis",0,y,284,add_entity,NULL);actions(root,menu_event);
}
static const choice_t language_choice={"language","pl\nen\nde",false},theme_choice={"theme","graphite\nmidnight",false};
static const uint32_t colors[]={DSR_SC_BLUE,0x34cbb5,0x8c9dff,0xe4a858};
static void color_selected(lv_event_t *event){unsigned at=lv_dropdown_get_selected(lv_event_get_target(event));if(at<4)cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"accent"),colors[at]);}
static void appearance(lv_event_t *event)
{(void)event;editing=draft;lv_obj_t *root=screen("Wygląd i język");add_choice("Język: Polski / English / Deutsch",&language_choice,0);add_choice("Motyw",&theme_choice,80);dsr_sc_text(body,"Kolor akcentu",2,160,284,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *color=lv_dropdown_create(body);lv_obj_set_pos(color,0,181);lv_obj_set_size(color,284,44);lv_dropdown_set_options(color,dsr_sc_tr("Niebieski\nTurkusowy\nLawendowy\nBursztynowy\nWłasny z Web Panelu"));unsigned chosen=4;for(unsigned i=0;i<4;i++)if((uint32_t)integer(draft,"accent")==colors[i])chosen=i;lv_dropdown_set_selected(color,chosen);lv_obj_add_event_cb(color,color_selected,LV_EVENT_VALUE_CHANGED,NULL);object_choice("Pulpit startowy","active_dashboard",240);actions(root,return_settings);}
static void show_menu(void)
{
 lv_obj_t *root=screen(settings_context?"Konfiguracja wpisów":"Edytuj pilot");
 view_save_target=settings_context?SAVE_SETTINGS:SAVE_HOME;
 const char *labels[]={"Pulpity","Pomieszczenia","Urządzenia","Sceny"},*keys[]={"dashboards","rooms","devices","scenes"};
 unsigned first=settings_context?2:0;
 for(unsigned i=first;i<first+2;i++)dsr_ui_button(body,labels[i],0,(int)(i-first)*56,284,show_collection,(void *)keys[i]);
 actions(root,settings_context?return_settings:return_home);
}
void dsr_ui_config_appearance(lv_event_t *event)
{
 (void)event;if(!dsr_ui_settings_can_open())return;
 settings_context=true;appearance_editor=true;error_return_rooms=false;error_return_settings=true;
 open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 editor_active=true;dashboard_picker=room_editor=room_new=room_deleted=false;room_dialog=NULL;
 appearance(NULL);
}
void dsr_ui_config_registry(lv_event_t *event)
{
 (void)event;if(!dsr_ui_settings_can_open())return;
 settings_context=true;appearance_editor=false;error_return_rooms=false;error_return_settings=true;
 open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 editor_active=true;dashboard_picker=room_editor=room_new=room_deleted=false;room_dialog=NULL;
 show_menu();
}
void dsr_ui_edit_dashboard(void)
{
 settings_context=appearance_editor=error_return_settings=false;error_return_rooms=false;config_log("edit_dashboard",0);open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 room_editor=room_new=room_deleted=false;room_dialog=NULL;editor_active=true;dashboard_picker=false;tile_page=0;config_log("show_menu",0);show_menu();
}
static bool room_begin(void)
{
 settings_context=appearance_editor=error_return_settings=false;error_return_rooms=true;open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return false;}
 editor_active=room_editor=true;dashboard_picker=room_new=room_deleted=false;room_dialog=NULL;
 room_return_detail=false;room_return_id=0;
 snprintf(collection_name,sizeof(collection_name),"rooms");return true;
}
void dsr_ui_add_room(void)
{
 config_log("add_room",0);if(!room_begin())return;
 const cJSON *rooms=item(draft,"rooms");
 if(cJSON_GetArraySize(rooms)>=DSR_UI_MAX_ROOMS){
  lv_obj_t *root=screen("Pomieszczenia");dsr_sc_text(body,"Limit: 8 pomieszczeń. Edytuj lub usuń istniejący pokój.",4,32,284,&dsr_roboto_18,DSR_SC_TEXT);dsr_ui_button(root,"Wstecz",12,420,296,room_leave,NULL);return;
 }
 int id=1;while(object_index(rooms,id)!=255)id++;
 cJSON *room=cJSON_CreateObject();if(!room){config_error(OPEN_OBJECT_MEMORY);return;}
 if(!cJSON_AddNumberToObject(room,"id",id)||!cJSON_AddStringToObject(room,"name","")||!cJSON_AddNumberToObject(room,"icon",0)||!cJSON_AddStringToObject(room,"photo","")){cJSON_Delete(room);config_error(OPEN_OBJECT_MEMORY);return;}
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(draft,"rooms"),room);
 room_new=true;editing=room;show_room_form();
}
void dsr_ui_edit_room(uint32_t id)
{
 bool detail=dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_ROOM;
 if(!room_begin())return;
 room_return_detail=detail;room_return_id=id;
 cJSON *room;cJSON_ArrayForEach(room,cJSON_GetObjectItemCaseSensitive(draft,"rooms"))if((uint32_t)integer(room,"id")==id){editing=room;show_room_form();return;}
 /* A Web edit may remove/reorder rooms before the LCD click is handled. */
 room_leave(NULL);
}
void dsr_ui_edit_device(uint32_t id)
{
 dsr_ui_edit_dashboard();if(!draft)return;
 cJSON *entry;cJSON_ArrayForEach(entry,cJSON_GetObjectItemCaseSensitive(draft,"devices"))if((uint32_t)integer(entry,"id")==id){
  snprintf(collection_name,sizeof(collection_name),"devices");show_entity(entry);return;
 }
}
static void pick_dashboard(lv_event_t *event)
{cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"active_dashboard"),(unsigned)(uintptr_t)lv_event_get_user_data(event));save(NULL);}
void dsr_ui_select_dashboard(void)
{
 settings_context=appearance_editor=error_return_settings=false;error_return_rooms=false;open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 draft_revision=(uint32_t)integer(draft,"revision");editor_active=true;own_save=false;dashboard_picker=true;room_editor=room_new=room_deleted=false;room_dialog=NULL;
 lv_obj_t *root=screen("Wybierz pulpit");int y=0;const cJSON *d;
 view_save_target=SAVE_HOME;
 cJSON_ArrayForEach(d,item(draft,"dashboards")){lv_obj_t *b=dsr_ui_button(body,string(d,"name"),0,y,284,pick_dashboard,(void *)(uintptr_t)integer(d,"id"));if(integer(d,"id")==integer(draft,"active_dashboard"))dsr_ui_primary(b);y+=56;}
 dsr_ui_button(root,"Wstecz",12,420,296,return_home,NULL);
}
void dsr_ui_workspace_tick(void)
{
 if(!editor_active||!draft||!status_label)return;
 bool busy;char message[96];dsr_workspace_status(&busy,message);uint32_t revision=dsr_workspace_revision();
 if(revision!=draft_revision){
  bool acknowledged=false;
  if(own_save&&submitted_draft&&revision==draft_revision+1){
   cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(submitted_draft,"revision"),revision);
   char *json=dsr_workspace_snapshot();cJSON *published=json?cJSON_Parse(json):NULL;free(json);
   acknowledged=published&&cJSON_Compare(published,submitted_draft,true);cJSON_Delete(published);
  }
  if(!acknowledged){
   if(own_save&&!busy){own_save=false;save_failed=true;cJSON_Delete(submitted_draft);submitted_draft=NULL;}
   dsr_sc_label_set_text(status_label,"Zmieniono konfigurację. Wróć i otwórz edycję ponownie.");return;
  }
  draft_revision=revision;own_save=false;save_confirmed=true;save_failed=false;
  cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"revision"),revision);
  cJSON_Delete(saved_baseline);saved_baseline=submitted_draft;submitted_draft=NULL;
  /* A fresh exact durable ACK owns this navigation opportunity. Never use
   * sticky save_confirmed: edits B or an intervening screen must stay put. */
  save_target_t target=submitted_save_target;submitted_save_target=SAVE_STAY;
  bool clean=cJSON_Compare(draft,saved_baseline,true);
  if(clean&&submitted_generation==view_generation&&target!=SAVE_STAY){
   if(target==SAVE_HOME){return_home(NULL);return;}
   if(target==SAVE_ROOMS){room_leave(NULL);return;}
   if(target==SAVE_SETTINGS){return_settings(NULL);return;}
   if(target==SAVE_MENU){show_menu();return;}
   if(target==SAVE_COLLECTION){show_collection(NULL);return;}
   if(target==SAVE_ENTITY){show_entity(editing);return;}
  }
  if(appearance_editor){
   /* Display the acknowledged style A while retaining later draft edits B. */
   const char *locale=string(saved_baseline,"language");
   dsr_sc_appearance((uint32_t)integer(saved_baseline,"accent"),!strcmp(string(saved_baseline,"theme"),"midnight"),!strcmp(locale,"en")?1:!strcmp(locale,"de")?2:0);
   appearance(NULL);
  }
  if(room_editor&&cJSON_Compare(draft,saved_baseline,true)&&(room_new||room_deleted)){room_new=false;show_room_form();}
 }else if(own_save&&!busy){own_save=false;save_failed=true;cJSON_Delete(submitted_draft);submitted_draft=NULL;}
 bool dirty=!saved_baseline||!cJSON_Compare(draft,saved_baseline,true);
 if(own_save)dsr_sc_label_set_text(status_label,"Zapisywanie…");
 else if(save_failed)dsr_sc_label_set_text(status_label,"Zapis nie powiódł się. Zmiany pozostają w edytorze.");
 else if(dirty)dsr_sc_label_set_text(status_label,save_confirmed?"Zapisano wcześniejszą wersję • są niezapisane zmiany":"Niezapisane zmiany");
 else if(save_confirmed)dsr_sc_label_set_text(status_label,"Zapisano • zsynchronizowano");
}
void dsr_ui_workspace_migrate_bound(const dsr_connection_t *connection,uint32_t room,const char *previous_host)
{
 if(!connection||!connection->device.host[0])return;
 char *json=dsr_workspace_snapshot();if(!json)return;cJSON *r=cJSON_Parse(json);free(json);if(!r)return;
 cJSON *a=cJSON_GetObjectItemCaseSensitive(r,"devices");bool found=false,changed=false;int id=1;
 if(room!=UINT32_MAX&&room&&object_index(item(r,"rooms"),(int)room)==255)room=0;
 for(cJSON *v=a->child;v;v=v->next){
  if((!strcmp(string(v,"host"),connection->device.host)||(previous_host&&previous_host[0]&&!strcmp(string(v,"host"),previous_host)))&&integer(v,"channel")==connection->device.channel){
   found=true;
   if(strcmp(string(v,"host"),connection->device.host)){cJSON_ReplaceItemInObjectCaseSensitive(v,"host",cJSON_CreateString(connection->device.host));changed=true;}
   if(room!=UINT32_MAX){cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(v,"room"),room);cJSON_ReplaceItemInObjectCaseSensitive(v,"name",cJSON_CreateString(connection->device.name));changed=true;}
  }if(integer(v,"id")>=id)id=integer(v,"id")+1;
 }
 if(!found&&cJSON_GetArraySize(a)<16){changed=true;cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"id",id);cJSON_AddStringToObject(d,"name",connection->device.name[0]?connection->device.name:"Shelly");cJSON_AddStringToObject(d,"host",connection->device.host);cJSON_AddNumberToObject(d,"channel",connection->device.channel);cJSON_AddNumberToObject(d,"room",room==UINT32_MAX?0:room);cJSON_AddStringToObject(d,"kind","relay");cJSON_AddItemToArray(a,d);
  cJSON *dash=cJSON_GetArrayItem(item(r,"dashboards"),(int)object_index(item(r,"dashboards"),integer(r,"active_dashboard"))),*tiles=cJSON_GetObjectItemCaseSensitive(dash,"tiles"),*t=cJSON_CreateObject();int tile_id=1;for(cJSON *v=tiles->child;v;v=v->next)if(integer(v,"id")>=tile_id)tile_id=integer(v,"id")+1;
  cJSON_AddNumberToObject(t,"id",tile_id);cJSON_AddStringToObject(t,"type","device");cJSON_AddNumberToObject(t,"ref",id);cJSON_AddNumberToObject(t,"span",2);cJSON_AddNumberToObject(t,"height",1);cJSON_AddNumberToObject(t,"fields",255);if(cJSON_GetArraySize(tiles)<24)cJSON_AddItemToArray(tiles,t);else cJSON_Delete(t);
 }
 if(changed){char *next=cJSON_PrintUnformatted(r);if(next){dsr_workspace_submit(next,(uint32_t)integer(r,"revision"));free(next);}}
 cJSON_Delete(r);
}
void dsr_ui_workspace_migrate(const dsr_connection_t *connection){dsr_ui_workspace_migrate_bound(connection,UINT32_MAX,NULL);}
#endif
