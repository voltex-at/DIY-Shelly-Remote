/* Old workspace fixtures do not exercise NVS; no success is fabricated. */
#include "dsr_automation_types.h"
#include "esp_err.h"
esp_err_t dsr_auto_store_save(const dsr_auto_record_t *record){(void)record;return ESP_ERR_NOT_SUPPORTED;}
esp_err_t dsr_auto_store_load(dsr_auto_record_t *record,bool *found){(void)record;*found=false;return ESP_OK;}
