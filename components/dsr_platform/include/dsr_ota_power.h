#ifndef DSR_OTA_POWER_H
#define DSR_OTA_POWER_H
#include "dsr_core.h"
#define DSR_OTA_MIN_BATTERY_PERCENT 20u
typedef enum {DSR_OTA_POWER_READY,DSR_OTA_POWER_UNKNOWN,DSR_OTA_POWER_LOW,
 DSR_OTA_POWER_UNCHECKED,DSR_OTA_POWER_BUSY,DSR_OTA_POWER_USB_REQUIRED} dsr_ota_power_result_t;
/* Sole worker only: a fresh ADC/profile read, never cached LCD data. */
dsr_ota_power_result_t dsr_ota_power_read(void);
/* HTTP only; success reserves the worker slot until release, including LOW.
 * At most 1000 ms waiting for the worker. Busy/timeout never yields UNKNOWN. */
dsr_result_t dsr_ota_power_acquire(dsr_ota_power_result_t *out);
void dsr_ota_power_release(void);
#endif
