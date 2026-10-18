#include "dsr_ui_internal.h"
#include "dsr_blu.h"
#include "dsr_device_settings.h"
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {INTRO,SCANNING,PAIRING,PIN,SETTINGS,EDIT,CONFIRM,LAN_SCAN,GATEWAYS,CREDENTIALS,REGISTER,RESULT,FORGET};
typedef struct {
 dsr_ble_device_t candidate;
 dsr_ble_snapshot_t scan;
 dsr_lan_snapshot_t lan;
 dsr_blu_snapshot_t snapshot;
 dsr_device_settings_snapshot_t gateway_state;
 dsr_lan_device_t gateway;
 dsr_credentials_t credentials;
 uint8_t target[6],page,field;
 uint32_t selected_id,started,value;
 bool active,target_known,register_requested,scanning;
 dsr_result_t error;
 lv_obj_t *body,*input,*username,*password,*keyboard,*choice,*notice;
} ui_t;
static ui_t *ui;
static void draw(void);
static void wipe(void *p,size_t n){volatile unsigned char *v=p;while(n--)*v++=0;}
bool dsr_ui_blu_active(void){return ui&&ui->active;}
void dsr_ui_blu_hide(void)
{if(ui){dsr_device_settings_visibility(false);if(ui->scanning)dsr_ble_cancel();if(ui->page==LAN_SCAN)dsr_lan_cancel();wipe(ui,sizeof(*ui));free(ui);ui=NULL;dsr_blu_close();dsr_device_settings_close();}}
static const char *failure(dsr_result_t r)
{return r==DSR_AUTH_REQUIRED?"Wymagane parowanie lub poprawny PIN. Przytrzymaj przycisk do niebieskiej diody.":r==DSR_CONFLICT?"Ustawienie zmieniono z innego urządzenia. Odczytaj ponownie.":r==DSR_FULL?"Brak wolnego miejsca na powiązanie":r==DSR_UNSUPPORTED?"Model lub bramka nie obsługuje tej funkcji":r==DSR_IDENTITY_MISMATCH?"Nie zgadza się tożsamość urządzenia":r==DSR_BUSY?"Operacja w toku. Poczekaj.":r==DSR_INVALID?"Sprawdź wartość i dozwolony zakres":"Nie potwierdzono operacji. Sprawdź zasięg i ponów odczyt.";}
static void back(lv_event_t *event)
{
 (void)event;if(!ui)return;
 if(ui->page==EDIT||ui->page==CONFIRM||ui->page==FORGET){ui->page=SETTINGS;draw();return;}
 if(ui->page==GATEWAYS||ui->page==CREDENTIALS||ui->page==RESULT){ui->page=ui->snapshot.bonded?SETTINGS:INTRO;draw();return;}
 uint32_t id=ui->selected_id;dsr_ui_blu_hide();if(id)dsr_network_device_settings(id);else dsr_network_device(NULL);
}
static lv_obj_t *screen(const char *title)
{
 lv_obj_t *root=lv_screen_active();lv_obj_clean(root);lv_obj_remove_style_all(root);dsr_sc_background(root);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);dsr_sc_header(root,title,false,dsr_wifi_connected(),back,NULL);
 ui->body=dsr_sc_panel(root,12,90,296,302,DSR_SC_BG,0);lv_obj_set_style_bg_opa(ui->body,LV_OPA_TRANSP,0);lv_obj_add_flag(ui->body,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(ui->body,LV_DIR_VER);dsr_sc_scrollbar(ui->body);ui->input=ui->username=ui->password=ui->keyboard=ui->choice=ui->notice=NULL;return root;
}
static lv_obj_t *button(lv_obj_t *root,const char *text,int y,lv_event_cb_t cb,uintptr_t index)
{lv_obj_t *b=dsr_sc_button(root,0,y,296,50,DSR_SC_CARD,10,cb,(void *)index);dsr_sc_text(b,text,12,16,272,&dsr_roboto_14,DSR_SC_TEXT);return b;}
static void start(lv_event_t *event)
{(void)event;if(!ui)return;if(dsr_blu_active()||!dsr_ble_scan()){ui->error=DSR_BUSY;ui->page=RESULT;}else{ui->scanning=true;ui->started=lv_tick_get();ui->page=SCANNING;}draw();}
static void select_field(lv_event_t *event)
{if(!ui)return;ui->field=(uint8_t)(uintptr_t)lv_event_get_user_data(event);ui->page=EDIT;draw();}
static void confirm(lv_event_t *event)
{
 (void)event;if(!ui)return;const dsr_blu_setting_t *f=dsr_blu_setting(ui->field);if(!f)return;
 if(f->boolean)ui->value=lv_obj_has_state(ui->choice,LV_STATE_CHECKED)?1:0;
 else {const char *text=lv_textarea_get_text(ui->input);char *end;errno=0;unsigned long value=strtoul(text,&end,10);if(!text[0]||*end||errno||value>UINT32_MAX||text[0]=='-'||!dsr_blu_value_valid(ui->field,ui->snapshot.model,(uint32_t)value)){if(ui->notice)dsr_sc_label_set_text(ui->notice,"Sprawdź wartość i dozwolony zakres");return;}ui->value=(uint32_t)value;}
 ui->page=CONFIRM;draw();
}
static void save(lv_event_t *event)
{(void)event;if(!ui)return;dsr_result_t r=dsr_blu_write(ui->snapshot.generation,ui->field,ui->value);if(r!=DSR_OK){ui->error=r;ui->page=RESULT;}else ui->page=PAIRING;draw();}
static void pin(lv_event_t *event)
{(void)event;if(!ui)return;dsr_result_t r=dsr_blu_pin(ui->snapshot.generation,lv_textarea_get_text(ui->input));if(r==DSR_OK){lv_textarea_set_text(ui->input,"");ui->page=PAIRING;draw();}else if(ui->notice)dsr_sc_label_set_text(ui->notice,"Wpisz sześciocyfrowy PIN urządzenia");}
static void forget_confirm(lv_event_t *event)
{(void)event;if(ui){ui->page=FORGET;draw();}}
static void forget(lv_event_t *event)
{(void)event;if(!ui)return;dsr_result_t r=dsr_blu_forget(ui->snapshot.generation);if(r!=DSR_OK){ui->error=r;ui->page=RESULT;}else ui->page=PAIRING;draw();}
static void gateway_scan(lv_event_t *event)
{(void)event;if(!ui)return;if(!dsr_wifi_connected()||!dsr_lan_scan()){ui->error=DSR_OFFLINE;ui->page=RESULT;}else{ui->page=LAN_SCAN;ui->started=lv_tick_get();}draw();}
static void register_gateway(lv_event_t *event)
{
 (void)event;if(!ui)return;
 if(ui->username&&ui->password){snprintf(ui->credentials.username,sizeof(ui->credentials.username),"%s",lv_textarea_get_text(ui->username));snprintf(ui->credentials.password,sizeof(ui->credentials.password),"%s",lv_textarea_get_text(ui->password));lv_textarea_set_text(ui->password,"");}
 ui->page=REGISTER;ui->register_requested=false;ui->started=lv_tick_get();draw();
}
static void choose_gateway(lv_event_t *event)
{if(!ui)return;unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(i>=ui->lan.count)return;ui->gateway=ui->lan.devices[i];wipe(&ui->credentials,sizeof(ui->credentials));if(ui->gateway.auth_required){ui->page=CREDENTIALS;draw();}else register_gateway(NULL);}
static void add_card(lv_event_t *event)
{
 (void)event;if(!ui||!ui->gateway_state.blu_registered)return;
 dsr_lan_device_t gateway=ui->gateway;dsr_credentials_t credentials=ui->credentials;unsigned cid=ui->gateway_state.blu_cid;char name[48];snprintf(name,sizeof(name),"%s",dsr_ble_model_name(&ui->candidate));
 dsr_ui_blu_hide();dsr_ui_lan_adopt_blu(&gateway,&credentials,cid,name);wipe(&credentials,sizeof(credentials));
}
static void focus(lv_event_t *event)
{if(ui&&ui->keyboard)lv_keyboard_set_textarea(ui->keyboard,lv_event_get_target(event));}
static lv_obj_t *input(lv_obj_t *root,int y,const char *value,unsigned length,bool secret)
{lv_obj_t *t=lv_textarea_create(root);dsr_ui_textarea(t);lv_obj_set_pos(t,8,y);lv_obj_set_size(t,280,46);lv_textarea_set_one_line(t,true);lv_textarea_set_max_length(t,length);lv_textarea_set_password_mode(t,secret);lv_textarea_set_text(t,value);lv_obj_add_event_cb(t,focus,LV_EVENT_FOCUSED,NULL);return t;}
static void draw(void)
{
 if(!ui)return;
 lv_obj_t *root=screen(ui->page==SETTINGS||ui->page==EDIT||ui->page==CONFIRM?"Ustawienia BLU":"Dodaj urządzenie BLU");
 if(ui->page==INTRO){dsr_sc_text(ui->body,"Potwierdź na urządzeniu",8,8,280,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(ui->body,"Pierwsze parowanie: przytrzymaj przycisk ponad 10 s, do migania niebieskiej diody. Potem uruchom skan.",8,58,280,&dsr_roboto_16,DSR_SC_TEXT);dsr_sc_text(ui->body,"Już sparowane: naciśnij krótko przycisk, aby obudzić urządzenie. Pilot zachowa istniejący PIN i szyfrowanie.",8,190,280,&dsr_roboto_14,DSR_SC_MUTED);dsr_ui_button(root,"Wstecz",12,412,142,back,NULL);dsr_ui_primary(dsr_ui_button(root,"Skanuj i połącz",166,412,142,start,NULL));return;}
 if(ui->page==SCANNING||ui->page==PAIRING||ui->page==LAN_SCAN||ui->page==REGISTER){
  const char *text=ui->page==SCANNING?"Skan Bluetooth • do 20 s":ui->page==LAN_SCAN?"Wyszukiwanie bramek w sieci":ui->page==REGISTER?"Dodawanie czujnika do bramki…":ui->snapshot.phase==DSR_BLU_QUARANTINED?"Zamykanie połączenia. Poczekaj.":ui->snapshot.phase==DSR_BLU_WRITING?"Zapisywanie i sprawdzanie ustawienia…":"Łączenie i sprawdzanie urządzenia…";
  dsr_sc_text(ui->body,text,8,36,280,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(ui->body,"Operacja trwa. Nie restartuj pilota.",8,150,280,&dsr_roboto_14,DSR_SC_MUTED);dsr_ui_button(root,"Anuluj",12,412,296,back,NULL);return;
 }
 if(ui->page==SETTINGS){
  int y=0;for(unsigned i=0;i<DSR_BLU_SETTINGS;i++)if(ui->snapshot.available&(1u<<i)){const dsr_blu_setting_t *f=dsr_blu_setting(i);if(!f)continue;char text[96];snprintf(text,sizeof(text),"%s: %lu",dsr_sc_tr(f->label),(unsigned long)ui->snapshot.values[i]);button(ui->body,dsr_sc_user(text),y,select_field,i);y+=58;}
  if(!y){dsr_sc_text(ui->body,"Sparowano. Brak dodatkowych ustawień tego modelu.",8,12,280,&dsr_roboto_16,DSR_SC_TEXT);y=90;}
  button(ui->body,"Wybierz bramkę Shelly",y,gateway_scan,0);button(ui->body,"Odczytaj ponownie",y+58,start,0);button(ui->body,"Zapomnij parowanie",y+116,forget_confirm,0);dsr_ui_button(root,"Gotowe",12,412,296,back,NULL);return;
 }
 if(ui->page==GATEWAYS){int y=0;for(unsigned i=0;i<ui->lan.count;i++){const dsr_lan_device_t *g=&ui->lan.devices[i];if(g->generation<2||g->generation>4)continue;char title[96];snprintf(title,sizeof(title),"%s · %s",g->name[0]?g->name:g->id,g->host);button(ui->body,dsr_sc_user(title),y,choose_gateway,i);y+=58;}if(!y)dsr_sc_text(ui->body,"Nie znaleziono zgodnej bramki w sieci",8,24,280,&dsr_roboto_16,DSR_SC_TEXT);dsr_ui_button(root,"Ponów wyszukiwanie",12,412,296,gateway_scan,NULL);return;}
 if(ui->page==RESULT){
  if(ui->gateway_state.blu_registered){dsr_sc_text(ui->body,ui->gateway_state.blu_receiving?"Czujnik dodany. Bramka odbiera pomiary.":"Czujnik zapisany na bramce. Odbiór pomiarów niepotwierdzony.",8,16,280,&dsr_roboto_18,DSR_SC_TEXT);if(ui->error!=DSR_OK)dsr_sc_text(ui->body,"Część konfiguracji wymaga ponownego sprawdzenia. Ponowne dodanie użyje istniejącego czujnika.",8,150,280,&dsr_roboto_14,0xffc18c);dsr_ui_button(root,"Wstecz",12,412,142,back,NULL);dsr_ui_primary(dsr_ui_button(root,"Zapisz kartę",166,412,142,add_card,NULL));}
  else{dsr_sc_text(ui->body,ui->snapshot.uncertain?"Zapis mógł dotrzeć. Wynik niepotwierdzony; odczytaj ustawienia ponownie.":failure(ui->error),8,24,280,&dsr_roboto_16,DSR_SC_TEXT);dsr_ui_button(root,"Wstecz",12,412,142,back,NULL);dsr_ui_button(root,"Ponów odczyt",166,412,142,start,NULL);}return;
 }
 if(ui->page==FORGET){dsr_sc_text(ui->body,"Usunąć powiązanie Bluetooth tego urządzenia z pilotem?",8,16,280,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(ui->body,"Karta i konfiguracja bramki pozostaną. Ponowne połączenie będzie wymagało parowania.",8,140,280,&dsr_roboto_14,DSR_SC_MUTED);dsr_ui_button(root,"Wstecz",12,412,142,back,NULL);dsr_ui_button(root,"Zapomnij",166,412,142,forget,NULL);return;}
 if(ui->page==CONFIRM){const dsr_blu_setting_t *f=dsr_blu_setting(ui->field);char text[96];snprintf(text,sizeof(text),"%s: %lu",f?dsr_sc_tr(f->label):"",(unsigned long)ui->value);dsr_sc_text(ui->body,dsr_sc_user(text),8,16,280,&dsr_roboto_18,DSR_SC_TEXT);dsr_sc_text(ui->body,"Zapisać ustawienie w czujniku BLU?",8,120,280,&dsr_roboto_16,DSR_SC_TEXT);dsr_ui_button(root,"Wstecz",12,412,142,back,NULL);dsr_ui_primary(dsr_ui_button(root,"Zapisz",166,412,142,save,NULL));return;}
 if(ui->page==EDIT){const dsr_blu_setting_t *f=dsr_blu_setting(ui->field);if(!f)return;dsr_sc_text(ui->body,f->label,8,8,280,&dsr_roboto_18,DSR_SC_TEXT);if(f->boolean){ui->choice=lv_switch_create(ui->body);lv_obj_set_pos(ui->choice,12,82);if(ui->snapshot.values[ui->field])lv_obj_add_state(ui->choice,LV_STATE_CHECKED);dsr_ui_button(root,"Wstecz",12,412,142,back,NULL);dsr_ui_button(root,"Dalej",166,412,142,confirm,NULL);return;}
  char value[16],range[64];snprintf(value,sizeof(value),"%lu",(unsigned long)ui->snapshot.values[ui->field]);ui->input=input(ui->body,65,value,10,false);snprintf(range,sizeof(range),"%lu – %lu",(unsigned long)f->minimum,(unsigned long)f->maximum);ui->notice=dsr_sc_text(ui->body,dsr_sc_user(range),8,122,280,&dsr_roboto_12,DSR_SC_MUTED);
 }else if(ui->page==PIN){dsr_sc_text(ui->body,"PIN urządzenia BLU",8,8,280,&dsr_roboto_18,DSR_SC_TEXT);ui->input=input(ui->body,62,"",6,true);lv_textarea_set_accepted_chars(ui->input,"0123456789");ui->notice=dsr_sc_text(ui->body,"Wpisz sześciocyfrowy PIN urządzenia",8,120,280,&dsr_roboto_12,DSR_SC_MUTED);
 }else if(ui->page==CREDENTIALS){lv_obj_set_height(ui->body,174);dsr_sc_text(ui->body,"Login",8,0,280,&dsr_roboto_12,DSR_SC_MUTED);ui->username=input(ui->body,20,"admin",32,false);dsr_sc_text(ui->body,"Hasło Shelly",8,76,280,&dsr_roboto_12,DSR_SC_MUTED);ui->password=input(ui->body,96,"",64,true);ui->input=ui->password;}
 lv_obj_set_height(ui->body,174);ui->keyboard=lv_keyboard_create(root);dsr_ui_keyboard(ui->keyboard);lv_obj_align(ui->keyboard,LV_ALIGN_BOTTOM_MID,0,0);lv_keyboard_set_textarea(ui->keyboard,ui->input);lv_keyboard_set_mode(ui->keyboard,ui->page==CREDENTIALS?LV_KEYBOARD_MODE_TEXT_LOWER:LV_KEYBOARD_MODE_NUMBER);
 lv_event_cb_t forward=ui->page==PIN?pin:ui->page==CREDENTIALS?register_gateway:confirm;lv_obj_add_event_cb(ui->keyboard,forward,LV_EVENT_READY,NULL);dsr_ui_button(root,"Wstecz",12,265,142,back,NULL);dsr_ui_primary(dsr_ui_button(root,"Dalej",166,265,142,forward,NULL));
}
static bool target_matches(const dsr_ble_device_t *d)
{
 if(!dsr_blu_supported(d))return false;
 if(ui->target_known){uint8_t address[6];if(d->has_mac)memcpy(address,d->mac,6);else for(unsigned i=0;i<6;i++)address[i]=d->address[5-i];return !memcmp(address,ui->target,6);}
 return d->address_type==ui->candidate.address_type&&!memcmp(d->address,ui->candidate.address,6);
}
void dsr_ui_blu_open(const dsr_ble_device_t *candidate)
{
 dsr_ble_device_t copy={0};if(candidate)copy=*candidate;if(candidate&&!dsr_blu_supported(candidate))return;if(dsr_network_ota_busy())return;
 dsr_network_home(NULL);dsr_mobile_hide();
#ifdef ESP_PLATFORM
 ui=heap_caps_calloc(1,sizeof(*ui),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
 ui=calloc(1,sizeof(*ui));
#endif
 if(!ui){dsr_network_device(NULL);return;}ui->candidate=copy;ui->active=true;dsr_device_settings_visibility(true);ui->page=INTRO;if(copy.has_mac){memcpy(ui->target,copy.mac,6);ui->target_known=true;}draw();
}
void dsr_ui_blu_settings(uint32_t id)
{
 dsr_mobile_device_t device;if(!dsr_network_typed_device(id,&device)||device.endpoint_kind!=DSR_ENDPOINT_BLU)return;
 const char *mac=strrchr(device.identity,'_');if(!mac||strlen(++mac)!=12)return;uint8_t address[6];for(unsigned i=0;i<6;i++){unsigned byte;if(sscanf(mac+i*2,"%2x",&byte)!=1)return;address[i]=(uint8_t)byte;}
 dsr_ui_blu_open(NULL);if(ui){memcpy(ui->target,address,6);ui->target_known=true;ui->selected_id=id;}
}
void dsr_ui_blu_tick(void)
{
 if(!ui||!ui->active)return;
 if(ui->page==SCANNING){
  dsr_ble_snapshot(&ui->scan);if(ui->scan.busy||ui->scan.phase==DSR_BLE_SCANNING||ui->scan.phase==DSR_BLE_STARTING)return;
  ui->scanning=false;bool found=false;for(unsigned i=0;i<ui->scan.count;i++)if(target_matches(&ui->scan.devices[i])){ui->candidate=ui->scan.devices[i];found=true;break;}
  if(!found){ui->error=DSR_OFFLINE;ui->page=RESULT;draw();return;}
  dsr_result_t r=dsr_blu_start(&ui->candidate);if(r==DSR_BUSY&&lv_tick_get()-ui->started<25000)return;
  if(r==DSR_OK){dsr_blu_snapshot(&ui->snapshot);ui->page=PAIRING;}else{ui->error=r;ui->page=RESULT;}draw();return;
 }
 if(ui->page==PAIRING||ui->page==PIN){
  dsr_blu_snapshot_t next;dsr_blu_snapshot(&next);bool changed=next.phase!=ui->snapshot.phase;ui->snapshot=next;
  if(next.phase==DSR_BLU_PIN&&ui->page!=PIN){ui->page=PIN;draw();return;}
  if(next.active){if(changed&&ui->page!=PIN)draw();return;}
  if(next.result==DSR_OK&&next.bonded)ui->page=SETTINGS;else{ui->error=next.result;ui->page=next.result==DSR_OK?INTRO:RESULT;}draw();return;
 }
 if(ui->page==LAN_SCAN){dsr_lan_snapshot(&ui->lan);if(ui->lan.phase==DSR_LAN_SCANNING)return;ui->page=GATEWAYS;draw();return;}
 if(ui->page==REGISTER){
  if(!ui->register_requested){dsr_result_t r=dsr_network_blu_gateway_request(&ui->gateway,&ui->credentials,ui->snapshot.generation);if(r==DSR_BUSY&&lv_tick_get()-ui->started<30000)return;if(r!=DSR_OK){ui->error=r;ui->page=RESULT;draw();return;}ui->register_requested=true;return;}
  dsr_device_settings_snapshot(&ui->gateway_state);if(ui->gateway_state.busy)return;ui->error=ui->gateway_state.result;ui->page=RESULT;draw();
 }
}
