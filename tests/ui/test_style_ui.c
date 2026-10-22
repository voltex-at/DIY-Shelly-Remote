/* Genuine LVGL: "Jasny" style, owner background artwork and boot splash.
 * SDK/storage remain MODEL. Contrast guard: every visible label must differ
 * from the nearest opaque ancestor background by a readable luminance gap. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "../../components/dsr_ui/dsr_ui_brand.h"
#include <math.h>
static double luminance(lv_color_t c)
{
 double v[3]={c.red/255.0,c.green/255.0,c.blue/255.0};
 for(int i=0;i<3;i++)v[i]=v[i]<=0.03928?v[i]/12.92:pow((v[i]+0.055)/1.055,2.4);
 return 0.2126*v[0]+0.7152*v[1]+0.0722*v[2];
}
static unsigned contrast_checks,low_contrast;
static void contrast(lv_obj_t *o,double minimum)
{
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return;
 if(lv_obj_check_type(o,&lv_label_class)&&lv_label_get_text(o)[0]){
  lv_obj_t *p=lv_obj_get_parent(o);
  while(p&&lv_obj_get_style_bg_opa(p,LV_PART_MAIN)<LV_OPA_50&&!lv_obj_get_style_bg_image_src(p,LV_PART_MAIN))p=lv_obj_get_parent(p);
  if(p&&!lv_obj_has_state(p,LV_STATE_DISABLED)){
   double a=luminance(lv_obj_get_style_text_color(o,LV_PART_MAIN)),b=luminance(lv_obj_get_style_bg_color(p,LV_PART_MAIN));
   double ratio=(fmax(a,b)+0.05)/(fmin(a,b)+0.05);
   if(ratio<minimum){fprintf(stderr,"low contrast %.2f for '%s' text=%06x bg=%06x style=%u\n",ratio,lv_label_get_text(o),lv_color_to_u32(lv_obj_get_style_text_color(o,LV_PART_MAIN))&0xffffff,lv_color_to_u32(lv_obj_get_style_bg_color(p,LV_PART_MAIN))&0xffffff,dsr_sc_style());low_contrast++;}
   contrast_checks++;
  }
 }
 for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)contrast(lv_obj_get_child(o,(int32_t)i),minimum);
}
static void publish_style(const char *style)
{cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(style));publish_document(doc);tick();}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 /* Boot splash follows the NVS style hint before the workspace exists. */
 style_hint=DSR_SC_STYLE_GRAPHITE;dsr_ui_splash_begin();CHECK(dsr_ui_splash_visible()&&provision_gate&&dsr_ui_brand_splash()==dsr_ui_brand_splash());
 screenshot("330-splash-dark");dsr_ui_splash_end();CHECK(!provision_gate);dsr_ui_splash_release();
 style_hint=DSR_SC_STYLE_LIGHT;dsr_ui_splash_begin();CHECK(dsr_sc_style()==DSR_SC_STYLE_LIGHT);screenshot("331-splash-light");dsr_ui_splash_end();
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();CHECK(!dsr_ui_splash_visible());tick();
 /* Same frames in each of the exactly three skins; the hint follows the config. */
 const char *styles[]={"graphite","light","cyber_blue"};const unsigned ids[]={DSR_SC_STYLE_GRAPHITE,DSR_SC_STYLE_LIGHT,DSR_SC_STYLE_CYBER_BLUE};
 for(unsigned s=0;s<3;s++){
  publish_style(styles[s]);CHECK(dsr_sc_style()==ids[s]&&style_hint==dsr_sc_style());
  char name[48];const double minimum=4.0;
  dsr_network_home(NULL);tick();CHECK(lv_obj_get_style_bg_image_src(lv_screen_active(),LV_PART_MAIN)==(const void *)dsr_ui_brand_background());
  contrast(lv_screen_active(),minimum);snprintf(name,sizeof(name),"%u-style-home-%s",332+s*10,styles[s]);screenshot(name);
  if(!s){
   dsr_network_device(NULL);choose_method("Dodaj ręcznie (IP)");lv_textarea_set_text(find_type(lv_screen_active(),&lv_textarea_class),"192.168.1.77");click("Check & save device");CHECK(submitted.kind==DSR_JOB_BIND);
   response.device=(dsr_device_t){.host="192.168.1.77",.name="Desk lamp",.device_id="shellyplus1-AABBCCDDEEFF",.generation=DSR_GEN2_PLUS,.online=true,.has_switch=true,.output=true,.has_power=true,.power_w=4.1};
   response.telemetry=(dsr_telemetry_t){.has_voltage=true,.has_current=true,.voltage_v=230.4,.current_a=0.018};deliver(DSR_OK,DSR_WIFI_IDLE);
  }
  dsr_network_home(NULL);tick();CHECK(find_label(lv_screen_active(),"Desk lamp"));contrast(lv_screen_active(),minimum);snprintf(name,sizeof(name),"%u-style-card-%s",333+s*10,styles[s]);screenshot(name);
  click("Desk lamp");tick();contrast(lv_screen_active(),minimum);snprintf(name,sizeof(name),"%u-style-device-%s",334+s*10,styles[s]);screenshot(name);
  dsr_ui_open_settings();tick();contrast(lv_screen_active(),minimum);snprintf(name,sizeof(name),"%u-style-settings-%s",335+s*10,styles[s]);screenshot(name);
  tap_row("Wygląd i język");tick();contrast(lv_screen_active(),minimum);snprintf(name,sizeof(name),"%u-style-appearance-%s",336+s*10,styles[s]);screenshot(name);
  dsr_network_device(NULL);tick();contrast(lv_screen_active(),minimum);snprintf(name,sizeof(name),"%u-style-wizard-%s",337+s*10,styles[s]);screenshot(name);
  choose_method("Dodaj ręcznie (IP)");tick();snprintf(name,sizeof(name),"%u-style-form-%s",338+s*10,styles[s]);screenshot(name);
  dsr_ui_edit_dashboard();tick();contrast(lv_screen_active(),minimum);snprintf(name,sizeof(name),"%u-style-editor-%s",339+s*10,styles[s]);screenshot(name);
  dsr_network_home(NULL);tick();
 }
 /* The retired midnight skin, e.g. saved by a Web Panel page opened before the
  * update, is applied as graphite: dark art, one hint write, never id 1. */
 unsigned writes=style_hint_writes;publish_style("midnight");CHECK(dsr_sc_style()==DSR_SC_STYLE_GRAPHITE&&style_hint==DSR_SC_STYLE_GRAPHITE&&style_hint_writes==writes+1);
 cJSON *saved=room_document();CHECK(!strcmp(cJSON_GetObjectItemCaseSensitive(saved,"theme")->valuestring,"graphite"));cJSON_Delete(saved);
 publish_style("midnight");CHECK(style_hint_writes==writes+1);
 /* Any legacy style id 1 (old hint or caller) renders graphite. */
 dsr_sc_appearance(DSR_SC_BLUE,1,0);CHECK(dsr_sc_style()==DSR_SC_STYLE_GRAPHITE);CHECK(dsr_sc_style_from_name("midnight")==DSR_SC_STYLE_GRAPHITE);
 /* The LCD theme picker offers exactly three skins in PL/EN/DE. */
 const char *picker[]={"Grafit\nJasny\nCyber Blue","Graphite\nLight\nCyber Blue","Graphit\nHell\nCyber Blue"};
 for(unsigned language=0;language<3;language++){
  cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));publish_document(doc);tick();
  dsr_ui_open_settings();tick();tap_row("Wygląd i język");tick();
  unsigned skip=1;lv_obj_t *theme=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);CHECK(theme&&!strcmp(lv_dropdown_get_options(theme),picker[language]));
  CHECK(lv_dropdown_get_option_count(theme)==3&&lv_dropdown_get_selected(theme)==0);
  char name[48];snprintf(name,sizeof(name),"360-style-picker-%u",language);screenshot(name);dsr_network_home(NULL);tick();
 }
 CHECK(!low_contrast);
 printf("PASS: %u style/brand LCD checks; %u frames; %u label contrast checks >= 4.0:1 (graphite, light, cyber_blue), midnight migrated to graphite, splash per NVS hint, hint writes only on change; SDK/storage MODEL\n",checks,screenshots,contrast_checks);
 lv_deinit();return 0;
}
