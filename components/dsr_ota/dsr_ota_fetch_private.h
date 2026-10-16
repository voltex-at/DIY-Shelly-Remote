#ifndef DSR_OTA_FETCH_PRIVATE_H
#define DSR_OTA_FETCH_PRIVATE_H
#include "dsr_ota_fetch.h"
/* Internal transport only. Caller supplies 16385-byte heap storage. No parser,
 * candidate trust, app requests or installation. No public arbitrary URL API. */
dsr_ota_fetch_result_t dsr_ota_fetch_metadata_body(const char *url,
 const dsr_ota_fetch_control_t *control,unsigned char *body,size_t *length);
#endif
