#include "dsr_ota_trust.h"
#include <stddef.h>
#include <string.h>
/* Owner-approved production publisher anchor (2026-10-17, issue #24).
 * Generated with tools/ota_keygen.py; the private seed is held only by the
 * owner and the GitHub Actions secret DSR_OTA_SIGNING_KEY. Rotation = add a
 * new id here in a release signed by the CURRENT key, then retire the old.
 * Never insert an RFC/test key here. */
#define DSR_OTA_KEY_ID "voltex-ota-2026-1"
static const uint8_t production_key[32]={0x23,0xad,0xf9,0x9d,0x68,0xe6,0xce,0x23,0x15,0xa7,0xb0,0xb2,0xb5,0x3c,0x58,0x39,0xd9,0xdd,0x15,0xe3,0x9e,0xe7,0x48,0xa2,0x4e,0xc6,0x88,0x41,0x86,0x1a,0x47,0xfe};
const uint8_t *dsr_ota_trusted_key(const char *id)
{
 return id&&!strcmp(id,DSR_OTA_KEY_ID)?production_key:NULL;
}
