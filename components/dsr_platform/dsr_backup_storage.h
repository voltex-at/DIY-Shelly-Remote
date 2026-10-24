#ifndef DSR_BACKUP_STORAGE_H
#define DSR_BACKUP_STORAGE_H
#include "dsr_platform.h"
#include "dsr_automation_types.h"
/* Private boot recovery adapters. Every implementation checks boot authority. */
esp_err_t dsr_settings_restore(const dsr_settings_t *);
esp_err_t dsr_connections_restore(const dsr_connection_t *,size_t count);
esp_err_t dsr_battery_profile_restore(const dsr_battery_profile_t *);
esp_err_t dsr_style_hint_restore(uint8_t);
esp_err_t dsr_auto_store_restore(const dsr_auto_record_t *);
/* Live owner copies only when journal/worker and heating cycles are quiet. */
dsr_result_t dsr_automation_backup_record(dsr_auto_record_t *out);
#endif
