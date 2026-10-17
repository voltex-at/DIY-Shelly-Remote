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
/* blu: bit i = gateway reports BTHomeDevice cid 200+i (Shelly BLU sensor). */
typedef struct {uint16_t switches, covers, lights, rgbs;bool sensors,ht;uint16_t blu;} dsr_lan_capabilities_t;
_Static_assert(sizeof(dsr_lan_capabilities_t)==12,"bounded capability record");
/* What a gateway says about each BLU channel (cid 200+i): the name set on the
 * gateway (may be empty), attrs.model_id and the last two MAC bytes. Display
 * only; identity is still proven by the BLU probe at bind time. */
typedef struct {char name[24];uint8_t model,mac_tail[2];} dsr_blu_label_t;
typedef struct {uint16_t valid;dsr_blu_label_t items[16];} dsr_blu_labels_t;
_Static_assert(sizeof(dsr_blu_labels_t)<=448,"bounded BLU label table");
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
/* Valid HT bundle requires all three actual ID0 components; measurements may be null/missing. */
dsr_result_t dsr_ht_parse(const cJSON *json,dsr_telemetry_t *out);
/* Shelly BLU read through a Gen2+ gateway. cid = 200 + endpoint channel. */
typedef struct {uint16_t sensor;uint8_t obj_id;} dsr_blu_object_t;
#define DSR_BLU_MAX_OBJECTS 8u
dsr_result_t dsr_blu_parse_address(const cJSON *config,unsigned cid,char mac[13]);
dsr_result_t dsr_blu_parse_known(const cJSON *known,unsigned cid,dsr_blu_object_t *objects,unsigned capacity,unsigned *count);
dsr_result_t dsr_blu_parse_device(const cJSON *status,unsigned cid,dsr_telemetry_t *out);
dsr_result_t dsr_blu_apply_sensor(const cJSON *status,unsigned sensor,uint8_t obj_id,dsr_telemetry_t *out);
bool dsr_blu_sensor_shown(uint8_t obj_id);
dsr_result_t dsr_lan_parse_capabilities(const cJSON *json,unsigned generation,dsr_lan_capabilities_t *out);
/* One Shelly.GetComponents?dynamic_only=true page: BTHomeDevice cids 200..215
 * become BLU channel bits. Verifies the echoed offset; *next is offset+items. */
dsr_result_t dsr_lan_parse_components(const cJSON *page,unsigned offset,uint16_t *blu,dsr_blu_labels_t *labels,unsigned *next,unsigned *total);
/* Verified product name for a BTHome attrs.model_id, or NULL when unknown. */
const char *dsr_blu_model_name(uint8_t model);
/* BluTrv.GetConfig: true only when this BluTrv instance is linked to
 * bthomedevice:<cid> ("trv" field). */
bool dsr_blu_trv_linked(const cJSON *config,unsigned id,unsigned cid);
/* BluTrv.GetStatus into the BLU record: target/current C, valve %, errors. */
dsr_result_t dsr_blu_parse_trv(const cJSON *status,unsigned id,dsr_telemetry_t *out);
dsr_result_t dsr_lan_upsert(dsr_lan_snapshot_t *scan,const dsr_lan_device_t *device);
bool dsr_lan_plan_add(dsr_lan_plan_t *plan,const char *host,const char *local,const char *mask);

/* Real HTTP requests. Public identity has no credentials; protected status is
 * read only after selection, with Gen1 Basic / Gen2+ Digest from existing code. */
dsr_result_t dsr_http_lan_identity(const char *host,dsr_lan_device_t *out);
dsr_result_t dsr_http_lan_inspect(const dsr_lan_device_t *expected,const dsr_credentials_t *credentials,dsr_lan_capabilities_t *out,dsr_blu_labels_t *labels);

/* Nonblocking UI/API entry points. One separate bounded network task. */
esp_err_t dsr_lan_init(void);
bool dsr_lan_scan(void);
void dsr_lan_cancel(void);
/* Worker-only radio handoff barrier; rejects new LAN jobs until resume. */
dsr_result_t dsr_lan_suspend(void);
void dsr_lan_resume(void);
bool dsr_lan_inspect(const char *host,const char *id,const dsr_credentials_t *credentials);
void dsr_lan_snapshot(dsr_lan_snapshot_t *out);
/* BLU labels from the last successful inspection (valid==0 otherwise). */
void dsr_lan_blu_labels(dsr_blu_labels_t *out);
#endif
