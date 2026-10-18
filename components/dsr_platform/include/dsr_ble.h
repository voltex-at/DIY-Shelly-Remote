#ifndef DSR_BLE_H
#define DSR_BLE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define DSR_BLE_MAX_DEVICES 16
#define DSR_BLE_SCAN_MS 20000u
/* Host + controller are released after this long without a BLE job (0.7.52). */
#define DSR_BLE_IDLE_RELEASE_MS 30000u
#define DSR_BLE_MAX_ADV 31u
typedef enum {DSR_BLE_IDLE,DSR_BLE_STARTING,DSR_BLE_SCANNING,DSR_BLE_DONE,DSR_BLE_CANCELLED,DSR_BLE_UNAVAILABLE} dsr_ble_phase_t;
typedef enum {DSR_BLE_SHELLY_RPC,DSR_BLE_SHELLY_BLU,DSR_BLE_OTHER} dsr_ble_kind_t;
enum {DSR_BLE_MFG=1,DSR_BLE_RPC=2,DSR_BLE_BTHOME=4,DSR_BLE_BLU_SERVICE=8,DSR_BLE_NAME=16,DSR_BLE_RPC_FLAG=32};
/* Shelly 0x0BA9 manufacturer block 0x01 flag bits (shelly-api-docs, docs-ble). */
enum {DSR_BLE_FLAG_DISCOVERABLE=1,DSR_BLE_FLAG_AUTH=2,DSR_BLE_FLAG_RPC=4,DSR_BLE_FLAG_BUZZER=8,DSR_BLE_FLAG_PAIRING=16};
/* Any hint that a Gen2+ Wi-Fi Shelly may answer RPC over GATT. The 128-bit
 * service UUID rarely fits the 31-byte legacy advertisement, so the Shelly
 * manufacturer RPC flag or a "Shelly…" local name also qualify; the GATT
 * session still verifies the real service before anything is written. */
#define DSR_BLE_RPC_HINTS (DSR_BLE_RPC|DSR_BLE_RPC_FLAG|DSR_BLE_NAME)
typedef struct {
    uint8_t address[6],address_type,evidence,bthome_version;
    int8_t rssi;
    char name[32];
    uint16_t model_id,flags;
    uint8_t mac[6];
    bool has_model,encrypted,has_flags,has_mac;
    dsr_ble_kind_t kind;
    uint32_t seen_ms;
} dsr_ble_device_t;
typedef struct {
    dsr_ble_phase_t phase;
    bool busy;
    uint32_t serial,generation,started_ms,elapsed_ms,packets,malformed,dropped,snapshot_ms;
    uint8_t count;
    dsr_ble_device_t devices[DSR_BLE_MAX_DEVICES];
} dsr_ble_snapshot_t;

/* Legacy AD structures / scan responses, not GATT, bonding or provisioning.
 * Radio names/company IDs are discovery hints, never authenticated identity. */
bool dsr_ble_parse(const uint8_t *data,size_t length,dsr_ble_device_t *out);
const char *dsr_ble_model_name(const dsr_ble_device_t *device);
void dsr_ble_begin(dsr_ble_snapshot_t *state,uint32_t now);
bool dsr_ble_ingest(dsr_ble_snapshot_t *state,uint32_t generation,const uint8_t address[6],uint8_t type,int8_t rssi,const uint8_t *data,size_t length,uint32_t now);
bool dsr_ble_finish(dsr_ble_snapshot_t *state,uint32_t generation,dsr_ble_phase_t phase,uint32_t now);

/* Snapshot and requests only; no LVGL or network configuration in callbacks. */
esp_err_t dsr_ble_init(void);
bool dsr_ble_scan(void);
void dsr_ble_cancel(void);
void dsr_ble_snapshot(dsr_ble_snapshot_t *out);
/* Authenticated WWW handoff: only a current scan serial/index is accepted.
 * Candidate resolution/copy is atomic under the shared scanner mutex. No
 * address or credential is accepted from the browser; LCD owns both consents. */
typedef enum {DSR_BLE_HANDOFF_UNAVAILABLE,DSR_BLE_HANDOFF_READY,DSR_BLE_HANDOFF_REQUESTED,DSR_BLE_HANDOFF_ACTIVE,DSR_BLE_HANDOFF_EXPIRED} dsr_ble_handoff_t;
void dsr_ble_handoff_publish(bool available,bool active,uint32_t now_ms);
bool dsr_ble_handoff_request(uint32_t scan_serial,unsigned index);
bool dsr_ble_handoff_take(dsr_ble_device_t *candidate,uint32_t now_ms);
dsr_ble_handoff_t dsr_ble_handoff_status(uint32_t now_ms);
bool dsr_ble_handoff_parse(const char *json,uint32_t *serial,unsigned *index);
bool dsr_ble_host_active(void); /* BLE host/controller currently initialized */
#endif
