#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_ui_cyber.h"
#include "dsr_ui_internal.h"
#include "dsr_workspace.h"
#include "dsr_board.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_attr.h"
#endif

_Static_assert(sizeof(dsr_mobile_model_t)<=8192,"bounded shared mobile view model");
_Static_assert(sizeof(dsr_mobile_device_t)==336&&sizeof(dsr_mobile_model_t)==7200,"Light uses existing mobile padding");
/* UI-task snapshots only: no ISR/DMA or cache-off access. Keep their 14.4 KB
 * outside internal RAM, using the board's existing external-BSS mapping. */
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static dsr_mobile_model_t model;
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static dsr_mobile_model_t preview_saved;
static bool cover_preview;
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static dsr_mobile_actions_t actions;
static dsr_mobile_page_t page;
static unsigned selected;
static dsr_mobile_page_t previous_page;
static unsigned previous_selected;
static bool active;
static lv_obj_t *content;
static lv_obj_t *sample_age;
static lv_obj_t *info_overlay;
static bool info_open,cover_drag,cover_drag_group;
static uint8_t cover_preview_position;
static uint32_t cover_drag_id;
static lv_obj_t *cover_preview_label;
static lv_obj_t *cover_position_label;
static dsr_cyber_shutter_t *cover_visual;
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static struct {uint32_t id;int from;bool animate,group;} cover_transition;
static lv_timer_t *age_timer;
static unsigned rendered_language,rendered_style;
static bool rgb_color_view,rgb_dirty;
static uint32_t rgb_draft_id;
static uint8_t rgb_draft[3];
static lv_obj_t *rgb_swatch,*rgb_values[3],*rgb_apply_button;
static bool light_drag;
static uint32_t light_drag_id;
static lv_obj_t *light_slider;
static lv_obj_t *light_knob,*light_value,*wheel_knob,*rgb_text;
static bool lighting_page(void);
static dsr_mobile_device_t *lighting_device(void);
static bool lighting_selected(void);
static void lighting_light_details(lv_obj_t *root);
static void lighting_rgb_details(lv_obj_t *root);
static void rgb_details(lv_obj_t *root);
static void trv_details_view(lv_obj_t *root,const dsr_mobile_device_t *d,bool group);
static void trv_knob(uint8_t half);
static bool group_trv_selected(void);
static bool group_shared_selected(void);
static bool group_cover_selected(void);
static dsr_mobile_device_t *detail_device(void);
static void group_refresh_members(void);
static void shared_cover_details(lv_obj_t *root,dsr_mobile_device_t *d);
static void cover_view_reset(void);
static void relay_details(lv_obj_t *root,const dsr_mobile_device_t *d);
static int group_display_index(const dsr_mobile_device_t *d);
static void trv_round_action(lv_obj_t *parent,unsigned index,int x,int y,int size,bool plus,bool valid,lv_event_cb_t callback);
static const char *group_display_state(const dsr_mobile_device_t *d);
static void lighting_light_tabs(lv_obj_t *root);
static void render(void);
static void trv_reset(void);
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static bool repaint_pending;
static bool wake_repaint;
static bool pointer_busy(void)
{
 for(lv_indev_t *i=lv_indev_get_next(NULL);i;i=lv_indev_get_next(i))if(lv_indev_get_type(i)==LV_INDEV_TYPE_POINTER&&(lv_indev_get_state(i)==LV_INDEV_STATE_PRESSED||lv_indev_get_scroll_obj(i)))return true;
 return false;
}
/* Cards on the list pages, so a poll that changes values rebuilds only the
 * affected cards (0.7.50) instead of the whole screen. */
typedef struct{lv_obj_t *obj;uint8_t type,index,fields;int16_t x,y,w,h;} placed_card_t;
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static placed_card_t placed[24+DSR_UI_MAX_DEVICES];
static unsigned placed_count;
static void place(lv_obj_t *obj,uint8_t type,unsigned index,unsigned fields,int x,int y,int w,int h)
{if(obj&&placed_count<sizeof(placed)/sizeof(placed[0]))placed[placed_count++]=(placed_card_t){obj,type,(uint8_t)index,(uint8_t)fields,(int16_t)x,(int16_t)y,(int16_t)w,(int16_t)h};}
static bool unbound(const dsr_mobile_device_t *d);
static bool trv_device(const dsr_mobile_device_t *d);static bool trv_known(const dsr_mobile_device_t *d);static bool trv_can(const dsr_mobile_device_t *d);
static uint8_t trv_shown(const dsr_mobile_device_t *d);static int trv_slot(uint32_t id);static void trv_round(lv_obj_t *parent,unsigned index,int x,int y,int size,bool plus,bool valid);
static void update_sample_age(void)
{
 if(!active||!sample_age)return;
 if(page==DSR_MOBILE_GROUP_CONTROL){
  if(selected>=model.group_count)return;
  unsigned total=0,online=0;for(unsigned i=0;i<model.device_count;i++)if(model.groups[selected].member_mask&(1u<<i)){total++;online+=model.devices[i].online&&model.devices[i].error==DSR_OK;}
  char text[80];snprintf(text,sizeof(text),dsr_sc_tr("Urządzenia: %u • online: %u"),total,online);lv_label_set_text(sample_age,text);return;
 }
 if(selected>=model.device_count)return;
 const dsr_mobile_device_t *d=&model.devices[selected];char text[72];
 if(d->has_sample_time&&(dsr_sc_style()!=DSR_SC_STYLE_CYBER_BLUE||(d->online&&d->error==DSR_OK)))snprintf(text,sizeof(text),dsr_sc_tr(DSR_ENDPOINT_IS_SENSOR(d->endpoint_kind)?"Odpowiedź odebrana %u s temu":"Odczyt potwierdzony %u s temu"),(unsigned)((lv_tick_get()-d->sample_ms)/1000));
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Brak potwierdzonego odczytu"));
 if(strcmp(lv_label_get_text(sample_age),text))lv_label_set_text(sample_age,text);
}
/* Nothing is visible while the backlight is off: no age-label or deferred
 * repaints then. Waking runs the deferred repaint on the next 30 ms tick. */
static void age_tick(lv_timer_t *timer)
{
 (void)timer;
 if(dsr_board_display_sleeping()){if(active)wake_repaint=true;return;}
 if(active&&(repaint_pending||wake_repaint)&&!pointer_busy()){
  bool pending=repaint_pending;repaint_pending=false;
  if(cover_preview)render();else dsr_mobile_update(pending?&preview_saved:&model);
 }
 update_sample_age();
}

void dsr_mobile_bind(const dsr_mobile_actions_t *a){if(a)actions=*a;else memset(&actions,0,sizeof(actions));if(!age_timer)age_timer=lv_timer_create(age_tick,30,NULL);}
bool dsr_mobile_active(void){return active;}
dsr_mobile_page_t dsr_mobile_page(void){return page;}
static void end_preview(void){if(cover_preview){model=preview_saved;memset(&preview_saved,0,sizeof(preview_saved));cover_preview=false;}}
void dsr_mobile_hide(void){repaint_pending=wake_repaint=false;trv_reset();placed_count=0;rgb_dirty=rgb_color_view=false;cover_drag=false;cover_preview_label=cover_position_label=NULL;cover_visual=NULL;cover_view_reset();light_drag=false;light_slider=light_knob=light_value=wheel_knob=rgb_text=NULL;end_preview();active=false;info_open=false;content=sample_age=info_overlay=NULL;}
static void center(lv_obj_t *o){lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0);}
static void truncate(lv_obj_t *o,int height){lv_obj_set_height(o,height);lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_DOTS);}
static void edge(lv_obj_t *o,uint32_t c,int width){dsr_sc_border_color(o,dsr_sc_color(c),0);dsr_sc_border_width(o,width,0);}
static void navigate(lv_event_t *e)
{
    unsigned target=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    /* Shared lighting controls keep their group target through tab changes.
     * A group ordinal must never become a physical-device index. */
    if(page==DSR_MOBILE_GROUP_CONTROL&&(target==14||target==15||target==16)){
     if(target==15){rgb_color_view=rgb_dirty=false;render();}
     else if(target==14&&selected<model.group_count&&actions.edit_action)actions.edit_action(0,model.groups[selected].id);
     return;
    }
    if(target==10){if(actions.open_settings)actions.open_settings();return;}
    if(target==11){if(actions.add_device)actions.add_device();return;}
    if(target==12){
#ifdef ESP_PLATFORM
        ESP_LOGI("dsr_config_ui","stage=pencil_click callback=%u",(unsigned)(actions.edit_dashboard!=NULL));
#endif
        if(actions.edit_dashboard)actions.edit_dashboard();
        return;}
    if(target==13){if(actions.select_dashboard)actions.select_dashboard();return;}
    if(target==16){if(!cover_preview&&!model.demo&&selected<model.device_count&&actions.open_usage)actions.open_usage(model.devices[selected].id);return;}
    if(target==18||target==19){if(!model.demo&&actions.open_actions)actions.open_actions(target==18?0:1);return;}
    if(target==14||target==15){dsr_mobile_show(target==14?DSR_MOBILE_DEVICE_SETTINGS:DSR_MOBILE_DETAILS,selected);return;}
    if(target==98&&page==DSR_MOBILE_DEVICE_SETTINGS){dsr_mobile_show(DSR_MOBILE_DETAILS,selected);return;}
    if(target==98){bool previous=page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_GROUP_CONTROL;dsr_mobile_show(previous?previous_page:DSR_MOBILE_ROOMS,previous?previous_selected:0);return;}
    dsr_mobile_show((dsr_mobile_page_t)target,0);
}
static void open_device(lv_event_t *e){dsr_mobile_show(DSR_MOBILE_DETAILS,(unsigned)(uintptr_t)lv_event_get_user_data(e));}
static void open_room(lv_event_t *e){dsr_mobile_show(DSR_MOBILE_ROOM,(unsigned)(uintptr_t)lv_event_get_user_data(e));}
static void add_room(lv_event_t *e){
#ifdef ESP_PLATFORM
 ESP_LOGI("dsr_config_ui","stage=add_room_click event=%u demo=%u callback=%u",(unsigned)(e?lv_event_get_code(e):0),(unsigned)model.demo,(unsigned)(actions.add_room!=NULL));
#endif
 (void)e;if(!model.demo&&actions.add_room)actions.add_room();}
static void edit_room(lv_event_t *e)
{uint32_t id=(uint32_t)(uintptr_t)lv_event_get_user_data(e);if(!model.demo&&id&&actions.edit_room)actions.edit_room(id);}
static void refresh(lv_event_t *event){(void)event;if(page==DSR_MOBILE_GROUP_CONTROL){group_refresh_members();return;}if(!cover_preview&&!model.demo&&selected<model.device_count&&model.devices[selected].can_refresh&&actions.refresh)actions.refresh(model.devices[selected].id);}
static void edit_device(lv_event_t *event){(void)event;if(!cover_preview&&!model.demo&&selected<model.device_count&&model.devices[selected].id&&actions.edit_device)actions.edit_device(model.devices[selected].id);}
static const char *device_error(dsr_result_t result)
{
 switch(result){case DSR_AUTH_REQUIRED:return "Błąd logowania";case DSR_OFFLINE:return "Brak odpowiedzi";case DSR_IDENTITY_MISMATCH:return "Zmieniona tożsamość";case DSR_RATE_LIMITED:return "Limit zapytań";case DSR_BAD_RESPONSE:return "Stan niepotwierdzony";case DSR_UNSUPPORTED:return "Nieobsługiwany kanał";case DSR_IO:return "Błąd pamięci";case DSR_INVALID:return "Nieprawidłowe dane";default:return "Błąd komunikacji";}
}
static void control(lv_event_t *e)
{
    unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(i>=model.device_count)return;
    dsr_mobile_device_t *d=&model.devices[i];
    if(!d->online||d->busy||!d->can_switch)return;
    bool desired=!d->output;
    if(model.demo){d->output=desired;if(!desired&&d->has_power)d->power_w=0;if(!desired&&d->has_current)d->current_a=0;render();}
    else if(actions.set_output)actions.set_output(i,desired);
}

static lv_obj_t *icon_button(lv_obj_t *p,dsr_ui_icon_t icon,int x,int y,int size,uint32_t bg,
                             lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *b=dsr_sc_button(p,x,y,size,size,bg,7,cb,ctx);
    dsr_sc_icon_centered(b,icon,24,dsr_sc_icon_accent());return b;
}

