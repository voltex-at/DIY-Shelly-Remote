#ifndef DSR_AUTOMATION_STORE_H
#define DSR_AUTOMATION_STORE_H
#include "dsr_automation_types.h"
#include "esp_err.h"
/* Called by config owner only. No credentials, erasure or silent fallback. */
esp_err_t dsr_auto_store_load(dsr_auto_record_t *out,bool *found);
esp_err_t dsr_auto_store_save(const dsr_auto_record_t *record);
#endif
