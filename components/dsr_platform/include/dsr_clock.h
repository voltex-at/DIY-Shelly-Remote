#ifndef DSR_CLOCK_H
#define DSR_CLOCK_H
#include <stdbool.h>
typedef enum {DSR_CLOCK_READY,DSR_CLOCK_CANCELLED,DSR_CLOCK_UNAVAILABLE} dsr_clock_result_t;
/* One lazy SDK SNTP service for native OTA and the LCD clock. Plausible UTC
 * is not publisher authentication. */
bool dsr_clock_start(void); /* non-blocking, idempotent; call with Wi-Fi up */
dsr_clock_result_t dsr_clock_sync(bool (*cancelled)(void *),void *context); /* sole worker only */
#endif
