#ifndef DSR_HISTORY_PRIVATE_H
#define DSR_HISTORY_PRIVATE_H
#include "dsr_history.h"
#define DSR_HISTORY_SLOTS 16u
#define DSR_HISTORY_MEMORY_MAX (192u*1024u)
#define DSR_HISTORY_FILE_MAX (256u*1024u)
#define DSR_HISTORY_SNAPSHOT_MAX (32u+DSR_HISTORY_SLOTS*80u+DSR_HISTORY_SLOTS*DSR_HISTORY_POINTS*40u)
typedef struct {
 dsr_history_key_t key;
 double sum[3],last_energy;
 uint64_t last_mono;
 uint32_t current_utc,last_request,last_utc,version;
 uint16_t counts[3];
 bool used,active,has_energy,has_request,sealed;
} dsr_history_slot_t;
typedef struct {
 dsr_history_slot_t slots[DSR_HISTORY_SLOTS];
 dsr_history_point_t points[DSR_HISTORY_SLOTS][DSR_HISTORY_POINTS];
 uint32_t dirty[DSR_HISTORY_SLOTS][DSR_HISTORY_POINTS];
 uint32_t without_time;
} dsr_history_model_t;
_Static_assert(sizeof(dsr_history_point_t)==32,"history point format footprint");
_Static_assert(sizeof(dsr_history_model_t)<=DSR_HISTORY_MEMORY_MAX,"history PSRAM model budget");
_Static_assert(sizeof(dsr_history_series_t)<=16384,"bounded history query");
_Static_assert(DSR_HISTORY_SNAPSHOT_MAX<=DSR_HISTORY_MEMORY_MAX,"bounded PSRAM checkpoint");
bool dsr_history_key_equal(const dsr_history_key_t *a,const dsr_history_key_t *b);
int dsr_history_model_binding(dsr_history_model_t *m,const dsr_history_key_t *key,bool active);
void dsr_history_model_record(dsr_history_model_t *m,const dsr_history_key_t *key,
 const dsr_device_t *device,const dsr_telemetry_t *telemetry,uint32_t request,uint32_t utc,uint64_t mono);
void dsr_history_model_query(const dsr_history_model_t *m,unsigned slot,unsigned hours,uint32_t now,dsr_history_series_t *out);
void dsr_history_model_seal(dsr_history_model_t *m,uint32_t utc);
uint32_t dsr_history_crc(const void *data,size_t length);
/* Explicit little-endian field codec: no native struct/padding ABI on Flash. */
size_t dsr_history_encode_key(unsigned char *out,unsigned slot,const dsr_history_key_t *key);
size_t dsr_history_encode_point(unsigned char *out,unsigned slot,const dsr_history_point_t *point);
bool dsr_history_decode(dsr_history_model_t *m,const unsigned char *data,size_t length);
size_t dsr_history_checkpoint(const dsr_history_model_t *m,unsigned char *out,size_t capacity,uint32_t generation);
/* Store owns its two history files only; failures never erase configuration,
 * photos, NVS or partitions. All methods below execute on the sole worker. */
typedef struct {
 uint32_t generation;size_t length;unsigned slot;bool loaded,future,repair;
 dsr_history_key_t keys[DSR_HISTORY_SLOTS];bool known[DSR_HISTORY_SLOTS];
} dsr_history_store_t;
bool dsr_history_store_load(dsr_history_store_t *store,dsr_history_model_t *model,const char *dir);
bool dsr_history_store_append(dsr_history_store_t *store,const void *data,size_t length,const char *dir);
bool dsr_history_store_checkpoint(dsr_history_store_t *store,const void *data,size_t length,const char *dir);
#endif
