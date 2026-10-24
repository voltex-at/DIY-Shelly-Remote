#ifndef DSR_RESTART_INTERNAL_H
#define DSR_RESTART_INTERNAL_H
#include "dsr_restart.h"
#include <stdint.h>
/* Private committed-intent handoff. Notification failure preserves the commit
 * latch; only boot recovery may consume it. No HTTP caller supplies these IDs. */
dsr_restart_result_t dsr_restart_request_maintenance(uint64_t reservation,uint64_t generation);
#endif
