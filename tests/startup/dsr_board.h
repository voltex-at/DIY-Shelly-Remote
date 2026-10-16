#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
esp_err_t dsr_board_init(void);
const char *dsr_board_hardware_id(void);
esp_err_t dsr_board_start_ui_task(bool (*ready)(void));
esp_err_t dsr_board_set_backlight(uint8_t percent);

esp_err_t dsr_board_restore_backlight(void);
