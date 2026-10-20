#include "dsr_blu.h"
#include <string.h>
/* Deterministic LCD fixture only; real GATT has a separate callback suite. */
static dsr_blu_snapshot_t snapshot;
static bool pin_required;
static unsigned writes;
void test_blu_phase(dsr_blu_phase_t phase,bool active){snapshot.phase=phase;snapshot.active=active;snapshot.result=active?DSR_BUSY:DSR_IO;snapshot.serial++;}
void test_blu_pin_required(bool pin){pin_required=pin;writes=0;}
unsigned test_blu_writes(void){return writes;}
dsr_result_t dsr_blu_start(const dsr_ble_device_t *c)
{if(!dsr_blu_supported(c))return DSR_UNSUPPORTED;snapshot=(dsr_blu_snapshot_t){.generation=1,.phase=pin_required?DSR_BLU_PIN:DSR_BLU_DONE,.result=pin_required?DSR_BUSY:DSR_OK,.active=pin_required,.bonded=!pin_required,.model=c->model_id,.available=1u<<2};snapshot.values[2]=60;return DSR_OK;}
dsr_result_t dsr_blu_write(uint32_t gen,unsigned field,uint32_t value)
{if(gen!=snapshot.generation||!dsr_blu_value_valid(field,snapshot.model,value))return DSR_INVALID;writes++;snapshot.values[field]=value;snapshot.phase=DSR_BLU_DONE;snapshot.result=DSR_OK;return DSR_OK;}
dsr_result_t dsr_blu_forget(uint32_t gen){if(gen!=snapshot.generation)return DSR_INVALID;snapshot.bonded=false;return DSR_OK;}
dsr_result_t dsr_blu_pin(uint32_t gen,const char *pin)
{if(gen!=snapshot.generation||strcmp(pin,"123456"))return DSR_AUTH_REQUIRED;snapshot.active=false;snapshot.bonded=true;snapshot.phase=DSR_BLU_DONE;snapshot.result=DSR_OK;return DSR_OK;}
void dsr_blu_cancel(uint32_t gen){if(gen==snapshot.generation)snapshot.active=false;}
void dsr_blu_snapshot(dsr_blu_snapshot_t *out){*out=snapshot;}
bool dsr_blu_active(void){return snapshot.active;}
void dsr_blu_close(void){snapshot=(dsr_blu_snapshot_t){0};}
bool dsr_blu_gateway_key(uint32_t gen,char address[18],char key[33]){(void)gen;(void)address;(void)key;return false;}
