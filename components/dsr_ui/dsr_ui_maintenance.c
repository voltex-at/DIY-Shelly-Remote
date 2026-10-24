#include "dsr_ui_internal.h"
#include "dsr_ui_brand.h"
#include "dsr_maintenance_api.h"
#include "dsr_ui.h"
#include "dsr_device_settings.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
static portMUX_TYPE mailbox_lock=portMUX_INITIALIZER_UNLOCKED;
static void lock(void){portENTER_CRITICAL(&mailbox_lock);}
static void unlock(void){portEXIT_CRITICAL(&mailbox_lock);}
#else
#include <stdatomic.h>
static atomic_flag mailbox_lock=ATOMIC_FLAG_INIT;
static void lock(void){while(atomic_flag_test_and_set_explicit(&mailbox_lock,memory_order_acquire)){};}
static void unlock(void){atomic_flag_clear_explicit(&mailbox_lock,memory_order_release);}
#endif
static struct {dsr_maint_status_t status;uint32_t sequence;unsigned action;bool usb;} mailbox;
static struct {bool active,building,usb;dsr_maint_status_t shown;} view;
static struct {lv_obj_t *root;lv_timer_t *timer;dsr_maint_status_t shown;uint32_t sequence;bool confirming,usb,waiting,owner;} boot;
static bool holding,holding_boot;static uint32_t pressed_at;static uint8_t pressed_id[16];static lv_obj_t *hold_bar;
static void draw(const dsr_maint_status_t *s);
static void boot_draw(void);
static bool same(const dsr_maint_status_t *a,const dsr_maint_status_t *b)
{return a->op==b->op&&a->phase==b->phase&&a->result==b->result&&a->progress==b->progress&&a->can_confirm==b->can_confirm&&a->usb_required==b->usb_required&&a->photos==b->photos&&a->connections==b->connections&&a->rules==b->rules&&a->archive.created==b->archive.created&&!strcmp(a->archive.firmware,b->archive.firmware)&&!memcmp(a->request_id,b->request_id,16);}
static int frame(lv_obj_t *body,const char *text,int y,bool warning)
{lv_obj_t *label=warning?dsr_sc_warning(body,text,0,y,288,&dsr_roboto_14):dsr_sc_text_panel(body,text,0,y,288,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_t *panel=lv_obj_get_parent(label);lv_obj_update_layout(panel);return y+lv_obj_get_height(panel)+12;}
static lv_obj_t *body(lv_obj_t *root,int y,int height)
{lv_obj_t *b=lv_obj_create(root);lv_obj_remove_style_all(b);lv_obj_set_pos(b,12,y);lv_obj_set_size(b,296,height);lv_obj_set_scroll_dir(b,LV_DIR_VER);dsr_sc_scrollbar(b);return b;}
static lv_obj_t *button(lv_obj_t *root,const char *text,int x,int y,int width,lv_event_cb_t cb,bool enabled,bool primary)
{
 lv_obj_t *b=dsr_sc_button(root,x,y,width,44,DSR_SC_PANEL,DSR_SC_RADIUS,cb,NULL);
 if(primary)dsr_ui_primary(b);
 lv_obj_t *label=dsr_sc_text(b,text,8,0,width-16,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(label);if(!enabled)lv_obj_add_state(b,LV_STATE_DISABLED);return b;
}
static void cancel(lv_event_t *event)
{(void)event;(void)dsr_maintenance_cancel(view.shown.request_id);view.active=false;holding=false;dsr_ui_system_open(NULL);}
void dsr_ui_maintenance_hide(void)
{if(view.building)return;if(view.active)(void)dsr_maintenance_cancel(view.shown.request_id);view.active=false;holding=false;hold_bar=NULL;}
static void reset_start(lv_event_t *event)
{
 (void)event;dsr_result_t result=dsr_maintenance_local_reset_request();dsr_maint_status_t s;dsr_maintenance_status(&s);
 if(result!=DSR_OK){s.phase=DSR_MAINT_FAULT;s.result=result;}draw(&s);
}
static void confirm(lv_event_t *event)
{(void)event;(void)dsr_maintenance_confirm(view.shown.request_id,view.usb);dsr_ui_maintenance_tick();}
static void restart(lv_event_t *event){(void)event;(void)dsr_maintenance_restart();}
static void usb_toggle(lv_event_t *event)
{(void)event;if(boot.root&&boot.confirming){boot.usb=!boot.usb;boot_draw();}else{view.usb=!view.usb;draw(&view.shown);}}
static void usb_button(lv_obj_t *root,int y,bool selected)
{
 lv_obj_t *b=button(root,"Potwierdzam stabilne USB",12,y,296,usb_toggle,true,selected);
 lv_obj_t *dot=dsr_sc_panel(b,270,14,16,16,selected?DSR_SC_TEXT:DSR_SC_CARD,LV_RADIUS_CIRCLE);dsr_sc_border_width(dot,1,0);dsr_sc_border_color(dot,dsr_sc_color(DSR_SC_BLUE),0);
 if(selected)lv_obj_set_style_bg_color(dot,lv_color_white(),0);
 lv_obj_t *label=lv_obj_get_child(b,0);lv_obj_set_width(label,252);lv_obj_align(label,LV_ALIGN_LEFT_MID,10,0);
}
static void hold_event(lv_event_t *event)
{
 lv_event_code_t code=lv_event_get_code(event);
 if(code==LV_EVENT_PRESSED){holding=true;holding_boot=boot.root&&boot.confirming;pressed_at=lv_tick_get();memcpy(pressed_id,view.shown.request_id,16);}
 else if(code==LV_EVENT_RELEASED||code==LV_EVENT_PRESS_LOST||code==LV_EVENT_DELETE){holding=false;if(hold_bar&&code!=LV_EVENT_DELETE)lv_bar_set_value(hold_bar,0,LV_ANIM_OFF);}
}
static void hold_control(lv_obj_t *root,int y,bool enabled)
{
 lv_obj_t *b=button(root,"Przytrzymaj 3 s",164,y,144,NULL,enabled,true);lv_obj_add_event_cb(b,hold_event,LV_EVENT_ALL,NULL);
 hold_bar=lv_bar_create(root);lv_obj_set_pos(hold_bar,12,y-8);lv_obj_set_size(hold_bar,296,4);lv_bar_set_range(hold_bar,0,3000);lv_bar_set_value(hold_bar,0,LV_ANIM_OFF);
 lv_obj_set_style_bg_color(hold_bar,dsr_sc_color(DSR_SC_CARD),LV_PART_MAIN);lv_obj_set_style_bg_opa(hold_bar,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_bg_color(hold_bar,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);lv_obj_set_style_bg_opa(hold_bar,LV_OPA_COVER,LV_PART_INDICATOR);
}
static const char *reason(const dsr_maint_status_t *s)
{
 if(s->result==DSR_OFFLINE)return "Bateria poniżej 20%. Doładuj pilot.";
 if(s->usb_required)return "Brak świeżego procentu baterii. Potwierdź stabilne USB.";
 if(s->result==DSR_FULL)return "Za mało miejsca. Dane zachowano.";
 if(s->result==DSR_BUSY)return "Zajęty. Zakończ inne operacje i ponów.";
 if(s->result==DSR_CONFLICT)return "Zgoda wygasła lub dane się zmieniły. Zacznij ponownie.";
 return "Nie potwierdzono zapisu. Sprawdź pilot przed ponowieniem.";
}
static void draw(const dsr_maint_status_t *input)
{
 dsr_maint_status_t s=*input;if(memcmp(s.request_id,view.shown.request_id,16)){view.usb=false;holding=false;}
 const char *heading=s.phase==DSR_MAINT_CONFIRM&&s.op==DSR_MAINT_RESET?"Pełny reset danych":s.phase==DSR_MAINT_CONFIRM&&s.op==DSR_MAINT_IMPORT_COMMIT?"Przywróć tę kopię":"Kopia i reset";
 view.building=true;lv_obj_t *root=dsr_ui_screen(heading,s.phase!=DSR_MAINT_COMMITTED);view.building=false;view.active=true;view.shown=s;holding=false;hold_bar=NULL;
 if(s.phase!=DSR_MAINT_COMMITTED)dsr_sc_header_back(root,cancel,NULL);
 bool landing=s.phase==DSR_MAINT_IDLE||s.phase==DSR_MAINT_COMPLETE||s.phase==DSR_MAINT_FAULT;
 lv_obj_t *b=body(root,88,s.usb_required||landing?216:264);int y=0;bool consent=s.phase==DSR_MAINT_CONFIRM;
 if(consent){
  const char *title=s.op==DSR_MAINT_EXPORT?"Zezwól na eksport":s.op==DSR_MAINT_IMPORT_UPLOAD?"Zezwól na przesłanie":s.op==DSR_MAINT_IMPORT_COMMIT?"Przywróć tę kopię":"Usuń wszystkie dane pilota";
  if(s.op<DSR_MAINT_IMPORT_COMMIT)y=frame(b,title,y,false);
  const char *detail=s.op==DSR_MAINT_EXPORT?"Pełna kopia obejmuje hasła, zdjęcia, połączenia i Bluetooth. Panel zaszyfruje plik Twoim hasłem.":s.op==DSR_MAINT_IMPORT_UPLOAD?"Zezwalasz tylko na przesłanie i sprawdzenie kopii. Odtworzenie wymaga osobnej zgody na LCD.":s.op==DSR_MAINT_IMPORT_COMMIT?"Ta kopia zastąpi dane pilota. Po restarcie sparuj panel WWW ponownie.":"Znikną Wi-Fi, konfiguracja, zdjęcia, połączenia, Bluetooth i dostęp WWW. Firmware pozostanie.";
  if(s.op==DSR_MAINT_IMPORT_COMMIT){
   char counts[128],date[40],identity[128];struct tm tm;time_t stamp=(time_t)s.archive.created;
   if(s.archive.created&&gmtime_r(&stamp,&tm))strftime(date,sizeof(date),"UTC: %Y-%m-%d %H:%M:%S",&tm);else snprintf(date,sizeof(date),"%s",dsr_sc_tr("Czas kopii nieustawiony"));
   snprintf(identity,sizeof(identity),"Firmware: %s\n%s",s.archive.firmware,date);y=frame(b,dsr_sc_user(identity),y,false);
   snprintf(counts,sizeof(counts),dsr_sc_tr("Zdjęcia: %u • Połączenia: %u\nReguły: %u"),(unsigned)s.photos,(unsigned)s.connections,(unsigned)s.rules);y=frame(b,dsr_sc_user(counts),y,false);y=frame(b,"Kopia zastąpi dane i odtworzy włączone reguły dla zapisanych, zweryfikowanych tożsamości. Sparuj WWW ponownie. Shelly zachowają stan.",y,true);
  }
  else y=frame(b,detail,y,s.op==DSR_MAINT_RESET);
  if(s.result!=DSR_OK||s.usb_required)y=frame(b,reason(&s),y,true);
  if(s.op==DSR_MAINT_RESET&&!s.usb_required)y=frame(b,"Urządzenia Shelly zachowają swój obecny stan.",y,false);
  if(s.usb_required)usb_button(root,312,view.usb);
  button(root,"Anuluj",12,368,144,cancel,true,false);
  if(s.op==DSR_MAINT_RESET)hold_control(root,368,s.can_confirm);else button(root,"Potwierdź",164,368,144,confirm,s.can_confirm,true);
 }else if(s.phase==DSR_MAINT_COMMITTED){
  y=frame(b,"Zapisano intencję operacji. Po restarcie pilot dokończy odzyskiwanie danych.",y,true);
  if(s.result!=DSR_OK)y=frame(b,reason(&s),y,true);
  button(root,"Restart i odzyskiwanie",12,368,296,restart,true,true);
 }else if(s.phase==DSR_MAINT_WAITING||s.phase==DSR_MAINT_CAPTURING||s.phase==DSR_MAINT_VALIDATING||s.phase==DSR_MAINT_STAGING||s.phase==DSR_MAINT_GRANTED){
  y=frame(b,s.phase==DSR_MAINT_GRANTED?"Zgoda jednorazowa. Kontynuuj w panelu WWW.":"Przygotowanie i sprawdzanie danych…",y,false);
  lv_obj_t *spinner=lv_spinner_create(b);lv_spinner_set_anim_params(spinner,1200,240);lv_obj_set_pos(spinner,110,y+8);lv_obj_set_size(spinner,64,64);lv_obj_set_style_arc_color(spinner,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);
  button(root,"Anuluj",12,368,296,cancel,true,false);
 }else{
  if(s.phase==DSR_MAINT_FAULT)y=frame(b,reason(&s),y,true);
  y=frame(b,"Otwórz sparowany panel WWW: System i OTA → Kopia i przywracanie. Hasło kopii wpisujesz tylko w przeglądarce.",y,false);
  y=frame(b,"Przywracanie działa na tym samym pilocie, także po pełnym resecie.",y,false);
  button(root,"Pełny reset danych",12,316,296,reset_start,true,false);button(root,"Wstecz",12,368,296,cancel,true,false);
 }
 (void)y;
}
void dsr_ui_maintenance_open(lv_event_t *event)
{(void)event;dsr_maint_status_t s;dsr_maintenance_status(&s);draw(&s);}
static void hold_tick(bool early)
{
 if(!holding||holding_boot!=early)return;
 uint32_t elapsed=lv_tick_elaps(pressed_at);if(hold_bar)lv_bar_set_value(hold_bar,(int32_t)(elapsed>3000?3000:elapsed),LV_ANIM_OFF);if(elapsed<3000)return;
 holding=false;
 if(early&&boot.confirming){lock();if(!mailbox.action){mailbox.action=2;mailbox.usb=boot.usb;}unlock();boot.confirming=false;boot.waiting=true;boot_draw();}
 else if(!early&&view.active){dsr_maint_status_t s;dsr_maintenance_status(&s);if(s.op==DSR_MAINT_RESET&&s.can_confirm&&!memcmp(s.request_id,pressed_id,16))(void)dsr_maintenance_confirm(pressed_id,view.usb);}
}
void dsr_ui_maintenance_tick(void)
{
 dsr_maint_status_t s;dsr_maintenance_status(&s);
 if(view.active){if(memcmp(s.request_id,view.shown.request_id,16))holding=false;else hold_tick(false);dsr_maintenance_status(&s);if(!same(&s,&view.shown))draw(&s);}
 else if(s.phase==DSR_MAINT_CONFIRM&&s.can_confirm&&!dsr_ap_sensitive()&&!dsr_ble_rpc_sensitive()&&!dsr_device_settings_sensitive())draw(&s);
}
void dsr_ui_maintenance_boot_publish(const dsr_maint_status_t *s)
{if(!s)return;lock();mailbox.status=*s;if(!++mailbox.sequence)mailbox.sequence++;unlock();}
unsigned dsr_ui_maintenance_boot_take_action(bool *usb)
{lock();unsigned action=mailbox.action;if(usb)*usb=mailbox.usb;mailbox.action=0;unlock();return action;}
static void boot_retry(lv_event_t *event)
{(void)event;lock();if(!mailbox.action)mailbox.action=1;unlock();boot.waiting=true;boot_draw();}
static void boot_reset(lv_event_t *event){(void)event;boot.confirming=true;boot.usb=false;boot_draw();}
static void boot_cancel(lv_event_t *event){(void)event;boot.confirming=false;boot_draw();}
static void boot_draw(void)
{
 holding=false;hold_bar=NULL;if(boot.root)lv_obj_delete(boot.root);
 boot.root=dsr_sc_panel(lv_screen_active(),0,0,320,480,DSR_SC_BG,0);lv_obj_add_flag(boot.root,LV_OBJ_FLAG_CLICKABLE);dsr_sc_background(boot.root);
 dsr_sc_text(boot.root,"Odzyskiwanie danych",12,24,296,&dsr_roboto_18,DSR_SC_TEXT);lv_obj_t *b=body(boot.root,72,270);int y=0;
 if(boot.confirming){
  y=frame(b,"Usuń wszystkie dane pilota",y,true);y=frame(b,"Znikną Wi-Fi, konfiguracja, zdjęcia, połączenia, Bluetooth i dostęp WWW. Firmware pozostanie.",y,true);y=frame(b,"Urządzenia Shelly zachowają swój obecny stan.",y,false);
  if(boot.shown.usb_required){usb_button(boot.root,352,boot.usb);y=frame(b,reason(&boot.shown),y,true);}
  button(boot.root,"Anuluj",12,420,144,boot_cancel,true,false);hold_control(boot.root,420,true);
 }else if(boot.shown.phase==DSR_MAINT_FAULT&&!boot.waiting){
  y=frame(b,"Odzyskiwanie zatrzymane. Zwykłe usługi nie zostały uruchomione. Dane zachowano do ponowienia.",y,true);y=frame(b,reason(&boot.shown),y,true);
  button(boot.root,"Ponów",12,360,296,boot_retry,true,true);button(boot.root,"Pełny reset danych",12,420,296,boot_reset,true,false);
 }else{
  y=frame(b,boot.shown.phase==DSR_MAINT_COMPLETE?"Dane gotowe. Uruchamianie…":"Nie odłączaj zasilania. Pilot dokańcza zapis danych.",y,true);
  lv_obj_t *bar=lv_bar_create(boot.root);lv_obj_set_pos(bar,12,360);lv_obj_set_size(bar,296,14);lv_bar_set_range(bar,0,100);lv_bar_set_value(bar,boot.shown.progress,LV_ANIM_OFF);lv_obj_set_style_bg_color(bar,dsr_sc_color(DSR_SC_CARD),LV_PART_MAIN);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_bg_color(bar,dsr_sc_color(DSR_SC_BLUE),LV_PART_INDICATOR);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_INDICATOR);
  char text[24];snprintf(text,sizeof(text),"%u%%",(unsigned)boot.shown.progress);lv_obj_t *label=dsr_sc_text_panel(boot.root,dsr_sc_user(text),12,388,296,&dsr_roboto_18,DSR_SC_TEXT);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(label);
 }
 (void)y;
}
static void boot_tick(lv_timer_t *timer)
{
 (void)timer;dsr_maint_status_t s;uint32_t sequence;lock();s=mailbox.status;sequence=mailbox.sequence;unlock();
 if(sequence&&sequence!=boot.sequence){boot.sequence=sequence;boot.shown=s;boot.confirming=false;boot.waiting=false;boot_draw();}
 hold_tick(true);if(boot.root)lv_display_trigger_activity(NULL);
}
void dsr_ui_maintenance_boot_begin(void)
{if(!boot.timer)boot.timer=lv_timer_create(boot_tick,100,NULL);}
void dsr_ui_maintenance_boot_owner(bool running){boot.owner=running;}
bool dsr_ui_maintenance_boot_available(void){return boot.timer&&boot.owner;}
void dsr_ui_maintenance_boot_release(void)
{if(boot.timer)lv_timer_delete(boot.timer);if(boot.root)lv_obj_delete(boot.root);memset(&boot,0,sizeof(boot));holding=false;hold_bar=NULL;lock();memset(&mailbox,0,sizeof(mailbox));unlock();}
