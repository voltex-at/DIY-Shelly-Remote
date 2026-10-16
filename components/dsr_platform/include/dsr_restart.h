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
#endif
