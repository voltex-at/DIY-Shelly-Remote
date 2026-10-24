#ifndef DSR_MAINTENANCE_MODEL_H
#define DSR_MAINTENANCE_MODEL_H
#include "dsr_maintenance_types.h"
/* Private owner lease, never a web capability. Release matches reservation ID. */
typedef struct {uint64_t reservation;dsr_maint_generation_t generation;} dsr_maint_lease_t;
enum {DSR_MAINT_PREPARING=1,DSR_MAINT_COMMIT,DSR_MAINT_APPLY,DSR_MAINT_RECEIPT,DSR_MAINT_JOURNAL_FAULT};
enum {DSR_MAINT_JOURNAL_BYTES=256,DSR_MAINT_RECORD_HEADER=144,DSR_MAINT_IO_CHUNK=4096,
 DSR_MAINT_IMPORT_PARTS=31,DSR_MAINT_RESET_PARTS=1023};
typedef struct {
 uint64_t sequence,generation;uint8_t operation_id[16],device[32],candidate_sha256[32],manifest_sha256[32],key[32],nonce_prefix[8];
 uint32_t candidate_bytes,metadata_bytes;uint16_t applied_photos,applied_parts;
 uint8_t op,phase,photos,connections,rules,bonds,cancelled;
} dsr_maint_record_t;
typedef struct {size_t total,used,nvs_available,psram_free,psram_largest,internal_free,internal_largest;} dsr_maint_limits_t;
dsr_result_t dsr_maint_preflight(const dsr_maint_limits_t *,uint32_t metadata,unsigned photos,unsigned bonds);
bool dsr_maint_record_valid(const dsr_maint_record_t *);
bool dsr_maint_record_encode(const dsr_maint_record_t *,uint8_t wire[DSR_MAINT_JOURNAL_BYTES]);
dsr_result_t dsr_maint_record_decode(const uint8_t *wire,size_t size,dsr_maint_record_t *);
bool dsr_maint_record_name(const uint8_t operation[16],unsigned record,char name[32]);
#endif
