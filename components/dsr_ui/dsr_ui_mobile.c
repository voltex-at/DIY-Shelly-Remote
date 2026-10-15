#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static dsr_mobile_model_t model;
static dsr_mobile_actions_t actions;
static dsr_mobile_page_t page;
static unsigned selected;
static dsr_mobile_page_t previous_page;
static unsigned previous_selected;
static bool active;
static lv_obj_t *content;
static void render(void);

void dsr_mobile_bind(const dsr_mobile_actions_t *a){if(a)actions=*a;else memset(&actions,0,sizeof(actions));}
bool dsr_mobile_active(void){return active;}
dsr_mobile_page_t dsr_mobile_page(void){return page;}
void dsr_mobile_hide(void){active=false;content=NULL;}
static void center(lv_obj_t *o){lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0);}
static void truncate(lv_obj_t *o,int height){lv_obj_set_height(o,height);lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_DOTS);}
static void edge(lv_obj_t *o,uint32_t c,int width){lv_obj_set_style_border_color(o,dsr_sc_color(c),0);lv_obj_set_style_border_width(o,width,0);}
static void navigate(lv_event_t *e)
{
    unsigned target=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(target==10){if(actions.open_settings)actions.open_settings();return;}
    if(target==11){if(actions.add_device)actions.add_device();return;}
    if(target==12){if(actions.edit_dashboard)actions.edit_dashboard();return;}
    if(target==13){if(actions.select_dashboard)actions.select_dashboard();return;}
    if(target==98){dsr_mobile_show(page==DSR_MOBILE_DETAILS?previous_page:DSR_MOBILE_ROOMS,
        page==DSR_MOBILE_DETAILS?previous_selected:0);return;}
    dsr_mobile_show((dsr_mobile_page_t)target,0);
}
static void open_device(lv_event_t *e){dsr_mobile_show(DSR_MOBILE_DETAILS,(unsigned)(uintptr_t)lv_event_get_user_data(e));}
static void open_room(lv_event_t *e){dsr_mobile_show(DSR_MOBILE_ROOM,(unsigned)(uintptr_t)lv_event_get_user_data(e));}
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
    if(back)icon_button(root,DSR_ICON_BACK,0,0,44,DSR_SC_BG,navigate,(void *)(uintptr_t)98);
    lv_obj_t *name=dsr_sc_text(root,title,back?42:12,9,back?204:207,&dsr_roboto_18,DSR_SC_TEXT);truncate(name,24);
    if(!back)dsr_sc_icon(root,DSR_ICON_CHEVRON,116,12,15,DSR_SC_MUTED);
    if(model.demo){lv_obj_t *badge=dsr_sc_panel(root,256,9,54,21,0x5c492a,5);edge(badge,0xe1bb70,1);
        center(dsr_sc_text(badge,"DEMO",0,4,54,&dsr_roboto_12,0xffd68a));}
    else dsr_sc_icon(root,DSR_ICON_WIFI,280,10,22,model.wifi?DSR_SC_BLUE:DSR_SC_OFF);
}
static void tab(lv_obj_t *root,const char *text,int x,int w,bool chosen,lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *b=dsr_sc_button(root,x,44,w,44,DSR_SC_BG,7,cb,ctx);
    if(chosen){edge(b,DSR_SC_BLUE,1);lv_obj_set_style_border_side(b,LV_BORDER_SIDE_TOP|LV_BORDER_SIDE_LEFT|LV_BORDER_SIDE_RIGHT,0);}
    lv_obj_t *s=dsr_sc_text(b,text,0,14,w,&dsr_roboto_14,chosen?DSR_SC_TEXT:DSR_SC_MUTED);center(s);
    truncate(s,18);
    if(!cb&&!chosen){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,LV_STATE_DISABLED);}
}
static void tabs(lv_obj_t *root)
{
    dsr_sc_panel(root,0,87,320,1,DSR_SC_BLUE,0);
    if(page==DSR_MOBILE_HOME){tab(root,"Aktywność",0,110,false,NULL,NULL);tab(root,model.dashboard_name[0]?model.dashboard_name:"Mój pulpit",114,162,true,NULL,NULL);return;}
    if(page==DSR_MOBILE_DETAILS){tab(root,"Sterowanie",0,109,true,NULL,NULL);tab(root,"Wykresy",111,101,false,NULL,NULL);tab(root,"Ustawienia",214,106,false,NULL,NULL);return;}
    if(page==DSR_MOBILE_ROOM){tab(root,"Urządzenia",0,115,true,NULL,NULL);tab(root,"Grupy",117,87,false,NULL,NULL);tab(root,"Sceny",206,114,false,NULL,NULL);return;}
    tab(root,"Pomieszczenia",0,101,page==DSR_MOBILE_ROOMS,navigate,(void *)(uintptr_t)DSR_MOBILE_ROOMS);
    tab(root,"Grupy",103,55,false,NULL,NULL);tab(root,"Sceny",160,55,false,NULL,NULL);
    tab(root,"Urządzenia",217,103,page==DSR_MOBILE_DEVICES,navigate,(void *)(uintptr_t)DSR_MOBILE_DEVICES);
}
static void bottom(lv_obj_t *root)
{
    lv_obj_t *bar=dsr_sc_panel(root,0,428,320,52,DSR_SC_NAV,0);edge(bar,0x60646b,1);
    lv_obj_set_style_bg_grad_color(bar,lv_color_hex(0x232b33),0);lv_obj_set_style_bg_grad_dir(bar,LV_GRAD_DIR_VER,0);
    int current=page==DSR_MOBILE_HOME?0:1;
    dsr_sc_panel(bar,11+current*53,0,32,3,DSR_SC_BLUE,2);
    const dsr_ui_icon_t icons[]={DSR_ICON_DASHBOARD,DSR_ICON_HOME,DSR_ICON_ENERGY,DSR_ICON_SETTINGS,DSR_ICON_SCENES,DSR_ICON_PLUS};
    const unsigned targets[]={DSR_MOBILE_HOME,DSR_MOBILE_ROOMS,99,10,12,11};
    for(unsigned i=0;i<6;i++){
        int x=(int)i*320/6,w=(int)(i+1)*320/6-x;
        lv_obj_t *b=dsr_sc_button(bar,x,4,w,47,DSR_SC_NAV,0,
            targets[i]==99?NULL:navigate,(void *)(uintptr_t)targets[i]);
        lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,0);
        if(i==5){lv_obj_t *circle=dsr_sc_panel(b,0,0,36,36,DSR_SC_BLUE,LV_RADIUS_CIRCLE);lv_obj_center(circle);
            dsr_sc_icon_centered(circle,icons[i],20,DSR_SC_TEXT);}
        else dsr_sc_icon_centered(b,icons[i],26,i==(unsigned)current?DSR_SC_TEXT:0xbec5cd);
        if((i==3&&!actions.open_settings)||(i==4&&!actions.edit_dashboard)||(i==5&&!actions.add_device)||i==2)lv_obj_add_state(b,LV_STATE_DISABLED);
    }
}
static void toolbar(lv_obj_t *root,const char *title)
{
    lv_obj_t *bar=dsr_sc_panel(root,10,94,300,44,DSR_SC_PANEL,10);edge(bar,DSR_SC_EDGE,1);
    lv_obj_t *choose=dsr_sc_button(bar,0,0,152,44,DSR_SC_PANEL,8,page==DSR_MOBILE_HOME&&actions.select_dashboard?navigate:NULL,(void *)(uintptr_t)13);
    lv_obj_t *name=dsr_sc_text(choose,title,9,12,122,&dsr_roboto_16,DSR_SC_TEXT);truncate(name,22);
    if(page==DSR_MOBILE_HOME&&actions.select_dashboard)dsr_sc_icon(choose,DSR_ICON_CHEVRON,131,14,16,DSR_SC_MUTED);
    lv_obj_t *plus=icon_button(bar,DSR_ICON_PLUS,156,0,44,0x287ec7,navigate,(void *)(uintptr_t)11);
    if(!actions.add_device)lv_obj_add_state(plus,LV_STATE_DISABLED);
    lv_obj_t *edit=icon_button(bar,DSR_ICON_EDIT,204,0,44,0x287ec7,navigate,(void *)(uintptr_t)12);if(!actions.edit_dashboard)lv_obj_add_state(edit,LV_STATE_DISABLED);
    lv_obj_t *info=icon_button(bar,DSR_ICON_INFO,252,0,44,DSR_SC_PANEL,NULL,NULL);lv_obj_remove_flag(info,LV_OBJ_FLAG_CLICKABLE);
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
 if(!d->online||d->busy||!d->can_cover)return;
 if(model.demo){d->position_pct=direction>0?100:0;render();}
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
    const char *state=!d->online?"Brak połączenia":d->busy?"Oczekiwanie…":sensor?(d->has_contact&&(fields&64)?(d->contact_open?"Otwarte":"Zamknięte"):"Online"):cover?"Roleta":d->output?"Włączone":"Wyłączone";
    char subtitle[96];snprintf(subtitle,sizeof(subtitle),"%s • %s",dsr_sc_tr(room_name(d)),dsr_sc_tr(state));
    dsr_sc_text(card,subtitle,68,35,cover?124:218,&dsr_roboto_12,d->online?DSR_SC_MUTED:0xffc18c);
    if(cover){
     bool valid=d->online&&!d->busy&&d->can_cover&&(model.demo||actions.set_cover);
     card_control(card,i,DSR_ICON_UP,width-104,44,valid,cover_control,i*2+1);
     card_control(card,i,DSR_ICON_DOWN,width-54,44,valid,cover_control,i*2);
    }else if(!sensor)card_control(card,i,DSR_ICON_POWER,width-58,48,d->online&&!d->busy&&d->can_switch,control,i);
    if(!fields)return;
    lv_obj_t *strip=dsr_sc_panel(card,68,height-26,width-78,20,0x292d30,6);
    char values[128]={0},part[40];
    if(!d->online)snprintf(values,sizeof(values),"Offline • odczyty niedostępne");
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
    dsr_sc_text(strip,values,7,3,width-92,&dsr_roboto_12,d->online?DSR_SC_TEXT:0xffc18c);
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
static void details(lv_obj_t *root)
{
    if(selected>=model.device_count)return;
    dsr_mobile_device_t *d=&model.devices[selected];
    lv_obj_t *panel=dsr_sc_panel(root,12,89,296,256,DSR_SC_CARD,10);
    if(d->kind==DSR_DEVICE_COVER){
        dsr_sc_text(panel,"Sterowanie roletą",13,12,270,&dsr_roboto_16,DSR_SC_TEXT);
        center(dsr_sc_text(panel,d->online?"Pozycja rolety":"Offline • stan niepotwierdzony",8,43,280,&dsr_roboto_12,DSR_SC_MUTED));
        char position[24];if(d->online&&d->has_position)snprintf(position,sizeof(position),"%.0f%%",d->position_pct);else snprintf(position,sizeof(position),"—");
        center(dsr_sc_text(panel,position,8,72,280,&dsr_roboto_28,DSR_SC_TEXT));
        bool valid=d->online&&!d->busy&&d->can_cover&&(model.demo||actions.set_cover);
        lv_obj_t *up=dsr_sc_button(panel,72,130,68,64,0xe8ecef,14,cover_control,(void *)(uintptr_t)(selected*2+1));
        lv_obj_t *down=dsr_sc_button(panel,156,130,68,64,0xe8ecef,14,cover_control,(void *)(uintptr_t)(selected*2));
        dsr_sc_icon_centered(up,DSR_ICON_UP,35,valid?DSR_SC_BLUE:DSR_SC_OFF);dsr_sc_icon_centered(down,DSR_ICON_DOWN,35,valid?DSR_SC_BLUE:DSR_SC_OFF);
        if(!valid){lv_obj_add_state(up,LV_STATE_DISABLED);lv_obj_add_state(down,LV_STATE_DISABLED);}
        center(dsr_sc_text(panel,d->busy?"Oczekiwanie…":!valid?"Sterowanie niedostępne":"Góra / dół",8,216,280,&dsr_roboto_14,DSR_SC_MUTED));
        metric(root,"Pozycja",position,12);metric(root,"Stan",dsr_sc_tr(d->online?"Online":"Offline"),164);return;
    }
    dsr_sc_text(panel,"Główne sterowanie",13,12,270,&dsr_roboto_16,DSR_SC_TEXT);
    const char *status=!d->online?"Offline • stan niepotwierdzony":d->busy?"Oczekiwanie na odczyt…":d->output?"Włączone • Online":"Wyłączone • Online";
    center(dsr_sc_text(panel,status,8,41,280,&dsr_roboto_12,d->online?DSR_SC_TEXT:DSR_SC_MUTED));
    char power[40];if(d->online&&d->has_power&&!d->busy)snprintf(power,sizeof(power),"%.1f W",d->power_w);else snprintf(power,sizeof(power),"—");
    center(dsr_sc_text(panel,power,8,70,280,&dsr_roboto_28,DSR_SC_TEXT));
    center(dsr_sc_text(panel,d->has_power?"Moc chwilowa":"Pomiar mocy niedostępny",8,103,280,&dsr_roboto_12,DSR_SC_MUTED));
    power_button(panel,selected,105,130,86);
    center(dsr_sc_text(panel,!d->online?"Brak połączenia":d->busy?"Oczekiwanie…":d->output?"Wyłącz":"Włącz",8,226,280,&dsr_roboto_14,d->online?DSR_SC_BLUE:DSR_SC_MUTED));
    char voltage[32],current[32];
    if(d->online&&d->has_voltage)snprintf(voltage,sizeof(voltage),"%.1f V",d->voltage_v);else snprintf(voltage,sizeof(voltage),"—");
    if(d->online&&d->has_current)snprintf(current,sizeof(current),"%.3f A",d->current_a);else snprintf(current,sizeof(current),"—");
    metric(root,"Napięcie",voltage,12);metric(root,"Natężenie",current,164);
}
static void render(void)
{
    int scroll=content?lv_obj_get_scroll_y(content):0;
    lv_obj_t *root=lv_screen_active();content=NULL;lv_obj_clean(root);lv_obj_remove_style_all(root);
    lv_obj_set_style_bg_color(root,dsr_sc_color(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
    lv_obj_set_style_bg_grad_color(root,lv_color_hex(0x1a2733),0);lv_obj_set_style_bg_grad_dir(root,LV_GRAD_DIR_VER,0);
    lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
    const char *title=page==DSR_MOBILE_DETAILS&&selected<model.device_count?model.devices[selected].name:
        page==DSR_MOBILE_ROOM&&selected<model.room_count?model.rooms[selected].name:model.home_name[0]?model.home_name:"Mój dom";
    header(root,title,page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_ROOM);tabs(root);
    if(page==DSR_MOBILE_DETAILS)details(root);
    else {
        toolbar(root,page==DSR_MOBILE_HOME?(model.dashboard_name[0]?model.dashboard_name:"Mój pulpit"):page==DSR_MOBILE_ROOMS?"Pomieszczenia":"Urządzenia");
        content=dsr_sc_panel(root,12,146,296,270,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);
        lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);
        lv_obj_set_scrollbar_mode(content,LV_SCROLLBAR_MODE_AUTO);
        if(page==DSR_MOBILE_ROOMS){
            for(unsigned i=0;i<model.room_count;i++)room_card(content,i,(int)(i%2)*152,(int)(i/2)*138,144,130);
            if(!model.room_count)empty("Brak pomieszczeń","Nie utworzono jeszcze pomieszczeń.\nUrządzenia pozostają dostępne w zakładce Urządzenia.");
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
}
void dsr_mobile_show(dsr_mobile_page_t p,unsigned index)
{
    if(p>DSR_MOBILE_DETAILS)return;
    if(p==DSR_MOBILE_DETAILS&&page!=DSR_MOBILE_DETAILS){previous_page=page;previous_selected=selected;}
    content=NULL;page=p;selected=index;active=true;render();
}
void dsr_mobile_update(const dsr_mobile_model_t *m)
{
    if(!m||m->device_count>DSR_UI_MAX_DEVICES||m->room_count>DSR_UI_MAX_ROOMS||m->tile_count>24||m->scene_count>8)return;
    if(!memcmp(&model,m,sizeof(model)))return;
    if(page==DSR_MOBILE_HOME&&model.dashboard_id!=m->dashboard_id)content=NULL;
    if(page==DSR_MOBILE_DETAILS&&selected<model.device_count&&model.devices[selected].id){
        uint32_t id=model.devices[selected].id;unsigned next=0;while(next<m->device_count&&m->devices[next].id!=id)next++;
        if(next==m->device_count){page=DSR_MOBILE_HOME;selected=0;content=NULL;}else selected=next;
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