static void header(lv_obj_t *root,const char *title,bool back)
{
    dsr_sc_header(root,title,model.demo,model.wifi,back?navigate:NULL,(void *)(uintptr_t)98);
}
static void close_info(lv_event_t *event){(void)event;info_open=false;info_overlay=NULL;render();}
static void dashboard_info(lv_event_t *event)
{
 (void)event;if(info_overlay)return;info_open=true;lv_obj_t *overlay=info_overlay=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);
 lv_obj_set_style_bg_opa(overlay,170,0);lv_obj_add_flag(overlay,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *panel=dsr_sc_panel(overlay,12,88,296,284,DSR_SC_CARD,14);
 dsr_sc_text(panel,"Informacje o pulpicie",12,14,272,&dsr_roboto_18,DSR_SC_TEXT);
 lv_obj_t *name=dsr_sc_text(panel,dsr_sc_name(model.dashboard_name[0]?model.dashboard_name:"Mój pulpit"),12,47,268,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,52);
 unsigned online=0;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].online)online++;
 char text[96];snprintf(text,sizeof(text),dsr_sc_tr("Urządzenia: %u • online: %u"),model.device_count,online);dsr_sc_text(panel,text,12,112,268,&dsr_roboto_14,DSR_SC_MUTED);
 snprintf(text,sizeof(text),dsr_sc_tr("Pomieszczenia: %u"),model.room_count);dsr_sc_text(panel,text,12,139,268,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_sc_text(panel,model.demo?"DEMO • dane przykładowe":"Stan online dotyczy ostatnich odczytów.",12,174,268,&dsr_roboto_12,model.demo?0xe1bb70:DSR_SC_MUTED);
 lv_obj_t *close=dsr_sc_button(panel,12,224,268,44,DSR_SC_PANEL,10,close_info,overlay);
 center(dsr_sc_text(close,"Zamknij",4,12,264,&dsr_roboto_16,DSR_SC_TEXT));
}
static void refresh_info(void)
{
 if(!info_overlay)return;
 /* The sensor modal owns a different child layout, even if its device is
  * removed/rebound while it is open. Never route it through dashboard labels. */
 if(lv_obj_get_user_data(info_overlay))return;
 lv_obj_t *panel=lv_obj_get_child(info_overlay,0);if(!panel)return;
 char text[96];unsigned online=0;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].online)online++;
 snprintf(text,sizeof(text),dsr_sc_tr("Urządzenia: %u • online: %u"),model.device_count,online);lv_obj_t *count=lv_obj_get_child(panel,2);if(strcmp(lv_label_get_text(count),text))lv_label_set_text(count,text);
 snprintf(text,sizeof(text),dsr_sc_tr("Pomieszczenia: %u"),model.room_count);count=lv_obj_get_child(panel,3);if(strcmp(lv_label_get_text(count),text))lv_label_set_text(count,text);
}
static void tab(lv_obj_t *root,const char *text,int x,int w,bool chosen,lv_event_cb_t cb,void *ctx)
{
    bool cover=(page==DSR_MOBILE_DETAILS&&selected<model.device_count&&model.devices[selected].kind==DSR_DEVICE_COVER)||group_cover_selected();
    int y=cover?52:page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS||group_shared_selected()?44:52;
    lv_obj_t *b=dsr_sc_button(root,x,y,w,44,DSR_SC_BG,7,cb,ctx);
    if(chosen){edge(b,DSR_SC_BLUE,1);lv_obj_set_style_border_side(b,LV_BORDER_SIDE_TOP|LV_BORDER_SIDE_LEFT|LV_BORDER_SIDE_RIGHT,0);}
    if(chosen&&dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_panel(b,true);
    /* Owner 2026-10: unselected Cyber tabs keep a slim contour, not the 2 px frame. */
    else if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){lv_obj_set_style_border_width(b,1,0);lv_obj_set_style_border_opa(b,LV_OPA_70,0);}
    int inset=lv_obj_get_style_border_width(b,LV_PART_MAIN);
    const char *caption=dsr_sc_tr(text);lv_point_t size;lv_text_get_size(&size,caption,dsr_sc_font(&dsr_roboto_14),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    bool device_title=(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS||group_shared_selected())&&x==12;
    const lv_font_t *font=size.x>(w-2*inset-8)*(device_title?2:1)?&dsr_roboto_12:&dsr_roboto_14;
    if(cover&&device_title){lv_text_get_size(&size,caption,dsr_sc_font(&dsr_roboto_16),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(size.x<=w-2*inset-8)font=&dsr_roboto_16;}
    lv_obj_t *s=dsr_sc_text(b,dsr_sc_user(caption),4,0,w-2*inset-8,font,chosen?DSR_SC_TEXT:DSR_SC_MUTED);lv_obj_align(s,LV_ALIGN_CENTER,0,0);
    if(device_title){lv_obj_set_style_text_align(s,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(s,LV_LABEL_LONG_MODE_WRAP);lv_obj_set_style_max_height(s,36,0);}
    if(!cb&&!chosen){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,LV_STATE_DISABLED);}
    if(!cb&&chosen)lv_obj_remove_flag(b,LV_OBJ_FLAG_CLICKABLE);
}
static void tabs(lv_obj_t *root)
{
    bool cover=(page==DSR_MOBILE_DETAILS&&selected<model.device_count&&model.devices[selected].kind==DSR_DEVICE_COVER)||group_cover_selected();
    if(lighting_page()){lighting_light_tabs(root);return;}
    if(group_shared_selected()){
        dsr_sc_panel(root,12,cover?95:87,296,1,DSR_SC_BLUE,0);
        tab(root,dsr_sc_name(model.groups[selected].name),12,192,true,NULL,NULL);
        tab(root,"Ustawienia",208,100,false,actions.edit_action?navigate:NULL,(void *)(uintptr_t)14);return;
    }
    if(page!=DSR_MOBILE_HOME)dsr_sc_panel(root,12,cover?95:page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS?87:99,296,1,DSR_SC_BLUE,0);
    /* Groups and scenes live under Pomieszczenia (owner 2026-10-18). */
    if(page==DSR_MOBILE_HOME)return;
    if(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS){
        bool settings=page==DSR_MOBILE_DEVICE_SETTINGS;
        bool editable=!cover_preview&&!model.demo&&selected<model.device_count&&model.devices[selected].id;
        tab(root,selected<model.device_count?dsr_sc_name(model.devices[selected].name):"Sterowanie",12,192,!settings,settings?navigate:NULL,(void *)(uintptr_t)15);
        tab(root,"Ustawienia",208,100,settings,!settings&&editable?navigate:NULL,(void *)(uintptr_t)14);return;
    }
    if(page==DSR_MOBILE_ROOM){tab(root,"Urządzenia",12,110,true,NULL,NULL);tab(root,"Grupy",126,82,false,actions.open_actions&&!model.demo?navigate:NULL,(void *)18);tab(root,"Sceny",212,96,false,actions.open_actions&&!model.demo?navigate:NULL,(void *)19);return;}
    tab(root,"Pomieszczenia",12,96,page==DSR_MOBILE_ROOMS,navigate,(void *)(uintptr_t)DSR_MOBILE_ROOMS);
    tab(root,"Grupy",112,60,page==DSR_MOBILE_GROUPS,actions.open_actions&&!model.demo?navigate:NULL,(void *)18);tab(root,"Sceny",176,54,page==DSR_MOBILE_SCENES,actions.open_actions&&!model.demo?navigate:NULL,(void *)19);
    tab(root,"Urządzenia",234,74,page==DSR_MOBILE_DEVICES,navigate,(void *)(uintptr_t)DSR_MOBILE_DEVICES);
}
static void bottom_action(lv_event_t *event)
{
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);
 if(i==0)dsr_mobile_show(DSR_MOBILE_HOME,0);else if(i==1)dsr_mobile_show(DSR_MOBILE_ROOMS,0);
 else if(i==2&&actions.open_battery)actions.open_battery();
 else if(i==3&&actions.open_settings)actions.open_settings();else if(i==4){if((page==DSR_MOBILE_GROUPS||page==DSR_MOBILE_SCENES)&&actions.edit_action){actions.edit_action(page==DSR_MOBILE_SCENES,0);return;}if(page==DSR_MOBILE_ROOMS)add_room(NULL);else if(actions.add_device)actions.add_device();}
}
static void bottom(lv_obj_t *root)
{
 unsigned enabled=3u|(actions.open_battery?4u:0u)|(actions.open_settings?8u:0u)|((page==DSR_MOBILE_GROUPS||page==DSR_MOBILE_SCENES?!model.demo&&actions.edit_action!=NULL:page==DSR_MOBILE_ROOMS?!model.demo&&actions.add_room!=NULL:actions.add_device!=NULL)?16u:0u);
 dsr_mobile_page_t context=page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS||page==DSR_MOBILE_GROUP_CONTROL?previous_page:page;
 dsr_sc_navigation(root,context==DSR_MOBILE_HOME?0:1,enabled,bottom_action);
}
void dsr_mobile_actions_shell(lv_obj_t *root,uint8_t kind)
{
 dsr_mobile_hide();page=kind?DSR_MOBILE_SCENES:DSR_MOBILE_GROUPS;
 lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 header(root,"",false);tabs(root);bottom(root);
}
static void toolbar(lv_obj_t *root,const char *title)
{
    if(page==DSR_MOBILE_ROOMS||page==DSR_MOBILE_ROOM){
        bool list=page==DSR_MOBILE_ROOMS;
        uint32_t id=!list&&selected<model.room_count?model.rooms[selected].id:0;
        lv_obj_t *button=dsr_sc_button(root,12,104,296,44,DSR_SC_PANEL,10,list?add_room:edit_room,(void *)(uintptr_t)id);
        if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_frame(button,false);else edge(button,DSR_SC_EDGE,1);
        dsr_sc_icon(button,list?DSR_ICON_PLUS:DSR_ICON_EDIT,12,10,24,DSR_SC_BLUE);
        lv_obj_t *name=dsr_sc_text(button,list?"Dodaj pomieszczenie":selected<model.room_count?dsr_sc_name(model.rooms[selected].name):"Edytuj pomieszczenie",48,0,232,&dsr_roboto_14,DSR_SC_TEXT);truncate(name,lv_font_get_line_height(dsr_sc_font(&dsr_roboto_14)));lv_obj_align(name,LV_ALIGN_LEFT_MID,48,0);
        if(model.demo||(list?!actions.add_room:!actions.edit_room||!id))lv_obj_add_state(button,LV_STATE_DISABLED);
        return;
    }
    /* Owner reference (2026-10): one row, dashboard selector beside three
     * square 44 px actions, so three device cards stay in view. */
    bool home=page==DSR_MOBILE_HOME,hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
    if(home){
     lv_obj_t *choose=dsr_sc_button(root,12,50,146,44,DSR_SC_PANEL,10,actions.select_dashboard?navigate:NULL,(void *)(uintptr_t)13);
     if(hud)dsr_ui_cyber_frame(choose,true);else edge(choose,DSR_SC_EDGE,1);
     int room=actions.select_dashboard?100:122;const lv_font_t *font=&dsr_roboto_16;lv_point_t size;const char *shown=dsr_sc_tr(title);
     lv_text_get_size(&size,shown,dsr_sc_font(font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(size.x>room)font=&dsr_roboto_14;
     /* Keep the owner's single toolbar row; long titles scroll in their
      * own clipped label, with LVGL's pauses at both ends. */
     int line=lv_font_get_line_height(dsr_sc_font(font));
     lv_obj_t *name=dsr_sc_text(choose,dsr_sc_user(shown),12,0,room,font,DSR_SC_TEXT);lv_obj_set_height(name,line);
     lv_obj_set_style_anim_duration(name,lv_anim_speed_clamped(32,900,16000),0);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_SCROLL);lv_obj_align(name,LV_ALIGN_LEFT_MID,12,0);
     if(actions.select_dashboard)dsr_sc_icon(choose,DSR_ICON_CHEVRON,118,14,16,hud?DSR_SC_GLOW:DSR_SC_MUTED);
    }else dsr_sc_text(root,title,12,116,136,&dsr_roboto_16,DSR_SC_TEXT);
    const dsr_ui_icon_t icons[]={DSR_ICON_PLUS,DSR_ICON_EDIT,DSR_ICON_INFO};
    for(unsigned k=0;k<3;k++){
     int x=home?164+(int)k*50:160+(int)k*52;
     lv_obj_t *b=dsr_sc_button(root,x,home?50:104,44,44,k<2&&!hud?DSR_SC_BLUE:DSR_SC_PANEL,hud?8:10,k==2?dashboard_info:navigate,(void *)(uintptr_t)(11+k));
     dsr_sc_icon_centered(b,icons[k],24,dsr_sc_icon_accent());
     if((k==0&&!actions.add_device)||(k==1&&!actions.edit_dashboard))lv_obj_add_state(b,LV_STATE_DISABLED);
    }
}
static lv_obj_t *power_button(lv_obj_t *parent,unsigned index,int x,int y,int size)
{
    dsr_mobile_device_t *d=&model.devices[index];bool valid=d->online&&!d->busy&&d->can_switch;
    return dsr_sc_power_button(parent,x,y,size,valid&&d->output,valid,DSR_ICON_POWER,DSR_SC_GLOW,control,(void *)(uintptr_t)index);
}
static dsr_ui_icon_t kind_icon(dsr_mobile_kind_t kind);
static dsr_ui_icon_t contact_icon(const dsr_mobile_device_t *d,bool open);
static bool cyber_blu(const dsr_mobile_device_t *d);
static bool cyber_contact(const dsr_mobile_device_t *d);
static bool cyber_blu_live(const dsr_mobile_device_t *d);
static dsr_ui_icon_t cyber_contact_icon(const dsr_mobile_device_t *d);
static uint32_t cyber_contact_color(const dsr_mobile_device_t *d);
/* Owner-chosen icon (Shelly app "Standardbilder"), otherwise by type. */
static dsr_ui_icon_t device_icon(const dsr_mobile_device_t *d)
{return d->icon&&d->icon<=DSR_SC_DEVICE_ICONS?dsr_sc_device_icons[d->icon-1]:trv_device(d)?DSR_ICON_THERMOSTAT:kind_icon(d->kind);}
static dsr_ui_icon_t kind_icon(dsr_mobile_kind_t kind)
{return (kind==DSR_DEVICE_LIGHT||kind==DSR_DEVICE_RGB)?DSR_ICON_LIGHT:kind==DSR_DEVICE_COVER?DSR_ICON_COVER:kind==DSR_DEVICE_CONTACT||kind==DSR_DEVICE_SENSOR?DSR_ICON_CONTACT:DSR_ICON_PLUG;}
/* Workspace card without a private binding (e.g. power cut between the
 * workspace ACK and the binding save, or a legacy display entry). It is not
 * "offline": adding the same device again reuses this card's id. */
static bool unbound(const dsr_mobile_device_t *d){return !model.demo&&group_display_index(d)<0&&!d->identity[0];}
static const char *room_name(const dsr_mobile_device_t *d){return d->room<model.room_count?model.rooms[d->room].name:"Nieprzypisane";}
static void cover_control(lv_event_t *event)
{
 uintptr_t value=(uintptr_t)lv_event_get_user_data(event);unsigned index=(unsigned)(value/2);int direction=value%2?1:-1;
 if(index>=model.device_count)return;
 dsr_mobile_device_t *d=&model.devices[index];
 if(!d->online||d->error!=DSR_OK||d->busy||!d->can_cover)return;
 if(model.demo){d->has_position=true;d->position_pct=direction>0?100:0;d->cover_state=DSR_COVER_STOPPED;render();}
 else if(actions.set_cover)actions.set_cover(index,direction);
}
static void card_control(lv_obj_t *parent,unsigned i,dsr_ui_icon_t icon,int x,int width,bool valid,lv_event_cb_t cb,uintptr_t value)
{
 (void)i;lv_obj_t *b=dsr_sc_button(parent,x,8,width,48,0xe8ecef,icon==DSR_ICON_POWER?LV_RADIUS_CIRCLE:12,cb,(void *)value);
 dsr_sc_icon_centered(b,icon,26,valid?DSR_SC_BLUE:0x818991);
 if(!valid){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_color(b,dsr_sc_color(0x555b60),LV_STATE_DISABLED);}
}
#include "dsr_ui_mobile_group_model.inc"
/* A second label per tile exhausted the fixed 128 KiB pool on a 24-card
 * dashboard. Draw the room row in the existing card's LVGL layer instead.
 * The stable index is owned by the same render lifecycle as its controls. */
typedef struct {int name_w,name_h,body,row_y,room_w,room_max,room_h,state_x,state_w,metric_x,metric_y,metric_w,height;} card_layout_t;
#define CARD_ROW_X 76
static const lv_font_t *card_name_font(void){return dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?&dsr_roboto_18:&dsr_roboto_16;}
/* Never read, out of date and no Wi-Fi are different facts, not one "offline". */
static bool card_warn(const dsr_mobile_device_t *d){return d->error!=DSR_OK||unbound(d)||(!d->online&&(d->has_sample_time||!model.wifi));}
/* The owner prefers the room instead of a routine movement state on cover
 * cards. Keep offline/error messages; STOP still represents an active move. */
static const char *card_cover_room(const dsr_mobile_device_t *d)
{return d->kind==DSR_DEVICE_COVER&&d->online&&d->error==DSR_OK&&!unbound(d)?(d->room<model.room_count?room_name(d):dsr_sc_tr("Nieprzypisane")):NULL;}
/* "Room • " in front of the state, cut to max pixels; returns its width. */
static int card_room_text(const dsr_mobile_device_t *d,char *text,size_t size,int max)
{
 const lv_font_t *font=dsr_sc_font(&dsr_roboto_14);lv_point_t s;
 snprintf(text,size,"%s • ",d->room<model.room_count?room_name(d):dsr_sc_tr("Nieprzypisane"));
 lv_text_get_size(&s,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 size_t length=strlen(text)-strlen(" • ");
 while(s.x>max&&length){
  do{length--;}while(length&&((unsigned char)text[length]&0xc0)==0x80);
  snprintf(text+length,size-length,"… • ");lv_text_get_size(&s,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 }
 return s.x;
}
static bool card_state_box(const dsr_mobile_device_t *d)
{return !trv_device(d)&&(d->kind==DSR_DEVICE_CONTACT||d->kind==DSR_DEVICE_SENSOR)&&(d->has_contact||(d->endpoint_kind==DSR_ENDPOINT_BLU&&(d->blu.flags&DSR_BLU_HAS_MOTION)))&&d->online;}
static bool card_contact_columns(const dsr_mobile_device_t *d)
{return card_state_box(d)&&d->has_contact&&d->error==DSR_OK&&!unbound(d)&&(d->endpoint_kind!=DSR_ENDPOINT_BLU||(cyber_blu_live(d)&&(d->blu.flags&DSR_BLU_HAS_OPEN)));}
static bool card_switch_rows(const dsr_mobile_device_t *d)
{return d->endpoint_kind==DSR_ENDPOINT_SWITCH&&(d->kind==DSR_DEVICE_RELAY||d->kind==DSR_DEVICE_PLUG||d->kind==DSR_DEVICE_LIGHT);}
static void card_switch_metric_center(lv_obj_t *pill)
{lv_obj_t *value=lv_obj_get_child(pill,0);lv_obj_set_height(value,LV_SIZE_CONTENT);lv_obj_align(value,LV_ALIGN_LEFT_MID,8,0);}
static void card_sensor_text(const dsr_mobile_device_t *d,char *text,size_t capacity)
{
 text[0]=0;
 if(d->has_temperature)snprintf(text,capacity,"%.1f °C",d->temperature_c);
 if(d->has_humidity)snprintf(text+strlen(text),capacity-strlen(text),"%s%.0f%% RH",text[0]?" • ":"",d->humidity_pct);
 if(d->endpoint_kind==DSR_ENDPOINT_BLU&&d->has_illuminance)snprintf(text+strlen(text),capacity-strlen(text),"%s%.0f lx",text[0]?" • ":"",d->illuminance_lux);
 if(d->has_battery)snprintf(text+strlen(text),capacity-strlen(text),"%s%s %.0f%%",text[0]?" • ":"",dsr_sc_tr("Bateria"),d->battery_pct);
}
/* One geometry for every skin: name, then a 52 px icon square with the
 * room/state rows and value pill beside it, and the device's single action
 * (power / up-down / state tile) on the right. Switch cards wrap the full room
 * above the state and values. A thermostat adds its setpoint
 * bar below; values too wide for the pill get their own full row. Contacts
 * stack the room, lx and battery between their two state graphics. Covers
 * use the room alone above position; this text can wrap beside the controls. */
static const char *not_online(const dsr_mobile_device_t *d);
static const char *card_state(const dsr_mobile_device_t *d,unsigned fields)
{
 const char *group=group_display_state(d);if(group)return group;
 bool cover=d->kind==DSR_DEVICE_COVER,sensor=d->kind==DSR_DEVICE_CONTACT||d->kind==DSR_DEVICE_SENSOR;
 if(cover&&d->online&&d->error==DSR_OK&&!unbound(d))return d->cover_state==DSR_COVER_OPENING?"Otwieranie":d->cover_state==DSR_COVER_CLOSING?"Zamykanie":d->cover_state==DSR_COVER_STOPPED?"Zatrzymano":"Roleta";
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&cyber_blu(d)&&cyber_contact(d)&&(!cyber_blu_live(d)||!(d->blu.flags&DSR_BLU_HAS_OPEN))&&d->error==DSR_OK&&!unbound(d))return "Brak danych";
 return unbound(d)?"Brak powiązania":d->error!=DSR_OK?device_error(d->error):!d->online?not_online(d):d->busy&&!(sensor&&d->endpoint_kind==DSR_ENDPOINT_BLU)?(d->command_pending?"Wysyłanie polecenia…":"Odczyt stanu…"):sensor?(d->has_contact&&((fields&64)||d->endpoint_kind==DSR_ENDPOINT_BLU)?(d->contact_open?"Otwarte":"Zamknięte"):(d->blu.flags&DSR_BLU_HAS_MOTION)&&d->endpoint_kind==DSR_ENDPOINT_BLU?((d->blu.flags&DSR_BLU_MOTION)?"Ruch wykryty":"Brak ruchu"):"Online"):cover?"Roleta":d->output?"Włączone":"Wyłączone";
}
static card_layout_t card_layout(const dsr_mobile_device_t *d,int width)
{
 bool trv=trv_device(d),box=card_state_box(d),cover=d->kind==DSR_DEVICE_COVER,sensor=d->kind==DSR_DEVICE_SENSOR||d->kind==DSR_DEVICE_CONTACT;
 card_layout_t l={.name_w=width-24};
 const lv_font_t *font=dsr_sc_font(card_name_font());int line=lv_font_get_line_height(font);
 lv_point_t s;lv_text_get_size(&s,dsr_sc_tr(dsr_sc_name(d->name)),font,0,0,l.name_w,LV_TEXT_FLAG_NONE);l.name_h=s.y>line?2*line:line;
 l.body=8+l.name_h+6;l.row_y=l.body+3;
 int right=trv||cover?104:(sensor&&!box)?0:box?84:58,row=width-CARD_ROW_X-12-(right?right+8:0);
 const char *cover_room=card_cover_room(d);
 if(cover){
  lv_text_get_size(&s,cover_room?cover_room:dsr_sc_tr(card_state(d,255)),dsr_sc_font(&dsr_roboto_14),0,0,row,LV_TEXT_FLAG_NONE);
  l.room_h=s.y;l.state_x=CARD_ROW_X;l.state_w=row;
  l.metric_x=CARD_ROW_X;l.metric_y=l.body+(s.y+11>29?s.y+11:29);l.metric_w=row;
  l.height=l.metric_y+38;if(l.height<l.body+68)l.height=l.body+68;return l;
 }
 if(card_contact_columns(d)){
  const char *room=d->room<model.room_count?room_name(d):dsr_sc_tr("Nieprzypisane");
  lv_text_get_size(&s,room,dsr_sc_font(&dsr_roboto_14),0,0,row,LV_TEXT_FLAG_NONE);
  l.room_w=row;l.room_h=s.y;l.metric_x=CARD_ROW_X;l.metric_y=l.body+l.room_h+6;l.metric_w=row;l.height=l.body+l.room_h+74;return l;
 }
 if(card_switch_rows(d)){
  const char *room=d->room<model.room_count?room_name(d):dsr_sc_tr("Nieprzypisane");
  lv_text_get_size(&s,room,dsr_sc_font(&dsr_roboto_14),0,0,row,LV_TEXT_FLAG_NONE);
  l.room_w=row;l.room_h=s.y;l.row_y=l.body+l.room_h+6;l.state_x=CARD_ROW_X;l.state_w=row;
  l.metric_x=CARD_ROW_X;l.metric_y=l.row_y+lv_font_get_line_height(dsr_sc_font(&dsr_roboto_14))+8;l.metric_w=row;
  char values[96];snprintf(values,sizeof(values),"%.1f W  |  %.1f V",d->power_w,d->voltage_v);
  lv_text_get_size(&s,values,dsr_sc_font(&dsr_roboto_12),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
  if(s.x+20>l.metric_w){l.metric_x=12;l.metric_w=width-24;}
  l.height=l.metric_y+38;return l;
 }
 char text[160];l.room_max=row*11/20;l.room_w=card_warn(d)?0:card_room_text(d,text,sizeof(text),l.room_max);
 /* A long state (waiting, sending) keeps the whole row; the room yields. */
 lv_text_get_size(&s,dsr_sc_tr(card_state(d,255)),dsr_sc_font(&dsr_roboto_14),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(l.room_w+s.x>row)l.room_w=0;
 l.state_x=CARD_ROW_X+l.room_w;l.state_w=row-l.room_w;
 l.metric_x=CARD_ROW_X;l.metric_y=l.body+29;l.metric_w=row;l.height=l.body+68;
 if(trv)l.height=l.body+98;
 else if(box){
  l.height=l.body+76;
  card_sensor_text(d,text,sizeof(text));lv_text_get_size(&s,text,dsr_sc_font(&dsr_roboto_12),0,0,l.metric_w,LV_TEXT_FLAG_NONE);
  if(text[0]&&l.height<l.metric_y+s.y+10)l.height=l.metric_y+s.y+10;
 }
 else if(!sensor&&!cover){
  snprintf(text,sizeof(text),"%.1f W  |  %.1f V",d->power_w,d->voltage_v);
  lv_text_get_size(&s,text,dsr_sc_font(&dsr_roboto_12),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
  if(s.x+20>l.metric_w){l.metric_x=12;l.metric_y=l.body+60;l.metric_w=width-24;l.height=l.metric_y+34;}
 }
 return l;
}
static void device_room_draw(lv_event_t *event)
{
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);
 const dsr_mobile_device_t *d=card_draw_device(i);if(!d)return;
 lv_obj_t *card=lv_event_get_target(event);
 card_layout_t l=card_layout(d,lv_obj_get_width(card));if(!l.room_w)return;
 bool rows=card_switch_rows(d);char text[96];if(rows)snprintf(text,sizeof(text),"%s",d->room<model.room_count?room_name(d):dsr_sc_tr("Nieprzypisane"));else card_room_text(d,text,sizeof(text),l.room_max);const lv_font_t *font=dsr_sc_font(&dsr_roboto_14);
 lv_area_t a;lv_obj_get_content_coords(card,&a);a.x1+=CARD_ROW_X;a.y1+=rows?l.body:l.row_y;a.x2=a.x1+l.room_w-(rows?1:0);a.y2=a.y1+(rows?l.room_h:lv_font_get_line_height(font))-1;
 lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);if(rows)label.base.obj=card;label.text=text;label.text_local=1;label.font=font;
 label.color=dsr_sc_text_color(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE?0x6fd3ff:DSR_SC_MUTED);
 lv_draw_label(lv_event_get_layer(event),&label,&a);
}
static void contact_metric_draw(lv_obj_t *card,lv_layer_t *layer,dsr_ui_icon_t icon,const char *text,int x,int y,int width,uint32_t color)
{
 const lv_font_t *font=dsr_sc_font(&dsr_roboto_14);lv_point_t size;lv_text_get_size(&size,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 if(size.x+44>width){font=dsr_sc_font(&dsr_roboto_12);lv_text_get_size(&size,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);}
 int w=size.x+44;lv_area_t frame={x+(width-w)/2,y,x+(width-w)/2+w-1,y+23};
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_draw_contour(layer,&frame,DSR_CYBER_CYAN,LV_OPA_80);
 else{lv_draw_rect_dsc_t border;lv_draw_rect_dsc_init(&border);border.base.obj=card;border.bg_opa=LV_OPA_TRANSP;border.border_width=1;border.border_color=dsr_sc_color(DSR_SC_GLOW);border.radius=8;lv_draw_rect(layer,&border,&frame);}
 lv_area_t a={frame.x1+6,y,frame.x1+29,y+23};dsr_sc_draw_icon(card,layer,icon,&a,color);
 int h=lv_font_get_line_height(font);a=(lv_area_t){frame.x1+38,y+(24-h)/2,frame.x2-6,y+(24-h)/2+h-1};
 lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.base.obj=card;label.text=text;label.text_local=1;label.font=font;label.color=dsr_sc_text_color(color);lv_draw_label(layer,&label,&a);
}
static void card_contact_draw(lv_event_t *event)
{
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(i>=model.device_count)return;
 const dsr_mobile_device_t *d=&model.devices[i];lv_obj_t *card=lv_event_get_target(event);card_layout_t l=card_layout(d,lv_obj_get_width(card));
 lv_layer_t *layer=lv_event_get_layer(event);lv_area_t a;lv_obj_get_content_coords(card,&a);int x=a.x1+CARD_ROW_X,y=a.y1+l.body;
 a=(lv_area_t){x,y,x+l.room_w-1,y+l.room_h-1};lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);
 label.base.obj=card;label.text=d->room<model.room_count?room_name(d):dsr_sc_tr("Nieprzypisane");label.text_local=1;label.font=dsr_sc_font(&dsr_roboto_14);label.align=LV_TEXT_ALIGN_CENTER;label.color=dsr_sc_text_color(DSR_SC_MUTED);lv_draw_label(layer,&label,&a);
 char text[40];bool lux=d->has_illuminance&&isfinite(d->illuminance_lux)&&d->illuminance_lux>=0;
 if(lux)snprintf(text,sizeof(text),d->illuminance_lux<1e6?"%.0f lx":"%.3g lx",d->illuminance_lux);else snprintf(text,sizeof(text),"— lx");
 contact_metric_draw(card,layer,DSR_ICON_ILLUMINANCE,text,x,y+l.room_h+6,l.metric_w,lux?DSR_SC_GLOW:DSR_SC_OFF);
 bool battery=d->has_battery&&isfinite(d->battery_pct)&&d->battery_pct>=0&&d->battery_pct<=100;
 if(battery)snprintf(text,sizeof(text),"%.0f%%",d->battery_pct);else snprintf(text,sizeof(text),"—");
 contact_metric_draw(card,layer,DSR_ICON_BATTERY,text,x,y+l.room_h+36,l.metric_w,battery?(d->battery_pct<=20?0xf0a020:DSR_SC_GLOW):DSR_SC_OFF);
}
/* Draw readouts in their existing card/frame rather than allocating three
 * more objects per TRV. The fixed 128 KiB pool must also fit a full dashboard. */
static void card_battery_draw(lv_event_t *event)
{
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);const dsr_mobile_device_t *d=card_draw_device(i);if(!d)return;
 lv_obj_t *card=lv_event_get_target(event);card_layout_t l=card_layout(d,lv_obj_get_width(card));
 bool known=d->online&&d->error==DSR_OK&&d->has_battery&&isfinite(d->battery_pct)&&d->battery_pct>=0&&d->battery_pct<=100;
 uint32_t color=known?(d->battery_pct<=20?0xf0a020:DSR_SC_GLOW):DSR_SC_OFF;
 char text[16];if(known)snprintf(text,sizeof(text),"%.0f%%",d->battery_pct);else snprintf(text,sizeof(text),"—");
 const lv_font_t *font=dsr_sc_font(&dsr_roboto_14);int h=lv_font_get_line_height(font);lv_point_t size;
 lv_text_get_size(&size,text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 lv_area_t a;lv_obj_get_content_coords(card,&a);
 a.x1+=CARD_ROW_X;a.y1+=l.body+60;a.x2=a.x1+23;a.y2=a.y1+23;
 lv_area_t frame={a.x1-6,a.y1,a.x1+32+size.x+5,a.y2};lv_layer_t *layer=lv_event_get_layer(event);
 lv_obj_t *reading=lv_obj_get_child(card,-1);
 if(reading&&lv_obj_get_x(reading)==l.metric_x&&lv_obj_get_y(reading)==l.metric_y){
  lv_area_t value;lv_obj_get_coords(reading,&value);int shift=(value.x1+value.x2-frame.x1-frame.x2)/2;
  a.x1+=shift;a.x2+=shift;frame.x1+=shift;frame.x2+=shift;
 }
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE)dsr_ui_cyber_draw_contour(layer,&frame,DSR_CYBER_CYAN,LV_OPA_80);
 else{lv_draw_rect_dsc_t border;lv_draw_rect_dsc_init(&border);border.base.obj=card;border.bg_opa=LV_OPA_TRANSP;border.border_width=1;border.border_color=dsr_sc_color(DSR_SC_GLOW);border.radius=8;lv_draw_rect(layer,&border,&frame);}
 dsr_sc_draw_icon(card,lv_event_get_layer(event),DSR_ICON_BATTERY,&a,color);
 a.x1+=32;a.x2=frame.x2-6;a.y1+=(24-h)/2;a.y2=a.y1+h-1;
 lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.base.obj=card;label.text=text;label.text_local=1;label.font=font;label.color=dsr_sc_text_color(color);
 lv_draw_label(lv_event_get_layer(event),&label,&a);
}
static void card_target_draw(lv_event_t *event)
{
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);const dsr_mobile_device_t *d=card_draw_device(i);if(!d)return;char value[20],text[48];
 if(trv_known(d)||trv_slot(d->id)>=0){uint8_t h=trv_shown(d);snprintf(value,sizeof(value),"%u.%u °C",(unsigned)h/2u,(unsigned)(h%2u)*5u);}else snprintf(value,sizeof(value),"— °C");
 snprintf(text,sizeof(text),"%s %s",dsr_sc_tr("Nastawa"),value);
 lv_obj_t *frame=lv_event_get_target(event);lv_area_t a;lv_obj_get_content_coords(frame,&a);
 const lv_font_t *font=dsr_sc_font(&dsr_roboto_12);int h=lv_font_get_line_height(font);a.x1+=4;a.x2-=4;a.y1+=(lv_area_get_height(&a)-h)/2;a.y2=a.y1+h-1;
 lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.base.obj=frame;label.text=text;label.text_local=1;label.font=font;label.align=LV_TEXT_ALIGN_CENTER;label.color=dsr_sc_text_color(DSR_SC_TEXT);
 lv_draw_label(lv_event_get_layer(event),&label,&a);
}
/* While a cover moves, or a movement/STOP is being sent, STOP is the largest
 * and only red control on the card and the detail page. Reverse actions stay
 * reachable (and refused while a command is in flight). */
static bool cover_moving(const dsr_mobile_device_t *d)
{int g=group_display_index(d);return d->kind==DSR_DEVICE_COVER&&d->online&&d->error==DSR_OK&&((g>=0&&group_state((unsigned)g).cover_motion)||d->cover_state==DSR_COVER_OPENING||d->cover_state==DSR_COVER_CLOSING||d->command_pending);}
#define DSR_STOP_RED 0xc62828
static lv_obj_t *stop_button(lv_obj_t *parent,int x,int y,int w,int h,bool valid,lv_event_cb_t cb,void *context)
{
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 lv_obj_t *b=dsr_sc_button(parent,x,y,w,h,DSR_STOP_RED,hud?10:12,cb,context);
 lv_obj_set_style_bg_color(b,lv_color_hex(0x8e1b1b),LV_STATE_PRESSED);
 if(hud)dsr_sc_border_color(b,lv_color_hex(0xff8a80),0);
 const lv_font_t *font=h>=56?&dsr_roboto_18:&dsr_roboto_16;int glyph=h>=56?16:14;
 lv_point_t size;lv_text_get_size(&size,dsr_sc_tr("Stop"),dsr_sc_font(font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 int content=glyph+8+size.x,left=(w-content)/2;if(left<4)left=4;
 dsr_sc_panel(b,left,(h-glyph)/2,glyph,glyph,valid?0xffffff:0xb7bcc2,2);
 lv_obj_t *label=dsr_sc_text(b,"Stop",left+glyph+8,(h-lv_font_get_line_height(dsr_sc_font(font)))/2,w-left-glyph-12,font,valid?0xffffff:0xb7bcc2);lv_label_set_long_mode(label,LV_LABEL_LONG_MODE_DOTS);
 if(!valid)lv_obj_add_state(b,LV_STATE_DISABLED);
 return b;
}
/* Not online without an error: never read, out of date, or no Wi-Fi. */
static const char *not_online(const dsr_mobile_device_t *d)
{return !model.wifi?"Brak Wi-Fi":!d->has_sample_time?"Oczekiwanie na odczyt…":"Odczyt nieaktualny";}
static void card_stop(lv_event_t *e)
{
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(e);
 if(i>=model.device_count)return;
 dsr_mobile_device_t *d=&model.devices[i];
 if(!d->can_stop_cover)return;
 if(model.demo){d->cover_state=DSR_COVER_STOPPED;d->command_pending=false;render();}
 else if(actions.stop_cover)actions.stop_cover(i);
}
static int device_card_height(const dsr_mobile_device_t *d,bool tall)
{return card_layout(d,296).height+(tall?24:0);}
static lv_obj_t *device_card_shared(lv_obj_t *parent,unsigned i,const dsr_mobile_device_t *d,bool group,int x,int y,int width,int height,unsigned fields)
{
    /* Name, square device icon, type-specific room/state rows, value pill
     * and the device's one action on the right. */
    uintptr_t drawing=group?GROUP_DRAW_CONTEXT|i:i;
    bool cover=d->kind==DSR_DEVICE_COVER,sensor=d->kind==DSR_DEVICE_CONTACT||d->kind==DSR_DEVICE_SENSOR,trv=trv_device(d);
    bool on=!sensor&&!cover&&d->online&&d->output&&d->error==DSR_OK&&!unbound(d);
    lv_obj_t *card=dsr_sc_button(parent,x,y,width,height,0x2b2f33,DSR_SC_RADIUS,group?group_open:open_device,(void *)(uintptr_t)i);
    bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
    if(hud)dsr_ui_cyber_device(card,on,d->error!=DSR_OK||unbound(d)||!d->online);else{edge(card,on?DSR_SC_GLOW:0x3a3f44,on?2:1);dsr_sc_glow(card,on);}
    const char *cover_room=card_cover_room(d),*state=cover_room?cover_room:card_state(d,fields);bool warn=card_warn(d);
    /* The measurement frame already reports stale readings. Keep distinct
     * connection/authentication warnings, and the state when no frame is shown. */
    if(fields&&!DSR_ENDPOINT_IS_SENSOR(d->endpoint_kind)&&!unbound(d)&&!d->online&&d->error==DSR_OK&&model.wifi&&d->has_sample_time)state="";
    card_layout_t layout=card_layout(d,width);int body=layout.body;
    bool state_box=card_state_box(d);
    lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(d->name),12,8,layout.name_w,card_name_font(),DSR_SC_TEXT);truncate(name,layout.name_h);
    bool columns=card_contact_columns(d);int group_h=columns?layout.room_h+60:52;
    lv_obj_t *thumb=dsr_sc_panel(card,12,body+(group_h-52)/2,52,52,0x17191c,10);
    if(hud)dsr_ui_cyber_inset(thumb);
    bool contact=d->endpoint_kind==DSR_ENDPOINT_BLU&&cyber_contact(d);
    dsr_sc_icon_centered(thumb,contact?cyber_contact_icon(d):d->has_contact?contact_icon(d,d->contact_open):device_icon(d),32,contact?cyber_contact_color(d):d->online?(on?DSR_SC_GLOW:dsr_sc_icon_accent()):DSR_SC_OFF);
    if(columns){
     lv_obj_t *box=dsr_sc_panel(card,width-96,body+(group_h-64)/2,84,64,0x1d2124,10);edge(box,d->contact_open?0xf0a020:DSR_SC_GLOW,1);
     dsr_sc_icon_centered(box,contact?cyber_contact_icon(d):contact_icon(d,d->contact_open),52,contact?cyber_contact_color(d):d->contact_open?0xf0a020:DSR_SC_GLOW);
     lv_obj_add_event_cb(card,card_contact_draw,LV_EVENT_DRAW_MAIN_END,(void *)(uintptr_t)i);return card;
    }
    const lv_font_t *state_font=&dsr_roboto_14;lv_point_t state_size;lv_text_get_size(&state_size,cover_room?cover_room:dsr_sc_tr(state),dsr_sc_font(state_font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(!cover&&state_size.x>layout.state_w)state_font=&dsr_roboto_12;
    if(state_font==&dsr_roboto_12){lv_text_get_size(&state_size,dsr_sc_tr(state),dsr_sc_font(state_font),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(state_size.x>layout.state_w)state_font=&dsr_roboto_11;}
    lv_obj_t *status=dsr_sc_text(card,cover_room?dsr_sc_user(cover_room):state,layout.state_x,layout.row_y+(state_font==&dsr_roboto_14?0:state_font==&dsr_roboto_12?1:2),layout.state_w,state_font,warn?0xffc18c:on?(hud?DSR_SC_GLOW:DSR_SC_BLUE):hud?0x6fd3ff:DSR_SC_MUTED);
    if(cover)lv_obj_set_height(status,layout.room_h);else truncate(status,lv_font_get_line_height(dsr_sc_font(&dsr_roboto_14)));
    lv_obj_add_event_cb(card,device_room_draw,LV_EVENT_DRAW_MAIN_END,(void *)drawing);
    if(cover){
     /* Framed pair of round up/down actions; STOP takes the whole frame while moving. */
     bool valid=group?group_ready(group_state(i)):d->online&&d->error==DSR_OK&&!d->busy&&d->can_cover&&(model.demo||actions.set_cover);
     lv_obj_t *pill=dsr_sc_panel(card,width-116,body-2,104,56,0x1d2124,12);
     if(cover_moving(d)){int border=lv_obj_get_style_border_width(pill,0);stop_button(pill,4,4,96-2*border,48-2*border,d->can_stop_cover&&(group?actions.set_group!=NULL:model.demo||actions.stop_cover),group?group_stop:card_stop,(void *)(uintptr_t)i);}
     else{
      edge(pill,DSR_SC_GLOW,1);int border=lv_obj_get_style_border_width(pill,0),inner=104-2*border,left=(inner-96)/2,top=(56-2*border-44)/2;
      dsr_sc_panel(pill,left+47,top+6,1,32,hud?DSR_SC_GLOW:DSR_SC_EDGE,0);
      for(unsigned k=0;k<2;k++){
       lv_obj_t *b=dsr_sc_button(pill,left+(int)k*52,top,44,44,hud?DSR_SC_BLUE:valid?0xeceff1:0x5a6066,LV_RADIUS_CIRCLE,group?group_cover_pair:cover_control,(void *)(uintptr_t)(i*2+(k?0:1)));
       if(hud)dsr_ui_cyber_power(b,true,valid);
       dsr_sc_icon_centered(b,k?DSR_ICON_DOWN:DSR_ICON_UP,24,valid?DSR_SC_GLOW:0x8a9096);if(!valid)lv_obj_add_state(b,LV_STATE_DISABLED);
      }
     }
    }else if(trv){
     /* Same 104x56 frame and 44 px controls as the cover pair. */
     lv_obj_t *bar=dsr_sc_panel(card,width-116,body-2,104,56,0x1d2124,12);
     edge(bar,DSR_SC_GLOW,1);bool valid=group?group_ready(group_state(i)):trv_can(d);int border=lv_obj_get_style_border_width(bar,0),inner=104-2*border,left=(inner-96)/2,top=(56-2*border-44)/2;
     if(group){trv_round_action(bar,i,left,top,44,false,valid,group_target);trv_round_action(bar,i,left+52,top,44,true,valid,group_target);}else{trv_round(bar,i,left,top,44,false,valid);trv_round(bar,i,left+52,top,44,true,valid);}
     lv_obj_t *setpoint=dsr_sc_panel(card,width-116,body+60,104,24,0x1d2124,8);edge(setpoint,DSR_SC_GLOW,1);
     lv_obj_add_event_cb(setpoint,card_target_draw,LV_EVENT_DRAW_MAIN_END,(void *)drawing);
     lv_obj_add_event_cb(card,card_battery_draw,LV_EVENT_DRAW_MAIN_END,(void *)drawing);
    }else if(state_box){
     /* A large state graphic; the room row already carries its text. */
     const char *what=hud&&cyber_blu(d)&&cyber_contact(d)&&(!cyber_blu_live(d)||!(d->blu.flags&DSR_BLU_HAS_OPEN))?"Brak danych":d->has_contact?(d->contact_open?"Otwarte":"Zamknięte"):(d->blu.flags&DSR_BLU_MOTION)?"Ruch":"Brak ruchu";
     bool alert=d->has_contact?d->contact_open:(d->blu.flags&DSR_BLU_MOTION)!=0;
     lv_obj_t *box=dsr_sc_panel(card,width-96,body-2,84,64,0x1d2124,10);
     edge(box,alert?0xf0a020:DSR_SC_GLOW,1);
     if(d->has_contact||contact){
      dsr_ui_icon_t graphic=contact?cyber_contact_icon(d):contact_icon(d,d->contact_open);
      dsr_sc_icon_centered(box,graphic,52,contact?cyber_contact_color(d):alert?0xf0a020:DSR_SC_GLOW);
     }else{
      const lv_font_t *font=&dsr_roboto_14;lv_obj_t *label=dsr_sc_text(box,what,2,0,76,font,alert&&hud?0xffc65b:DSR_SC_TEXT);center(label);lv_obj_center(label);
     }
    }else if(!sensor){
     bool valid=d->online&&!d->busy&&d->can_switch;
     dsr_sc_power_button(card,width-70,body-3,58,on,valid,DSR_ICON_POWER,DSR_SC_GLOW,group?group_power:control,(void *)(uintptr_t)i);
    }
    if(!fields)return card;
    char part[6][40];const char *texts[6];unsigned n=0;
#define ADD_PART(...) do{if(n<6){snprintf(part[n],sizeof(part[n]),__VA_ARGS__);texts[n]=part[n];n++;}}while(0)
    if((!d->online||d->error!=DSR_OK)&&!DSR_ENDPOINT_IS_SENSOR(d->endpoint_kind))ADD_PART("%s",dsr_sc_tr("Odczyty niedostępne"));
    else if(cover){if((fields&32)&&d->has_position)ADD_PART("%s %.0f%%",dsr_sc_tr("Pozycja"),d->position_pct);else ADD_PART("%s —",dsr_sc_tr("Pozycja"));
      if((fields&1)&&d->has_power)ADD_PART("%.1f W",d->power_w);}
    else if(sensor){
      if(trv){if(d->has_temperature)ADD_PART("%.1f °C",d->temperature_c);else ADD_PART("— °C");}
      else{
       if((fields&8)&&d->has_temperature)ADD_PART("%.1f °C",d->temperature_c);
       if((fields&16)&&d->has_humidity)ADD_PART("%.0f%% RH",d->humidity_pct);
       if(d->endpoint_kind==DSR_ENDPOINT_BLU&&!trv&&d->has_illuminance)ADD_PART("%.0f lx",d->illuminance_lux);
      }
      if(!trv&&(fields&4)&&d->has_battery)ADD_PART("%s %.0f%%",dsr_sc_tr("Bateria"),d->battery_pct);
    }else{
      if(fields&1){if(d->has_power)ADD_PART("%.1f W",d->power_w);else ADD_PART("— W");}
      if(fields&2){if(d->has_voltage)ADD_PART("%.1f V",d->voltage_v);else ADD_PART("— V");}
    }
#undef ADD_PART
    if(n){if(state_box){char text[240]="";for(unsigned k=0;k<n;k++){if(k)strncat(text," • ",sizeof(text)-strlen(text)-1);strncat(text,texts[k],sizeof(text)-strlen(text)-1);}dsr_sc_text(card,dsr_sc_user(text),layout.metric_x,layout.metric_y,layout.metric_w,&dsr_roboto_12,DSR_SC_TEXT);}
     else{
      dsr_sc_segments(card,texts,n,layout.metric_x,layout.metric_y,layout.metric_w,d->online?DSR_SC_TEXT:0xffc18c);
      if(card_switch_rows(d))card_switch_metric_center(lv_obj_get_child(card,-1));
      if(trv){lv_obj_t *reading=lv_obj_get_child(lv_obj_get_child(card,-1),0);lv_obj_set_height(reading,LV_SIZE_CONTENT);center(reading);lv_obj_center(reading);}
     }}
    return card;
}
static lv_obj_t *device_card(lv_obj_t *parent,unsigned i,int x,int y,int width,int height,unsigned fields)
{return device_card_shared(parent,i,&model.devices[i],false,x,y,width,height,fields);}
bool dsr_ui_photo_source(const char *id,lv_image_dsc_t *out)
{
    const void *pixels=id&&id[0]?dsr_photo_pixels(id):NULL;unsigned w=DSR_PHOTO_WIDTH,h=DSR_PHOTO_HEIGHT;
    if(!pixels){const dsr_room_photo_t *builtin=dsr_room_photo_find(id);if(!builtin)return false;pixels=builtin->pixels;w=h=DSR_ROOM_PHOTO_SIZE;}
    *out=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=w,.h=h,.stride=w*2},.data_size=w*h*2,.data=pixels};return true;
}
static dsr_ui_icon_t room_icon(unsigned i)
{
    const dsr_ui_icon_t kinds[]={DSR_ICON_SOFA,DSR_ICON_KITCHEN,DSR_ICON_BED,DSR_ICON_BATH};
    return kinds[model.rooms[i].icon%4];
}
static lv_obj_t *room_card(lv_obj_t *parent,unsigned i,int x,int y,int width,int height)
{
    /* Cyber: one slim chamfered contour; the photo has no frame of its own
     * (owner 2026-10: the nested double borders were too heavy). */
    bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;int inset=hud?3:0;
    lv_obj_t *card=dsr_sc_button(parent,x,y,width,height,DSR_SC_CARD,DSR_SC_RADIUS,open_room,(void *)(uintptr_t)i);
    if(hud)dsr_ui_cyber_frame(card,false);
    static lv_image_dsc_t photos[DSR_UI_MAX_ROOMS];
    if(dsr_ui_photo_source(model.rooms[i].photo,&photos[i])){
        int w=width-2*inset,h=height-90-inset;
        lv_obj_t *window=dsr_sc_panel(card,inset,inset,w,h,DSR_SC_CARD,hud?5:DSR_SC_RADIUS);lv_obj_set_style_clip_corner(window,true,0);lv_obj_t *image=lv_image_create(window);lv_image_set_src(image,&photos[i]);
        uint32_t scale=(uint32_t)w*256/photos[i].header.w,vertical=(uint32_t)h*256/photos[i].header.h;if(vertical>scale)scale=vertical;
        lv_image_set_scale(image,scale);lv_image_set_antialias(image,true);lv_obj_center(image);
    }else{lv_obj_t *thumb=dsr_sc_panel(card,(width-44)/2,54,44,44,0x303943,LV_RADIUS_CIRCLE);dsr_sc_icon_centered(thumb,room_icon(i),28,hud?DSR_SC_GLOW:0xe8edf2);}
    lv_obj_t *tag=dsr_sc_panel(card,inset+6,inset+6,hud?66:76,hud?20:22,0x25292b,hud?5:7);lv_obj_set_style_bg_opa(tag,220,0);dsr_sc_icon(tag,room_icon(i),hud?5:6,hud?2:3,16,dsr_sc_icon_accent());dsr_sc_text(tag,"Pokój",hud?25:30,hud?3:4,40,&dsr_roboto_12,DSR_SC_TEXT);
    const char *room=dsr_sc_name(model.rooms[i].name);const lv_font_t *font=&dsr_roboto_14;lv_point_t size;
    lv_text_get_size(&size,dsr_sc_tr(room),dsr_sc_font(font),0,0,width-16,LV_TEXT_FLAG_NONE);if(size.y>46)font=&dsr_roboto_12;
    lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(model.rooms[i].name),8,height-82,width-16,font,DSR_SC_TEXT);center(name);lv_label_set_long_mode(name,LV_LABEL_LONG_MODE_WRAP);lv_obj_set_height(name,46);
    double power=0;unsigned count=0;bool complete=true;
    for(unsigned d=0;d<model.device_count;d++)if(model.devices[d].room==i){count++;complete&=model.devices[d].online&&model.devices[d].has_power;power+=model.devices[d].power_w;}
    lv_obj_t *badge=dsr_sc_panel(card,(width-82)/2,height-32,82,26,0x262a2d,10);
    char text[32];if(count&&complete)snprintf(text,sizeof(text),"%.1f W",power);else snprintf(text,sizeof(text),"%u %s",count,dsr_sc_tr("urz."));
    dsr_sc_icon(badge,DSR_ICON_ENERGY,5,5,16,dsr_sc_icon_accent());center(dsr_sc_text(badge,text,23,6,55,&dsr_roboto_12,DSR_SC_TEXT));
    if(page==DSR_MOBILE_ROOMS){
        /* A 34 px pencil keeps its 44 px touch area (extended click margin). */
        lv_obj_t *edit=hud?dsr_sc_button(card,width-inset-40,inset+6,34,34,0x25292b,8,edit_room,(void *)(uintptr_t)model.rooms[i].id):icon_button(card,DSR_ICON_EDIT,width-52,6,44,0x25292b,edit_room,(void *)(uintptr_t)model.rooms[i].id);
        if(hud){lv_obj_set_style_bg_opa(edit,210,0);dsr_sc_icon_centered(edit,DSR_ICON_EDIT,20,DSR_SC_GLOW);}
        if(model.demo||!model.rooms[i].id||!actions.edit_room)lv_obj_add_state(edit,LV_STATE_DISABLED);
    }
    return card;
}
static void scene_run(lv_event_t *event)
{unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(!model.demo&&i<model.scene_count&&actions.run_scene)actions.run_scene(model.scenes[i].id);}
static void action_edit(lv_event_t *event)
{unsigned value=(unsigned)(uintptr_t)lv_event_get_user_data(event),index=value/2;bool scene=(value&1)!=0;if(model.demo||!actions.edit_action)return;if(scene&&index<model.scene_count)actions.edit_action(1,model.scenes[index].id);else if(!scene&&index<model.group_count)actions.edit_action(0,model.groups[index].id);}
static void scene_card(unsigned index,int y,int height)
{
 dsr_mobile_scene_t *scene=&model.scenes[index];lv_obj_t *card=dsr_sc_button(content,0,y,296,height,0x414548,12,navigate,(void *)(uintptr_t)19);
 lv_obj_t *icon=dsr_sc_panel(card,8,17,50,50,0x29343b,LV_RADIUS_CIRCLE);dsr_sc_icon_centered(icon,DSR_ICON_SCENES,32,dsr_sc_icon_accent());
 lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(scene->name),68,10,164,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
 char text[80];snprintf(text,sizeof(text),dsr_sc_tr("Scena • %u działań"),scene->actions);dsr_sc_text(card,text,68,36,210,&dsr_roboto_12,DSR_SC_MUTED);
 card_control(card,0,DSR_ICON_EDIT,244,26,actions.edit_action!=NULL,action_edit,index*2+1);
 lv_obj_t *run=dsr_sc_button(card,68,height-36,164,30,DSR_SC_PANEL,8,scene_run,(void *)(uintptr_t)index);dsr_sc_text(run,"Uruchom scenę",4,7,156,&dsr_roboto_12,DSR_SC_TEXT);if(model.demo||!actions.run_scene||!scene->actions)lv_obj_add_state(run,LV_STATE_DISABLED);
}
#include "dsr_ui_mobile_groups.inc"
static void empty(const char *heading,const char *text)
{
    dsr_sc_icon(content,DSR_ICON_HOME,118,28,56,DSR_SC_MUTED);
    lv_obj_t *h=dsr_sc_text(content,heading,10,109,280,&dsr_roboto_18,DSR_SC_TEXT);center(h);
    lv_obj_t *s=dsr_sc_text(content,text,19,148,262,&dsr_roboto_14,DSR_SC_MUTED);center(s);
}
static void metric(lv_obj_t *root,const char *label,const char *value,int x)
{
    lv_obj_t *box=dsr_sc_panel(root,x,355,144,62,DSR_SC_CARD,9);
    center(dsr_sc_text(box,label,4,9,136,&dsr_roboto_12,DSR_SC_MUTED));
    center(dsr_sc_text(box,value,4,31,136,&dsr_roboto_18,DSR_SC_TEXT));
}
static bool cover_position(const dsr_mobile_device_t *d)
{return d->online&&d->error==DSR_OK&&d->has_position&&isfinite(d->position_pct)&&d->position_pct>=0&&d->position_pct<=100;}
static const char *cover_fault(uint8_t errors)
{const char *names[]={"Przegrzanie","Przekroczona moc","Zbyt wysokie napięcie","Zbyt niskie napięcie","Przekroczony prąd","Wykryto przeszkodę","Wejście bezpieczeństwa","Błąd rolety"};for(unsigned i=0;i<8;i++)if(errors&(1u<<i))return names[i];return "Błąd rolety";}
static void cover_stop(lv_event_t *event)
{
 if(page==DSR_MOBILE_GROUP_CONTROL){if(group_cover_selected())group_send(selected,(dsr_group_intent_t){.command=DSR_ACTION_STOP});return;}
 (void)event;dsr_mobile_device_t *d=&model.devices[selected];
 if(!d->can_stop_cover)return;
 if(model.demo){d->cover_state=DSR_COVER_STOPPED;render();}
 else if(actions.stop_cover)actions.stop_cover(selected);
}
/* Reverse action beside the central STOP while moving: icon above its label. */
static lv_obj_t *cover_compact(lv_obj_t *parent,int x,int y,int w,int h,unsigned i,bool valid)
{
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE,tall=h>=56;int size=tall?28:20;
 lv_obj_t *b=dsr_sc_button(parent,x,y,w,h,hud?DSR_SC_BLUE:DSR_SC_CARD,hud?10:14,group_cover_selected()?group_cover_pair:cover_control,(void *)(uintptr_t)(selected*2+(i==0)));
 if(hud)dsr_ui_cyber_panel(b,true);
 dsr_sc_icon(b,hud?(i?DSR_ICON_COVER_CLOSE:DSR_ICON_COVER_OPEN):(i?DSR_ICON_DOWN:DSR_ICON_UP),(w-size)/2,tall?7:3,size,valid?(hud?DSR_SC_GLOW:DSR_SC_BLUE):DSR_SC_OFF);
 lv_obj_t *label=dsr_sc_text(b,i?"Zamknij":"Otwórz",2,tall?38:25,w-4,tall?&dsr_roboto_14:&dsr_roboto_12,valid?DSR_SC_TEXT:DSR_SC_OFF);center(label);truncate(label,tall?18:16);
 if(!valid)lv_obj_add_state(b,LV_STATE_DISABLED);
 return b;
}
static bool position_enabled(const dsr_mobile_device_t *d)
{if(group_display_index(d)>=0){group_state_t s=group_state(selected);return group_ready(s)&&s.family==DSR_ENDPOINT_COVER&&s.known&&d->can_position;}return !cover_preview&&d->online&&d->error==DSR_OK&&!d->busy&&d->has_position&&d->can_position&&actions.set_position;}
static void cover_drag_event(lv_event_t *event)
{
 lv_event_code_t code=lv_event_get_code(event);lv_obj_t *rail=lv_event_get_target(event);
 dsr_mobile_device_t *d=detail_device();if(!d)return;
 if(code==LV_EVENT_PRESS_LOST){cover_drag=false;if(cover_preview_label)lv_label_set_text(cover_preview_label,"");if(cover_visual)dsr_ui_cyber_cover_restore(cover_visual,(int)lround(d->position_pct));return;}
 if(!position_enabled(d)){cover_drag=false;if(cover_preview_label)lv_label_set_text(cover_preview_label,"");if(cover_visual)dsr_ui_cyber_cover_restore(cover_visual,(int)lround(d->position_pct));return;}
 lv_indev_t *input=lv_indev_active();if(!input)return;lv_point_t point;lv_indev_get_point(input,&point);lv_area_t area;lv_obj_get_coords(rail,&area);
 bool inside=point.x>=area.x1&&point.x<=area.x2&&point.y>=area.y1&&point.y<=area.y2;
 uint32_t id=group_cover_selected()?model.groups[selected].id:d->id;
 if(code==LV_EVENT_PRESSED){if(!inside)return;cover_drag=true;cover_drag_group=group_cover_selected();cover_drag_id=id;}
 if(!cover_drag||cover_drag_group!=group_cover_selected()||cover_drag_id!=id)return;
 int value;
 if(cover_visual)value=dsr_ui_cyber_cover_target(cover_visual,point.y);
 else value=(int)lround((area.y1+138-point.y)*100.0/130.0);
 if(value<0)value=0;
 if(value>100)value=100;
 cover_preview_position=(uint8_t)value;
 if(cover_visual)dsr_ui_cyber_cover_preview(cover_visual,value,(int)lround(d->position_pct));
 if(cover_preview_label){char text[48];snprintf(text,sizeof(text),"%s %u%%",dsr_sc_tr("Podgląd"),cover_preview_position);lv_label_set_text(cover_preview_label,text);}
 if(code==LV_EVENT_RELEASED){cover_drag=false;uint8_t target=cover_preview_position;if(cover_preview_label)lv_label_set_text(cover_preview_label,"");if(cover_visual)dsr_ui_cyber_cover_restore(cover_visual,(int)lround(d->position_pct));if(inside){if(group_cover_selected())group_send(selected,(dsr_group_intent_t){.command=DSR_ACTION_POSITION,.value={target}});else if(actions.set_position)actions.set_position(selected,target);}}
}
#include "dsr_ui_mobile_cover_cyber.inc"
static void cover_details(lv_obj_t *root)
{shared_cover_details(root,&model.devices[selected]);}
static bool lighting_selected(void)
{return page==DSR_MOBILE_GROUP_CONTROL?selected<model.group_count:selected<model.device_count;}
static dsr_mobile_device_t *lighting_device(void)
{return page==DSR_MOBILE_GROUP_CONTROL?group_model(selected):&model.devices[selected];}
static void lighting_send_brightness(uint8_t value)
{if(page==DSR_MOBILE_GROUP_CONTROL)group_send(selected,(dsr_group_intent_t){.command=DSR_ACTION_BRIGHTNESS,.value={value}});else if(actions.set_brightness)actions.set_brightness(selected,value);}
static void lighting_send_rgb(uint8_t r,uint8_t g,uint8_t b)
{if(page==DSR_MOBILE_GROUP_CONTROL)group_send(selected,(dsr_group_intent_t){.command=DSR_ACTION_RGB,.value={r,g,b}});else if(actions.set_rgb)actions.set_rgb(selected,r,g,b);}
static bool light_enabled(const dsr_mobile_device_t *d)
{return !model.demo&&(d->endpoint_kind==DSR_ENDPOINT_LIGHT||d->endpoint_kind==DSR_ENDPOINT_RGB)&&d->online&&d->error==DSR_OK&&!d->busy&&d->can_light&&(page==DSR_MOBILE_GROUP_CONTROL?actions.set_group!=NULL:actions.set_brightness!=NULL);}
static uint8_t brightness(const dsr_mobile_device_t *d)
{return d->endpoint_kind==DSR_ENDPOINT_RGB?d->rgb.brightness:d->light.brightness;}
static void light_drag_event(lv_event_t *event)
{
 if(!lighting_selected())return;
 dsr_mobile_device_t *d=lighting_device();lv_event_code_t code=lv_event_get_code(event);
 if(code==LV_EVENT_PRESS_LOST||!light_enabled(d)){light_drag=false;if(light_slider)lv_slider_set_value(light_slider,brightness(d),LV_ANIM_OFF);return;}
 if(code==LV_EVENT_PRESSED){light_drag=true;light_drag_id=d->id;}
 if(!light_drag||light_drag_id!=d->id)return;
 int value=lv_slider_get_value(lv_event_get_target(event));if(value<0||value>100){light_drag=false;return;}
 if(code==LV_EVENT_RELEASED){
  light_drag=false;
  lv_indev_t *input=lv_indev_active();lv_point_t point;lv_area_t area;lv_obj_get_coords(lv_event_get_target(event),&area);
  if(!input)return;
  lv_indev_get_point(input,&point);
  if(point.x<area.x1-12||point.x>area.x2+12||point.y<area.y1-12||point.y>area.y2+12){lv_slider_set_value(light_slider,brightness(d),LV_ANIM_OFF);return;}
  lighting_send_brightness((uint8_t)value);
 }
}
static void rgb_view(lv_event_t *event);
static void lighting_light_details(lv_obj_t *root);
static void light_details(lv_obj_t *root)
{lighting_light_details(root);}
static bool rgb_enabled(const dsr_mobile_device_t *d)
{return d->endpoint_kind==DSR_ENDPOINT_RGB&&!model.demo&&d->online&&d->error==DSR_OK&&!d->busy&&d->can_rgb&&(page==DSR_MOBILE_GROUP_CONTROL?actions.set_group!=NULL:actions.set_rgb!=NULL);}
static void rgb_view(lv_event_t *event)
{(void)event;rgb_color_view=!rgb_color_view;rgb_dirty=false;render();}
static void rgb_preview(void)
{
 bool readable=lighting_selected()&&lighting_device()->online&&lighting_device()->error==DSR_OK&&((lighting_device()->rgb.flags&DSR_RGB_VALID)||(page==DSR_MOBILE_GROUP_CONTROL&&rgb_dirty));
 if(rgb_swatch)lv_obj_set_style_bg_color(rgb_swatch,readable?lv_color_hex(((uint32_t)rgb_draft[0]<<16)|((uint32_t)rgb_draft[1]<<8)|rgb_draft[2]):dsr_sc_color(DSR_SC_OFF),0);
 for(unsigned i=0;i<3;i++)if(rgb_values[i]){char text[24];if(readable)snprintf(text,sizeof(text),"%c  %u","RGB"[i],rgb_draft[i]);else snprintf(text,sizeof(text),"%c  —","RGB"[i]);lv_label_set_text(rgb_values[i],text);}
 if(rgb_apply_button){if(rgb_dirty&&lighting_selected()&&rgb_enabled(lighting_device()))lv_obj_remove_state(rgb_apply_button,LV_STATE_DISABLED);else lv_obj_add_state(rgb_apply_button,LV_STATE_DISABLED);}
}
static void rgb_color_event(lv_event_t *event)
{
 if(!lighting_selected()||!rgb_enabled(lighting_device()))return;
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(i>2)return;
 int value=lv_slider_get_value(lv_event_get_target(event));if(value<0||value>255)return;
 if(lv_event_get_code(event)==LV_EVENT_PRESS_LOST){
  const dsr_rgb_status_t *c=&lighting_device()->rgb;const uint8_t actual[]={c->r,c->g,c->b};rgb_draft[i]=actual[i];lv_slider_set_value(lv_event_get_target(event),actual[i],LV_ANIM_OFF);rgb_dirty=memcmp(rgb_draft,actual,3)!=0;
 }else {rgb_draft[i]=(uint8_t)value;rgb_dirty=true;}
 rgb_draft_id=lighting_device()->id;rgb_preview();
}
static void rgb_apply(lv_event_t *event)
{
 (void)event;if(!rgb_dirty||!lighting_selected()||rgb_draft_id!=lighting_device()->id||!rgb_enabled(lighting_device()))return;
 uint8_t r=rgb_draft[0],g=rgb_draft[1],b=rgb_draft[2];rgb_dirty=false;lighting_send_rgb(r,g,b);
}
#include "dsr_ui_mobile_lighting_cyber.inc"
static void rgb_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=lighting_device();bool readable=d->online&&d->error==DSR_OK&&(d->rgb.flags&DSR_RGB_VALID);
 rgb_draft[0]=readable?d->rgb.r:0;rgb_draft[1]=readable?d->rgb.g:0;rgb_draft[2]=readable?d->rgb.b:0;rgb_draft_id=d->id;
 lighting_rgb_details(root);
}
static void ht_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];char text[80];
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){
  bool fresh=d->online&&d->error==DSR_OK;
  dsr_sc_badge(root,fresh?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,fresh?"Online":"Offline",12,94,104,fresh?DSR_SC_GLOW:0xffc65b);
  dsr_sc_badge(root,DSR_ICON_CONTACT,d->busy?"Odczyt stanu…":d->error!=DSR_OK?device_error(d->error):fresh?"Odpowiedź odebrana":"Możliwe uśpienie",124,94,184,fresh?DSR_SC_TEXT:0xffc65b);
  const dsr_ui_icon_t icons[]={DSR_ICON_THERMOSTAT,DSR_ICON_ILLUMINANCE};const char *names[]={"Temperatura","Wilgotność"};
  for(unsigned i=0;i<2;i++){
   lv_obj_t *card=dsr_sc_panel(root,12+(int)i*152,128,144,102,DSR_SC_CARD,10);dsr_sc_icon(card,icons[i],12,10,28,DSR_SC_GLOW);
   dsr_sc_text(card,names[i],10,45,124,&dsr_roboto_14,DSR_SC_MUTED);
   if(fresh&&(i?d->has_humidity:d->has_temperature))snprintf(text,sizeof(text),i?"%.1f%%":"%.1f °C",i?d->humidity_pct:d->temperature_c);else snprintf(text,sizeof(text),i?"— %%":"— °C");
   dsr_sc_text(card,dsr_sc_user(text),10,69,124,&dsr_roboto_22,DSR_SC_TEXT);
  }
  lv_obj_t *battery=dsr_sc_panel(root,12,238,296,60,DSR_SC_CARD,10);dsr_sc_icon(battery,DSR_ICON_BATTERY,12,18,28,DSR_SC_GLOW);
  if(fresh&&d->has_battery)snprintf(text,sizeof(text),"%s %.0f%%",dsr_sc_tr("Bateria"),d->battery_pct);else snprintf(text,sizeof(text),"%s —",dsr_sc_tr("Bateria"));
  dsr_sc_text(battery,dsr_sc_user(text),52,10,232,&dsr_roboto_18,DSR_SC_TEXT);
  dsr_sc_text(battery,!fresh||!(d->ht.flags&DSR_HT_EXTERNAL_KNOWN)?"Zasilanie zewnętrzne: nieznane":d->ht.flags&DSR_HT_EXTERNAL_PRESENT?"Zasilanie zewnętrzne: tak":"Zasilanie zewnętrzne: nie",52,37,232,&dsr_roboto_12,DSR_SC_MUTED);
  lv_obj_t *read=dsr_sc_button(root,12,306,296,48,DSR_SC_BLUE,10,refresh,NULL);dsr_sc_icon(read,DSR_ICON_REFRESH,12,12,24,dsr_sc_icon_accent());center(dsr_sc_text(read,"Odśwież status",46,15,240,&dsr_roboto_16,DSR_SC_TEXT));if(d->busy||!d->can_refresh||!actions.refresh)lv_obj_add_state(read,LV_STATE_DISABLED);
  lv_obj_t *note=dsr_sc_panel(root,12,356,296,70,DSR_SC_BG,0);lv_obj_add_flag(note,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(note,LV_DIR_VER);dsr_sc_scrollbar(note);
  dsr_sc_text(note,d->ht.errors?"Błędy pomiarów":"Czas pomiaru nieznany. Czujnik usypia także na USB. Wybudź przyciskiem, jeśli nie odpowiada.",4,0,280,&dsr_roboto_12,d->ht.errors?0xffc18c:DSR_SC_MUTED);return;
 }
 lv_obj_t *panel=dsr_sc_panel(root,12,89,296,246,DSR_SC_CARD,10);
 dsr_sc_text(panel,"H&T Gen3 • tylko odczyt",12,10,272,&dsr_roboto_16,DSR_SC_TEXT);
 const char *state=d->busy?"Odczyt stanu…":d->error!=DSR_OK?device_error(d->error):d->online?"Odpowiedź urządzenia odebrana":"Brak odpowiedzi • możliwe uśpienie";
 dsr_sc_text(panel,state,12,40,272,&dsr_roboto_12,DSR_SC_MUTED);
 if(d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"— °C");
 center(dsr_sc_text(panel,text,8,69,280,&dsr_roboto_28,DSR_SC_TEXT));
 if(d->has_humidity)snprintf(text,sizeof(text),"%.1f%% RH",d->humidity_pct);else snprintf(text,sizeof(text),"— %% RH");
 center(dsr_sc_text(panel,text,8,110,280,&dsr_roboto_22,DSR_SC_TEXT));
 if(d->has_battery)snprintf(text,sizeof(text),"%s %.1f%%",dsr_sc_tr("Bateria"),d->battery_pct);else snprintf(text,sizeof(text),"%s —",dsr_sc_tr("Bateria"));
 center(dsr_sc_text(panel,text,8,144,280,&dsr_roboto_16,DSR_SC_TEXT));
 const char *external=!(d->ht.flags&DSR_HT_EXTERNAL_KNOWN)?"Zasilanie zewnętrzne: nieznane":d->ht.flags&DSR_HT_EXTERNAL_PRESENT?"Zasilanie zewnętrzne: tak":"Zasilanie zewnętrzne: nie";
 center(dsr_sc_text(panel,external,8,172,280,&dsr_roboto_12,DSR_SC_MUTED));
 if(d->ht.errors){snprintf(text,sizeof(text),"%s: %u",dsr_sc_tr("Błędy pomiarów"),(unsigned)d->ht.errors);center(dsr_sc_text(panel,text,8,194,280,&dsr_roboto_12,0xffc18c));}
 sample_age=dsr_sc_text(panel,"",8,229,280,&dsr_roboto_12,DSR_SC_MUTED);center(sample_age);update_sample_age();
 lv_obj_t *button=dsr_sc_button(root,12,344,296,48,DSR_SC_CARD,10,refresh,NULL);center(dsr_sc_text(button,"Odśwież status",8,15,280,&dsr_roboto_14,DSR_SC_TEXT));if(d->busy||!d->can_refresh||!actions.refresh)lv_obj_add_state(button,LV_STATE_DISABLED);
 dsr_sc_text(root,"Czas pomiaru nieznany. Czujnik usypia także na USB. Wybudź przyciskiem, jeśli nie odpowiada.",16,396,288,&dsr_roboto_12,DSR_SC_MUTED);
}
/* Shelly BLU read through a gateway: state, measurements, age of the last
 * BLE packet the gateway received. Read-only; refresh re-reads the gateway. */
