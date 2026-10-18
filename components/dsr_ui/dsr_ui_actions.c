#include "dsr_ui_internal.h"
#include "dsr_actions.h"
#include "dsr_workspace.h"
#include "esp_random.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static lv_obj_t *body;
static bool active;static uint8_t page;static uint32_t next_draw,last_status;static dsr_result_t admission;
static void draw(void);
void dsr_ui_actions_hide(void){active=false;body=NULL;}
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
{lv_obj_t *b=dsr_sc_button(parent,x,y,width,40,DSR_SC_CARD,8,cb,(void *)data);lv_obj_t *l=dsr_sc_text(b,text,3,12,width-6,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);return b;}
static void back(lv_event_t *event){(void)event;dsr_network_rooms();}
static void choose(lv_event_t *event){page=(uint8_t)(uintptr_t)lv_event_get_user_data(event);admission=DSR_OK;draw();}
void dsr_ui_actions_execute(uint8_t kind,uint32_t id,bool on)
{
 dsr_actions_snapshot_t status;dsr_actions_snapshot(&status);char request[256];
 snprintf(request,sizeof(request),"{\"id\":%u,\"revision\":%u,\"boot_id\":%u,\"admission_generation\":%u,\"nonce\":\"%08x%08x\"%s}",(unsigned)id,(unsigned)status.revision,(unsigned)status.boot_id,(unsigned)status.admission_generation,(unsigned)esp_random(),(unsigned)esp_random(),kind?"":on?",\"on\":true":",\"on\":false");
 uint32_t run=0;dsr_result_t accepted=dsr_actions_request(request,kind,1,&run);dsr_network_home(NULL);dsr_mobile_hide();active=true;page=3;admission=accepted;draw();
}
static void run(lv_event_t *event)
{uintptr_t value=(uintptr_t)lv_event_get_user_data(event);dsr_ui_actions_execute((uint8_t)(value&1),(uint32_t)(value>>2),(value&2)!=0);}
static void cancel(lv_event_t *event)
{(void)event;dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);(void)dsr_actions_cancel(s.boot_id,s.run_id);draw();}
static void progress(lv_obj_t *root,const dsr_actions_snapshot_t *s)
{
 char text[128];snprintf(text,sizeof(text),"#%u • %u/%u",(unsigned)s->run_id,(unsigned)s->current,(unsigned)s->count);dsr_sc_text(root,text,12,96,296,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(root,admission!=DSR_OK?"Nie przyjęto. Odśwież i sprawdź urządzenia.":s->active?"Wykonywanie kolejno. Możliwy częściowy wynik.":s->run_id?"Zakończono. Wyniki dotyczą każdego kroku osobno.":"Brak uruchomienia",12,125,296,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *list=body=dsr_sc_panel(root,12,177,296,228,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);
 for(unsigned i=0;i<s->count;i++){const dsr_action_result_t *step=&s->steps[i];snprintf(text,sizeof(text),"#%u • %s",(unsigned)step->layout_id,dsr_sc_tr(command(step->command)));dsr_sc_text(list,text,6,(int)i*58,270,&dsr_roboto_14,DSR_SC_TEXT);snprintf(text,sizeof(text),"%s%s%s",dsr_sc_tr(outcome(step->outcome)),step->result==DSR_OK?"":" • ",step->result==DSR_OK?"":dsr_sc_tr(reason(step->result)));dsr_sc_text(list,text,6,(int)i*58+24,270,&dsr_roboto_12,step->outcome==DSR_ACTION_UNCERTAIN?0xffc18c:DSR_SC_MUTED);}
 if(s->active)button(root,"Anuluj pozostałe kroki",12,426,296,cancel,0);else dsr_sc_text(root,"Bez automatycznego ponawiania poleceń.",12,432,296,&dsr_roboto_12,DSR_SC_MUTED);
}
static void objects(lv_obj_t *root,const dsr_actions_snapshot_t *s)
{
 dsr_sc_text(root,"Cały dom • każde urządzenie ma własny wynik.",12,96,296,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *list=body=dsr_sc_panel(root,12,132,296,274,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);
 char *json=dsr_workspace_snapshot();cJSON *r=json?cJSON_Parse(json):NULL;free(json);char error[96];
 if(!r||!dsr_config_validate(r,error)){cJSON_Delete(r);dsr_sc_text(list,"Konfiguracja niedostępna",8,20,270,&dsr_roboto_14,DSR_SC_MUTED);return;}
 const cJSON *v,*all=cJSON_GetObjectItemCaseSensitive(r,page?"scenes":"groups");unsigned i=0;
 cJSON_ArrayForEach(v,all){const cJSON *id=cJSON_GetObjectItemCaseSensitive(v,"id"),*name=cJSON_GetObjectItemCaseSensitive(v,"name"),*members=cJSON_GetObjectItemCaseSensitive(v,page?"actions":"members");int y=(int)i++*100;dsr_sc_text(list,dsr_sc_name(name->valuestring),6,y+4,270,&dsr_roboto_16,DSR_SC_TEXT);
  lv_obj_t *first=button(list,page?"Uruchom scenę":"Włącz grupę",0,y+36,page?280:136,run,((uintptr_t)id->valueint<<2)|(page?1:2));if(!s->available||!cJSON_GetArraySize(members))lv_obj_add_state(first,LV_STATE_DISABLED);
  if(!page){lv_obj_t *off=button(list,"Wyłącz grupę",144,y+36,136,run,(uintptr_t)id->valueint<<2);if(!s->available||!cJSON_GetArraySize(members))lv_obj_add_state(off,LV_STATE_DISABLED);}
 }
 if(!i)dsr_sc_text(list,"Dodaj wpis w edytorze konfiguracji.",8,20,270,&dsr_roboto_14,DSR_SC_MUTED);
 cJSON_Delete(r);
 if(s->active)button(root,"Pokaż bieżące wykonanie",12,427,296,choose,3);else dsr_sc_text(root,"Polecenia wymagają zgodnego powiązania i uprawnień.",12,420,296,&dsr_roboto_14,DSR_SC_MUTED);
}
static void draw(void)
{
 if(!active)return;
 int scroll=body?lv_obj_get_scroll_y(body):0;body=NULL;lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 dsr_sc_header(root,page==0?"Grupy":page==1?"Sceny":"Wykonanie",false,dsr_wifi_connected(),back,NULL);
 /* LCD "Aktywność" removed in 0.7.51 (owner); groups and scenes only. */
 const char *tabs[]={"Grupy","Sceny"};for(unsigned i=0;i<2;i++){lv_obj_t *b=button(root,tabs[i],(int)i*162,45,158,choose,i);if(page==i){lv_obj_set_style_border_width(b,2,0);lv_obj_set_style_border_color(b,dsr_sc_color(DSR_SC_BLUE),0);}}
 dsr_actions_snapshot_t s;dsr_actions_snapshot(&s);last_status=status_hash(&s);if(page==3)progress(root,&s);else objects(root,&s);if(body)lv_obj_scroll_to_y(body,scroll,LV_ANIM_OFF);lv_obj_update_layout(root);next_draw=lv_tick_get()+1000;
}
void dsr_ui_actions_open(uint8_t kind)
{dsr_network_home(NULL);dsr_mobile_hide();page=kind<2?kind:0;admission=DSR_OK;active=true;draw();}
void dsr_ui_actions_tick(void){if(!active)return;dsr_actions_snapshot_t status;dsr_actions_snapshot(&status);if(status_hash(&status)!=last_status||(int32_t)(lv_tick_get()-next_draw)>=0)draw();}
