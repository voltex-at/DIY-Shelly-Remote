#ifndef DSR_BACKUP_BONDS_H
#define DSR_BACKUP_BONDS_H
#include "dsr_core.h"
#include "cJSON.h"
/* Snapshot runs on the acknowledged NimBLE owner barrier. Apply is boot-only. */
dsr_result_t dsr_backup_bonds_capture(cJSON *manifest);
dsr_result_t dsr_backup_bonds_preflight(const cJSON *manifest);
dsr_result_t dsr_backup_bonds_apply(const cJSON *manifest);
#endif
