#ifndef DSR_BLE_SHARED_H
#define DSR_BLE_SHARED_H
#include <stdbool.h>
/* Same existing scanner manager and NimBLE host, one active BLE operation. */
bool dsr_ble_manager_rpc(void (*operation)(bool host_ready));
bool dsr_ble_host_healthy(void);
/* Manager-task only, after bounded disconnect failed. Seal new BLE work until
 * the old host is stopped/deinitialized. Never retain the global activity
 * lease while waiting for a missing GAP callback; restart remains possible. */
void dsr_ble_manager_quarantine(void);
#define DSR_BLE_CLOSE_MS 4000u
#define DSR_BLU_OPERATION_MS 120000u
#endif
