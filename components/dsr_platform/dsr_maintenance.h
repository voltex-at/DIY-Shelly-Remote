#ifndef DSR_MAINTENANCE_H
#define DSR_MAINTENANCE_H
#include "dsr_maintenance_model.h"
bool dsr_maintenance_acquire(dsr_maint_lease_t *);
void dsr_maintenance_release(const dsr_maint_lease_t *);
bool dsr_maintenance_lease_valid(const dsr_maint_lease_t *);
dsr_maint_generation_t dsr_maintenance_generation(void);
bool dsr_maintenance_generation_matches(dsr_maint_generation_t);
void dsr_maintenance_mutated(void);
void dsr_maintenance_snapshot(dsr_maint_status_t *);
#endif
