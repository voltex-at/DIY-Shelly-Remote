#ifndef DSR_BLE_RPC_PROTOCOL_H
#define DSR_BLE_RPC_PROTOCOL_H
#include "dsr_discovery.h"

#define DSR_BLE_RPC_TX_MAX 1024u
#define DSR_BLE_RPC_RX_MAX 8192u
#define DSR_BLE_RPC_TIMEOUT_MS 15000u
typedef enum {DSR_RPC_INFO,DSR_RPC_SYS_CONFIG,DSR_RPC_METHODS,DSR_RPC_STATUS,DSR_RPC_WIFI_STATUS,DSR_RPC_WIFI_SET,DSR_RPC_METHOD_COUNT} dsr_rpc_method_t;
typedef enum {DSR_RPC_TX_CTL,DSR_RPC_TX_DATA,DSR_RPC_RX_CTL,DSR_RPC_RX_DATA,DSR_RPC_DONE,DSR_RPC_FAILED} dsr_rpc_step_t;
typedef struct {
 uint32_t session,sequence,request_id,started_ms;
 uint16_t connection,mtu;
 dsr_rpc_step_t step;
 dsr_result_t result;
 bool pending,write_method,write_possible;
 const uint8_t *tx;
 uint8_t *rx;
 size_t tx_length,tx_used,rx_length,rx_used,pending_length;
} dsr_rpc_stream_t;
typedef struct {dsr_rpc_step_t kind;uint32_t session,sequence;uint16_t connection;const uint8_t *bytes;size_t length;uint8_t control[4];} dsr_rpc_action_t;
typedef struct {dsr_lan_device_t device;char firmware[32];bool confirmed,rgb_profile;} dsr_rpc_identity_t;

/* Private transport-independent protocol. Buffer ownership remains with its
 * manager until disconnect/drain. Response storage is RX_MAX+1 bytes, including
 * the trailing NUL. Only successful matching ACK advances TX. */
dsr_result_t dsr_rpc_stream_begin(dsr_rpc_stream_t *stream,uint32_t session,uint16_t connection,uint16_t mtu,const uint8_t *request,size_t length,uint8_t *response,bool write_method,uint32_t now);
bool dsr_rpc_stream_next(dsr_rpc_stream_t *stream,uint32_t now,dsr_rpc_action_t *action);
bool dsr_rpc_stream_complete(dsr_rpc_stream_t *stream,uint32_t session,uint32_t sequence,uint16_t connection,dsr_result_t result,const uint8_t *bytes,size_t length,uint32_t now);
void dsr_rpc_stream_cancel(dsr_rpc_stream_t *stream);
dsr_result_t dsr_rpc_request(dsr_rpc_method_t method,uint32_t id,const char *client,const dsr_settings_t *home,char **json);
/* A notification returns BUSY with notification=true; it is never an ACK.
 * Every other non-OK result is terminal. Caller owns successful result. */
dsr_result_t dsr_rpc_response(const uint8_t *json,size_t length,uint32_t id,const char *client,const char *device,cJSON **result,bool *notification);
dsr_result_t dsr_rpc_identity(const cJSON *result,dsr_rpc_identity_t *identity);
dsr_result_t dsr_rpc_setup_policy(const cJSON *system,const cJSON *methods,const cJSON *status,const dsr_rpc_identity_t *identity);
dsr_result_t dsr_rpc_station(const cJSON *result,const dsr_settings_t *home,char host[16]);
#endif
