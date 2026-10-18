#include "dsr_blu.h"
static const dsr_blu_setting_t fields[]={
 {"Tryb beacon","cb9e957e-952d-4761-a7e1-4416494a5bfa",0,1,0,1,true},
 {"Czułość ruchu","21b5b57b-da8d-4ea4-baf8-7654a2214650",5,1,0,2,false},
 {"Podtrzymanie ruchu (s)","219a1ecc-2567-4378-9dbd-0c97d10630ad",5,2,30,600,false},
 {"Liczba impulsów ruchu","22f36e64-e682-4fbc-8dd6-a87f6e7b7d92",5,1,1,4,false},
 {"Dioda LED","24f52308-6cc6-4065-acf0-1d4574d9ba0f",5,1,0,1,true},
 {"Próg zmiany kąta","86e7cc43-19f4-4f38-b5ad-1ae586237e2a",2,1,0,255,false},
 {"Dźwięk przycisku","dd78bf35-7680-484e-ad86-1bc1e7738e14",1,1,0,1,true},
 {"Szybkość wielokliku","35c8a5e2-1cac-4b03-8386-eb6846a1776e",1,1,0,5,false},
 {"Znajdź urządzenie","5b026510-4088-c297-46d8-be6c736a087b",1,1,0,1,true},
};
const dsr_blu_setting_t *dsr_blu_setting(unsigned index)
{return index<sizeof(fields)/sizeof(*fields)?&fields[index]:NULL;}
bool dsr_blu_supported(const dsr_ble_device_t *d)
{if(!d||!d->has_model||!(d->evidence&DSR_BLE_MFG)||d->kind!=DSR_BLE_SHELLY_BLU)return false;switch(d->model_id){case 1:case 2:case 3:case 5:case 6:case 7:case 8:return true;default:return false;}}
bool dsr_blu_value_valid(unsigned index,uint16_t model,uint32_t value)
{const dsr_blu_setting_t *f=dsr_blu_setting(index);return f&&(!f->model||f->model==model)&&!(index==0&&model==3)&&value>=f->minimum&&value<=f->maximum;}
