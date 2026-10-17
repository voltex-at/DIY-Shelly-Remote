#ifndef DSR_CLOCK_H
#define DSR_CLOCK_H
#include <stdbool.h>
typedef enum {DSR_CLOCK_READY,DSR_CLOCK_CANCELLED,DSR_CLOCK_UNAVAILABLE} dsr_clock_result_t;
/* Sole I/O worker only, explicit user request. One lazy SDK SNTP service for
 * history and native OTA. Plausible UTC is not publisher authentication. */
dsr_clock_result_t dsr_clock_sync(bool (*cancelled)(void *),void *context);
#endif
