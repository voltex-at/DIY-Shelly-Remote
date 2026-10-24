#ifndef DSR_MAINTENANCE_TYPES_H
#define DSR_MAINTENANCE_TYPES_H
#include "dsr_core.h"
typedef uint64_t dsr_maint_generation_t;
typedef enum {DSR_MAINT_EXPORT=1,DSR_MAINT_IMPORT_UPLOAD,DSR_MAINT_IMPORT_COMMIT,DSR_MAINT_RESET} dsr_maint_op_t;
typedef enum {DSR_MAINT_IDLE,DSR_MAINT_WAITING,DSR_MAINT_CAPTURING,DSR_MAINT_VALIDATING,
 DSR_MAINT_STAGING,DSR_MAINT_CONFIRM,DSR_MAINT_COMMITTED,DSR_MAINT_APPLYING,DSR_MAINT_COMPLETE,DSR_MAINT_FAULT,DSR_MAINT_GRANTED} dsr_maint_phase_t;
/* Public request/status contain no data, physical identities or credentials. */
typedef struct {dsr_maint_op_t op;uint8_t nonce[16],session[32],candidate_sha256[32];} dsr_maint_request_t;
typedef struct {
 uint32_t revision;dsr_result_t result;uint8_t op,phase,progress,photos,connections,rules;
 uint8_t request_id[16],candidate_id[16];bool can_confirm,usb_required;
} dsr_maint_status_t;
#endif
