#ifndef DSR_PLATFORM_H
#define DSR_PLATFORM_H
#include "dsr_core.h"
#include "esp_err.h"
#include "dsr_battery.h"
#include "dsr_ap.h"

typedef enum { DSR_WIFI_OPEN, DSR_WIFI_WPA, DSR_WIFI_WPA2, DSR_WIFI_WPA3,
               DSR_WIFI_WPA2_WPA3, DSR_WIFI_UNSUPPORTED } dsr_wifi_security_t;
typedef struct {
    char ssid[33];
    char password[65];
    uint8_t brightness;
    uint8_t wifi_security;
    uint16_t idle_seconds;
} dsr_settings_t;
typedef struct { char username[33], password[65]; } dsr_credentials_t;
/* Volatile measurements, deliberately excluded from the legacy NVS record. */
typedef struct {bool has_voltage,has_current,has_energy;double voltage_v,current_a,energy_wh;} dsr_telemetry_t;
typedef struct { dsr_device_t device; dsr_credentials_t credentials; dsr_telemetry_t telemetry;uint32_t layout_id;bool legacy_fallback;} dsr_connection_t;
/* Volatile origin consumes tail padding; never enters installed raw records. */
_Static_assert(offsetof(dsr_connection_t,legacy_fallback)==offsetof(dsr_connection_t,layout_id)+4&&sizeof(dsr_connection_t)==offsetof(dsr_connection_t,layout_id)+8,"origin consumes existing connection tail padding");
_Static_assert(sizeof(dsr_telemetry_t)<=32,"bounded telemetry extension for history/Cover");

/* Prototype storage is unencrypted NVS. Never log credentials. */
esp_err_t dsr_settings_load(dsr_settings_t *out);
esp_err_t dsr_settings_save(const dsr_settings_t *settings);
esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found);
/* Schema-2 endpoint registry. Legacy single record remains untouched. */
esp_err_t dsr_connections_load(dsr_connection_t *out,size_t capacity,size_t *count);
esp_err_t dsr_connection_save(const dsr_connection_t *connection);
esp_err_t dsr_connection_layout_id(const dsr_connection_t *connection,uint32_t *id);
esp_err_t dsr_connection_publish(const dsr_connection_t *connection);
esp_err_t dsr_connection_remove(const dsr_connection_t *connection);
bool dsr_connection_valid(const dsr_connection_t *connection, bool bound);
/* Durable shared mapping guard used before the sole worker publishes NVS. */
bool dsr_workspace_mapping_matches(const dsr_connection_t *connection);
/* Initialize once, even without credentials, so scanning is available. */
esp_err_t dsr_wifi_start(const dsr_settings_t *settings);
bool dsr_wifi_connected(void);
#define DSR_WIFI_MAX_NETWORKS 20
typedef struct {
    char ssid[33]; uint8_t bssid[6]; int8_t rssi; uint8_t channel; dsr_wifi_security_t security;
} dsr_wifi_ap_t;
typedef struct { dsr_wifi_ap_t items[DSR_WIFI_MAX_NETWORKS]; uint8_t count; } dsr_wifi_scan_t;
typedef enum {
    DSR_WIFI_IDLE, DSR_WIFI_CONNECTING, DSR_WIFI_CONNECTED,
    DSR_WIFI_BAD_PASSWORD, DSR_WIFI_NOT_FOUND, DSR_WIFI_TIMED_OUT,
    DSR_WIFI_DRIVER_ERROR, DSR_WIFI_STORAGE_ERROR, DSR_WIFI_CANCELLED
} dsr_wifi_status_t;
typedef struct { dsr_wifi_status_t status; char ssid[33], ip[16]; bool connected; } dsr_wifi_info_t;
void dsr_wifi_get_info(dsr_wifi_info_t *out);
bool dsr_wifi_settings_valid(const dsr_settings_t *settings);
/* Worker only. Save after DHCP; restore previous settings on failure. */
dsr_result_t dsr_wifi_scan(dsr_wifi_scan_t *out);
dsr_result_t dsr_wifi_connect_saved(const dsr_settings_t *settings, dsr_wifi_status_t *detail);
void dsr_wifi_cancel_connect(void); /* Nonblocking, safe from UI. */
bool dsr_wifi_temporary_active(void);
dsr_result_t dsr_wifi_temporary_connect(const dsr_wifi_ap_t *ap,dsr_wifi_status_t *detail);
dsr_result_t dsr_wifi_restore_home(dsr_wifi_status_t *detail);
void dsr_wifi_prepare_connect(void); /* Worker submit clears stale cancellation before queueing. */

/* Worker only. No redirects; Basic Gen1 / SHA-256 Digest Gen2+ via IDF.
 * Refresh and write verify the saved identity before using an endpoint. */
dsr_result_t dsr_http_probe(const dsr_connection_t *connection, dsr_device_t *out);
dsr_result_t dsr_http_probe_telemetry(const dsr_connection_t *connection,dsr_device_t *out,dsr_telemetry_t *telemetry);
dsr_result_t dsr_http_refresh(dsr_connection_t *connection);
dsr_result_t dsr_http_set_output(dsr_connection_t *connection, bool on);

typedef enum { DSR_JOB_PROBE, DSR_JOB_SET_OUTPUT, DSR_JOB_BIND, DSR_JOB_WIFI_SCAN, DSR_JOB_WIFI_CONNECT,DSR_JOB_BATTERY_READ, DSR_JOB_DISPLAY_SAVE, DSR_JOB_LAYOUT_PUBLISH, DSR_JOB_UNBIND, DSR_JOB_AP_SCAN, DSR_JOB_AP_INSPECT, DSR_JOB_AP_PROVISION, DSR_JOB_AP_RESTORE, DSR_JOB_AP_VERIFY,DSR_JOB_OTA_CHECK,DSR_JOB_OTA_INSTALL,DSR_JOB_LAN_STA_VERIFY,DSR_JOB_HISTORY_SERVICE,DSR_JOB_HISTORY_CLOCK } dsr_job_kind_t;
typedef struct {
    uint32_t id;
    dsr_job_kind_t kind;
    dsr_connection_t connection;
    dsr_settings_t settings;
    bool on,ap_confirmed;
    union {dsr_wifi_ap_t ap;dsr_ap_identity_t ap_identity;};
} dsr_job_t;
/* Credentials are never returned through the event queue. */
typedef struct {
    uint32_t id; dsr_result_t result; dsr_device_t device;
    uint32_t layout_id;
    dsr_result_t ap_write_result;
    dsr_telemetry_t telemetry;
    union {dsr_wifi_scan_t scan;dsr_ap_identity_t ap_identity;}; dsr_wifi_status_t wifi_detail;
    dsr_battery_sample_t battery;
    uint8_t brightness; uint16_t idle_seconds;
} dsr_event_t;
typedef enum {DSR_BIND_VERIFY,DSR_BIND_WORKSPACE,DSR_BIND_PRIVATE} dsr_bind_stage_t;
/* One outstanding operation until receive; bounded queues, no command replay. */
esp_err_t dsr_worker_start(void);
dsr_result_t dsr_worker_submit(const dsr_job_t *job);
bool dsr_worker_receive(dsr_event_t *event);
#endif
