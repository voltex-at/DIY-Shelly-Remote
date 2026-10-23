#ifndef DSR_PANEL_PERMISSION_H
#define DSR_PANEL_PERMISSION_H
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* Internal physical-consent window; the caller owns the deadline and lock.
 * No wait, network operation or write is performed when consent is refused. */
bool dsr_panel_ota_enable(SemaphoreHandle_t lock,bool panel_ready,int64_t *until,bool stable_usb);
#endif
