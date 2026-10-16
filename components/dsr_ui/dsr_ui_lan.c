#include "dsr_ui_internal.h"
#include "dsr_ui_mobile.h"
#include "dsr_discovery.h"
#include "dsr_workspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool active,building,waiting_scan,waiting_inspect;
static uint32_t observed;
static dsr_lan_snapshot_t snapshot;
static dsr_lan_device_t selected;
static dsr_lan_capabilities_t capabilities;
static dsr_credentials_t credentials;
static lv_obj_t *notice,*username,*password,*name,*room,*channel,*save_button;
static lv_obj_t *form_body;
static uint32_t room_ids[9];
static uint8_t channels[16];
static void show_results(void);
static void show_credentials(void);
static void show_components(void);
bool dsr_ui_lan_active(void){return active;}

void dsr_ui_lan_reset(void)
{
    active=false;notice=username=password=name=room=channel=save_button=form_body=NULL;
    if(!building){memset(&credentials,0,sizeof(credentials));waiting_scan=waiting_inspect=false;}
}
static lv_obj_t *screen(const char *title)
{building=true;lv_obj_t *root=dsr_ui_screen(title,false);building=false;active=true;return root;}
static void leave(lv_event_t *event){(void)event;dsr_lan_cancel();dsr_network_device(NULL);}
static void results_back(lv_event_t *event){(void)event;memset(&credentials,0,sizeof(credentials));show_results();}
static const char *failure(dsr_result_t result)
{
    return result==DSR_BUSY?"Operacja w toku. Poczekaj.":result==DSR_AUTH_REQUIRED?"Błąd logowania. Sprawdź dane.":result==DSR_IDENTITY_MISMATCH?"Tożsamość urządzenia się zmieniła. Wyszukaj ponownie.":result==DSR_RATE_LIMITED?"Limit logowań Shelly. Poczekaj i odśwież.":result==DSR_OFFLINE?"Shelly offline lub brak Wi-Fi":"Nie udało się zweryfikować urządzenia.";
}
static lv_obj_t *field(lv_obj_t *root,const char *caption,int y,const char *text,bool secret)
{
    dsr_sc_text(root,caption,16,y,288,&dsr_roboto_12,DSR_SC_MUTED);lv_obj_t *f=lv_textarea_create(root);dsr_ui_textarea(f);
    lv_obj_set_pos(f,16,y+18);lv_obj_set_size(f,288,44);lv_textarea_set_one_line(f,true);lv_textarea_set_max_length(f,secret?64:47);lv_textarea_set_password_mode(f,secret);lv_textarea_set_text(f,text);return f;
}
static void keyboard_close(lv_event_t *event){lv_obj_t *kb=lv_event_get_user_data(event);lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);lv_keyboard_set_textarea(kb,NULL);if(form_body)lv_obj_set_height(form_body,214);}
static void focus(lv_event_t *event)
{lv_obj_t *kb=lv_event_get_user_data(event);if(form_body)lv_obj_set_height(form_body,82);dsr_ui_keyboard_focus(kb,lv_event_get_target(event));lv_obj_scroll_to_view(lv_event_get_target(event),LV_ANIM_OFF);}
static void keyboard(lv_obj_t *root,lv_obj_t *a,lv_obj_t *b)
{
    lv_obj_t *kb=lv_keyboard_create(root);dsr_ui_keyboard(kb);lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb,keyboard_close,LV_EVENT_READY,kb);lv_obj_add_event_cb(kb,keyboard_close,LV_EVENT_CANCEL,kb);
    lv_obj_add_event_cb(a,focus,LV_EVENT_CLICKED,kb);lv_obj_add_event_cb(a,focus,LV_EVENT_FOCUSED,kb);if(b){lv_obj_add_event_cb(b,focus,LV_EVENT_CLICKED,kb);lv_obj_add_event_cb(b,focus,LV_EVENT_FOCUSED,kb);}
}
static void toggle_secret(lv_event_t *event){if(password)lv_textarea_set_password_mode(password,!lv_obj_has_state(lv_event_get_target(event),LV_STATE_CHECKED));}
static lv_obj_t *form(lv_obj_t *root)
{form_body=dsr_sc_panel(root,0,154,320,214,DSR_SC_BG,0);lv_obj_add_flag(form_body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(form_body,LV_DIR_VER);return form_body;}
static void identity(lv_obj_t *root)
{
    lv_obj_t *card=dsr_sc_panel(root,12,83,296,64,DSR_SC_CARD,12);dsr_sc_icon(card,DSR_ICON_PLUG,10,16,32,DSR_SC_TEXT);
    dsr_sc_text(card,selected.model,52,10,234,&dsr_roboto_14,DSR_SC_TEXT);
    char text[80];snprintf(text,sizeof(text),"%s • Gen %u",selected.host,selected.generation);dsr_sc_text(card,text,52,34,234,&dsr_roboto_12,DSR_SC_MUTED);
}
static void inspect(lv_event_t *event)
{
    (void)event;if(username&&password){const char *u=lv_textarea_get_text(username),*p=lv_textarea_get_text(password);
        if(strlen(u)>=sizeof(credentials.username)||strlen(p)>=sizeof(credentials.password)){dsr_sc_label_set_text(notice,"Dane logowania są za długie.");return;}
        strcpy(credentials.username,u);strcpy(credentials.password,p);}
    if(!dsr_lan_inspect(selected.host,selected.id,&credentials)){dsr_sc_label_set_text(notice,"Operacja w toku. Poczekaj.");return;}
    waiting_inspect=true;observed=0;lv_obj_t *root=screen("Sprawdź urządzenie");identity(root);
    notice=dsr_sc_text(root,"Odczyt możliwości urządzenia…",16,192,288,&dsr_roboto_14,DSR_SC_MUTED);dsr_ui_button(root,"Anuluj",12,420,296,leave,NULL);
}
static void choose(lv_event_t *event)
{
    unsigned index=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(index>=snapshot.count)return;
    selected=snapshot.devices[index];memset(&credentials,0,sizeof(credentials));
    if(selected.auth_required)show_credentials();else{username=password=NULL;inspect(NULL);}
}
static void start(lv_event_t *event)
{
    (void)event;bool ok=dsr_lan_scan();dsr_lan_snapshot(&snapshot);waiting_scan=ok||snapshot.phase==DSR_LAN_SCANNING;waiting_inspect=false;observed=0;
    const char *message=waiting_scan?"Wyszukiwanie mDNS i weryfikacja API…":!dsr_wifi_connected()?"Najpierw połącz Wi-Fi":snapshot.result==DSR_IO?"Usługa LAN niedostępna. Sprawdź firmware.":"Operacja w toku. Poczekaj.";
    lv_obj_t *root=screen("Szukaj Shelly w sieci");notice=dsr_sc_text(root,message,16,118,288,&dsr_roboto_14,DSR_SC_TEXT);
    if(ok){lv_obj_t *spinner=lv_spinner_create(root);lv_obj_set_pos(spinner,136,212);lv_obj_set_size(spinner,48,48);lv_spinner_set_anim_params(spinner,1000,200);}
    dsr_ui_button(root,"Anuluj",12,420,140,leave,NULL);dsr_ui_button(root,"Ponów",168,420,140,start,NULL);
}
void dsr_ui_lan_open(lv_event_t *event){start(event);}
static void show_results(void)
{
    dsr_lan_snapshot(&snapshot);lv_obj_t *root=screen("Znalezione urządzenia");
    lv_obj_t *list=dsr_sc_panel(root,12,86,296,274,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);
    for(unsigned i=0;i<snapshot.count;i++){
        const dsr_lan_device_t *d=&snapshot.devices[i];lv_obj_t *card=dsr_sc_button(list,0,(int)i*88,296,80,DSR_SC_CARD,12,choose,(void *)(uintptr_t)i);
        dsr_sc_icon(card,DSR_ICON_PLUG,10,22,32,DSR_SC_TEXT);dsr_sc_text(card,d->model,52,9,234,&dsr_roboto_14,DSR_SC_TEXT);
        char text[80];snprintf(text,sizeof(text),"%s • Gen %u",d->host,d->generation);dsr_sc_text(card,text,52,31,234,&dsr_roboto_12,DSR_SC_MUTED);
        dsr_sc_text(card,dsr_network_endpoint_added(d->id,d->generation==1?DSR_GEN1:DSR_GEN2_PLUS,UINT32_MAX)?"Już dodane":d->auth_required?"Wymaga hasła":"Gotowe do sprawdzenia",52,52,234,&dsr_roboto_12,DSR_SC_BLUE);
    }
    if(!snapshot.count)dsr_sc_text(list,snapshot.phase==DSR_LAN_NO_WIFI?"Najpierw połącz Wi-Fi":"Nie znaleziono Shelly. Sprawdź sieć i ponów.",12,40,272,&dsr_roboto_16,DSR_SC_TEXT);
    notice=dsr_sc_text(root,snapshot.phase==DSR_LAN_CANCELLED?"Wyszukiwanie przerwane":"Zakończono wyszukiwanie",16,372,288,&dsr_roboto_12,DSR_SC_MUTED);
    dsr_ui_button(root,"Wstecz",12,420,140,leave,NULL);dsr_ui_button(root,"Ponów",168,420,140,start,NULL);
}
static void show_credentials(void)
{
    lv_obj_t *root=screen("Sprawdź urządzenie");identity(root);
    lv_obj_t *body=form(root);username=field(body,"Login",0,"admin",false);password=field(body,"Hasło Shelly",76,"",true);
    lv_obj_t *show=lv_checkbox_create(body);lv_obj_set_pos(show,16,152);lv_checkbox_set_text(show,dsr_sc_tr("Show"));lv_obj_add_event_cb(show,toggle_secret,LV_EVENT_VALUE_CHANGED,NULL);
    notice=dsr_sc_text(root,"Dane logowania służą tylko wybranemu urządzeniu.",16,372,288,&dsr_roboto_12,DSR_SC_MUTED);
    dsr_ui_button(root,"Wstecz",12,420,140,results_back,NULL);dsr_ui_button(root,"Sprawdź",168,420,140,inspect,NULL);keyboard(root,username,password);
}
static void bind(lv_event_t *event)
{
    (void)event;const char *text=lv_textarea_get_text(name);if(!text[0]||strlen(text)>=48){dsr_sc_label_set_text(notice,"Wpisz nazwę do 47 bajtów.");return;}
    dsr_connection_t c={.credentials=credentials};snprintf(c.device.host,sizeof(c.device.host),"%s",selected.host);snprintf(c.device.name,sizeof(c.device.name),"%s",text);strcpy(c.device.device_id,selected.id);
    c.device.generation=selected.generation==1?DSR_GEN1:DSR_GEN2_PLUS;c.device.channel=channels[lv_dropdown_get_selected(channel)];
    dsr_result_t result=dsr_network_bind_verified(&c,room_ids[lv_dropdown_get_selected(room)]);memset(&c,0,sizeof(c));
    if(result==DSR_OK){lv_obj_add_state(save_button,LV_STATE_DISABLED);dsr_sc_label_set_text(notice,"Sprawdzanie urządzenia przed zapisem…");}
    else dsr_sc_label_set_text(notice,failure(result));
}
void dsr_ui_lan_bind_result(dsr_result_t result)
{if(active&&notice){if(result==DSR_AUTH_REQUIRED)show_credentials();dsr_sc_label_set_text(notice,failure(result));if(save_button)lv_obj_remove_state(save_button,LV_STATE_DISABLED);}}
static void show_components(void)
{
    lv_obj_t *root=screen("Nazwa i pomieszczenie");identity(root);
    lv_obj_t *body=form(root);name=field(body,"Nazwa",0,selected.name,false);dsr_sc_text(body,"Pomieszczenie",16,76,288,&dsr_roboto_12,DSR_SC_MUTED);
    char options[512];snprintf(options,sizeof(options),"%s",dsr_sc_tr("Nieprzypisane"));room_ids[0]=0;unsigned count=1;
    char *configuration=dsr_workspace_snapshot();if(configuration){cJSON *r=cJSON_Parse(configuration);free(configuration);const cJSON *v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(r,"rooms")){
        if(count>=9)break;
        const cJSON *id=cJSON_GetObjectItemCaseSensitive(v,"id"),*label=cJSON_GetObjectItemCaseSensitive(v,"name");if(!cJSON_IsNumber(id)||!cJSON_IsString(label))continue;
        strncat(options,"\n",sizeof(options)-strlen(options)-1);strncat(options,label->valuestring,sizeof(options)-strlen(options)-1);room_ids[count++]=(uint32_t)id->valueint;
    }cJSON_Delete(r);}
    room=lv_dropdown_create(body);lv_obj_set_pos(room,16,94);lv_obj_set_size(room,288,44);lv_dropdown_set_options(room,options);
    dsr_sc_text(body,"Kanał z odpowiedzi API",16,146,288,&dsr_roboto_12,DSR_SC_MUTED);options[0]=0;count=0;
    for(unsigned i=0;i<16;i++)if(capabilities.switches&(1u<<i)){char text[16];snprintf(text,sizeof(text),"%s%u",count?"\n":"",i);strncat(options,text,sizeof(options)-strlen(options)-1);channels[count++]=(uint8_t)i;}
    channel=lv_dropdown_create(body);lv_obj_set_pos(channel,16,164);lv_obj_set_size(channel,288,44);lv_dropdown_set_options(channel,count?options:dsr_sc_tr("Brak obsługiwanego kanału"));
    notice=dsr_sc_text(root,count?"Dodanie zachowuje poprzednie urządzenia i ich kanały.":"Urządzenie rozpoznane. Sterowanie tym typem nie jest wdrożone.",16,372,288,&dsr_roboto_12,DSR_SC_MUTED);
    dsr_ui_button(root,"Wstecz",12,420,140,results_back,NULL);save_button=dsr_ui_button(root,"Zapisz",168,420,140,bind,NULL);if(!count)lv_obj_add_state(save_button,LV_STATE_DISABLED);keyboard(root,name,NULL);
}
void dsr_ui_lan_tick(void)
{
    if(!active)return;
    dsr_lan_snapshot(&snapshot);if(snapshot.serial==observed)return;observed=snapshot.serial;
    if(waiting_scan){
        if(snapshot.phase==DSR_LAN_SCANNING){dsr_sc_label_set_text_fmt(notice,"Weryfikacja API: %u / %u",snapshot.probed,snapshot.total);return;}
        waiting_scan=false;show_results();
    }else if(waiting_inspect){
        if(snapshot.phase==DSR_LAN_INSPECTING)return;
        waiting_inspect=false;
        if(snapshot.phase==DSR_LAN_INSPECTED){capabilities=snapshot.capabilities;show_components();}
        else{show_credentials();dsr_sc_label_set_text(notice,failure(snapshot.result));}
    }
}
