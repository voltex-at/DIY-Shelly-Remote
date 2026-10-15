#include "dsr_ui_mobile.h"
#include "dsr_ui_style.h"
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
static void edge(lv_obj_t *o,uint32_t c,int width){lv_obj_set_style_border_color(o,lv_color_hex(c),0);lv_obj_set_style_border_width(o,width,0);}
static void navigate(lv_event_t *e)
{
    unsigned target=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(target==10){if(actions.open_settings)actions.open_settings();return;}
    if(target==11){if(actions.add_device)actions.add_device();return;}
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
    dsr_sc_icon_centered(b,icon,18,DSR_SC_TEXT);return b;
}

static void header(lv_obj_t *root,const char *title,bool back)
{
    if(back)icon_button(root,DSR_ICON_BACK,2,0,40,DSR_SC_BG,navigate,(void *)(uintptr_t)98);
    lv_obj_t *name=dsr_sc_text(root,title,back?42:12,9,back?204:207,&dsr_roboto_18,DSR_SC_TEXT);truncate(name,24);
    if(!back)dsr_sc_icon(root,DSR_ICON_CHEVRON,116,12,15,DSR_SC_MUTED);
    if(model.demo){lv_obj_t *badge=dsr_sc_panel(root,256,9,54,21,0x5c492a,5);edge(badge,0xe1bb70,1);
        center(dsr_sc_text(badge,"DEMO",0,4,54,&dsr_roboto_12,0xffd68a));}
    else dsr_sc_icon(root,DSR_ICON_WIFI,280,10,22,model.wifi?DSR_SC_BLUE:DSR_SC_OFF);
}
static void tab(lv_obj_t *root,const char *text,int x,int w,bool chosen,lv_event_cb_t cb,void *ctx)
{
    lv_obj_t *b=dsr_sc_button(root,x,40,w,36,DSR_SC_BG,5,cb,ctx);
    if(chosen){edge(b,DSR_SC_BLUE,1);lv_obj_set_style_border_side(b,LV_BORDER_SIDE_TOP|LV_BORDER_SIDE_LEFT|LV_BORDER_SIDE_RIGHT,0);}
    lv_obj_t *s=dsr_sc_text(b,text,0,10,w,&dsr_roboto_12,chosen?DSR_SC_TEXT:DSR_SC_MUTED);center(s);
    if(!cb&&!chosen){lv_obj_add_state(b,LV_STATE_DISABLED);lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,LV_STATE_DISABLED);}
}
static void tabs(lv_obj_t *root)
{
    dsr_sc_panel(root,0,75,320,1,DSR_SC_BLUE,0);
    if(page==DSR_MOBILE_HOME){tab(root,"Mój pulpit",0,110,true,NULL,NULL);tab(root,"Aktywność",114,108,false,NULL,NULL);return;}
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
    dsr_sc_panel(bar,16+current*64,0,32,3,DSR_SC_BLUE,2);
    const dsr_ui_icon_t icons[]={DSR_ICON_DASHBOARD,DSR_ICON_HOME,DSR_ICON_PLUG,DSR_ICON_SETTINGS,DSR_ICON_PLUS};
    const unsigned targets[]={DSR_MOBILE_HOME,DSR_MOBILE_ROOMS,99,10,11};
    for(unsigned i=0;i<5;i++){
        lv_obj_t *b=dsr_sc_button(bar,(int)i*64,4,64,47,DSR_SC_NAV,0,
            targets[i]==99?NULL:navigate,(void *)(uintptr_t)targets[i]);
        lv_obj_set_style_bg_opa(b,LV_OPA_TRANSP,0);
        if(i==4){lv_obj_t *circle=dsr_sc_panel(b,0,0,36,36,DSR_SC_BLUE,LV_RADIUS_CIRCLE);lv_obj_center(circle);
            dsr_sc_icon_centered(circle,icons[i],20,DSR_SC_TEXT);}
        else dsr_sc_icon_centered(b,icons[i],22,i==(unsigned)current?DSR_SC_TEXT:0xbec5cd);
        if((i==3&&!actions.open_settings)||(i==4&&!actions.add_device)||i==2)lv_obj_add_state(b,LV_STATE_DISABLED);
    }
}
static void toolbar(lv_obj_t *root,const char *title)
{
    lv_obj_t *bar=dsr_sc_panel(root,10,84,300,34,DSR_SC_PANEL,8);edge(bar,DSR_SC_EDGE,1);
    dsr_sc_text(bar,title,9,7,187,&dsr_roboto_16,DSR_SC_TEXT);
    lv_obj_t *plus=icon_button(bar,DSR_ICON_PLUS,194,2,30,0x2e6fb7,navigate,(void *)(uintptr_t)11);
    if(!actions.add_device)lv_obj_add_state(plus,LV_STATE_DISABLED);
    lv_obj_t *edit=icon_button(bar,DSR_ICON_EDIT,229,2,30,0x2e6fb7,NULL,NULL);lv_obj_add_state(edit,LV_STATE_DISABLED);
    icon_button(bar,DSR_ICON_INFO,264,2,30,DSR_SC_PANEL,NULL,NULL);
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
static dsr_ui_icon_t kind_icon(dsr_mobile_kind_t kind){return kind==DSR_DEVICE_LIGHT?DSR_ICON_LIGHT:DSR_ICON_PLUG;}
static const char *room_name(const dsr_mobile_device_t *d){return d->room<model.room_count?model.rooms[d->room].name:"Nieprzypisane";}
static void device_card(lv_obj_t *parent,unsigned i,int x,int y,int width,int height)
{
    dsr_mobile_device_t *d=&model.devices[i];
    lv_obj_t *card=dsr_sc_button(parent,x,y,width,height,DSR_SC_CARD,9,open_device,(void *)(uintptr_t)i);
    edge(card,0x5b5f65,1);
    dsr_sc_text(card,d->kind==DSR_DEVICE_LIGHT?"Światło":"Przekaźnik",7,5,width-14,&dsr_roboto_11,DSR_SC_MUTED);
    lv_obj_t *thumb=dsr_sc_panel(card,(width-38)/2,21,38,38,0x2c343e,LV_RADIUS_CIRCLE);
    dsr_sc_icon_centered(thumb,kind_icon(d->kind),22,DSR_SC_TEXT);
    lv_obj_t *name=dsr_sc_text(card,d->name,6,64,width-12,&dsr_roboto_14,DSR_SC_TEXT);center(name);truncate(name,32);
    lv_obj_t *room=dsr_sc_text(card,room_name(d),6,94,width-12,&dsr_roboto_11,DSR_SC_MUTED);center(room);truncate(room,14);
    char value[32];
    if(!d->online)snprintf(value,sizeof(value),"Offline");
    else if(d->busy)snprintf(value,sizeof(value),"Czekaj…");
    else if(d->has_power)snprintf(value,sizeof(value),"%.1f W",d->power_w);
    else snprintf(value,sizeof(value),"%s",d->output?"Włączone":"Wyłączone");
    dsr_sc_text(card,value,9,height-29,width-55,&dsr_roboto_12,d->online?DSR_SC_TEXT:0xc5c8cc);
    power_button(card,i,width-49,height-44,40);
}
static dsr_ui_icon_t room_icon(unsigned i)
{
    const dsr_ui_icon_t kinds[]={DSR_ICON_SOFA,DSR_ICON_KITCHEN,DSR_ICON_BED,DSR_ICON_BATH};
    return kinds[model.rooms[i].icon%4];
}
static void room_card(lv_obj_t *parent,unsigned i,int x,int y)
{
    lv_obj_t *card=dsr_sc_button(parent,x,y,144,138,DSR_SC_CARD,9,open_room,(void *)(uintptr_t)i);edge(card,0x5b5f65,1);
    dsr_sc_text(card,"Pomieszczenie",7,5,130,&dsr_roboto_11,DSR_SC_MUTED);
    lv_obj_t *thumb=dsr_sc_panel(card,46,24,52,52,0x303943,LV_RADIUS_CIRCLE);
    dsr_sc_icon_centered(thumb,room_icon(i),30,0xe8edf2);
    lv_obj_t *name=dsr_sc_text(card,model.rooms[i].name,6,82,132,&dsr_roboto_14,DSR_SC_TEXT);center(name);truncate(name,18);
    double power=0;unsigned count=0;bool complete=true;
    for(unsigned d=0;d<model.device_count;d++)if(model.devices[d].room==i){count++;complete&=model.devices[d].online&&model.devices[d].has_power;power+=model.devices[d].power_w;}
    lv_obj_t *badge=dsr_sc_panel(card,35,106,74,25,0x30353a,6);
    char text[32];if(count&&complete)snprintf(text,sizeof(text),"%.1f W",power);else snprintf(text,sizeof(text),"%u urz.",count);
    center(dsr_sc_text(badge,text,0,5,74,&dsr_roboto_12,DSR_SC_TEXT));
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
    lv_obj_set_style_bg_color(root,lv_color_hex(DSR_SC_BG),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
    lv_obj_set_style_bg_grad_color(root,lv_color_hex(0x1a2733),0);lv_obj_set_style_bg_grad_dir(root,LV_GRAD_DIR_VER,0);
    lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
    const char *title=page==DSR_MOBILE_DETAILS&&selected<model.device_count?model.devices[selected].name:
        page==DSR_MOBILE_ROOM&&selected<model.room_count?model.rooms[selected].name:model.home_name[0]?model.home_name:"Mój dom";
    header(root,title,page==DSR_MOBILE_DETAILS||page==DSR_MOBILE_ROOM);tabs(root);
    if(page==DSR_MOBILE_DETAILS)details(root);
    else {
        toolbar(root,page==DSR_MOBILE_HOME?"Mój pulpit":page==DSR_MOBILE_ROOMS?"Pomieszczenia":"Urządzenia");
        content=dsr_sc_panel(root,12,126,296,290,DSR_SC_BG,0);lv_obj_set_style_bg_opa(content,LV_OPA_TRANSP,0);
        lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);lv_obj_set_scroll_dir(content,LV_DIR_VER);
        lv_obj_set_scrollbar_mode(content,LV_SCROLLBAR_MODE_AUTO);
        if(page==DSR_MOBILE_ROOMS){
            for(unsigned i=0;i<model.room_count;i++)room_card(content,i,(int)(i%2)*152,(int)(i/2)*146);
            if(!model.room_count)empty("Brak pomieszczeń","Nie utworzono jeszcze pomieszczeń.\nUrządzenia pozostają dostępne w zakładce Urządzenia.");
        } else {
            unsigned at=0;
            for(unsigned i=0;i<model.device_count;i++)if(page!=DSR_MOBILE_ROOM||model.devices[i].room==selected){device_card(content,i,(int)(at%2)*152,(int)(at/2)*150,144,144);at++;}
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
    if(!m||m->device_count>DSR_UI_MAX_DEVICES||m->room_count>DSR_UI_MAX_ROOMS)return;
    if(!memcmp(&model,m,sizeof(model)))return;
    model=*m;
#ifndef DSR_UI_REVIEW_DEMO
    model.demo=false;
#endif
    if(active)render();
}
