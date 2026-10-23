#ifndef DSR_AUTOMATION_H
#define DSR_AUTOMATION_H
#include "dsr_fleet.h"
#include "dsr_automation_types.h"
typedef struct cJSON cJSON;
esp_err_t dsr_automation_init(void);
void dsr_automation_tick(dsr_fleet_t *fleet,uint32_t now_ms,uint32_t utc,bool online);
bool dsr_automation_manual_ready(uint32_t layout_id,uint32_t intent_id);
dsr_result_t dsr_automation_manual_result(uint32_t layout_id,uint32_t intent_id);
bool dsr_automation_config_allowed(const cJSON *candidate,char error[96]);
void dsr_automation_config_published(const cJSON *root);
void dsr_automation_snapshot(dsr_auto_snapshot_t *out);
/* Trusted worker/actor entrances, never HTTP fields. */
bool dsr_automation_auto_allowed(uint32_t rule_id,uint32_t cycle,uint32_t generation,uint32_t layout_id,uint8_t half);
bool dsr_automation_job_owned(uint32_t job_id);
dsr_result_t dsr_automation_worker_allowed(uint32_t job_id,const dsr_connection_t *connection);
bool dsr_automation_prepare_read(const dsr_fleet_t *fleet,uint32_t job_id,uint32_t now_ms,dsr_job_t *out);
void dsr_automation_complete_read(const dsr_event_t *event,uint32_t now_ms);
void dsr_automation_read_rejected(uint32_t job_id,dsr_result_t result);
bool dsr_automation_binding_matches(uint32_t layout_id,const dsr_connection_t *connection);
#endif
