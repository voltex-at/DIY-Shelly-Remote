#include "dsr_style_hint.h"
#include "nvs.h"
static int cached=-1; /* applied style after legacy mapping */
static int stored=-1; /* raw NVS byte: a retired or invalid id is rewritten once */
static uint8_t effective(uint8_t value){return value==DSR_STYLE_HINT_RETIRED||value>3?0:value;}
uint8_t dsr_style_hint_get(void)
{
 if(cached>=0)return (uint8_t)cached;
 uint8_t value=0;nvs_handle_t handle;
 if(nvs_open("dsr_ui",NVS_READONLY,&handle)==ESP_OK){if(nvs_get_u8(handle,"style",&value)!=ESP_OK)value=0;nvs_close(handle);}
 stored=value;cached=effective(value);return (uint8_t)cached;
}
void dsr_style_hint_set(uint8_t style)
{
 if(style>3||style==DSR_STYLE_HINT_RETIRED)return;
 if(dsr_style_hint_get()==style&&stored==style)return; /* write only on a real change */
 nvs_handle_t handle;
 if(nvs_open("dsr_ui",NVS_READWRITE,&handle)!=ESP_OK)return;
 if(nvs_set_u8(handle,"style",style)==ESP_OK&&nvs_commit(handle)==ESP_OK)cached=stored=style;
 nvs_close(handle);
}
