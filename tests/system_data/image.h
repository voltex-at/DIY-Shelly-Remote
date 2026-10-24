#ifndef DSR_SYSTEM_DATA_IMAGE_FIXTURE_H
#define DSR_SYSTEM_DATA_IMAGE_FIXTURE_H
#include "dsr_backup_format.h"
/* Synthetic, public checked-in metadata only. Never reads a device backup. */
dsr_backup_image_t *test_backup_image(const char *fixture,unsigned photos,unsigned bonds);
#endif
