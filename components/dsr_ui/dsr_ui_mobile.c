#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_log.h"
#endif

_Static_assert(sizeof(dsr_mobile_model_t)<=8192,"bounded shared mobile view model");
_Static_assert(sizeof(dsr_mobile_device_t)==320&&sizeof(dsr_mobile_model_t)==6944,"Light uses existing mobile padding");
static dsr_mobile_model_t model;
static dsr_mobile_model_t preview_saved;
static bool cover_preview;
static dsr_mobile_actions_t actions;
static dsr_mobile_page_t page;
static unsigned selected;
static dsr_mobile_page_t previous_page;
static unsigned previous_selected;
static bool active;
static lv_obj_t *content;
static lv_obj_t *sample_age;
static lv_obj_t *info_overlay;
static bool info_open,cover_drag;
static uint8_t cover_preview_position;
static uint32_t cover_drag_id;
static lv_obj_t *cover_preview_label;
static lv_timer_t *age_timer;
static unsigned rendered_language;
static bool rgb_color_view,rgb_dirty;
static uint32_t rgb_draft_id;
static uint8_t rgb_draft[3];
static lv_obj_t *rgb_swatch,*rgb_values[3],*rgb_apply_button;
static bool light_drag;
static uint32_t light_drag_id;
static lv_obj_t *light_slider,*light_preview_label;
static void render(void);
static bool unbound(const dsr_mobile_device_t *d);
static bool trv_device(const dsr_mobile_device_t *d);static bool trv_known(const dsr_mobile_device_t *d);static bool trv_can(const dsr_mobile_device_t *d);
static uint8_t trv_shown(const dsr_mobile_device_t *d);static void trv_round(lv_obj_t *parent,unsigned index,int x,int y,int size,bool plus,bool valid);
static void update_sample_age(void)
{
 if(!active||!sample_age||selected>=model.device_count)return;
 const dsr_mobile_device_t *d=&model.devices[selected];char text[72];
 if(d->has_sample_time)snprintf(text,sizeof(text),dsr_sc_tr(DSR_ENDPOINT_IS_SENSOR(d->endpoint_kind)?"Odpowiedź odebrana %u s temu":"Odczyt potwierdzony %u s temu"),(unsigned)((lv_tick_get()-d->sample_ms)/1000));
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Brak potwierdzonego odczytu"));
 lv_label_set_text(sample_age,text);
}
static void age_tick(lv_timer_t *timer){(void)timer;update_sample_age();}

void dsr_mobile_bind(const dsr_mobile_actions_t *a){if(a)actions=*a;else memset(&actions,0,sizeof(actions));if(!age_timer)age_timer=lv_timer_create(age_tick,1000,NULL);}
bool dsr_mobile_active(void){return active;}
dsr_mobile_page_t dsr_mobile_page(void){return page;}
static void end_preview(void){if(cover_preview){model=preview_saved;memset(&preview_saved,0,sizeof(preview_saved));cover_preview=false;}}
void dsr_mobile_hide(void){rgb_dirty=rgb_color_view=false;cover_drag=false;cover_preview_label=NULL;light_drag=false;light_slider=light_preview_label=NULL;end_preview();active=false;info_open=false;content=sample_age=info_overlay=NULL;}
static void center(lv_obj_t *o){lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0);}
static void truncate(lv_obj_t *o,int height){lv_obj_set_height(o,height);lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_DOTS);}
static void edge(lv_obj_t *o,uint32_t c,int width){lv_obj_set_style_border_color(o,dsr_sc_color(c),0);lv_obj_set_style_border_width(o,width,0);}
static void navigate(lv_event_t *e)
{
    unsigned target=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(target==10){if(actions.open_settings)actions.open_settings();return;}
    if(target==11){if(actions.add_device)actions.add_device();return;}
    if(target==12){
#ifdef ESP_PLATFORM
        ESP_LOGI("dsr_config_ui","stage=pencil_click callback=%u",(unsigned)(actions.edit_dashboard!=NULL));
#endif
        if(actions.edit_dashboard)actions.edit_dashboard();
        return;}
    if(target==13){if(actions.select_dashboard)actions.select_dashboard();return;}
    if(target==16){if(!cover_preview&&!model.demo&&selected<model.device_count&&actions.open_charts)actions.open_charts(model.devices[selected].id);return;}
    if(target>=17&&target<=19){if(!model.demo&&actions.open_actions)actions.open_actions(target==17?2:target==18?0:1);return;}
    if(target==14||target==15){dsr_mobile_show(target==14?DSR_MOBILE_DEVICE_SETTINGS:DSR_MOBILE_DETAILS,selected);return;}
    if(target==98&&page==DSR_MOBILE_DEVICE_SETTINGS){dsr_mobile_show(DSR_MOBILE_DETAILS,selected);return;}
    if(target==98){dsr_mobile_show(page==DSR_MOBILE_DETAILS?previous_page:DSR_MOBILE_ROOMS,
        page==DSR_MOBILE_DETAILS?previous_selected:0);return;}
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
static void refresh(lv_event_t *event){(void)event;if(!cover_preview&&!model.demo&&selected<model.device_count&&model.devices[selected].can_refresh&&actions.refresh)actions.refresh(model.devices[selected].id);}
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
    dsr_sc_icon_centered(b,icon,24,DSR_SC_TEXT);return b;
}

