#ifndef DSR_BACKUP_FORMAT_H
#define DSR_BACKUP_FORMAT_H
#include "dsr_core.h"
#include "cJSON.h"
#define DSR_BACKUP_MAX_BYTES 2097152u
#define DSR_BACKUP_METADATA_BYTES 65536u
#define DSR_BACKUP_MAX_PHOTOS 16u
#define DSR_BACKUP_PHOTO_BYTES 115200u
#define DSR_BACKUP_HARDWARE "lcdwiki-es3c35p-n16r8"
typedef struct dsr_backup_image dsr_backup_image_t;
typedef struct {char id[33];uint8_t sha256[32];const uint8_t *pixels;} dsr_backup_photo_t;
/* Always takes ownership, including failure. NULL output on every error. */
dsr_result_t dsr_backup_image_open(uint8_t *owned,size_t size,const uint8_t device_id[32],dsr_backup_image_t **out);
/* Structural manifest/config/record validation. Source adapters additionally
 * validate all private fields and cross-store identity/enrollment references. */
dsr_result_t dsr_backup_manifest_validate(const cJSON *manifest,const uint8_t device_id[32]);
void dsr_backup_image_free(dsr_backup_image_t *image);
const uint8_t *dsr_backup_image_bytes(const dsr_backup_image_t *image,size_t *size);
const cJSON *dsr_backup_image_manifest(const dsr_backup_image_t *image);
const dsr_backup_photo_t *dsr_backup_image_photo(const dsr_backup_image_t *image,unsigned index);
void dsr_backup_wipe(void *data,size_t size);
void dsr_backup_json_discard(cJSON *owned);
#endif
