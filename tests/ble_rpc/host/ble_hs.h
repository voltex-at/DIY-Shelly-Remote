#pragma once
#include <stdint.h>
#define BLE_HS_EDONE 14
#define BLE_HS_ATT_ERR(x) (0x100+(x))
#define BLE_ATT_ERR_INSUFFICIENT_AUTHEN 5
#define BLE_ATT_ERR_INSUFFICIENT_ENC 15
#define BLE_ERR_REM_USER_CONN_TERM 0x13
#define BLE_GAP_EVENT_CONNECT 0
#define BLE_GAP_EVENT_DISCONNECT 1
#define BLE_GAP_EVENT_ENC_CHANGE 10
#define BLE_GAP_EVENT_PASSKEY_ACTION 11
#define BLE_GAP_EVENT_REPEAT_PAIRING 17
#define BLE_GAP_REPEAT_PAIRING_IGNORE 2
#define BLE_GATT_CHR_F_READ 2
#define BLE_GATT_CHR_F_WRITE 8
typedef struct {uint8_t type,val[6];} ble_addr_t;
typedef struct {uint8_t type;char text[37];} ble_uuid_t;
typedef union {ble_uuid_t u;} ble_uuid_any_t;
struct ble_gap_event {int type;union {struct {int status;uint16_t conn_handle;}connect,enc_change;struct {struct {uint16_t conn_handle;}conn;}disconnect;};};
struct ble_gap_conn_params;
struct ble_gatt_error {uint16_t status,att_handle;};
struct ble_gatt_svc {uint16_t start_handle,end_handle;ble_uuid_any_t uuid;};
struct ble_gatt_chr {uint16_t def_handle,val_handle;uint8_t properties;ble_uuid_any_t uuid;};
struct os_mbuf;
struct ble_gatt_attr {uint16_t handle,offset;struct os_mbuf *om;};
typedef int ble_gatt_disc_svc_fn(uint16_t,const struct ble_gatt_error *,const struct ble_gatt_svc *,void *);
typedef int ble_gatt_chr_fn(uint16_t,const struct ble_gatt_error *,const struct ble_gatt_chr *,void *);
typedef int ble_gatt_attr_fn(uint16_t,const struct ble_gatt_error *,struct ble_gatt_attr *,void *);
int ble_hs_id_infer_auto(int privacy,uint8_t *type);
int ble_gap_connect(uint8_t own,const ble_addr_t *peer,int32_t duration,const struct ble_gap_conn_params *params,int (*cb)(struct ble_gap_event *,void *),void *arg);
int ble_gap_conn_cancel(void);
int ble_gap_terminate(uint16_t connection,uint8_t reason);
uint16_t ble_att_mtu(uint16_t connection);
int ble_uuid_from_str(const char *text,ble_uuid_any_t *uuid);
int ble_uuid_cmp(const ble_uuid_t *a,const ble_uuid_t *b);
int ble_gattc_disc_svc_by_uuid(uint16_t,const ble_uuid_t *,ble_gatt_disc_svc_fn *,void *);
int ble_gattc_disc_all_chrs(uint16_t,uint16_t,uint16_t,ble_gatt_chr_fn *,void *);
int ble_gattc_write_flat(uint16_t,uint16_t,const void *,uint16_t,ble_gatt_attr_fn *,void *);
int ble_gattc_read(uint16_t,uint16_t,ble_gatt_attr_fn *,void *);
