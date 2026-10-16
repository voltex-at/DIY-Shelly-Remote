#pragma once
#include "esp_err.h"
#include <stddef.h>
esp_err_t esp_spiffs_info(const char *,size_t *,size_t *);
