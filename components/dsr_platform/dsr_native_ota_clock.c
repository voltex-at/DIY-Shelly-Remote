#include "dsr_ota_fetch.h"
#include "dsr_clock.h"
/* Native OTA and explicit history synchronization share the sole worker and
 * one canonical lazy SDK SNTP owner; no duplicate initialization service. */
dsr_ota_fetch_result_t dsr_native_ota_clock(const dsr_ota_fetch_control_t *control)
{
 dsr_clock_result_t result=dsr_clock_sync(control?control->cancelled:NULL,control?control->context:NULL);
 return result==DSR_CLOCK_READY?DSR_FETCH_OK:result==DSR_CLOCK_CANCELLED?DSR_FETCH_CANCELLED:DSR_FETCH_CLOCK;
}
