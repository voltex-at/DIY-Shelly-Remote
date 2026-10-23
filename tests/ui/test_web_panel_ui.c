/* Actual pinned LVGL; only pairing/HTTP/worker providers are synthetic. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static lv_obj_t *panel_message(const char *text,bool warning,unsigned skin)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label);lv_obj_t *frame=lv_obj_get_parent(label);
 CHECK(lv_obj_get_width(frame)==296&&lv_obj_get_style_border_width(frame,0)>0);
 if(skin==DSR_SC_STYLE_CYBER_BLUE)CHECK(lv_obj_get_style_border_width(frame,0)==2);
 if(warning){uint32_t gold=skin==DSR_SC_STYLE_LIGHT?0xb16b00:0xffc107;CHECK((lv_color_to_u32(lv_obj_get_style_text_color(label,0))&0xffffff)==gold);CHECK((lv_color_to_u32(lv_obj_get_style_border_color(frame,0))&0xffffff)==gold);}
 lv_area_t a,b;lv_obj_get_coords(label,&a);lv_obj_get_coords(frame,&b);CHECK(a.x1>=b.x1+8&&a.x2<=b.x2-8&&a.y1>=b.y1+8&&a.y2<=b.y2-8);
 for(unsigned i=0;i<lv_obj_get_child_count(frame);i++){lv_obj_t *one=lv_obj_get_child(frame,i);if(!lv_obj_check_type(one,&lv_label_class))continue;lv_area_t x;lv_obj_get_coords(one,&x);CHECK(x.x1>=b.x1+8&&x.x2<=b.x2-8&&x.y1>=b.y1+8&&x.y2<=b.y2-8);
  for(unsigned j=i+1;j<lv_obj_get_child_count(frame);j++){lv_obj_t *two=lv_obj_get_child(frame,j);if(!lv_obj_check_type(two,&lv_label_class))continue;lv_area_t z;lv_obj_get_coords(two,&z);bool separated=x.x2<z.x1||z.x2<x.x1||x.y2+7<z.y1||z.y2+7<x.y1;if(!separated)fprintf(stderr,"card %s: child%u y%d..%d vs child%u y%d..%d\n",text,i,(int)x.y1,(int)x.y2,j,(int)z.y1,(int)z.y2);CHECK(separated);}}
 lv_obj_t *body=lv_obj_get_parent(frame);int previous=-10000;
 for(unsigned i=0;i<lv_obj_get_child_count(body);i++){lv_obj_t *item=lv_obj_get_child(body,i);int y=lv_obj_get_y(item);CHECK(y>=previous+8);previous=y+lv_obj_get_height(item);}
 return frame;
}
static void panel_shot(unsigned skin,unsigned lang,const char *name)
{if(lang==2||skin==DSR_SC_STYLE_CYBER_BLUE){char file[96];snprintf(file,sizeof(file),"web-panel-%u-%u-%s",skin,lang,name);screenshot(file);}}
static void panel_open(void)
{dsr_ui_open_settings();tap_row("Panel WWW / bezpieczeństwo");lv_obj_update_layout(lv_screen_active());}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 test_workspace_disable();dsr_ui_init();tick();if(in_flight){response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);}wifi.connected=false;revoke_regression();
 const unsigned skins[]={DSR_SC_STYLE_GRAPHITE,DSR_SC_STYLE_LIGHT,DSR_SC_STYLE_CYBER_BLUE};
 for(unsigned s=0;s<3;s++)for(unsigned lang=0;lang<3;lang++){
  unsigned skin=skins[s];dsr_sc_appearance(DSR_SC_BLUE,skin,lang);dsr_ui_apply_theme();test_panel_ready(true);test_pair_accept(true);test_pairing_state(true,true,false);panel_open();
  panel_message("Dostęp WWW: aktywny.",false,skin);panel_message("SHA-256 certyfikatu tego pilota",false,skin);panel_shot(skin,lang,"security");unsigned codes=test_pair_code_requests();tap_row("Wygeneruj kod parowania");lv_obj_update_layout(lv_screen_active());CHECK(test_pair_code_requests()==codes+1);
  panel_message("0123456789abcdef",false,skin);panel_message("Sprawdź odcisk certyfikatu w przeglądarce przed wpisaniem kodu.",true,skin);panel_shot(skin,lang,"pair");
  lv_tick_inc(120001);dsr_ui_settings_tick();lv_obj_update_layout(lv_screen_active());CHECK(!find_label(lv_screen_active(),"0123456789abcdef"));panel_message("Kod parowania wygasł.",true,skin);panel_shot(skin,lang,"expired");
  revoke_open();lv_obj_update_layout(lv_screen_active());panel_message("Stare cookie utracą dostęp. Trwające operacje mogą się zakończyć. Wi-Fi i dane pozostaną.",true,skin);panel_shot(skin,lang,"revoke");
  next_submit_result=DSR_BUSY;click("Unieważnij parowanie");lv_obj_update_layout(lv_screen_active());panel_message("Pilot jest zajęty. Spróbuj ponownie.",true,skin);CHECK(!in_flight);panel_shot(skin,lang,"busy");
  click("Unieważnij parowanie");CHECK(in_flight);response=(dsr_event_t){0};test_pairing_state(true,true,true);deliver(DSR_IO,DSR_WIFI_IDLE);lv_obj_update_layout(lv_screen_active());panel_message("Nie potwierdzono trwałego unieważnienia. Spróbuj ponownie.",true,skin);panel_shot(skin,lang,"failure");click("Wstecz");
 }
 CHECK(!job_counts[DSR_JOB_OTA_INSTALL]&&!restart_requests);printf("PASS: %u web security LCD checks; real LVGL, pairing/worker MODEL\n",checks);lv_deinit();return 0;
}
