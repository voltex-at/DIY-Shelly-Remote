#include "dsr_device_settings.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define B(n,p,l) {n,p,l,NULL,0,1,0,DSR_SETTING_BOOL,0}
#define N(n,p,l,a,b) {n,p,l,NULL,a,b,0,DSR_SETTING_NUMBER,0}
#define I(n,p,l,a,b) {n,p,l,NULL,a,b,0,DSR_SETTING_NUMBER,DSR_SETTING_INTEGER}
#define T(n,p,l,k) {n,p,l,NULL,0,0,k,DSR_SETTING_TEXT,0}
#define E(n,p,l,c) {n,p,l,c,0,0,0,DSR_SETTING_ENUM,0}
#define S(n,p,l,k) {n,p,l,NULL,0,0,k,DSR_SETTING_SECRET,DSR_SETTING_NETWORK}
/* Allowlisted config leaves only: never expose reset, firmware upload, scripts,
 * auth/CA replacement or arbitrary RPC through this editor. A field must also
 * exist in the live GetConfig (write-only secrets have explicit exceptions). */
static const dsr_setting_field_t schema[]={
 T("Sys","device.name","Nazwa urządzenia",64),B("Sys","device.eco_mode","Tryb ECO"),T("Sys","location.tz","Strefa czasowa",48),N("Sys","location.lat","Szerokość geograficzna",-90,90),N("Sys","location.lon","Długość geograficzna",-180,180),T("Sys","sntp.server","Serwer czasu",100),
 T("Switch","name","Nazwa kanału",64),E("Switch","initial_state","Stan po zasileniu","off\non\nrestore_last\nmatch_input"),E("Switch","in_mode","Tryb wejścia","momentary\nfollow\nflip\ndetached\ncycle\nactivate"),B("Switch","in_locked","Blokada wejścia"),B("Switch","auto_on","Automatyczne włączenie"),N("Switch","auto_on_delay","Opóźnienie włączenia (s)",0.1,86400),B("Switch","auto_off","Automatyczne wyłączenie"),N("Switch","auto_off_delay","Opóźnienie wyłączenia (s)",0.1,86400),B("Switch","power_restore_delay_enable","Opóźnienie po zasileniu"),I("Switch","power_restore_delay","Czas po zasileniu (s)",1,600),
 T("Input","name","Nazwa wejścia",64),E("Input","type","Typ wejścia","switch\nbutton\nanalog\ncount"),B("Input","enable","Wejście włączone"),B("Input","invert","Odwrócenie wejścia"),
 T("Cover","name","Nazwa rolety",64),E("Cover","in_mode","Tryb wejścia","single\ndual\ndetached"),E("Cover","initial_state","Stan po zasileniu","stopped\nopen\nclosed"),B("Cover","in_locked","Blokada wejścia"),B("Cover","swap_inputs","Zamiana wejść"),B("Cover","invert_directions","Odwrócenie kierunków"),N("Cover","maxtime_open","Maks. otwieranie (s)",0.1,300),N("Cover","maxtime_close","Maks. zamykanie (s)",0.1,300),B("Cover","slat.enable","Sterowanie lamelami"),N("Cover","slat.open_time","Obrót lameli: otwieranie (s)",0.5,30),N("Cover","slat.close_time","Obrót lameli: zamykanie (s)",0.5,30),I("Cover","slat.step","Krok lameli (%)",1,100),B("Cover","slat.retain_pos","Zachowaj pozycję lameli"),
 T("Light","name","Nazwa światła",64),E("Light","initial_state","Stan po zasileniu","off\non\nrestore_last\ntoggle"),B("Light","auto_on","Automatyczne włączenie"),N("Light","auto_on_delay","Opóźnienie włączenia (s)",0.1,86400),B("Light","auto_off","Automatyczne wyłączenie"),N("Light","auto_off_delay","Opóźnienie wyłączenia (s)",0.1,86400),N("Light","transition_duration","Czas przejścia (s)",0,60),N("Light","min_brightness_on_toggle","Minimalna jasność (%)",0,100),B("Light","night_mode.enable","Tryb nocny"),N("Light","night_mode.brightness","Jasność nocna (%)",0,100),N("Light","gamma","Korekcja gamma",0.4,4),I("Light","button_fade_rate","Tempo ściemniania przyciskiem",1,5),
 T("RGB","name","Nazwa światła",64),E("RGB","initial_state","Stan po zasileniu","off\non\nrestore_last\ntoggle"),B("RGB","auto_on","Automatyczne włączenie"),N("RGB","auto_on_delay","Opóźnienie włączenia (s)",0.1,86400),B("RGB","auto_off","Automatyczne wyłączenie"),N("RGB","auto_off_delay","Opóźnienie wyłączenia (s)",0.1,86400),N("RGB","transition_duration","Czas przejścia (s)",0,60),B("RGB","night_mode.enable","Tryb nocny"),N("RGB","night_mode.brightness","Jasność nocna (%)",0,100),
 T("RGBW","name","Nazwa światła",64),E("RGBW","initial_state","Stan po zasileniu","off\non\nrestore_last\ntoggle"),B("RGBW","auto_on","Automatyczne włączenie"),N("RGBW","auto_on_delay","Opóźnienie włączenia (s)",0.1,86400),B("RGBW","auto_off","Automatyczne wyłączenie"),N("RGBW","auto_off_delay","Opóźnienie wyłączenia (s)",0.1,86400),N("RGBW","transition_duration","Czas przejścia (s)",0,60),
 B("WiFi","sta.enable","Wi-Fi podstawowe"),T("WiFi","sta.ssid","Nazwa sieci podstawowej",32),S("WiFi","sta.pass","Hasło sieci podstawowej",63),E("WiFi","sta.ipv4mode","Adresacja IP","dhcp\nstatic"),T("WiFi","sta.ip","Adres IP",15),T("WiFi","sta.netmask","Maska podsieci",15),T("WiFi","sta.gw","Brama sieciowa",15),T("WiFi","sta.nameserver","Serwer DNS",15),B("WiFi","sta1.enable","Wi-Fi zapasowe"),T("WiFi","sta1.ssid","Nazwa sieci zapasowej",32),S("WiFi","sta1.pass","Hasło sieci zapasowej",63),B("WiFi","ap.enable","Punkt dostępowy"),T("WiFi","ap.ssid","Nazwa punktu dostępowego",29),S("WiFi","ap.pass","Hasło punktu dostępowego",63),B("WiFi","ap.range_extender.enable","Rozszerzanie zasięgu"),
 B("BLE","rpc.enable","Sterowanie przez Bluetooth"),B("Cloud","enable","Połączenie z Shelly Cloud"),
 B("MQTT","enable","Połączenie MQTT"),T("MQTT","server","Serwer MQTT",100),T("MQTT","user","Użytkownik MQTT",64),S("MQTT","pass","Hasło MQTT",100),T("MQTT","topic_prefix","Prefiks MQTT",100),B("MQTT","rpc_ntf","Powiadomienia RPC"),B("MQTT","status_ntf","Powiadomienia statusu"),B("MQTT","enable_rpc","RPC przez MQTT"),B("MQTT","enable_control","Sterowanie przez MQTT"),
 T("Temperature","name","Nazwa pomiaru",64),N("Temperature","offset_C","Korekta temperatury (°C)",-20,20),N("Temperature","report_thr_C","Próg temperatury (°C)",0.1,10),T("Humidity","name","Nazwa pomiaru",64),N("Humidity","offset","Korekta wilgotności (%)",-50,50),N("Humidity","report_thr","Próg wilgotności (%)",0.1,50),
 B("TRV","enable","Termostat włączony"),B("TRV","override_enable","Nadpisanie temperatury"),I("TRV","default_boost_duration","Domyślny boost (s)",1,86400),I("TRV","default_override_duration","Domyślne nadpisanie (s)",1,86400),B("TRV","flags.anticlog","Ochrona przed zablokowaniem"),B("TRV","flags.auto_calibrate","Automatyczna kalibracja"),B("TRV","flags.power_save","Oszczędzanie baterii"),B("TRV","flags.silent_mode","Tryb cichy"),
};
const dsr_setting_field_t *dsr_setting_field(unsigned index)
{return index<sizeof(schema)/sizeof(*schema)?&schema[index]:NULL;}
const cJSON *dsr_setting_value(const cJSON *root,const char *path)
{
 if(!root||!path)return NULL;
 while(*path){char key[48];const char *end=strchr(path,'.');size_t n=end?(size_t)(end-path):strlen(path);if(!n||n>=sizeof(key))return NULL;memcpy(key,path,n);key[n]=0;root=cJSON_GetObjectItemCaseSensitive(root,key);if(!root)return NULL;if(!end)return root;path=end+1;}
 return NULL;
}
static bool choice(const char *choices,const char *value)
{if(!choices||!value)return false;size_t n=strlen(value);while(*choices){const char *end=strchr(choices,'\n');size_t k=end?(size_t)(end-choices):strlen(choices);if(n==k&&!memcmp(choices,value,n))return true;if(!end)break;choices=end+1;}return false;}
static bool present(const dsr_setting_field_t *f,const cJSON *configuration)
{
 if(f->type==DSR_SETTING_SECRET){char path[48];snprintf(path,sizeof(path),"%s",f->path);char *last=strrchr(path,'.');if(last){*last=0;return cJSON_IsObject(dsr_setting_value(configuration,path));}return cJSON_IsObject(configuration);}
 const cJSON *v=dsr_setting_value(configuration,f->path);
 if(!v)return false;
 return f->type==DSR_SETTING_BOOL?cJSON_IsBool(v):f->type==DSR_SETTING_NUMBER?cJSON_IsNumber(v)||cJSON_IsNull(v):cJSON_IsString(v)||cJSON_IsNull(v);
}
dsr_result_t dsr_setting_patch(const cJSON *configuration,unsigned field,const char *text,cJSON **out)
{
 if(out)*out=NULL;
 const dsr_setting_field_t *f=dsr_setting_field(field);
 if(!out||!f||!text||strlen(text)>128||!present(f,configuration))return DSR_INVALID;
 cJSON *value=NULL;
 if(f->type==DSR_SETTING_BOOL){if(strcmp(text,"true")&&strcmp(text,"false"))return DSR_INVALID;value=cJSON_CreateBool(!strcmp(text,"true"));}
 else if(f->type==DSR_SETTING_NUMBER){char *end;errno=0;double n=strtod(text,&end);if(!text[0]||isspace((unsigned char)text[0])||*end||errno||!isfinite(n)||n<f->minimum||n>f->maximum||((f->flags&DSR_SETTING_INTEGER)&&floor(n)!=n))return DSR_INVALID;value=cJSON_CreateNumber(n);}
 else{size_t n=strlen(text);if(f->type==DSR_SETTING_ENUM){if(!choice(f->choices,text))return DSR_INVALID;}else if(n>f->max_length)return DSR_INVALID;
  for(size_t i=0;i<n;i++)if((unsigned char)text[i]<32||(unsigned char)text[i]==127)return DSR_INVALID;
  if((strstr(f->path,".ip")&&!strstr(f->path,"ipv4"))||strstr(f->path,"netmask")||strstr(f->path,"nameserver")||!strcmp(f->path,"sta.gw")){if(text[0]&&!dsr_ipv4_valid(text))return DSR_INVALID;}
  if(f->type==DSR_SETTING_SECRET&&!strcmp(f->component,"WiFi")&&n&&n<8)return DSR_INVALID;
  value=cJSON_CreateString(text);
 }
 if(!value)return DSR_IO;
 cJSON *root=cJSON_CreateObject(),*parent=root;if(!root){cJSON_Delete(value);return DSR_IO;}
 const char *path=f->path;
 while(true){char key[48];const char *end=strchr(path,'.');size_t n=end?(size_t)(end-path):strlen(path);memcpy(key,path,n);key[n]=0;if(!end){if(!cJSON_AddItemToObject(parent,key,value))goto fail;break;}cJSON *child=cJSON_CreateObject();if(!child)goto fail;if(!cJSON_AddItemToObject(parent,key,child)){cJSON_Delete(child);goto fail;}parent=child;path=end+1;}
 /* WiFi.SetConfig requires pass whenever ssid is supplied. Changing SSID
  * uses the dedicated network form, which supplies both; single-leaf editor
  * refuses it rather than silently opening a previously protected network. */
 if(!strcmp(f->component,"WiFi")&&(!strcmp(f->path,"sta.ssid")||!strcmp(f->path,"sta1.ssid"))){cJSON_Delete(root);return DSR_UNSUPPORTED;}
 *out=root;return DSR_OK;
fail:cJSON_Delete(value);cJSON_Delete(root);return DSR_IO;
}
bool dsr_setting_equal(const cJSON *a,const cJSON *b)
{return a&&b&&cJSON_Compare(a,b,true);}
dsr_result_t dsr_setting_network_patch(const cJSON *configuration,unsigned field,const char *ssid,const char *password,cJSON **out)
{
 if(out)*out=NULL;
 const dsr_setting_field_t *f=dsr_setting_field(field);
 if(!out||!f||strcmp(f->component,"WiFi")||(strcmp(f->path,"sta.ssid")&&strcmp(f->path,"sta1.ssid"))||!present(f,configuration)||!ssid||!ssid[0]||strlen(ssid)>32||!password||strlen(password)>63||(password[0]&&strlen(password)<8))return DSR_INVALID;
 for(const unsigned char *p=(const unsigned char *)ssid;*p;p++)if(*p<32||*p==127)return DSR_INVALID;
 cJSON *root=cJSON_CreateObject(),*sta=cJSON_CreateObject();if(!root||!sta){cJSON_Delete(root);cJSON_Delete(sta);return DSR_IO;}
 if(!cJSON_AddItemToObject(root,f->path[3]=='1'?"sta1":"sta",sta)){cJSON_Delete(root);cJSON_Delete(sta);return DSR_IO;}
 if(!cJSON_AddStringToObject(sta,"ssid",ssid)||!(password[0]?cJSON_AddStringToObject(sta,"pass",password):cJSON_AddNullToObject(sta,"pass"))||!cJSON_AddBoolToObject(sta,"enable",true)||!cJSON_AddStringToObject(sta,"ipv4mode","dhcp")){dsr_setting_json_delete(root);return DSR_IO;}
 *out=root;return DSR_OK;
}
static void erase_strings(cJSON *value)
{for(cJSON *v=value;v;v=v->next){if(cJSON_IsString(v)&&v->valuestring&&!(v->type&cJSON_IsReference)){volatile char *p=v->valuestring;size_t n=strlen(v->valuestring);while(n--)*p++=0;}if(v->child)erase_strings(v->child);}}
void dsr_setting_json_delete(cJSON *value)
{erase_strings(value);cJSON_Delete(value);}
static bool method(const cJSON *methods,const char *ns,const char *suffix)
{char name[64];snprintf(name,sizeof(name),"%s.%s",ns,suffix);const cJSON *v;cJSON_ArrayForEach(v,cJSON_GetObjectItemCaseSensitive(methods,"methods"))if(cJSON_IsString(v)&&!strcmp(v->valuestring,name))return true;return false;}
dsr_result_t dsr_setting_groups(const cJSON *methods,const cJSON *components,dsr_setting_group_t *groups,unsigned *count)
{
 if(!methods||!components||!groups||!count||*count>DSR_SETTINGS_GROUPS||!cJSON_IsArray(cJSON_GetObjectItemCaseSensitive(methods,"methods"))||!cJSON_IsArray(cJSON_GetObjectItemCaseSensitive(components,"components")))return DSR_BAD_RESPONSE;
 static const char *single[]={"Sys","WiFi","BLE","Cloud","MQTT"};
 for(unsigned i=0;i<5;i++)if(method(methods,single[i],"GetConfig")&&method(methods,single[i],"SetConfig")){
  bool exists=false;for(unsigned j=0;j<*count;j++)if(!strcmp(groups[j].component,single[i]))exists=true;
  if(!exists){if(*count==DSR_SETTINGS_GROUPS)return DSR_FULL;dsr_setting_group_t *g=&groups[(*count)++];memset(g,0,sizeof(*g));snprintf(g->component,sizeof(g->component),"%s",single[i]);g->singleton=true;}
 }
 const cJSON *c;cJSON_ArrayForEach(c,cJSON_GetObjectItemCaseSensitive(components,"components")){
  const cJSON *key=cJSON_GetObjectItemCaseSensitive(c,"key");if(!cJSON_IsString(key)||strlen(key->valuestring)>=32)return DSR_BAD_RESPONSE;
  char name[32];strcpy(name,key->valuestring);char *colon=strchr(name,':');if(!colon)continue;*colon++=0;char *end;long id=strtol(colon,&end,10);if(!colon[0]||*end||id<0||id>299)return DSR_BAD_RESPONSE;
  const char *ns=!strcmp(name,"switch")?"Switch":!strcmp(name,"input")?"Input":!strcmp(name,"cover")?"Cover":!strcmp(name,"light")?"Light":!strcmp(name,"rgb")?"RGB":!strcmp(name,"rgbw")?"RGBW":!strcmp(name,"temperature")?"Temperature":!strcmp(name,"humidity")?"Humidity":NULL;
  if(!ns||!method(methods,ns,"GetConfig")||!method(methods,ns,"SetConfig"))continue;
  bool exists=false;for(unsigned j=0;j<*count;j++)if(!strcmp(groups[j].component,ns)&&groups[j].id==id)exists=true;
  if(exists)continue;
  if(*count==DSR_SETTINGS_GROUPS)return DSR_FULL;
  dsr_setting_group_t *g=&groups[(*count)++];memset(g,0,sizeof(*g));snprintf(g->component,sizeof(g->component),"%s",ns);g->id=(uint16_t)id;
 }
 return DSR_OK;
}
unsigned dsr_setting_fields(const dsr_setting_group_t *group,const cJSON *configuration,uint16_t *fields,unsigned capacity)
{unsigned n=0;if(!group||!configuration||!fields)return 0;for(unsigned i=0;i<sizeof(schema)/sizeof(*schema)&&n<capacity;i++)if(!strcmp(schema[i].component,group->component)&&present(&schema[i],configuration))fields[n++]=(uint16_t)i;return n;}
