#ifndef DSR_HISTORY_H
#define DSR_HISTORY_H
#include "dsr_platform.h"
#include "dsr_clock.h"
#define DSR_HISTORY_POINTS 289u
#define DSR_HISTORY_STEP 300u
#define DSR_HISTORY_W 1u
#define DSR_HISTORY_V 2u
#define DSR_HISTORY_A 4u
#define DSR_HISTORY_WH 8u
#define DSR_HISTORY_RESET 1u
#define DSR_HISTORY_GAP 2u
typedef struct {
 uint32_t utc;
 float power,voltage,current;
 double energy_wh;
 uint16_t count[3];
 uint8_t mask,flags;
} dsr_history_point_t;
/* kind0 = the existing Switch/relay adapter. Other kinds remain unsupported
 * and preserved on disk; future Cover must not alias switch:channel. */
typedef struct {char device_id[64];uint32_t layout_id;uint8_t generation,channel,kind;} dsr_history_key_t;
typedef enum {DSR_HISTORY_LOADING,DSR_HISTORY_READY,DSR_HISTORY_UNAVAILABLE,DSR_HISTORY_IO,DSR_HISTORY_FUTURE} dsr_history_state_t;
typedef struct {
 dsr_history_state_t state;
 bool clock_valid,clock_pending,clock_busy,durable;
 dsr_clock_result_t clock_result;
 uint32_t now_utc,step_seconds,retention_seconds;
 uint32_t samples_without_time;
} dsr_history_status_t;
typedef struct {
 dsr_history_status_t status;
 dsr_history_key_t key;
 uint32_t from_utc,to_utc;
 uint16_t count;
 dsr_history_point_t points[DSR_HISTORY_POINTS];
} dsr_history_series_t;
/* Optional bounded PSRAM allocation only; no Flash/radio/task here. Failure
 * disables history while ordinary controls continue. Main calls before UI. */
esp_err_t dsr_history_init(void);
void dsr_history_status(dsr_history_status_t *out);
/* LVGL owner only. Genuine correlated new accepted GET/SET readback only.
 * Never feed a copied runtime snapshot, error, stale sample or demo. RAM only. */
void dsr_history_record(const dsr_connection_t *connection,uint32_t request_id);
void dsr_history_binding(const dsr_connection_t *connection,bool enabled);
void dsr_history_forget(const dsr_device_t *device);
/* Exact physical identity/channel, optionally expected numeric layout ID.
 * Allocated series belongs to caller. One shared query for LCD and HTTPS;
 * only1h/24h, gaps remain missing points, UTC labels never guessed local time. */
dsr_history_series_t *dsr_history_query(const dsr_history_key_t *key,unsigned hours);
void dsr_history_series_free(dsr_history_series_t *series);
/* Existing sole worker integrates these payload-free service jobs. service
 * performs load/closed-bucket flush/rotation, never called by LVGL or HTTP.
 * service_due takes low32 ESP timer milliseconds, matching the worker's retry
 * clock (esp_timer_get_time()/1000), never the separately started LVGL clock. */
bool dsr_history_service_due(uint32_t now_ms);
void dsr_history_service(void);
bool dsr_history_request_clock(void);
bool dsr_history_clock_pending(void);
void dsr_history_clock_execute(void);
void dsr_history_cancel_clock(void);
/* Authenticated handler helpers: bounded exact JSON identity/hour parser and
 * serializer. No arbitrary target/file path, secrets or command execution. */
bool dsr_history_parse(const char *json,dsr_history_key_t *key,unsigned *hours);
char *dsr_history_json(const dsr_history_key_t *key,unsigned hours);
#endif
