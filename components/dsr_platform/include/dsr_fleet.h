#ifndef DSR_FLEET_H
#define DSR_FLEET_H
#include "dsr_platform.h"
/* Single UI-task owner. Fixed storage, one explicit user intent per endpoint.
 * The transport remains serial; another endpoint can queue while I/O runs. */
typedef struct {
 dsr_connection_t connection;
 dsr_result_t result;
 dsr_device_t intent_identity;
 uint32_t sample_ms,next_poll,intent_order,request_id,command_id,next_maintenance;
 uint8_t failures;
 bool sample_valid,blocked,command_error,queued,in_flight,layout_enabled,urgent_stop;
 uint8_t mailbox_pending; /* LCD accepted intent before shared-mailbox take: move1/STOP2. */
 union {bool on;dsr_cover_intent_t cover;dsr_light_intent_t light;dsr_rgb_intent_t rgb;dsr_trv_intent_t trv;};
 dsr_job_kind_t operation;
} dsr_fleet_slot_t;
typedef struct {
 dsr_fleet_slot_t slots[DSR_MAX_DEVICES];
 size_t count;
 unsigned cursor;
 uint32_t order;
} dsr_fleet_t;
_Static_assert(sizeof(dsr_fleet_t)<=8192,"Existing fixed fleet budget");
bool dsr_endpoint_equal(const dsr_device_t *a,const dsr_device_t *b);
/* How long a successful sample counts as current. BLU is polled every 15 s
 * through ~7 gateway requests, so its window covers one poll plus one slow or
 * failed attempt; a 15 s window equal to the poll period made values blink. */
#define DSR_FLEET_FRESH_MS 15000u
#define DSR_FLEET_BLU_FRESH_MS 45000u
static inline uint32_t dsr_fleet_fresh_ms(const dsr_device_t *d){return d&&d->endpoint_kind==DSR_ENDPOINT_BLU?DSR_FLEET_BLU_FRESH_MS:DSR_FLEET_FRESH_MS;}
int dsr_fleet_find(const dsr_fleet_t *fleet,const dsr_device_t *device);
dsr_result_t dsr_fleet_upsert(dsr_fleet_t *fleet,const dsr_connection_t *connection,uint32_t now);
dsr_result_t dsr_fleet_request(dsr_fleet_t *fleet,unsigned index,bool on,bool wifi,uint32_t now);
dsr_result_t dsr_fleet_cover_request(dsr_fleet_t *fleet,unsigned index,dsr_cover_intent_t intent,bool wifi,uint32_t now);
/* Explicit read-only GET may be queued without a current sample. */
dsr_result_t dsr_fleet_ht_refresh(dsr_fleet_t *fleet,unsigned index,bool wifi,uint32_t now);
/* BLU TRV target: only for a fresh sample that proved the BluTrv component. */
dsr_result_t dsr_fleet_trv_request(dsr_fleet_t *fleet,unsigned index,dsr_trv_intent_t intent,bool wifi,uint32_t now);
dsr_result_t dsr_fleet_rgb_request(dsr_fleet_t *fleet,unsigned index,dsr_rgb_intent_t intent,bool wifi,uint32_t now);
dsr_result_t dsr_fleet_light_request(dsr_fleet_t *fleet,unsigned index,dsr_light_intent_t intent,bool wifi,uint32_t now);
void dsr_fleet_cancel_queued(dsr_fleet_t *fleet,unsigned index);
bool dsr_fleet_next(dsr_fleet_t *fleet,uint32_t now,bool allow_poll,dsr_job_t *job,unsigned *index);
void dsr_fleet_started(dsr_fleet_t *fleet,unsigned index,const dsr_job_t *job);
bool dsr_fleet_complete(dsr_fleet_t *fleet,unsigned index,const dsr_event_t *event,uint32_t now);
void dsr_fleet_wifi(dsr_fleet_t *fleet,bool online,uint32_t now);
void dsr_fleet_refresh(dsr_fleet_t *fleet,uint32_t now);
#endif
