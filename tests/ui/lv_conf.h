#ifndef LV_CONF_H
#define LV_CONF_H
#define LV_COLOR_DEPTH 16
#define LV_USE_OS LV_OS_NONE
#define LV_MEM_SIZE (128 * 1024U)
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_16
#define LV_USE_LOG 0
#include <stdio.h>
#include <stdlib.h>
/* Fail visibly on the host; the embedded default spins forever. */
#define LV_ASSERT_HANDLER do { fprintf(stderr,"LVGL assertion at %s:%d\n",__FILE__,__LINE__); abort(); } while(0);
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_OBJ 1
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_DRAW_SW 1
#endif
