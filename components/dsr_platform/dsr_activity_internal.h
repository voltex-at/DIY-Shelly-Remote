#ifndef DSR_ACTIVITY_INTERNAL_H
#define DSR_ACTIVITY_INTERNAL_H
#include "dsr_activity.h"
/* Restart backend ONLY. Public callers use dsr_restart_request, which also
 * requires boot readiness and a live deferred dispatcher. */
bool dsr_activity_restart_reserve(void);
void dsr_activity_restart_cancel(void);
bool dsr_activity_restart_dispatch(void);
#endif
