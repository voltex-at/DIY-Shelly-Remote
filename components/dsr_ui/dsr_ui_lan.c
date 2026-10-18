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
static uint8_t channels[64],channel_count;
static void show_results(void);
static void show_credentials(void);
static void show_components(void);
static bool blu_gateway(void){return !strcmp(selected.model,"S3GW-1DBT001");}
bool dsr_ui_lan_active(void){return active;}

void dsr_ui_lan_reset(void)
{
    active=false;notice=username=password=name=room=channel=save_button=form_body=NULL;
    if(!building){memset(&credentials,0,sizeof(credentials));waiting_scan=waiting_inspect=false;}
}
static char auto_name[48];
static dsr_blu_labels_t blu_labels;
/* Gateway name, else verified model, else generic; MAC tail tells twins apart. */
static void blu_label(unsigned i,char *out,size_t size)
{
    const dsr_blu_label_t *l=&blu_labels.items[i&15];const char *model=dsr_blu_model_name(l->model);
    if(!(blu_labels.valid&(1u<<(i&15))))snprintf(out,size,"%s %u",dsr_sc_tr("Czujnik BLU"),(i&15)+1);
    else if(l->name[0])snprintf(out,size,"%s",l->name);
    else snprintf(out,size,"%s %02x%02x",model?model:dsr_sc_tr("Czujnik BLU"),l->mac_tail[0],l->mac_tail[1]);
}
/* A BLU channel gets a sensor name instead of the gateway's, unless the user typed one. */
static void channel_changed(lv_event_t *e)
{
    (void)e;unsigned i=lv_dropdown_get_selected(channel);if(i>=channel_count||strcmp(lv_textarea_get_text(name),auto_name))return;
    char next[48];if(channels[i]&32)blu_label(channels[i]&15,next,sizeof(next));else snprintf(next,sizeof(next),"%s",selected.name);
    lv_textarea_set_text(name,next);snprintf(auto_name,sizeof(auto_name),"%s",next);
}
static lv_obj_t *screen(const char *title,unsigned step)
{building=true;lv_obj_t *root=dsr_onboarding_screen(title,step);building=false;active=true;return root;}
static void leave(lv_event_t *event){(void)event;dsr_lan_cancel();dsr_network_device(NULL);}
static void results_back(lv_event_t *event){(void)event;memset(&credentials,0,sizeof(credentials));show_results();}
static const char *failure(dsr_result_t result)
{
    return result==DSR_CONFLICT?"Konfiguracja zmieniła się. Wczytaj ją i ponów dodawanie urządzenia.":result==DSR_BUSY?"Operacja w toku. Poczekaj.":result==DSR_AUTH_REQUIRED?"Błąd logowania. Sprawdź dane.":result==DSR_IDENTITY_MISMATCH?"Tożsamość urządzenia się zmieniła. Wyszukaj ponownie.":result==DSR_RATE_LIMITED?"Limit logowań Shelly. Poczekaj i odśwież.":result==DSR_OFFLINE?"Shelly offline lub brak Wi-Fi":
        /* Specific causes instead of one generic "verification failed": a
         * BLU gateway or a full registry is not a verification failure. */
        result==DSR_UNSUPPORTED?"To urządzenie nie ma obsługiwanego kanału sterowania (np. bramka BLU). Nie dodano.":
        result==DSR_FULL?"Limit 16 urządzeń. Usuń jedno, aby dodać nowe.":
        result==DSR_BAD_RESPONSE?"Nieprawidłowa odpowiedź urządzenia. Nie dodano.":
        result==DSR_IO?"Błąd pamięci pilota. Nie dodano; zapisane dane bez zmian.":
        result==DSR_INVALID?"Sprawdź adres IP, kanał i długość pól.":"Nie udało się zweryfikować urządzenia.";
}
static lv_obj_t *field(lv_obj_t *root,const char *caption,int y,const char *text,bool secret)
{
    dsr_sc_text(root,caption,16,y-1,288,&dsr_roboto_14,DSR_SC_MUTED);lv_obj_t *f=lv_textarea_create(root);dsr_ui_textarea(f);
    lv_obj_set_pos(f,16,y+18);lv_obj_set_size(f,288,44);lv_textarea_set_one_line(f,true);lv_textarea_set_max_length(f,secret?64:47);lv_textarea_set_password_mode(f,secret);lv_textarea_set_text(f,text);return f;
}
/* Form below the 84 px device card; with the keyboard (240 px) it moves up
 * over the card so the focused field stays visible. */
