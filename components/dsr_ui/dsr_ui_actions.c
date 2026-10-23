#include "dsr_ui_internal.h"
#include "dsr_ui_search.h"
#include "dsr_actions.h"
#include "dsr_workspace.h"
#include "esp_random.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
static lv_obj_t *body;
static bool active;static uint8_t page;static uint32_t next_draw,last_status;static dsr_result_t admission;
typedef struct {
 char query[48];uint32_t room,revision;bool typing;
 lv_obj_t *search,*keyboard;
 lv_image_dsc_t photos[8];
} browse_t;
_Static_assert(sizeof(browse_t)<=512,"bounded presentation heap in PSRAM");
static browse_t *browse_context;
#define browse (*browse_context)
static bool browse_prepare(void)
{
 if(browse_context)return true;
#ifdef ESP_PLATFORM
 browse_context=heap_caps_calloc(1,sizeof(*browse_context),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
 browse_context=calloc(1,sizeof(*browse_context));
#endif
 return browse_context!=NULL;
}
static void draw(void);
void dsr_ui_actions_hide(void){active=false;body=NULL;if(browse_context){browse.search=browse.keyboard=NULL;browse.typing=false;}}
static const char *outcome(unsigned value)
{return value==DSR_ACTION_SUCCESS?"Potwierdzono":value==DSR_ACTION_FAILED?"Błąd kroku":value==DSR_ACTION_UNCERTAIN?"Wynik niepewny • sprawdź stan":value==DSR_ACTION_CANCELLED?"Anulowano pozostałe kroki":"Oczekuje";}
static const char *command(unsigned value)
{return value==DSR_ACTION_ON?"Włącz":value==DSR_ACTION_OFF?"Wyłącz":value==DSR_ACTION_OPEN?"Otwórz":value==DSR_ACTION_CLOSE?"Zamknij":value==DSR_ACTION_STOP?"Stop":"Uruchomienie";}
static const char *reason(dsr_result_t result)
{return result==DSR_AUTH_REQUIRED?"Błąd logowania":result==DSR_IDENTITY_MISMATCH?"Zmieniona tożsamość":result==DSR_RATE_LIMITED?"Limit zapytań":result==DSR_UNSUPPORTED?"Nieobsługiwany kanał":result==DSR_OFFLINE?"Brak odpowiedzi":result==DSR_IO?"Błąd pamięci":"Stan niepotwierdzony";}
static uint32_t status_hash(const dsr_actions_snapshot_t *s)
{
 uint32_t hash=s->run_id*33u+s->admission_generation;hash=hash*33u+s->current;hash=hash*33u+s->active;hash=hash*33u+s->available;
 for(unsigned i=0;i<s->count;i++){hash=hash*33u+s->steps[i].outcome;hash=hash*33u+(uint32_t)s->steps[i].result;}return hash;
}
static lv_obj_t *button(lv_obj_t *parent,const char *text,int x,int y,int width,lv_event_cb_t cb,uintptr_t data)
{lv_obj_t *b=dsr_sc_button(parent,x,y,width,44,DSR_SC_CARD,8,cb,(void *)data);lv_obj_t *l=dsr_sc_text(b,text,3,0,width-6,width<=136?&dsr_roboto_12:&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(l);return b;}
static void back(lv_event_t *event){(void)event;dsr_network_rooms();}
static void choose(lv_event_t *event){page=(uint8_t)(uintptr_t)lv_event_get_user_data(event);browse.query[0]=0;browse.room=0;admission=DSR_OK;draw();}
void dsr_ui_actions_execute(uint8_t kind,uint32_t id,bool on)
{
 if(!browse_prepare())return;
 dsr_actions_snapshot_t status;dsr_actions_snapshot(&status);char request[256];
 snprintf(request,sizeof(request),"{\"id\":%u,\"revision\":%u,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%08x%08x\"%s}",(unsigned)id,(unsigned)status.revision,(unsigned)status.boot_id,(unsigned)status.admission_generation,(unsigned)esp_random(),(unsigned)esp_random(),kind?"":on?",\"on\":true":",\"on\":false");
 uint32_t run=0;dsr_result_t accepted=dsr_actions_request(request,kind,1,&run);dsr_network_prepare_home();active=true;page=3;admission=accepted;draw();
}
static void run(lv_event_t *event)
{uintptr_t value=(uintptr_t)lv_event_get_user_data(event);dsr_ui_actions_execute((uint8_t)(value&1),(uint32_t)(value>>2),(value&2)!=0);}
static void cancel(lv_event_t *event)
{(void)event;dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);(void)dsr_actions_cancel(s.boot_id,s.run_id);draw();}
static void progress(lv_obj_t *root,const dsr_actions_snapshot_t *s)
{
 uint32_t warning=dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xf0a020:0xffc107;bool important=admission!=DSR_OK;
 for(unsigned i=0;i<s->count;i++)if(s->steps[i].outcome==DSR_ACTION_FAILED||s->steps[i].outcome==DSR_ACTION_UNCERTAIN)important=true;
 char text[128];snprintf(text,sizeof(text),"#%u • %u/%u",(unsigned)s->run_id,(unsigned)s->current,(unsigned)s->count);dsr_sc_text(root,text,12,96,296,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(root,admission!=DSR_OK?"Nie przyjęto. Odśwież i sprawdź urządzenia.":s->active?"Wykonywanie kolejno. Możliwy częściowy wynik.":s->run_id?"Zakończono. Wyniki dotyczą każdego kroku osobno.":"Brak uruchomienia",12,125,296,&dsr_roboto_12,important?warning:DSR_SC_MUTED);
 lv_obj_t *list=body=dsr_sc_panel(root,12,177,296,228,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);
 for(unsigned i=0;i<s->count;i++){const dsr_action_result_t *step=&s->steps[i];snprintf(text,sizeof(text),"#%u • %s",(unsigned)step->layout_id,dsr_sc_tr(command(step->command)));dsr_sc_text(list,text,6,(int)i*58,270,&dsr_roboto_14,DSR_SC_TEXT);snprintf(text,sizeof(text),"%s%s%s",dsr_sc_tr(outcome(step->outcome)),step->result==DSR_OK?"":" • ",step->result==DSR_OK?"":dsr_sc_tr(reason(step->result)));dsr_sc_text(list,text,6,(int)i*58+24,270,&dsr_roboto_12,step->outcome==DSR_ACTION_FAILED||step->outcome==DSR_ACTION_UNCERTAIN?warning:DSR_SC_MUTED);}
 if(s->active)button(root,"Anuluj pozostałe kroki",12,426,296,cancel,0);else dsr_sc_text(root,"Bez automatycznego ponawiania poleceń.",12,432,296,&dsr_roboto_12,DSR_SC_MUTED);
}
static const cJSON *get(const cJSON *o,const char *key){return cJSON_GetObjectItemCaseSensitive(o,key);}
static int number(const cJSON *o,const char *key){const cJSON *v=get(o,key);return cJSON_IsNumber(v)?v->valueint:0;}
static const char *text(const cJSON *o,const char *key){const cJSON *v=get(o,key);return cJSON_IsString(v)?v->valuestring:"";}
static void edit(lv_event_t *event)
{dsr_ui_edit_action(page,(uint32_t)(uintptr_t)lv_event_get_user_data(event));}
static void search_done(lv_event_t *event)
{(void)event;if(browse.search)snprintf(browse.query,sizeof(browse.query),"%s",lv_textarea_get_text(browse.search));browse.typing=false;draw();}
static void search_focus(lv_event_t *event)
{browse.typing=true;lv_obj_set_height(body,90);dsr_ui_keyboard_focus(browse.keyboard,lv_event_get_target(event));}
static void room_filter(lv_event_t *event)
{
 unsigned at=lv_dropdown_get_selected(lv_event_get_target(event));browse.room=0;
 char *json=dsr_workspace_snapshot();cJSON *r=json?cJSON_Parse(json):NULL;free(json);
 if(at)browse.room=(uint32_t)number(cJSON_GetArrayItem(get(r,"rooms"),(int)at-1),"id");
 cJSON_Delete(r);draw();
}
static __attribute__((noinline)) int object_card(lv_obj_t *list,const cJSON *r,const cJSON *v,unsigned i,int y,const dsr_actions_snapshot_t *s)
{
 unsigned id=(unsigned)number(v,"id"),count=(unsigned)cJSON_GetArraySize(get(v,page?"actions":"members"));
 const char *photo=text(v,"photo");bool has_photo=dsr_ui_photo_source(photo,&browse.photos[i]);int height=has_photo?164:132;
 lv_obj_t *card=dsr_sc_panel(list,0,y,280,height,DSR_SC_CARD,12);lv_obj_set_style_clip_corner(card,true,0);
 const cJSON *room=NULL,*candidate;
 cJSON_ArrayForEach(candidate,get(r,"rooms"))if(number(candidate,"id")==number(v,"room"))room=candidate;
 if(has_photo){
  lv_obj_t *clip=dsr_sc_panel(card,0,0,280,112,DSR_SC_PANEL,0);lv_obj_set_style_clip_corner(clip,true,0);
  lv_obj_t *image=lv_image_create(clip);lv_image_set_src(image,&browse.photos[i]);
  uint32_t scale=280u*256u/browse.photos[i].header.w,vertical=112u*256u/browse.photos[i].header.h;
  lv_image_set_scale(image,scale>vertical?scale:vertical);lv_obj_center(image);
 }
 if(has_photo){lv_obj_t *shade=dsr_sc_panel(card,0,0,280,112,0x101820,0);lv_obj_set_style_bg_opa(shade,170,0);}
 lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(text(v,"name")),10,10,210,&dsr_roboto_16,has_photo?0xffffff:DSR_SC_TEXT);
 lv_obj_set_height(name,44);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_DOTS);
 lv_obj_t *pencil=dsr_sc_button(card,232,4,44,44,DSR_SC_PANEL,10,edit,(void *)(uintptr_t)id);dsr_sc_icon_centered(pencil,DSR_ICON_EDIT,24,DSR_SC_BLUE);
 char caption[96];snprintf(caption,sizeof(caption),"%s • %u",*text(room,"name")?text(room,"name"):dsr_sc_tr("Cały dom"),count);
 dsr_sc_text(card,dsr_sc_user(caption),10,has_photo?83:60,258,&dsr_roboto_12,has_photo?0xffffff:DSR_SC_TEXT);
 lv_obj_t *first=button(card,page?"Uruchom scenę":"Włącz grupę",4,has_photo?114:82,page?272:132,run,((uintptr_t)id<<2)|(page?1:2));
 if(!s->available||!count)lv_obj_add_state(first,LV_STATE_DISABLED);
 if(!page){lv_obj_t *off=button(card,"Wyłącz grupę",144,has_photo?114:82,132,run,(uintptr_t)id<<2);if(!s->available||!count)lv_obj_add_state(off,LV_STATE_DISABLED);}
 return height;
}
static __attribute__((noinline)) void objects(lv_obj_t *root,const dsr_actions_snapshot_t *s)
{
 lv_obj_t *add=button(root,page?"+ Scena":"+ Grupa",12,104,112,edit,0);dsr_ui_primary(add);
 browse.search=lv_textarea_create(root);dsr_ui_textarea(browse.search);lv_obj_set_pos(browse.search,132,104);lv_obj_set_size(browse.search,176,44);lv_textarea_set_one_line(browse.search,true);lv_textarea_set_max_length(browse.search,47);lv_textarea_set_placeholder_text(browse.search,dsr_sc_tr("Szukaj"));lv_textarea_set_text(browse.search,browse.query);
 lv_obj_add_event_cb(browse.search,search_focus,LV_EVENT_FOCUSED,NULL);lv_obj_add_event_cb(browse.search,search_focus,LV_EVENT_CLICKED,NULL);
 lv_obj_t *list=body=dsr_sc_panel(root,12,208,296,s->active?156:192,DSR_SC_BG,0);lv_obj_set_style_bg_opa(list,LV_OPA_TRANSP,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);
 char *json=dsr_workspace_snapshot();cJSON *r=json?cJSON_Parse(json):NULL;free(json);char error[96];
 if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);dsr_sc_text(list,"Konfiguracja niedostępna",8,20,270,&dsr_roboto_14,DSR_SC_MUTED);return;}
 char options[512];snprintf(options,sizeof(options),"%s",dsr_sc_tr("Cały dom"));unsigned index=0,selected=0;const cJSON *v;
 cJSON_ArrayForEach(v,get(r,"rooms")){strncat(options,"\n",sizeof(options)-strlen(options)-1);strncat(options,text(v,"name"),sizeof(options)-strlen(options)-1);index++;if((uint32_t)number(v,"id")==browse.room)selected=index;}
 lv_obj_t *filter=lv_dropdown_create(root);lv_obj_set_pos(filter,12,156);lv_obj_set_size(filter,296,44);lv_obj_set_style_text_font(filter,&dsr_roboto_14,0);lv_obj_set_style_text_font(lv_dropdown_get_list(filter),&dsr_roboto_14,0);lv_dropdown_set_options(filter,options);lv_dropdown_set_selected(filter,selected);lv_obj_add_event_cb(filter,room_filter,LV_EVENT_VALUE_CHANGED,NULL);
 unsigned i=0;int y=0;cJSON_ArrayForEach(v,get(r,page?"scenes":"groups")){
  if(browse.room&&browse.room!=(uint32_t)number(v,"room"))continue;
  if(!dsr_ui_search_match(text(v,"name"),browse.query))continue;
  y+=object_card(list,r,v,i,y,s)+12;i++;
 }
 if(!i){dsr_sc_icon(list,page?DSR_ICON_SCENES:DSR_ICON_GRID,116,24,48,DSR_SC_MUTED);dsr_sc_text(list,"Dodaj pierwszy wpis przyciskiem + u góry.",12,92,260,&dsr_roboto_16,DSR_SC_TEXT);}
 browse.revision=(uint32_t)number(r,"revision");cJSON_Delete(r);
 if(s->active)button(root,"Pokaż bieżące wykonanie",12,372,296,choose,3);else dsr_sc_text(root,page?"Sceny uruchamiane ręcznie":"Cały dom • każde urządzenie ma własny wynik.",12,410,296,&dsr_roboto_11,DSR_SC_MUTED);
 browse.keyboard=lv_keyboard_create(root);dsr_ui_keyboard(browse.keyboard);lv_obj_add_event_cb(browse.keyboard,search_done,LV_EVENT_READY,NULL);lv_obj_add_event_cb(browse.keyboard,search_done,LV_EVENT_CANCEL,NULL);lv_obj_add_flag(browse.keyboard,LV_OBJ_FLAG_HIDDEN);
}
static void draw(void)
{
 if(!active)return;
 int scroll=body?lv_obj_get_scroll_y(body):0;body=NULL;lv_obj_t *root=lv_screen_active();
 if(page<2)dsr_mobile_actions_shell(root,page);
 else{lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);dsr_sc_header(root,"Wykonanie",false,dsr_wifi_connected(),back,NULL);}
 dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);last_status=page==3?status_hash(&s):(uint32_t)s.available*2u+s.active;if(page==3)progress(root,&s);else objects(root,&s);if(body)lv_obj_scroll_to_y(body,scroll,LV_ANIM_OFF);lv_obj_update_layout(root);next_draw=lv_tick_get()+1000;
}
void dsr_ui_actions_open(uint8_t kind)
{dsr_network_prepare_home();if(!browse_prepare()){lv_obj_t *root=dsr_ui_screen("Konfiguracja niedostępna",false);dsr_sc_text(root,"Brak pamięci do otwarcia edytora. Ustawienia pozostają zachowane.",12,96,296,&dsr_roboto_16,DSR_SC_TEXT);dsr_ui_button(root,"Wstecz",12,420,296,back,NULL);return;}dsr_mobile_hide();page=kind<2?kind:0;browse.query[0]=0;browse.room=0;admission=DSR_OK;active=true;draw();}
void dsr_ui_actions_tick(void){if(!active)return;dsr_actions_snapshot_t status;dsr_actions_snapshot(&status);bool held=false;for(lv_indev_t *i=lv_indev_get_next(NULL);i;i=lv_indev_get_next(i))if(lv_indev_get_type(i)==LV_INDEV_TYPE_POINTER&&(lv_indev_get_state(i)==LV_INDEV_STATE_PRESSED||lv_indev_get_scroll_obj(i)))held=true;
 if(!held&&!browse.typing&&((page==3?status_hash(&status):(uint32_t)status.available*2u+status.active)!=last_status||(page==3&&(int32_t)(lv_tick_get()-next_draw)>=0)||(page<2&&browse.revision!=dsr_workspace_revision())))draw();}
