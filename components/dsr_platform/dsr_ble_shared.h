#ifndef DSR_BLE_SHARED_H
#define DSR_BLE_SHARED_H
#include <stdbool.h>
/* Same existing scanner manager and NimBLE host, one active BLE operation. */
bool dsr_ble_manager_rpc(void (*operation)(bool host_ready));
bool dsr_ble_host_healthy(void);
#endif
