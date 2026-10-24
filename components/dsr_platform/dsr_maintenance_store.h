#ifndef DSR_MAINTENANCE_STORE_H
#define DSR_MAINTENANCE_STORE_H
#include "dsr_maintenance_model.h"
/* Config owner under maintenance reservation, or committed boot recovery only.
 * Any load error fails closed, including a missing/corrupt single slot. */
dsr_result_t dsr_maint_store_load(dsr_maint_record_t *out,bool *found,bool *pending);
dsr_result_t dsr_maint_store_begin(dsr_maint_record_t *record);
dsr_result_t dsr_maint_store_write(dsr_maint_record_t *record);
dsr_result_t dsr_maint_store_finish(dsr_maint_record_t *record,bool cancelled);
#endif
