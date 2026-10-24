#pragma once
#include "dsr_storage_progress.h"
#include "dsr_maintenance_recovery.h"
void dsr_ui_init(void);
const dsr_storage_observer_t *dsr_ui_storage_observer(void);
void dsr_ui_splash_begin(void);
void dsr_ui_splash_end(void);
void dsr_ui_maintenance_boot_publish(const dsr_maint_status_t *);
bool dsr_ui_maintenance_boot_available(void);
unsigned dsr_ui_maintenance_boot_take_action(bool *usb);
