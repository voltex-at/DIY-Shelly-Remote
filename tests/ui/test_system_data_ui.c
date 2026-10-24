#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_maintenance_api.h"
#include "dsr_maintenance_recovery.h"
void test_maintenance_set(unsigned,unsigned,dsr_result_t,unsigned);
void test_maintenance_usb(void);
void test_maintenance_archive(uint32_t,const char *);
unsigned test_maintenance_confirms(void);
unsigned test_maintenance_resets(void);
bool test_maintenance_last_usb(void);
static lv_obj_t *hold_button(void){lv_obj_t *l=find_label(lv_screen_active(),"Przytrzymaj 3 s");CHECK(l);return lv_obj_get_parent(l);}
static void press_hold(lv_obj_t *b,unsigned ms){lv_obj_send_event(b,LV_EVENT_PRESSED,NULL);lv_obj_add_state(b,LV_STATE_PRESSED);lv_tick_inc(ms);dsr_ui_maintenance_tick();}
static void release_hold(lv_obj_t *b){lv_obj_send_event(b,LV_EVENT_RELEASED,NULL);lv_obj_remove_state(b,LV_STATE_PRESSED);}
static void button_bounds(const char *key){lv_obj_t *l=find_label(lv_screen_active(),key);CHECK(l);lv_obj_t *b=lv_obj_get_parent(l);lv_obj_update_layout(b);lv_area_t a,t;lv_obj_get_coords(b,&a);lv_obj_get_coords(l,&t);CHECK(a.x1>=12&&a.x2<=307&&a.y2<=411&&lv_obj_get_height(b)>=44);CHECK(t.x1>a.x1&&t.x2<a.x2&&t.y1>=a.y1&&t.y2<=a.y2);}
static void no_private(lv_obj_t *root){if(lv_obj_check_type(root,&lv_label_class)){const char *t=lv_label_get_text(root);CHECK(!strstr(t,"192.168.")&&!strstr(t,"super-secret")&&!strstr(t,"aabbccddeeff")&&!strstr(t,"-----BEGIN"));}for(unsigned i=0;i<lv_obj_get_child_count(root);i++)no_private(lv_obj_get_child(root,i));}
static void shot(unsigned skin,unsigned language,const char *name){char file[96];snprintf(file,sizeof(file),"maintenance-%u-%u-%s",skin,language,name);screenshot(file);}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 test_workspace_disable();dsr_ui_init();tick();if(in_flight){response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);}
 const unsigned skins[]={0,2,3};unsigned id=1,initial=requests;
 for(unsigned s=0;s<3;s++)for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,skins[s],lang);dsr_ui_apply_theme();test_maintenance_set(0,DSR_MAINT_IDLE,DSR_OK,id++);dsr_ui_maintenance_open(NULL);CHECK(find_label(lv_screen_active(),"Kopia i reset"));
  test_maintenance_set(DSR_MAINT_EXPORT,DSR_MAINT_CONFIRM,DSR_OK,id++);dsr_ui_maintenance_tick();CHECK(find_label(lv_screen_active(),"Zezwól na eksport"));button_bounds("Potwierdź");no_private(lv_screen_active());if(lang==0)shot(skins[s],lang,"export");unsigned before=test_maintenance_confirms();click("Potwierdź");CHECK(test_maintenance_confirms()==before+1);
  test_maintenance_set(DSR_MAINT_IMPORT_UPLOAD,DSR_MAINT_CONFIRM,DSR_OK,id++);dsr_ui_maintenance_tick();CHECK(find_label(lv_screen_active(),"Zezwól na przesłanie"));click("Anuluj");
  test_maintenance_set(DSR_MAINT_IMPORT_COMMIT,DSR_MAINT_CONFIRM,DSR_OK,id++);test_maintenance_archive(0,"0.7.70-dev");dsr_ui_maintenance_tick();CHECK(find_label(lv_screen_active(),"Przywróć tę kopię"));char identity[128];snprintf(identity,sizeof(identity),"Firmware: 0.7.70-dev\n%s",dsr_sc_tr("Czas kopii nieustawiony"));CHECK(find_label(lv_screen_active(),identity));
  test_maintenance_archive(1792800000,"0.7.71-dev");dsr_ui_maintenance_tick();CHECK(find_label(lv_screen_active(),"Firmware: 0.7.71-dev\nUTC: 2026-10-24 00:00:00"));button_bounds("Potwierdź");no_private(lv_screen_active());if(lang==1)shot(skins[s],lang,"candidate");
  test_maintenance_set(DSR_MAINT_RESET,DSR_MAINT_CONFIRM,DSR_OK,id++);dsr_ui_maintenance_tick();unsigned reset_before=test_maintenance_resets();lv_obj_t *b=hold_button();press_hold(b,2999);CHECK(test_maintenance_resets()==reset_before);release_hold(b);dsr_ui_maintenance_tick();CHECK(test_maintenance_resets()==reset_before);
  b=hold_button();press_hold(b,2999);lv_tick_inc(1);dsr_ui_maintenance_tick();CHECK(test_maintenance_resets()==reset_before+1);dsr_ui_maintenance_tick();CHECK(test_maintenance_resets()==reset_before+1);
  test_maintenance_set(DSR_MAINT_RESET,DSR_MAINT_CONFIRM,DSR_INVALID,id++);test_maintenance_usb();dsr_ui_maintenance_tick();CHECK(find_label(lv_screen_active(),"Potwierdzam stabilne USB"));click("Potwierdzam stabilne USB");b=hold_button();if(lang==2)shot(skins[s],lang,"reset-usb");press_hold(b,3000);CHECK(test_maintenance_last_usb());
  test_maintenance_set(DSR_MAINT_RESET,DSR_MAINT_CONFIRM,DSR_OK,id++);dsr_ui_maintenance_tick();b=hold_button();press_hold(b,100);reset_before=test_maintenance_resets();dsr_ui_system_open(NULL);lv_tick_inc(3000);dsr_ui_maintenance_tick();CHECK(test_maintenance_resets()==reset_before);
  test_maintenance_set(DSR_MAINT_RESET,DSR_MAINT_CONFIRM,DSR_OK,id++);dsr_ui_maintenance_tick();b=hold_button();press_hold(b,2999);test_maintenance_set(DSR_MAINT_RESET,DSR_MAINT_CONFIRM,DSR_OK,id++);lv_tick_inc(1);dsr_ui_maintenance_tick();CHECK(test_maintenance_resets()==reset_before);
  test_maintenance_set(DSR_MAINT_IMPORT_COMMIT,DSR_MAINT_CONFIRM,DSR_OFFLINE,id++);dsr_ui_maintenance_tick();CHECK(find_label(lv_screen_active(),"Bateria poniżej 20%. Doładuj pilot."));
  dsr_ui_maintenance_hide();
 }
 dsr_sc_appearance(DSR_SC_BLUE,3,1);dsr_ui_maintenance_boot_begin();dsr_maint_status_t status={.op=DSR_MAINT_IMPORT_COMMIT,.phase=DSR_MAINT_APPLYING,.progress=50};dsr_ui_maintenance_boot_publish(&status);lv_tick_inc(101);lv_timer_handler();CHECK(find_label(lv_screen_active(),"Odzyskiwanie danych"));no_private(lv_screen_active());shot(3,1,"recovery");
 status.phase=DSR_MAINT_FAULT;status.result=DSR_IO;dsr_ui_maintenance_boot_publish(&status);lv_tick_inc(101);lv_timer_handler();shot(3,1,"recovery-fault");click("Ponów");bool usb=false;CHECK(dsr_ui_maintenance_boot_take_action(&usb)==1);CHECK(dsr_ui_maintenance_boot_take_action(&usb)==0);
 dsr_ui_maintenance_boot_publish(&status);lv_tick_inc(101);lv_timer_handler();click("Pełny reset danych");lv_obj_t *b=hold_button();lv_obj_send_event(b,LV_EVENT_PRESSED,NULL);lv_tick_inc(2999);lv_timer_handler();CHECK(dsr_ui_maintenance_boot_take_action(&usb)==0);click("Anuluj");lv_tick_inc(200);lv_timer_handler();CHECK(dsr_ui_maintenance_boot_take_action(&usb)==0);
 status.usb_required=true;dsr_ui_maintenance_boot_publish(&status);lv_tick_inc(101);lv_timer_handler();click("Pełny reset danych");click("Potwierdzam stabilne USB");b=hold_button();lv_obj_send_event(b,LV_EVENT_PRESSED,NULL);lv_tick_inc(3000);lv_timer_handler();CHECK(dsr_ui_maintenance_boot_take_action(&usb)==2&&usb);CHECK(dsr_ui_maintenance_boot_take_action(&usb)==0);
 dsr_ui_maintenance_boot_release();CHECK(requests==initial&&!restart_requests);printf("PASS: %u maintenance LCD checks; real LVGL9.3, grant/storage/power MODEL; no reset or device commands\n",checks);lv_deinit();return 0;
}
