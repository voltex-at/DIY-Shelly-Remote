#include "dsr_ota_release_chain.h"
#include "dsr_ota_fetch_private.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
 dsr_ota_release_candidate_t candidate;
 dsr_ota_verified_t verified;
} release_chain_t;
_Static_assert(sizeof(release_chain_t)<=1024,"bounded release-chain workspace");
dsr_ota_fetch_result_t dsr_ota_fetch_release_app(
 const dsr_ota_release_policy_t *release_policy,const dsr_ota_policy_t *manifest_policy,
 const dsr_ota_fetch_control_t *control,dsr_ota_verified_t *out,
 dsr_ota_release_result_t *release_why,dsr_manifest_result_t *why,
 dsr_image_result_t *image_why)
{
 if(release_why)*release_why=DSR_RELEASE_INVALID;
 if(why)*why=DSR_MANIFEST_INVALID;
 if(image_why)*image_why=DSR_IMAGE_STATE;
 if(!out)return DSR_FETCH_ARGUMENT;
 memset(out,0,sizeof(*out));
 dsr_ota_release_result_t checked=dsr_ota_release_policy_check(release_policy);
 if(checked!=DSR_RELEASE_OK){if(release_why)*release_why=checked;return DSR_FETCH_ARGUMENT;}
 if(!dsr_ota_fetch_policy_valid(manifest_policy)||manifest_policy->selected_channel!=DSR_OTA_STABLE)return DSR_FETCH_ARGUMENT;
 release_chain_t *s=calloc(1,sizeof(*s));if(!s)return DSR_FETCH_NOMEM;
 dsr_ota_fetch_result_t result=dsr_ota_fetch_release(release_policy,control,&s->candidate,release_why);
 if(result==DSR_FETCH_OK)result=dsr_ota_fetch_app_sized(s->candidate.manifest_url,
  manifest_policy,control,&s->verified,why,image_why,s->candidate.manifest_size);
 if(result==DSR_FETCH_OK&&control&&control->cancelled&&control->cancelled(control->context))result=DSR_FETCH_CANCELLED;
 if(result==DSR_FETCH_OK)*out=s->verified;
 free(s);return result;
}
