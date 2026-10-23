#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
#include "dsr_ui_search.h"
#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#else
#define EXT_RAM_BSS_ATTR
#endif

static const cJSON *item(const cJSON *o,const char *k){return cJSON_GetObjectItemCaseSensitive(o,k);}
static int integer(const cJSON *o,const char *k){const cJSON *v=item(o,k);return cJSON_IsNumber(v)?v->valueint:0;}
static const char *string(const cJSON *o,const char *k){const cJSON *v=item(o,k);return cJSON_IsString(v)?v->valuestring:"";}
#ifndef DSR_UI_REVIEW_DEMO
static const char *display_name(const cJSON *device)
{
 const char *name=string(device,"name");if(name[0])return name;
 static char fallback[48];
 if(dsr_workspace_runtime_name((uint32_t)integer(device,"id"),string(device,"host"),(uint8_t)integer(device,"channel"),fallback))return fallback;
 snprintf(fallback,sizeof(fallback),"Shelly · %.15s/%u",string(device,"host"),(unsigned)integer(device,"channel"));return fallback;
}
#endif
static unsigned object_index(const cJSON *array,int id)
{unsigned i=0;const cJSON *v;cJSON_ArrayForEach(v,array){if(integer(v,"id")==id)return i;i++;}return 255;}
void dsr_mobile_configure_validated(dsr_mobile_model_t *m,const cJSON *r)
{
 m->room_count=0;m->device_count=0;m->tile_count=0;m->configured_layout=true;m->accent=(uint32_t)integer(r,"accent");const unsigned style=dsr_sc_style_from_name(string(r,"theme"));
 const char *locale=string(r,"language");dsr_sc_appearance(m->accent,style,!strcmp(locale,"en")?1:!strcmp(locale,"de")?2:0);dsr_ui_apply_theme();
 snprintf(m->home_name,sizeof(m->home_name),"%s",dsr_sc_tr("Mój dom"));
 const cJSON *v;cJSON_ArrayForEach(v,item(r,"rooms")){
  dsr_mobile_room_t *room=&m->rooms[m->room_count++];snprintf(room->name,sizeof(room->name),"%s",string(v,"name"));room->icon=(uint8_t)integer(v,"icon");snprintf(room->photo,sizeof(room->photo),"%.32s",string(v,"photo"));room->id=(uint32_t)integer(v,"id");
 }
 m->scene_count=0;cJSON_ArrayForEach(v,item(r,"scenes")){dsr_mobile_scene_t *s=&m->scenes[m->scene_count++];snprintf(s->name,sizeof(s->name),"%s",string(v,"name"));s->actions=(uint8_t)cJSON_GetArraySize(item(v,"actions"));s->id=(uint32_t)integer(v,"id");}
 m->group_count=0;cJSON_ArrayForEach(v,item(r,"groups")){dsr_mobile_group_t *g=&m->groups[m->group_count++];snprintf(g->name,sizeof(g->name),"%s",string(v,"name"));g->id=(uint32_t)integer(v,"id");g->members=(uint8_t)cJSON_GetArraySize(item(v,"members"));}
 cJSON_ArrayForEach(v,item(r,"devices")){
  dsr_mobile_device_t *d=&m->devices[m->device_count++];memset(d,0,sizeof(*d));d->id=(uint32_t)integer(v,"id");snprintf(d->name,sizeof(d->name),"%s",string(v,"name"));snprintf(d->host,sizeof(d->host),"%s",string(v,"host"));d->channel=(uint8_t)integer(v,"channel");d->room=(uint8_t)object_index(item(r,"rooms"),integer(v,"room"));
  const char *kind=string(v,"kind");d->kind=!strcmp(kind,"light")?DSR_DEVICE_LIGHT:!strcmp(kind,"cover")?DSR_DEVICE_COVER:!strcmp(kind,"contact")?DSR_DEVICE_CONTACT:!strcmp(kind,"sensor")?DSR_DEVICE_SENSOR:!strcmp(kind,"plug")?DSR_DEVICE_PLUG:DSR_DEVICE_RELAY;d->fields=255;{int icon=integer(v,"icon");d->icon=(uint8_t)(icon>0&&icon<=DSR_SC_DEVICE_ICONS?icon:0);}
 }
 const cJSON *dashboard=cJSON_GetArrayItem(item(r,"dashboards"),(int)object_index(item(r,"dashboards"),integer(r,"active_dashboard")));
 m->dashboard_id=(uint32_t)integer(dashboard,"id");
 snprintf(m->dashboard_name,sizeof(m->dashboard_name),"%s",string(dashboard,"name"));
 cJSON_ArrayForEach(v,item(dashboard,"tiles")){
  dsr_mobile_tile_t *t=&m->tiles[m->tile_count++];const char *type=string(v,"type");t->type=!strcmp(type,"device")?0:!strcmp(type,"room")?1:!strcmp(type,"scene")?2:3;
  t->index=(uint8_t)object_index(item(r,t->type==0?"devices":t->type==1?"rooms":t->type==2?"scenes":"groups"),integer(v,"ref"));t->span=(uint8_t)integer(v,"span");t->height=(uint8_t)integer(v,"height");t->fields=(uint8_t)integer(v,"fields");
 }
}
bool dsr_mobile_configure(dsr_mobile_model_t *m,const char *json)
{
 if(!json||!dsr_config_json_depth(json))return false;
 cJSON *r=cJSON_Parse(json);char error[96];
 if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);return false;}
 dsr_mobile_configure_validated(m,r);cJSON_Delete(r);return true;
}

#ifndef DSR_UI_REVIEW_DEMO
static EXT_RAM_BSS_ATTR struct {
 bool active,fresh,deleted,selecting;uint8_t kind,step,gallery_page,return_step,edit_index;uint32_t pending_device;
 char query[48];lv_obj_t *count,*next,*back;lv_image_dsc_t photos[12];
} action_editor;
_Static_assert(sizeof(action_editor)<=1024,"bounded contextual editor PSRAM metadata");
static void action_form(void);
static void action_leave(lv_event_t *event);
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
/* Contextual name/room editor ported from PR34 (693d869); all persistence and
 * migration remain the current shared workspace/CAS implementation. */
static bool device_editor;
static uint32_t context_device_id;
static uint32_t deletion_id;
static lv_obj_t *deletion_state;
static const char *device_input_error;
static lv_obj_t *room_dialog,*settings_dialog;
static bool settings_context,appearance_editor;
static unsigned tile_page;
static int tile_focus=-1;
static uint32_t device_order_focus;
static cJSON *fields_tile;
typedef enum {SAVE_STAY,SAVE_HOME,SAVE_ROOMS,SAVE_MENU,SAVE_COLLECTION,SAVE_SETTINGS,SAVE_ENTITY,SAVE_DEVICE,SAVE_ACTIONS,SAVE_DEVICES} save_target_t;
static save_target_t view_save_target,submitted_save_target;
static uint32_t view_generation,submitted_generation;
static bool room_return_detail;
static uint32_t room_return_id;
typedef enum {OPEN_OK,OPEN_STORAGE,OPEN_SNAPSHOT_MEMORY,OPEN_PARSE,OPEN_INVALID,OPEN_BASELINE_MEMORY,OPEN_OBJECT_MEMORY,OPEN_PANEL} open_result_t;
static open_result_t open_error;
static bool error_return_rooms,error_return_settings;
static void config_status(const char *text,bool important)
{
 if(!status_label)return;
 dsr_sc_label_set_text(status_label,text);
 lv_obj_set_style_text_color(status_label,dsr_sc_text_color(important?(dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xf0a020:0xffc107):DSR_SC_MUTED),0);
}
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
{memset(&action_editor,0,sizeof(action_editor));cJSON_Delete(draft);cJSON_Delete(submitted_draft);cJSON_Delete(saved_baseline);draft=submitted_draft=saved_baseline=editing=NULL;own_save=false;save_failed=save_confirmed=false;device_input_error=NULL;deletion_id=0;deletion_state=NULL;submitted_save_target=view_save_target=SAVE_STAY;}
void dsr_ui_workspace_hide(void){editor_active=false;room_editor=room_new=room_deleted=device_editor=false;context_device_id=0;room_return_detail=false;room_return_id=0;room_dialog=settings_dialog=NULL;body=keyboard=status_label=NULL;settings_context=appearance_editor=false;fields_tile=NULL;tile_focus=-1;device_order_focus=0;clear_draft();}
static void settings_stay(lv_event_t *event)
{(void)event;if(settings_dialog){lv_obj_delete(settings_dialog);settings_dialog=NULL;}}
static void settings_discard(lv_event_t *event)
{(void)event;dsr_ui_workspace_hide();dsr_ui_open_settings();}
bool dsr_ui_settings_can_open(void)
{
 if(own_save){if(status_label)config_status("Poczekaj na zakończenie zapisu.",true);return false;}
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
static void show_device_form(void);
static void delete_entity(lv_event_t *event);
static void close_keyboard(lv_event_t *event)
{
 (void)event;
 if(keyboard){
  if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){lv_obj_t *field=lv_keyboard_get_textarea(keyboard);if(field)lv_obj_remove_state(field,LV_STATE_FOCUSED);lv_keyboard_set_textarea(keyboard,NULL);}
  lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
 }
 if(body)lv_obj_set_height(body,dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE||action_editor.active||!strcmp(collection_name,"dashboards")?398-lv_obj_get_y(body):292);
}
static void focus(lv_event_t *event)
{
 if(!keyboard)return;
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 if(!hud){lv_obj_set_height(body,action_editor.active?94:130);if(action_editor.active){if(action_editor.next)lv_obj_add_flag(action_editor.next,LV_OBJ_FLAG_HIDDEN);if(action_editor.back)lv_obj_add_flag(action_editor.back,LV_OBJ_FLAG_HIDDEN);}}
 dsr_ui_keyboard_focus(keyboard,lv_event_get_target(event));
 dsr_ui_keyboard_editor(keyboard,body);lv_obj_scroll_to_view(lv_event_get_target(event),LV_ANIM_OFF);
}
static bool action_name_valid(const char *name)
{const char *start=name;while(*start==' ')start++;return *start&&strlen(name)<=47;}
static void set_text(lv_event_t *event)
{const char *key=lv_event_get_user_data(event);lv_obj_t *field=lv_event_get_target(event);if(!editing)return;device_input_error=NULL;cJSON_ReplaceItemInObjectCaseSensitive(editing,key,cJSON_CreateString(lv_textarea_get_text(field)));
 if(action_editor.active&&action_editor.next&&!strcmp(key,"name")){if(action_name_valid(string(editing,"name")))lv_obj_remove_state(action_editor.next,LV_STATE_DISABLED);else lv_obj_add_state(action_editor.next,LV_STATE_DISABLED);}}
