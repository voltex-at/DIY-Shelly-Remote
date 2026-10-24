#ifndef DSR_BACKUP_SOURCES_H
#define DSR_BACKUP_SOURCES_H
#include "dsr_backup_format.h"
#include "dsr_maintenance_model.h"
dsr_result_t dsr_backup_sources_capture(const dsr_maint_lease_t *,dsr_backup_image_t **out);
dsr_result_t dsr_backup_sources_apply(const cJSON *validated_manifest);
bool dsr_backup_sources_quiet(void);
/* Factory identity plus board domain, stable across user-data reset. */
dsr_result_t dsr_backup_device_id(uint8_t out[32]);
#endif
