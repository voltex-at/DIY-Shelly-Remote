#include "dsr_ui_internal.h"
#include "dsr_ui_cyber.h"
#include "dsr_device_settings.h"
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
 dsr_device_settings_snapshot_t snapshot;
 uint32_t id,observed;
 unsigned field;
 uint8_t page,return_page;
 bool active;
 bool network_step,deferred;
 unsigned deferred_action,deferred_selection;
 uint32_t requested_at;
 char network_name[33];
 char pending[257];
 lv_obj_t *body,*input,*choice,*keyboard,*notice;
 lv_obj_t *open_network;
} editor_t;
static editor_t *editor;
enum {GROUPS,FIELDS,EDIT,CONFIRM,WAITING,RESULT};
static void draw(void);
/* Only the already-supported Sys editor receives the new composition. */
static bool cyber_general(void)
{return editor&&dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&editor->page!=GROUPS&&editor->snapshot.ready&&editor->snapshot.selected<editor->snapshot.group_count&&!strcmp(editor->snapshot.groups[editor->snapshot.selected].component,"Sys");}
static void general_help(lv_event_t *event)
{
 (void)event;lv_obj_t *box=lv_msgbox_create(NULL);lv_obj_set_width(box,296);
 lv_msgbox_add_title(box,dsr_sc_tr("Ogólne"));lv_msgbox_add_close_button(box);
 lv_msgbox_add_text(box,dsr_sc_tr("Zmiana zostanie zapisana w urządzeniu Shelly."));
 lv_msgbox_add_text(box,dsr_sc_tr("Zapis jest wykonywany jeden raz. Po utracie odpowiedzi pilot sprawdzi wynik."));
}
static dsr_ui_icon_t general_icon(const dsr_setting_field_t *f)
{return !strcmp(f->path,"device.name")?DSR_ICON_TV:!strcmp(f->path,"device.eco_mode")?DSR_ICON_ECO:!strncmp(f->path,"location.t",10)||!strcmp(f->path,"sntp.server")?DSR_ICON_CLOCK:DSR_ICON_LOCATION;}
static void general_text(lv_obj_t *parent,const char *text,int x,int y,int w,int h,const lv_font_t *font,uint32_t color)
{lv_obj_t *label=dsr_sc_text(parent,text,x,y,w,font,color);lv_obj_set_height(label,h);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);}
static bool network_field(const dsr_setting_field_t *f)
{return f&&!strcmp(f->component,"WiFi")&&(!strcmp(f->path,"sta.ssid")||!strcmp(f->path,"sta1.ssid"));}
static void wipe(void *p,size_t n){volatile unsigned char *v=p;while(n--)*v++=0;}
bool dsr_ui_device_settings_active(void){return editor&&editor->active;}
void dsr_ui_device_settings_hide(void)
{if(editor){editor->active=false;dsr_device_settings_visibility(false);wipe(editor->pending,sizeof(editor->pending));free(editor);editor=NULL;}dsr_device_settings_close();}
static const char *error_text(dsr_result_t r)
{return r==DSR_AUTH_REQUIRED?"Sprawdź dane logowania Shelly":r==DSR_CONFLICT?"Ustawienia zmieniły się. Odczytaj ponownie.":r==DSR_IDENTITY_MISMATCH?"Inne urządzenie pod tym adresem":r==DSR_UNSUPPORTED?"Ustawienie niedostępne w tym firmware":r==DSR_BUSY?"Operacja w toku. Poczekaj.":r==DSR_OFFLINE?"Brak połączenia z Shelly":r==DSR_INVALID?"Sprawdź wartość i dozwolony zakres":"Nie potwierdzono ustawienia";}
static const char *group_name(const char *name)
{return !strcmp(name,"Sys")?"Ogólne":!strcmp(name,"WiFi")?"Sieć Wi-Fi":!strcmp(name,"BLE")?"Bluetooth":!strcmp(name,"Cloud")?"Shelly Cloud":!strcmp(name,"MQTT")?"MQTT":!strcmp(name,"Switch")?"Przekaźnik":!strcmp(name,"Input")?"Wejście":!strcmp(name,"Cover")?"Roleta":!strcmp(name,"Temperature")?"Temperatura":!strcmp(name,"Humidity")?"Wilgotność":"Oświetlenie";}
static void back(lv_event_t *event)
{
 (void)event;if(!editor)return;
 if(editor->page==WAITING){uint32_t id=editor->id;dsr_ui_device_settings_hide();dsr_network_device_settings(id);return;}
 if(editor->page==EDIT||editor->page==CONFIRM){editor->page=FIELDS;wipe(editor->pending,sizeof(editor->pending));draw();return;}
 if(editor->page==FIELDS||editor->page==RESULT){editor->page=GROUPS;draw();return;}
 uint32_t id=editor->id;dsr_ui_device_settings_hide();dsr_network_device_settings(id);
}
static lv_obj_t *screen(void)
{
 lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 bool hud=cyber_general();
 if(hud)dsr_sc_blu_header(root,"Ustawienia Shelly",false,dsr_wifi_connected(),back,NULL,general_help);
 else dsr_sc_header(root,"Ustawienia Shelly",false,dsr_wifi_connected(),back,NULL);
 dsr_mobile_device_t device;if(dsr_network_typed_device(editor->id,&device)){char target[96];size_t n=strlen(device.identity);snprintf(target,sizeof(target),"%s • %.12s",device.name,n>12?device.identity+n-12:device.identity);lv_obj_t *label=dsr_sc_text(root,dsr_sc_user(target),hud?12:20,hud?54:64,hud?296:280,hud?&dsr_roboto_14:&dsr_roboto_12,DSR_SC_MUTED);lv_obj_set_height(label,hud?20:17);lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);}
 if(hud)dsr_sc_panel(root,12,77,296,1,DSR_SC_BLUE,0);
 editor->body=editor->input=editor->choice=editor->keyboard=editor->notice=editor->open_network=NULL;
 editor->body=dsr_sc_panel(root,12,hud?84:90,296,hud?324:302,DSR_SC_BG,0);lv_obj_set_style_bg_opa(editor->body,LV_OPA_TRANSP,0);lv_obj_add_flag(editor->body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(editor->body,LV_DIR_VER);dsr_sc_scrollbar(editor->body);
 return root;
}
static lv_obj_t *button(lv_obj_t *parent,const char *label,int y,lv_event_cb_t callback,uintptr_t index)
{lv_obj_t *b=dsr_sc_button(parent,0,y,296,50,DSR_SC_CARD,10,callback,(void *)index);lv_obj_t *t=dsr_sc_text(b,label,12,16,272,&dsr_roboto_14,DSR_SC_TEXT);lv_label_set_long_mode(t,LV_LABEL_LONG_CLIP);return b;}
static void request(unsigned action,unsigned selection,const char *value)
{
 dsr_result_t r=dsr_network_device_settings_request(editor->id,action,selection,value);
 if(r==DSR_BUSY){editor->deferred=true;editor->deferred_action=action;editor->deferred_selection=selection;editor->requested_at=lv_tick_get();editor->page=WAITING;draw();return;}
 if(r!=DSR_OK){editor->snapshot.result=r;editor->page=RESULT;draw();return;}
 editor->observed=0;editor->page=WAITING;draw();
}
static void choose_group(lv_event_t *event)
{if(editor)request(DSR_SETTINGS_GROUP,(unsigned)(uintptr_t)lv_event_get_user_data(event),NULL);}
static void reload(lv_event_t *event)
{(void)event;if(editor)request(DSR_SETTINGS_LOAD,0,NULL);}
static void select_field(lv_event_t *event)
{if(!editor)return;editor->field=(unsigned)(uintptr_t)lv_event_get_user_data(event);editor->network_step=false;editor->page=EDIT;draw();}
static void general_fields(lv_obj_t *root)
{
 unsigned row=0;
 for(unsigned i=0;i<editor->snapshot.field_count;i++){
  unsigned index=editor->snapshot.fields[i];const dsr_setting_field_t *f=dsr_setting_field(index);char value[129];
  if(!f||!dsr_device_settings_value(index,value,sizeof(value)))continue;
  lv_obj_t *card=dsr_sc_button(editor->body,0,(int)row++*72,296,64,DSR_SC_CARD,10,select_field,(void *)(uintptr_t)index);
  lv_obj_set_style_border_color(card,lv_color_hex(0x00cfff),0);lv_obj_set_style_border_width(card,1,0);
  lv_obj_t *well=dsr_sc_panel(card,8,9,44,46,0x071628,8);
  lv_obj_set_style_border_color(well,lv_color_hex(0x315a83),0);lv_obj_set_style_border_width(well,1,0);
  dsr_sc_icon_centered(well,general_icon(f),32,DSR_SC_GLOW);
  general_text(card,f->label,62,9,200,22,&dsr_roboto_16,DSR_SC_TEXT);
  const char *shown=!value[0]?"—":!strcmp(value,"true")?"Włączone":!strcmp(value,"false")?"Wyłączone":dsr_sc_user(value);
  general_text(card,shown,62,37,200,19,&dsr_roboto_14,0xa5cdeb);
  dsr_sc_icon(card,DSR_ICON_NEXT,266,20,24,DSR_SC_TEXT);
 }
 if(!row)dsr_sc_text(editor->body,"Brak obsługiwanych pól w odpowiedzi urządzenia",8,24,280,&dsr_roboto_16,DSR_SC_MUTED);
 dsr_ui_button(root,"Wstecz",12,420,296,back,NULL);
}
static void apply(lv_event_t *event)
{(void)event;if(!editor)return;request(DSR_SETTINGS_SAVE,editor->field,editor->pending);if(!editor->deferred)wipe(editor->pending,sizeof(editor->pending));}
static void confirm(lv_event_t *event)
{
 (void)event;if(!editor)return;const dsr_setting_field_t *f=dsr_setting_field(editor->field);if(!f)return;
 if(network_field(f)){
  const char *text=lv_textarea_get_text(editor->input);
  if(!editor->network_step){if(!text[0]||strlen(text)>32)return;snprintf(editor->network_name,sizeof(editor->network_name),"%s",text);editor->network_step=true;draw();return;}
  bool open=lv_obj_has_state(editor->open_network,LV_STATE_CHECKED);
  if(!open&&strlen(text)<8){if(editor->notice)dsr_sc_label_set_text(editor->notice,"Hasło wymaga co najmniej 8 znaków");return;}
  cJSON *v=cJSON_CreateObject();if(!v)return;cJSON_AddStringToObject(v,"ssid",editor->network_name);cJSON_AddStringToObject(v,"pass",open?"":text);char *json=cJSON_PrintUnformatted(v);dsr_setting_json_delete(v);if(!json)return;snprintf(editor->pending,sizeof(editor->pending),"%s",json);wipe(json,strlen(json));free(json);editor->page=CONFIRM;draw();return;
 }
 if(f->type==DSR_SETTING_BOOL)snprintf(editor->pending,sizeof(editor->pending),"%s",lv_obj_has_state(editor->choice,LV_STATE_CHECKED)?"true":"false");
 else if(f->type==DSR_SETTING_ENUM)lv_dropdown_get_selected_str(editor->choice,editor->pending,sizeof(editor->pending));
 else snprintf(editor->pending,sizeof(editor->pending),"%s",lv_textarea_get_text(editor->input));
 editor->page=CONFIRM;draw();
}
static void draw(void)
{
 if(!editor)return;
 lv_obj_t *root=screen();
 if(editor->page==GROUPS){
  for(unsigned i=0;i<editor->snapshot.group_count;i++){const dsr_setting_group_t *g=&editor->snapshot.groups[i];char title[96];if(g->singleton)snprintf(title,sizeof(title),"%s",dsr_sc_tr(group_name(g->component)));else snprintf(title,sizeof(title),"%s • %u",dsr_sc_tr(group_name(g->component)),g->id);button(editor->body,dsr_sc_user(title),(int)i*58,choose_group,i);}
  if(!editor->snapshot.group_count)dsr_sc_text(editor->body,"Brak dostępnych ustawień",8,24,280,&dsr_roboto_16,DSR_SC_MUTED);
  dsr_ui_button(root,"Odczytaj ponownie",12,412,296,reload,NULL);return;
 }
 if(editor->page==FIELDS){
  if(cyber_general()){general_fields(root);return;}
  for(unsigned i=0;i<editor->snapshot.field_count;i++){unsigned index=editor->snapshot.fields[i];const dsr_setting_field_t *f=dsr_setting_field(index);if(!f)continue;char value[129];if(!dsr_device_settings_value(index,value,sizeof(value)))continue;
   lv_obj_t *b=dsr_sc_button(editor->body,0,(int)i*76,296,68,DSR_SC_CARD,10,select_field,(void *)(uintptr_t)index);dsr_sc_text(b,f->label,10,10,276,&dsr_roboto_14,DSR_SC_TEXT);
   dsr_sc_text(b,f->type==DSR_SETTING_SECRET?"Zmień hasło":!strcmp(value,"true")?"Włączone":!strcmp(value,"false")?"Wyłączone":dsr_sc_user(value),10,39,276,&dsr_roboto_12,DSR_SC_MUTED);
  }
  if(!editor->snapshot.field_count)dsr_sc_text(editor->body,"Brak obsługiwanych pól w odpowiedzi urządzenia",8,24,280,&dsr_roboto_16,DSR_SC_MUTED);
  dsr_ui_button(root,"Wstecz",12,412,296,back,NULL);return;
 }
 if(editor->page==WAITING){dsr_sc_text(editor->body,"Odczyt i weryfikacja ustawień…",8,36,280,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(editor->body,"Zapis jest wykonywany jeden raz. Po utracie odpowiedzi pilot sprawdzi wynik.",8,120,280,&dsr_roboto_14,DSR_SC_MUTED);dsr_ui_button(root,"Wstecz",12,412,296,back,NULL);return;}
 if(editor->page==RESULT){const dsr_device_settings_snapshot_t *s=&editor->snapshot;
  const char *text=s->uncertain?"Zapis mógł dotrzeć. Wynik niepotwierdzony; odczytaj ustawienia ponownie.":s->result==DSR_OK?"Ustawienie zapisane i potwierdzone":error_text(s->result);
  dsr_sc_text(editor->body,text,8,24,280,&dsr_roboto_18,DSR_SC_TEXT);
  if(s->restart_required)dsr_sc_text(editor->body,"Shelly wymaga ponownego uruchomienia, aby zastosować zmianę.",8,145,280,&dsr_roboto_14,0xffc18c);
  dsr_ui_button(root,"Odczytaj ponownie",12,350,296,reload,NULL);dsr_ui_button(root,"Wstecz",12,412,296,back,NULL);return;
 }
 const dsr_setting_field_t *f=dsr_setting_field(editor->field);if(!f)return;
 bool hud=cyber_general();
 if(hud){dsr_ui_cyber_panel(editor->body,false);lv_obj_set_style_bg_color(editor->body,dsr_sc_color(DSR_SC_CARD),0);lv_obj_set_style_bg_opa(editor->body,LV_OPA_COVER,0);lv_obj_set_style_radius(editor->body,10,0);lv_obj_set_style_border_color(editor->body,lv_color_hex(0x00cfff),0);dsr_sc_icon(editor->body,general_icon(f),8,7,24,DSR_SC_GLOW);}
 lv_obj_t *caption=dsr_sc_text(editor->body,network_field(f)&&editor->network_step?"Hasło Wi-Fi":f->label,hud?42:8,8,hud?246:280,&dsr_roboto_16,DSR_SC_TEXT);lv_obj_set_height(caption,22);lv_label_set_long_mode(caption,LV_LABEL_LONG_DOT);
 if(editor->page==CONFIRM){
  dsr_sc_text(editor->body,"Zmiana zostanie zapisana w urządzeniu Shelly.",8,62,280,&dsr_roboto_14,DSR_SC_TEXT);
  dsr_sc_text(editor->body,network_field(f)?dsr_sc_user(editor->network_name):f->type==DSR_SETTING_SECRET?"Nowe hasło • ukryte":dsr_sc_user(editor->pending),8,130,280,&dsr_roboto_16,DSR_SC_BLUE);
  if(!strcmp(f->component,"WiFi")||!strcmp(f->component,"BLE")||!strcmp(f->component,"MQTT")||!strcmp(f->component,"Cover"))dsr_sc_text(editor->body,"Zmiana może przerwać połączenie lub zmienić działanie urządzenia.",8,190,280,&dsr_roboto_14,0xffc18c);
  dsr_ui_button(root,"Wstecz",12,hud?420:412,142,back,NULL);dsr_ui_primary(dsr_ui_button(root,"Zapisz w Shelly",166,hud?420:412,142,apply,NULL));return;
 }
 char current[129];dsr_device_settings_value(editor->field,current,sizeof(current));
 if(network_field(f)&&editor->network_step){
  lv_obj_set_height(editor->body,100);editor->input=lv_textarea_create(editor->body);dsr_ui_textarea(editor->input);lv_obj_set_pos(editor->input,8,36);lv_obj_set_size(editor->input,280,44);lv_textarea_set_one_line(editor->input,true);lv_textarea_set_password_mode(editor->input,true);lv_textarea_set_max_length(editor->input,63);
  editor->open_network=lv_checkbox_create(editor->body);lv_checkbox_set_text(editor->open_network,dsr_sc_tr("Bez hasła"));lv_obj_set_pos(editor->open_network,8,82);lv_obj_set_style_text_font(editor->open_network,&dsr_roboto_12,0);
  editor->keyboard=lv_keyboard_create(root);dsr_ui_keyboard(editor->keyboard);lv_obj_align(editor->keyboard,LV_ALIGN_BOTTOM_MID,0,0);lv_keyboard_set_textarea(editor->keyboard,editor->input);lv_obj_add_event_cb(editor->keyboard,confirm,LV_EVENT_READY,NULL);dsr_ui_button(root,"Wstecz",12,190,142,back,NULL);dsr_ui_primary(dsr_ui_button(root,"Dalej",166,190,142,confirm,NULL));return;
 }
 if(f->type==DSR_SETTING_BOOL){editor->choice=lv_switch_create(editor->body);lv_obj_set_pos(editor->choice,12,88);lv_obj_set_size(editor->choice,70,38);if(!strcmp(current,"true"))lv_obj_add_state(editor->choice,LV_STATE_CHECKED);}
 else if(f->type==DSR_SETTING_ENUM){editor->choice=lv_dropdown_create(editor->body);lv_obj_set_pos(editor->choice,8,70);lv_obj_set_size(editor->choice,280,48);lv_dropdown_set_options(editor->choice,f->choices);unsigned i=0;const char *p=f->choices;while(p&&*p){const char *end=strchr(p,'\n');size_t n=end?(size_t)(end-p):strlen(p);if(strlen(current)==n&&!memcmp(current,p,n)){lv_dropdown_set_selected(editor->choice,i);break;}p=end?end+1:NULL;i++;}}
 else {
  lv_obj_set_height(editor->body,100);editor->input=lv_textarea_create(editor->body);dsr_ui_textarea(editor->input);lv_obj_set_pos(editor->input,8,36);lv_obj_set_size(editor->input,280,44);lv_textarea_set_one_line(editor->input,true);lv_textarea_set_max_length(editor->input,f->type==DSR_SETTING_NUMBER?32:f->max_length);lv_textarea_set_text(editor->input,current);lv_textarea_set_password_mode(editor->input,f->type==DSR_SETTING_SECRET);
  if(hud){lv_obj_set_style_border_color(editor->input,lv_color_hex(0x00cfff),LV_STATE_FOCUSED);lv_obj_add_state(editor->input,LV_STATE_FOCUSED);}
  if(f->type==DSR_SETTING_NUMBER){char range[80];snprintf(range,sizeof(range),"%.10g – %.10g",f->minimum,f->maximum);dsr_sc_text(editor->body,dsr_sc_user(range),8,83,280,&dsr_roboto_12,DSR_SC_MUTED);}
  editor->keyboard=lv_keyboard_create(root);dsr_ui_keyboard(editor->keyboard);lv_obj_align(editor->keyboard,LV_ALIGN_BOTTOM_MID,0,0);lv_keyboard_set_textarea(editor->keyboard,editor->input);lv_keyboard_set_mode(editor->keyboard,f->type==DSR_SETTING_NUMBER?LV_KEYBOARD_MODE_NUMBER:LV_KEYBOARD_MODE_TEXT_LOWER);lv_obj_add_event_cb(editor->keyboard,confirm,LV_EVENT_READY,NULL);
 }
 int y=editor->keyboard?190:hud?420:412;dsr_ui_button(root,"Wstecz",12,y,142,back,NULL);dsr_ui_primary(dsr_ui_button(root,"Dalej",166,y,142,confirm,NULL));
}
void dsr_ui_device_settings_open(uint32_t id)
{
 dsr_network_home(NULL);dsr_mobile_hide();
#ifdef ESP_PLATFORM
 editor=heap_caps_calloc(1,sizeof(*editor),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
 editor=calloc(1,sizeof(*editor));
#endif
 if(!editor){dsr_network_device_settings(id);return;}editor->id=id;editor->active=true;dsr_device_settings_visibility(true);request(DSR_SETTINGS_LOAD,0,NULL);
}
void dsr_ui_device_settings_tick(void)
{
 if(!editor||!editor->active||editor->page!=WAITING)return;
 if(editor->deferred){dsr_result_t r=dsr_network_device_settings_request(editor->id,editor->deferred_action,editor->deferred_selection,editor->deferred_action==DSR_SETTINGS_SAVE?editor->pending:NULL);if(r==DSR_BUSY&&lv_tick_get()-editor->requested_at<30000)return;editor->deferred=false;wipe(editor->pending,sizeof(editor->pending));if(r!=DSR_OK){editor->snapshot.result=r;editor->page=RESULT;draw();return;}}
 dsr_device_settings_snapshot(&editor->snapshot);if(editor->snapshot.busy)return;
 if(editor->snapshot.result!=DSR_OK||editor->snapshot.uncertain||editor->snapshot.restart_required)editor->page=RESULT;
 else editor->page=editor->snapshot.ready?FIELDS:GROUPS;
 draw();
}
