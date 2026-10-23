#ifndef DSR_ACTIONS_H
#define DSR_ACTIONS_H
#include "dsr_platform.h"
#define DSR_ACTION_STEPS 16u
#define DSR_ACTION_LOG_COUNT 128u
#define DSR_ACTION_LOG_PAGE 32u
typedef enum {DSR_ACTION_ON=1,DSR_ACTION_OFF,DSR_ACTION_OPEN,DSR_ACTION_CLOSE,DSR_ACTION_STOP,
 DSR_ACTION_POSITION,DSR_ACTION_BRIGHTNESS,DSR_ACTION_RGB,DSR_ACTION_TARGET} dsr_action_command_t;
/* Public group intent only: position/brightness 0..100, target in half degrees,
 * RGB uses all three bytes. It never contains a locator or credentials. */
typedef struct {uint8_t command,value[3];} dsr_group_intent_t;
typedef enum {DSR_ACTION_WAITING,DSR_ACTION_SUCCESS,DSR_ACTION_FAILED,DSR_ACTION_UNCERTAIN,DSR_ACTION_CANCELLED} dsr_action_outcome_t;
typedef struct {uint32_t layout_id;uint8_t command,outcome;dsr_result_t result;} dsr_action_result_t;
typedef struct {
 uint32_t boot_id,admission_generation,revision,run_id,object_id;
 char nonce[17]; /* Last public request token, never a credential. */
 uint8_t source,kind,count,current;bool available,active,cancelled;
 dsr_action_result_t steps[DSR_ACTION_STEPS];
} dsr_actions_snapshot_t;
_Static_assert(sizeof(dsr_actions_snapshot_t)<=2048,"bounded public action snapshot");
/* Exactly32 bytes. No identity/host/MAC/name/credentials. UTC0 is unset.
 * kind0/1=group/scene,2=state change,3=manual completion; command0=run admission/summary.
 * kind2 is an observation, never a command ACK: command0=availability/error,
 * ON/OFF only describe a changed Switch output; resultOFFLINE denotes a
 * transport loss even if the cached gate was OK. Render kind2 separately.
 * source0=WWW,1=LCD,2=observed runtime. RAM-only and empty after restart. */
typedef struct {
 uint32_t sequence,utc,monotonic_ms,run_id,object_id,layout_id;
 uint8_t source,kind,command,outcome;uint16_t result,reserved;
} dsr_action_log_entry_t;
_Static_assert(sizeof(dsr_action_log_entry_t)==32,"bounded redacted activity entry");
typedef struct {dsr_action_log_entry_t entries[DSR_ACTION_LOG_PAGE];uint32_t epoch,newest;uint16_t count,total;} dsr_action_log_page_t;
_Static_assert(sizeof(dsr_action_log_page_t)<=2048,"bounded activity query");
/* Optional PSRAM only; no new task/flash/NVS/SNTP owner. */
esp_err_t dsr_actions_init(void);
void dsr_actions_snapshot(dsr_actions_snapshot_t *out);
/* Bounded strict JSON: id/revision/boot_id/admission_generation/nonce;
 * groups(kind0) require on:boolean OR command with its typed value/rgb payload;
 * scenes(kind1) forbid both. Last nonce
 * returns the same run even after completion. A new run consumes generation.
 * source is supplied by the trusted handler, never accepted from JSON. */
dsr_result_t dsr_actions_request(const char *json,uint8_t kind,uint8_t source,uint32_t *run_id);
dsr_result_t dsr_actions_auto_request(uint32_t rule_id,uint32_t cycle,uint32_t generation,
 const uint32_t *ids,const uint8_t *halves,size_t count,uint32_t *run_id);
bool dsr_actions_auto_result(uint32_t run_id,dsr_result_t *result);
bool dsr_actions_auto_attempted(uint32_t run_id);
void dsr_actions_auto_cancel(uint32_t run_id);
bool dsr_actions_cancel(uint32_t boot_id,uint32_t run_id);
bool dsr_actions_reserved(const dsr_device_t *device);
bool dsr_actions_active(void);
/* LVGL publishes a secret-free private identity/locator cache. Each begin/end
 * pair owns the actions mutex only; never call workspace APIs inside it. */
bool dsr_actions_publish_begin(uint32_t revision,bool available);
void dsr_actions_publish_at(const dsr_connection_t *connection,bool enabled,bool pending,dsr_result_t gate,bool fresh);
void dsr_actions_publish_end(void);
/* Sole UI owner dispatches one ordinary worker job at a time. Target returns
 * numeric ID only. Prepare reserves exact jobID before worker admission;
 * unqueue only undoes a rejected submission, never a sent command. */
uint32_t dsr_actions_target(void);
dsr_action_command_t dsr_actions_command(void);
bool dsr_actions_job_owned(uint32_t job_id);
bool dsr_actions_auto_job_owned(uint32_t job_id);
bool dsr_actions_job_attempted(uint32_t job_id);
bool dsr_actions_native_reserve(uint32_t layout_id);
void dsr_actions_native_release(uint32_t layout_id);
bool dsr_actions_prepare(const dsr_connection_t *connection,dsr_result_t gate,bool fresh,bool capable,uint32_t job_id,dsr_job_kind_t *kind,bool *on);
/* Same reservation/guard contract, with the typed payload frozen at admission. */
bool dsr_actions_prepare_job(dsr_job_t *job,dsr_result_t gate,bool fresh);
void dsr_actions_missing(uint32_t layout_id);
void dsr_actions_unqueue(uint32_t job_id);
void dsr_actions_complete(uint32_t job_id,dsr_result_t result);
void dsr_actions_mode_block(void); /* Cancel remaining work before radio modes. */
/* Actual worker calls allowed before first I/O and again before write legs.
 * It is a no-op for unrelated jobIDs. No credentials retained by actor. */
dsr_result_t dsr_actions_worker_allowed(uint32_t job_id,const dsr_connection_t *connection);
/* Exact registered command attempt, called only by the HTTP control-boundary
 * hook. Later errors preserve their reason but cannot claim zero writes. */
void dsr_actions_worker_write_attempted(uint32_t job_id);
bool dsr_actions_log(uint32_t epoch,unsigned offset,unsigned limit,dsr_action_log_page_t *page);
void dsr_actions_log_clear(void);
/* Correlated manual command completion; numeric layout only. source2 leaves
 * the origin unattributed rather than guessing LCD/WWW from operationIDs. */
void dsr_actions_observe(uint32_t layout_id,dsr_action_command_t command,dsr_result_t result);
char *dsr_actions_status_json(void);
char *dsr_actions_log_json(uint32_t epoch,unsigned offset,unsigned limit);
bool dsr_actions_cancel_parse(const char *json,uint32_t *boot_id,uint32_t *run_id);
bool dsr_actions_log_parse(const char *json,uint32_t *epoch,unsigned *offset,unsigned *limit);
#endif
