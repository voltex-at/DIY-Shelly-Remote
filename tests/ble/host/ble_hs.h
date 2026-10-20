#pragma once
#include <stdint.h>
#define BLE_GAP_EVENT_DISC 1
#define BLE_GAP_EVENT_DISC_COMPLETE 2
#define BLE_HS_ETIMEOUT 13
#define BLE_HS_EUNKNOWN 17
void ble_hs_sched_reset(int reason);
typedef struct {uint8_t type,val[6];} ble_addr_t;
struct ble_gap_disc_desc {ble_addr_t addr;int8_t rssi;uint8_t length_data;const uint8_t *data;};
struct ble_gap_event {int type;union {struct ble_gap_disc_desc disc;struct {int reason;} disc_complete;};};
struct ble_gap_disc_params {uint16_t itvl,window;uint8_t filter_policy;unsigned limited:1,passive:1,filter_duplicates:1;};
struct test_ble_hs_cfg {void (*reset_cb)(int);void (*sync_cb)(void);};
extern struct test_ble_hs_cfg ble_hs_cfg;
int ble_hs_id_infer_auto(int privacy,uint8_t *type);
int ble_gap_disc(uint8_t type,int32_t duration,const struct ble_gap_disc_params *params,int (*callback)(struct ble_gap_event *,void *),void *arg);
int ble_gap_disc_cancel(void);
int ble_gap_disc_active(void);
