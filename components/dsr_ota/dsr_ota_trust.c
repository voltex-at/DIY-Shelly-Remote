#include "dsr_ota_trust.h"
#include <stddef.h>
const uint8_t *dsr_ota_trusted_key(const char *id)
{
 (void)id;
 /* No approved publisher anchor exists. Never insert an RFC/test key here. */
 return NULL;
}
