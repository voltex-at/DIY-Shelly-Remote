#ifndef DSR_BACKUP_SOURCES_MODEL_H
#define DSR_BACKUP_SOURCES_MODEL_H
#include "dsr_platform.h"
#include "dsr_automation_types.h"
#include "cJSON.h"
/* Allocate on heap/PSRAM, never the LVGL or startup stack. */
typedef struct {
 dsr_settings_t settings;dsr_battery_profile_t battery;
 dsr_connection_t connections[DSR_MAX_DEVICES];size_t count;uint8_t style;
 dsr_auto_record_t enrollment;
} dsr_backup_sources_data_t;
_Static_assert(sizeof(dsr_backup_sources_data_t)<=12288,"bounded private source bundle");
/* No IO. Private byte strings are canonical lower-case hex, not raw UTF-8;
 * SSIDs and stored credentials can contain non-UTF-8 bytes. */
dsr_result_t dsr_backup_sources_decode(const cJSON *root,const uint8_t device_id[32],dsr_backup_sources_data_t *out);
dsr_result_t dsr_backup_sources_encode(cJSON *root,const dsr_backup_sources_data_t *data);
/* Resolve only already-approved identities, without operational history. */
dsr_result_t dsr_backup_enrollment_prepare(const cJSON *config,const dsr_auto_record_t *trusted,const dsr_connection_t *connections,size_t count,dsr_auto_record_t *out);
bool dsr_backup_photo_builtin(const char *id);
#endif
