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
static void render(void);
static void update_sample_age(void)
{
 if(!active||!sample_age||selected>=model.device_count)return;
 const dsr_mobile_device_t *d=&model.devices[selected];char text[72];
 if(d->has_sample_time)snprintf(text,sizeof(text),dsr_sc_tr("Odczyt potwierdzony %u s temu"),(unsigned)((lv_tick_get()-d->sample_ms)/1000));
 else snprintf(text,sizeof(text),"%s",dsr_sc_tr("Brak potwierdzonego odczytu"));
 lv_label_set_text(sample_age,text);
}
static void age_tick(lv_timer_t *timer){(void)timer;update_sample_age();}

void dsr_mobile_bind(const dsr_mobile_actions_t *a){if(a)actions=*a;else memset(&actions,0,sizeof(actions));if(!age_timer)age_timer=lv_timer_create(age_tick,1000,NULL);}
bool dsr_mobile_active(void){return active;}
dsr_mobile_page_t dsr_mobile_page(void){return page;}
static void end_preview(void){if(cover_preview){model=preview_saved;memset(&preview_saved,0,sizeof(preview_saved));cover_preview=false;}}
void dsr_mobile_hide(void){cover_drag=false;cover_preview_label=NULL;end_preview();active=false;info_open=false;content=sample_age=info_overlay=NULL;}
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
    if(page==DSR_MOBILE_HOME){tab(root,"Aktywność",0,110,false,NULL,NULL);tab(root,model.dashboard_name[0]?model.dashboard_name:"Mój pulpit",114,162,true,NULL,NULL);return;}
    if(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS){
        bool settings=page==DSR_MOBILE_DEVICE_SETTINGS;
        bool editable=!cover_preview&&!model.demo&&selected<model.device_count&&model.devices[selected].id;
        tab(root,"Sterowanie",0,109,!settings,settings?navigate:NULL,(void *)(uintptr_t)15);
        tab(root,"Wykresy",111,101,false,editable&&actions.open_charts?navigate:NULL,(void *)(uintptr_t)16);
        tab(root,"Ustawienia",214,106,settings,!settings&&editable?navigate:NULL,(void *)(uintptr_t)14);return;
    }
    if(page==DSR_MOBILE_ROOM){tab(root,"Urządzenia",0,115,true,NULL,NULL);tab(root,"Grupy",117,87,false,NULL,NULL);tab(root,"Sceny",206,114,false,NULL,NULL);return;}
    tab(root,"Pomieszczenia",0,101,page==DSR_MOBILE_ROOMS,navigate,(void *)(uintptr_t)DSR_MOBILE_ROOMS);
    tab(root,"Grupy",103,55,false,NULL,NULL);tab(root,"Sceny",160,55,false,NULL,NULL);
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
{return kind==DSR_DEVICE_LIGHT?DSR_ICON_LIGHT:kind==DSR_DEVICE_COVER?DSR_ICON_COVER:kind==DSR_DEVICE_CONTACT||kind==DSR_DEVICE_SENSOR?DSR_ICON_CONTACT:DSR_ICON_PLUG;}
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
 if(!valid){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_color(b,lv_color_hex(0x555b60),LV_STATE_DISABLED);}
}
static void device_card(lv_obj_t *parent,unsigned i,int x,int y,int width,int height)
{
    dsr_mobile_device_t *d=&model.devices[i];
    bool cover=d->kind==DSR_DEVICE_COVER,sensor=d->kind==DSR_DEVICE_CONTACT||d->kind==DSR_DEVICE_SENSOR;
    unsigned fields=page==DSR_MOBILE_HOME&&model.configured_layout?d->fields:255;
    lv_obj_t *card=dsr_sc_button(parent,x,y,width,height,0x41474b,DSR_SC_RADIUS,open_device,(void *)(uintptr_t)i);
    lv_obj_set_style_bg_grad_color(card,lv_color_hex(0x3c4246),0);lv_obj_set_style_bg_grad_dir(card,LV_GRAD_DIR_VER,0);
    lv_obj_t *thumb=dsr_sc_panel(card,8,9,50,50,d->online&&d->output?0x244357:0x29343b,LV_RADIUS_CIRCLE);
    dsr_sc_icon_centered(thumb,kind_icon(d->kind),32,d->online?DSR_SC_TEXT:DSR_SC_OFF);
    int name_width=cover?124:sensor?218:166;
    lv_obj_t *name=dsr_sc_text(card,d->name,68,10,name_width,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
    const char *state=d->error!=DSR_OK?device_error(d->error):!d->online?"Brak połączenia":d->busy?(d->command_pending?"Wysyłanie polecenia…":"Odczyt stanu…"):sensor?(d->has_contact&&(fields&64)?(d->contact_open?"Otwarte":"Zamknięte"):"Online"):cover?"Roleta":d->output?"Włączone":"Wyłączone";
    char subtitle[96];snprintf(subtitle,sizeof(subtitle),"%s • %s",dsr_sc_tr(room_name(d)),dsr_sc_tr(state));
    lv_obj_t *status=dsr_sc_text(card,d->error!=DSR_OK?state:subtitle,68,35,cover?124:218,&dsr_roboto_12,d->online&&d->error==DSR_OK?DSR_SC_MUTED:0xffc18c);truncate(status,18);
    if(cover){
     bool valid=d->online&&!d->busy&&d->can_cover&&(model.demo||actions.set_cover);
     card_control(card,i,DSR_ICON_UP,width-104,44,valid,cover_control,i*2+1);
     card_control(card,i,DSR_ICON_DOWN,width-54,44,valid,cover_control,i*2);
    }else if(!sensor)card_control(card,i,DSR_ICON_POWER,width-58,48,d->online&&!d->busy&&d->can_switch,control,i);
    if(!fields)return;
    char values[128]={0},part[40];
    if(!d->online||d->error!=DSR_OK)snprintf(values,sizeof(values),"%s",dsr_sc_tr("Odczyty niedostępne"));
    else if(cover){if((fields&32)&&d->has_position)snprintf(values,sizeof(values),"%s   %.0f%%",dsr_sc_tr("Pozycja"),d->position_pct);else snprintf(values,sizeof(values),"%s   —",dsr_sc_tr("Pozycja"));}
    else if(sensor){
      if((fields&4)&&d->has_battery)snprintf(values,sizeof(values),"%s %.0f%%",dsr_sc_tr("Bateria"),d->battery_pct);
      if((fields&8)&&d->has_temperature){snprintf(part,sizeof(part),"  %.1f °C",d->temperature_c);strncat(values,part,sizeof(values)-strlen(values)-1);}
      if((fields&16)&&d->has_humidity){snprintf(part,sizeof(part),"  %.0f%% RH",d->humidity_pct);strncat(values,part,sizeof(values)-strlen(values)-1);}
      if(!values[0])snprintf(values,sizeof(values),"Brak dodatkowych pomiarów");
    }else{
      if(fields&1){if(d->has_power)snprintf(values,sizeof(values),"%.1f W",d->power_w);else snprintf(values,sizeof(values),"— W");}
      if(fields&2){if(d->has_voltage)snprintf(part,sizeof(part),"    %.1f V",d->voltage_v);else snprintf(part,sizeof(part),"    — V");strncat(values,part,sizeof(values)-strlen(values)-1);}
    }
    if(values[0])dsr_sc_chip(card,values,68,height-26,width-78,d->online?DSR_SC_TEXT:0xffc18c);
}
static dsr_ui_icon_t room_icon(unsigned i)
{
    const dsr_ui_icon_t kinds[]={DSR_ICON_SOFA,DSR_ICON_KITCHEN,DSR_ICON_BED,DSR_ICON_BATH};
    return kinds[model.rooms[i].icon%4];
}
static void room_card(lv_obj_t *parent,unsigned i,int x,int y,int width,int height)
{
    lv_obj_t *card=dsr_sc_button(parent,x,y,width,height,DSR_SC_CARD,DSR_SC_RADIUS,open_room,(void *)(uintptr_t)i);
    const void *pixels=dsr_photo_pixels(model.rooms[i].photo);
    if(pixels){
        static lv_image_dsc_t photos[DSR_UI_MAX_ROOMS];photos[i]=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=DSR_PHOTO_WIDTH,.h=DSR_PHOTO_HEIGHT,.stride=DSR_PHOTO_WIDTH*2},.data_size=DSR_PHOTO_BYTES,.data=pixels};
        lv_obj_t *window=dsr_sc_panel(card,0,0,width,height-65,DSR_SC_CARD,DSR_SC_RADIUS);lv_obj_set_style_clip_corner(window,true,0);lv_obj_t *image=lv_image_create(window);lv_image_set_src(image,&photos[i]);
        uint32_t scale=(uint32_t)width*256/DSR_PHOTO_WIDTH,vertical=(uint32_t)(height-65)*256/DSR_PHOTO_HEIGHT;if(vertical>scale)scale=vertical;
        lv_image_set_scale(image,scale);lv_image_set_antialias(image,true);lv_obj_center(image);
    }else{lv_obj_t *thumb=dsr_sc_panel(card,(width-52)/2,24,52,52,0x303943,LV_RADIUS_CIRCLE);dsr_sc_icon_centered(thumb,room_icon(i),30,0xe8edf2);}
    lv_obj_t *tag=dsr_sc_panel(card,0,0,78,22,0x25292b,7);lv_obj_set_style_bg_opa(tag,220,0);dsr_sc_icon(tag,room_icon(i),4,3,16,DSR_SC_TEXT);dsr_sc_text(tag,"Pokój",24,4,50,&dsr_roboto_11,DSR_SC_TEXT);
    lv_obj_t *name=dsr_sc_text(card,model.rooms[i].name,6,height-56,width-12,&dsr_roboto_16,DSR_SC_TEXT);center(name);truncate(name,21);
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
static void scene_card(unsigned index,int y,int height)
{
 dsr_mobile_scene_t *scene=&model.scenes[index];lv_obj_t *card=dsr_sc_button(content,0,y,296,height,0x414548,12,navigate,(void *)(uintptr_t)12);
 lv_obj_t *icon=dsr_sc_panel(card,8,17,50,50,0x29343b,LV_RADIUS_CIRCLE);dsr_sc_icon_centered(icon,DSR_ICON_SCENES,32,DSR_SC_TEXT);
 lv_obj_t *name=dsr_sc_text(card,scene->name,68,10,164,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
 char text[80];snprintf(text,sizeof(text),dsr_sc_tr("Scena • %u działań"),scene->actions);dsr_sc_text(card,text,68,36,210,&dsr_roboto_12,DSR_SC_MUTED);
 card_control(card,0,DSR_ICON_EDIT,238,48,actions.edit_dashboard!=NULL,navigate,12);
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
 lv_obj_t *name=dsr_sc_text(root,title,50,11,216,&dsr_roboto_18,DSR_SC_TEXT);truncate(name,24);
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
static void details(lv_obj_t *root)
{
    if(selected>=model.device_count)return;
    dsr_mobile_device_t *d=&model.devices[selected];
    if(d->kind==DSR_DEVICE_COVER){
        cover_details(root);return;
    }
    lv_obj_t *panel=dsr_sc_panel(root,12,89,296,256,DSR_SC_CARD,10);
    dsr_sc_text(panel,"Główne sterowanie",13,12,270,&dsr_roboto_16,DSR_SC_TEXT);
    const char *status=d->error!=DSR_OK?device_error(d->error):!d->online?"Offline • stan niepotwierdzony":d->busy?(d->command_pending?"Wysyłanie polecenia…":"Odczyt stanu…"):d->output?"Włączone • Online":"Wyłączone • Online";
    center(dsr_sc_text(panel,status,8,41,280,&dsr_roboto_12,d->error!=DSR_OK?0xffc18c:d->online?DSR_SC_TEXT:DSR_SC_MUTED));
    char power[40];if(d->online&&d->error==DSR_OK&&d->has_power&&!d->busy)snprintf(power,sizeof(power),"%.1f W",d->power_w);else snprintf(power,sizeof(power),"—");
    center(dsr_sc_text(panel,power,8,70,280,&dsr_roboto_28,DSR_SC_TEXT));
    center(dsr_sc_text(panel,d->has_power?"Moc chwilowa":"Pomiar mocy niedostępny",8,103,280,&dsr_roboto_12,DSR_SC_MUTED));
    power_button(panel,selected,105,130,86);
    if(actions.refresh){lv_obj_t *button=dsr_sc_button(panel,202,148,82,48,DSR_SC_PANEL,10,refresh,NULL);lv_obj_t *label=dsr_sc_text(button,"Odśwież stan",4,8,74,&dsr_roboto_12,DSR_SC_TEXT);center(label);if(d->busy||!d->can_refresh)lv_obj_add_state(button,LV_STATE_DISABLED);}
    center(dsr_sc_text(panel,d->error!=DSR_OK?"Odśwież, aby potwierdzić stan":!d->online?"Brak połączenia":d->busy?"Oczekiwanie…":d->output?"Wyłącz":"Włącz",8,220,280,&dsr_roboto_14,d->online?DSR_SC_BLUE:DSR_SC_MUTED));
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
 dsr_sc_text(identity,d->error!=DSR_OK?device_error(d->error):d->online?"Online":"Brak połączenia",12,44,272,&dsr_roboto_14,d->online&&d->error==DSR_OK?DSR_SC_BLUE:DSR_SC_MUTED);
 lv_obj_t *model_name=dsr_sc_text(identity,d->model[0]?d->model:"Model nieodczytany",12,68,272,&dsr_roboto_14,DSR_SC_MUTED);truncate(model_name,20);
 snprintf(text,sizeof(text),dsr_sc_tr("Adres karty: %s • kanał %u"),d->host[0]?d->host:"—",d->channel);dsr_sc_text(identity,text,12,90,272,&dsr_roboto_12,DSR_SC_TEXT);
 const char *family=d->identity[0]?(d->generation==DSR_GEN1?"Gen1":d->generation==DSR_GEN2_PLUS?"Gen2+":"—"):"—";
 snprintf(text,sizeof(text),"%s • %s",dsr_sc_tr("Zapisane ID"),family);dsr_sc_text(identity,text,12,110,272,&dsr_roboto_11,DSR_SC_MUTED);
 lv_obj_t *id=dsr_sc_text(identity,d->identity[0]?d->identity:"Tożsamość niepotwierdzona",12,130,272,&dsr_roboto_12,DSR_SC_TEXT);truncate(id,20);
 lv_obj_t *readings=dsr_sc_panel(content,0,164,296,78,DSR_SC_PANEL,12);
 dsr_sc_text(readings,"Dostępne pomiary",12,11,272,&dsr_roboto_14,DSR_SC_TEXT);
 text[0]=0;bool valid=d->online&&d->error==DSR_OK;
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
    cover_drag=false;cover_preview_label=NULL;int scroll=content?lv_obj_get_scroll_y(content):0;
    lv_obj_t *root=lv_screen_active();content=sample_age=info_overlay=NULL;lv_obj_clean(root);lv_obj_remove_style_all(root);
    lv_obj_set_style_bg_color(root,dsr_sc_color(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
    lv_obj_set_style_bg_grad_color(root,lv_color_hex(0x1a2733),0);lv_obj_set_style_bg_grad_dir(root,LV_GRAD_DIR_VER,0);
    lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
    const char *title=(page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS)&&selected<model.device_count?model.devices[selected].name:
        page==DSR_MOBILE_ROOM&&selected<model.room_count?model.rooms[selected].name:model.home_name[0]?model.home_name:"Mój dom";
    if(page==DSR_MOBILE_DETAILS&&selected<model.device_count&&model.devices[selected].kind==DSR_DEVICE_COVER)cover_header(root,title);
    else header(root,title,page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_DEVICE_SETTINGS||page==DSR_MOBILE_ROOM);
    tabs(root);
    if(page==DSR_MOBILE_DEVICE_SETTINGS){device_settings(root);if(content)lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);}
    else if(page==DSR_MOBILE_DETAILS)details(root);
    else {
        toolbar(root,page==DSR_MOBILE_HOME?(model.dashboard_name[0]?model.dashboard_name:"Mój pulpit"):page==DSR_MOBILE_ROOMS?"Pomieszczenia":"Urządzenia");
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
                }
            }else for(unsigned i=0;i<model.device_count;i++)if(page!=DSR_MOBILE_ROOM||model.devices[i].room==selected){device_card(content,i,0,(int)at*92,296,84);at++;}
            if(!at)empty("Brak urządzeń","Dodaj pierwsze urządzenie Shelly przyciskiem +, aby rozpocząć sterowanie swoim domem.");
        }
        lv_obj_scroll_to_y(content,scroll,LV_ANIM_OFF);
    }
    if(model.demo){lv_obj_t *caption=dsr_sc_text(root,"DEMO • dane przykładowe",12,415,296,&dsr_roboto_11,0xe1bb70);center(caption);}
    bottom(root);
    if(info_open)dashboard_info(NULL);
    /* A horizontal metrics drag must never displace the whole screen. */
    lv_obj_update_layout(root);lv_obj_scroll_to(root,0,0,LV_ANIM_OFF);
}
void dsr_mobile_show(dsr_mobile_page_t p,unsigned index)
{
    if(p>DSR_MOBILE_DEVICE_SETTINGS)return;
    info_open=false;
    if(cover_preview&&p!=DSR_MOBILE_DETAILS)end_preview();
    if(p==DSR_MOBILE_DETAILS&&page!=DSR_MOBILE_DETAILS&&page!=DSR_MOBILE_DEVICE_SETTINGS){previous_page=page;previous_selected=selected;}
    content=NULL;page=p;selected=index;active=true;render();
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
void dsr_mobile_update(const dsr_mobile_model_t *m)
{
    if(!m||m->device_count>DSR_UI_MAX_DEVICES||m->room_count>DSR_UI_MAX_ROOMS||m->tile_count>24||m->scene_count>8)return;
    if(cover_preview){preview_saved=*m;
#ifndef DSR_UI_REVIEW_DEMO
        preview_saved.demo=false;
#endif
        return;}
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
