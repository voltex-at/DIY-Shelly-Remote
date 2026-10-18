#include "dsr_boot.h"
#include "dsr_activity.h"
#include "dsr_config_health.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stdatomic.h>

static atomic_int state = DSR_BOOT_STARTING;
void dsr_boot_begin(void) { atomic_store(&state, DSR_BOOT_STARTING); }
dsr_boot_state_t dsr_boot_state(void) { return (dsr_boot_state_t)atomic_load(&state); }
bool dsr_boot_ready(void) { return dsr_boot_state() == DSR_BOOT_READY; }
/* First-format progress screen only (#28): opened and closed by app_main's
 * storage observer while no other UI exists. Never confirms OTA or boot. */
static atomic_bool provisioning_ui;
void dsr_boot_provisioning_ui(bool open) { atomic_store(&provisioning_ui, open); }
bool dsr_boot_ui_gate(void) { return dsr_boot_ready() || atomic_load(&provisioning_ui); }
bool dsr_boot_finish(void)
{
    int expected = DSR_BOOT_STARTING;
    return atomic_compare_exchange_strong(&state, &expected, DSR_BOOT_READY);
}
void dsr_boot_diagnostic(const char *stage, esp_err_t error)
{
    atomic_store(&state, DSR_BOOT_DIAGNOSTIC);
    ESP_LOGE("dsr_boot", "DIAGNOSTIC stage=%s error=%s; UI/admin/OTA confirmation disabled; no automatic restart or erase",
             stage, esp_err_to_name(error));
    ESP_LOGE("dsr_boot", "Keep USB serial at 115200; collect this log and use explicit USB recovery; user data preserved");
    ESP_LOGE("dsr_boot", "diagnostic internal_free=%u largest=%u psram_free=%u",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
bool dsr_ota_boot_confirm(void)
{
    if (!dsr_boot_ready()) return false;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running) return false;
    esp_ota_img_states_t image_state;
    esp_err_t error = esp_ota_get_state_partition(running, &image_state);
    /* USB provisioning may have no OTA selection record. Do not write one. */
    if (error == ESP_ERR_NOT_FOUND) return true;
    if (error != ESP_OK) return false;
    if (image_state == ESP_OTA_IMG_PENDING_VERIFY) {
        /* The recovery UI remains usable when storage fails. That must not
         * cancel rollback for a new app that cannot load the saved workspace.
         * This is startup health evidence, not a destructive write probe. */
        dsr_config_health_t health;
        dsr_config_health_snapshot(&health);
        if (health.storage != DSR_STORAGE_READY || health.storage_error != ESP_OK ||
            health.workspace != DSR_WORKSPACE_READY || health.workspace_error != ESP_OK)
            return false;
        if(!dsr_activity_begin())return false;
        bool confirmed=esp_ota_mark_app_valid_cancel_rollback()==ESP_OK;
        dsr_activity_end();return confirmed;
    }
    return image_state == ESP_OTA_IMG_VALID || image_state == ESP_OTA_IMG_UNDEFINED;
}
