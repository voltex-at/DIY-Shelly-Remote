#ifndef DSR_OTA_RELEASE_H
#define DSR_OTA_RELEASE_H
#include "dsr_ota_manifest.h"

#define DSR_OTA_RELEASE_MAX 16384u
#define DSR_OTA_RELEASE_MAX_DEPTH 8u
#define DSR_OTA_RELEASE_MAX_NODES 512u
#define DSR_OTA_RELEASE_MAX_ASSETS 32u
typedef enum {
 DSR_RELEASE_OK,DSR_RELEASE_ARGUMENT,DSR_RELEASE_INVALID,DSR_RELEASE_POLICY,
 DSR_RELEASE_NOT_FOUND,DSR_RELEASE_AMBIGUOUS
} dsr_ota_release_result_t;
typedef struct {
 /* Explicit routing policy, NOT a tag-to-semver convention or publisher key.
  * Safe ASCII segments: tag1..64, asset1..96; neither '.' nor containing '..'.
  * This stage deliberately accepts only STABLE, never silently maps DEV. */
 const char *expected_tag;
 const char *manifest_asset;
 dsr_ota_channel_t selected_channel;
} dsr_ota_release_policy_t;
typedef struct {
 char tag[65],manifest_url[257];
 uint32_t manifest_size;
} dsr_ota_release_candidate_t;
/* PURE single GitHub Release-object parser, NO I/O/UI/Flash or trust changes.
 * ONLY an UNVERIFIED CANDIDATE: metadata/author/digest/immutable are not publisher
 * authentication. A future caller must use the genuine signed manifest chain;
 * this output cannot substitute dsr_ota_verified_t or select/install a release.
 * No latest/list selection, automatic invocation, version inference, dev policy,
 * notes/HTML output or production asset/tag naming convention is introduced.
 * Strict bounded profile: valid UTF-8/JSON strings (including surrogate pairs),
 * no decoded NUL anywhere, all numbers canonical unsigned integers<=2^53-1.
 * Alloc-free grammar/depth/node preflight; genuine cJSON decode, duplicate keys
 * rejected in EVERY object after decoding. Unknown fields remain untrusted and
 * are ignored only after this validation. Input need not be NUL terminated.
 * Args and pointed data remain valid/immutable; out is exclusively owned until
 * return. Every failure zeros out; NULL/empty/oversize input is ARGUMENT.
 * OOM is INVALID because cJSON does not distinguish it from parse failure.
 * NOT_FOUND means no named asset in THIS object, never 'no new firmware'.
 * Recursive depth bounded8; target own frames/ELF audited separately. Node/input
 * caps are NOT a measured total heap or transitive cJSON/recursive stack peak. */
/* Shared pre-I/O policy validation; does not inspect metadata or confer trust. */
dsr_ota_release_result_t dsr_ota_release_policy_check(const dsr_ota_release_policy_t *policy);
dsr_ota_release_result_t dsr_ota_release_candidate_parse(const void *json,size_t length,
 const dsr_ota_release_policy_t *policy,dsr_ota_release_candidate_t *out);
#endif
