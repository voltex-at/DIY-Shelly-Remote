#ifndef DSR_NATIVE_OTA_H
#define DSR_NATIVE_OTA_H
#include "dsr_platform.h"
#include "dsr_ota_install.h"
#include "dsr_ota_discovery.h"

typedef enum {
 DSR_NATIVE_IDLE,DSR_NATIVE_CHECKING,DSR_NATIVE_AVAILABLE,
 DSR_NATIVE_INSTALLING,DSR_NATIVE_STAGED,DSR_NATIVE_CANCELLED,DSR_NATIVE_FAILED
} dsr_native_ota_state_t;
typedef enum {
 DSR_NATIVE_ERROR_NONE,DSR_NATIVE_ERROR_OFFLINE,DSR_NATIVE_ERROR_BUSY,
 DSR_NATIVE_ERROR_CLOCK,DSR_NATIVE_ERROR_NO_RELEASE,DSR_NATIVE_ERROR_CATALOGUE,
 DSR_NATIVE_ERROR_MANIFEST,DSR_NATIVE_ERROR_POWER,DSR_NATIVE_ERROR_SLOT,
 DSR_NATIVE_ERROR_BOOT,DSR_NATIVE_ERROR_EXPIRED,DSR_NATIVE_ERROR_CHANGED,
 DSR_NATIVE_ERROR_INSTALL,DSR_NATIVE_ERROR_MEMORY,DSR_NATIVE_ERROR_LOCAL_VERSION,
 DSR_NATIVE_ERROR_RATE_LIMIT,DSR_NATIVE_ERROR_MANIFEST_DOWNLOAD
} dsr_native_ota_error_t;
typedef struct {
 dsr_native_ota_state_t state;
 dsr_native_ota_error_t error;
 dsr_ota_channel_t channel;
 dsr_ota_install_phase_t phase;
 dsr_ota_fetch_result_t fetch_result;
 dsr_manifest_result_t manifest_result;
 dsr_ota_install_result_t install_result;
 uint32_t operation_id,received,total;
 int http_status;
 char installed[32],version[32],notes[DSR_OTA_NATIVE_NOTES_MAX+1];
 bool boot_selection_attempted;
} dsr_native_ota_snapshot_t;
/* One actor on the existing sole I/O worker. prepare is LCD admission only;
 * no HTTP install API, automatic checks, task, key or release publishing.
 * INSTALL consumes the AVAILABLE tuple once, within120s, with explicit physical
 * stable-power attestation. A fresh signature/tuple/slot gate runs in installer.
 * snapshot/progress/cancel are synchronized; never call LVGL from the worker. */
esp_err_t dsr_native_ota_init(void);
dsr_result_t dsr_native_ota_prepare(uint32_t id,dsr_job_kind_t kind,
 dsr_ota_channel_t channel,bool power_confirmed);
bool dsr_native_ota_admitted(uint32_t id,dsr_job_kind_t kind);
void dsr_native_ota_reject(uint32_t id);
void dsr_native_ota_execute(const dsr_job_t *job,dsr_event_t *event);
void dsr_native_ota_snapshot(dsr_native_ota_snapshot_t *out);
void dsr_native_ota_cancel(void);
#endif