static void set_number(lv_event_t *event)
{const char *key=lv_event_get_user_data(event);if(!editing)return;cJSON_ReplaceItemInObjectCaseSensitive(editing,key,cJSON_CreateNumber(strtoul(lv_textarea_get_text(lv_event_get_target(event)),NULL,10)));}
static lv_obj_t *screen(const char *title)
{
 view_generation++;view_save_target=SAVE_STAY;room_dialog=settings_dialog=NULL;
 dsr_network_reset_view();dsr_mobile_hide();lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_set_style_text_font(root,&dsr_roboto_14,0);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 dsr_sc_header(root,"",false,dsr_wifi_connected(),dsr_ui_home,NULL);dsr_sc_text(root,title,12,46,296,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_panel(root,12,75,296,1,DSR_SC_BLUE,0);
 status_label=dsr_sc_text(root,"Zmiany zostaną zastosowane po zapisaniu",14,82,292,&dsr_roboto_12,DSR_SC_MUTED);
 body=dsr_sc_panel(root,12,106,296,292,DSR_SC_BG,0);lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 keyboard=lv_keyboard_create(root);dsr_ui_keyboard(keyboard);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_READY,NULL);lv_obj_add_event_cb(keyboard,close_keyboard,LV_EVENT_CANCEL,NULL);lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
 return root;
}
static void return_home(lv_event_t *e){(void)e;dsr_ui_workspace_hide();dsr_network_home(NULL);}
static void error_back(lv_event_t *e){(void)e;bool settings=error_return_settings,rooms=error_return_rooms;dsr_ui_workspace_hide();if(settings)dsr_ui_open_settings();else if(rooms)dsr_network_rooms();else dsr_network_home(NULL);}
static void config_error(open_result_t result);
static int diagnostic_text_height(const char *text,const lv_font_t *font)
{
 lv_point_t size;lv_text_get_size(&size,dsr_sc_tr(text),font,0,0,226,LV_TEXT_FLAG_NONE);return size.y;
}
static lv_obj_t *diagnostic_card(int y,int height,dsr_ui_icon_t icon,uint32_t color)
{
 lv_obj_t *card=dsr_sc_panel(body,0,y,296,height,DSR_SC_CARD,10);
 lv_obj_t *well=dsr_sc_panel(card,5,5,42,36,DSR_SC_PANEL,8);dsr_ui_cyber_inset(well);
 dsr_sc_icon_centered(well,icon,32,icon==DSR_ICON_WARNING?color:DSR_SC_GLOW);
 dsr_sc_border_color(card,dsr_sc_color(icon==DSR_ICON_WARNING?color:DSR_SC_GLOW),0);return card;
}
static int diagnostic_description(int y,dsr_ui_icon_t icon,const char *title,const char *text)
{
 int height=42+diagnostic_text_height(text,&dsr_roboto_14);
 lv_obj_t *card=diagnostic_card(y,height,icon,DSR_SC_GLOW);
 dsr_sc_text(card,title,56,9,226,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_panel(card,56,32,226,1,DSR_SC_BLUE,0);
 dsr_sc_text(card,text,56,38,226,&dsr_roboto_14,DSR_SC_MUTED);return y+height+8;
}
static void diagnostics(lv_event_t *event)
{
 (void)event;dsr_config_health_t h;dsr_config_health_snapshot(&h);
 const char *storage=h.storage==DSR_STORAGE_READY?"SPIFFS: zamontowany":h.storage==DSR_STORAGE_NONEMPTY?"SPIFFS: niepusty, niezamontowany":h.storage==DSR_STORAGE_READ_ERROR?"SPIFFS: błąd odczytu":h.storage==DSR_STORAGE_PROVISION_ERROR?"SPIFFS: błąd inicjalizacji":"SPIFFS: niedostępny";
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){
  lv_obj_t *root=dsr_ui_screen("Diagnostyka konfiguracji",false);
  status_label=dsr_sc_text(root,"Tylko odczyt • dane zachowane",14,80,292,&dsr_roboto_12,DSR_SC_MUTED);
  body=dsr_sc_panel(root,12,104,296,304,DSR_SC_BG,0);lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
  uint32_t storage_color=h.storage==DSR_STORAGE_READY?0x72dfb0:0xffc107;
  int height=diagnostic_text_height(storage,&dsr_roboto_16)+20;if(height<48)height=48;
  lv_obj_t *card=diagnostic_card(0,height,DSR_ICON_CPU,storage_color);
  dsr_sc_text(card,storage,56,10,226,&dsr_roboto_16,storage_color);int y=height+8;
  card=diagnostic_card(y,98,DSR_ICON_PSRAM,DSR_SC_GLOW);
  dsr_sc_text(card,"Pamięć SPIFFS",56,9,226,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_panel(card,56,32,226,1,DSR_SC_BLUE,0);
  const char *names[]={"Storage:","Workspace:","HTTPS:"};const int states[]={(int)h.storage,(int)h.workspace,(int)h.panel};const unsigned errors[]={(unsigned)h.storage_error,(unsigned)h.workspace_error,(unsigned)h.panel_error};
  for(unsigned i=0;i<3;i++){char code[32];snprintf(code,sizeof(code),"%d / 0x%x",states[i],errors[i]);dsr_sc_text(card,names[i],56,37+(int)i*18,98,&dsr_roboto_14,DSR_SC_MUTED);dsr_sc_text(card,code,158,37+(int)i*18,124,&dsr_roboto_14,DSR_SC_TEXT);}
  y+=106;
  const char *https=h.panel==DSR_PANEL_READY?"HTTPS uruchomiony":h.panel==DSR_PANEL_FAILED?"HTTPS: błąd uruchomienia":"HTTPS nie został uruchomiony";
  height=diagnostic_text_height(https,&dsr_roboto_16)+20;if(height<48)height=48;
  uint32_t https_color=h.panel==DSR_PANEL_READY?0x72dfb0:0xffc107;
  card=diagnostic_card(y,height,DSR_ICON_SHIELD,https_color);dsr_sc_text(card,https,56,10,226,&dsr_roboto_16,https_color);y+=height+8;
  y=diagnostic_description(y,DSR_ICON_USB,"Port USB","USB 115200 • zachowaj kopię partycji przed naprawą.");
  y=diagnostic_description(y,DSR_ICON_DOCUMENT,"Adresy danych","dsrdata: 0xC20000 + 0x3E0000\nNVS: 0x9000 + 0x10000");
  const char *warning="Nie kasuj i nie formatuj pamięci.";height=diagnostic_text_height(warning,&dsr_roboto_14)+20;if(height<48)height=48;
  card=diagnostic_card(y,height,DSR_ICON_WARNING,0xffc107);dsr_ui_cyber_warning(card,false);dsr_sc_text(card,warning,56,10,226,&dsr_roboto_14,0xffc107);
  dsr_ui_button(root,"Wstecz",12,420,296,error_back,NULL);config_log("diagnostics",open_error);return;
 }
 lv_obj_t *root=screen("Diagnostyka konfiguracji");config_status("Tylko odczyt • dane zachowane",true);
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
 clear_draft();editor_active=false;room_editor=room_new=room_deleted=device_editor=false;context_device_id=0;room_dialog=settings_dialog=NULL;open_error=result;
 lv_obj_t *root=screen("Konfiguracja niedostępna");config_status("Nie można otworzyć edytora",true);
 dsr_config_health_t h;dsr_config_health_snapshot(&h);
 const char *reason=result==OPEN_PANEL?"Serwer HTTPS jest niedostępny. Kod parowania nie został utworzony.":result==OPEN_STORAGE?(h.storage==DSR_STORAGE_NONEMPTY?"Magazyn konfiguracji niedostępny: niepusta partycja SPIFFS nie została zamontowana.":"Magazyn konfiguracji jest niedostępny."):result==OPEN_PARSE||result==OPEN_INVALID?"Nie można odczytać poprawnej konfiguracji. Sprawdź pamięć i kopię danych.":"Brak pamięci do otwarcia edytora. Ustawienia pozostają zachowane.";
 lv_obj_t *message=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?dsr_sc_panel(body,0,0,296,284,DSR_SC_CARD,12):body;
 dsr_sc_text(message,reason,12,16,268,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(message,"Dane nie zostały skasowane. Szczegóły są w diagnostyce i logu USB.",12,153,268,&dsr_roboto_14,DSR_SC_MUTED);
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
 (void)event;if(own_save){config_status("Zapisywanie…",false);return;}
 if((room_editor||action_editor.active)&&editing&&(!string(editing,"name")[0]||strlen(string(editing,"name"))>47||(action_editor.active&&!action_name_valid(string(editing,"name"))))){
  config_status(action_editor.active?"Podaj nazwę (maks. 47 bajtów).":!string(editing,"name")[0]?"Podaj nazwę pomieszczenia.":"Podaj krótszą nazwę pomieszczenia.",true);return;
 }
 if(device_editor&&editing&&strlen(string(editing,"name"))>47){
  device_input_error=!string(editing,"name")[0]?"Podaj nazwę urządzenia.":"Podaj krótszą nazwę urządzenia.";config_status(device_input_error,true);return;
 }
 char error[96];if(!dsr_config_validate(draft,error)){config_status("Nieprawidłowa konfiguracja. Sprawdź pola i powiązania.",true);return;}
 cJSON *snapshot=cJSON_Duplicate(draft,true);char *json=snapshot?cJSON_PrintUnformatted(snapshot):NULL;
 bool ok=json&&dsr_workspace_submit(json,draft_revision);free(json);
 if(ok){cJSON_Delete(submitted_draft);submitted_draft=snapshot;own_save=true;save_failed=false;submitted_save_target=view_save_target;submitted_generation=view_generation;}else{cJSON_Delete(snapshot);save_failed=true;}
 config_status(ok?"Zapisywanie…":"Konflikt lub zapis w toku. Wczytaj z pilota.",!ok);
}
static void actions(lv_obj_t *root,lv_event_cb_t back)
{dsr_ui_button(root,"Wstecz",12,420,140,back,NULL);dsr_ui_button(root,"Zapisz",168,420,140,save,NULL);}
static void menu_event(lv_event_t *e){(void)e;show_menu();}
static void add_field(const char *caption,const char *key,int y,bool numeric)
{
 dsr_sc_text(body,caption,2,y-1,280,&dsr_roboto_14,DSR_SC_MUTED);lv_obj_t *f=lv_textarea_create(body);dsr_ui_textarea(f);lv_obj_set_pos(f,0,y+21);lv_obj_set_size(f,284,44);lv_textarea_set_one_line(f,true);
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
 dsr_sc_text(body,caption,2,y-1,284,&dsr_roboto_14,DSR_SC_MUTED);lv_obj_t *d=lv_dropdown_create(body);lv_obj_set_pos(d,0,y+21);lv_obj_set_size(d,284,44);
 const char *options=!strcmp(choice->key,"kind")?"Oświetlenie\nPrzekaźnik\nGniazdko\nRoleta\nCzujnik otwarcia\nCzujnik":!strcmp(choice->key,"theme")?"Grafit\nJasny\nCyber Blue":!strcmp(choice->key,"language")?"Polski\nEnglish\nDeutsch":choice->options;
 lv_dropdown_set_options(d,dsr_sc_tr(options));lv_obj_set_style_text_font(d,dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?&dsr_roboto_16:&dsr_roboto_14,0);
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
 cJSON_ArrayForEach(v,a){if(options[0])strncat(options,"\n",sizeof(options)-strlen(options)-1);strncat(options,display_name(v),sizeof(options)-strlen(options)-1);if(integer(v,"id")==integer(editing,key))selected=index;index++;}
 dsr_sc_text(body,caption,2,y-1,284,&dsr_roboto_14,DSR_SC_MUTED);lv_obj_t *d=lv_dropdown_create(body);lv_obj_set_pos(d,0,y+21);lv_obj_set_size(d,284,44);lv_dropdown_set_options(d,options);lv_dropdown_set_selected(d,selected);lv_obj_add_event_cb(d,object_selected,LV_EVENT_VALUE_CHANGED,(void *)key);
}
/* Gallery order: uploaded photos, then built-in room photos, then "none". */
static const char *gallery_id(unsigned index)
{
 unsigned users=0;while(dsr_photo_id(users))users++;
 if(index<users)return dsr_photo_id(index);
 const dsr_room_photo_t *builtin=dsr_room_photo(index-users);return builtin?builtin->id:NULL;
}
static void choose_photo(lv_event_t *event)
{
 (void)event;unsigned index=0;const char *id;
 while((id=gallery_id(index))){if(!strcmp(id,string(editing,"photo"))){index++;break;}index++;}
 if(!string(editing,"photo")[0])index=0;
 id=gallery_id(index);
 cJSON_ReplaceItemInObjectCaseSensitive(editing,"photo",cJSON_CreateString(id?id:""));show_entity(editing);
}
/* Contextual rooms use the same draft, CAS submission and durable acknowledgement
 * as the full editor. Only their entry points and navigation differ. */
static bool room_dirty(void){return draft&&(!saved_baseline||!cJSON_Compare(draft,saved_baseline,true));}
static void room_leave(lv_event_t *event)
{(void)event;if(device_editor){dsr_network_device_settings(context_device_id);return;}bool detail=room_return_detail;uint32_t id=room_return_id;if(detail)dsr_network_room_context(id);else dsr_network_rooms();}
static void room_dialog_close(lv_event_t *event)
{(void)event;if(room_dialog){lv_obj_delete(room_dialog);room_dialog=NULL;}}
static lv_obj_t *room_modal(const char *title,const char *text)
{
 close_keyboard(NULL);room_dialog_close(NULL);
 room_dialog=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);lv_obj_set_style_bg_opa(room_dialog,180,0);lv_obj_add_flag(room_dialog,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *panel=dsr_sc_panel(room_dialog,12,117,296,246,DSR_SC_PANEL,14);
 dsr_sc_text(panel,title,14,16,268,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(panel,text,14,53,268,&dsr_roboto_14,dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xf0a020:0xffc107);return panel;
}
static void room_back(lv_event_t *event)
{
 (void)event;if(own_save){config_status("Poczekaj na zakończenie zapisu.",true);return;}
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
 (void)event;if(own_save){config_status("Poczekaj na zakończenie zapisu.",true);return;}
 unsigned devices=0,tiles=0;const cJSON *d,*t;int id=integer(editing,"id");
 cJSON_ArrayForEach(d,item(draft,"devices"))if(integer(d,"room")==id)devices++;
 cJSON_ArrayForEach(d,item(draft,"dashboards"))cJSON_ArrayForEach(t,item(d,"tiles"))if(!strcmp(string(t,"type"),"room")&&integer(t,"ref")==id)tiles++;
 const cJSON *object;const char *collections[]={"groups","scenes"};
 for(unsigned i=0;i<2;i++)cJSON_ArrayForEach(object,item(draft,collections[i]))if(integer(object,"room")==id)tiles++;
 if(devices||tiles){
  char text[224];snprintf(text,sizeof(text),dsr_sc_tr("Urządzenia: %u • karty pomieszczenia: %u. Przenieś urządzenia i usuń te karty z pulpitów. Sceny i urządzenia pozostaną zachowane."),devices,tiles);
  lv_obj_t *panel=room_modal("Pomieszczenie jest używane",text);dsr_ui_button(panel,"Wstecz",12,188,272,room_dialog_close,NULL);return;
 }
 lv_obj_t *panel=room_modal("Usunąć pomieszczenie?","Po potwierdzeniu wybierz Zapisz. Zdjęcie może być używane przez inne pokoje.");
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){
  lv_obj_t *description=lv_obj_get_child(panel,1);lv_obj_set_y(description,104);
  lv_obj_t *name=dsr_sc_text(panel,dsr_sc_user(string(editing,"name")),14,52,268,&dsr_roboto_18,DSR_SC_TEXT);lv_obj_set_height(name,44);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);
 }
 dsr_ui_button(panel,"Anuluj",12,188,130,room_dialog_close,NULL);dsr_ui_button(panel,"Usuń",154,188,130,room_delete_confirm,NULL);
}
static void room_photo(void)
{
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 lv_obj_t *button=dsr_sc_button(body,0,150,284,hud?78:72,DSR_SC_CARD,12,choose_photo,NULL);
 lv_obj_t *thumb=dsr_sc_panel(button,8,8,64,48,DSR_SC_PANEL,6);lv_obj_set_style_clip_corner(thumb,true,0);
 static lv_image_dsc_t photo;
 if(dsr_ui_photo_source(string(editing,"photo"),&photo)){
  uint32_t scale=64u*256u/photo.header.w,vertical=48u*256u/photo.header.h;if(vertical>scale)scale=vertical; /* cover the 64x48 thumb */
  lv_obj_t *image=lv_image_create(thumb);lv_image_set_src(image,&photo);lv_image_set_scale(image,scale);lv_image_set_antialias(image,true);lv_obj_center(image);
 }else dsr_sc_icon_centered(thumb,DSR_ICON_HOME,32,DSR_SC_MUTED);
 const dsr_room_photo_t *builtin=dsr_room_photo_find(string(editing,"photo"));
 lv_obj_t *caption=dsr_sc_text(button,builtin?builtin->label:string(editing,"photo")[0]?"Zmień zdjęcie z pamięci":"Wybierz zdjęcie z pamięci",84,hud?8:10,188,&dsr_roboto_14,DSR_SC_TEXT);
 if(hud){lv_obj_set_height(caption,18);lv_label_set_long_mode(caption,LV_LABEL_LONG_MODE_DOTS);}
 /* Upload hint inside the card: no free text that a fixed footer can clip. */
 dsr_sc_text(button,"Dotknij, aby zmienić. Własne zdjęcia dodasz w Web Panelu.",84,hud?32:34,188,&dsr_roboto_12,DSR_SC_MUTED);
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
  /* All controls fit above the fixed footer. Cyber's photo hint has its own
   * reserved text area; Back/Save also remain usable with the keyboard. */
  add_field("Nazwa","name",0,false);add_choice("Ikona",&icon_choice,74);
  room_photo();
  if(!room_new)dsr_ui_button(body,"Usuń pomieszczenie",0,232,284,room_delete,NULL);
 }
 if(deleted_saved)dsr_ui_button(root,"Wstecz",12,420,296,room_back,NULL);else actions(root,room_back);
}
/* Icon gallery like the Shelly app "Standardbilder" (owner 2026-10-18).
 * "icon" is written only when not automatic, so unchanged configurations stay
 * readable by 0.7.50 and older. */
