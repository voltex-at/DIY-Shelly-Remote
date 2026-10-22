#ifndef DSR_RESTART_H
#define DSR_RESTART_H
#include "esp_err.h"

typedef enum {
 DSR_RESTART_ACCEPTED, DSR_RESTART_BUSY, DSR_RESTART_UNAVAILABLE
} dsr_restart_result_t;
/* Startup calls once before UI admission. Failure is nonfatal and leaves
 * requests unavailable; this function does not retry or restart anything. */
esp_err_t dsr_restart_start(void);
/* Caller owns explicit user confirmation. Nonblocking task-context request;
 * no ISR, I/O, erase, storage formatting or restart from this call. ACCEPTED
 * means a live dispatcher was notified after exclusive activity reservation.
 * BUSY means an operation or another restart owns admission. UNAVAILABLE
 * includes incomplete boot, failed startup/notification or terminal dispatch.
 * Notification failure reopens only this request's pending reservation.
 * Dispatch waits500ms outside LVGL, rechecks boot readiness and atomically
 * claims once. Boot loss cancels; no automatic retry. This does not promise
 * an HTTP response was delivered or an LCD frame was physically rendered. */
dsr_restart_result_t dsr_restart_request(void);
/* Physical, explicitly confirmed local restart. Blocks new activity, keeps
 * existing lease counts intact, and dispatches after the same fixed 500 ms
 * display grace even if an operation is stuck. No wait for network/BLE success.
 * A hardware RTC watchdog guards SDK shutdown (3 s system, then 3 s RTC reset).
 * Web and automatic OTA requests continue to use dsr_restart_request(). */
dsr_restart_result_t dsr_restart_request_confirmed(void);
#endif
