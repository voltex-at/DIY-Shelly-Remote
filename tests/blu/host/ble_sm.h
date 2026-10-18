#pragma once
#include "ble_hs.h"
#define BLE_HS_IO_KEYBOARD_ONLY 2
#define BLE_SM_PAIR_KEY_DIST_ENC 1
#define BLE_SM_PAIR_KEY_DIST_ID 2
#define BLE_SM_IOACT_INPUT 2
struct ble_sm_io {uint8_t action;uint32_t passkey;};
int ble_sm_inject_io(uint16_t,struct ble_sm_io *);
