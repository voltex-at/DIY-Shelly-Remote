#ifndef DSR_MAINTENANCE_MODEL_H
#define DSR_MAINTENANCE_MODEL_H
#include "dsr_maintenance_types.h"
/* Private owner lease, never a web capability. Release matches reservation ID. */
typedef struct {uint64_t reservation;dsr_maint_generation_t generation;} dsr_maint_lease_t;
#endif