/* BLU TRV, Shelly Smart Control style: -/+ change a local target per device
 * and it is sent 1.2 s after the last tap (one BLE write per adjustment).
 * Big values elsewhere are what the gateway reports, never the request. */
#define TRV_SEND_DELAY_MS 1200u
static struct {uint32_t id,due,intent;uint8_t half;bool dirty,pending;} trv_local[16];
static lv_timer_t *trv_timer;
#ifdef ESP_PLATFORM
EXT_RAM_BSS_ATTR
#endif
static struct {lv_obj_t *label,*knob,*caption,*arc;uint32_t id;bool dragging;} trv_dial;
static void trv_knob(uint8_t half);
static void trv_reset(void){memset(trv_local,0,sizeof(trv_local));memset(&trv_dial,0,sizeof(trv_dial));group_trv_cancel();}

static bool trv_device(const dsr_mobile_device_t *d){return d->endpoint_kind==DSR_ENDPOINT_BLU&&(d->blu.errors&DSR_BLU_TRV);}
static bool trv_known(const dsr_mobile_device_t *d){return d->online&&d->error==DSR_OK&&d->has_trv_target&&isfinite(d->trv_target_c)&&d->trv_target_c>=DSR_TRV_MIN_HALF/2.0&&d->trv_target_c<=DSR_TRV_MAX_HALF/2.0;}
static uint8_t trv_reported(const dsr_mobile_device_t *d){return trv_known(d)?(uint8_t)(d->trv_target_c*2.0+0.5):40u;}
static int trv_slot(uint32_t id){for(unsigned i=0;i<16;i++)if(trv_local[i].dirty&&trv_local[i].id==id)return (int)i;return -1;}
void dsr_mobile_trv_request_state(uint32_t id,uint8_t half,uint32_t intent,bool retained)
{
 if(!id||!intent||half<DSR_TRV_MIN_HALF||half>DSR_TRV_MAX_HALF)return;
 int at=trv_slot(id);
 if(retained){
  if(at<0)for(unsigned i=0;i<16;i++)if(!trv_local[i].dirty){at=(int)i;break;}
  if(at>=0){trv_local[at].id=id;trv_local[at].half=half;trv_local[at].intent=intent;trv_local[at].dirty=true;trv_local[at].pending=true;}
 }else if(at>=0&&trv_local[at].pending&&trv_local[at].intent==intent){trv_local[at].dirty=trv_local[at].pending=false;}
 if(active)render();
}
/* What the dial and the pill show: the unsent local target, else the gateway's. */
static uint8_t trv_shown(const dsr_mobile_device_t *d){int at=trv_slot(d->id);return at>=0?trv_local[at].half:trv_reported(d);}
static bool trv_can(const dsr_mobile_device_t *d){return trv_known(d)&&!(d->blu.errors&DSR_BLU_TRV_FAULT)&&!d->busy&&!unbound(d)&&(model.demo||actions.set_trv);}
static void trv_send(lv_timer_t *timer)
{
 (void)timer;if(!active)return;uint32_t now=lv_tick_get();
 for(unsigned i=0;i<16;i++){
  if(!trv_local[i].dirty||trv_local[i].pending||(trv_dial.dragging&&trv_dial.id==trv_local[i].id)||(int32_t)(now-trv_local[i].due)<0)continue;
  trv_local[i].dirty=false;unsigned index=0;while(index<model.device_count&&model.devices[index].id!=trv_local[i].id)index++;
  if(index>=model.device_count)continue;
  dsr_mobile_device_t *d=&model.devices[index];
  if(model.demo){d->trv_target_c=trv_local[i].half/2.0;d->has_trv_target=true;render();}
  else if(trv_can(d)&&trv_local[i].half!=trv_reported(d))actions.set_trv(index,trv_local[i].half);
  else render();
 }
}
static bool trv_draft(unsigned index,uint8_t half)
{
 if(index>=model.device_count||!trv_can(&model.devices[index]))return false;
 const dsr_mobile_device_t *d=&model.devices[index];int at=trv_slot(d->id);
 if(at<0){for(unsigned i=0;i<16&&at<0;i++)if(!trv_local[i].dirty)at=(int)i;if(at<0)return false;trv_local[at].id=d->id;}
 trv_local[at].half=half;trv_local[at].dirty=true;trv_local[at].pending=false;trv_local[at].due=lv_tick_get()+TRV_SEND_DELAY_MS;
 if(!trv_timer)trv_timer=lv_timer_create(trv_send,100,NULL);
 return true;
}
static void trv_step(lv_event_t *event)
{
 uintptr_t value=(uintptr_t)lv_event_get_user_data(event);unsigned index=(unsigned)(value/2);int step=value%2?1:-1;
 if(index>=model.device_count||!trv_can(&model.devices[index]))return;
 int next=(int)trv_shown(&model.devices[index])+step;
 if(next<(int)DSR_TRV_MIN_HALF)next=(int)DSR_TRV_MIN_HALF;
 if(next>(int)DSR_TRV_MAX_HALF)next=(int)DSR_TRV_MAX_HALF;
 if(trv_draft(index,(uint8_t)next)){
  if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&page==DSR_MOBILE_DETAILS&&index==selected&&trv_dial.label)trv_knob((uint8_t)next);
  else render();
 }
}
static void trv_round_action(lv_obj_t *parent,unsigned index,int x,int y,int size,bool plus,bool valid,lv_event_cb_t callback)
{
 lv_obj_t *b=dsr_sc_button(parent,x,y,size,size,0xe8ecef,LV_RADIUS_CIRCLE,callback,(void *)(uintptr_t)(index*2+(plus?1:0)));
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 if(hud)dsr_ui_cyber_power(b,true,valid);
 lv_obj_t *t=dsr_sc_text(b,plus?"+":"-",0,0,LV_SIZE_CONTENT,&dsr_roboto_22,hud?DSR_SC_TEXT:plus?0xe5322d:DSR_SC_BLUE);
 /* Center the visible glyph, including its baseline and side bearings. */
 const lv_font_t *font=lv_obj_get_style_text_font(t,0);lv_font_glyph_dsc_t glyph;
 int inner=size-2*lv_obj_get_style_border_width(b,0);
 if(lv_font_get_glyph_dsc(font,&glyph,plus?'+':'-',0))
  lv_obj_set_pos(t,(inner-glyph.box_w)/2-glyph.ofs_x,(inner-glyph.box_h)/2-(font->line_height-font->base_line-glyph.box_h-glyph.ofs_y));
 else lv_obj_center(t);
 if(!valid){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_color(b,dsr_sc_color(0x555b60),LV_STATE_DISABLED);}
}
static void trv_round(lv_obj_t *parent,unsigned index,int x,int y,int size,bool plus,bool valid)
{trv_round_action(parent,index,x,y,size,plus,valid,trv_step);}
static void half_text(char *out,size_t size,uint8_t half){snprintf(out,size,"%u.%u °C",(unsigned)half/2u,(unsigned)(half%2u)*5u);}
static void trv_knob(uint8_t half)
{
 if(!trv_dial.label||!trv_dial.knob)return;
 char text[32];half_text(text,sizeof(text),half);lv_label_set_text(trv_dial.label,text);
 double angle=(135.0+270.0*(half-DSR_TRV_MIN_HALF)/(double)(DSR_TRV_MAX_HALF-DSR_TRV_MIN_HALF))*3.14159265358979/180.0;
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;int radius=hud?96:82;
 lv_obj_set_pos(trv_dial.knob,(hud?162:160)+(int)lround(radius*cos(angle))-13,(hud?218:222)+(int)lround(radius*sin(angle))-13);
 if(group_trv_selected()&&trv_dial.arc)lv_arc_set_value(trv_dial.arc,half);
 if(hud){
  bool pending=group_trv_selected()?group_draft.dirty||group_state(selected).busy:selected<model.device_count&&(trv_slot(model.devices[selected].id)>=0||model.devices[selected].command_pending);
  lv_obj_set_style_text_color(trv_dial.label,dsr_sc_color(pending?0xffbf24:DSR_SC_TEXT),0);
  if(trv_dial.caption)dsr_sc_label_set_text(trv_dial.caption,pending?"Nastawa • oczekuje":"Nastawa temperatury");
  if(trv_dial.arc)lv_arc_set_value(trv_dial.arc,half);
 }
}
static void trv_arc_event(lv_event_t *e)
{
 if(group_trv_selected()){group_trv_arc(e);return;}
 lv_event_code_t code=lv_event_get_code(e);unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(e);
 if(!active||page!=DSR_MOBILE_DETAILS||i!=selected||i>=model.device_count||!trv_can(&model.devices[i]))return;
 if(code==LV_EVENT_PRESSED){trv_dial.dragging=true;trv_dial.id=model.devices[i].id;}
 else if(code==LV_EVENT_VALUE_CHANGED&&trv_dial.dragging){uint8_t half=(uint8_t)lv_arc_get_value(lv_event_get_target(e));if(trv_draft(i,half))trv_knob(half);}
 else if(code==LV_EVENT_RELEASED){trv_dial.dragging=false;int at=trv_slot(trv_dial.id);if(at>=0)trv_local[at].due=lv_tick_get()+TRV_SEND_DELAY_MS;}
 else if(code==LV_EVENT_PRESS_LOST&&trv_dial.dragging){trv_reset();render();}
}
#include "dsr_ui_mobile_trv_cyber.inc"
static void trv_details(lv_obj_t *root)
{trv_details_view(root,&model.devices[selected],false);}
static void trv_details_view(lv_obj_t *root,const dsr_mobile_device_t *d,bool group)
{
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){cyber_trv_details(root,d,group);return;}
 char text[96];const dsr_blu_status_t *b=&d->blu;bool live=d->online,known=group||trv_known(d)||trv_slot(d->id)>=0,can=group?group_ready(group_state(selected)):trv_can(d);
 uint8_t half=group?group_draft.value:trv_shown(d);
 const int cx=160,cy=222,r=88;
 static const uint32_t stops[]={0x2f6bff,0xeeeef2,0xe5322d};dsr_sc_gradient_arc(root,cx,cy,r,12,135,270,stops,3,18,live);
 /* Native LVGL arc supplies annular hit testing and continuous pointer input.
  * The coloured dial remains visible beneath its transparent input surface. */
 lv_obj_t *arc=lv_arc_create(root);trv_dial.arc=arc;lv_obj_remove_style_all(arc);lv_obj_set_pos(arc,cx-r,cy-r);lv_obj_set_size(arc,r*2,r*2);
 lv_arc_set_range(arc,DSR_TRV_MIN_HALF,DSR_TRV_MAX_HALF);lv_arc_set_bg_angles(arc,135,45);lv_arc_set_value(arc,half);lv_arc_set_change_rate(arc,720);
 lv_obj_set_style_arc_width(arc,32,LV_PART_MAIN);lv_obj_set_style_arc_width(arc,32,LV_PART_INDICATOR);lv_obj_set_style_arc_opa(arc,LV_OPA_TRANSP,LV_PART_MAIN);lv_obj_set_style_arc_opa(arc,LV_OPA_TRANSP,LV_PART_INDICATOR);
 lv_obj_add_flag(arc,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_remove_flag(arc,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_add_event_cb(arc,trv_arc_event,LV_EVENT_ALL,(void *)(uintptr_t)selected);
 if(!can)lv_obj_add_state(arc,LV_STATE_DISABLED);
 trv_dial.knob=dsr_sc_panel(root,0,0,26,26,0xe8ecef,LV_RADIUS_CIRCLE);edge(trv_dial.knob,0x1b1f22,2);lv_obj_remove_flag(trv_dial.knob,LV_OBJ_FLAG_CLICKABLE);
 if(!known)lv_obj_add_flag(trv_dial.knob,LV_OBJ_FLAG_HIDDEN);
 if(known&&(!group||d->has_temperature))snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"— °C");
 center(dsr_sc_text(root,text,cx-60,157,120,&dsr_roboto_16,DSR_SC_TEXT));
 center(dsr_sc_text(root,"Temperatura",cx-60,179,120,&dsr_roboto_12,DSR_SC_MUTED));
 trv_dial.label=dsr_sc_text(root,"",cx-80,208,160,&dsr_roboto_28,DSR_SC_TEXT);center(trv_dial.label);trv_knob(half);
 if(!known)lv_label_set_text(trv_dial.label,"— °C");
 bool retained=!group&&trv_slot(d->id)>=0;
 center(dsr_sc_text(root,retained?"Nastawa • oczekuje":"Nastawa",cx-98,240,196,&dsr_roboto_12,retained?0xffbf24:DSR_SC_MUTED));
 trv_round_action(root,selected,cx-54,270,48,false,can,group?group_trv_step:trv_step);trv_round_action(root,selected,cx+6,270,48,true,can,group?group_trv_step:trv_step);
 dsr_sc_text(root,"4 °C",cx-r-8,304,40,&dsr_roboto_12,DSR_SC_MUTED);dsr_sc_text(root,"30 °C",cx+r-30,304,44,&dsr_roboto_12,DSR_SC_MUTED);
 if(group){group_trv_members(root);group_trv_note(root);return;}
 if(known)snprintf(text,sizeof(text),dsr_sc_tr("zawór %u%%"),(unsigned)b->trv_pos);else snprintf(text,sizeof(text),"%s",dsr_sc_tr("zawór —"));
 dsr_sc_icon(root,DSR_ICON_VALVE,24,333,26,known?DSR_SC_BLUE:DSR_SC_OFF);
 center(dsr_sc_text(root,dsr_sc_user(text),52,338,104,&dsr_roboto_14,DSR_SC_MUTED));
 if(live&&d->has_battery)snprintf(text,sizeof(text),dsr_sc_tr("Bateria %.0f%%"),d->battery_pct);else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Bateria —"));
 dsr_sc_icon(root,DSR_ICON_BATTERY,166,334,24,live&&d->has_battery?DSR_SC_BLUE:DSR_SC_OFF);
 center(dsr_sc_text(root,dsr_sc_user(text),192,338,116,&dsr_roboto_14,DSR_SC_MUTED));
 bool sending=trv_slot(d->id)>=0||(d->busy&&d->command_pending&&live);
 const char *state=!live&&(d->error==DSR_OK||d->error==DSR_OFFLINE)?"Bramka nie odpowiada":d->error!=DSR_OK?device_error(d->error):sending?"Wysyłanie do termostatu przez bramkę…":"";
 center(dsr_sc_text(root,state,12,369,296,&dsr_roboto_12,d->error!=DSR_OK||!live?0xffc18c:DSR_SC_MUTED));
 if(live&&(b->errors&DSR_BLU_TRV_FAULT))center(dsr_sc_text(root,"Termostat zgłasza błąd (np. brak kalibracji)",12,391,296,&dsr_roboto_12,0xffc18c));
 else{sample_age=dsr_sc_text(root,"",12,404,296,&dsr_roboto_11,DSR_SC_MUTED);center(sample_age);update_sample_age();}
}
static dsr_ui_icon_t contact_icon(const dsr_mobile_device_t *d,bool open)
{return device_icon(d)==DSR_ICON_DOOR?(open?DSR_ICON_DOOR_OPEN:DSR_ICON_DOOR_CLOSED):(open?DSR_ICON_WINDOW_OPEN:DSR_ICON_WINDOW_CLOSED);}
/* Cyber-only presentation. Flags and readings remain gateway-owned; there
 * is no tilt/alarm inference and no change to the refresh transport. */
