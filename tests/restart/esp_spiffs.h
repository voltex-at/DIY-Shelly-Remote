#ifndef DSR_TEST_PHOTO_SPIFFS_H
#define DSR_TEST_PHOTO_SPIFFS_H
#include <stddef.h>
#include "esp_err.h"
esp_err_t esp_spiffs_info(const char *,size_t *,size_t *);
#endif
