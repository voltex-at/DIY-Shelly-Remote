#ifndef DSR_UI_TASK_H
#define DSR_UI_TASK_H
#include "esp_err.h"
#include <stdbool.h>
/* Called once from app_main (or earlier by the first-format progress
 * screen with the same gate). LVGL is only touched while the gate is open. */
esp_err_t dsr_ui_task_start(bool (*startup_ready)(void));
/* Wait until no lv_timer_handler pass is running. The caller must have
 * closed startup_ready() first; then main may touch LVGL again (#28). */
void dsr_ui_task_quiesce(void);
bool dsr_ui_task_handler_active(void);
bool dsr_ui_task_started_with(bool (*startup_ready)(void));
#endif
