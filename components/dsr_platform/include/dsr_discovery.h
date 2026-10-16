#ifndef DSR_DISCOVERY_H
#define DSR_DISCOVERY_H
#include "dsr_platform.h"
#include "cJSON.h"

#define DSR_LAN_MAX_DEVICES 16
#define DSR_LAN_MAX_HOSTS 32
typedef struct {
    char host[16], id[64], mac[13], model[40], name[48];
    uint8_t generation;
    bool auth_required;
} dsr_lan_device_t;
typedef struct {uint16_t switches, covers, lights, rgbs;bool sensors;} dsr_lan_capabilities_t;
typedef enum {DSR_LAN_IDLE,DSR_LAN_SCANNING,DSR_LAN_DONE,DSR_LAN_CANCELLED,
              DSR_LAN_NO_WIFI,DSR_LAN_INSPECTING,DSR_LAN_INSPECTED,DSR_LAN_ERROR} dsr_lan_phase_t;
typedef struct {
    dsr_lan_phase_t phase;
    dsr_result_t result;
    uint32_t serial;
    uint8_t count, probed, total, skipped;
    dsr_lan_device_t devices[DSR_LAN_MAX_DEVICES];
    dsr_lan_device_t selected;
    dsr_lan_capabilities_t capabilities;
} dsr_lan_snapshot_t;
typedef struct {char hosts[DSR_LAN_MAX_HOSTS][16];uint8_t count;} dsr_lan_plan_t;

/* Portable parsers. No model-name capability guesses and no fabricated rows. */
dsr_result_t dsr_shelly_parse_identity(const cJSON *json,dsr_lan_device_t *out);
dsr_result_t dsr_lan_parse_identity(const cJSON *json,const char *host,dsr_lan_device_t *out);
dsr_result_t dsr_lan_parse_capabilities(const cJSON *json,unsigned generation,dsr_lan_capabilities_t *out);
dsr_result_t dsr_lan_upsert(dsr_lan_snapshot_t *scan,const dsr_lan_device_t *device);
bool dsr_lan_plan_add(dsr_lan_plan_t *plan,const char *host,const char *local,const char *mask);

/* Real HTTP requests. Public identity has no credentials; protected status is
 * read only after selection, with Gen1 Basic / Gen2+ Digest from existing code. */
dsr_result_t dsr_http_lan_identity(const char *host,dsr_lan_device_t *out);
dsr_result_t dsr_http_lan_inspect(const dsr_lan_device_t *expected,const dsr_credentials_t *credentials,dsr_lan_capabilities_t *out);

/* Nonblocking UI/API entry points. One separate bounded network task. */
esp_err_t dsr_lan_init(void);
bool dsr_lan_scan(void);
void dsr_lan_cancel(void);
/* Worker-only radio handoff barrier; rejects new LAN jobs until resume. */
dsr_result_t dsr_lan_suspend(void);
void dsr_lan_resume(void);
bool dsr_lan_inspect(const char *host,const char *id,const dsr_credentials_t *credentials);
void dsr_lan_snapshot(dsr_lan_snapshot_t *out);
#endif
