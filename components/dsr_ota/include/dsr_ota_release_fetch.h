#ifndef DSR_OTA_RELEASE_FETCH_H
#define DSR_OTA_RELEASE_FETCH_H
#include "dsr_ota_fetch.h"
#include "dsr_ota_release.h"
/* Blocking backend, not for UI task. One explicit tag, exact own-repo HTTPS
 * API URL, no redirects/auth/retries/discovery. Returns only an UNVERIFIED
 * candidate; never invokes manifest/app download or installs anything.
 * Policy/control pointers and their data stay immutable until return.
 * Only whole DSR_FETCH_OK publishes out. Detail result is not authorization.
 * 16 KiB response storage is heap-owned, separate from the <=8192 transport
 * context. SDK/TLS/header/cJSON allocations and transitive stack are additional.
 * Existing UTC floor, CA/date/hostname and cooperative 30s/SDK5s apply; this
 * does not establish trusted time or a hard total deadline. Every error zeros
 * out, including late cancellation, parser failure and cleanup failure. */
dsr_ota_fetch_result_t dsr_ota_fetch_release(const dsr_ota_release_policy_t *policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_release_candidate_t *out,
 dsr_ota_release_result_t *release_result);
#endif
