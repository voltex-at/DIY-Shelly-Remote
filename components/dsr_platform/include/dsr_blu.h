#ifndef DSR_BLU_H
#define DSR_BLU_H
#include "dsr_ble.h"
#include "dsr_core.h"
#define DSR_BLU_SETTINGS 16u
typedef enum {DSR_BLU_IDLE,DSR_BLU_CONNECTING,DSR_BLU_PAIRING,DSR_BLU_PIN,DSR_BLU_READING,DSR_BLU_WRITING,DSR_BLU_CLOSING,DSR_BLU_DONE,DSR_BLU_FAILED,DSR_BLU_CANCELLED,DSR_BLU_QUARANTINED} dsr_blu_phase_t;
typedef struct {const char *label,*uuid;uint16_t model;uint8_t bytes;uint32_t minimum,maximum;bool boolean;} dsr_blu_setting_t;
typedef struct {
 uint32_t serial,generation;
 dsr_blu_phase_t phase;dsr_result_t result;
 bool active,bonded,key_available,write_possible,uncertain;
 uint16_t model;
 uint16_t available;
 uint32_t values[DSR_BLU_SETTINGS];
} dsr_blu_snapshot_t;
const dsr_blu_setting_t *dsr_blu_setting(unsigned index);
bool dsr_blu_supported(const dsr_ble_device_t *candidate);
bool dsr_blu_value_valid(unsigned index,uint16_t model,uint32_t value);
void dsr_blu_host_setup(void);
dsr_result_t dsr_blu_start(const dsr_ble_device_t *candidate);
dsr_result_t dsr_blu_write(uint32_t generation,unsigned field,uint32_t value);
dsr_result_t dsr_blu_forget(uint32_t generation);
dsr_result_t dsr_blu_pin(uint32_t generation,const char *pin);
void dsr_blu_cancel(uint32_t generation);
void dsr_blu_snapshot(dsr_blu_snapshot_t *out);
bool dsr_blu_active(void);
void dsr_blu_close(void);
/* Trusted LCD/worker only; never serialized in configuration or Web API. */
bool dsr_blu_gateway_key(uint32_t generation,char address[18],char key[33]);
#endif
