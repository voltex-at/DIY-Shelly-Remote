#ifndef DSR_WORKSPACE_H
#define DSR_WORKSPACE_H
#include "dsr_config.h"
#include "esp_err.h"
#include "dsr_platform.h"
#include "dsr_boot.h"
#include "dsr_config_health.h"
#include "dsr_automation_types.h"
#define DSR_PHOTO_WIDTH 320
#define DSR_PHOTO_HEIGHT 180
#define DSR_PHOTO_BYTES (DSR_PHOTO_WIDTH*DSR_PHOTO_HEIGHT*2)
/* These APIs never call LVGL. All flash work belongs to dsr_config task. */
esp_err_t dsr_workspace_start(void);
char *dsr_workspace_snapshot(void); /* malloc; caller frees */
typedef enum {DSR_SNAPSHOT_OK,DSR_SNAPSHOT_UNAVAILABLE,DSR_SNAPSHOT_NO_MEMORY} dsr_snapshot_result_t;
dsr_snapshot_result_t dsr_workspace_snapshot_checked(char **out);
uint32_t dsr_workspace_revision(void);
/* Nonblocking. Configuration stays unchanged until durable write succeeds. */
bool dsr_workspace_submit(const char *json,uint32_t revision);
void dsr_workspace_status(bool *busy,char message[96]);
bool dsr_workspace_auto_submit(const dsr_auto_record_t *record,uint32_t generation);
bool dsr_workspace_auto_take_result(uint32_t *generation,esp_err_t *result);
/* One copied startup result; NVS was read before LVGL starts. */
bool dsr_workspace_auto_boot(dsr_auto_record_t *record,bool *found,esp_err_t *result);
/* Pairing requires a local touch to open a 120-second, rate-limited window.
 * No shared/factory password. Pairing credentials must never be logged. */
void dsr_panel_pair_begin(char code[17]);
/* True only when the ready panel accepted the physical 120-second window. */
/* Physical LCD action only: true attests stable USB for the one-shot window. */
bool dsr_panel_allow_ota(bool stable_usb_confirmed);
esp_err_t dsr_panel_start(void);
bool dsr_panel_ready(void);
/* LVGL owns rendering; the server only requests/copies a published frame. */
bool dsr_panel_preview_requested(void);
void dsr_panel_preview_publish(const void *rgb565,size_t size);
void dsr_panel_tick(void); /* called by existing LVGL owner task */
void dsr_photos_load(void);
const void *dsr_photo_pixels(const char *id);
bool dsr_photo_save(const char *id,const void *pixels,size_t size);
const char *dsr_photo_id(unsigned index);
esp_err_t dsr_tls_identity(void);
const unsigned char *dsr_tls_certificate(void);
const unsigned char *dsr_tls_key(void);
const char *dsr_tls_fingerprint(void);
bool dsr_workspace_bind(const dsr_connection_t *connection);
bool dsr_workspace_bind_with_room(const dsr_connection_t *connection,uint32_t room);
bool dsr_workspace_take_bind_with_room(dsr_connection_t *connection,uint32_t *room);
void dsr_workspace_bind_result(dsr_result_t result);
void dsr_workspace_bind_result_stage(dsr_result_t result,dsr_bind_stage_t stage);
void dsr_workspace_bind_status(bool *busy,char message[64]);
dsr_result_t dsr_workspace_bind_result_code(void);
/* Secret-free WWW -> LCD request; the LVGL owner publishes eligibility and
 * calls take only when it can open the local wizard. Worker BUSY may defer
 * take without making the wizard unavailable. All callers use the same
 * monotonic millisecond clock. Pending requests expire after 10 seconds.
 * No flash work or retained activity lease: a restart may discard a request.
 * The LCD caller covers take -> wizard admission with its own activity. */
typedef enum {
 DSR_AP_HANDOFF_UNAVAILABLE=0,DSR_AP_HANDOFF_READY=1,
 DSR_AP_HANDOFF_REQUESTED=2,DSR_AP_HANDOFF_ACTIVE=3,
 DSR_AP_HANDOFF_EXPIRED=4
} dsr_ap_handoff_t;
void dsr_workspace_ap_publish(bool available,bool active,uint32_t now_ms);
bool dsr_workspace_request_ap_open(uint32_t now_ms);
bool dsr_workspace_take_ap_open(uint32_t now_ms);
dsr_ap_handoff_t dsr_workspace_ap_handoff(uint32_t now_ms);
/* Secret-free live data; UI owns providers, HTTP receives copies only. */
typedef struct {
 dsr_device_t device;
 dsr_telemetry_t telemetry;
 dsr_result_t error;
 bool pending,has_sample_time;union {bool can_stop;bool can_refresh;};bool stop_pending;
 uint32_t sample_age_ms,operation_id,layout_id;
} dsr_runtime_device_t;
typedef struct {dsr_device_t device;union {bool on;dsr_cover_intent_t cover;dsr_light_intent_t light;dsr_rgb_intent_t rgb;};uint32_t operation_id;} dsr_output_request_t;
bool dsr_workspace_runtime_publish_begin(size_t count);
void dsr_workspace_runtime_publish_end(void);
/* Advisory label for editors; never authorizes an action or binding. */
bool dsr_workspace_runtime_name(uint32_t id,const char *host,uint8_t channel,char out[48]);
void dsr_workspace_runtime_info_at(size_t index,const dsr_device_info_t *info);
bool dsr_workspace_runtime_info(uint32_t layout_id,const char *identity,dsr_device_info_t *info);
void dsr_workspace_runtime_publish_at(size_t index,size_t count,const dsr_runtime_device_t *device);
size_t dsr_workspace_runtime_snapshot(dsr_runtime_device_t *devices,size_t capacity);
dsr_result_t dsr_workspace_output_request(const dsr_device_t *device,bool on,uint32_t *operation_id);
dsr_result_t dsr_workspace_cover_request(const dsr_device_t *device,dsr_cover_intent_t intent,uint32_t *operation_id);
dsr_result_t dsr_workspace_ht_refresh(const dsr_device_t *device,uint32_t *operation_id);
dsr_result_t dsr_workspace_rgb_request(const dsr_device_t *device,dsr_rgb_intent_t intent,uint32_t *operation_id);
dsr_result_t dsr_workspace_light_request(const dsr_device_t *device,dsr_light_intent_t intent,uint32_t *operation_id);
bool dsr_workspace_take_output(dsr_output_request_t *request);
#endif
