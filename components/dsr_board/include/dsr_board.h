#ifndef DSR_BOARD_H
#define DSR_BOARD_H
#include "esp_err.h"
const char *dsr_board_hardware_id(void);
esp_err_t dsr_board_init(void);
#ifdef DSR_BOARD_ES3C35P
#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"
lv_display_t *dsr_board_display(void);
/* The owner waits for startup_ready before handling any LVGL timer/input. */
esp_err_t dsr_board_start_ui_task(bool (*startup_ready)(void));
/* Actual reserved UI stack capacity, in bytes (IDF task-depth units). */
extern const uint32_t dsr_board_ui_stack_bytes;
/* After closing the gate: wait until the UI task is outside LVGL. */
void dsr_board_ui_quiesce(void);
/* Block the calling (main) task so the UI task can render meanwhile. */
void dsr_board_ui_hold(uint32_t ms);
esp_err_t dsr_board_set_backlight(uint8_t percent);
void dsr_board_sleep_display(void);
bool dsr_board_display_sleeping(void);
esp_err_t dsr_board_restore_backlight(void);
esp_err_t dsr_board_apply_display(uint8_t brightness, uint16_t idle_seconds);
bool dsr_board_touch_available(void);
#endif
#endif
