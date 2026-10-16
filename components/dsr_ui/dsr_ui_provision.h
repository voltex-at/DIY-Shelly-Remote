#ifndef DSR_UI_PROVISION_H
#define DSR_UI_PROVISION_H
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "lvgl.h"
/* First-format progress screen. show/finish/release run on the thread that
 * currently owns LVGL; the screen's own timer runs in the UI task. */
void dsr_ui_provision_show(bool resume,uint32_t total_blocks,uint32_t remnant_blocks,uint32_t (*progress)(void));
void dsr_ui_provision_finish(esp_err_t result);
void dsr_ui_provision_release(void);
bool dsr_ui_provision_visible(void);
bool dsr_ui_provision_layout_ok(void);
#endif