static void header(lv_obj_t *root,const char *title,bool back)
{
    dsr_sc_header(root,title,model.demo,model.wifi,back?navigate:NULL,(void *)(uintptr_t)98);
}
static void close_info(lv_event_t *event){info_open=false;info_overlay=NULL;lv_obj_delete(lv_event_get_user_data(event));}
static void dashboard_info(lv_event_t *event)
{
 (void)event;if(info_overlay)return;info_open=true;lv_obj_t *overlay=info_overlay=dsr_sc_panel(lv_screen_active(),0,0,320,480,0x000000,0);
 lv_obj_set_style_bg_opa(overlay,170,0);lv_obj_add_flag(overlay,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *panel=dsr_sc_panel(overlay,12,108,296,252,DSR_SC_CARD,14);
 dsr_sc_text(panel,"Informacje o pulpicie",12,14,272,&dsr_roboto_18,DSR_SC_TEXT);
 lv_obj_t *name=dsr_sc_text(panel,model.dashboard_name[0]?model.dashboard_name:"Mój pulpit",12,47,272,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
 unsigned online=0;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].online)online++;
 char text[96];snprintf(text,sizeof(text),dsr_sc_tr("Urządzenia: %u • online: %u"),model.device_count,online);dsr_sc_text(panel,text,12,87,272,&dsr_roboto_14,DSR_SC_MUTED);
 snprintf(text,sizeof(text),dsr_sc_tr("Pomieszczenia: %u"),model.room_count);dsr_sc_text(panel,text,12,114,272,&dsr_roboto_14,DSR_SC_MUTED);
 dsr_sc_text(panel,model.demo?"DEMO • dane przykładowe":"Stan online dotyczy ostatnich odczytów.",12,149,272,&dsr_roboto_12,model.demo?0xe1bb70:DSR_SC_MUTED);
 lv_obj_t *close=dsr_sc_button(panel,12,193,272,44,DSR_SC_PANEL,10,close_info,overlay);
 center(dsr_sc_text(close,"Zamknij",4,12,264,&dsr_roboto_16,DSR_SC_TEXT));
}
static void tab(lv_obj_t *root,const char *text,int x,int w,bool chosen,lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *b=dsr_sc_button(root,x,44,w,44,DSR_SC_BG,7,cb,ctx);
    if(chosen){edge(b,DSR_SC_BLUE,1);lv_obj_set_style_border_side(b,LV_BORDER_SIDE_TOP|LV_BORDER_SIDE_LEFT|LV_BORDER_SIDE_RIGHT,0);}
    lv_obj_t *s=dsr_sc_text(b,text,0,14,w,&dsr_roboto_14,chosen?DSR_SC_TEXT:DSR_SC_MUTED);center(s);
    truncate(s,18);
    if(!cb&&!chosen){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,LV_STATE_DISABLED);}
    if(!cb&&chosen)lv_obj_remove_flag(b,LV_OBJ_FLAG_CLICKABLE);
}
static void tabs(lv_obj_t *root)
{
    dsr_sc_panel(root,0,87,320,1,DSR_SC_BLUE,0);
    if(page==DSR_MOBILE_HOME){tab(root,"Aktywność",0,110,false,actions.open_actions&&!model.demo?navigate:NULL,(void *)17);tab(root,model.dashboard_name[0]?dsr_sc_name(model.dashboard_name):"Mój pulpit",114,162,true,NULL,NULL);return;}
    if(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS){
        bool settings=page==DSR_MOBILE_DEVICE_SETTINGS;
        bool editable=!cover_preview&&!model.demo&&selected<model.device_count&&model.devices[selected].id;
        tab(root,"Sterowanie",0,109,!settings,settings?navigate:NULL,(void *)(uintptr_t)15);
        tab(root,"Wykresy",111,101,false,editable&&actions.open_charts?navigate:NULL,(void *)(uintptr_t)16);
        tab(root,"Ustawienia",214,106,settings,!settings&&editable?navigate:NULL,(void *)(uintptr_t)14);return;
    }
    if(page==DSR_MOBILE_ROOM){tab(root,"Urządzenia",0,115,true,NULL,NULL);tab(root,"Grupy",117,87,false,actions.open_actions&&!model.demo?navigate:NULL,(void *)18);tab(root,"Sceny",206,114,false,actions.open_actions&&!model.demo?navigate:NULL,(void *)19);return;}
    tab(root,"Pomieszczenia",0,101,page==DSR_MOBILE_ROOMS,navigate,(void *)(uintptr_t)DSR_MOBILE_ROOMS);
    tab(root,"Grupy",103,55,false,actions.open_actions&&!model.demo?navigate:NULL,(void *)18);tab(root,"Sceny",160,55,false,actions.open_actions&&!model.demo?navigate:NULL,(void *)19);
    tab(root,"Urządzenia",217,103,page==DSR_MOBILE_DEVICES,navigate,(void *)(uintptr_t)DSR_MOBILE_DEVICES);
}
static void bottom_action(lv_event_t *event)
{
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);
 if(i==0)dsr_mobile_show(DSR_MOBILE_HOME,0);else if(i==1)dsr_mobile_show(DSR_MOBILE_ROOMS,0);
 else if(i==2&&actions.open_energy&&!model.demo)actions.open_energy();
 else if(i==3&&actions.open_settings)actions.open_settings();else if(i==4){if(page==DSR_MOBILE_ROOMS)add_room(NULL);else if(actions.add_device)actions.add_device();}
}
static void bottom(lv_obj_t *root)
{
 unsigned enabled=3u|(!model.demo&&actions.open_energy?4u:0u)|(actions.open_settings?8u:0u)|((page==DSR_MOBILE_ROOMS?!model.demo&&actions.add_room!=NULL:actions.add_device!=NULL)?16u:0u);
 dsr_sc_navigation(root,page==DSR_MOBILE_HOME?0:1,enabled,bottom_action);
}
static void toolbar(lv_obj_t *root,const char *title)
{
    if(page==DSR_MOBILE_ROOMS||page==DSR_MOBILE_ROOM){
        bool list=page==DSR_MOBILE_ROOMS;
        uint32_t id=!list&&selected<model.room_count?model.rooms[selected].id:0;
        lv_obj_t *button=dsr_sc_button(root,12,94,296,44,DSR_SC_PANEL,10,list?add_room:edit_room,(void *)(uintptr_t)id);
        edge(button,DSR_SC_EDGE,1);dsr_sc_icon(button,list?DSR_ICON_PLUS:DSR_ICON_EDIT,12,10,24,DSR_SC_BLUE);
        dsr_sc_text(button,list?"Dodaj pomieszczenie":"Edytuj pomieszczenie",48,13,240,&dsr_roboto_14,DSR_SC_TEXT);
        if(model.demo||(list?!actions.add_room:!actions.edit_room||!id))lv_obj_add_state(button,LV_STATE_DISABLED);
        return;
    }
    lv_obj_t *bar=dsr_sc_panel(root,10,94,300,44,DSR_SC_PANEL,10);edge(bar,DSR_SC_EDGE,1);
    lv_obj_t *choose=dsr_sc_button(bar,0,0,152,44,DSR_SC_PANEL,8,page==DSR_MOBILE_HOME&&actions.select_dashboard?navigate:NULL,(void *)(uintptr_t)13);
    lv_obj_t *name=dsr_sc_text(choose,title,9,12,122,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
    if(page==DSR_MOBILE_HOME&&actions.select_dashboard)dsr_sc_icon(choose,DSR_ICON_CHEVRON,131,14,16,DSR_SC_MUTED);
    lv_obj_t *plus=icon_button(bar,DSR_ICON_PLUS,156,0,44,0x287ec7,navigate,(void *)(uintptr_t)11);
    if(!actions.add_device)lv_obj_add_state(plus,LV_STATE_DISABLED);
    lv_obj_t *edit=icon_button(bar,DSR_ICON_EDIT,204,0,44,0x287ec7,navigate,(void *)(uintptr_t)12);if(!actions.edit_dashboard)lv_obj_add_state(edit,LV_STATE_DISABLED);
    icon_button(bar,DSR_ICON_INFO,252,0,44,DSR_SC_PANEL,dashboard_info,NULL);
}
static lv_obj_t *power_button(lv_obj_t *parent,unsigned index,int x,int y,int size)
{
    dsr_mobile_device_t *d=&model.devices[index];bool valid=d->online&&!d->busy&&d->can_switch;
    lv_obj_t *b=dsr_sc_button(parent,x,y,size,size,0x252a2e,LV_RADIUS_CIRCLE,control,(void *)(uintptr_t)index);
    edge(b,valid&&d->output?DSR_SC_BLUE:DSR_SC_OFF,size>=70?3:2);
    int icon=size>=70?35:22;
    dsr_sc_icon_centered(b,DSR_ICON_POWER,icon,valid?DSR_SC_TEXT:DSR_SC_OFF);
    if(!valid)lv_obj_add_state(b,LV_STATE_DISABLED);
    return b;
}
static dsr_ui_icon_t kind_icon(dsr_mobile_kind_t kind)
{return (kind==DSR_DEVICE_LIGHT||kind==DSR_DEVICE_RGB)?DSR_ICON_LIGHT:kind==DSR_DEVICE_COVER?DSR_ICON_COVER:kind==DSR_DEVICE_CONTACT||kind==DSR_DEVICE_SENSOR?DSR_ICON_CONTACT:DSR_ICON_PLUG;}
/* Workspace card without a private binding (e.g. power cut between the
 * workspace ACK and the binding save, or a legacy display entry). It is not
 * "offline": adding the same device again reuses this card's id. */
static bool unbound(const dsr_mobile_device_t *d){return !model.demo&&!d->identity[0];}
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
static void device_card(lv_obj_t *parent,unsigned i,int x,int y,int width,int height)
{
    dsr_mobile_device_t *d=&model.devices[i];
    bool cover=d->kind==DSR_DEVICE_COVER,sensor=d->kind==DSR_DEVICE_CONTACT||d->kind==DSR_DEVICE_SENSOR;
    unsigned fields=page==DSR_MOBILE_HOME&&model.configured_layout?d->fields:255;
    lv_obj_t *card=dsr_sc_button(parent,x,y,width,height,0x41474b,DSR_SC_RADIUS,open_device,(void *)(uintptr_t)i);
    lv_obj_set_style_bg_grad_color(card,dsr_sc_color(0x3c4246),0);lv_obj_set_style_bg_grad_dir(card,LV_GRAD_DIR_VER,0);
    lv_obj_t *thumb=dsr_sc_panel(card,8,9,50,50,d->online&&d->output?0x244357:0x29343b,LV_RADIUS_CIRCLE);
    dsr_sc_icon_centered(thumb,kind_icon(d->kind),32,d->online?DSR_SC_TEXT:DSR_SC_OFF);
    int name_width=cover?124:trv_device(d)?98:sensor?218:166;
    lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(d->name),68,10,name_width,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
    const char *state=unbound(d)?"Brak powiązania":d->error!=DSR_OK?device_error(d->error):!d->online?"Brak połączenia":d->busy&&!(sensor&&d->endpoint_kind==DSR_ENDPOINT_BLU)?(d->command_pending?"Wysyłanie polecenia…":"Odczyt stanu…"):sensor?(d->has_contact&&((fields&64)||d->endpoint_kind==DSR_ENDPOINT_BLU)?(d->contact_open?"Otwarte":"Zamknięte"):(d->blu.flags&DSR_BLU_HAS_MOTION)&&d->endpoint_kind==DSR_ENDPOINT_BLU?((d->blu.flags&DSR_BLU_MOTION)?"Ruch wykryty":"Brak ruchu"):"Online"):cover?"Roleta":d->output?"Włączone":"Wyłączone";
    char subtitle[96];snprintf(subtitle,sizeof(subtitle),"%s • %s",d->room<model.room_count?room_name(d):dsr_sc_tr(room_name(d)),dsr_sc_tr(state));
    lv_obj_t *status=dsr_sc_text(card,d->error!=DSR_OK||unbound(d)?state:dsr_sc_user(subtitle),68,35,cover?124:218,&dsr_roboto_12,d->online&&d->error==DSR_OK?DSR_SC_MUTED:0xffc18c);truncate(status,18);
    if(trv_device(d))lv_obj_set_width(status,98);
    if(cover){
     bool valid=d->online&&!d->busy&&d->can_cover&&(model.demo||actions.set_cover);
     card_control(card,i,DSR_ICON_UP,width-104,44,valid,cover_control,i*2+1);
     card_control(card,i,DSR_ICON_DOWN,width-54,44,valid,cover_control,i*2);
    }else if(trv_device(d)){
     /* Shelly-style pill: -  20.0 °C  + */
     lv_obj_t *pill=dsr_sc_panel(card,width-126,8,118,48,0x1d3f73,24);edge(pill,DSR_SC_BLUE,2);bool valid=trv_can(d);
     trv_round(pill,i,4,4,36,false,valid);trv_round(pill,i,74,4,36,true,valid);
     char t[16];if(trv_known(d)){uint8_t h=trv_shown(d);snprintf(t,sizeof(t),"%u.%u",(unsigned)h/2u,(unsigned)(h%2u)*5u);}else snprintf(t,sizeof(t),"—");
     center(dsr_sc_text(pill,t,40,4,34,&dsr_roboto_14,DSR_SC_TEXT));center(dsr_sc_text(pill,"°C",40,24,34,&dsr_roboto_11,DSR_SC_TEXT));
    }else if(!sensor)card_control(card,i,DSR_ICON_POWER,width-58,48,d->online&&!d->busy&&d->can_switch,control,i);
    if(!fields)return;
    char values[128]={0},part[40];
    if((!d->online||d->error!=DSR_OK)&&!DSR_ENDPOINT_IS_SENSOR(d->endpoint_kind))snprintf(values,sizeof(values),"%s",dsr_sc_tr("Odczyty niedostępne"));
    else if(cover){if((fields&32)&&d->has_position)snprintf(values,sizeof(values),"%s   %.0f%%",dsr_sc_tr("Pozycja"),d->position_pct);else snprintf(values,sizeof(values),"%s   —",dsr_sc_tr("Pozycja"));}
    else if(sensor){
      if((fields&4)&&d->has_battery)snprintf(values,sizeof(values),"%s %.0f%%",dsr_sc_tr("Bateria"),d->battery_pct);
      if((fields&8)&&d->has_temperature){snprintf(part,sizeof(part),"  %.1f °C",d->temperature_c);strncat(values,part,sizeof(values)-strlen(values)-1);}
      if((fields&16)&&d->has_humidity){snprintf(part,sizeof(part),"  %.0f%% RH",d->humidity_pct);strncat(values,part,sizeof(values)-strlen(values)-1);}
      if(trv_device(d)&&trv_known(d)){snprintf(part,sizeof(part),dsr_sc_tr("  cel %.1f °C  zawór %u%%"),d->trv_target_c,(unsigned)d->blu.trv_pos);strncat(values,part,sizeof(values)-strlen(values)-1);}
      else if(d->endpoint_kind==DSR_ENDPOINT_BLU&&!trv_device(d)&&d->has_illuminance){ /* TRV target shares the lux storage */snprintf(part,sizeof(part),"  %.0f lx",d->illuminance_lux);strncat(values,part,sizeof(values)-strlen(values)-1);}
      if(!values[0])snprintf(values,sizeof(values),"Brak dodatkowych pomiarów");
    }else{
      if(fields&1){if(d->has_power)snprintf(values,sizeof(values),"%.1f W",d->power_w);else snprintf(values,sizeof(values),"— W");}
      if(fields&2){if(d->has_voltage)snprintf(part,sizeof(part),"    %.1f V",d->voltage_v);else snprintf(part,sizeof(part),"    — V");strncat(values,part,sizeof(values)-strlen(values)-1);}
    }
    if(values[0])dsr_sc_chip(card,values,68,height-26,width-78,d->online?DSR_SC_TEXT:0xffc18c);
}
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
static void room_card(lv_obj_t *parent,unsigned i,int x,int y,int width,int height)
{
    lv_obj_t *card=dsr_sc_button(parent,x,y,width,height,DSR_SC_CARD,DSR_SC_RADIUS,open_room,(void *)(uintptr_t)i);
    static lv_image_dsc_t photos[DSR_UI_MAX_ROOMS];
    if(dsr_ui_photo_source(model.rooms[i].photo,&photos[i])){
        lv_obj_t *window=dsr_sc_panel(card,0,0,width,height-65,DSR_SC_CARD,DSR_SC_RADIUS);lv_obj_set_style_clip_corner(window,true,0);lv_obj_t *image=lv_image_create(window);lv_image_set_src(image,&photos[i]);
        uint32_t scale=(uint32_t)width*256/photos[i].header.w,vertical=(uint32_t)(height-65)*256/photos[i].header.h;if(vertical>scale)scale=vertical;
        lv_image_set_scale(image,scale);lv_image_set_antialias(image,true);lv_obj_center(image);
    }else{lv_obj_t *thumb=dsr_sc_panel(card,(width-52)/2,24,52,52,0x303943,LV_RADIUS_CIRCLE);dsr_sc_icon_centered(thumb,room_icon(i),30,0xe8edf2);}
    lv_obj_t *tag=dsr_sc_panel(card,0,0,78,22,0x25292b,7);lv_obj_set_style_bg_opa(tag,220,0);dsr_sc_icon(tag,room_icon(i),4,3,16,DSR_SC_TEXT);dsr_sc_text(tag,"Pokój",24,4,50,&dsr_roboto_11,DSR_SC_TEXT);
    lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(model.rooms[i].name),6,height-56,width-12,&dsr_roboto_16,DSR_SC_TEXT);center(name);truncate(name,21);
    double power=0;unsigned count=0;bool complete=true;
    for(unsigned d=0;d<model.device_count;d++)if(model.devices[d].room==i){count++;complete&=model.devices[d].online&&model.devices[d].has_power;power+=model.devices[d].power_w;}
    lv_obj_t *badge=dsr_sc_panel(card,(width-82)/2,height-32,82,26,0x262a2d,10);
    char text[32];if(count&&complete)snprintf(text,sizeof(text),"%.1f W",power);else snprintf(text,sizeof(text),"%u %s",count,dsr_sc_tr("urz."));
    dsr_sc_icon(badge,DSR_ICON_ENERGY,5,5,16,DSR_SC_TEXT);center(dsr_sc_text(badge,text,23,6,55,&dsr_roboto_12,DSR_SC_TEXT));
    if(page==DSR_MOBILE_ROOMS){
        lv_obj_t *edit=icon_button(card,DSR_ICON_EDIT,width-44,0,44,0x25292b,edit_room,(void *)(uintptr_t)model.rooms[i].id);
        if(model.demo||!model.rooms[i].id||!actions.edit_room)lv_obj_add_state(edit,LV_STATE_DISABLED);
    }
}
static void scene_run(lv_event_t *event)
{unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(!model.demo&&i<model.scene_count&&actions.run_scene)actions.run_scene(model.scenes[i].id);}
static void group_run(lv_event_t *event)
{unsigned value=(unsigned)(uintptr_t)lv_event_get_user_data(event),i=value/2;if(!model.demo&&i<model.group_count&&actions.run_group)actions.run_group(model.groups[i].id,(value&1)!=0);}
static void scene_card(unsigned index,int y,int height)
{
 dsr_mobile_scene_t *scene=&model.scenes[index];lv_obj_t *card=dsr_sc_button(content,0,y,296,height,0x414548,12,navigate,(void *)(uintptr_t)19);
 lv_obj_t *icon=dsr_sc_panel(card,8,17,50,50,0x29343b,LV_RADIUS_CIRCLE);dsr_sc_icon_centered(icon,DSR_ICON_SCENES,32,DSR_SC_TEXT);
 lv_obj_t *name=dsr_sc_text(card,dsr_sc_name(scene->name),68,10,164,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
 char text[80];snprintf(text,sizeof(text),dsr_sc_tr("Scena • %u działań"),scene->actions);dsr_sc_text(card,text,68,36,210,&dsr_roboto_12,DSR_SC_MUTED);
 card_control(card,0,DSR_ICON_EDIT,244,26,actions.edit_dashboard!=NULL,navigate,12);
 lv_obj_t *run=dsr_sc_button(card,68,height-36,164,30,DSR_SC_PANEL,8,scene_run,(void *)(uintptr_t)index);dsr_sc_text(run,"Uruchom scenę",4,7,156,&dsr_roboto_12,DSR_SC_TEXT);if(model.demo||!actions.run_scene||!scene->actions)lv_obj_add_state(run,LV_STATE_DISABLED);
}
static void group_card(unsigned index,int y,int height)
{
 dsr_mobile_group_t *group=&model.groups[index];lv_obj_t *card=dsr_sc_button(content,0,y,296,height,DSR_SC_CARD,12,navigate,(void *)18);dsr_sc_text(card,dsr_sc_name(group->name),12,10,230,&dsr_roboto_16,DSR_SC_TEXT);card_control(card,0,DSR_ICON_EDIT,244,26,actions.edit_dashboard!=NULL,navigate,12);
 lv_obj_t *on=dsr_sc_button(card,12,height-44,128,34,DSR_SC_PANEL,8,group_run,(void *)(uintptr_t)(index*2+1)),*off=dsr_sc_button(card,152,height-44,128,34,DSR_SC_PANEL,8,group_run,(void *)(uintptr_t)(index*2));dsr_sc_text(on,"Włącz grupę",4,10,120,&dsr_roboto_12,DSR_SC_TEXT);dsr_sc_text(off,"Wyłącz grupę",4,10,120,&dsr_roboto_12,DSR_SC_TEXT);if(model.demo||!actions.run_group||!group->members){lv_obj_add_state(on,LV_STATE_DISABLED);lv_obj_add_state(off,LV_STATE_DISABLED);}
}
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
 (void)event;dsr_mobile_device_t *d=&model.devices[selected];
 if(!d->can_stop_cover)return;
 if(model.demo){d->cover_state=DSR_COVER_STOPPED;render();}
 else if(actions.stop_cover)actions.stop_cover(selected);
}
static void cover_header(lv_obj_t *root,const char *title)
{
 lv_obj_t *back=dsr_sc_button(root,0,0,44,44,DSR_SC_BG,8,navigate,(void *)(uintptr_t)98);
 dsr_sc_icon_centered(back,DSR_ICON_BACK,24,DSR_SC_TEXT);
 lv_obj_t *name=dsr_sc_text(root,dsr_sc_name(title),50,11,216,&dsr_roboto_18,DSR_SC_TEXT);truncate(name,24);
 lv_obj_t *edit=icon_button(root,DSR_ICON_EDIT,272,0,44,DSR_SC_BG,edit_device,NULL);
 if(model.demo||!model.devices[selected].id||!actions.edit_device)lv_obj_add_state(edit,LV_STATE_DISABLED);
}
static void cover_metric(lv_obj_t *row,int *x,const char *value)
{lv_obj_t *chip=dsr_sc_chip(row,value,*x,0,180,DSR_SC_TEXT);*x+=lv_obj_get_style_width(chip,LV_PART_MAIN)+6;}
static bool position_enabled(const dsr_mobile_device_t *d)
{return !cover_preview&&d->online&&d->error==DSR_OK&&!d->busy&&d->has_position&&d->can_position&&actions.set_position;}
static void cover_drag_event(lv_event_t *event)
{
 lv_event_code_t code=lv_event_get_code(event);lv_obj_t *rail=lv_event_get_target(event);
 if(selected>=model.device_count)return;
 dsr_mobile_device_t *d=&model.devices[selected];
 if(code==LV_EVENT_PRESS_LOST){cover_drag=false;if(cover_preview_label)lv_label_set_text(cover_preview_label,"");return;}
 if(!position_enabled(d)){cover_drag=false;return;}
 lv_indev_t *input=lv_indev_active();if(!input)return;lv_point_t point;lv_indev_get_point(input,&point);lv_area_t area;lv_obj_get_coords(rail,&area);
 bool inside=point.x>=area.x1&&point.x<=area.x2&&point.y>=area.y1&&point.y<=area.y2;
 if(code==LV_EVENT_PRESSED){if(!inside)return;cover_drag=true;cover_drag_id=d->id;}
 if(!cover_drag||cover_drag_id!=d->id)return;
 int value=(int)lround((area.y1+138-point.y)*100.0/130.0);if(value<0)value=0;if(value>100)value=100;cover_preview_position=(uint8_t)value;
 if(cover_preview_label){char text[48];snprintf(text,sizeof(text),"%s %u%%",dsr_sc_tr("Podgląd"),cover_preview_position);lv_label_set_text(cover_preview_label,text);}
 if(code==LV_EVENT_RELEASED){cover_drag=false;uint8_t target=cover_preview_position;if(cover_preview_label)lv_label_set_text(cover_preview_label,"");if(inside&&actions.set_position)actions.set_position(selected,target);}
}
static void cover_details(lv_obj_t *root)
{
 dsr_mobile_device_t *d=&model.devices[selected];bool known=cover_position(d),readable=d->online&&d->error==DSR_OK;
 lv_obj_t *row=dsr_sc_panel(root,12,99,296,26,DSR_SC_BG,0);lv_obj_set_style_bg_opa(row,LV_OPA_TRANSP,0);
 lv_obj_add_flag(row,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(row,LV_DIR_HOR);
 lv_obj_remove_flag(row,LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 lv_obj_set_scrollbar_mode(row,LV_SCROLLBAR_MODE_AUTO);int x=0;char value[64];
 if(known)snprintf(value,sizeof(value),"%s %.0f%%",dsr_sc_tr("Pozycja"),d->position_pct);else snprintf(value,sizeof(value),"%s —",dsr_sc_tr("Pozycja"));cover_metric(row,&x,value);
 if(readable&&d->has_power&&isfinite(d->power_w)){snprintf(value,sizeof(value),"%.1f W",d->power_w);cover_metric(row,&x,value);}
 if(readable&&d->has_voltage&&isfinite(d->voltage_v)&&d->voltage_v>=0){snprintf(value,sizeof(value),"%.1f V",d->voltage_v);cover_metric(row,&x,value);}
 if(readable&&d->has_frequency&&isfinite(d->frequency_hz)&&d->frequency_hz>=0){snprintf(value,sizeof(value),"%.1f Hz",d->frequency_hz);cover_metric(row,&x,value);}
 const char *state=d->error!=DSR_OK?device_error(d->error):!d->online?"Offline":d->cover_errors?cover_fault(d->cover_errors):d->command_pending?"Wysyłanie polecenia…":d->cover_state==DSR_COVER_OPENING?"Otwieranie":d->cover_state==DSR_COVER_CLOSING?"Zamykanie":d->cover_state==DSR_COVER_CALIBRATING?"Kalibracja":d->cover_state==DSR_COVER_STOPPED?"Zatrzymano":"Stan nieznany";
 center(dsr_sc_text(root,state,12,132,296,&dsr_roboto_12,readable?DSR_SC_MUTED:0xffc18c));
 /* Actual position remains the fill; dragging previews a separate target. */
 lv_obj_t *rail=dsr_sc_panel(root,112,156,96,148,DSR_SC_PANEL,16);edge(rail,DSR_SC_EDGE,1);
 if(position_enabled(d)){lv_obj_add_flag(rail,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(rail,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_VER);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESSING,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_RELEASED,NULL);lv_obj_add_event_cb(rail,cover_drag_event,LV_EVENT_PRESS_LOST,NULL);}
 lv_obj_t *well=dsr_sc_panel(rail,8,8,78,130,0x101a22,10);lv_obj_set_style_clip_corner(well,true,0);
 int fill=known?(int)lround(d->position_pct*130/100):0;
 if(fill){lv_obj_t *blue=dsr_sc_panel(well,0,130-fill,78,fill,DSR_SC_BLUE,0);lv_obj_set_style_bg_grad_color(blue,dsr_sc_color(0x2379c8),0);lv_obj_set_style_bg_grad_dir(blue,LV_GRAD_DIR_VER,0);}
 for(int y=12;y<130;y+=14){lv_obj_t *line=dsr_sc_panel(well,8,y,62,1,DSR_SC_TEXT,0);lv_obj_set_style_bg_opa(line,24,0);}
 if(known){int y=130-fill-2;if(y<3)y=3;if(y>124)y=124;dsr_sc_panel(well,22,y,34,3,DSR_SC_TEXT,2);}
 else center(dsr_sc_text(well,"—",0,49,78,&dsr_roboto_28,DSR_SC_OFF));
 cover_preview_label=dsr_sc_text(root,"",212,164,106,&dsr_roboto_12,DSR_SC_BLUE);
 dsr_sc_text(root,"100%",67,159,40,&dsr_roboto_11,DSR_SC_OFF);dsr_sc_text(root,"0%",80,289,27,&dsr_roboto_11,DSR_SC_OFF);
 if(known)snprintf(value,sizeof(value),"%s %.0f%%",dsr_sc_tr(d->position_pct==100?"Otwarte":d->position_pct==0?"Zamknięte":"Pozycja"),d->position_pct);
 else snprintf(value,sizeof(value),"%s",dsr_sc_tr("Nieznana pozycja"));
 center(dsr_sc_text(root,value,12,313,296,known?&dsr_roboto_22:&dsr_roboto_18,DSR_SC_TEXT));
 bool valid=d->online&&d->error==DSR_OK&&!d->busy&&d->can_cover&&(model.demo||actions.set_cover);
 bool stop_valid=d->can_stop_cover&&(model.demo||actions.stop_cover);
 lv_obj_t *stop=dsr_sc_button(root,242,225,64,64,DSR_SC_PANEL,12,cover_stop,NULL);edge(stop,DSR_SC_EDGE,1);
 dsr_sc_panel(stop,27,12,10,10,stop_valid?DSR_SC_BLUE:DSR_SC_OFF,2);center(dsr_sc_text(stop,"Stop",2,36,60,&dsr_roboto_14,stop_valid?DSR_SC_TEXT:DSR_SC_OFF));
 if(!stop_valid)lv_obj_add_state(stop,LV_STATE_DISABLED);
 lv_obj_t *controls=dsr_sc_panel(root,12,345,296,64,DSR_SC_CARD,14);
 for(unsigned i=0;i<2;i++){
  lv_obj_t *b=dsr_sc_button(controls,(int)i*148,0,148,64,DSR_SC_CARD,14,cover_control,(void *)(uintptr_t)(selected*2+(i==0)));
  lv_obj_t *circle=dsr_sc_panel(b,10,10,44,44,valid?0xe8ecef:0x555b60,LV_RADIUS_CIRCLE);
  dsr_sc_icon_centered(circle,i?DSR_ICON_DOWN:DSR_ICON_UP,28,valid?DSR_SC_BLUE:DSR_SC_OFF);
  lv_obj_t *label=dsr_sc_text(b,i?"Zamknij":"Otwórz",60,22,82,&dsr_roboto_16,valid?DSR_SC_TEXT:DSR_SC_OFF);truncate(label,22);
  if(!valid)lv_obj_add_state(b,LV_STATE_DISABLED);
 }
 dsr_sc_panel(controls,147,12,1,40,DSR_SC_EDGE,0);
 if(!model.demo){const char *footer=d->error!=DSR_OK?"Ruch zablokowany":!d->online?"Brak aktualnego odczytu":d->cover_errors||d->cover_state==DSR_COVER_CALIBRATING?"Ruch zablokowany":!d->can_cover?"Brak obsługi Cover RPC":!d->can_position?"Pozycjonowanie niedostępne":NULL;if(footer)center(dsr_sc_text(root,footer,12,415,296,&dsr_roboto_11,0xe1bb70));}
}
static bool light_enabled(const dsr_mobile_device_t *d)
{return !model.demo&&(d->endpoint_kind==DSR_ENDPOINT_LIGHT||d->endpoint_kind==DSR_ENDPOINT_RGB)&&d->online&&d->error==DSR_OK&&!d->busy&&d->can_light&&actions.set_brightness;}
static uint8_t brightness(const dsr_mobile_device_t *d)
{return d->endpoint_kind==DSR_ENDPOINT_RGB?d->rgb.brightness:d->light.brightness;}
static void light_drag_event(lv_event_t *event)
{
 if(selected>=model.device_count)return;
 dsr_mobile_device_t *d=&model.devices[selected];lv_event_code_t code=lv_event_get_code(event);
 if(code==LV_EVENT_PRESS_LOST||!light_enabled(d)){light_drag=false;if(light_preview_label)lv_label_set_text(light_preview_label,"");if(light_slider)lv_slider_set_value(light_slider,brightness(d),LV_ANIM_OFF);return;}
 if(code==LV_EVENT_PRESSED){light_drag=true;light_drag_id=d->id;}
 if(!light_drag||light_drag_id!=d->id)return;
 int value=lv_slider_get_value(lv_event_get_target(event));if(value<0||value>100){light_drag=false;return;}
 if(light_preview_label){char text[48];snprintf(text,sizeof(text),"%s %d%%",dsr_sc_tr("Podgląd"),value);lv_label_set_text(light_preview_label,text);}
 if(code==LV_EVENT_RELEASED){
  light_drag=false;if(light_preview_label)lv_label_set_text(light_preview_label,"");
  lv_indev_t *input=lv_indev_active();lv_point_t point;lv_area_t area;lv_obj_get_coords(lv_event_get_target(event),&area);
  if(!input)return;
  lv_indev_get_point(input,&point);
  if(point.x<area.x1-12||point.x>area.x2+12||point.y<area.y1-12||point.y>area.y2+12){lv_slider_set_value(light_slider,brightness(d),LV_ANIM_OFF);return;}
  actions.set_brightness(selected,(uint8_t)value);
 }
}
static void rgb_view(lv_event_t *event);
static void light_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];bool readable=d->online&&d->error==DSR_OK;char text[80];
 lv_obj_t *panel=dsr_sc_panel(root,12,89,296,256,DSR_SC_CARD,10);
 dsr_sc_text(panel,d->endpoint_kind==DSR_ENDPOINT_RGB?"Jasność • RGB RPC":"Jasność • Light RPC",12,12,272,&dsr_roboto_16,DSR_SC_TEXT);
 const char *state=unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):!d->online?"Offline • stan niepotwierdzony":d->busy?"Wysyłanie polecenia…":(d->endpoint_kind==DSR_ENDPOINT_RGB?(d->rgb.flags&DSR_RGB_ERROR)!=0:d->light.conditions||d->light.errors)?"Sterowanie światłem zablokowane":d->output?"Włączone • Online":"Wyłączone • Online";
 center(dsr_sc_text(panel,state,8,42,280,&dsr_roboto_12,DSR_SC_MUTED));
 if(readable&&(d->endpoint_kind==DSR_ENDPOINT_RGB?(d->rgb.flags&DSR_RGB_VALID):(d->light.flags&DSR_LIGHT_BRIGHTNESS)))snprintf(text,sizeof(text),"%u%%",brightness(d));else snprintf(text,sizeof(text),"—");
 center(dsr_sc_text(panel,text,8,70,280,&dsr_roboto_28,DSR_SC_TEXT));
 light_preview_label=dsr_sc_text(panel,"",8,106,280,&dsr_roboto_14,DSR_SC_BLUE);center(light_preview_label);
 light_slider=lv_slider_create(panel);lv_obj_set_pos(light_slider,24,145);lv_obj_set_size(light_slider,248,24);lv_slider_set_range(light_slider,0,100);lv_slider_set_value(light_slider,readable?brightness(d):0,LV_ANIM_OFF);
 lv_obj_set_ext_click_area(light_slider,12);lv_obj_set_style_bg_color(light_slider,dsr_sc_color(DSR_SC_OFF),LV_PART_KNOB|LV_STATE_DISABLED);lv_obj_set_style_bg_color(light_slider,dsr_sc_color(DSR_SC_OFF),LV_PART_INDICATOR|LV_STATE_DISABLED);
 lv_obj_remove_flag(light_slider,LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
 lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_PRESSED,NULL);lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_VALUE_CHANGED,NULL);lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_RELEASED,NULL);lv_obj_add_event_cb(light_slider,light_drag_event,LV_EVENT_PRESS_LOST,NULL);
 if(!light_enabled(d))lv_obj_add_state(light_slider,LV_STATE_DISABLED);
 if(readable&&d->endpoint_kind==DSR_ENDPOINT_RGB&&(d->rgb.flags&DSR_RGB_TRANSITION))snprintf(text,sizeof(text),"%s",dsr_sc_tr("Przejście • poczekaj na odczyt"));else if(readable&&d->endpoint_kind==DSR_ENDPOINT_LIGHT&&(d->light.flags&DSR_LIGHT_TRANSITION))snprintf(text,sizeof(text),"%s %u%%",dsr_sc_tr("Przejście do"),d->light.target);else snprintf(text,sizeof(text),"%s",dsr_sc_tr("0% jasności nie oznacza OFF"));
 center(dsr_sc_text(panel,text,8,184,280,&dsr_roboto_12,DSR_SC_MUTED));
 sample_age=dsr_sc_text(panel,"",8,226,280,&dsr_roboto_11,DSR_SC_MUTED);center(sample_age);update_sample_age();
 lv_obj_t *controls=dsr_sc_panel(root,12,355,296,62,DSR_SC_CARD,10);power_button(controls,selected,12,6,48);
 if(d->endpoint_kind==DSR_ENDPOINT_RGB){lv_obj_t *color=dsr_sc_button(controls,68,7,130,48,DSR_SC_PANEL,10,rgb_view,NULL);center(dsr_sc_text(color,"Kolor RGB",4,15,122,&dsr_roboto_14,DSR_SC_TEXT));}
 else center(dsr_sc_text(controls,d->output?"Wyłącz":"Włącz",68,22,132,&dsr_roboto_14,DSR_SC_TEXT));
 lv_obj_t *read=dsr_sc_button(controls,206,7,82,48,DSR_SC_PANEL,10,refresh,NULL);center(dsr_sc_text(read,"Odśwież stan",4,8,74,&dsr_roboto_12,DSR_SC_TEXT));if(d->busy||!d->can_refresh)lv_obj_add_state(read,LV_STATE_DISABLED);
}
static bool rgb_enabled(const dsr_mobile_device_t *d)
{return d->endpoint_kind==DSR_ENDPOINT_RGB&&!model.demo&&d->online&&d->error==DSR_OK&&!d->busy&&d->can_rgb&&actions.set_rgb;}
static void rgb_view(lv_event_t *event)
{(void)event;rgb_color_view=!rgb_color_view;rgb_dirty=false;render();}
static void rgb_preview(void)
{
 bool readable=selected<model.device_count&&model.devices[selected].online&&model.devices[selected].error==DSR_OK&&(model.devices[selected].rgb.flags&DSR_RGB_VALID);
 if(rgb_swatch)lv_obj_set_style_bg_color(rgb_swatch,readable?lv_color_hex(((uint32_t)rgb_draft[0]<<16)|((uint32_t)rgb_draft[1]<<8)|rgb_draft[2]):dsr_sc_color(DSR_SC_OFF),0);
 for(unsigned i=0;i<3;i++)if(rgb_values[i]){char text[24];if(readable)snprintf(text,sizeof(text),"%c  %u","RGB"[i],rgb_draft[i]);else snprintf(text,sizeof(text),"%c  —","RGB"[i]);lv_label_set_text(rgb_values[i],text);}
 if(rgb_apply_button){if(rgb_dirty&&selected<model.device_count&&rgb_enabled(&model.devices[selected]))lv_obj_remove_state(rgb_apply_button,LV_STATE_DISABLED);else lv_obj_add_state(rgb_apply_button,LV_STATE_DISABLED);}
}
static void rgb_color_event(lv_event_t *event)
{
 if(selected>=model.device_count||!rgb_enabled(&model.devices[selected]))return;
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(i>2)return;
 int value=lv_slider_get_value(lv_event_get_target(event));if(value<0||value>255)return;
 if(lv_event_get_code(event)==LV_EVENT_PRESS_LOST){
  const dsr_rgb_status_t *c=&model.devices[selected].rgb;const uint8_t actual[]={c->r,c->g,c->b};rgb_draft[i]=actual[i];lv_slider_set_value(lv_event_get_target(event),actual[i],LV_ANIM_OFF);rgb_dirty=memcmp(rgb_draft,actual,3)!=0;
 }else {rgb_draft[i]=(uint8_t)value;rgb_dirty=true;}
 rgb_draft_id=model.devices[selected].id;rgb_preview();
}
static void rgb_apply(lv_event_t *event)
{
 (void)event;if(!rgb_dirty||selected>=model.device_count||rgb_draft_id!=model.devices[selected].id||!rgb_enabled(&model.devices[selected]))return;
 uint8_t r=rgb_draft[0],g=rgb_draft[1],b=rgb_draft[2];rgb_dirty=false;actions.set_rgb(selected,r,g,b);
}
static void rgb_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];bool readable=d->online&&d->error==DSR_OK&&(d->rgb.flags&DSR_RGB_VALID);
 rgb_draft[0]=readable?d->rgb.r:0;rgb_draft[1]=readable?d->rgb.g:0;rgb_draft[2]=readable?d->rgb.b:0;rgb_draft_id=d->id;
 lv_obj_t *panel=dsr_sc_panel(root,12,89,296,256,DSR_SC_CARD,10);
 dsr_sc_text(panel,"Kolor • RGB RPC",12,10,272,&dsr_roboto_16,DSR_SC_TEXT);
 rgb_swatch=dsr_sc_panel(panel,12,40,272,24,0,6);
 for(unsigned i=0;i<3;i++){
  rgb_values[i]=dsr_sc_text(panel,"",12,74+(int)i*51,272,&dsr_roboto_12,DSR_SC_TEXT);
  lv_obj_t *slider=lv_slider_create(panel);lv_obj_set_pos(slider,24,98+(int)i*51);lv_obj_set_size(slider,248,18);lv_slider_set_range(slider,0,255);lv_slider_set_value(slider,rgb_draft[i],LV_ANIM_OFF);lv_obj_set_ext_click_area(slider,12);
  lv_obj_add_event_cb(slider,rgb_color_event,LV_EVENT_VALUE_CHANGED,(void *)(uintptr_t)i);lv_obj_add_event_cb(slider,rgb_color_event,LV_EVENT_PRESS_LOST,(void *)(uintptr_t)i);if(!rgb_enabled(d))lv_obj_add_state(slider,LV_STATE_DISABLED);
 }
 sample_age=dsr_sc_text(panel,readable?((d->rgb.flags&DSR_RGB_TRANSITION)?"Przejście • poczekaj na odczyt":"Podgląd • Zastosuj wysyła kolor"):"Brak aktualnego odczytu",8,225,280,&dsr_roboto_11,DSR_SC_MUTED);center(sample_age);sample_age=NULL;
 lv_obj_t *controls=dsr_sc_panel(root,12,355,296,62,DSR_SC_CARD,10);power_button(controls,selected,12,6,48);
 lv_obj_t *level=dsr_sc_button(controls,68,7,130,48,DSR_SC_PANEL,10,rgb_view,NULL);center(dsr_sc_text(level,"Jasność",4,15,122,&dsr_roboto_14,DSR_SC_TEXT));
 rgb_apply_button=dsr_sc_button(controls,206,7,82,48,DSR_SC_BLUE,10,rgb_apply,NULL);center(dsr_sc_text(rgb_apply_button,"Zastosuj",4,15,74,&dsr_roboto_12,DSR_SC_TEXT));rgb_preview();
}
static void ht_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];char text[80];
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
 sample_age=dsr_sc_text(panel,"",8,229,280,&dsr_roboto_11,DSR_SC_MUTED);center(sample_age);update_sample_age();
 lv_obj_t *button=dsr_sc_button(root,12,344,296,48,DSR_SC_CARD,10,refresh,NULL);center(dsr_sc_text(button,"Odśwież status",8,15,280,&dsr_roboto_14,DSR_SC_TEXT));if(d->busy||!d->can_refresh||!actions.refresh)lv_obj_add_state(button,LV_STATE_DISABLED);
 dsr_sc_text(root,"Czas pomiaru nieznany. Czujnik usypia także na USB. Wybudź przyciskiem, jeśli nie odpowiada.",16,396,288,&dsr_roboto_11,DSR_SC_MUTED);
}
/* Shelly BLU read through a gateway: state, measurements, age of the last
 * BLE packet the gateway received. Read-only; refresh re-reads the gateway. */
