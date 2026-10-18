#include "dsr_blu.h"
/* Deterministic LCD fixture only; the genuine GATT driver has its own suite. */
static dsr_blu_snapshot_t snapshot;
dsr_result_t dsr_blu_start(const dsr_ble_device_t *c){if(!dsr_blu_supported(c))return DSR_UNSUPPORTED;snapshot=(dsr_blu_snapshot_t){.generation=1,.phase=DSR_BLU_DONE,.result=DSR_OK,.bonded=true,.model=c->model_id};return DSR_OK;}
dsr_result_t dsr_blu_write(uint32_t gen,unsigned field,uint32_t value){return gen==snapshot.generation&&dsr_blu_value_valid(field,snapshot.model,value)?DSR_OK:DSR_INVALID;}
dsr_result_t dsr_blu_forget(uint32_t gen){if(gen!=snapshot.generation)return DSR_INVALID;snapshot.bonded=false;return DSR_OK;}
dsr_result_t dsr_blu_pin(uint32_t gen,const char *pin){(void)gen;(void)pin;return DSR_AUTH_REQUIRED;}
void dsr_blu_cancel(uint32_t gen){(void)gen;}
void dsr_blu_snapshot(dsr_blu_snapshot_t *out){*out=snapshot;}
bool dsr_blu_active(void){return snapshot.active;}
void dsr_blu_close(void){snapshot=(dsr_blu_snapshot_t){0};}
bool dsr_blu_gateway_key(uint32_t gen,char address[18],char key[33]){(void)gen;(void)address;(void)key;return false;}
