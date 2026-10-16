#ifndef DSR_OTA_FETCH_PRIVATE_H
#define DSR_OTA_FETCH_PRIVATE_H
#include "dsr_ota_fetch.h"
/* Internal transport only. Caller supplies 16385-byte heap storage. No parser,
 * candidate trust, app requests or installation. No public arbitrary URL API. */
dsr_ota_fetch_result_t dsr_ota_fetch_metadata_body(const char *url,
 const dsr_ota_fetch_control_t *control,unsigned char *body,size_t *length);
/* Native discovery only: exact own-repo latest (STABLE) or bounded list
 * ?per_page=5 (DEV). Old explicit-tag metadata semantics remain unchanged.
 * http_status is diagnostic only; it never authenticates release metadata. */
dsr_ota_fetch_result_t dsr_ota_fetch_discovery_body(dsr_ota_channel_t channel,
 const dsr_ota_fetch_control_t *control,unsigned char *body,size_t *length,int *http_status);
bool dsr_ota_fetch_policy_valid(const dsr_ota_policy_t *policy);
/* Internal length-bound entry to the SAME genuine manifest/app chain. A size
 * of zero retains the old public standalone API semantics; bridge uses1..2048. */
dsr_ota_fetch_result_t dsr_ota_fetch_app_sized(const char *url,
 const dsr_ota_policy_t *policy,const dsr_ota_fetch_control_t *control,
 dsr_ota_verified_t *out,dsr_manifest_result_t *why,dsr_image_result_t *image_why,
 size_t expected_manifest_size);
/* Private installer bridge. The sink cannot bypass fresh manifest signature
 * verification or exact consent matching. Invoked only after each real image
 * update succeeds; header_ready is the image helper's result, not caller data.
 * Sink failure is IO; installer retains SDK-specific diagnostics and cleanup.
 * No final commit callback: transport cleanup/hash/image/late cancellation must
 * all succeed before the caller may end/select a staged SDK image. */
typedef struct {
 bool (*write)(void *context,const void *data,size_t length,bool header_ready);
 void *context;
} dsr_ota_stream_sink_t;
dsr_ota_fetch_result_t dsr_ota_fetch_stream_app(const char *url,
 const dsr_ota_policy_t *policy,const dsr_ota_verified_t *consent,
 const dsr_ota_fetch_control_t *control,const dsr_ota_stream_sink_t *sink,
 dsr_ota_verified_t *out,dsr_manifest_result_t *why,
 dsr_image_result_t *image_why,bool *consent_changed);
#endif
