#ifndef DSR_ACTIONS_PRIVATE_H
#define DSR_ACTIONS_PRIVATE_H
#include "dsr_actions.h"
#define DSR_ACTION_DEADLINE_MS 180000u
typedef enum {DSR_ACTION_RESOLVE,DSR_ACTION_PROBE_READY,DSR_ACTION_PROBE_QUEUED,DSR_ACTION_PROBE_SENT,DSR_ACTION_WRITE_READY,DSR_ACTION_WRITE_QUEUED,DSR_ACTION_WRITE_SENT,DSR_ACTION_DONE} dsr_action_phase_t;
/* Private short-lived execution identity. Never exported into activity/API.
 * No credentials; locator is frozen so DHCP/rebind cannot retarget a run. */
typedef struct {char device_id[64],host[16];uint32_t layout_id;uint8_t generation,kind,channel;} dsr_action_key_t;
typedef struct {
 dsr_action_key_t key;
 uint32_t request_id;
 dsr_result_t result,rejected;
 dsr_action_command_t command;
 dsr_action_phase_t phase;
 dsr_action_outcome_t outcome;
 bool probed,write_attempted;
} dsr_action_step_t;
typedef struct {
 dsr_action_step_t steps[DSR_ACTION_STEPS];
 uint32_t run_id,generation,boot_id,object_id,revision,admitted_ms;
 char nonce[17];
 uint8_t count,current,source,kind;
 bool active,cancel;
} dsr_actions_model_t;
_Static_assert(sizeof(dsr_actions_model_t)<=8192,"bounded PSRAM actions actor");
bool dsr_action_key_equal(const dsr_action_key_t *a,const dsr_action_key_t *b);
bool dsr_actions_model_begin(dsr_actions_model_t *model,const dsr_action_step_t *steps,size_t count,
 uint32_t run_id,uint32_t expected_generation,uint32_t boot_id,uint32_t object_id,uint32_t revision,const char nonce[17],uint8_t source,uint8_t kind,uint32_t now_ms);
bool dsr_actions_model_duplicate(const dsr_actions_model_t *model,uint32_t boot_id,const char *nonce);
typedef struct {
 uint32_t object_id,revision,boot_id,admission_generation;
 char nonce[17];
 uint8_t kind;bool on;
} dsr_actions_request_t;
typedef struct {
 uint32_t layout_ids[DSR_ACTION_STEPS];
 uint8_t commands[DSR_ACTION_STEPS],count,kind;
 uint32_t object_id,revision;
} dsr_actions_plan_t;
bool dsr_actions_request_parse(const char *json,uint8_t kind,dsr_actions_request_t *request);
dsr_result_t dsr_actions_plan_build(const char *json,const dsr_actions_request_t *request,dsr_actions_plan_t *plan);
/* Sole-owner model methods. Wrapper provides its mutex; no SDK/I/O/allocation. */
int dsr_actions_model_current(dsr_actions_model_t *model,uint32_t now_ms);
void dsr_actions_model_cancel(dsr_actions_model_t *model);
bool dsr_actions_model_check(dsr_actions_model_t *model,unsigned index,const dsr_action_key_t *actual,
 dsr_result_t gate,bool fresh,bool capable,uint32_t now_ms);
bool dsr_actions_model_queued(dsr_actions_model_t *model,unsigned index,uint32_t request_id,uint32_t now_ms);
void dsr_actions_model_unqueue(dsr_actions_model_t *model,uint32_t request_id);
/* Called by the actual worker before first SDK I/O and each write leg. A
 * refusal is retained until the exact completion, never advances early. */
bool dsr_actions_model_worker_allowed(dsr_actions_model_t *model,uint32_t request_id,const dsr_action_key_t *actual,uint32_t now_ms);
bool dsr_actions_model_write_attempted(dsr_actions_model_t *model,uint32_t request_id);
bool dsr_actions_model_complete(dsr_actions_model_t *model,uint32_t request_id,dsr_result_t result,uint32_t now_ms);
/* Exactly32 bytes, no host/MAC/device identity/name/credentials. UTC0 means
 * unset; monotonic is boot-relative, never presented as wall-clock time. */
typedef struct {dsr_action_log_entry_t entries[DSR_ACTION_LOG_COUNT];uint32_t serial,epoch;uint16_t count,next;} dsr_action_log_model_t;
_Static_assert(sizeof(dsr_action_log_model_t)<=4096+32,"bounded PSRAM activity ring");
void dsr_action_log_add(dsr_action_log_model_t *log,const dsr_action_log_entry_t *entry);
void dsr_action_log_clear(dsr_action_log_model_t *log);
bool dsr_action_log_page(const dsr_action_log_model_t *log,uint32_t expected_epoch,unsigned offset,unsigned limit,dsr_action_log_page_t *page);
#endif
