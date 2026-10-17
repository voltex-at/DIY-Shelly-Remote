#ifndef DSR_UI_BRAND_H
#define DSR_UI_BRAND_H
#include <stdbool.h>
#include "lvgl.h"
const lv_image_dsc_t *dsr_ui_brand_background(void);
const lv_image_dsc_t *dsr_ui_brand_splash(void);
void dsr_ui_splash_show(void);
void dsr_ui_splash_release(void);
bool dsr_ui_splash_visible(void);
#endif