#define FORM_Y 176
#define FORM_H 200
static void keyboard_close(lv_event_t *event){lv_obj_t *kb=lv_event_get_user_data(event);lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);lv_keyboard_set_textarea(kb,NULL);dsr_onboarding_keyboard(lv_screen_active(),false);if(form_body){lv_obj_set_y(form_body,FORM_Y);lv_obj_set_height(form_body,FORM_H);}}
static void focus(lv_event_t *event)
{lv_obj_t *kb=lv_event_get_user_data(event);dsr_onboarding_keyboard(lv_screen_active(),true);if(form_body){lv_obj_set_y(form_body,154);lv_obj_set_height(form_body,82);}dsr_ui_keyboard_focus(kb,lv_event_get_target(event));lv_obj_scroll_to_view(lv_event_get_target(event),LV_ANIM_OFF);}
static void keyboard(lv_obj_t *root,lv_obj_t *a,lv_obj_t *b)
{
    lv_obj_t *kb=lv_keyboard_create(root);dsr_ui_keyboard(kb);lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb,keyboard_close,LV_EVENT_READY,kb);lv_obj_add_event_cb(kb,keyboard_close,LV_EVENT_CANCEL,kb);
    lv_obj_add_event_cb(a,focus,LV_EVENT_CLICKED,kb);lv_obj_add_event_cb(a,focus,LV_EVENT_FOCUSED,kb);if(b){lv_obj_add_event_cb(b,focus,LV_EVENT_CLICKED,kb);lv_obj_add_event_cb(b,focus,LV_EVENT_FOCUSED,kb);}
}
static void toggle_secret(lv_event_t *event){if(password)lv_textarea_set_password_mode(password,!lv_obj_has_state(lv_event_get_target(event),LV_STATE_CHECKED));}
static lv_obj_t *form(lv_obj_t *root)
{form_body=dsr_sc_panel(root,0,FORM_Y,320,FORM_H,DSR_SC_BG,0);lv_obj_add_flag(form_body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(form_body,LV_DIR_VER);dsr_sc_scrollbar(form_body);return form_body;}
/* Product line from the SKU prefix only (no guessed model names). */
static const char *family(const char *sku)
{return !strncmp(sku,"SP",2)?"Shelly Pro":!strncmp(sku,"SN",2)?"Shelly Plus":!strncmp(sku,"S3",2)?"Shelly Gen3":!strncmp(sku,"S4",2)?"Shelly Gen4":!strncmp(sku,"SH",2)?"Shelly Gen1":"Shelly";}
static void line(lv_obj_t *card,const char *text,int y,const lv_font_t *font,uint32_t color)
{lv_obj_t *l=dsr_sc_text(card,dsr_sc_user(text),52,y,236,font,color);lv_obj_set_height(l,font==&dsr_roboto_16?20:17);lv_label_set_long_mode(l,LV_LABEL_LONG_MODE_DOTS);}
/* Owner 2026-10-18: show what the remote actually knows about the device
 * (name set in Shelly, SKU and line, address, MAC, channels, password). */
static void identity(lv_obj_t *root)
{
    lv_obj_t *card=dsr_sc_panel(root,12,88,296,84,DSR_SC_CARD,12);dsr_sc_icon(card,blu_gateway()?DSR_ICON_INFO:DSR_ICON_PLUG,10,24,32,DSR_SC_TEXT);
    char text[96];
    bool named=selected.name[0]&&strcmp(selected.name,selected.model); /* name defaults to the SKU when unset */
    snprintf(text,sizeof(text),"%s",named?selected.name:family(selected.model));line(card,text,6,&dsr_roboto_16,DSR_SC_TEXT);
    if(named)snprintf(text,sizeof(text),"%s • %s • Gen %u",family(selected.model),selected.model,selected.generation);else snprintf(text,sizeof(text),"%s • Gen %u",selected.model,selected.generation);
    line(card,text,28,&dsr_roboto_14,DSR_SC_MUTED);
    char mac[18]="";if(strlen(selected.mac)==12)snprintf(mac,sizeof(mac),"%.2s:%.2s:%.2s:%.2s:%.2s:%.2s",selected.mac,selected.mac+2,selected.mac+4,selected.mac+6,selected.mac+8,selected.mac+10);
    unsigned switches=0,covers=0,lights=0,rgbs=0,blu=0;for(unsigned i=0;i<16;i++){switches+=(capabilities.switches>>i)&1u;covers+=(capabilities.covers>>i)&1u;lights+=(capabilities.lights>>i)&1u;rgbs+=(capabilities.rgbs>>i)&1u;blu+=(capabilities.blu>>i)&1u;}
    char parts[64]="";const unsigned counts[5]={switches,covers,lights,rgbs,blu};const char *const kinds[5]={"Switch","Cover","Light","RGB","BLU"};
    for(unsigned k=0;k<5;k++)if(counts[k]){size_t used=strlen(parts);snprintf(parts+used,sizeof(parts)-used,"%s%s: %u",used?" • ":"",kinds[k],counts[k]);}
    if(parts[0])snprintf(text,sizeof(text),"%s • %s%s",selected.host,parts,selected.auth_required?dsr_sc_tr(" • hasło"):"");
    else snprintf(text,sizeof(text),"%s%s%s%s",selected.host,mac[0]?" • ":"",mac,selected.auth_required?dsr_sc_tr(" • hasło"):"");
    line(card,text,50,&dsr_roboto_14,DSR_SC_MUTED);
}
static void inspect(lv_event_t *event)
{
    (void)event;if(username&&password){const char *u=lv_textarea_get_text(username),*p=lv_textarea_get_text(password);
        if(strlen(u)>=sizeof(credentials.username)||strlen(p)>=sizeof(credentials.password)){dsr_sc_label_set_text(notice,"Dane logowania są za długie.");return;}
        strcpy(credentials.username,u);strcpy(credentials.password,p);}
    if(!dsr_lan_inspect(selected.host,selected.id,&credentials)){dsr_sc_label_set_text(notice,"Operacja w toku. Poczekaj.");return;}
    waiting_inspect=true;observed=0;lv_obj_t *root=screen("Sprawdź urządzenie",2);identity(root);
    notice=dsr_sc_text(root,"Odczyt możliwości urządzenia…",16,190,288,&dsr_roboto_14,DSR_SC_MUTED);dsr_onboarding_footer(root,"Anuluj",leave,NULL,NULL);
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
    lv_obj_t *root=screen("Szukaj Shelly w sieci",2);notice=dsr_sc_text(root,message,16,118,288,&dsr_roboto_14,DSR_SC_TEXT);
    if(ok){lv_obj_t *spinner=lv_spinner_create(root);lv_obj_set_pos(spinner,136,212);lv_obj_set_size(spinner,48,48);lv_spinner_set_anim_params(spinner,1000,200);}
    dsr_onboarding_footer(root,"Anuluj",leave,"Ponów",start);
}
void dsr_ui_lan_open(lv_event_t *event){start(event);}
static void show_results(void)
{
    dsr_lan_snapshot(&snapshot);lv_obj_t *root=screen("Znalezione urządzenia",2);
    lv_obj_t *list=dsr_sc_panel(root,12,86,296,274,DSR_SC_BG,0);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(list,LV_DIR_VER);dsr_sc_scrollbar(list);
    for(unsigned i=0;i<snapshot.count;i++){
        const dsr_lan_device_t *d=&snapshot.devices[i];char text[120];if(d->name[0])snprintf(text,sizeof(text),"%s • %s • Gen %u",d->name,d->host,d->generation);else snprintf(text,sizeof(text),"%s • Gen %u",d->host,d->generation);
        const char *state=dsr_network_endpoint_added(d->id,d->generation==1?DSR_GEN1:DSR_GEN2_PLUS,UINT32_MAX)?"Już dodane":d->auth_required?"API • wymaga hasła":"Zweryfikowano API";
        dsr_onboarding_result(list,(int)i*96,!strcmp(d->model,"S3GW-1DBT001")?DSR_ICON_INFO:DSR_ICON_PLUG,d->model,text,state,true,choose,(void *)(uintptr_t)i);
    }
    if(!snapshot.count)dsr_sc_text(list,snapshot.phase==DSR_LAN_NO_WIFI?"Najpierw połącz Wi-Fi":"Nie znaleziono Shelly. Sprawdź sieć i ponów.",12,40,272,&dsr_roboto_16,DSR_SC_TEXT);
    notice=dsr_sc_text(root,snapshot.phase==DSR_LAN_CANCELLED?"Wyszukiwanie przerwane":"Zakończono wyszukiwanie",16,372,288,&dsr_roboto_12,DSR_SC_MUTED);
    dsr_onboarding_footer(root,"Wstecz",leave,"Ponów",start);
}
static void show_credentials(void)
{
    lv_obj_t *root=screen("Sprawdź urządzenie",2);identity(root);
    lv_obj_t *body=form(root);username=field(body,"Login",0,"admin",false);password=field(body,"Hasło Shelly",76,"",true);
    lv_obj_t *show=lv_checkbox_create(body);lv_obj_set_pos(show,16,152);lv_checkbox_set_text(show,dsr_sc_tr("Show"));lv_obj_add_event_cb(show,toggle_secret,LV_EVENT_VALUE_CHANGED,NULL);
    notice=dsr_sc_text(root,"Dane logowania służą tylko wybranemu urządzeniu.",16,378,288,&dsr_roboto_12,DSR_SC_MUTED);
    dsr_onboarding_footer(root,"Wstecz",results_back,"Sprawdź",inspect);keyboard(root,username,password);
}
static void bind(lv_event_t *event)
{
    (void)event;bool blu_choice=channel_count&&lv_dropdown_get_selected(channel)<channel_count&&(channels[lv_dropdown_get_selected(channel)]&32);
    if((blu_gateway()&&!blu_choice)||!channel_count){dsr_sc_label_set_text(notice,blu_gateway()?"Bramka BLU • przekaźnik nieobsługiwany.":"Urządzenie rozpoznane. Sterowanie tym typem nie jest wdrożone.");return;}
    unsigned selected_channel=lv_dropdown_get_selected(channel);if(selected_channel>=channel_count)return;
    unsigned code=channels[selected_channel];bool supported=(code&32)?(capabilities.blu&(1u<<(code&15)))!=0:(code&16)?capabilities.ht:((((code&192)==192)?capabilities.rgbs:code&128?capabilities.covers:code&64?capabilities.lights:capabilities.switches)&(1u<<(code&15)))!=0;if(!supported)return;
    const char *text=lv_textarea_get_text(name);if(!text[0]||strlen(text)>=48){dsr_sc_label_set_text(notice,"Wpisz nazwę do 47 bajtów.");return;}
    dsr_connection_t c={.credentials=credentials};snprintf(c.device.host,sizeof(c.device.host),"%s",selected.host);snprintf(c.device.name,sizeof(c.device.name),"%s",text);strcpy(c.device.device_id,selected.id);
    c.device.generation=selected.generation==1?DSR_GEN1:DSR_GEN2_PLUS;c.device.channel=channels[lv_dropdown_get_selected(channel)]&15;c.device.endpoint_kind=code&32?DSR_ENDPOINT_BLU:code&16?DSR_ENDPOINT_HT:(code&192)==192?DSR_ENDPOINT_RGB:(code&128)?DSR_ENDPOINT_COVER:(code&64)?DSR_ENDPOINT_LIGHT:DSR_ENDPOINT_SWITCH;
    dsr_result_t result=dsr_network_bind_verified(&c,room_ids[lv_dropdown_get_selected(room)]);memset(&c,0,sizeof(c));
    if(result==DSR_OK){lv_obj_add_state(save_button,LV_STATE_DISABLED);dsr_sc_label_set_text(notice,"Sprawdzanie urządzenia przed zapisem…");}
    else dsr_sc_label_set_text(notice,failure(result));
}
void dsr_ui_lan_bind_result(dsr_result_t result,dsr_bind_stage_t stage)
{if(active&&notice){if(result==DSR_AUTH_REQUIRED&&stage==DSR_BIND_VERIFY)show_credentials();dsr_sc_label_set_text(notice,stage==DSR_BIND_VERIFY?failure(result):dsr_network_bind_result_text(result,stage));if(save_button)lv_obj_remove_state(save_button,LV_STATE_DISABLED);}}
static void show_components(void)
{
    lv_obj_t *root=screen("Nazwa i pomieszczenie",3);identity(root);
    lv_obj_t *body=form(root);name=field(body,"Nazwa",0,selected.name,false);dsr_sc_text(body,"Pomieszczenie",16,68,288,&dsr_roboto_14,DSR_SC_MUTED);
    static char options[1024];snprintf(options,sizeof(options),"%s",dsr_sc_tr("Nieprzypisane"));room_ids[0]=0;unsigned count=1;
    char *configuration=dsr_workspace_snapshot();if(configuration){cJSON *r=cJSON_Parse(configuration);free(configuration);const cJSON *v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(r,"rooms")){
        if(count>=9)break;
        const cJSON *id=cJSON_GetObjectItemCaseSensitive(v,"id"),*label=cJSON_GetObjectItemCaseSensitive(v,"name");if(!cJSON_IsNumber(id)||!cJSON_IsString(label))continue;
        strncat(options,"\n",sizeof(options)-strlen(options)-1);strncat(options,label->valuestring,sizeof(options)-strlen(options)-1);room_ids[count++]=(uint32_t)id->valueint;
    }cJSON_Delete(r);}
    room=lv_dropdown_create(body);lv_obj_set_pos(room,16,88);lv_obj_set_size(room,288,44);lv_dropdown_set_options(room,options);
    dsr_sc_text(body,"Kanał z odpowiedzi API",16,136,288,&dsr_roboto_14,DSR_SC_MUTED);options[0]=0;count=0;
    for(unsigned kind=0;kind<4;kind++)for(unsigned i=0;i<16;i++)if((kind?(selected.generation>=2?(kind==1?capabilities.covers:kind==2?capabilities.lights:capabilities.rgbs):0):capabilities.switches)&(1u<<i)){char text[24];snprintf(text,sizeof(text),"%s%s %u",count?"\n":"",kind==1?"Cover":kind==2?"Light":kind==3?"RGB":"Switch",i);strncat(options,text,sizeof(options)-strlen(options)-1);channels[count++]=(uint8_t)(i|(kind==1?128:kind==2?64:kind==3?192:0));}
    if(capabilities.ht&&selected.generation==3&&!strcmp(selected.model,DSR_HT_MODEL)&&count<sizeof(channels)){strncat(options,count?"\nH&T 0":"H&T 0",sizeof(options)-strlen(options)-1);channels[count++]=16;}
    /* Shelly BLU sensors known to this gateway (BTHomeDevice cid 200+i). */
    for(unsigned i=0;i<16&&count<sizeof(channels);i++)if(selected.generation>=2&&(capabilities.blu&(1u<<i))){char label[48],text[52];blu_label(i,label,sizeof(label));snprintf(text,sizeof(text),"%s%s",count?"\n":"",label);strncat(options,text,sizeof(options)-strlen(options)-1);channels[count++]=(uint8_t)(32|i);}
    channel_count=(uint8_t)count;channel=lv_dropdown_create(body);lv_obj_set_pos(channel,16,156);lv_obj_set_size(channel,288,44);lv_dropdown_set_options(channel,count?options:dsr_sc_tr("Brak obsługiwanego kanału"));
    snprintf(auto_name,sizeof(auto_name),"%s",lv_textarea_get_text(name));lv_obj_add_event_cb(channel,channel_changed,LV_EVENT_VALUE_CHANGED,NULL);if(count)channel_changed(NULL);
    notice=dsr_sc_text(root,capabilities.blu?"Czujniki BLU są odczytywane przez tę bramkę Shelly.":blu_gateway()?"Bramka BLU • przekaźnik nieobsługiwany.":count?"Dodanie zachowuje poprzednie urządzenia i ich kanały.":"Urządzenie rozpoznane. Sterowanie tym typem nie jest wdrożone.",16,378,288,&dsr_roboto_12,DSR_SC_MUTED);
    save_button=dsr_onboarding_footer(root,"Wstecz",results_back,"Zapisz",bind);if(!count||(blu_gateway()&&!capabilities.blu))lv_obj_add_state(save_button,LV_STATE_DISABLED);keyboard(root,name,NULL);
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
        if(snapshot.phase==DSR_LAN_INSPECTED&&snapshot.selected.generation==selected.generation&&!strcmp(snapshot.selected.id,selected.id)&&!strcmp(snapshot.selected.host,selected.host)){capabilities=snapshot.capabilities;dsr_lan_blu_labels(&blu_labels);show_components();}
        else{show_credentials();dsr_sc_label_set_text(notice,failure(snapshot.result));}
    }
}

void dsr_ui_lan_adopt(const dsr_lan_device_t *device,const dsr_lan_capabilities_t *caps)
{if(!device||!caps)return;selected=*device;capabilities=*caps;memset(&blu_labels,0,sizeof(blu_labels));memset(&credentials,0,sizeof(credentials));waiting_scan=waiting_inspect=false;show_components();}
