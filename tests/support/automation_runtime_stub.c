/* Unconfigured legacy host fixtures: normal manual controls unchanged;
 * no automatic command is ever authorized by these test providers. */
#include "dsr_automation.h"
#include <string.h>
esp_err_t dsr_automation_init(void){return ESP_OK;}
void dsr_automation_tick(dsr_fleet_t *f,uint32_t n,uint32_t utc,bool online){(void)f;(void)n;(void)utc;(void)online;}
bool dsr_automation_manual_ready(uint32_t id,uint32_t intent){(void)id;return intent!=0;}
dsr_result_t dsr_automation_manual_result(uint32_t id,uint32_t intent){(void)id;return intent?DSR_OK:DSR_INVALID;}
static bool guard_error;
static dsr_auto_snapshot_t sample;
void test_automation_guard_error(bool enabled){guard_error=enabled;}
void test_automation_status(const dsr_auto_snapshot_t *value){sample=*value;}
bool dsr_automation_config_allowed(const cJSON *r,char error[96]){(void)r;strcpy(error,guard_error?"Disable the active automation before changing its contacts, targets or temperature":"");return !guard_error;}
void dsr_automation_config_published(const cJSON *r){(void)r;}
void dsr_automation_snapshot(dsr_auto_snapshot_t *out){*out=sample;}
bool dsr_automation_auto_allowed(uint32_t r,uint32_t c,uint32_t g,uint32_t id,uint8_t half){(void)r;(void)c;(void)g;(void)id;(void)half;return false;}
bool dsr_automation_binding_matches(uint32_t id,const dsr_connection_t *c){(void)id;(void)c;return false;}
bool dsr_automation_job_owned(uint32_t id){(void)id;return false;}
dsr_result_t dsr_automation_worker_allowed(uint32_t id,const dsr_connection_t *c){(void)id;(void)c;return DSR_OK;}
bool dsr_automation_prepare_read(const dsr_fleet_t *f,uint32_t id,uint32_t now,dsr_job_t *out){(void)f;(void)id;(void)now;(void)out;return false;}
void dsr_automation_read_rejected(uint32_t id,dsr_result_t r){(void)id;(void)r;}
void dsr_automation_complete_read(const dsr_event_t *e,uint32_t now){(void)e;(void)now;}
