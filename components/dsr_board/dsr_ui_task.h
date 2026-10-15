#ifndef DSR_UI_TASK_H
#define DSR_UI_TASK_H
#include "esp_err.h"
/* Called once, after UI construction, from app_main. No LVGL init races. */
esp_err_t dsr_ui_task_start(void);
#endif
