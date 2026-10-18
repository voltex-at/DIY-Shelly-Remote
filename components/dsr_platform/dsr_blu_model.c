#include "dsr_blu.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define FIELD(l,u,m,b,lo,hi,bo) {l,u,m,b,lo,hi,bo,0,1,false}
#define FLAG(l,bit) {l,"7ff16da2-6f68-4dc6-a97a-bee89d36e739",8,2,0,1,true,1u<<(bit),1,false}
static const dsr_blu_setting_t fields[]={
 FIELD("Tryb beacon","cb9e957e-952d-4761-a7e1-4416494a5bfa",0,1,0,1,true),
 FIELD("Czułość ruchu","21b5b57b-da8d-4ea4-baf8-7654a2214650",5,1,0,2,false),
 FIELD("Podtrzymanie ruchu (s)","219a1ecc-2567-4378-9dbd-0c97d10630ad",5,2,30,600,false),
 FIELD("Liczba impulsów ruchu","22f36e64-e682-4fbc-8dd6-a87f6e7b7d92",5,1,1,4,false),
 FIELD("Dioda LED","24f52308-6cc6-4065-acf0-1d4574d9ba0f",5,1,0,1,true),
 FIELD("Próg zmiany kąta","86e7cc43-19f4-4f38-b5ad-1ae586237e2a",2,1,0,255,false),
 FIELD("Dźwięk przycisku","dd78bf35-7680-484e-ad86-1bc1e7738e14",1,1,0,1,true),
 FIELD("Szybkość wielokliku","35c8a5e2-1cac-4b03-8386-eb6846a1776e",1,1,0,5,false),
 FIELD("Znajdź urządzenie","5b026510-4088-c297-46d8-be6c736a087b",1,1,0,1,true),
 FLAG("Termostat włączony",0),
 FLAG("Ogrzewanie podłogowe",1),
 FLAG("Przyspieszenie regulacji",2),
 FLAG("Automatyczna kalibracja",3),
 FLAG("Ochrona przed zablokowaniem",4),
 FLAG("Oszczędzanie energii",5),
 FLAG("Tryb cichy",6),
 FLAG("Nadpisywanie temperatury",7),
 FLAG("Blokada rodzicielska",8),
 FLAG("Jednostki Celsjusza",9),
 FLAG("Obróć wyświetlacz",10),
 FIELD("Minimalne otwarcie zaworu (%)","8f4ec08c-91be-4e44-b75a-59c0432cd5b2",8,1,0,99,false),
 FIELD("Domyślny boost (s)","a184e9b0-9c01-4f02-b6fc-0e2d247a7979",8,4,0,UINT32_MAX,false),
 FIELD("Domyślne nadpisanie (s)","89c78846-3139-4d69-b4b6-045bfa21c081",8,4,0,UINT32_MAX,false),
 {"Korekta temperatury (°C)","56133ba2-7fc5-4fd9-a0ef-621afddd8b7d",8,1,-125,125,false,0,10,true},
 {"Domyślna temperatura (°C)","886187bf-46d1-487a-a914-adf51caebadd",8,2,40,300,false,0,10,false},
 {"Minimalna temperatura (°C)","a4e3b1d7-7c92-4f38-b5a1-1d2e3f4a5b6c",8,2,40,299,false,0,10,false},
 {"Maksymalna temperatura (°C)","b5f4c2e8-8da3-5049-c6b2-2e3f4a5b6c7d",8,2,41,300,false,0,10,false},
 {"Maksymalna kompensacja (°C)","1bd55b4b-4ec5-4922-9ca8-ae2a72dd5a9d",8,2,0,50,false,0,10,false},
};
const dsr_blu_setting_t *dsr_blu_setting(unsigned index)
{return index<sizeof(fields)/sizeof(*fields)?&fields[index]:NULL;}
bool dsr_blu_supported(const dsr_ble_device_t *d)
{if(!d||!d->has_model||!(d->evidence&DSR_BLE_MFG)||d->kind!=DSR_BLE_SHELLY_BLU)return false;switch(d->model_id){case 1:case 2:case 3:case 5:case 6:case 7:case 8:return true;default:return false;}}
bool dsr_blu_value_valid(unsigned index,uint16_t model,uint32_t value)
{const dsr_blu_setting_t *f=dsr_blu_setting(index);if(!f)return false;int64_t n=f->signed_value?(int8_t)value:(int64_t)value;return (!f->model||f->model==model)&&!(index==0&&model==3)&&(!f->signed_value||value<=255)&&n>=f->minimum&&n<=f->maximum;}
bool dsr_blu_value_parse(unsigned index,uint16_t model,const char *text,uint32_t *value)
{
 const dsr_blu_setting_t *f=dsr_blu_setting(index);if(!f||!text||!text[0]||!value||text[0]==' '||text[0]=='\t')return false;
 char *end;errno=0;double n=strtod(text,&end)*(f->scale?f->scale:1);if(errno||*end||!isfinite(n)||n<f->minimum||n>f->maximum||fabs(n-round(n))>0.000001)return false;
 uint32_t raw=f->signed_value?(uint8_t)(int8_t)(int32_t)round(n):(uint32_t)round(n);if(!dsr_blu_value_valid(index,model,raw))return false;*value=raw;return true;
}
void dsr_blu_value_format(unsigned index,uint32_t value,char *text,size_t size)
{const dsr_blu_setting_t *f=dsr_blu_setting(index);if(!text||!size)return;if(!f){text[0]=0;return;}double n=f->signed_value?(int8_t)value:(double)value;snprintf(text,size,f->scale==10?"%.1f":"%.0f",n/(f->scale?f->scale:1));}
