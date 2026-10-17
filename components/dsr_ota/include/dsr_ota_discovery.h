#ifndef DSR_OTA_DISCOVERY_H
#define DSR_OTA_DISCOVERY_H
#include "dsr_ota_release_fetch.h"
#define DSR_OTA_NATIVE_MANIFEST_ASSET "DIY-Shelly-Remote-es3c35p-manifest.json"
#define DSR_OTA_NATIVE_NOTES_MAX 320u
/* Untrusted catalogue metadata. Only the genuine manifest verifier can make
 * its version/image authentic; this record never authorizes installation. */
typedef struct {
 dsr_ota_release_candidate_t candidate;
 char notes[DSR_OTA_NATIVE_NOTES_MAX+1];
} dsr_ota_discovery_t;
/* STABLE uses one /latest release object. DEV uses the first eligible
 * prerelease of a bounded array page (<=5 entries); entries reports the page
 * size (0 = end of catalogue) so the fetcher can walk at most five one-release
 * pages. No inferred semver, fallback across channels or arbitrary URLs. */
dsr_ota_release_result_t dsr_ota_discovery_parse(const void *json,size_t length,
 dsr_ota_channel_t channel,dsr_ota_discovery_t *out,unsigned *entries);
dsr_ota_fetch_result_t dsr_ota_discovery_fetch(dsr_ota_channel_t channel,
 const dsr_ota_fetch_control_t *control,dsr_ota_discovery_t *out,
 dsr_ota_release_result_t *release_result,int *http_status);
#endif
