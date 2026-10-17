#include "dsr_ota_stage.h"
#include "dsr_activity.h"
#include <stdatomic.h>
#include <stddef.h>
struct dsr_ota_stage {unsigned char opaque;};
static struct dsr_ota_stage owner;
static atomic_int state;
dsr_ota_stage_t *dsr_ota_stage_acquire(void)
{
 int expected=DSR_OTA_STAGE_IDLE;
 if(!atomic_compare_exchange_strong(&state,&expected,DSR_OTA_STAGE_HELD))return NULL;
 if(!dsr_activity_begin()){atomic_store(&state,DSR_OTA_STAGE_IDLE);return NULL;}
 return &owner;
}
static void finish(dsr_ota_stage_t *stage,dsr_ota_stage_state_t next)
{
 if(stage!=&owner)return;
 int expected=DSR_OTA_STAGE_HELD;
 if(atomic_compare_exchange_strong(&state,&expected,next))dsr_activity_end();
}
void dsr_ota_stage_release(dsr_ota_stage_t *stage){finish(stage,DSR_OTA_STAGE_IDLE);}
void dsr_ota_stage_commit(dsr_ota_stage_t *stage){finish(stage,DSR_OTA_STAGE_STAGED);}
void dsr_ota_stage_uncertain(dsr_ota_stage_t *stage){finish(stage,DSR_OTA_STAGE_UNCERTAIN);}
dsr_ota_stage_state_t dsr_ota_stage_status(void){return (dsr_ota_stage_state_t)atomic_load(&state);}