static bool cyber_blu(const dsr_mobile_device_t *d)
{return dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE&&d->endpoint_kind==DSR_ENDPOINT_BLU&&!trv_device(d);}
static bool cyber_contact(const dsr_mobile_device_t *d)
{return d->kind==DSR_DEVICE_CONTACT||device_icon(d)==DSR_ICON_DOOR||(d->icon&&device_icon(d)==DSR_ICON_CONTACT)||(d->blu.flags&DSR_BLU_HAS_OPEN);}
static bool cyber_blu_live(const dsr_mobile_device_t *d)
{return d->online&&d->error==DSR_OK&&(d->blu.flags&DSR_BLU_VALID)&&!(d->blu.errors&DSR_BLU_ERROR_MASK);}
static dsr_ui_icon_t cyber_contact_icon(const dsr_mobile_device_t *d)
{
 bool door=device_icon(d)==DSR_ICON_DOOR;
 if(!cyber_blu_live(d)||!(d->blu.flags&DSR_BLU_HAS_OPEN))return door?DSR_ICON_DOOR_NO_DATA:DSR_ICON_WINDOW_NO_DATA;
 return contact_icon(d,(d->blu.flags&DSR_BLU_OPEN)!=0);
}
static uint32_t cyber_contact_color(const dsr_mobile_device_t *d)
{return !cyber_blu_live(d)||!(d->blu.flags&DSR_BLU_HAS_OPEN)?0x8295ab:(d->blu.flags&DSR_BLU_OPEN)?0xffb52e:0x00cfff;}
static void blu_info(lv_event_t *event)
{
 (void)event;if(info_overlay||selected>=model.device_count)return;
 info_open=true;info_overlay=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);lv_obj_set_user_data(info_overlay,(void *)(uintptr_t)1);lv_obj_set_style_bg_opa(info_overlay,190,0);lv_obj_add_flag(info_overlay,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *p=dsr_sc_panel(info_overlay,12,110,296,248,DSR_SC_CARD,12);
 dsr_sc_text(p,"Informacje o czujniku",12,12,272,&dsr_roboto_18,DSR_SC_TEXT);
 lv_obj_t *name=dsr_sc_text(p,dsr_sc_name(model.devices[selected].name),12,46,272,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,42);
 dsr_sc_text(p,"Dane czujnika są odczytywane przez bramkę Shelly. Odświeżanie sprawdza bramkę; nie wymusza nowego sygnału czujnika.",12,96,272,&dsr_roboto_14,DSR_SC_MUTED);
 lv_obj_t *close=dsr_sc_button(p,12,192,272,44,DSR_SC_PANEL,8,close_info,NULL);center(dsr_sc_text(close,"Zamknij",4,12,264,&dsr_roboto_16,DSR_SC_TEXT));
}
static void cyber_blu_details(lv_obj_t *root,const dsr_mobile_device_t *d)
{
 const dsr_blu_status_t *b=&d->blu;bool live=cyber_blu_live(d),contact=cyber_contact(d);char text[96];
 uint32_t state_color=contact?cyber_contact_color(d):live?0x00cfff:0x8295ab;
 content=dsr_sc_panel(root,8,90,304,334,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);
 lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);dsr_sc_scrollbar(content);
 lv_obj_t *panel=dsr_sc_panel(content,0,0,304,202,DSR_SC_CARD,12);edge(panel,DSR_SC_BLUE,1);
 dsr_sc_badge(panel,d->online&&d->error==DSR_OK?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,d->online&&d->error==DSR_OK?"Online":"Offline",8,8,96,d->online&&d->error==DSR_OK?DSR_SC_GLOW:0xffc65b);
 dsr_sc_icon(panel,contact?cyber_contact_icon(d):DSR_ICON_CONTACT,112,3,80,live?0x00cfff:0x8295ab);
 const char *main_text=!live?"Brak danych":contact?((b->flags&DSR_BLU_HAS_OPEN)?((b->flags&DSR_BLU_OPEN)?"Otwarte":"Zamknięte"):"Brak danych"):(b->flags&DSR_BLU_HAS_MOTION)?((b->flags&DSR_BLU_MOTION)?"Ruch wykryty":"Brak ruchu"):NULL;
 if(!main_text){if(d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Brak danych"));main_text=dsr_sc_user(text);}
 lv_obj_t *well=dsr_sc_panel(panel,16,86,272,38,DSR_SC_PANEL,8);dsr_ui_cyber_inset(well);edge(well,state_color,1);
 lv_obj_t *label=dsr_sc_text(well,main_text,2,2,268,&dsr_roboto_28,state_color);center(label);truncate(label,34);
 dsr_sc_panel(panel,12,130,280,1,DSR_SC_EDGE,0);dsr_sc_panel(panel,147,138,1,35,DSR_SC_EDGE,0);
 dsr_sc_icon(panel,d->has_illuminance||contact?DSR_ICON_ILLUMINANCE:DSR_ICON_THERMOSTAT,16,141,24,live?0x00cfff:0x8295ab);
 if(live&&d->has_illuminance)snprintf(text,sizeof(text),"%.0f lx",d->illuminance_lux);
 else if(live&&d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"—");
 label=dsr_sc_text(panel,text,46,143,96,&dsr_roboto_18,DSR_SC_TEXT);truncate(label,25);
 dsr_sc_icon(panel,DSR_ICON_BATTERY,160,136,20,live?0x00cfff:0x8295ab);
 if(live&&d->has_battery)snprintf(text,sizeof(text),"%s %.0f%%",dsr_sc_tr("Bateria"),d->battery_pct);else snprintf(text,sizeof(text),"%s —",dsr_sc_tr("Bateria"));
 label=dsr_sc_text(panel,dsr_sc_user(text),184,136,110,&dsr_roboto_14,DSR_SC_TEXT);truncate(label,18);
 dsr_sc_icon(panel,DSR_ICON_SIGNAL,160,156,20,live?0x00cfff:0x8295ab);
 if(live&&(b->flags&DSR_BLU_HAS_RSSI))snprintf(text,sizeof(text),"%d dBm",(int)b->rssi);else snprintf(text,sizeof(text),"— dBm");
 dsr_sc_text(panel,text,184,157,110,&dsr_roboto_14,DSR_SC_TEXT);
 dsr_sc_panel(panel,12,177,280,1,DSR_SC_EDGE,0);dsr_sc_icon(panel,DSR_ICON_CLOCK,12,182,16,DSR_SC_MUTED);
 if(live&&d->blu_age_s>=0){unsigned age=(unsigned)d->blu_age_s;
  if(age<120)snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u s temu"),age);else if(age<7200)snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u min temu"),age/60);else snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u h temu"),age/3600);}
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Czas ostatniego sygnału nieznany"));
 label=dsr_sc_text(panel,dsr_sc_user(text),34,182,256,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_update_layout(label);
 int panel_height=182+lv_obj_get_height(label)+5;
 /* Non-contact BLU models retain their additional measurements, on demand in
  * the same bounded scroll area, instead of losing them in a clipped block. */
 if(live&&(d->has_humidity||(b->flags&DSR_BLU_HAS_BUTTON)||(d->has_temperature&&d->has_illuminance))){
  text[0]=0;if(d->has_temperature&&d->has_illuminance)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);
  if(d->has_humidity)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f%% RH",text[0]?" • ":"",d->humidity_pct);
  if(b->flags&DSR_BLU_HAS_BUTTON)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%s %u",text[0]?" • ":"",dsr_sc_tr("Przycisk"),(unsigned)b->button);
  label=dsr_sc_text(panel,dsr_sc_user(text),12,panel_height,280,&dsr_roboto_14,DSR_SC_TEXT);lv_obj_update_layout(label);panel_height+=lv_obj_get_height(label)+6;
 }
 if(panel_height<202)panel_height=202;
 lv_obj_set_height(panel,panel_height);
 int gateway_y=panel_height+8;
 lv_obj_t *gateway=dsr_sc_panel(content,0,gateway_y,304,68,DSR_SC_CARD,10);dsr_sc_icon(gateway,d->online&&d->error==DSR_OK?DSR_ICON_ONLINE:DSR_ICON_OFFLINE,10,16,32,DSR_SC_BLUE);dsr_sc_panel(gateway,48,10,1,48,DSR_SC_EDGE,0);
 dsr_sc_text(gateway,"Shelly BLU • przez bramkę",58,5,234,&dsr_roboto_14,DSR_SC_TEXT);
 const char *state=d->busy&&!d->online?"Odczyt stanu…":!d->online&&(d->error==DSR_OK||d->error==DSR_OFFLINE)?"Bramka nie odpowiada":d->error!=DSR_OK?device_error(d->error):(b->errors&DSR_BLU_ERROR_MASK)?((b->errors&(DSR_BLU_ERROR_KEY|DSR_BLU_ERROR_DECRYPT))?"Bramka: brak lub błędny klucz szyfrowania":"Bramka: błąd danych BTHome"):d->online?"Odczyt z bramki odebrany":"Bramka nie odpowiada";
 label=dsr_sc_text(gateway,state,58,25,234,&dsr_roboto_12,d->online&&d->error==DSR_OK&&!(b->errors&DSR_BLU_ERROR_MASK)?0x00cfff:0xffb52e);lv_obj_update_layout(label);
 int age_y=25+lv_obj_get_height(label)+4;
 sample_age=dsr_sc_text(gateway,"",58,age_y,234,&dsr_roboto_12,DSR_SC_MUTED);update_sample_age();lv_obj_update_layout(sample_age);
 int gateway_height=age_y+lv_obj_get_height(sample_age)+6;if(gateway_height<68)gateway_height=68;lv_obj_set_height(gateway,gateway_height);
 lv_obj_t *button=dsr_sc_button(content,0,gateway_y+gateway_height+8,304,48,DSR_SC_BLUE,10,refresh,NULL);dsr_ui_cyber_panel(button,true);
 dsr_sc_icon(button,DSR_ICON_REFRESH,16,12,24,dsr_sc_icon_accent());dsr_sc_panel(button,52,10,1,28,DSR_SC_EDGE,0);
 center(dsr_sc_text(button,d->busy?"Odczyt stanu…":"Odśwież status",62,14,228,&dsr_roboto_16,DSR_SC_TEXT));
 if(d->busy||!d->can_refresh||!actions.refresh||model.demo)lv_obj_add_state(button,LV_STATE_DISABLED);
}
static void blu_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];char text[96];const dsr_blu_status_t *b=&d->blu;
 if(cyber_blu(d)){cyber_blu_details(root,d);return;}
 bool hud=dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE;
 lv_obj_t *panel=dsr_sc_panel(root,12,89,296,hud?208:246,DSR_SC_CARD,10);
 lv_obj_t *gateway=panel;
 if(hud){gateway=dsr_sc_panel(root,12,304,296,70,DSR_SC_CARD,10);lv_obj_add_flag(gateway,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(gateway,LV_DIR_VER);dsr_sc_scrollbar(gateway);lv_obj_set_flex_flow(gateway,LV_FLEX_FLOW_COLUMN);lv_obj_set_style_pad_hor(gateway,10,0);lv_obj_set_style_pad_ver(gateway,6,0);lv_obj_set_style_pad_row(gateway,4,0);}
 dsr_sc_text(gateway,"Shelly BLU • przez bramkę",12,hud?6:10,272,hud?&dsr_roboto_12:&dsr_roboto_16,DSR_SC_TEXT);
 /* Background polls keep the last values; only a reading with nothing current says so. */
 const char *state=d->busy&&!d->online?"Odczyt stanu…":!d->online&&(d->error==DSR_OK||d->error==DSR_OFFLINE)?"Bramka nie odpowiada":d->error!=DSR_OK?device_error(d->error):d->online?"Odczyt z bramki odebrany":"Bramka nie odpowiada";
 dsr_sc_text(gateway,state,12,hud?24:40,272,&dsr_roboto_12,DSR_SC_MUTED);
 bool live=d->online;
 const char *main_text=!live?"—":(b->flags&DSR_BLU_HAS_OPEN)?((b->flags&DSR_BLU_OPEN)?"Otwarte":"Zamknięte"):(b->flags&DSR_BLU_HAS_MOTION)?((b->flags&DSR_BLU_MOTION)?"Ruch wykryty":"Brak ruchu"):NULL;
 if(b->flags&DSR_BLU_HAS_OPEN){
  if(hud){lv_obj_t *well=dsr_sc_panel(panel,12,8,272,44,DSR_SC_PANEL,10);dsr_ui_cyber_inset(well);}
  lv_obj_t *art=dsr_sc_panel(panel,16,hud?58:68,76,hud?76:86,DSR_SC_PANEL,14);
  dsr_sc_icon(art,live?contact_icon(d,(b->flags&DSR_BLU_OPEN)!=0):DSR_ICON_CONTACT,6,8,64,!live?DSR_SC_OFF:(b->flags&DSR_BLU_OPEN)?0xf0a020:DSR_SC_BLUE);
  center(dsr_sc_text(panel,main_text?main_text:"—",hud?12:100,hud?14:78,hud?272:184,hud?&dsr_roboto_28:&dsr_roboto_22,hud?(live?((b->flags&DSR_BLU_OPEN)?0xffc107:DSR_SC_BLUE):DSR_SC_OFF):DSR_SC_TEXT));
 }else if(main_text)center(dsr_sc_text(panel,main_text,8,hud?20:69,280,hud?&dsr_roboto_22:&dsr_roboto_28,DSR_SC_TEXT));
 else{if(d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"—");center(dsr_sc_text(panel,text,8,hud?20:69,280,&dsr_roboto_28,DSR_SC_TEXT));}
 text[0]=0;
 if(live&&d->has_illuminance)snprintf(text,sizeof(text),"%.0f lx",d->illuminance_lux);
 if(live&&main_text&&d->has_temperature)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.1f °C",text[0]?" • ":"",d->temperature_c);
 if(live&&d->has_humidity)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f%% RH",text[0]?" • ":"",d->humidity_pct);
 if(live&&(b->flags&DSR_BLU_HAS_BUTTON))snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%s %u",text[0]?" • ":"",dsr_sc_tr("Przycisk"),(unsigned)b->button);
 lv_obj_t *metrics=dsr_sc_text(panel,text[0]?text:"—",(b->flags&DSR_BLU_HAS_OPEN)?100:8,hud?66:112,(b->flags&DSR_BLU_HAS_OPEN)?184:280,hud?(strlen(text)>24?&dsr_roboto_14:&dsr_roboto_18):&dsr_roboto_16,DSR_SC_TEXT);center(metrics);
 if(hud){lv_obj_set_height(metrics,66);lv_label_set_long_mode(metrics,LV_LABEL_LONG_MODE_DOTS);}
 if(live&&d->has_battery)snprintf(text,sizeof(text),"%s %.0f%%",dsr_sc_tr("Bateria"),d->battery_pct);else snprintf(text,sizeof(text),"%s —",dsr_sc_tr("Bateria"));
 if(live&&(b->flags&DSR_BLU_HAS_RSSI))snprintf(text+strlen(text),sizeof(text)-strlen(text)," • %d dBm",(int)b->rssi);
 if(hud)dsr_sc_panel(panel,12,139,272,1,DSR_SC_EDGE,0);
 center(dsr_sc_text(panel,text,8,hud?148:144,280,hud?&dsr_roboto_16:&dsr_roboto_14,DSR_SC_TEXT));
 if(live&&d->blu_age_s>=0){unsigned age=(unsigned)d->blu_age_s;
  if(age<120)snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u s temu"),age);else if(age<7200)snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u min temu"),age/60);else snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u h temu"),age/3600);}
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Czas ostatniego sygnału nieznany"));
 center(dsr_sc_text(panel,dsr_sc_user(text),8,hud?177:172,280,&dsr_roboto_12,DSR_SC_MUTED));
 if(live&&(b->errors&DSR_BLU_ERROR_MASK))center(dsr_sc_text(hud?gateway:panel,(b->errors&(DSR_BLU_ERROR_KEY|DSR_BLU_ERROR_DECRYPT))?"Bramka: brak lub błędny klucz szyfrowania":"Bramka: błąd danych BTHome",8,hud?66:194,280,&dsr_roboto_12,0xffc18c));
 sample_age=dsr_sc_text(hud?gateway:panel,"",8,hud?46:229,280,&dsr_roboto_12,DSR_SC_MUTED);center(sample_age);update_sample_age();
 lv_obj_t *button=dsr_sc_button(root,12,hud?380:344,296,hud?44:48,DSR_SC_CARD,10,refresh,NULL);center(dsr_sc_text(button,"Odśwież status",8,hud?12:15,280,hud?&dsr_roboto_16:&dsr_roboto_14,DSR_SC_TEXT));if(d->busy||!d->can_refresh||!actions.refresh)lv_obj_add_state(button,LV_STATE_DISABLED);
 if(!hud)dsr_sc_text(root,"Dane z bramki Shelly",16,402,288,&dsr_roboto_12,DSR_SC_MUTED);
}
/* Recovered owner-approved Cyber compositions share the real model/actions. */
static void open_device_settings(lv_event_t *event);
#include "dsr_ui_mobile_cyber.inc"
static void details(lv_obj_t *root)
{
    if(selected>=model.device_count)return;
    dsr_mobile_device_t *d=&model.devices[selected];
    if(d->kind==DSR_DEVICE_COVER){
        cover_details(root);return;
    }
    if(d->endpoint_kind==DSR_ENDPOINT_HT){ht_details(root);return;}
    if(d->endpoint_kind==DSR_ENDPOINT_BLU){if(d->blu.errors&DSR_BLU_TRV)trv_details(root);else blu_details(root);return;}
    if(d->endpoint_kind==DSR_ENDPOINT_RGB){if(rgb_color_view)rgb_details(root);else light_details(root);return;}
    if(d->endpoint_kind==DSR_ENDPOINT_LIGHT){light_details(root);return;}
    relay_details(root,d);
}
static void relay_details(lv_obj_t *root,const dsr_mobile_device_t *d)
{
    bool group=group_display_index(d)>=0;
    if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){cyber_relay_details(root,d);return;}
    lv_obj_t *panel=dsr_sc_panel(root,12,89,296,256,DSR_SC_CARD,10);
    dsr_sc_text(panel,"Główne sterowanie",13,12,270,&dsr_roboto_16,DSR_SC_TEXT);
    const char *status=group?group_relay_status(d):unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):!d->online?"Offline • stan niepotwierdzony":d->busy?(d->command_pending?"Wysyłanie polecenia…":"Odczyt stanu…"):d->output?"Włączone • Online":"Wyłączone • Online";
    center(dsr_sc_text(panel,status,8,41,280,&dsr_roboto_12,d->error!=DSR_OK?0xffc18c:d->online?DSR_SC_TEXT:DSR_SC_MUTED));
    char power[40];if(d->online&&d->error==DSR_OK&&d->has_power&&!d->busy)snprintf(power,sizeof(power),"%.1f W",d->power_w);else snprintf(power,sizeof(power),"—");
    center(dsr_sc_text(panel,power,8,70,280,&dsr_roboto_28,DSR_SC_TEXT));
    center(dsr_sc_text(panel,d->has_power?"Moc chwilowa":"Pomiar mocy niedostępny",8,103,280,&dsr_roboto_12,DSR_SC_MUTED));
    if(group)dsr_sc_power_button(panel,105,130,86,d->output,d->online&&group_ready(group_state(selected)),DSR_ICON_POWER,DSR_SC_GLOW,group_power,(void *)(uintptr_t)selected);else power_button(panel,selected,105,130,86);
    if(actions.refresh){lv_obj_t *button=dsr_sc_button(panel,202,148,82,48,DSR_SC_PANEL,10,refresh,NULL);lv_obj_t *label=dsr_sc_text(button,"Odśwież stan",4,8,74,&dsr_roboto_12,DSR_SC_TEXT);center(label);if(d->busy||!d->can_refresh)lv_obj_add_state(button,LV_STATE_DISABLED);}
    center(dsr_sc_text(panel,unbound(d)?"Dodaj urządzenie ponownie przyciskiem +":d->error!=DSR_OK?"Odśwież, aby potwierdzić stan":!d->online?not_online(d):d->busy?"Oczekiwanie…":d->output?"Wyłącz":"Włącz",8,220,280,&dsr_roboto_14,d->online?DSR_SC_BLUE:DSR_SC_MUTED));
    sample_age=dsr_sc_text(panel,"",8,242,280,&dsr_roboto_12,DSR_SC_MUTED);center(sample_age);update_sample_age();
    char voltage[32],current[32];
    if(d->online&&d->error==DSR_OK&&d->has_voltage)snprintf(voltage,sizeof(voltage),"%.1f V",d->voltage_v);else snprintf(voltage,sizeof(voltage),"—");
    if(d->online&&d->error==DSR_OK&&d->has_current)snprintf(current,sizeof(current),"%.3f A",d->current_a);else snprintf(current,sizeof(current),"—");
    metric(root,"Napięcie",voltage,12);metric(root,"Natężenie",current,164);
}
static void open_device_settings(lv_event_t *event)
{(void)event;if(selected<model.device_count&&actions.device_settings)actions.device_settings(model.devices[selected].id);}
static void device_settings(lv_obj_t *root)
{
 if(selected>=model.device_count)return;
 const dsr_mobile_device_t *d=&model.devices[selected];char text[160];
 if(dsr_sc_style()==DSR_SC_STYLE_CYBER_BLUE){cyber_device_settings(root,d);return;}
 content=dsr_sc_panel(root,12,96,296,246,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);
 lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);dsr_sc_scrollbar(content);
 lv_obj_t *identity=dsr_sc_panel(content,0,0,296,174,DSR_SC_CARD,14);
 dsr_sc_icon(identity,DSR_ICON_INFO,12,13,24,DSR_SC_BLUE);dsr_sc_text(identity,dsr_sc_name(d->name),48,17,236,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(identity,unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):d->online?"Online":not_online(d),12,44,272,&dsr_roboto_14,d->online&&d->error==DSR_OK?DSR_SC_BLUE:DSR_SC_MUTED);
 lv_obj_t *model_name=dsr_sc_text(identity,d->model[0]?d->model:"Model nieodczytany",12,68,272,&dsr_roboto_14,DSR_SC_MUTED);truncate(model_name,20);
 snprintf(text,sizeof(text),dsr_sc_tr("Adres karty: %s • kanał %u"),d->host[0]?d->host:"—",d->channel);dsr_sc_text(identity,text,12,110,272,&dsr_roboto_12,DSR_SC_TEXT);
 snprintf(text,sizeof(text),d->fw[0]?dsr_sc_tr("Firmware: %s"):"%s",d->fw[0]?d->fw:dsr_sc_tr("Firmware nieodczytany"));dsr_sc_text(identity,text,12,90,272,&dsr_roboto_12,DSR_SC_MUTED);
 const char *family=d->identity[0]?(d->generation==DSR_GEN1?"Gen1":d->generation==DSR_GEN2_PLUS?"Gen2+":"—"):"—";
 if(d->physical_generation)snprintf(text,sizeof(text),"%s • Gen %u",dsr_sc_tr("Zapisane ID"),d->physical_generation);else snprintf(text,sizeof(text),"%s • %s",dsr_sc_tr("Zapisane ID"),family);dsr_sc_text(identity,text,12,130,272,&dsr_roboto_12,DSR_SC_MUTED);
 lv_obj_t *id=dsr_sc_text(identity,d->identity[0]?d->identity:"Tożsamość niepotwierdzona",12,150,272,&dsr_roboto_12,DSR_SC_TEXT);truncate(id,20);
 lv_obj_t *readings=dsr_sc_panel(content,0,184,296,78,DSR_SC_PANEL,12);
 dsr_sc_text(readings,"Dostępne pomiary",12,11,272,&dsr_roboto_14,DSR_SC_TEXT);
 text[0]=0;bool valid=d->online&&d->error==DSR_OK;
 if(trv_device(d)){if(valid&&d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);if(valid&&d->has_trv_target)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%s %.1f °C",text[0]?" • ":"",dsr_sc_tr("Nastawa"),d->trv_target_c);if(valid&&d->has_battery)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f%%",text[0]?" • ":"",d->battery_pct);valid=false;}
 else if(d->endpoint_kind==DSR_ENDPOINT_BLU){if(d->has_contact)snprintf(text,sizeof(text),"%s",dsr_sc_tr(d->contact_open?"Otwarte":"Zamknięte"));else if(d->blu.flags&DSR_BLU_HAS_MOTION)snprintf(text,sizeof(text),"%s",dsr_sc_tr((d->blu.flags&DSR_BLU_MOTION)?"Ruch wykryty":"Brak ruchu"));if(d->has_illuminance)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f lx",text[0]?" • ":"",d->illuminance_lux);if(d->has_battery)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f%%",text[0]?" • ":"",d->battery_pct);valid=false;}
 if(d->endpoint_kind==DSR_ENDPOINT_HT){if(d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);if(d->has_humidity)snprintf(text+strlen(text),sizeof(text)-strlen(text)," • %.1f%% RH",d->humidity_pct);if(d->has_battery)snprintf(text+strlen(text),sizeof(text)-strlen(text)," • %.1f%%",d->battery_pct);valid=false;}
 if(valid&&d->has_power&&isfinite(d->power_w))snprintf(text,sizeof(text),"%.1f W",d->power_w);
 if(valid&&d->has_voltage&&isfinite(d->voltage_v))snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.1f V",text[0]?" • ":"",d->voltage_v);
 if(valid&&d->has_current&&isfinite(d->current_a))snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.3f A",text[0]?" • ":"",d->current_a);
 dsr_sc_text(readings,text[0]?text:!d->can_refresh?"Brak aktywnego połączenia":valid?"Brak obsługiwanych pomiarów":"Odczyty niedostępne",12,37,272,&dsr_roboto_14,DSR_SC_MUTED);
 sample_age=dsr_sc_text(readings,"",12,58,272,&dsr_roboto_12,DSR_SC_MUTED);update_sample_age();
 lv_obj_t *update=dsr_sc_button(root,12,354,142,48,DSR_SC_CARD,12,refresh,NULL);center(dsr_sc_text(update,"Odśwież status",4,16,134,&dsr_roboto_14,DSR_SC_TEXT));
 if(!d->can_refresh||!actions.refresh||d->busy)lv_obj_add_state(update,LV_STATE_DISABLED);
 lv_obj_t *edit=dsr_sc_button(root,166,354,142,48,DSR_SC_CARD,12,edit_device,NULL);center(dsr_sc_text(edit,"Edytuj kartę",4,16,134,&dsr_roboto_14,DSR_SC_TEXT));
 if(model.demo||!d->id||!actions.edit_device)lv_obj_add_state(edit,LV_STATE_DISABLED);
 dsr_sc_text(root,"Ustawienia karty • lokalnie",12,410,296,&dsr_roboto_12,DSR_SC_MUTED);
 if(actions.device_settings&&!model.demo&&d->id&&d->generation==DSR_GEN2_PLUS){
  lv_obj_t *b=dsr_sc_button(content,0,274,296,50,DSR_SC_CARD,12,open_device_settings,NULL);
  center(dsr_sc_text(b,"Ustawienia Shelly",8,16,280,&dsr_roboto_14,DSR_SC_TEXT));
 }
 dsr_sc_text(content,"Nazwa i pokój karty dotyczą pilota. Ustawienia Shelly są zapisywane w urządzeniu.",4,338,288,&dsr_roboto_12,DSR_SC_MUTED);
}
static void render(void)
{
    if(!active)return;
    if(dsr_board_display_sleeping()){wake_repaint=true;return;}
    wake_repaint=false;
    cover_view_reset();
    trv_dial.dragging=false;trv_dial.label=trv_dial.knob=trv_dial.caption=trv_dial.arc=NULL;rgb_dirty=false;rgb_swatch=rgb_apply_button=NULL;memset(rgb_values,0,sizeof(rgb_values));cover_drag=false;cover_preview_label=cover_position_label=NULL;cover_visual=NULL;light_drag=false;light_slider=light_knob=light_value=wheel_knob=rgb_text=NULL;int scroll=content?lv_obj_get_scroll_y(content):0;
    lv_obj_t *root=lv_screen_active();content=sample_age=info_overlay=NULL;placed_count=0;lv_obj_clean(root);lv_obj_remove_style_all(root);
    dsr_sc_background(root);
    lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
    static const char fallback_title[]="Mój dom";
    const char *title=(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS)&&selected<model.device_count?model.devices[selected].name:
        page==DSR_MOBILE_ROOM&&selected<model.room_count?model.rooms[selected].name:fallback_title;
    if(page==DSR_MOBILE_DETAILS&&selected<model.device_count&&cyber_blu(&model.devices[selected]))dsr_sc_blu_header(root,dsr_sc_name(title),model.demo,model.wifi,navigate,(void *)(uintptr_t)98,blu_info);
    else header(root,"",page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS||page==DSR_MOBILE_ROOM||page==DSR_MOBILE_GROUP_CONTROL);
    if(page!=DSR_MOBILE_GROUP_CONTROL||lighting_page()||group_shared_selected())tabs(root);
    if(page==DSR_MOBILE_DEVICE_SETTINGS){device_settings(root);if(content)lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);}
    else if(page==DSR_MOBILE_DETAILS){details(root);if(content)lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);}
    else if(page==DSR_MOBILE_GROUP_CONTROL)group_details(root);
    else {
        toolbar(root,page==DSR_MOBILE_HOME?(model.dashboard_name[0]?dsr_sc_name(model.dashboard_name):"Mój pulpit"):page==DSR_MOBILE_ROOMS?"Pomieszczenia":"Urządzenia");
        int top=page==DSR_MOBILE_HOME?102:156;
        content=dsr_sc_panel(root,12,top,296,DSR_SC_NAV_TOP-top-DSR_SC_MARGIN,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);
        lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);dsr_sc_scrollbar(content);
        lv_obj_set_scrollbar_mode(content,LV_SCROLLBAR_MODE_AUTO);
        if(page==DSR_MOBILE_ROOMS){
            for(unsigned i=0;i<model.room_count;i++)room_card(content,i,(int)(i%2)*152,(int)(i/2)*194,144,186);/* rooms page: not partial */
            if(!model.room_count){
                empty("Brak pomieszczeń","Utwórz pierwszy pokój, aby uporządkować urządzenia.");
                lv_obj_t *add=dsr_sc_button(content,12,210,272,48,DSR_SC_PANEL,10,add_room,NULL);
                center(dsr_sc_text(add,"Dodaj pomieszczenie",4,15,264,&dsr_roboto_14,DSR_SC_TEXT));
                if(model.demo||!actions.add_room)lv_obj_add_state(add,LV_STATE_DISABLED);
            }
        } else {
            unsigned at=0;
            if(page==DSR_MOBILE_HOME&&model.configured_layout){
                int y=0,x=0,row_height=0;
                for(unsigned t=0;t<model.tile_count;t++){
                    dsr_mobile_tile_t *tile=&model.tiles[t];
                    if(tile->type==0&&tile->index<model.device_count){if(x){x=0;y+=row_height+8;row_height=0;}int h=device_card_height(&model.devices[tile->index],tile->height==2);place(device_card(content,tile->index,0,y,296,h,tile->fields),0,tile->index,tile->fields,0,y,296,h);y+=h+8;at++;}
                    else if(tile->type==1&&tile->index<model.room_count){int height=tile->height==2?240:186;if(tile->span==2&&x){x=0;y+=row_height+8;row_height=0;}place(room_card(content,tile->index,x,y,tile->span==2?296:144,height),1,tile->index,0,x,y,tile->span==2?296:144,height);if(height>row_height)row_height=height;if(x||tile->span==2){x=0;y+=row_height+8;row_height=0;}else x=152;at++;}
                    else if(tile->type==2&&tile->index<model.scene_count){if(x){x=0;y+=row_height+8;row_height=0;}scene_card(tile->index,y,tile->height==2?120:90);y+=tile->height==2?128:98;at++;}
                    else if(tile->type==3&&tile->index<model.group_count){if(x){x=0;y+=row_height+8;row_height=0;}place(group_card(tile->index,y,group_card_height(tile->index)),2,tile->index,0,0,y,296,group_card_height(tile->index));y+=group_card_height(tile->index)+8;at++;}
                }
            }else {int y=0;if(page==DSR_MOBILE_DEVICES)for(unsigned g=0;g<model.group_count;g++){place(group_card(g,y,group_card_height(g)),2,g,0,0,y,296,group_card_height(g));y+=group_card_height(g)+DSR_SC_GAP;at++;}for(unsigned i=0;i<model.device_count;i++)if(page!=DSR_MOBILE_ROOM||model.devices[i].room==selected){int h=device_card_height(&model.devices[i],false);place(device_card(content,i,0,y,296,h,255),0,i,255,0,y,296,h);y+=h+DSR_SC_GAP;at++;}}
            if(!at)empty("Brak urządzeń","Dodaj pierwsze urządzenie Shelly przyciskiem +, aby rozpocząć sterowanie swoim domem.");
        }
        lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);
    }
    if(model.demo){lv_obj_t *caption=dsr_sc_text(root,"DEMO • dane przykładowe",12,415,296,&dsr_roboto_12,0xe1bb70);center(caption);}
    bottom(root);
    if(info_open)dashboard_info(NULL);
    rendered_language=dsr_sc_language();rendered_style=dsr_sc_style();
    /* A horizontal metrics drag must never displace the whole screen. */
    lv_obj_update_layout(root);lv_obj_scroll_to(root,0,0,LV_ANIM_OFF);
}
void dsr_mobile_show(dsr_mobile_page_t p,unsigned index)
{
    if(p>DSR_MOBILE_GROUP_CONTROL)return;
    if(p==DSR_MOBILE_GROUPS||p==DSR_MOBILE_SCENES){if(actions.open_actions)actions.open_actions(p==DSR_MOBILE_SCENES);return;}
    if(actions.leave_actions)actions.leave_actions();
    repaint_pending=false;trv_reset();info_open=false;
    if(cover_preview&&p!=DSR_MOBILE_DETAILS)end_preview();
    if(p==DSR_MOBILE_GROUP_CONTROL){previous_page=page==DSR_MOBILE_HOME?DSR_MOBILE_HOME:DSR_MOBILE_DEVICES;previous_selected=0;group_draft.initialized=false;}
    if((p==DSR_MOBILE_DETAILS||p==DSR_MOBILE_DEVICE_SETTINGS)&&page!=DSR_MOBILE_DETAILS&&page!=DSR_MOBILE_DEVICE_SETTINGS){previous_page=page;previous_selected=selected;}
    rgb_dirty=rgb_color_view=false;content=NULL;page=p;selected=index;active=true;render();
}
void dsr_mobile_show_device_settings(uint32_t id)
{
 for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==id){dsr_mobile_show(DSR_MOBILE_DEVICE_SETTINGS,i);return;}
 dsr_mobile_show(DSR_MOBILE_DEVICES,0);
}
void dsr_mobile_return_device(uint32_t id)
{
 for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==id){content=NULL;page=DSR_MOBILE_DETAILS;selected=i;active=true;render();return;}
 dsr_mobile_show(DSR_MOBILE_DEVICES,0);
}
void dsr_mobile_open_group(uint32_t id)
{for(unsigned i=0;i<model.group_count;i++)if(model.groups[i].id==id){dsr_mobile_show(DSR_MOBILE_GROUP_CONTROL,i);return;}dsr_mobile_show(DSR_MOBILE_DEVICES,0);}
static void translate_labels(lv_obj_t *object)
{
 if(lv_obj_check_type(object,&lv_label_class))dsr_sc_label_set_text(object,lv_label_get_text(object));
 for(unsigned i=0;i<lv_obj_get_child_count(object);i++)translate_labels(lv_obj_get_child(object,(int)i));
}
static void translate_draft(void)
{
 if(rendered_language==dsr_sc_language())return;
 translate_labels(lv_screen_active());rendered_language=dsr_sc_language();
}
/* Same screen structure, only device values differ: rebuild just those cards
 * (and room cards, whose badge sums device power). Anything else, or any doubt,
 * falls back to the full render. */
