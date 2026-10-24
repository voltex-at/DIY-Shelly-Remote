#ifndef DSR_ACTIVITY_INTERNAL_H
#define DSR_ACTIVITY_INTERNAL_H
#include "dsr_activity.h"
#include <stdint.h>
/* Restart backend ONLY. Public callers use dsr_restart_request, which also
 * requires boot readiness and a live deferred dispatcher. */
bool dsr_activity_restart_reserve(void);
bool dsr_activity_restart_confirmed_reserve(void);
void dsr_activity_restart_cancel(void);
bool dsr_activity_restart_dispatch(void);
bool dsr_activity_maintenance_reserve(uint64_t *reservation,uint64_t *generation);
void dsr_activity_maintenance_release(uint64_t reservation);
bool dsr_activity_maintenance_valid(uint64_t reservation,uint64_t generation);
bool dsr_activity_maintenance_reserved(void);
bool dsr_activity_maintenance_committed(void);
uint64_t dsr_activity_data_generation(void);
bool dsr_activity_maintenance_commit_begin(uint64_t reservation,uint64_t generation);
bool dsr_activity_maintenance_commit_valid(uint64_t reservation,uint64_t generation);
/* Only after the stage owner proves its journal is still PREPARING. An
 * ambiguous/committed write stays closed until restart/recovery. */
bool dsr_activity_maintenance_commit_abort(uint64_t reservation,uint64_t generation);
bool dsr_activity_maintenance_restart_reserve(uint64_t reservation,uint64_t generation);
#endif
