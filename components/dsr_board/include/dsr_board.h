#ifndef DSR_BOARD_H
#define DSR_BOARD_H
#include "esp_err.h"
const char *dsr_board_hardware_id(void);
esp_err_t dsr_board_init(void);
#endif
