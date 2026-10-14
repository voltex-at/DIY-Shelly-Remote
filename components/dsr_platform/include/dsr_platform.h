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

/* NVS must be initialized by app_main. Passwords are never logged. */
esp_err_t dsr_settings_load(dsr_settings_t *out);
esp_err_t dsr_settings_save(const dsr_settings_t *settings);
esp_err_t dsr_wifi_start(const dsr_settings_t *settings);
bool dsr_wifi_connected(void);

/* Blocking transport; run from network worker only, never the UI task.
 * No credential forwarding or redirects. HTTP 401/403 is explicit; digest
 * authentication is intentionally not represented as implemented yet. */
dsr_result_t dsr_http_probe(const char *host, uint8_t channel, dsr_device_t *out);
dsr_result_t dsr_http_refresh(dsr_device_t *device);
dsr_result_t dsr_http_set_output(dsr_device_t *device, bool on);

typedef enum { DSR_JOB_PROBE, DSR_JOB_REFRESH, DSR_JOB_SET_OUTPUT } dsr_job_kind_t;
typedef struct {
    uint32_t id;
    dsr_job_kind_t kind;
    dsr_device_t device;
    bool on;
} dsr_job_t;
typedef struct { uint32_t id; dsr_result_t result; dsr_device_t device; } dsr_event_t;
/* One network task owns transport operations. Both queues are bounded.
 * One outstanding job; submit returns BUSY until its result has been received.
 * No silent dropping or replaying commands after a connection error. */
esp_err_t dsr_worker_start(void);
dsr_result_t dsr_worker_submit(const dsr_job_t *job);
bool dsr_worker_receive(dsr_event_t *event);
#endif
