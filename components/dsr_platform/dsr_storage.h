#ifndef DSR_STORAGE_H
#define DSR_STORAGE_H
#include "esp_err.h"
/* Never formats a nonempty or unreadable partition, or on allocation errors. */
esp_err_t dsr_storage_mount(void);
#endif