/* Electrical numbers do not change controls, status, geometry or identity.
 * Update their existing pill instead of deleting an entire glowing card. */
static bool electrical_only(const dsr_mobile_device_t *a,const dsr_mobile_device_t *b)
{
 if(a->kind==DSR_DEVICE_SENSOR||a->kind==DSR_DEVICE_CONTACT||a->kind==DSR_DEVICE_COVER)return false;
 const size_t first=offsetof(dsr_mobile_device_t,power_w),after=offsetof(dsr_mobile_device_t,id),end=offsetof(dsr_mobile_device_t,energy_wh);
 return !memcmp(a,b,first)&&!memcmp((const char *)a+after,(const char *)b+after,end-after);
}
static bool update_electrical(placed_card_t *c,const dsr_mobile_device_t *d)
{
 if(!c->fields)return true;
 lv_obj_t *pill=lv_obj_get_child(c->obj,-1);if(!pill||lv_obj_get_child_count(pill)!=1||!lv_obj_check_type(lv_obj_get_child(pill,0),&lv_label_class))return false;
 char power[40],voltage[40];const char *values[2];unsigned count=0;
 if(!d->online||d->error!=DSR_OK)values[count++]=dsr_sc_tr("Odczyty niedostępne");
 else{
  if(c->fields&1){if(d->has_power)snprintf(power,sizeof(power),"%.1f W",d->power_w);else snprintf(power,sizeof(power),"— W");values[count++]=power;}
  if(c->fields&2){if(d->has_voltage)snprintf(voltage,sizeof(voltage),"%.1f V",d->voltage_v);else snprintf(voltage,sizeof(voltage),"— V");values[count++]=voltage;}
 }
 if(!count)return false;
 card_layout_t layout=card_layout(d,c->w);lv_obj_set_pos(pill,layout.metric_x,layout.metric_y);dsr_sc_segments_update(pill,values,count,layout.metric_w);if(card_switch_rows(d))card_switch_metric_center(pill);return true;
}
static bool refresh_cards(const dsr_mobile_model_t *m)
{
    if(rendered_language!=dsr_sc_language()||rendered_style!=dsr_sc_style()||!content||!placed_count||info_open||(page!=DSR_MOBILE_HOME&&page!=DSR_MOBILE_DEVICES&&page!=DSR_MOBILE_ROOM))return false;
    bool demo=m->demo;
#ifndef DSR_UI_REVIEW_DEMO
    demo=false;
#endif
    if(demo!=model.demo||m->wifi!=model.wifi||memcmp(m->home_name,model.home_name,sizeof(model.home_name))||m->device_count!=model.device_count||m->room_count!=model.room_count)return false;
    const size_t after=offsetof(dsr_mobile_model_t,rooms);
    if(memcmp((const char *)m+after,(const char *)&model+after,sizeof(model)-after))return false;
    bool changed[DSR_UI_MAX_DEVICES]={0},numbers[DSR_UI_MAX_DEVICES]={0},any=false;
    for(unsigned i=0;i<model.device_count;i++){
     const dsr_mobile_device_t *a=&model.devices[i],*b=&m->devices[i];
     if(memcmp(a,b,sizeof(*a))){if(a->id!=b->id||device_card_height(a,false)!=device_card_height(b,false)||(page==DSR_MOBILE_ROOM&&a->room!=b->room))return false;changed[i]=any=true;numbers[i]=electrical_only(a,b);}
    }
    if(!any)return false;
    /* Cards are only ever deleted by render(), which also empties placed[]. */
    if(!lv_obj_is_valid(content))return false;
    for(unsigned k=0;k<placed_count;k++)if(lv_obj_get_parent(placed[k].obj)!=content)return false;
    int scroll=lv_obj_get_scroll_y(content);
    memcpy(model.devices,m->devices,sizeof(model.devices));
    for(unsigned k=0;k<placed_count;k++){
     placed_card_t *c=&placed[k];if(c->type==2){bool member_changed=false;for(unsigned i=0;i<model.device_count;i++)if((model.groups[c->index].member_mask&(1u<<i))&&changed[i])member_changed=true;if(!member_changed)continue;if(group_card_height(c->index)!=c->h)return false;}else if(c->type==0?!changed[c->index]:c->type!=1)continue;
     if(c->type==0&&numbers[c->index]&&update_electrical(c,&model.devices[c->index]))continue;
     int32_t at=lv_obj_get_index(c->obj);lv_obj_delete(c->obj);
     c->obj=c->type==0?device_card(content,c->index,c->x,c->y,c->w,c->h,c->fields):c->type==2?group_card(c->index,c->y,c->h):room_card(content,c->index,c->x,c->y,c->w,c->h);
     lv_obj_move_to_index(c->obj,at);
    }
    lv_obj_update_layout(content);if(lv_obj_get_scroll_y(content)!=scroll)lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);
    return true;
}
void dsr_mobile_update(const dsr_mobile_model_t *m)
{
    if(!m||m->device_count>DSR_UI_MAX_DEVICES||m->room_count>DSR_UI_MAX_ROOMS||m->tile_count>24||m->scene_count>8||m->group_count>8)return;
    bool cancel_group_drag=false;
    cover_transition.animate=false;
    if(active&&page==DSR_MOBILE_DETAILS&&selected<model.device_count){
     const dsr_mobile_device_t *old=&model.devices[selected];
     if(old->kind==DSR_DEVICE_COVER&&cover_position(old))for(unsigned i=0;i<m->device_count;i++){
      const dsr_mobile_device_t *next=&m->devices[i];
      if(next->id==old->id&&next->kind==DSR_DEVICE_COVER&&cover_position(next)&&!next->cover_errors&&old->channel==next->channel&&old->generation==next->generation&&!strcmp(old->identity,next->identity)&&!strcmp(old->host,next->host)){
       bool moving=(next->cover_state==DSR_COVER_OPENING&&next->position_pct>=old->position_pct)||(next->cover_state==DSR_COVER_CLOSING&&next->position_pct<=old->position_pct);
       cover_transition.group=false;cover_transition.id=next->id;cover_transition.from=(int)lround(old->position_pct);cover_transition.animate=moving;
      }
     }
    }
    if(active&&group_cover_selected()&&group_members_stable(m)){
     group_state_t old=group_state(selected),next=group_state_of(m,selected);
     bool moving=(next.cover_motion==1&&next.value>=old.value)||(next.cover_motion==2&&next.value<=old.value);
     cover_transition.group=true;cover_transition.id=model.groups[selected].id;cover_transition.from=old.value;cover_transition.animate=old.known&&!old.mixed&&next.known&&!next.mixed&&moving;
    }
    if(cover_preview){preview_saved=*m;
#ifndef DSR_UI_REVIEW_DEMO
        preview_saved.demo=false;
#endif
        return;}
    /* Keep the last telemetry while dark, without changing LVGL objects or
     * starting transitions. The next awake age tick applies it once. */
    if(active&&dsr_board_display_sleeping()){preview_saved=*m;repaint_pending=wake_repaint=true;return;}
    if(wake_repaint)cover_transition.animate=false;
    if(page==DSR_MOBILE_GROUP_CONTROL&&group_draft.family==DSR_ENDPOINT_BLU){
     if(!group_trv_draft_stable(m))group_trv_cancel();
     else if(group_draft.dragging){model=*m;group_model(selected);translate_draft();return;}
    }
    for(unsigned t=0;t<16;t++)if(trv_local[t].dirty){
     const dsr_mobile_device_t *old=NULL,*next=NULL;
     for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==trv_local[t].id)old=&model.devices[i];
     for(unsigned i=0;i<m->device_count;i++)if(m->devices[i].id==trv_local[t].id)next=&m->devices[i];
     bool valid=old&&next&&trv_device(next)&&(trv_local[t].pending||trv_can(next))&&old->generation==next->generation&&old->endpoint_kind==next->endpoint_kind&&old->channel==next->channel&&!strcmp(old->identity,next->identity)&&!strcmp(old->host,next->host)&&(trv_local[t].pending||old->trv_target_c==next->trv_target_c);
     if(!valid){trv_local[t].dirty=false;trv_dial.dragging=false;}
    }
    if(trv_dial.dragging){model=*m;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==trv_dial.id)selected=i;translate_draft();update_sample_age();return;}
    if(page==DSR_MOBILE_GROUP_CONTROL&&rgb_dirty){if(group_rgb_draft_stable(m)){model=*m;group_model(selected);translate_draft();return;}rgb_dirty=false;}
    if(page!=DSR_MOBILE_GROUP_CONTROL&&rgb_dirty&&selected<model.device_count){
     const dsr_mobile_device_t *old=&model.devices[selected],*next=NULL;
     for(unsigned i=0;i<m->device_count;i++)if(m->devices[i].id==rgb_draft_id)next=&m->devices[i];
     if(next&&rgb_enabled(next)&&next->generation==old->generation&&next->endpoint_kind==old->endpoint_kind&&next->channel==old->channel&&!strcmp(next->identity,old->identity)&&!strcmp(next->host,old->host)&&!memcmp(&next->rgb,&old->rgb,sizeof(old->rgb))){model=*m;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==rgb_draft_id)selected=i;translate_draft();return;}
     rgb_dirty=false;
    }
    if(page!=DSR_MOBILE_GROUP_CONTROL&&light_drag&&selected<model.device_count){
     const dsr_mobile_device_t *old=&model.devices[selected],*next=NULL;
     for(unsigned i=0;i<m->device_count;i++)if(m->devices[i].id==light_drag_id)next=&m->devices[i];
     if(next&&light_enabled(next)&&next->generation==old->generation&&next->endpoint_kind==old->endpoint_kind&&next->channel==old->channel&&!strcmp(next->identity,old->identity)&&!strcmp(next->host,old->host)&&(old->endpoint_kind==DSR_ENDPOINT_RGB?next->rgb.flags==old->rgb.flags:next->light.target==old->light.target&&(next->light.flags&DSR_LIGHT_TRANSITION)==(old->light.flags&DSR_LIGHT_TRANSITION))){model=*m;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==light_drag_id)selected=i;translate_draft();update_sample_age();return;}
     light_drag=false;
    }
    if(cover_drag&&cover_drag_group){
     group_state_t old=group_state(selected),next=group_state_of(m,selected);bool capable=true;
     if(selected<m->group_count)for(unsigned i=0;i<m->device_count;i++)if(m->groups[selected].member_mask&(1u<<i))capable&=m->devices[i].can_position;
     if(group_members_stable(m)&&next.family==DSR_ENDPOINT_COVER&&next.known&&!next.busy&&old.mixed==next.mixed&&capable){
      model=*m;dsr_mobile_device_t *view=group_model(selected);if(cover_visual)dsr_ui_cyber_cover_preview(cover_visual,cover_preview_position,(int)lround(view->position_pct));cover_confirmed_text(view);translate_draft();return;
     }
     cover_drag=false;cancel_group_drag=true;
    }
    if(cover_drag&&!cover_drag_group&&selected<model.device_count){
     const dsr_mobile_device_t *old=&model.devices[selected];const dsr_mobile_device_t *next=NULL;
     for(unsigned i=0;i<m->device_count;i++)if(m->devices[i].id==cover_drag_id)next=&m->devices[i];
     if(next&&position_enabled(next)&&next->generation==old->generation&&next->endpoint_kind==old->endpoint_kind&&next->channel==old->channel&&!strcmp(next->identity,old->identity)&&!strcmp(next->host,old->host)){model=*m;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==cover_drag_id)selected=i;if(cover_visual){dsr_ui_cyber_cover_preview(cover_visual,cover_preview_position,(int)lround(model.devices[selected].position_pct));cover_confirmed_text(&model.devices[selected]);}update_sample_age();return;}
     cover_drag=false;
    }
    /* Freshness changes update one label, not the entire screen or scroll state. */
    for(unsigned i=0;i<DSR_UI_MAX_DEVICES;i++){model.devices[i].has_sample_time=m->devices[i].has_sample_time;model.devices[i].sample_ms=m->devices[i].sample_ms;}
    if(!wake_repaint&&!memcmp(&model,m,sizeof(model))&&rendered_language==dsr_sc_language()&&rendered_style==dsr_sc_style()){update_sample_age();return;}
    if(page==DSR_MOBILE_HOME&&model.dashboard_id!=m->dashboard_id)content=NULL;
    if(page==DSR_MOBILE_GROUP_CONTROL&&selected<model.group_count){uint32_t id=model.groups[selected].id;unsigned i=0;while(i<m->group_count&&m->groups[i].id!=id)i++;if(i==m->group_count){page=DSR_MOBILE_DEVICES;selected=0;content=NULL;}else selected=i;}
    if((page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS)&&selected<model.device_count&&model.devices[selected].id){
        uint32_t id=model.devices[selected].id;unsigned next=0;while(next<m->device_count&&m->devices[next].id!=id)next++;
        if(next==m->device_count){page=DSR_MOBILE_HOME;selected=0;content=NULL;}else selected=next;
    }
    if((page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS)&&previous_page==DSR_MOBILE_ROOM&&previous_selected<model.room_count&&model.rooms[previous_selected].id){
        uint32_t id=model.rooms[previous_selected].id;unsigned next=0;while(next<m->room_count&&m->rooms[next].id!=id)next++;
        if(next==m->room_count){previous_page=DSR_MOBILE_ROOMS;previous_selected=0;}else previous_selected=next;
    }
    if(page==DSR_MOBILE_ROOM&&selected<model.room_count&&model.rooms[selected].id){
        uint32_t id=model.rooms[selected].id;unsigned next=0;while(next<m->room_count&&m->rooms[next].id!=id)next++;
        if(next==m->room_count){page=DSR_MOBILE_ROOMS;selected=0;content=NULL;}else selected=next;
    }
    /* A modal owns its targets until Close; background values stay current
     * without deleting the button under a held pointer. */
    if(active&&info_open){model=*m;wake_repaint=false;refresh_info();return;}
    if(active&&!wake_repaint&&(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS)&&selected<model.device_count&&m->device_count==model.device_count&&rendered_language==dsr_sc_language()&&rendered_style==dsr_sc_style()&&model.wifi==m->wifi&&!memcmp(&model.devices[selected],&m->devices[selected],sizeof(model.devices[selected]))){model=*m;return;}
    /* Stable identities only: never defer a rebind/removal behind an old
     * actionable target. A poll during a press/scroll paints after release. */
    bool stable=m->device_count==model.device_count&&m->room_count==model.room_count&&rendered_language==dsr_sc_language()&&rendered_style==dsr_sc_style();
    const size_t structure=offsetof(dsr_mobile_model_t,rooms);
    if(stable&&memcmp((const char *)m+structure,(const char *)&model+structure,sizeof(model)-structure))stable=false;
    for(unsigned i=0;stable&&i<m->device_count;i++)if(m->devices[i].id!=model.devices[i].id||m->devices[i].generation!=model.devices[i].generation||m->devices[i].endpoint_kind!=model.devices[i].endpoint_kind||m->devices[i].channel!=model.devices[i].channel||strcmp(m->devices[i].identity,model.devices[i].identity)||strcmp(m->devices[i].host,model.devices[i].host))stable=false;
    if(active&&stable&&pointer_busy()&&!cancel_group_drag){preview_saved=*m;repaint_pending=true;return;}
    repaint_pending=false;
    if(active&&!wake_repaint&&stable&&refresh_cover_details(m))return;
    if(active&&!wake_repaint&&refresh_cards(m))return;
    model=*m;
#ifndef DSR_UI_REVIEW_DEMO
    model.demo=false;
#endif
    if(active)render();
}
void dsr_mobile_cover_preview(void)
{
 if(!cover_preview)preview_saved=model;
 cover_preview=true;model=(dsr_mobile_model_t){.demo=true,.device_count=1};
 model.devices[0]=(dsr_mobile_device_t){.kind=DSR_DEVICE_COVER,.room=DSR_UI_NO_ROOM,.online=true,.can_cover=true,.can_stop_cover=true,.has_position=true,.position_pct=65,.cover_state=DSR_COVER_STOPPED};
 snprintf(model.devices[0].name,sizeof(model.devices[0].name),"%s",dsr_sc_tr("Roleta w salonie"));
 previous_page=DSR_MOBILE_HOME;previous_selected=0;content=NULL;page=DSR_MOBILE_DETAILS;selected=0;active=true;render();
#ifdef ESP_PLATFORM
 ESP_LOGI("dsr_ui","Cover DEMO: internal_free=%u internal_largest=%u psram_free=%u",(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),(unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),(unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif
}