static void device_icon_pick(lv_event_t *event)
{
 int icon=(int)(uintptr_t)lv_event_get_user_data(event);if(!editing||icon<0||icon>DSR_SC_DEVICE_ICONS)return;
 cJSON_DeleteItemFromObjectCaseSensitive(editing,"icon");if(icon&&!cJSON_AddNumberToObject(editing,"icon",icon))return;
 int scroll=body?lv_obj_get_scroll_y(body):0;show_device_form();if(body){lv_obj_update_layout(body);lv_obj_scroll_to_y(body,scroll,LV_ANIM_OFF);}
}
static void device_icons(int y)
{
 dsr_sc_text(body,"Ikona",2,y-1,280,&dsr_roboto_14,DSR_SC_MUTED);
 int chosen=integer(editing,"icon");if(chosen<0||chosen>DSR_SC_DEVICE_ICONS)chosen=0;
 for(int i=0;i<=DSR_SC_DEVICE_ICONS;i++){
  lv_obj_t *b=dsr_sc_button(body,(i%5)*57+2,y+24+(i/5)*58,50,50,0x17191c,LV_RADIUS_CIRCLE,device_icon_pick,(void *)(uintptr_t)i);
  dsr_sc_border_width(b,i==chosen?3:1,0);dsr_sc_border_color(b,dsr_sc_color(i==chosen?DSR_SC_GLOW:0x3a3f44),0);dsr_sc_glow(b,i==chosen);
  if(i)dsr_sc_icon_centered(b,dsr_sc_device_icons[i-1],28,i==chosen?DSR_SC_GLOW:dsr_sc_icon_accent());
  else{lv_obj_t *t=dsr_sc_text(b,"Auto",0,0,44,&dsr_roboto_12,i==chosen?DSR_SC_GLOW:DSR_SC_TEXT);lv_obj_set_style_text_align(t,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(t);}
 }
 dsr_sc_text(body,"Nazwa, pokój i ikona dotyczą pilota. Konfiguracja samego Shelly wymaga obsługi API danego modelu.",2,y+24+5*58+4,280,&dsr_roboto_14,DSR_SC_MUTED);
}
static void use_shelly_name(lv_event_t *event)
{(void)event;if(editing){cJSON_ReplaceItemInObjectCaseSensitive(editing,"name",cJSON_CreateString(""));show_device_form();}}
static void show_device_form(void)
{
 lv_obj_t *root=screen("Edytuj kartę urządzenia");room_dialog=NULL;
 view_save_target=SAVE_DEVICE;
 lv_obj_set_y(body,118);lv_obj_set_height(body,280);
 add_field("Nazwa","name",0,false);object_choice("Pomieszczenie","room",80);
 dsr_ui_button(body,"Użyj nazwy z Shelly",0,148,284,use_shelly_name,NULL);
 device_icons(208);
 dsr_ui_button(body,"Usuń urządzenie",0,584,284,delete_entity,NULL);
 actions(root,room_back);
}
static void scene_action_cycle(lv_event_t *event)
{
 cJSON *a=lv_event_get_user_data(event);const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),integer(a,"device")));bool cover=!strcmp(string(d,"kind"),"cover");const char *commands[]={"on","off","open","close","stop"};unsigned first=cover?2:0,count=cover?3:2,at=0;
 while(at<count&&strcmp(string(a,"command"),commands[first+at]))at++;
 cJSON *next=cJSON_CreateString(commands[first+(at+1)%count]);if(next)cJSON_ReplaceItemInObjectCaseSensitive(a,"command",next);show_entity(editing);
}
static void scene_device_cycle(lv_event_t *event)
{
 cJSON *a=lv_event_get_user_data(event);const cJSON *devices=item(draft,"devices");int count=cJSON_GetArraySize(devices);if(!count)return;unsigned at=object_index(devices,integer(a,"device"));const cJSON *d=cJSON_GetArrayItem(devices,(int)((at+1)%(unsigned)count));cJSON *next=cJSON_CreateString(!strcmp(string(d,"kind"),"cover")?"open":"on");if(!next)return;cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(a,"device"),integer(d,"id"));cJSON_ReplaceItemInObjectCaseSensitive(a,"command",next);show_entity(editing);
}
static void scene_action_remove(lv_event_t *event){cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(editing,"actions"),(int)(uintptr_t)lv_event_get_user_data(event));show_entity(editing);}
static void scene_action_add(lv_event_t *event)
{
 (void)event;cJSON *actions=cJSON_GetObjectItemCaseSensitive(editing,"actions");const cJSON *devices=item(draft,"devices");if(!devices->child||cJSON_GetArraySize(actions)>=8){dsr_sc_label_set_text(status_label,"Dodaj urządzenie; limit sceny: 8 działań");return;}
 cJSON *a=cJSON_CreateObject();cJSON_AddNumberToObject(a,"device",integer(devices->child,"id"));cJSON_AddStringToObject(a,"command",!strcmp(string(devices->child,"kind"),"cover")?"open":"on");cJSON_AddItemToArray(actions,a);show_entity(editing);
}
static void group_member(lv_event_t *event)
{
 uint32_t id=(uint32_t)(uintptr_t)lv_event_get_user_data(event);cJSON *members=cJSON_GetObjectItemCaseSensitive(editing,"members");int at=0;for(cJSON *v=members->child;v;v=v->next,at++)if((uint32_t)v->valueint==id){cJSON_DeleteItemFromArray(members,at);return;}
 const cJSON *device;bool supported=false;cJSON_ArrayForEach(device,item(draft,"devices"))if((uint32_t)integer(device,"id")==id)supported=!strcmp(string(device,"kind"),"relay")||!strcmp(string(device,"kind"),"plug");
 if(!supported){lv_obj_remove_state(lv_event_get_target(event),LV_STATE_CHECKED);return;}
 if(cJSON_GetArraySize(members)>=16)return;
 cJSON *value=cJSON_CreateNumber(id);if(value)cJSON_AddItemToArray(members,value);
}
static bool entity_deletable(void)
{
 if(own_save){config_status("Poczekaj na zakończenie zapisu.",true);return false;}
 if(!draft||!editing)return false;
 const cJSON *a=item(draft,collection_name),*member;bool present=false;
 cJSON_ArrayForEach(member,a)if(member==editing)present=true;
 if(!present)return false;
 /* Device removal cleans exact references in one workspace CAS. A running
  * bind may still be publishing its locator and must finish first. */
 if(!strcmp(collection_name,"devices")){
  if(dsr_network_bind_busy()){config_status("Poczekaj na zakończenie dodawania urządzenia.",true);return false;}
  return true;
 }
 int id=integer(editing,"id");const cJSON *v,*t;bool referenced=false;
 const char *type=!strcmp(collection_name,"devices")?"device":!strcmp(collection_name,"rooms")?"room":!strcmp(collection_name,"scenes")?"scene":"group";
 cJSON_ArrayForEach(v,item(draft,"dashboards"))cJSON_ArrayForEach(t,item(v,"tiles"))if(!strcmp(string(t,"type"),type)&&integer(t,"ref")==id)referenced=true;
 if(!strcmp(collection_name,"devices"))cJSON_ArrayForEach(v,item(draft,"scenes"))cJSON_ArrayForEach(t,item(v,"actions"))if(integer(t,"device")==id)referenced=true;
 if(!strcmp(collection_name,"devices"))cJSON_ArrayForEach(v,item(draft,"groups"))cJSON_ArrayForEach(t,item(v,"members"))if(cJSON_IsNumber(t)&&t->valueint==id)referenced=true;
 if(!strcmp(collection_name,"rooms")){
  cJSON_ArrayForEach(v,item(draft,"devices"))if(integer(v,"room")==id)referenced=true;
  cJSON_ArrayForEach(v,item(draft,"groups"))if(integer(v,"room")==id)referenced=true;
  cJSON_ArrayForEach(v,item(draft,"scenes"))if(integer(v,"room")==id)referenced=true;
 }
 if(strcmp(collection_name,"dashboards")&&referenced){config_status("Najpierw usuń powiązane karty lub działania.",true);return false;}
 if(!strcmp(collection_name,"dashboards")&&cJSON_GetArraySize(a)<=1){config_status("Zachowaj przynajmniej jeden pulpit.",true);return false;}
 return true;
}
static void deletion_back(lv_event_t *event)
{
 (void)event;if(own_save){config_status("Poczekaj na zakończenie zapisu.",true);return;}
 if(save_confirmed){dsr_ui_workspace_hide();dsr_network_home(NULL);dsr_mobile_show(DSR_MOBILE_DEVICES,0);}
 else room_back(NULL);
}
static void show_device_deletion(void)
{
 lv_obj_t *root=screen("Usuwanie urządzenia");
 deletion_state=dsr_sc_text(body,"Zapisywanie…",8,16,276,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(body,"Usuwane są tylko ta karta, jej powiązanie i odwołania. Pozostałe kanały, pokoje i sceny zostają zachowane.",8,88,276,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_ui_button(root,"Wstecz",12,420,140,deletion_back,NULL);dsr_ui_button(root,"Zapisz",168,420,140,save,NULL);
 view_save_target=SAVE_STAY;
}
static void delete_entity_confirm(lv_event_t *event)
{
 (void)event;room_dialog_close(NULL);if(!entity_deletable())return;
 if(!strcmp(collection_name,"devices")){
  uint32_t id=(uint32_t)integer(editing,"id");char error[96];cJSON *next=dsr_config_remove_device(draft,id,error);
  if(!next){config_status("Nie udało się przygotować usunięcia. Dane zachowane.",true);return;}
  cJSON_Delete(draft);draft=next;editing=NULL;deletion_id=id;
  show_device_deletion();save(NULL);return;
 }
 cJSON *a=cJSON_GetObjectItemCaseSensitive(draft,collection_name);int id=integer(editing,"id"),at=0;
 for(cJSON *entry=a->child;entry;entry=entry->next,at++)if(entry==editing){cJSON_DeleteItemFromArray(a,at);break;}
 if(!strcmp(collection_name,"dashboards")&&integer(draft,"active_dashboard")==id)cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"active_dashboard"),integer(a->child,"id"));
 editing=NULL;if(action_editor.active){action_editor.deleted=true;action_form();}else{show_collection(NULL);view_save_target=SAVE_COLLECTION;}
}
static void delete_entity(lv_event_t *event)
{
 (void)event;if(!entity_deletable())return;
 char question[160];snprintf(question,sizeof(question),dsr_sc_tr("Usunąć \"%s\"?"),string(editing,"name"));
 bool device=!strcmp(collection_name,"devices");
 lv_obj_t *panel=room_modal(question,device?"Usunąć ten kanał z pilota oraz jego karty i odwołania? Pozostałe kanały, pomieszczenia i sceny pozostaną. Shelly nie zostanie zresetowane.":"Po potwierdzeniu wybierz Zapisz. Dane zostaną zmienione dopiero po udanym zapisie.");
 /* A maximum-width name can wrap beyond two lines. Bound this named heading
  * and move only this modal's description, leaving contextual dialogs intact. */
 lv_obj_t *heading=lv_obj_get_child(panel,0);lv_obj_set_height(heading,44);lv_label_set_long_mode(heading,LV_LABEL_LONG_MODE_DOTS);
 lv_obj_set_y(lv_obj_get_child(panel,1),72);
 dsr_ui_button(panel,"Anuluj",12,188,130,room_dialog_close,NULL);
 dsr_ui_button(panel,"Usuń",154,188,130,delete_entity_confirm,NULL);
}
#include "dsr_ui_action_editor.inc"
static void edit_event(lv_event_t *event){show_entity(lv_event_get_user_data(event));}
static void inactive_control(lv_obj_t *button)
{lv_obj_add_state(button,LV_STATE_DISABLED);lv_obj_set_style_opa(button,LV_OPA_60,LV_STATE_DISABLED);lv_obj_set_style_text_color(lv_obj_get_child(button,0),dsr_sc_text_color(DSR_SC_MUTED),0);}
static lv_obj_t *order_button(lv_obj_t *parent,int x,int y,int width,bool down,bool enabled,lv_event_cb_t callback,uintptr_t value)
{
 lv_obj_t *button=dsr_ui_button(parent,down?"Dół":"Góra",x,y,width,callback,(void *)value),*caption=lv_obj_get_child(button,0);
 dsr_ui_icon_t arrow=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?(down?DSR_ICON_COVER_CLOSE:DSR_ICON_COVER_OPEN):(down?DSR_ICON_DOWN:DSR_ICON_UP);
 lv_obj_remove_flag(button,LV_OBJ_FLAG_PRESS_LOCK);
 if(width<80){
  lv_obj_set_style_text_font(caption,dsr_sc_font(&dsr_roboto_11),0);lv_obj_set_width(caption,width-4);lv_obj_align(caption,LV_ALIGN_BOTTOM_MID,0,-3);
  dsr_sc_icon(button,arrow,(width-20)/2,4,20,enabled?DSR_SC_GLOW:DSR_SC_OFF);
 }else{
  lv_obj_set_width(caption,width-44);lv_obj_align(caption,LV_ALIGN_RIGHT_MID,-4,0);dsr_sc_icon(button,arrow,8,12,24,enabled?DSR_SC_GLOW:DSR_SC_OFF);
 }
 if(!enabled)inactive_control(button);
 return button;
}
static void device_order_leave(lv_event_t *event)
{(void)event;dsr_ui_workspace_hide();dsr_network_home(NULL);dsr_mobile_show(DSR_MOBILE_DEVICES,0);}
static void device_order_back(lv_event_t *event)
{
 (void)event;if(own_save){config_status("Poczekaj na zakończenie zapisu.",true);return;}
 if(!room_dirty()){device_order_leave(NULL);return;}
 lv_obj_t *panel=room_modal("Odrzucić zmiany?","Niezapisane zmiany tego formularza zostaną utracone.");
 dsr_ui_button(panel,"Zostań",12,188,130,room_dialog_close,NULL);dsr_ui_button(panel,"Odrzuć",154,188,130,device_order_leave,NULL);
}
static void show_device_order(void);
static void reorder_device(lv_event_t *event)
{
 if(own_save){config_status("Poczekaj na zakończenie zapisu.",true);return;}
 uintptr_t value=(uintptr_t)lv_event_get_user_data(event);int at=(int)(value/2),to=at+(value%2?1:-1);cJSON *devices=cJSON_GetObjectItemCaseSensitive(draft,"devices");
 if(at>=cJSON_GetArraySize(devices)||to<0||to>=cJSON_GetArraySize(devices))return;
 cJSON *device=cJSON_DetachItemFromArray(devices,at);device_order_focus=(uint32_t)integer(device,"id");cJSON_InsertItemInArray(devices,to,device);show_device_order();
}
static void show_device_order(void)
{
 lv_obj_t *root=screen("Kolejność urządzeń");view_save_target=SAVE_DEVICES;lv_obj_set_y(body,118);lv_obj_set_height(body,280);dsr_sc_header_back(root,device_order_back,NULL);
 const cJSON *devices=item(draft,"devices"),*device;int y=0,index=0,count=cJSON_GetArraySize(devices);lv_obj_t *focus_row=NULL;
 cJSON_ArrayForEach(device,devices){
  const char *name=display_name(device);lv_point_t size;lv_text_get_size(&size,name,dsr_sc_font(&dsr_roboto_14),0,0,148,LV_TEXT_FLAG_NONE);int height=size.y+20;if(height<68)height=68;
  lv_obj_t *row=dsr_sc_panel(body,0,y,284,height,DSR_SC_CARD,8),*label=dsr_sc_text(row,dsr_sc_user(name),10,0,148,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_align(label,LV_ALIGN_LEFT_MID,10,0);
  order_button(row,168,(height-48)/2,48,false,index>0,reorder_device,(uintptr_t)(index*2));order_button(row,224,(height-48)/2,48,true,index+1<count,reorder_device,(uintptr_t)(index*2+1));
  if((uint32_t)integer(device,"id")==device_order_focus)focus_row=row;
  y+=height+8;index++;
 }
 if(!count)dsr_sc_text(body,"Brak urządzeń",12,24,260,&dsr_roboto_16,DSR_SC_MUTED);
 actions(root,device_order_back);if(focus_row){lv_obj_update_layout(body);lv_obj_scroll_to_view(focus_row,LV_ANIM_OFF);}device_order_focus=0;
}
static void tile_redraw(void)
{int scroll=lv_obj_get_scroll_y(body);show_entity(editing);lv_obj_update_layout(body);lv_obj_scroll_to_y(body,scroll,LV_ANIM_OFF);}
static void reorder(lv_event_t *event)
{
 uintptr_t value=(uintptr_t)lv_event_get_user_data(event);int at=(int)(value/2),direction=value%2?1:-1;
 cJSON *a=cJSON_GetObjectItemCaseSensitive(editing,"tiles");int to=at+direction;if(to<0||to>=cJSON_GetArraySize(a))return;cJSON *t=cJSON_DetachItemFromArray(a,at);cJSON_InsertItemInArray(a,to,t);tile_page=(unsigned)to/3;tile_focus=to;show_entity(editing);
}
static void tile_remove(lv_event_t *event){cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(editing,"tiles"),(int)(uintptr_t)lv_event_get_user_data(event));show_entity(editing);}
static void tile_add(lv_event_t *event)
{
 unsigned type=(unsigned)(uintptr_t)lv_event_get_user_data(event);const char *key=type==0?"devices":type==1?"rooms":type==2?"scenes":"groups";
 const cJSON *objects=item(draft,key);if(!objects->child){config_status("Najpierw dodaj obiekt tego typu",true);return;}
 cJSON *tiles=cJSON_GetObjectItemCaseSensitive(editing,"tiles");if(cJSON_GetArraySize(tiles)>=24)return;int id=1;for(cJSON *t=tiles->child;t;t=t->next)if(integer(t,"id")>=id)id=integer(t,"id")+1;
 cJSON *t=cJSON_CreateObject();cJSON_AddNumberToObject(t,"id",id);cJSON_AddStringToObject(t,"type",type==0?"device":type==1?"room":type==2?"scene":"group");cJSON_AddNumberToObject(t,"ref",integer(objects->child,"id"));cJSON_AddNumberToObject(t,"span",type==1?1:2);cJSON_AddNumberToObject(t,"height",1);cJSON_AddNumberToObject(t,"fields",255);cJSON_AddItemToArray(tiles,t);show_entity(editing);
}
static void tile_next(lv_event_t *event)
{
 cJSON *t=lv_event_get_user_data(event);const char *type=string(t,"type");const cJSON *objects=item(draft,!strcmp(type,"device")?"devices":!strcmp(type,"room")?"rooms":!strcmp(type,"scene")?"scenes":"groups"),*v;
 unsigned count=(unsigned)cJSON_GetArraySize(objects);if(count<2)return;unsigned i=object_index(objects,integer(t,"ref"));v=cJSON_GetArrayItem(objects,(int)((i+1)%count));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"ref"),integer(v,"id"));tile_redraw();
}
static void tile_size(lv_event_t *event)
{cJSON *t=lv_event_get_user_data(event);cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"height"),integer(t,"height")==1?2:1);tile_redraw();}
static void tile_span(lv_event_t *event)
{cJSON *t=lv_event_get_user_data(event);if(strcmp(string(t,"type"),"room"))return;cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"span"),integer(t,"span")==1?2:1);tile_redraw();}
static void tile_type(lv_event_t *event)
{
 cJSON *t=lv_event_get_user_data(event);const char *types[]={"device","room","scene","group"},*keys[]={"devices","rooms","scenes","groups"};unsigned current=!strcmp(string(t,"type"),"device")?0:!strcmp(string(t,"type"),"room")?1:!strcmp(string(t,"type"),"scene")?2:3;
 for(unsigned n=1;n<=4;n++){unsigned to=(current+n)%4;const cJSON *a=item(draft,keys[to]);if(a->child){cJSON_ReplaceItemInObjectCaseSensitive(t,"type",cJSON_CreateString(types[to]));cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"ref"),integer(a->child,"id"));if(to!=1)cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(t,"span"),2);break;}}tile_redraw();
}
static void field_changed(lv_event_t *event)
{unsigned bit=(unsigned)(uintptr_t)lv_event_get_user_data(event);int f=integer(fields_tile,"fields");if(lv_obj_has_state(lv_event_get_target(event),LV_STATE_CHECKED))f|=(int)bit;else f&=~(int)bit;cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(fields_tile,"fields"),f);}
static void fields_back(lv_event_t *event){(void)event;show_entity(editing);}
static void tile_fields(lv_event_t *event)
{
 fields_tile=lv_event_get_user_data(event);lv_obj_t *root=screen("Dane wyświetlane na karcie");const char *labels[]={"Moc (W)","Napięcie (V)","Bateria","Temperatura","Wilgotność","Pozycja","Stan otwarcia","Dodatkowe parametry"};
 view_save_target=SAVE_ENTITY;
 for(unsigned i=0;i<8;i++){lv_obj_t *c=lv_checkbox_create(body);lv_obj_set_pos(c,6,(int)i*44);lv_checkbox_set_text(c,dsr_sc_tr(labels[i]));lv_obj_set_style_text_color(c,dsr_sc_color(DSR_SC_TEXT),0);if(integer(fields_tile,"fields")&(1<<i))lv_obj_add_state(c,LV_STATE_CHECKED);lv_obj_add_event_cb(c,field_changed,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)(1u<<i));}actions(root,fields_back);
}
static void tiles_page(lv_event_t *event){tile_page=(unsigned)(uintptr_t)lv_event_get_user_data(event);show_entity(editing);}
static void show_entity(cJSON *entity)
{
 if(room_editor){editing=entity;show_room_form();return;}
 if(device_editor){editing=entity;show_device_form();return;}
 editing=entity;bool dashboard=!strcmp(collection_name,"dashboards");lv_obj_t *root=screen(dashboard?"Edytuj dashboard":"Edytuj konfigurację");if(dashboard){lv_obj_set_y(body,118);lv_obj_set_height(body,280);}add_field("Nazwa","name",0,false);
 view_save_target=SAVE_COLLECTION;
 bool room=!strcmp(collection_name,"rooms");
 if(room||dashboard)dsr_ui_button(body,room?"Usuń pomieszczenie":"Usuń pulpit",0,76,284,delete_entity,NULL);
 if(room){add_choice("Ikona",&icon_choice,140);dsr_ui_button(body,string(editing,"photo")[0]?"Zmień zdjęcie z pamięci":"Wybierz zdjęcie z pamięci",0,228,284,choose_photo,NULL);dsr_sc_text(body,"Własne zdjęcia dodaj w lokalnym Web Panelu. Zostaną zapisane w pilocie.",0,288,284,&dsr_roboto_12,DSR_SC_MUTED);}
 if(!strcmp(collection_name,"devices")){add_choice("Typ",&kind_choice,80);add_field("Adres IPv4 Shelly","host",160,false);add_field("Kanał (0–15)","channel",240,true);object_choice("Pomieszczenie","room",320);}
 if(!strcmp(collection_name,"scenes")){
  const cJSON *actions_list=item(editing,"actions"),*a;int i=0;
  cJSON_ArrayForEach(a,actions_list){int y=80+i*112;const cJSON *d=cJSON_GetArrayItem(item(draft,"devices"),(int)object_index(item(draft,"devices"),integer(a,"device")));dsr_ui_button(body,display_name(d),0,y,284,scene_device_cycle,(void *)a);dsr_ui_button(body,string(a,"command"),0,y+52,180,scene_action_cycle,(void *)a);dsr_ui_button(body,"Usuń",190,y+52,94,scene_action_remove,(void *)(uintptr_t)i);i++;}dsr_ui_button(body,"+ Działanie",0,88+i*112,284,scene_action_add,NULL);
 }
 if(!strcmp(collection_name,"groups")){
  dsr_sc_text(body,"Do 16 kanałów Switch. Grupa ma jawne Włącz/Wyłącz.",0,78,284,&dsr_roboto_12,DSR_SC_MUTED);unsigned i=0;const cJSON *d,*member;
  cJSON_ArrayForEach(d,item(draft,"devices")){const char *kind=string(d,"kind");bool included=false;cJSON_ArrayForEach(member,item(editing,"members"))if(member->valueint==integer(d,"id"))included=true;if(strcmp(kind,"relay")&&strcmp(kind,"plug")&&!included)continue;lv_obj_t *check=lv_checkbox_create(body);lv_obj_set_pos(check,6,128+(int)i++*44);lv_checkbox_set_text(check,display_name(d));if(included)lv_obj_add_state(check,LV_STATE_CHECKED);lv_obj_add_event_cb(check,group_member,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)integer(d,"id"));}
 }
 if(!strcmp(collection_name,"dashboards")){
  bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
  /* Every skin reserves two title lines before the tile controls. */
  int stride=260,offset=48;lv_obj_t *focus_order=NULL;
  cJSON *tiles=cJSON_GetObjectItemCaseSensitive(entity,"tiles");int i=0,visible=0;
  unsigned pages=((unsigned)cJSON_GetArraySize(tiles)+2)/3;if(tile_page>=pages)tile_page=pages?pages-1:0;
  for(cJSON *t=tiles->child;t;t=t->next,i++){
   if((unsigned)i/3!=tile_page)continue;
   const char *type=string(t,"type");const cJSON *a=item(draft,!strcmp(type,"device")?"devices":!strcmp(type,"room")?"rooms":!strcmp(type,"scene")?"scenes":"groups"),*target=cJSON_GetArrayItem(a,(int)object_index(a,integer(t,"ref")));
   int y=140+visible++*stride;char text[100];snprintf(text,sizeof(text),"%s • %s",string(target,"name"),dsr_sc_tr(integer(t,"height")==2?"duża":"standard"));
   lv_obj_t *title=dsr_sc_text(body,dsr_sc_user(text),0,y,280,hud?&dsr_roboto_16:&dsr_roboto_14,DSR_SC_TEXT);
   lv_obj_set_height(title,40);lv_label_set_long_mode(title,LV_LABEL_LONG_MODE_DOTS);
   lv_obj_t *up=order_button(body,0,y+offset,136,false,i>0,reorder,(uintptr_t)(i*2));order_button(body,148,y+offset,136,true,i+1<cJSON_GetArraySize(tiles),reorder,(uintptr_t)(i*2+1));if(i==tile_focus)focus_order=up;
   bool multiple=cJSON_GetArraySize(a)>1,is_device=!strcmp(type,"device"),is_room=!strcmp(type,"room");
   lv_obj_t *object=dsr_ui_button(body,multiple?"Następny obiekt":"Tylko jeden obiekt",0,y+offset+52,136,tile_next,t);if(!multiple)inactive_control(object);
   const char *kinds[]={"devices","rooms","scenes","groups"};unsigned available=0;for(unsigned k=0;k<4;k++)if(cJSON_GetArraySize(item(draft,kinds[k])))available++;
   const char *kind=is_device?"Urządzenie":is_room?"Pomieszczenie":!strcmp(type,"scene")?"Scena":"Grupa";char caption[80];snprintf(caption,sizeof(caption),dsr_sc_tr("Typ: %s"),dsr_sc_tr(kind));lv_obj_t *type_button=dsr_ui_button(body,caption,148,y+offset+52,136,tile_type,t);if(available<2)inactive_control(type_button);
   lv_obj_t *width=dsr_ui_button(body,integer(t,"span")==1?"Połowa szerokości":"Pełna szerokość",0,y+offset+104,136,tile_span,t);if(!is_room)inactive_control(width);
   snprintf(caption,sizeof(caption),dsr_sc_tr("Rozmiar: %s"),dsr_sc_tr(integer(t,"height")==2?"duża":"standard"));dsr_ui_button(body,caption,148,y+offset+104,136,tile_size,t);
   lv_obj_t *data=dsr_ui_button(body,"Dane na karcie",0,y+offset+156,136,tile_fields,t);if(!is_device)inactive_control(data);dsr_ui_button(body,"Usuń kartę",148,y+offset+156,136,tile_remove,(void *)(uintptr_t)i);
  }
  int y=148+visible*stride;if(tile_page)dsr_ui_button(body,"Poprzednie karty",0,y,284,tiles_page,(void *)(uintptr_t)(tile_page-1));if(tile_page+1<pages)dsr_ui_button(body,"Następne karty",0,y+56,284,tiles_page,(void *)(uintptr_t)(tile_page+1));
  dsr_ui_button(body,"+ Urządzenie",0,y+112,284,tile_add,(void *)0);dsr_ui_button(body,"+ Pomieszczenie",0,y+168,284,tile_add,(void *)1);dsr_ui_button(body,"+ Scena",0,y+224,284,tile_add,(void *)2);dsr_ui_button(body,"+ Grupa",0,y+280,284,tile_add,(void *)3);
  if(focus_order){lv_obj_update_layout(body);lv_obj_scroll_to_view(focus_order,LV_ANIM_OFF);}tile_focus=-1;
 }
 if(!room&&!dashboard){int bottom=!strcmp(collection_name,"scenes")?160+cJSON_GetArraySize(item(entity,"actions"))*112:!strcmp(collection_name,"groups")?160+cJSON_GetArraySize(item(draft,"devices"))*44:420;dsr_ui_button(body,!strcmp(collection_name,"devices")?"Usuń urządzenie":"Usuń wpis",0,bottom,284,delete_entity,NULL);}actions(root,show_collection);
}
static void add_entity(lv_event_t *event)
{
 (void)event;cJSON *a=cJSON_GetObjectItemCaseSensitive(draft,collection_name);int id=1;for(cJSON *v=a->child;v;v=v->next)if(integer(v,"id")>=id)id=integer(v,"id")+1;
 int limit=!strcmp(collection_name,"dashboards")?4:!strcmp(collection_name,"devices")?16:8;if(cJSON_GetArraySize(a)>=limit){config_status("Osiągnięto limit wpisów",true);return;}
 cJSON *v=cJSON_CreateObject();cJSON_AddNumberToObject(v,"id",id);cJSON_AddStringToObject(v,"name","Nowy wpis");
 if(!strcmp(collection_name,"dashboards"))cJSON_AddItemToObject(v,"tiles",cJSON_CreateArray());
 if(!strcmp(collection_name,"rooms")){cJSON_AddNumberToObject(v,"icon",0);cJSON_AddStringToObject(v,"photo","");}
 if(!strcmp(collection_name,"devices")){cJSON_AddStringToObject(v,"kind","light");cJSON_AddStringToObject(v,"host","");cJSON_AddNumberToObject(v,"channel",0);cJSON_AddNumberToObject(v,"room",0);}
 if(!strcmp(collection_name,"scenes"))cJSON_AddItemToObject(v,"actions",cJSON_CreateArray());
 if(!strcmp(collection_name,"groups"))cJSON_AddItemToObject(v,"members",cJSON_CreateArray());
 cJSON_AddItemToArray(a,v);show_entity(v);
}
static void show_collection(lv_event_t *event)
{
 if(event&&lv_event_get_user_data(event))snprintf(collection_name,sizeof(collection_name),"%s",(const char *)lv_event_get_user_data(event));
 editing=NULL;lv_obj_t *root=screen("Edycja • wspólna konfiguracja");const cJSON *a=item(draft,collection_name);int y=0;const cJSON *v;
 view_save_target=SAVE_MENU;
 cJSON_ArrayForEach(v,a){dsr_ui_button(body,dsr_sc_name(display_name(v)),0,y,284,edit_event,(void *)v);y+=58;}dsr_ui_button(body,"+ Dodaj wpis",0,y,284,add_entity,NULL);actions(root,menu_event);
}
static const choice_t language_choice={"language","pl\nen\nde",false},theme_choice={"theme","graphite\nlight\ncyber_blue",false}; /* row i <-> stored id string, never a numeric id */
static const uint32_t colors[]={DSR_SC_BLUE,0x34cbb5,0x8c9dff,0xe4a858};
static void color_selected(lv_event_t *event){unsigned at=lv_dropdown_get_selected(lv_event_get_target(event));if(at<4)cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"accent"),colors[at]);}
static void appearance(lv_event_t *event)
{(void)event;editing=draft;lv_obj_t *root=screen("Wygląd i język");add_choice("Język: Polski / English / Deutsch",&language_choice,0);add_choice("Motyw",&theme_choice,80);dsr_sc_text(body,"Kolor akcentu",2,160,284,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *color=lv_dropdown_create(body);lv_obj_set_pos(color,0,181);lv_obj_set_size(color,284,44);lv_dropdown_set_options(color,dsr_sc_tr("Niebieski\nTurkusowy\nLawendowy\nBursztynowy\nWłasny z Web Panelu"));unsigned chosen=4;for(unsigned i=0;i<4;i++)if((uint32_t)integer(draft,"accent")==colors[i])chosen=i;lv_dropdown_set_selected(color,chosen);lv_obj_add_event_cb(color,color_selected,LV_EVENT_VALUE_CHANGED,NULL);object_choice("Pulpit startowy","active_dashboard",240);actions(root,return_settings);}
static void show_menu(void)
{
 lv_obj_t *root=screen(settings_context?"Konfiguracja wpisów":"Edytuj pilot");
 view_save_target=settings_context?SAVE_SETTINGS:SAVE_HOME;
 const char *labels[]={"Pulpity","Pomieszczenia","Urządzenia","Sceny","Grupy"},*keys[]={"dashboards","rooms","devices","scenes","groups"};
 unsigned first=settings_context?2:0;
 for(unsigned i=first;i<(settings_context?5u:2u);i++)dsr_ui_button(body,labels[i],0,(int)(i-first)*56,284,show_collection,(void *)keys[i]);
 actions(root,settings_context?return_settings:return_home);
}
void dsr_ui_config_appearance(lv_event_t *event)
{
 (void)event;if(!dsr_ui_settings_can_open())return;
 settings_context=true;appearance_editor=true;error_return_rooms=false;error_return_settings=true;
 open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 editor_active=true;dashboard_picker=room_editor=room_new=room_deleted=device_editor=false;context_device_id=0;room_dialog=NULL;
 appearance(NULL);
}
void dsr_ui_config_registry(lv_event_t *event)
{
 (void)event;if(!dsr_ui_settings_can_open())return;
 settings_context=true;appearance_editor=false;error_return_rooms=false;error_return_settings=true;
 open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 editor_active=true;dashboard_picker=room_editor=room_new=room_deleted=device_editor=false;context_device_id=0;room_dialog=NULL;
 show_menu();
}
void dsr_ui_edit_dashboard(void)
{
 settings_context=appearance_editor=error_return_settings=false;error_return_rooms=false;config_log("edit_dashboard",0);open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 room_editor=room_new=room_deleted=device_editor=false;context_device_id=0;room_dialog=NULL;editor_active=true;dashboard_picker=false;tile_page=0;config_log("show_menu",0);show_menu();
}
void dsr_ui_edit_layout(void)
{
 if(!dsr_mobile_active()||dsr_mobile_page()!=DSR_MOBILE_DEVICES){dsr_ui_edit_dashboard();return;}
 settings_context=appearance_editor=error_return_settings=error_return_rooms=false;open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 room_editor=room_new=room_deleted=device_editor=dashboard_picker=false;context_device_id=0;room_dialog=NULL;editor_active=true;snprintf(collection_name,sizeof(collection_name),"devices");show_device_order();
}
static bool room_begin(void)
{
 settings_context=appearance_editor=error_return_settings=false;error_return_rooms=true;open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return false;}
 editor_active=room_editor=true;dashboard_picker=room_new=room_deleted=device_editor=false;context_device_id=0;room_dialog=NULL;
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
void dsr_ui_edit_local_device(uint32_t id)
{
 if(!room_begin())return;
 room_editor=false;device_editor=true;context_device_id=id;error_return_rooms=false;
 snprintf(collection_name,sizeof(collection_name),"devices");
 cJSON *d;cJSON_ArrayForEach(d,cJSON_GetObjectItemCaseSensitive(draft,"devices"))if((uint32_t)integer(d,"id")==id){editing=d;show_device_form();return;}
 room_leave(NULL);
}
static void pick_dashboard(lv_event_t *event)
{cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"active_dashboard"),(unsigned)(uintptr_t)lv_event_get_user_data(event));save(NULL);}
void dsr_ui_select_dashboard(void)
{
 settings_context=appearance_editor=error_return_settings=false;error_return_rooms=false;open_result_t result=prepare_draft();if(result!=OPEN_OK){config_error(result);return;}
 draft_revision=(uint32_t)integer(draft,"revision");editor_active=true;own_save=false;dashboard_picker=true;room_editor=room_new=room_deleted=device_editor=false;context_device_id=0;room_dialog=NULL;
 lv_obj_t *root=screen("Wybierz pulpit");int y=0;const cJSON *d;
 view_save_target=SAVE_HOME;
 cJSON_ArrayForEach(d,item(draft,"dashboards")){lv_obj_t *b=dsr_ui_button(body,dsr_sc_name(string(d,"name")),0,y,284,pick_dashboard,(void *)(uintptr_t)integer(d,"id"));if(integer(d,"id")==integer(draft,"active_dashboard"))dsr_ui_primary(b);y+=56;}
 dsr_ui_button(root,"Wstecz",12,420,296,return_home,NULL);
}
static void migration_tick(void);
void dsr_ui_workspace_tick(void)
{
 migration_tick();
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
   config_status("Zmieniono konfigurację. Wróć i otwórz edycję ponownie.",true);if(deletion_id&&deletion_state)dsr_sc_label_set_text(deletion_state,"Nie potwierdzono zapisu usunięcia. Spróbuj ponownie.");return;
  }
  draft_revision=revision;own_save=false;save_confirmed=true;save_failed=false;
  cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(draft,"revision"),revision);
  cJSON_Delete(saved_baseline);saved_baseline=submitted_draft;submitted_draft=NULL;
  /* A fresh exact durable ACK owns this navigation opportunity. Never use
   * sticky save_confirmed: edits B or an intervening screen must stay put. */
  save_target_t target=submitted_save_target;submitted_save_target=SAVE_STAY;
  bool clean=cJSON_Compare(draft,saved_baseline,true);
  if(clean&&submitted_generation==view_generation&&target!=SAVE_STAY){
   if(target==SAVE_ACTIONS){action_leave(NULL);return;}
   if(target==SAVE_DEVICES){device_order_leave(NULL);return;}
   if(target==SAVE_HOME){return_home(NULL);return;}
   if(target==SAVE_ROOMS){room_leave(NULL);return;}
   if(target==SAVE_DEVICE){room_leave(NULL);return;}
   if(target==SAVE_SETTINGS){return_settings(NULL);return;}
   if(target==SAVE_MENU){show_menu();return;}
   if(target==SAVE_COLLECTION){show_collection(NULL);return;}
   if(target==SAVE_ENTITY){show_entity(editing);return;}
  }
  if(appearance_editor){
   /* Display the acknowledged style A while retaining later draft edits B. */
   const char *locale=string(saved_baseline,"language");
   dsr_sc_appearance((uint32_t)integer(saved_baseline,"accent"),dsr_sc_style_from_name(string(saved_baseline,"theme")),!strcmp(locale,"en")?1:!strcmp(locale,"de")?2:0);
   dsr_ui_apply_theme();appearance(NULL);
  }
  if(room_editor&&cJSON_Compare(draft,saved_baseline,true)&&(room_new||room_deleted)){room_new=false;show_room_form();}
 }else if(own_save&&!busy){own_save=false;save_failed=true;cJSON_Delete(submitted_draft);submitted_draft=NULL;}
 if(deletion_id&&deletion_state){
  dsr_result_t cleanup=dsr_network_removal_status(deletion_id);
  const char *text=own_save?"Zapisywanie…":save_failed?"Nie potwierdzono zapisu usunięcia. Spróbuj ponownie.":save_confirmed?(cleanup==DSR_BUSY?"Karta usunięta. Kończenie usuwania powiązania…":cleanup!=DSR_OK?"Karta usunięta. Błąd zapisu powiązania. Pilot ponowi próbę.":"Urządzenie usunięte."):"Usunięcie nie zostało zapisane.";
  dsr_sc_label_set_text(deletion_state,text);
 }
 bool dirty=!saved_baseline||!cJSON_Compare(draft,saved_baseline,true);
 if(device_editor&&device_input_error){config_status(device_input_error,true);return;}
 if(own_save)config_status("Zapisywanie…",false);
 else if(save_failed)config_status("Zapis nie powiódł się. Zmiany pozostają w edytorze.",true);
 else if(dirty)config_status(save_confirmed?"Zapisano wcześniejszą wersję • są niezapisane zmiany":"Niezapisane zmiany",true);
 else if(save_confirmed)config_status("Zapisano • zsynchronizowano",false);
}
typedef struct {dsr_device_t device;uint32_t layout_id,room,version,candidate_id;char previous_host[16];} migration_t;
static migration_t migrations[DSR_MAX_DEVICES];
_Static_assert(sizeof(migrations)<=4096,"Migration fixed storage budget");
static size_t migration_count;
static uint32_t migration_serial,migration_submitted,migration_revision,migration_retry;
static cJSON *migration_document;
static dsr_result_t migration_result=DSR_OK;
static bool migration_failed;
static bool migration_match(const cJSON *v,const migration_t *m)
{
 if(!dsr_endpoint_layout_matches(m->device.endpoint_kind,string(v,"kind")))return false;
 if(m->layout_id)return (uint32_t)integer(v,"id")==m->layout_id;
 if((m->device.endpoint_kind==DSR_ENDPOINT_RGB||DSR_ENDPOINT_IS_SENSOR(m->device.endpoint_kind)))return false; /* New typed RGB/H&T never convert host-only display cards. */
 if(integer(v,"channel")!=m->device.channel)return false;
 /* An unpublished endpoint may adopt legacy layout only if another durable
  * endpoint does not already own that numeric ID. */
 if(dsr_network_layout_reserved((uint32_t)integer(v,"id")))return false;
 return !strcmp(string(v,"host"),m->device.host)||(m->previous_host[0]&&!strcmp(string(v,"host"),m->previous_host));
}
static bool replace_text(cJSON *o,const char *key,const char *value)
{
 if(!strcmp(string(o,key),value))return true;
 cJSON *v=cJSON_CreateString(value);if(!v)return false;
 bool ok=cJSON_HasObjectItem(o,key)?cJSON_ReplaceItemInObjectCaseSensitive(o,key,v):cJSON_AddItemToObject(o,key,v);
 if(!ok)cJSON_Delete(v);
 return ok;
}
static bool merge_migration(cJSON *r,migration_t *m)
{
 cJSON *a=cJSON_GetObjectItemCaseSensitive(r,"devices"),*found=NULL;int id=1;
 for(cJSON *v=a->child;v;v=v->next){if(migration_match(v,m)&&!found)found=v;if(integer(v,"id")>=id)id=integer(v,"id")+1;}
 if(!found){
  /* Published IDs are owned by the current workspace. A concurrent deletion
   * must win over a delayed bind/DHCP migration, including after CAS retry. */
  if(m->layout_id)return false;
  if(cJSON_GetArraySize(a)>=DSR_MAX_DEVICES)return false;
  found=cJSON_CreateObject();if(!found)return false;
  if(!cJSON_AddNumberToObject(found,"id",id)||!cJSON_AddStringToObject(found,"name",m->device.name)||!cJSON_AddStringToObject(found,"host",m->device.host)||!cJSON_AddNumberToObject(found,"channel",m->device.channel)||!cJSON_AddNumberToObject(found,"room",0)||!cJSON_AddStringToObject(found,"kind",dsr_endpoint_kind_name(m->device.endpoint_kind))){cJSON_Delete(found);return false;}
  cJSON_AddItemToArray(a,found);
  cJSON *dash=cJSON_GetArrayItem(item(r,"dashboards"),(int)object_index(item(r,"dashboards"),integer(r,"active_dashboard"))),*tiles=cJSON_GetObjectItemCaseSensitive(dash,"tiles");
  if(cJSON_GetArraySize(tiles)<24){
   int tile_id=1;for(cJSON *v=tiles->child;v;v=v->next)if(integer(v,"id")>=tile_id)tile_id=integer(v,"id")+1;
   cJSON *t=cJSON_CreateObject();if(!t)return false;
   if(!cJSON_AddNumberToObject(t,"id",tile_id)||!cJSON_AddStringToObject(t,"type","device")||!cJSON_AddNumberToObject(t,"ref",id)||!cJSON_AddNumberToObject(t,"span",2)||!cJSON_AddNumberToObject(t,"height",1)||!cJSON_AddNumberToObject(t,"fields",255)){cJSON_Delete(t);return false;}
   cJSON_AddItemToArray(tiles,t);
  }
 }
 m->candidate_id=(uint32_t)integer(found,"id");
 if(!replace_text(found,"host",m->device.host))return false;
 if(m->room!=UINT32_MAX){uint32_t room=m->room;if(room&&object_index(item(r,"rooms"),(int)room)==255)room=0;cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(found,"room"),room);if(!replace_text(found,"name",m->device.name))return false;}
 return true;
}
static void migration_ack(void)
{
 size_t count=0;for(size_t i=0;i<migration_count;i++){
  if(migrations[i].version>migration_submitted)migrations[count++]=migrations[i];
  else if((migrations[i].device.endpoint_kind==DSR_ENDPOINT_RGB||DSR_ENDPOINT_IS_SENSOR(migrations[i].device.endpoint_kind))&&migrations[i].candidate_id)dsr_network_migrated_layout(&migrations[i].device,migrations[i].candidate_id);
 }
 memset(migrations+count,0,(migration_count-count)*sizeof(*migrations));migration_count=count;
 cJSON_Delete(migration_document);migration_document=NULL;migration_failed=false;migration_result=migration_count?DSR_BUSY:DSR_OK;
}
static void migration_tick(void)
{
 if(!migration_count)return;
 bool busy;char message[96];dsr_workspace_status(&busy,message);uint32_t revision=dsr_workspace_revision();
 if(migration_document){
  if(busy)return;
  char *json=dsr_workspace_snapshot();cJSON *actual=json?cJSON_Parse(json):NULL;free(json);
  bool acknowledged=actual&&revision==migration_revision+1&&cJSON_Compare(actual,migration_document,true);cJSON_Delete(actual);
  if(acknowledged){migration_ack();return;}
  cJSON_Delete(migration_document);migration_document=NULL;
  /* Revision conflicts are retried from a fresh snapshot. Failed Flash ACK
   * is reported as IO; a BIND caller cancels its unapplied RAM intent. */
  bool rgb_unconfirmed=false;for(size_t i=0;i<migration_count;i++)if((migrations[i].device.endpoint_kind==DSR_ENDPOINT_RGB||DSR_ENDPOINT_IS_SENSOR(migrations[i].device.endpoint_kind))&&!migrations[i].layout_id&&migrations[i].candidate_id)rgb_unconfirmed=true;
  if(rgb_unconfirmed&&revision!=migration_revision){migration_failed=true;migration_result=DSR_CONFLICT;migration_retry=lv_tick_get()+5000;return;}
  if(revision==migration_revision){migration_failed=true;migration_result=DSR_IO;migration_retry=lv_tick_get()+5000;return;}
 }
 if(busy||(migration_failed&&(int32_t)(lv_tick_get()-migration_retry)<0))return;
 char *json=dsr_workspace_snapshot();cJSON *r=json?cJSON_Parse(json):NULL;free(json);
 char error[96];
 if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);migration_failed=true;migration_result=DSR_IO;migration_retry=lv_tick_get()+5000;return;}
 size_t retained=0;
 for(size_t i=0;i<migration_count;i++)if(!migrations[i].layout_id||object_index(item(r,"devices"),(int)migrations[i].layout_id)!=255)migrations[retained++]=migrations[i];
 if(retained!=migration_count){
  memset(migrations+retained,0,(migration_count-retained)*sizeof(*migrations));migration_count=retained;
  cJSON_Delete(r);migration_failed=true;migration_result=DSR_CONFLICT;migration_retry=lv_tick_get()+5000;return;
 }
 cJSON *baseline=cJSON_Duplicate(r,true);bool ok=baseline!=NULL;
 for(size_t i=0;i<migration_count&&ok;i++)ok=merge_migration(r,&migrations[i]);
 ok=ok&&dsr_config_validate(r,error);migration_submitted=migration_serial;
 if(ok&&cJSON_Compare(r,baseline,true)){cJSON_Delete(baseline);cJSON_Delete(r);migration_ack();return;}
 cJSON_Delete(baseline);char *next=ok?cJSON_PrintUnformatted(r):NULL;
 if(!next){cJSON_Delete(r);migration_failed=true;migration_result=DSR_IO;migration_retry=lv_tick_get()+5000;return;}
 migration_revision=revision;
 if(dsr_workspace_submit(next,revision)){cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(r,"revision"),revision+1);migration_document=r;migration_failed=false;migration_result=DSR_BUSY;}
 else cJSON_Delete(r);
 free(next);
}
void dsr_ui_workspace_migrate_bound(const dsr_connection_t *connection,uint32_t room,const char *previous_host)
{
 if(!connection||!connection->device.device_id[0])return;
 size_t i=0;for(;i<migration_count;i++)if(migrations[i].device.endpoint_kind==connection->device.endpoint_kind&&migrations[i].device.channel==connection->device.channel&&migrations[i].device.generation==connection->device.generation&&!strcmp(migrations[i].device.device_id,connection->device.device_id))break;
 if(i==DSR_MAX_DEVICES){migration_failed=true;migration_result=DSR_FULL;return;}
 if(i==migration_count)migration_count++;
 migrations[i].device=connection->device;migrations[i].layout_id=connection->layout_id;migrations[i].room=room;migrations[i].candidate_id=0;migrations[i].version=++migration_serial;
 snprintf(migrations[i].previous_host,sizeof(migrations[i].previous_host),"%s",previous_host?previous_host:"");migration_failed=false;migration_result=DSR_BUSY;
}
bool dsr_ui_workspace_migrate_status(dsr_result_t *result)
{if(result)*result=migration_result;return migration_count&&!migration_failed;}
void dsr_ui_workspace_migrate_cancel(const dsr_device_t *device)
{
 /* Called only after a failed completed workspace attempt. Do not turn a
  * reported failure into a surprise later save of the same physical device. */
 if(!device||migration_document)return;
 size_t retained=0;
 for(size_t i=0;i<migration_count;i++)if(migrations[i].device.generation!=device->generation||strcmp(migrations[i].device.device_id,device->device_id))migrations[retained++]=migrations[i];
 memset(migrations+retained,0,(migration_count-retained)*sizeof(*migrations));migration_count=retained;
 if(!retained){migration_failed=false;migration_result=DSR_OK;}
}
void dsr_ui_workspace_migrate(const dsr_connection_t *connection){dsr_ui_workspace_migrate_bound(connection,UINT32_MAX,NULL);}
#endif
