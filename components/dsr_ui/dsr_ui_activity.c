#include "dsr_ui_internal.h"
#include "dsr_actions.h"
#include <stdio.h>
#include <string.h>

enum {PAGE_SIZE=6};
/* LVGL owner only. Copy six redacted records, never retain backend pointers. */
static struct {
 dsr_action_log_entry_t entries[PAGE_SIZE];
 uint32_t epoch,confirm_epoch;unsigned offset,count,total;
 bool active,available,confirming;
} view;
static void draw(bool confirming);
void dsr_ui_activity_hide(void){view.active=false;view.confirming=false;view.confirm_epoch=0;}
static bool load(uint32_t epoch,unsigned offset)
{
 dsr_action_log_page_t page;
 bool ok=dsr_actions_log(epoch,offset,PAGE_SIZE,&page);
 if(!ok&&epoch){offset=0;ok=dsr_actions_log(0,0,PAGE_SIZE,&page);}
 view.available=ok;view.count=view.total=0;view.offset=offset;
 if(ok){view.epoch=page.epoch;view.count=page.count;view.total=page.total;memcpy(view.entries,page.entries,page.count*sizeof(page.entries[0]));}
 return ok;
}
static const char *command(unsigned value)
{
 switch(value){case DSR_ACTION_ON:return "Włącz";case DSR_ACTION_OFF:return "Wyłącz";case DSR_ACTION_OPEN:return "Otwórz";
 case DSR_ACTION_CLOSE:return "Zamknij";case DSR_ACTION_STOP:return "Stop";case DSR_ACTION_POSITION:return "Pozycja";
 case DSR_ACTION_BRIGHTNESS:return "Jasność";case DSR_ACTION_RGB:return "Kolor";case DSR_ACTION_TARGET:return "Ustaw temperaturę";default:return "Uruchomienie";}
}
static const char *result(dsr_result_t value)
{
 switch(value){case DSR_AUTH_REQUIRED:return "Błąd logowania";case DSR_IDENTITY_MISMATCH:return "Zmieniona tożsamość";
 case DSR_RATE_LIMITED:return "Limit zapytań";case DSR_UNSUPPORTED:return "Nieobsługiwany kanał";case DSR_OFFLINE:return "Brak odpowiedzi";
 case DSR_IO:return "Błąd pamięci";case DSR_BUSY:return "Zajęty";default:return "Stan niepotwierdzony";}
}
static const char *outcome(unsigned value)
{
 switch(value){case DSR_ACTION_SUCCESS:return "Zakończono";case DSR_ACTION_FAILED:return "Błąd";case DSR_ACTION_UNCERTAIN:return "Stan niepotwierdzony";
 case DSR_ACTION_CANCELLED:return "Anulowano";default:return "Oczekuje";}
}
static void back(lv_event_t *event)
{(void)event;if(view.confirming){draw(false);return;}dsr_ui_activity_hide();dsr_ui_system_open(NULL);}
static void refresh(lv_event_t *event){(void)event;load(0,0);draw(false);}
static void newer(lv_event_t *event){(void)event;load(view.epoch,view.offset>=PAGE_SIZE?view.offset-PAGE_SIZE:0);draw(false);}
static void older(lv_event_t *event){(void)event;if(view.offset+view.count<view.total){load(view.epoch,view.offset+PAGE_SIZE);draw(false);}}
static void ask_clear(lv_event_t *event){(void)event;draw(true);}
static void cancel_clear(lv_event_t *event){(void)event;draw(false);}
static void confirm_clear(lv_event_t *event)
{
 (void)event;
 /* Another browser may have cleared and appended since the dialog opened. */
 (void)dsr_actions_log_clear_epoch(view.confirm_epoch);load(0,0);draw(false);
}
static lv_obj_t *button(lv_obj_t *root,const char *text,int x,int y,int w,lv_event_cb_t cb,bool enabled)
{
 lv_obj_t *b=dsr_sc_button(root,x,y,w,44,DSR_SC_PANEL,DSR_SC_RADIUS,cb,NULL);
 lv_obj_t *label=dsr_sc_text(b,text,4,0,w-8,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(label);
 if(!enabled)lv_obj_add_state(b,LV_STATE_DISABLED);
 return b;
}
static int text_height(const char *text,const lv_font_t *font,int width)
{lv_point_t size;lv_text_get_size(&size,text,dsr_sc_font(font),0,0,width,LV_TEXT_FLAG_NONE);return size.y;}
static int entry(lv_obj_t *body,const dsr_action_log_entry_t *e,int y)
{
 char stamp[48],meta[112],detail[160];
 if(e->utc)snprintf(stamp,sizeof(stamp),"%02u:%02u:%02u UTC",(e->utc/3600)%24,(e->utc/60)%60,e->utc%60);
 else snprintf(stamp,sizeof(stamp),dsr_sc_tr("Brak UTC • %u s od startu"),e->monotonic_ms/1000);
 const char *source=e->kind==2?"Odczyt":e->source==0?"WWW":e->source==1?"LCD":e->source==3?"Automatyzacja":"Źródło nieznane";
 snprintf(meta,sizeof(meta),"%s • %s • #%u",stamp,dsr_sc_tr(source),(unsigned)(e->layout_id?e->layout_id:e->object_id));
 snprintf(detail,sizeof(detail),"%s • %s",dsr_sc_tr(e->kind==2?"Zaobserwowano":command(e->command)),dsr_sc_tr(e->kind==2?(e->command?command(e->command):e->result==DSR_OK?"Online":"Offline"):outcome(e->outcome)));
 int mh=text_height(meta,&dsr_roboto_12,264),dh=text_height(detail,&dsr_roboto_14,264);
 bool warning=e->result!=DSR_OK||e->outcome==DSR_ACTION_FAILED||e->outcome==DSR_ACTION_UNCERTAIN;
 const char *reason=dsr_sc_tr(result(e->result));int rh=warning?text_height(reason,&dsr_roboto_14,264)+6:0;
 int height=24+mh+6+dh+rh;uint32_t gold=dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;
 lv_obj_t *card=dsr_sc_panel(body,0,y,288,height,DSR_SC_CARD,DSR_SC_RADIUS);
 dsr_sc_text(card,dsr_sc_user(meta),12,12,264,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(card,dsr_sc_user(detail),12,18+mh,264,&dsr_roboto_14,warning?gold:DSR_SC_TEXT);
 if(warning)dsr_sc_text(card,dsr_sc_user(reason),12,24+mh+dh,264,&dsr_roboto_14,gold);
 return y+height+8;
}
static void draw(bool confirming)
{
 uint32_t epoch=view.epoch;lv_obj_t *root=dsr_ui_screen("Aktywność",true);
 view.active=true;view.confirming=confirming;view.confirm_epoch=confirming?epoch:0;dsr_sc_header_back(root,back,NULL);
 if(confirming){
  dsr_sc_warning(root,"Wyczyścić historię aktywności?",12,96,296,&dsr_roboto_18);
  dsr_sc_text_panel(root,"Znikną tylko wpisy RAM. Konfiguracja i połączenia pozostaną.",12,192,296,&dsr_roboto_14,DSR_SC_MUTED);
  button(root,"Anuluj",12,316,296,cancel_clear,true);button(root,"Wyczyść historię",12,368,296,confirm_clear,true);return;
 }
 char count[48];snprintf(count,sizeof(count),"%u–%u / %u",view.count?view.offset+1:0,view.offset+view.count,view.total);
 dsr_sc_text(root,dsr_sc_user(count),12,84,296,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *body=lv_obj_create(root);lv_obj_remove_style_all(body);lv_obj_set_pos(body,12,108);lv_obj_set_size(body,296,196);lv_obj_set_scroll_dir(body,LV_DIR_VER);dsr_sc_scrollbar(body);
 if(!view.available)dsr_sc_warning(body,"Aktywność niedostępna",0,0,288,&dsr_roboto_14);
 else if(!view.count)dsr_sc_text_panel(body,"Brak aktywności od uruchomienia.",0,0,288,&dsr_roboto_14,DSR_SC_MUTED);
 else for(unsigned i=0,y=0;i<view.count;i++)y=(unsigned)entry(body,&view.entries[i],(int)y);
 button(root,"Nowsze",12,316,144,newer,view.available&&view.offset>0);
 button(root,"Starsze",164,316,144,older,view.available&&view.offset+view.count<view.total);
 lv_obj_t *reload=button(root,"Odśwież",12,368,144,refresh,true);dsr_ui_primary(reload);
 button(root,"Wyczyść",164,368,144,ask_clear,view.available&&view.total>0);
}
void dsr_ui_activity_open(lv_event_t *event)
{(void)event;memset(&view,0,sizeof(view));load(0,0);draw(false);}
void dsr_ui_activity_tick(void)
{
 if(!view.active||view.confirming)return;
 dsr_action_log_page_t probe;
 if(dsr_actions_log(0,0,1,&probe)&&probe.epoch!=view.epoch){load(0,0);draw(false);}
}
