/* Real pinned LVGL/UI; HTTPS permission and clock are host providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static void web_message(const char *key,bool warning,unsigned skin)
{
 lv_obj_t *text=find_label(lv_screen_active(),key);CHECK(text);
 lv_obj_t *frame=lv_obj_get_parent(text);CHECK(frame!=lv_screen_active());
 uint32_t gold=skin==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;
 if(warning){CHECK((lv_color_to_u32(lv_obj_get_style_text_color(text,0))&0xffffff)==gold);CHECK((lv_color_to_u32(lv_obj_get_style_border_color(frame,0))&0xffffff)==gold);}
 if(skin==DSR_SC_STYLE_CYBER_BLUE)CHECK(lv_obj_get_style_border_width(frame,0)==2);
 lv_area_t a,b;lv_obj_get_coords(text,&a);lv_obj_get_coords(frame,&b);
 CHECK(a.y1>=b.y1+8&&a.y2<=b.y2-8&&a.x1>=b.x1+8&&a.x2<=b.x2-8);
 CHECK(b.y2<=403); /* All message frames fit above the fixed Back action. */
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 test_workspace_disable();dsr_ui_init();tick();if(in_flight){response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);}
 const unsigned skins[]={DSR_SC_STYLE_GRAPHITE,DSR_SC_STYLE_LIGHT,DSR_SC_STYLE_CYBER_BLUE};
 for(unsigned s=0;s<3;s++)for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,skins[s],language);dsr_ui_apply_theme();
  for(unsigned allowed=0;allowed<2;allowed++){
   test_ota_accept(allowed);dsr_ui_web_ota_open(NULL);lv_obj_update_layout(lv_screen_active());
   char path[64];snprintf(path,sizeof(path),"web-ota-%u-%u-%s",skins[s],language,allowed?"allowed":"denied");
   if(language==2||s==2)screenshot(path);
   web_message(allowed?"OTA dozwolone przez 2 minuty. Wybierz BIN w panelu.":"Nie udzielono zgody OTA. HTTPS niedostępny lub zajęty.",!allowed,skins[s]);
   web_message("To lokalny tryb deweloperski, nie podpisany updater z pilota.",true,skins[s]);
   if(allowed){lv_tick_inc(120001);dsr_ui_settings_tick();lv_obj_update_layout(lv_screen_active());web_message("Zgoda na OTA wygasła.",true,skins[s]);if(language==2||s==2){snprintf(path,sizeof(path),"web-ota-%u-%u-expired",skins[s],language);screenshot(path);}}
   click("Wstecz");lv_tick_inc(120001);dsr_ui_settings_tick(); /* deleted labels cannot be reused */
  }
 }
 CHECK(!job_counts[DSR_JOB_OTA_INSTALL]&&!restart_requests);printf("PASS: %u local OTA LCD checks; permission/clock MODEL, no upload or restart\n",checks);lv_deinit();return 0;
}
