#ifndef DSR_BOOT_H
#define DSR_BOOT_H
#include <stdbool.h>
#include "esp_err.h"
typedef enum { DSR_BOOT_STARTING, DSR_BOOT_READY, DSR_BOOT_DIAGNOSTIC } dsr_boot_state_t;
/* Process-local latch only: no NVS, erase, restart, or automatic OTA rollback. */
void dsr_boot_begin(void);
bool dsr_boot_finish(void);
void dsr_boot_diagnostic(const char *stage, esp_err_t error);
bool dsr_boot_ready(void);
/* UI task gate: boot READY, or the first-format progress screen is open.
 * dsr_boot_ready() alone still guards OTA confirmation and all services. */
bool dsr_boot_ui_gate(void);
void dsr_boot_provisioning_ui(bool open);
dsr_boot_state_t dsr_boot_state(void);
/* True means no pending verification or successful confirmation; false retries. */
bool dsr_ota_boot_confirm(void);
#endif
