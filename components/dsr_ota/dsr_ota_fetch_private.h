#ifndef DSR_OTA_FETCH_PRIVATE_H
#define DSR_OTA_FETCH_PRIVATE_H
#include "dsr_ota_fetch.h"
/* Internal transport only. Caller supplies 16385-byte heap storage. No parser,
 * candidate trust, app requests or installation. No public arbitrary URL API. */
dsr_ota_fetch_result_t dsr_ota_fetch_metadata_body(const char *url,
 const dsr_ota_fetch_control_t *control,unsigned char *body,size_t *length);
bool dsr_ota_fetch_policy_valid(const dsr_ota_policy_t *policy);
/* Internal length-bound entry to the SAME genuine manifest/app chain. A size
 * of zero retains the old public standalone API semantics; bridge uses1..2048. */
dsr_ota_fetch_result_t dsr_ota_fetch_app_sized(const char *url,
 const dsr_ota_policy_t *policy,const dsr_ota_fetch_control_t *control,
 dsr_ota_verified_t *out,dsr_manifest_result_t *why,dsr_image_result_t *image_why,
 size_t expected_manifest_size);
#endif
