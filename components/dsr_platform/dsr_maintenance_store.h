#ifndef DSR_MAINTENANCE_STORE_H
#define DSR_MAINTENANCE_STORE_H
#include "dsr_maintenance_model.h"
/* Config owner under maintenance reservation, or committed boot recovery only.
 * Any load error fails closed, including a missing/corrupt single slot. */
dsr_result_t dsr_maint_store_load(dsr_maint_record_t *out,bool *found,bool *pending);
dsr_result_t dsr_maint_store_begin(dsr_maint_record_t *record);
dsr_result_t dsr_maint_store_write(dsr_maint_record_t *record);
dsr_result_t dsr_maint_store_finish(dsr_maint_record_t *record,bool cancelled);
/* Durable zero-key receipt without opening normal boot. Owner cleans staged
 * files afterwards, then finish clears the latch as the final persisted step. */
dsr_result_t dsr_maint_store_receipt(dsr_maint_record_t *record,bool cancelled);
/* Physical startup recovery reset only: replaces even an ambiguous old intent
 * by two already-COMMITTED RESET records. Never an automatic repair action. */
dsr_result_t dsr_maint_store_recovery_reset(dsr_maint_record_t *record);
#endif
