#include "dsr_config_health.h"
#include <stdatomic.h>
#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif
/* Startup is the sole writer, before the LVGL readiness gate opens. Atomic
 * codes also make diagnostics safe if a host fixture changes a boot state. */
static atomic_int storage,storage_error,workspace,workspace_error,panel,panel_error;
void dsr_config_health_storage(dsr_storage_phase_t p,esp_err_t e)
{atomic_store(&storage_error,e);atomic_store(&storage,p);}
void dsr_config_health_workspace(dsr_workspace_phase_t p,esp_err_t e)
{atomic_store(&workspace_error,e);atomic_store(&workspace,p);
#ifdef ESP_PLATFORM
 ESP_LOGI("dsr_config","workspace stage=%d result=0x%x",(int)p,(unsigned)e);
 if(p==DSR_WORKSPACE_READY)ESP_LOGI("dsr_config","workspace_ready");
#endif
}
void dsr_config_health_panel(esp_err_t e)
{atomic_store(&panel_error,e);atomic_store(&panel,e==ESP_OK?DSR_PANEL_READY:DSR_PANEL_FAILED);
#ifdef ESP_PLATFORM
 ESP_LOGI("dsr_config","https startup result=0x%x",(unsigned)e);
 if(e==ESP_OK)ESP_LOGI("dsr_config","https_ready");
#endif
}
void dsr_config_health_snapshot(dsr_config_health_t *out)
{if(out)*out=(dsr_config_health_t){.storage=atomic_load(&storage),.storage_error=atomic_load(&storage_error),.workspace=atomic_load(&workspace),.workspace_error=atomic_load(&workspace_error),.panel=atomic_load(&panel),.panel_error=atomic_load(&panel_error)};}
