#ifndef DSR_MAINTENANCE_RECOVERY_IO_H
#define DSR_MAINTENANCE_RECOVERY_IO_H
#include "dsr_core.h"
#include "cJSON.h"
/* Boot-only fixed destinations. No filename or namespace from the archive. */
dsr_result_t dsr_maint_recovery_mount(void);
dsr_result_t dsr_maint_recovery_photo_read(const char id[33],uint8_t out[115200]);
dsr_result_t dsr_maint_recovery_photo_write(const char id[33],const uint8_t bytes[115200]);
dsr_result_t dsr_maint_recovery_checkpoint(const cJSON *config,uint32_t revision,unsigned slot);
dsr_result_t dsr_maint_recovery_unpair(void);
dsr_result_t dsr_maint_recovery_reset_part(unsigned part);
#endif