/* BLU TRV, Shelly Smart Control style: -/+ change a local target per device
 * and it is sent 1.2 s after the last tap (one BLE write per adjustment).
 * Big values elsewhere are what the gateway reports, never the request. */
#define TRV_SEND_DELAY_MS 1200u
static struct {uint32_t id,due;uint8_t half;bool dirty;} trv_local[16];
static lv_timer_t *trv_timer;
static bool trv_device(const dsr_mobile_device_t *d){return d->endpoint_kind==DSR_ENDPOINT_BLU&&(d->blu.errors&DSR_BLU_TRV);}
static bool trv_known(const dsr_mobile_device_t *d){return d->online&&d->has_trv_target;}
static uint8_t trv_reported(const dsr_mobile_device_t *d){return trv_known(d)?(uint8_t)(d->trv_target_c*2.0+0.5):40u;}
static int trv_slot(uint32_t id){for(unsigned i=0;i<16;i++)if(trv_local[i].dirty&&trv_local[i].id==id)return (int)i;return -1;}
/* What the dial and the pill show: the unsent local target, else the gateway's. */
static uint8_t trv_shown(const dsr_mobile_device_t *d){int at=trv_slot(d->id);return at>=0?trv_local[at].half:trv_reported(d);}
static bool trv_can(const dsr_mobile_device_t *d){return trv_known(d)&&!d->busy&&!unbound(d)&&(model.demo||actions.set_trv);}
static void trv_send(lv_timer_t *timer)
{
 (void)timer;uint32_t now=lv_tick_get();
 for(unsigned i=0;i<16;i++){
  if(!trv_local[i].dirty||(int32_t)(now-trv_local[i].due)<0)continue;
  trv_local[i].dirty=false;unsigned index=0;while(index<model.device_count&&model.devices[index].id!=trv_local[i].id)index++;
  if(index>=model.device_count)continue;
  dsr_mobile_device_t *d=&model.devices[index];
  if(model.demo){d->trv_target_c=trv_local[i].half/2.0;d->has_trv_target=true;render();}
  else if(trv_can(d)&&trv_local[i].half!=trv_reported(d))actions.set_trv(index,trv_local[i].half);
  else render();
 }
}
static void trv_step(lv_event_t *event)
{
 uintptr_t value=(uintptr_t)lv_event_get_user_data(event);unsigned index=(unsigned)(value/2);int step=value%2?1:-1;
 if(index>=model.device_count||!trv_can(&model.devices[index]))return;
 const dsr_mobile_device_t *d=&model.devices[index];int at=trv_slot(d->id);
 if(at<0){for(unsigned i=0;i<16&&at<0;i++)if(!trv_local[i].dirty)at=(int)i;if(at<0)return;trv_local[at].id=d->id;trv_local[at].half=trv_reported(d);}
 int next=(int)trv_local[at].half+step;
 if(next<(int)DSR_TRV_MIN_HALF)next=(int)DSR_TRV_MIN_HALF;
 if(next>(int)DSR_TRV_MAX_HALF)next=(int)DSR_TRV_MAX_HALF;
 trv_local[at].half=(uint8_t)next;trv_local[at].dirty=true;trv_local[at].due=lv_tick_get()+TRV_SEND_DELAY_MS;
 if(!trv_timer)trv_timer=lv_timer_create(trv_send,100,NULL);
 render();
}
static void trv_round(lv_obj_t *parent,unsigned index,int x,int y,int size,bool plus,bool valid)
{
 lv_obj_t *b=dsr_sc_button(parent,x,y,size,size,0xe8ecef,LV_RADIUS_CIRCLE,trv_step,(void *)(uintptr_t)(index*2+(plus?1:0)));
 lv_obj_t *t=dsr_sc_text(b,plus?"+":"-",0,(size-26)/2,size,&dsr_roboto_22,plus?0xe5322d:DSR_SC_BLUE);center(t);
 if(!valid){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_color(b,dsr_sc_color(0x555b60),LV_STATE_DISABLED);}
}
static int chip_width(lv_obj_t *chip){lv_obj_update_layout(chip);return lv_obj_get_width(chip);}
static void half_text(char *out,size_t size,uint8_t half){snprintf(out,size,"%u.%u °C",(unsigned)half/2u,(unsigned)(half%2u)*5u);}
/* Arc 4..30 C over 270 degrees (LVGL: 0 = 3 o'clock, clockwise), blue to white to red. */
static uint32_t trv_color(unsigned i,unsigned n)
{
 static const uint8_t from[3][3]={{0x2f,0x6b,0xff},{0xee,0xee,0xf2},{0xe5,0x32,0x2d}};
 double t=(double)i/(double)(n-1)*2.0;unsigned k=t>=1.0?1u:0u;double f=t-k;uint32_t c=0;
 for(unsigned ch=0;ch<3;ch++)c=c<<8|(uint32_t)(from[k][ch]+(from[k+1][ch]-from[k][ch])*f+0.5);
 return c;
}
static void trv_details(lv_obj_t *root)
{
 dsr_mobile_device_t *d=&model.devices[selected];char text[96];const dsr_blu_status_t *b=&d->blu;bool live=d->online,known=trv_known(d),can=trv_can(d);
 uint8_t half=trv_shown(d);
 /* Value row, as in the app: room, target, valve, battery. */
 int x=12;char chip[32];
 if(known)snprintf(chip,sizeof(chip),"%.1f °C",d->temperature_c);else snprintf(chip,sizeof(chip),"— °C");x+=chip_width(dsr_sc_chip(root,chip,x,93,80,DSR_SC_TEXT))+6;
 if(known)snprintf(chip,sizeof(chip),dsr_sc_tr("cel %.1f °C"),d->trv_target_c);else snprintf(chip,sizeof(chip),"%s",dsr_sc_tr("cel — °C"));x+=chip_width(dsr_sc_chip(root,chip,x,93,96,DSR_SC_TEXT))+6;
 if(known)snprintf(chip,sizeof(chip),dsr_sc_tr("zawór %u%%"),(unsigned)b->trv_pos);else snprintf(chip,sizeof(chip),"%s",dsr_sc_tr("zawór —"));x+=chip_width(dsr_sc_chip(root,chip,x,93,80,DSR_SC_TEXT))+6;
 if(live&&d->has_battery)snprintf(chip,sizeof(chip),"%.0f%%",d->battery_pct);else snprintf(chip,sizeof(chip),"—%%");
 lv_obj_t *battery=dsr_sc_panel(root,x,93,320-12-x,20,0x292d30,6);dsr_sc_icon(battery,DSR_ICON_BATTERY,5,2,16,DSR_SC_TEXT);dsr_sc_text(battery,chip,24,3,320-12-x-26,&dsr_roboto_12,DSR_SC_TEXT);
 const int cx=160,cy=232,r=88,n=18;
 for(int i=0;i<n;i++){
  lv_obj_t *seg=lv_arc_create(root);lv_obj_remove_flag(seg,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_size(seg,2*r,2*r);lv_obj_set_pos(seg,cx-r,cy-r);
  int start=135+270*i/n,end=135+270*(i+1)/n+1;lv_arc_set_bg_angles(seg,(lv_value_precise_t)(start%360),(lv_value_precise_t)(end%360));
  lv_obj_set_style_arc_width(seg,12,LV_PART_MAIN);lv_obj_set_style_arc_color(seg,dsr_sc_color(live?trv_color((unsigned)i,(unsigned)n):0x4a5056),LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(seg,i==0||i==n-1,LV_PART_MAIN);lv_obj_set_style_arc_opa(seg,LV_OPA_TRANSP,LV_PART_INDICATOR);lv_obj_set_style_bg_opa(seg,LV_OPA_TRANSP,LV_PART_KNOB);lv_obj_set_style_pad_all(seg,0,LV_PART_KNOB);
 }
 if(known){
  double angle=(135.0+270.0*(half-DSR_TRV_MIN_HALF)/(double)(DSR_TRV_MAX_HALF-DSR_TRV_MIN_HALF))*3.14159265358979/180.0;
  int kx=cx+(int)lround((r-6)*cos(angle)),ky=cy+(int)lround((r-6)*sin(angle));
  lv_obj_t *knob=dsr_sc_panel(root,kx-13,ky-13,26,26,0xe8ecef,LV_RADIUS_CIRCLE);edge(knob,0x1b1f22,2);
 }
 if(known)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"— °C");
 center(dsr_sc_text(root,text,cx-60,160,120,&dsr_roboto_16,DSR_SC_TEXT));
 center(dsr_sc_text(root,"AKTUALNA\nTEMPERATURA",cx-60,182,120,&dsr_roboto_11,DSR_SC_MUTED));
 if(known)half_text(text,sizeof(text),half);else snprintf(text,sizeof(text),"— °C");
 center(dsr_sc_text(root,dsr_sc_user(text),cx-80,212,160,&dsr_roboto_28,DSR_SC_TEXT));
 trv_round(root,selected,cx-54,258,48,false,can);trv_round(root,selected,cx+6,258,48,true,can);
 dsr_sc_text(root,"4 °C",cx-r-8,296,40,&dsr_roboto_12,DSR_SC_TEXT);dsr_sc_text(root,"30 °C",cx+r-30,296,44,&dsr_roboto_12,DSR_SC_TEXT);
 bool sending=trv_slot(d->id)>=0||(d->busy&&d->command_pending&&live);
 const char *state=!live&&(d->error==DSR_OK||d->error==DSR_OFFLINE)?"Bramka nie odpowiada":d->error!=DSR_OK?device_error(d->error):sending?"Wysyłanie do termostatu przez bramkę…":"Odczyt z bramki odebrany";
 center(dsr_sc_text(root,state,12,326,296,&dsr_roboto_12,d->error!=DSR_OK||!live?0xffc18c:DSR_SC_MUTED));
 if(live&&(b->errors&DSR_BLU_TRV_FAULT))center(dsr_sc_text(root,"Termostat zgłasza błąd (np. brak kalibracji)",12,346,296,&dsr_roboto_12,0xffc18c));
 sample_age=dsr_sc_text(root,"",12,366,296,&dsr_roboto_11,DSR_SC_MUTED);center(sample_age);update_sample_age();
 dsr_sc_text(root,"Pilot wysyła nastawę do bramki Shelly, a ta do termostatu przez Bluetooth.",16,396,288,&dsr_roboto_11,DSR_SC_MUTED);
}
static void blu_details(lv_obj_t *root)
{
 const dsr_mobile_device_t *d=&model.devices[selected];char text[96];const dsr_blu_status_t *b=&d->blu;
 lv_obj_t *panel=dsr_sc_panel(root,12,89,296,246,DSR_SC_CARD,10);
 dsr_sc_text(panel,"Shelly BLU • przez bramkę",12,10,272,&dsr_roboto_16,DSR_SC_TEXT);
 /* Background polls keep the last values; only a reading with nothing current says so. */
 const char *state=d->busy&&!d->online?"Odczyt stanu…":!d->online&&(d->error==DSR_OK||d->error==DSR_OFFLINE)?"Bramka nie odpowiada":d->error!=DSR_OK?device_error(d->error):d->online?"Odczyt z bramki odebrany":"Bramka nie odpowiada";
 dsr_sc_text(panel,state,12,40,272,&dsr_roboto_12,DSR_SC_MUTED);
 bool live=d->online;
 const char *main_text=!live?"—":(b->flags&DSR_BLU_HAS_OPEN)?((b->flags&DSR_BLU_OPEN)?"Otwarte":"Zamknięte"):(b->flags&DSR_BLU_HAS_MOTION)?((b->flags&DSR_BLU_MOTION)?"Ruch wykryty":"Brak ruchu"):NULL;
 if(main_text)center(dsr_sc_text(panel,main_text,8,69,280,&dsr_roboto_28,DSR_SC_TEXT));
 else{if(d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);else snprintf(text,sizeof(text),"—");center(dsr_sc_text(panel,text,8,69,280,&dsr_roboto_28,DSR_SC_TEXT));}
 text[0]=0;
 if(live&&d->has_illuminance)snprintf(text,sizeof(text),"%.0f lx",d->illuminance_lux);
 if(live&&main_text&&d->has_temperature)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.1f °C",text[0]?" • ":"",d->temperature_c);
 if(live&&d->has_humidity)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f%% RH",text[0]?" • ":"",d->humidity_pct);
 if(live&&(b->flags&DSR_BLU_HAS_BUTTON))snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%s %u",text[0]?" • ":"",dsr_sc_tr("Przycisk"),(unsigned)b->button);
 center(dsr_sc_text(panel,text[0]?text:"—",8,112,280,&dsr_roboto_16,DSR_SC_TEXT));
 if(live&&d->has_battery)snprintf(text,sizeof(text),"%s %.0f%%",dsr_sc_tr("Bateria"),d->battery_pct);else snprintf(text,sizeof(text),"%s —",dsr_sc_tr("Bateria"));
 if(live&&(b->flags&DSR_BLU_HAS_RSSI))snprintf(text+strlen(text),sizeof(text)-strlen(text)," • %d dBm",(int)b->rssi);
 center(dsr_sc_text(panel,text,8,144,280,&dsr_roboto_14,DSR_SC_TEXT));
 if(live&&d->blu_age_s>=0){unsigned age=(unsigned)d->blu_age_s;
  if(age<120)snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u s temu"),age);else if(age<7200)snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u min temu"),age/60);else snprintf(text,sizeof(text),dsr_sc_tr("Ostatni sygnał czujnika %u h temu"),age/3600);}
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Czas ostatniego sygnału nieznany"));
 center(dsr_sc_text(panel,dsr_sc_user(text),8,172,280,&dsr_roboto_12,DSR_SC_MUTED));
 if(live&&(b->errors&DSR_BLU_ERROR_MASK))center(dsr_sc_text(panel,(b->errors&(DSR_BLU_ERROR_KEY|DSR_BLU_ERROR_DECRYPT))?"Bramka: brak lub błędny klucz szyfrowania":"Bramka: błąd danych BTHome",8,194,280,&dsr_roboto_12,0xffc18c));
 sample_age=dsr_sc_text(panel,"",8,229,280,&dsr_roboto_11,DSR_SC_MUTED);center(sample_age);update_sample_age();
 lv_obj_t *button=dsr_sc_button(root,12,344,296,48,DSR_SC_CARD,10,refresh,NULL);center(dsr_sc_text(button,"Odśwież status",8,15,280,&dsr_roboto_14,DSR_SC_TEXT));if(d->busy||!d->can_refresh||!actions.refresh)lv_obj_add_state(button,LV_STATE_DISABLED);
 dsr_sc_text(root,"Dane z bramki Shelly. Pilot nie łączy się z czujnikiem i niczym w nim nie steruje.",16,396,288,&dsr_roboto_11,DSR_SC_MUTED);
}
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
    lv_obj_t *panel=dsr_sc_panel(root,12,89,296,256,DSR_SC_CARD,10);
    dsr_sc_text(panel,"Główne sterowanie",13,12,270,&dsr_roboto_16,DSR_SC_TEXT);
    const char *status=unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):!d->online?"Offline • stan niepotwierdzony":d->busy?(d->command_pending?"Wysyłanie polecenia…":"Odczyt stanu…"):d->output?"Włączone • Online":"Wyłączone • Online";
    center(dsr_sc_text(panel,status,8,41,280,&dsr_roboto_12,d->error!=DSR_OK?0xffc18c:d->online?DSR_SC_TEXT:DSR_SC_MUTED));
    char power[40];if(d->online&&d->error==DSR_OK&&d->has_power&&!d->busy)snprintf(power,sizeof(power),"%.1f W",d->power_w);else snprintf(power,sizeof(power),"—");
    center(dsr_sc_text(panel,power,8,70,280,&dsr_roboto_28,DSR_SC_TEXT));
    center(dsr_sc_text(panel,d->has_power?"Moc chwilowa":"Pomiar mocy niedostępny",8,103,280,&dsr_roboto_12,DSR_SC_MUTED));
    power_button(panel,selected,105,130,86);
    if(actions.refresh){lv_obj_t *button=dsr_sc_button(panel,202,148,82,48,DSR_SC_PANEL,10,refresh,NULL);lv_obj_t *label=dsr_sc_text(button,"Odśwież stan",4,8,74,&dsr_roboto_12,DSR_SC_TEXT);center(label);if(d->busy||!d->can_refresh)lv_obj_add_state(button,LV_STATE_DISABLED);}
    center(dsr_sc_text(panel,unbound(d)?"Dodaj urządzenie ponownie przyciskiem +":d->error!=DSR_OK?"Odśwież, aby potwierdzić stan":!d->online?"Brak połączenia":d->busy?"Oczekiwanie…":d->output?"Wyłącz":"Włącz",8,220,280,&dsr_roboto_14,d->online?DSR_SC_BLUE:DSR_SC_MUTED));
    sample_age=dsr_sc_text(panel,"",8,242,280,&dsr_roboto_11,DSR_SC_MUTED);center(sample_age);update_sample_age();
    char voltage[32],current[32];
    if(d->online&&d->error==DSR_OK&&d->has_voltage)snprintf(voltage,sizeof(voltage),"%.1f V",d->voltage_v);else snprintf(voltage,sizeof(voltage),"—");
    if(d->online&&d->error==DSR_OK&&d->has_current)snprintf(current,sizeof(current),"%.3f A",d->current_a);else snprintf(current,sizeof(current),"—");
    metric(root,"Napięcie",voltage,12);metric(root,"Natężenie",current,164);
}
static void device_settings(lv_obj_t *root)
{
 if(selected>=model.device_count)return;
 const dsr_mobile_device_t *d=&model.devices[selected];char text[160];
 content=dsr_sc_panel(root,12,96,296,246,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);
 lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);
 lv_obj_t *identity=dsr_sc_panel(content,0,0,296,154,DSR_SC_CARD,14);
 dsr_sc_icon(identity,DSR_ICON_INFO,12,13,24,DSR_SC_BLUE);dsr_sc_text(identity,"Wybrane urządzenie",48,17,236,&dsr_roboto_16,DSR_SC_TEXT);
 dsr_sc_text(identity,unbound(d)?"Brak powiązania • dodaj ponownie":d->error!=DSR_OK?device_error(d->error):d->online?"Online":"Brak połączenia",12,44,272,&dsr_roboto_14,d->online&&d->error==DSR_OK?DSR_SC_BLUE:DSR_SC_MUTED);
 lv_obj_t *model_name=dsr_sc_text(identity,d->model[0]?d->model:"Model nieodczytany",12,68,272,&dsr_roboto_14,DSR_SC_MUTED);truncate(model_name,20);
 snprintf(text,sizeof(text),dsr_sc_tr("Adres karty: %s • kanał %u"),d->host[0]?d->host:"—",d->channel);dsr_sc_text(identity,text,12,90,272,&dsr_roboto_12,DSR_SC_TEXT);
 const char *family=d->identity[0]?(d->generation==DSR_GEN1?"Gen1":d->generation==DSR_GEN2_PLUS?"Gen2+":"—"):"—";
 snprintf(text,sizeof(text),"%s • %s",dsr_sc_tr("Zapisane ID"),family);dsr_sc_text(identity,text,12,110,272,&dsr_roboto_11,DSR_SC_MUTED);
 lv_obj_t *id=dsr_sc_text(identity,d->identity[0]?d->identity:"Tożsamość niepotwierdzona",12,130,272,&dsr_roboto_12,DSR_SC_TEXT);truncate(id,20);
 lv_obj_t *readings=dsr_sc_panel(content,0,164,296,78,DSR_SC_PANEL,12);
 dsr_sc_text(readings,"Dostępne pomiary",12,11,272,&dsr_roboto_14,DSR_SC_TEXT);
 text[0]=0;bool valid=d->online&&d->error==DSR_OK;
 if(d->endpoint_kind==DSR_ENDPOINT_BLU){if(d->has_contact)snprintf(text,sizeof(text),"%s",dsr_sc_tr(d->contact_open?"Otwarte":"Zamknięte"));else if(d->blu.flags&DSR_BLU_HAS_MOTION)snprintf(text,sizeof(text),"%s",dsr_sc_tr((d->blu.flags&DSR_BLU_MOTION)?"Ruch wykryty":"Brak ruchu"));if(d->has_illuminance)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f lx",text[0]?" • ":"",d->illuminance_lux);if(d->has_battery)snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.0f%%",text[0]?" • ":"",d->battery_pct);valid=false;}
 if(d->endpoint_kind==DSR_ENDPOINT_HT){if(d->has_temperature)snprintf(text,sizeof(text),"%.1f °C",d->temperature_c);if(d->has_humidity)snprintf(text+strlen(text),sizeof(text)-strlen(text)," • %.1f%% RH",d->humidity_pct);if(d->has_battery)snprintf(text+strlen(text),sizeof(text)-strlen(text)," • %.1f%%",d->battery_pct);valid=false;}
 if(valid&&d->has_power&&isfinite(d->power_w))snprintf(text,sizeof(text),"%.1f W",d->power_w);
 if(valid&&d->has_voltage&&isfinite(d->voltage_v))snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.1f V",text[0]?" • ":"",d->voltage_v);
 if(valid&&d->has_current&&isfinite(d->current_a))snprintf(text+strlen(text),sizeof(text)-strlen(text),"%s%.3f A",text[0]?" • ":"",d->current_a);
 dsr_sc_text(readings,text[0]?text:!d->can_refresh?"Brak aktywnego połączenia":valid?"Brak obsługiwanych pomiarów":"Odczyty niedostępne",12,37,272,&dsr_roboto_14,DSR_SC_MUTED);
 sample_age=dsr_sc_text(readings,"",12,58,272,&dsr_roboto_11,DSR_SC_MUTED);update_sample_age();
 lv_obj_t *update=dsr_sc_button(root,12,354,142,48,DSR_SC_CARD,12,refresh,NULL);center(dsr_sc_text(update,"Odśwież status",4,16,134,&dsr_roboto_14,DSR_SC_TEXT));
 if(!d->can_refresh||!actions.refresh||d->busy)lv_obj_add_state(update,LV_STATE_DISABLED);
 lv_obj_t *edit=dsr_sc_button(root,166,354,142,48,DSR_SC_CARD,12,edit_device,NULL);center(dsr_sc_text(edit,"Edytuj kartę",4,16,134,&dsr_roboto_14,DSR_SC_TEXT));
 if(model.demo||!d->id||!actions.edit_device)lv_obj_add_state(edit,LV_STATE_DISABLED);
 dsr_sc_text(root,"Ustawienia karty • lokalnie",12,410,296,&dsr_roboto_11,DSR_SC_MUTED);
 dsr_sc_text(content,"Nazwa i pokój dotyczą pilota. Konfiguracja samego Shelly wymaga obsługi API danego modelu.",4,254,288,&dsr_roboto_12,DSR_SC_MUTED);
 dsr_sc_text(content,"Historia 1 h / 24 h jest w zakładce Wykresy. Oś czasu UTC.",4,312,288,&dsr_roboto_12,DSR_SC_MUTED);
}
static void render(void)
{
    rgb_dirty=false;rgb_swatch=rgb_apply_button=NULL;memset(rgb_values,0,sizeof(rgb_values));cover_drag=false;cover_preview_label=NULL;light_drag=false;light_slider=light_preview_label=NULL;int scroll=content?lv_obj_get_scroll_y(content):0;
    lv_obj_t *root=lv_screen_active();content=sample_age=info_overlay=NULL;lv_obj_clean(root);lv_obj_remove_style_all(root);
    dsr_sc_background(root);
    lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
    static const char fallback_title[]="Mój dom";
    const char *title=(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS)&&selected<model.device_count?model.devices[selected].name:
        page==DSR_MOBILE_ROOM&&selected<model.room_count?model.rooms[selected].name:model.home_name[0]?model.home_name:fallback_title;
    if(page==DSR_MOBILE_DETAILS&&selected<model.device_count&&model.devices[selected].kind==DSR_DEVICE_COVER)cover_header(root,title);
    else header(root,title==fallback_title?title:dsr_sc_name(title),page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS||page==DSR_MOBILE_ROOM);
    tabs(root);
    if(page==DSR_MOBILE_DEVICE_SETTINGS){device_settings(root);if(content)lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);}
    else if(page==DSR_MOBILE_DETAILS)details(root);
    else {
        toolbar(root,page==DSR_MOBILE_HOME?(model.dashboard_name[0]?dsr_sc_name(model.dashboard_name):"Mój pulpit"):page==DSR_MOBILE_ROOMS?"Pomieszczenia":"Urządzenia");
        content=dsr_sc_panel(root,12,146,296,270,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);
        lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);
        lv_obj_set_scrollbar_mode(content,LV_SCROLLBAR_MODE_AUTO);
        if(page==DSR_MOBILE_ROOMS){
            for(unsigned i=0;i<model.room_count;i++)room_card(content,i,(int)(i%2)*152,(int)(i/2)*138,144,130);
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
                    if(tile->type==0&&tile->index<model.device_count){if(x){x=0;y+=row_height+8;row_height=0;}model.devices[tile->index].fields=tile->fields;device_card(content,tile->index,0,y,296,tile->height==2?120:84);y+=tile->height==2?128:92;at++;}
                    else if(tile->type==1&&tile->index<model.room_count){int height=tile->height==2?184:130;if(tile->span==2&&x){x=0;y+=row_height+8;row_height=0;}room_card(content,tile->index,x,y,tile->span==2?296:144,height);if(height>row_height)row_height=height;if(x||tile->span==2){x=0;y+=row_height+8;row_height=0;}else x=152;at++;}
                    else if(tile->type==2&&tile->index<model.scene_count){if(x){x=0;y+=row_height+8;row_height=0;}scene_card(tile->index,y,tile->height==2?120:90);y+=tile->height==2?128:98;at++;}
                    else if(tile->type==3&&tile->index<model.group_count){if(x){x=0;y+=row_height+8;row_height=0;}group_card(tile->index,y,tile->height==2?120:90);y+=tile->height==2?128:98;at++;}
                }
            }else for(unsigned i=0;i<model.device_count;i++)if(page!=DSR_MOBILE_ROOM||model.devices[i].room==selected){device_card(content,i,0,(int)at*92,296,84);at++;}
            if(!at)empty("Brak urządzeń","Dodaj pierwsze urządzenie Shelly przyciskiem +, aby rozpocząć sterowanie swoim domem.");
        }
        lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);
    }
    if(model.demo){lv_obj_t *caption=dsr_sc_text(root,"DEMO • dane przykładowe",12,415,296,&dsr_roboto_11,0xe1bb70);center(caption);}
    bottom(root);
    if(info_open)dashboard_info(NULL);
    rendered_language=dsr_sc_language();
    /* A horizontal metrics drag must never displace the whole screen. */
    lv_obj_update_layout(root);lv_obj_scroll_to(root,0,0,LV_ANIM_OFF);
}
void dsr_mobile_show(dsr_mobile_page_t p,unsigned index)
{
    if(p>DSR_MOBILE_DEVICE_SETTINGS)return;
    info_open=false;
    if(cover_preview&&p!=DSR_MOBILE_DETAILS)end_preview();
    if(p==DSR_MOBILE_DETAILS&&page!=DSR_MOBILE_DETAILS&&page!=DSR_MOBILE_DEVICE_SETTINGS){previous_page=page;previous_selected=selected;}
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
static void translate_labels(lv_obj_t *object)
{
 if(lv_obj_check_type(object,&lv_label_class))dsr_sc_label_set_text(object,lv_label_get_text(object));
 for(unsigned i=0;i<lv_obj_get_child_count(object);i++)translate_labels(lv_obj_get_child(object,(int)i));
}
static void translate_draft(void)
{
 if(rendered_language==dsr_sc_language())return;
 translate_labels(lv_screen_active());rendered_language=dsr_sc_language();
 if(light_drag&&light_preview_label&&light_slider){char text[48];snprintf(text,sizeof(text),"%s %d%%",dsr_sc_tr("Podgląd"),(int)lv_slider_get_value(light_slider));lv_label_set_text(light_preview_label,text);}
}
void dsr_mobile_update(const dsr_mobile_model_t *m)
{
    if(!m||m->device_count>DSR_UI_MAX_DEVICES||m->room_count>DSR_UI_MAX_ROOMS||m->tile_count>24||m->scene_count>8||m->group_count>8)return;
    if(cover_preview){preview_saved=*m;
#ifndef DSR_UI_REVIEW_DEMO
        preview_saved.demo=false;
#endif
        return;}
    if(rgb_dirty&&selected<model.device_count){
     const dsr_mobile_device_t *old=&model.devices[selected],*next=NULL;
     for(unsigned i=0;i<m->device_count;i++)if(m->devices[i].id==rgb_draft_id)next=&m->devices[i];
     if(next&&rgb_enabled(next)&&next->generation==old->generation&&next->endpoint_kind==old->endpoint_kind&&next->channel==old->channel&&!strcmp(next->identity,old->identity)&&!strcmp(next->host,old->host)&&!memcmp(&next->rgb,&old->rgb,sizeof(old->rgb))){model=*m;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==rgb_draft_id)selected=i;translate_draft();return;}
     rgb_dirty=false;
    }
    if(light_drag&&selected<model.device_count){
     const dsr_mobile_device_t *old=&model.devices[selected],*next=NULL;
     for(unsigned i=0;i<m->device_count;i++)if(m->devices[i].id==light_drag_id)next=&m->devices[i];
     if(next&&light_enabled(next)&&next->generation==old->generation&&next->endpoint_kind==old->endpoint_kind&&next->channel==old->channel&&!strcmp(next->identity,old->identity)&&!strcmp(next->host,old->host)&&(old->endpoint_kind==DSR_ENDPOINT_RGB?next->rgb.flags==old->rgb.flags:next->light.target==old->light.target&&(next->light.flags&DSR_LIGHT_TRANSITION)==(old->light.flags&DSR_LIGHT_TRANSITION))){model=*m;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==light_drag_id)selected=i;translate_draft();update_sample_age();return;}
     light_drag=false;
    }
    if(cover_drag&&selected<model.device_count){
     const dsr_mobile_device_t *old=&model.devices[selected];const dsr_mobile_device_t *next=NULL;
     for(unsigned i=0;i<m->device_count;i++)if(m->devices[i].id==cover_drag_id)next=&m->devices[i];
     if(next&&position_enabled(next)&&next->generation==old->generation&&next->endpoint_kind==old->endpoint_kind&&next->channel==old->channel&&!strcmp(next->identity,old->identity)&&!strcmp(next->host,old->host)){model=*m;for(unsigned i=0;i<model.device_count;i++)if(model.devices[i].id==cover_drag_id)selected=i;update_sample_age();return;}
     cover_drag=false;
    }
    /* Freshness changes update one label, not the entire screen or scroll state. */
    for(unsigned i=0;i<DSR_UI_MAX_DEVICES;i++){model.devices[i].has_sample_time=m->devices[i].has_sample_time;model.devices[i].sample_ms=m->devices[i].sample_ms;}
    if(!memcmp(&model,m,sizeof(model))){update_sample_age();return;}
    if(page==DSR_MOBILE_HOME&&model.dashboard_id!=m->dashboard_id)content=NULL;
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
