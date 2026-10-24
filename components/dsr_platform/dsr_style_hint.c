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
#ifdef DSR_BOARD_ES3C35P
#include "dsr_backup_storage.h"
#include "dsr_maintenance_private.h"
esp_err_t dsr_style_hint_restore(uint8_t style)
{
 if(!dsr_maintenance_boot_writes_allowed())return ESP_ERR_INVALID_STATE;
 if(style>3||style==DSR_STYLE_HINT_RETIRED)return ESP_ERR_INVALID_ARG;
 nvs_handle_t h;esp_err_t e=nvs_open("dsr_ui",NVS_READWRITE,&h);if(e!=ESP_OK)return e;
 e=nvs_set_u8(h,"style",style);if(e==ESP_OK)e=nvs_commit(h);uint8_t check=255;
 if(e==ESP_OK)e=nvs_get_u8(h,"style",&check);
 if(e==ESP_OK&&check!=style)e=ESP_FAIL;
 nvs_close(h);if(e==ESP_OK)cached=stored=style;return e;
}
#endif
