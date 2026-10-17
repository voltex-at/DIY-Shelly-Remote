#ifndef DSR_OTA_TRUST_H
#define DSR_OTA_TRUST_H
#include <stdint.h>
/* Private compile-time trust provider. Not a key setter, NVS source, URL or
 * manifest field. Host fixtures link their own provider, never into IDF. */
const uint8_t *dsr_ota_trusted_key(const char *id);
#endif
