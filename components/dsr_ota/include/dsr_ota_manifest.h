#ifndef DSR_OTA_MANIFEST_H
#define DSR_OTA_MANIFEST_H
#include <stddef.h>
#include <stdint.h>

#define DSR_OTA_MANIFEST_MAX 2048
#define DSR_OTA_HARDWARE_ID "lcdwiki-es3c35p-n16r8"
#define DSR_OTA_PARTITION_LAYOUT "es3c35p-16m-ota6m-dsrdata3e0000-v1"
typedef enum {DSR_OTA_STABLE,DSR_OTA_DEV} dsr_ota_channel_t;
typedef enum {
 DSR_MANIFEST_OK,DSR_MANIFEST_INVALID,DSR_MANIFEST_POLICY,
 DSR_MANIFEST_UNTRUSTED_KEY,DSR_MANIFEST_BAD_SIGNATURE
} dsr_manifest_result_t;
typedef struct {
 /* Trusted installed release base X.Y.Z, not an untrusted manifest value or
  * project build suffix. The future updater must supply trusted local state. */
 const char *current_version;
 dsr_ota_channel_t selected_channel;
 size_t inactive_slot_size;
} dsr_ota_policy_t;
typedef struct {
 char version[32],url[257];
 uint8_t sha256[32];
 uint32_t size;
 dsr_ota_channel_t channel;
} dsr_ota_verified_t;
/* Pure verification: bounded JSON, metadata policy and genuine Ed25519.
 * No network, Flash, UI or reboot. Only OK publishes output; all failures
 * zero it. Public API never accepts caller-supplied keys. Production trust
 * store is deliberately empty until the owner's publisher key is reviewed. */
dsr_manifest_result_t dsr_ota_manifest_verify(const void *json,size_t length,
 const dsr_ota_policy_t *policy,dsr_ota_verified_t *out);
#endif
