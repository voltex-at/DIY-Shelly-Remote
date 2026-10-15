#include "dsr_ui_internal.h"
#include "dsr_workspace.h"
#include "dsr_board.h"
#include <stdlib.h>

void dsr_panel_tick(void)
{
#ifdef ESP_PLATFORM
 static void *pixels;
 static uint32_t started;
 if(!started)started=lv_tick_get();
 static bool confirmed;
 if(!confirmed&&lv_tick_get()-started>5000&&dsr_workspace_revision()&&dsr_panel_ready()&&dsr_board_touch_available()){
  dsr_ota_boot_confirm();confirmed=true;
 }
 if(!dsr_panel_preview_requested())return;
 if(!pixels)pixels=malloc(320*480*2+64);
 if(!pixels)return;
 lv_image_dsc_t image={0};
 if(lv_snapshot_take_to_buf(lv_screen_active(),LV_COLOR_FORMAT_RGB565,&image,pixels,320*480*2+64)==LV_RESULT_OK)
  dsr_panel_preview_publish(image.data,320*480*2);
#endif
}
