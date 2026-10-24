#pragma once
#include "dsr_storage_progress.h"
#include "dsr_maintenance_recovery.h"
void dsr_ui_init(void);
/* Register with dsr_storage_set_observer() before storage starts (#28). */
const dsr_storage_observer_t *dsr_ui_storage_observer(void);
/* Boot splash (owner artwork): begin right after dsr_board_init, end before
 * dsr_ui_init. Opens/closes the early UI gate; no-op stays dark on failure. */
void dsr_ui_splash_begin(void);
void dsr_ui_splash_end(void);
/* Startup storage callbacks only copy this bounded mailbox. All LVGL work and
 * physical confirmations remain on the existing splash LVGL owner. */
void dsr_ui_maintenance_boot_begin(void);
void dsr_ui_maintenance_boot_owner(bool running);
bool dsr_ui_maintenance_boot_available(void);
void dsr_ui_maintenance_boot_publish(const dsr_maint_status_t *);
unsigned dsr_ui_maintenance_boot_take_action(bool *stable_usb);
void dsr_ui_maintenance_boot_release(void);
