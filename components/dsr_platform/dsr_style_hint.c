#include "dsr_style_hint.h"
#include "nvs.h"
static int cached=-1;
uint8_t dsr_style_hint_get(void)
{
 if(cached>=0)return (uint8_t)cached;
 uint8_t value=0;nvs_handle_t handle;
 if(nvs_open("dsr_ui",NVS_READONLY,&handle)==ESP_OK){if(nvs_get_u8(handle,"style",&value)!=ESP_OK||value>2)value=0;nvs_close(handle);}
 cached=value;return value;
}
void dsr_style_hint_set(uint8_t style)
{
 if(style>2||dsr_style_hint_get()==style)return; /* write only on a real change */
 nvs_handle_t handle;
 if(nvs_open("dsr_ui",NVS_READWRITE,&handle)!=ESP_OK)return;
 if(nvs_set_u8(handle,"style",style)==ESP_OK&&nvs_commit(handle)==ESP_OK)cached=style;
 nvs_close(handle);
}
