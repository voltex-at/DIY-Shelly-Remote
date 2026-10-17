#ifndef DSR_OTA_RELEASE_CHAIN_H
#define DSR_OTA_RELEASE_CHAIN_H
#include "dsr_ota_release_fetch.h"
/* Blocking validation/DISCARD chain, no UI/worker/Flash/install/reboot. Caller
 * supplies explicit release tag/asset/STABLE and trusted LOCAL version/slot
 * policy (also STABLE), never a candidate/verified struct or inferred tag version.
 * Genuine metadata fetch/parser precede genuine signed manifest/app validation.
 * candidate.manifest_size binds actual MANIFEST bytes, NOT the signed BIN size.
 * Metadata storage is freed before later phases. Each phase retains its own
 * cooperative 30s/SDK5s limits, not a hard total30s deadline. No bytes are kept.
 * Every failure zeros out. Only whole FETCH_OK publishes signed metadata;
 * detail results alone never authorize anything. No approved Release or new
 * publisher key/time/dev/naming policy is established. Pointers stay immutable
 * and out exclusively owned until return; never call from the UI task. */
dsr_ota_fetch_result_t dsr_ota_fetch_release_app(
 const dsr_ota_release_policy_t *release_policy,const dsr_ota_policy_t *manifest_policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,
 dsr_ota_release_result_t *release_result,dsr_manifest_result_t *manifest_result,
 dsr_image_result_t *image_result);
#endif
