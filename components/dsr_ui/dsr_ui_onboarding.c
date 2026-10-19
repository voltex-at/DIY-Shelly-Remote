#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include <stdio.h>

/* Repo-owned composition, using the existing Apache-2.0 symbol atlas. The
 * modal retains real HOME pixels but suspends its callbacks and rendering. */
static bool active,last_wifi,last_cleanup;
static int selected=-1;
static lv_event_cb_t manual_open;
static lv_obj_t *cards[4],*radios[4],*descriptions[3],*next_button,*hint;
static const char footer_tag;
static const char *names[]={"Bluetooth","Wi-Fi AP","Sieć lokalna","Dodaj ręcznie (IP)"};
static const char *descriptions_ready[]={"Reklamy BLE • także bez Wi-Fi","Otwarty punkt dostępowy Shelly","Urządzenia w domowej sieci"};
bool dsr_ui_onboarding_active(void){return active;}
void dsr_ui_onboarding_reset(void)
{active=false;next_button=hint=NULL;for(unsigned i=0;i<4;i++)cards[i]=radios[i]=NULL;for(unsigned i=0;i<3;i++)descriptions[i]=NULL;}
static bool available(unsigned method){return method==0||method==3||dsr_wifi_connected();}
static bool cleanup_pending(void){return dsr_network_bind_busy()||dsr_ui_ap_active()||dsr_ui_ble_rpc_active()||dsr_network_ota_busy()||dsr_worker_stop_pending();}
static void refresh_selection(void)
{
 last_wifi=dsr_wifi_connected();last_cleanup=cleanup_pending();
 for(unsigned i=0;i<4;i++){
  bool enabled=available(i),chosen=selected==(int)i;
  if(enabled)lv_obj_remove_state(cards[i],LV_STATE_DISABLED);else lv_obj_add_state(cards[i],LV_STATE_DISABLED);
  lv_obj_set_style_border_width(cards[i],chosen?2:1,0);
  lv_obj_set_style_border_color(cards[i],dsr_sc_color(chosen?DSR_SC_BLUE:DSR_SC_EDGE),0);
  lv_obj_set_style_bg_color(radios[i],dsr_sc_color(chosen?DSR_SC_BLUE:DSR_SC_CARD),0);
  lv_obj_set_style_border_color(radios[i],dsr_sc_color(chosen?DSR_SC_BLUE:DSR_SC_MUTED),0);
  if(i<3)dsr_sc_label_set_text(descriptions[i],enabled?descriptions_ready[i]:"Najpierw połącz domowe Wi-Fi");
 }
 bool ready=selected>=0&&available((unsigned)selected)&&!cleanup_pending();
 if(ready)lv_obj_remove_state(next_button,LV_STATE_DISABLED);else lv_obj_add_state(next_button,LV_STATE_DISABLED);
 dsr_sc_label_set_text(hint,cleanup_pending()?"Kończenie operacji…":"Więcej opcji poniżej");
}
void dsr_ui_onboarding_tick(void){if(active&&(last_wifi!=dsr_wifi_connected()||last_cleanup!=cleanup_pending()))refresh_selection();}
static void select_method(lv_event_t *event)
{
 unsigned index=(unsigned)(uintptr_t)lv_event_get_user_data(event);
 if(!active||index>=4||!available(index))return;
 selected=(int)index;refresh_selection(); /* Selection never invokes transport. */
}
static void cancel(lv_event_t *event){(void)event;active=false;dsr_network_home(NULL);}
static void next(lv_event_t *event)
{
 (void)event;if(!active||selected<0||!available((unsigned)selected)||cleanup_pending())return;
 int method=selected;active=false;
 if(method==0)dsr_ui_ble_scan_open(NULL);else if(method==1)dsr_ui_ap_scan_open(NULL);
 else if(method==2)dsr_ui_lan_open(NULL);else if(manual_open)manual_open(NULL);
}
static void segments(lv_obj_t *root,unsigned step,int y)
{
 for(unsigned i=0;i<3;i++)dsr_sc_panel(root,12+(int)i*102,y,92,4,i<step?DSR_SC_BLUE:DSR_SC_EDGE,2);
}
lv_obj_t *dsr_onboarding_screen(const char *title,unsigned step)
{
 lv_obj_t *root=dsr_ui_screen(title,false);
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *child=lv_obj_get_child(root,(int)i);
  if(lv_obj_check_type(child,&lv_label_class)&&lv_obj_get_style_y(child,0)==46){lv_obj_set_width(child,184);lv_label_set_long_mode(child,LV_LABEL_LONG_CLIP);dsr_sc_label_set_text(child,title);}
  if(lv_obj_get_style_y(child,0)==75)lv_obj_add_flag(child,LV_OBJ_FLAG_HIDDEN);
 }
 if(step<1)step=1;
 if(step>3)step=3;
 char caption[40];snprintf(caption,sizeof(caption),dsr_sc_tr("Krok %u z 3"),step);
 lv_obj_t *label=dsr_sc_text(root,caption,204,53,104,&dsr_roboto_11,DSR_SC_MUTED);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_RIGHT,0);
 segments(root,step,76);return root;
}
lv_obj_t *dsr_onboarding_footer(lv_obj_t *root,const char *back,lv_event_cb_t back_cb,const char *forward,lv_event_cb_t forward_cb)
{
 lv_obj_t *footer=dsr_sc_panel(root,0,408,320,72,DSR_SC_BG,0);lv_obj_set_user_data(footer,(void *)&footer_tag);lv_obj_add_flag(footer,LV_OBJ_FLAG_CLICKABLE);
 dsr_sc_panel(footer,12,0,296,1,DSR_SC_EDGE,0);
 if(!forward){dsr_ui_button(footer,back,12,12,296,back_cb,NULL);return NULL;}
 dsr_ui_button(footer,back,12,12,140,back_cb,NULL);
 lv_obj_t *button=dsr_ui_button(footer,forward,168,12,140,forward_cb,NULL);dsr_ui_primary(button);return button;
}
void dsr_onboarding_keyboard(lv_obj_t *root,bool shown)
{
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *child=lv_obj_get_child(root,(int)i);if(lv_obj_get_user_data(child)!=(void *)&footer_tag)continue;
  if(shown&&dsr_sc_style()!=DSR_SC_STYLE_CYBER_BLUE)lv_obj_add_flag(child,LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(child,LV_OBJ_FLAG_HIDDEN);
 }
}
lv_obj_t *dsr_onboarding_result(lv_obj_t *list,int y,dsr_ui_icon_t icon,const char *title,const char *detail,const char *status,bool verified,lv_event_cb_t callback,void *context)
{
 lv_obj_t *row=dsr_sc_button(list,0,y,296,88,DSR_SC_CARD,12,callback,context);
 lv_obj_set_style_border_width(row,1,0);lv_obj_set_style_border_color(row,dsr_sc_color(DSR_SC_EDGE),0);
 dsr_sc_icon(row,icon,12,26,28,DSR_SC_TEXT);
 lv_obj_t *a=dsr_sc_text(row,title,52,9,230,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_set_height(a,22);lv_label_set_long_mode(a,LV_LABEL_LONG_CLIP);
 lv_obj_t *b=dsr_sc_text(row,detail,52,34,230,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_set_height(b,19);lv_label_set_long_mode(b,LV_LABEL_LONG_CLIP);
 lv_obj_t *c=dsr_sc_text(row,status,52,59,230,&dsr_roboto_12,verified?DSR_SC_BLUE:DSR_SC_MUTED);lv_obj_set_height(c,19);lv_label_set_long_mode(c,LV_LABEL_LONG_CLIP);
 return row;
}
void dsr_ui_onboarding_open(lv_event_cb_t manual)
{
 active=true;selected=-1;manual_open=manual;
 lv_obj_t *overlay=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);
 lv_obj_set_style_bg_opa(overlay,170,0);lv_obj_add_flag(overlay,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *sheet=dsr_sc_panel(overlay,0,72,320,408,DSR_SC_BG,18);lv_obj_add_flag(sheet,LV_OBJ_FLAG_CLICKABLE);
 dsr_sc_panel(sheet,138,8,44,4,DSR_SC_MUTED,2);
 dsr_sc_text(sheet,"Dodaj urządzenie",16,25,288,&dsr_roboto_18,DSR_SC_TEXT);
 dsr_sc_text(sheet,"Krok 1 z 3",16,57,288,&dsr_roboto_12,DSR_SC_MUTED);segments(sheet,1,80);
 lv_obj_t *list=dsr_sc_panel(sheet,12,96,296,230,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);
 for(unsigned i=0;i<4;i++){
  lv_obj_t *row=cards[i]=dsr_sc_button(list,0,(int)i*78,296,i<3?72:52,DSR_SC_CARD,12,select_method,(void *)(uintptr_t)i);
  lv_obj_set_style_bg_color(row,dsr_sc_color(DSR_SC_PANEL),LV_STATE_DISABLED);lv_obj_set_style_opa(row,LV_OPA_COVER,LV_STATE_DISABLED);
  dsr_sc_icon(row,i==0?DSR_ICON_CONTACT:i==1?DSR_ICON_WIFI:i==2?DSR_ICON_GRID:DSR_ICON_SETTINGS,12,i<3?24:13,24,DSR_SC_TEXT);
  dsr_sc_text(row,names[i],48,i<3?9:15,206,&dsr_roboto_16,DSR_SC_TEXT);
  if(i<3)descriptions[i]=dsr_sc_text(row,descriptions_ready[i],48,35,206,&dsr_roboto_12,DSR_SC_MUTED);
  radios[i]=dsr_sc_panel(row,264,i<3?26:16,18,18,DSR_SC_CARD,9);lv_obj_set_style_border_width(radios[i],1,0);
 }
 hint=dsr_sc_text(sheet,"Więcej opcji poniżej",128,57,176,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_RIGHT,0);
 next_button=dsr_onboarding_footer(overlay,"Anuluj",cancel,"Dalej",next);refresh_selection();
 /* A new front door always starts at the first method. Resolve content bounds
  * before resetting so a previous focused/advanced row cannot shift the list. */
 lv_obj_update_layout(list);lv_obj_scroll_to_y(list,0,LV_ANIM_OFF);
}
