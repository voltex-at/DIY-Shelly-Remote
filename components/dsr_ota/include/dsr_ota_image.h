#ifndef DSR_OTA_IMAGE_H
#define DSR_OTA_IMAGE_H
#include <stdbool.h>
#include "dsr_ota_manifest.h"

#define DSR_OTA_IMAGE_PREFIX_SIZE 512u
#define DSR_OTA_IMAGE_MAX_SIZE 0x600000u
#define DSR_OTA_IMAGE_BOARD_MARKER "DSR-OTA:lcdwiki-es3c35p-n16r8:config-api-1"
typedef struct dsr_ota_image dsr_ota_image_t;
typedef enum {
 DSR_IMAGE_OK, DSR_IMAGE_ARGUMENT, DSR_IMAGE_NOMEM, DSR_IMAGE_STATE,
 DSR_IMAGE_EXTRA, DSR_IMAGE_HEADER, DSR_IMAGE_TRUNCATED,
 DSR_IMAGE_MARKER, DSR_IMAGE_HASH, DSR_IMAGE_CRYPTO
} dsr_image_result_t;
/* Pass metadata ONLY after dsr_ota_manifest_verify returns OK. This helper
 * cannot authenticate a caller-created struct. Copies size/hash, not pointers.
 * No network, Flash, partition selection, OTA calls or reboot. Not thread-safe.
 * create always clears *out; destroy(NULL) is safe. No stack-sized image probe. */
dsr_image_result_t dsr_ota_image_create(const dsr_ota_verified_t *verified,
 dsr_ota_image_t **out);
/* Explicit cancellation/reuse: discards the previous hash, prefix and marker.
 * A reset failure leaves the context failed, never retaining prior success. */
dsr_image_result_t dsr_ota_image_reset(dsr_ota_image_t *image,
 const dsr_ota_verified_t *verified);
/* OK means chunk consumed, NOT a verified image. Empty chunks are no-ops
 * while running. Every error is sticky until reset; extra bytes are rejected
 * before hashing/copying. Caller must not modify data during this call. */
dsr_image_result_t dsr_ota_image_update(dsr_ota_image_t *image,
 const void *data,size_t length);
/* The ONLY final acceptance result. Requires exact length, early header,
 * complete board marker INCLUDING NUL, and real SHA256. Terminal/idempotent:
 * repeating finish returns the same result; any update after success fails. */
dsr_image_result_t dsr_ota_image_finish(dsr_ota_image_t *image);
bool dsr_ota_image_header_ready(const dsr_ota_image_t *image);
size_t dsr_ota_image_received(const dsr_ota_image_t *image);
size_t dsr_ota_image_context_size(void);
void dsr_ota_image_destroy(dsr_ota_image_t *image);
#endif
