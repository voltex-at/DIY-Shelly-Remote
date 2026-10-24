#ifndef DSR_MAINTENANCE_CRYPTO_H
#define DSR_MAINTENANCE_CRYPTO_H
#include "dsr_maintenance_model.h"
/* One immutable record. Fresh operation/key/nonce prefix per prepare is the
 * staging owner's responsibility. Index0 metadata,1..16 photos. No file IO. */
dsr_result_t dsr_maint_crypto_seal(const dsr_maint_record_t *,unsigned index,const uint8_t *plain,size_t size,uint8_t *sealed,size_t capacity);
dsr_result_t dsr_maint_crypto_open(const dsr_maint_record_t *,unsigned index,const uint8_t *sealed,size_t size,uint8_t *plain,size_t capacity,size_t *written);
#endif
