/* Real LVGL and dsr_actions log. Only radio/storage/clock are host providers. */
#define main original_ui_main
#include "test_ui.c"
#undef main

static lv_obj_t *contains(lv_obj_t *root,const char *text)
{
 if(lv_obj_check_type(root,&lv_label_class)&&strstr(lv_label_get_text(root),text))return root;
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *found=contains(lv_obj_get_child(root,i),text);if(found)return found;}
 return NULL;
}
static void activity_open(void)
{dsr_ui_system_open(NULL);CHECK(find_label(lv_screen_active(),"Aktywność"));click("Aktywność");}
static void activity_seed(unsigned first,unsigned last)
{for(unsigned id=first;id<=last;id++)dsr_actions_observe(id,DSR_ACTION_ON,id==last?DSR_RATE_LIMITED:DSR_OK);}
static void activity_shot(unsigned skin,unsigned lang,const char *state)
{if(lang==2||skin==3){char file[80];snprintf(file,sizeof(file),"activity-%u-%u-%s",skin,lang,state);screenshot(file);}}
static void activity_bounds(void)
{
 const char *actions[]={"Nowsze","Starsze","Odśwież","Wyczyść"};
 for(unsigned i=0;i<4;i++){
  lv_obj_t *l=find_label(lv_screen_active(),actions[i]);CHECK(l);lv_obj_t *b=lv_obj_get_parent(l);lv_area_t a;lv_obj_get_coords(b,&a);
  CHECK(a.x1>=12&&a.x2<=307&&a.y2<=411&&lv_obj_get_height(b)>=44);
  lv_area_t t;lv_obj_get_coords(l,&t);CHECK(t.x1>a.x1&&t.x2<a.x2&&t.y1>=a.y1&&t.y2<=a.y2);
 }
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 test_workspace_disable();dsr_ui_init();tick();if(in_flight){response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);}wifi.connected=false;
 const unsigned skins[]={0,2,3};dsr_action_log_page_t page;unsigned initial_requests=requests;
 for(unsigned s=0;s<3;s++)for(unsigned lang=0;lang<3;lang++){
  unsigned skin=skins[s];dsr_sc_appearance(DSR_SC_BLUE,skin,lang);dsr_ui_apply_theme();dsr_actions_log_clear();activity_open();
  CHECK(find_label(lv_screen_active(),"Brak aktywności od uruchomienia."));activity_bounds();activity_shot(skin,lang,"empty");
  activity_seed(1,140);CHECK(dsr_actions_log(0,0,6,&page)&&page.total==128&&page.count==6);
  click("Odśwież");CHECK(contains(lv_screen_active(),"#140"));CHECK(contains(lv_screen_active(),"#135"));CHECK(!contains(lv_screen_active(),"#134"));
  if(lang){CHECK(!contains(lv_screen_active(),"Błąd"));CHECK(!contains(lv_screen_active(),"Zakończono"));}
  activity_bounds();activity_shot(skin,lang,"log");
  click("Starsze");CHECK(contains(lv_screen_active(),"#134"));CHECK(!contains(lv_screen_active(),"#140"));
  activity_seed(141,141);CHECK(contains(lv_screen_active(),"#134"));click("Odśwież");CHECK(contains(lv_screen_active(),"#141"));
  click("Wyczyść");CHECK(find_label(lv_screen_active(),"Wyczyścić historię aktywności?"));activity_shot(skin,lang,"confirm");click("Anuluj");CHECK(dsr_actions_log(0,0,6,&page)&&page.total==128);
  click("Wyczyść");dsr_actions_log_clear();activity_seed(500,500);click("Wyczyść historię");CHECK(dsr_actions_log(0,0,6,&page)&&page.total==1);CHECK(contains(lv_screen_active(),"#500"));
  click("Wyczyść");click("Wyczyść historię");CHECK(dsr_actions_log(0,0,6,&page)&&page.total==0);CHECK(find_label(lv_screen_active(),"Brak aktywności od uruchomienia."));
  dsr_actions_log_clear();activity_seed(700,700);dsr_ui_activity_tick();CHECK(contains(lv_screen_active(),"#700"));
  CHECK(!contains(lv_screen_active(),"#500"));
  click("Wyczyść");dsr_ui_system_open(NULL);activity_seed(701,701);dsr_ui_activity_tick();CHECK(dsr_actions_log(0,0,6,&page)&&page.total==2);
  dsr_actions_log_clear();fixture_utc=0;activity_seed(900,900);fixture_utc=1760877240;activity_open();
  CHECK(contains(lv_screen_active(),lang==0?"Brak UTC":lang==1?"UTC unset":"UTC fehlt"));
  dsr_connection_t observed={.layout_id=901,.device={.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true}};
  snprintf(observed.device.host,sizeof(observed.device.host),"192.0.2.222");snprintf(observed.device.device_id,sizeof(observed.device.device_id),"secret-identity");
  CHECK(dsr_actions_publish_begin(1,true));dsr_actions_publish_at(&observed,true,false,DSR_OK,true);dsr_actions_publish_end();
  dsr_actions_log_clear();observed.device.output=true;
  CHECK(dsr_actions_publish_begin(1,true));dsr_actions_publish_at(&observed,true,false,DSR_OK,true);dsr_actions_publish_end();
  click("Odśwież");CHECK(contains(lv_screen_active(),dsr_sc_tr("Zaobserwowano")));CHECK(!contains(lv_screen_active(),dsr_sc_tr("Zakończono")));
  CHECK(!contains(lv_screen_active(),"192.0.2.222")&&!contains(lv_screen_active(),"secret-identity"));activity_shot(skin,lang,"observed");
 }
 CHECK(requests==initial_requests);CHECK(!restart_requests);printf("PASS: %u activity LCD checks; real LVGL and real RAM log, radio/storage MODEL\n",checks);lv_deinit();return 0;
}
