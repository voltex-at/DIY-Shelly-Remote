#ifndef DSR_CONFIG_HEALTH_H
#define DSR_CONFIG_HEALTH_H
#include "esp_err.h"
/* Boot publishes only stage/error codes. No JSON, identifiers or secrets.
 * Readers (including the LCD when workspace is absent) never need Flash. */
typedef enum {DSR_STORAGE_NOT_STARTED,DSR_STORAGE_MOUNTING,DSR_STORAGE_READY,
 DSR_STORAGE_MOUNT_ERROR,DSR_STORAGE_NONEMPTY,DSR_STORAGE_READ_ERROR,
 DSR_STORAGE_PROVISION_ERROR} dsr_storage_phase_t;
typedef enum {DSR_WORKSPACE_NOT_STARTED,DSR_WORKSPACE_MOUNTING,
 DSR_WORKSPACE_LOADING,DSR_WORKSPACE_PROVISIONING,DSR_WORKSPACE_ALLOCATING,
 DSR_WORKSPACE_TASK,DSR_WORKSPACE_READY} dsr_workspace_phase_t;
typedef enum {DSR_PANEL_NOT_STARTED,DSR_PANEL_READY,DSR_PANEL_FAILED} dsr_panel_phase_t;
typedef struct {
 dsr_storage_phase_t storage;esp_err_t storage_error;
 dsr_workspace_phase_t workspace;esp_err_t workspace_error;
 dsr_panel_phase_t panel;esp_err_t panel_error;
} dsr_config_health_t;
void dsr_config_health_storage(dsr_storage_phase_t phase,esp_err_t error);
void dsr_config_health_workspace(dsr_workspace_phase_t phase,esp_err_t error);
void dsr_config_health_panel(esp_err_t error);
void dsr_config_health_snapshot(dsr_config_health_t *out);
#endif
