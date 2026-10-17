#ifndef DSR_OTA_INSTALL_H
#define DSR_OTA_INSTALL_H
#include "dsr_ota_fetch.h"
#include "esp_err.h"

typedef enum {
 DSR_INSTALL_OK,DSR_INSTALL_ARGUMENT,DSR_INSTALL_BUSY,DSR_INSTALL_POWER,
 DSR_INSTALL_BOOT,DSR_INSTALL_SLOT,DSR_INSTALL_FETCH,
 DSR_INSTALL_CONSENT_CHANGED,DSR_INSTALL_SDK,DSR_INSTALL_CANCELLED
} dsr_ota_install_result_t;
typedef enum {
 DSR_INSTALL_MANIFEST,DSR_INSTALL_DOWNLOADING,DSR_INSTALL_VERIFYING,
 DSR_INSTALL_SELECTING_BOOT,DSR_INSTALL_STAGED
} dsr_ota_install_phase_t;
typedef struct {
 bool (*cancelled)(void *context);
 void (*progress)(void *context,dsr_ota_install_phase_t phase,
                  uint32_t received,uint32_t total);
 void *context;
} dsr_ota_install_control_t;
typedef struct {
 dsr_ota_fetch_result_t fetch_result;
 dsr_manifest_result_t manifest_result;
 dsr_image_result_t image_result;
 esp_err_t sdk_error;
 uint32_t bytes_written;
 bool boot_selection_attempted,boot_selected;
} dsr_ota_install_report_t;
/* Point-in-time CHECK preflight, no reservation or writes. Returns OK with the
 * actual inactive size only for the fixed supported map/running/next and ready
 * boot; BUSY covers held, committed or uncertain stage. INSTALL repeats gates. */
dsr_ota_install_result_t dsr_ota_install_ready(size_t *inactive_size);
/* Blocking owner-worker operation, never LVGL/HTTP callback context. The caller
 * consumes its physical, one-shot consent before calling. consent is only the
 * expected version/channel/URL/size/SHA tuple, NEVER a trust credential. The
 * manifest is freshly fetched and genuinely verified before APP or Flash.
 * power_confirmed comes from physical consent/USB attestation, not guessed SOC.
 * The actual fixed board partition map and current boot readiness are checked.
 * The shared Web/native staging reservation spans all cleanup and boot choice;
 * after any boot-selection attempt it refuses further writes until reboot,
 * including unconfirmed SDK failure. It releases the activity lease so the
 * existing restart backend can be used.
 * Callbacks are cooperative, must be bounded and must not call LVGL, reenter
 * the installer, close clients or write Flash. Pointed inputs stay immutable
 * until return; report (optional) belongs exclusively to this operation.
 * No reboot/erase/data migration. SDK set_boot failure does NOT prove that
 * physical otadata is unchanged; boot_selected means confirmed SDK success. */
dsr_ota_install_result_t dsr_ota_install(const char *manifest_url,
 const dsr_ota_policy_t *policy,const dsr_ota_verified_t *consent,
 const dsr_ota_install_control_t *control,bool power_confirmed,
 dsr_ota_install_report_t *report);
#endif
