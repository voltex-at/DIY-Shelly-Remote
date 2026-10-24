#ifndef DSR_MAINTENANCE_RECOVERY_H
#define DSR_MAINTENANCE_RECOVERY_H
#include "dsr_maintenance_types.h"
typedef void (*dsr_maint_progress_fn)(const dsr_maint_status_t *status);
/* Startup owner, after NVS/board/splash and before every ordinary service.
 * Only OK permits normal startup. No format-on-error or automatic reset. */
dsr_result_t dsr_maintenance_recover(dsr_maint_progress_fn progress);
/* Physical startup recovery owner only, after the native 3-second hold.
 * Fresh ADC here precedes all workers/radio. No HTTP equivalent. */
dsr_result_t dsr_maintenance_recovery_reset(bool stable_usb);
#endif
