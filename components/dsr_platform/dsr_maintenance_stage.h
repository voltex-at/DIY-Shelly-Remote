#ifndef DSR_MAINTENANCE_STAGE_H
#define DSR_MAINTENANCE_STAGE_H
#include "dsr_backup_format.h"
#include "dsr_maintenance_model.h"
typedef struct dsr_maint_stage dsr_maint_stage_t;
/* Prepare owns a short reservation on the existing config owner. The caller
 * retains image ownership and may free it after this returns. */
dsr_result_t dsr_maint_stage_prepare(const dsr_backup_image_t *,dsr_maint_stage_t **out);
dsr_result_t dsr_maint_stage_commit(dsr_maint_stage_t *,const dsr_maint_lease_t *);
/* Split final barrier from promotion so the sole worker can sample power after
 * slow file/BLE IO. Reacquired lease must still match the verified generation. */
dsr_result_t dsr_maint_stage_barrier(dsr_maint_stage_t *,const dsr_maint_lease_t *);
dsr_result_t dsr_maint_stage_commit_verified(dsr_maint_stage_t *,const dsr_maint_lease_t *);
dsr_result_t dsr_maint_stage_cancel(dsr_maint_stage_t *);
void dsr_maint_stage_free(dsr_maint_stage_t *);
/* Private copy, includes key material: only the controller/recovery may call. */
bool dsr_maint_stage_record(const dsr_maint_stage_t *,dsr_maint_record_t *out);
/* Recovery reads one authenticated record. No plaintext is returned on error. */
dsr_result_t dsr_maint_stage_read(const dsr_maint_record_t *,unsigned index,uint8_t *out,size_t capacity,size_t *written);
dsr_result_t dsr_maint_stage_cleanup_preparing(const dsr_maint_record_t *);
#endif
