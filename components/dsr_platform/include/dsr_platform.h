#ifndef DSR_PLATFORM_H
#define DSR_PLATFORM_H
#include "dsr_core.h"
#include "esp_err.h"

typedef struct {
    char ssid[33];
    char password[65];
    uint8_t brightness;
    uint16_t idle_seconds;
} dsr_settings_t;
typedef struct { char username[33], password[65]; } dsr_credentials_t;
typedef struct { dsr_device_t device; dsr_credentials_t credentials; } dsr_connection_t;

/* Prototype storage is unencrypted NVS. Never log credentials. */
esp_err_t dsr_settings_load(dsr_settings_t *out);
esp_err_t dsr_settings_save(const dsr_settings_t *settings);
esp_err_t dsr_connection_load(dsr_connection_t *out, bool *found);
esp_err_t dsr_connection_save(const dsr_connection_t *connection);
bool dsr_connection_valid(const dsr_connection_t *connection, bool bound);
esp_err_t dsr_wifi_start(const dsr_settings_t *settings); /* once per boot */
bool dsr_wifi_connected(void);

/* Worker only. No redirects; Basic Gen1 / SHA-256 Digest Gen2+ via IDF.
 * Refresh and write verify the saved identity before using an endpoint. */
dsr_result_t dsr_http_probe(const dsr_connection_t *connection, dsr_device_t *out);
dsr_result_t dsr_http_refresh(dsr_connection_t *connection);
dsr_result_t dsr_http_set_output(dsr_connection_t *connection, bool on);

typedef enum { DSR_JOB_PROBE, DSR_JOB_SET_OUTPUT, DSR_JOB_BIND, DSR_JOB_SAVE_WIFI } dsr_job_kind_t;
typedef struct {
    uint32_t id;
    dsr_job_kind_t kind;
    dsr_connection_t connection;
    dsr_settings_t settings;
    bool on;
} dsr_job_t;
/* Credentials are never returned through the event queue. */
typedef struct { uint32_t id; dsr_result_t result; dsr_device_t device; } dsr_event_t;
/* One outstanding operation until receive; bounded queues, no command replay. */
esp_err_t dsr_worker_start(void);
dsr_result_t dsr_worker_submit(const dsr_job_t *job);
bool dsr_worker_receive(dsr_event_t *event);
#endif
