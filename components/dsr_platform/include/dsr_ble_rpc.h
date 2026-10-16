#ifndef DSR_BLE_RPC_H
#define DSR_BLE_RPC_H
#include "dsr_ble.h"
#include "dsr_discovery.h"
typedef enum {DSR_BLE_RPC_IDLE,DSR_BLE_RPC_CONNECTING,DSR_BLE_RPC_INSPECTING,DSR_BLE_RPC_CONFIRM,DSR_BLE_RPC_WRITING,DSR_BLE_RPC_VERIFYING,DSR_BLE_RPC_CLOSING,DSR_BLE_RPC_LAN_READY,DSR_BLE_RPC_CANCELLED,DSR_BLE_RPC_ERROR,DSR_BLE_RPC_QUARANTINED} dsr_ble_rpc_phase_t;
typedef struct {
 dsr_ble_rpc_phase_t phase;
 dsr_result_t result;
 uint32_t serial,generation;
 bool active,has_identity,write_possible;
 dsr_lan_device_t device;
 char firmware[32];
} dsr_ble_rpc_snapshot_t;
/* Trusted LCD controller only. Candidates are copies resolved from the shared
 * real scanner, never a MAC/credential supplied by an external client. */
dsr_result_t dsr_ble_rpc_start(const dsr_ble_device_t *candidate);
dsr_result_t dsr_ble_rpc_confirm(uint32_t generation,const char *home_ssid);
void dsr_ble_rpc_cancel(uint32_t generation);
void dsr_ble_rpc_snapshot(dsr_ble_rpc_snapshot_t *snapshot);
bool dsr_ble_rpc_active(void);
bool dsr_ble_rpc_sensitive(void);
void dsr_ble_rpc_visibility(bool visible);
#endif
