/* Real LVGL view lifetime/telemetry regressions; no physical motor or RPC. */
#define main original_ui_main
#include "test_ui.c"
#undef main

static dsr_mobile_model_t view;
static unsigned deleted,commands;
static void deleted_view(lv_event_t *e){(void)e;deleted++;}
static void move_cover(unsigned index,int direction){CHECK(index==0&&direction==1);commands++;}
static void stop_cover(unsigned index){CHECK(index==0);commands++;}
static void position_cover(unsigned index,uint8_t value){(void)index;(void)value;commands++;}
static lv_obj_t *rail(lv_obj_t *o)
{
 if(lv_obj_get_width(o)==176&&lv_obj_get_height(o)==142)return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=rail(lv_obj_get_child(o,(int)i));if(r)return r;}
 return NULL;
}
static void frame(const char *name,unsigned skin)
{char text[80];snprintf(text,sizeof(text),"cover-refresh-%u-%s",skin,name);screenshot(text);}
static lv_obj_t *position_text(unsigned value,bool historical)
{char text[64];if(historical)snprintf(text,sizeof(text),dsr_sc_tr("Ostatnia pozycja: %u%%"),value);else snprintf(text,sizeof(text),"%s %u%%",dsr_sc_tr("Pozycja"),value);return find_label(lv_screen_active(),text);}
static uint32_t visual_hash(void)
{lv_refr_now(display);uint32_t hash=2166136261u;for(unsigned y=182;y<320;y++)for(unsigned x=66;x<238;x++)for(unsigned c=0;c<3;c++)hash=(hash^canvas[(y*320+x)*3+c])*16777619u;return hash;}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 dsr_mobile_actions_t a={.set_cover=move_cover,.stop_cover=stop_cover,.set_position=position_cover};dsr_mobile_bind(&a);
 const unsigned skins[]={0,2,3};
 for(unsigned s=0;s<3;s++)for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,skins[s],lang);
  view=(dsr_mobile_model_t){.wifi=true,.device_count=1};
  dsr_mobile_device_t *d=&view.devices[0];*d=(dsr_mobile_device_t){.id=41,.name="Cover refresh",.identity="cover-test",.host="192.168.1.80",.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_COVER,.kind=DSR_DEVICE_COVER,.online=true,.can_cover=true,.can_stop_cover=true,.can_position=true,.has_position=true,.position_pct=79,.cover_state=DSR_COVER_STOPPED,.has_power=true,.power_w=3.2,.has_voltage=true,.voltage_v=230};
  dsr_mobile_update(&view);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());
  lv_obj_t *title=find_label(lv_screen_active(),"Cover refresh"),*art=rail(lv_screen_active());CHECK(title&&art);
  lv_obj_add_event_cb(title,deleted_view,LV_EVENT_DELETE,NULL);lv_obj_add_event_cb(art,deleted_view,LV_EVENT_DELETE,NULL);deleted=0;
  unsigned before=commands;click("Otwórz");CHECK(commands==before+1);
  d->busy=d->command_pending=true;dsr_mobile_update(&view);
  CHECK(!deleted&&rail(lv_screen_active())==art);
  CHECK(position_text(79,false)&&find_label(lv_screen_active(),"Wysyłanie polecenia…"));
  d->busy=d->command_pending=false;d->online=d->has_position=false;d->can_cover=d->can_position=false;d->has_power=d->has_voltage=false;d->error=DSR_OFFLINE;dsr_mobile_update(&view);
  CHECK(!deleted&&position_text(79,true));
  CHECK(find_label(lv_screen_active(),"Offline")&&!find_label(lv_screen_active(),"Nieznana pozycja"));
  CHECK(lv_obj_has_state(lv_obj_get_parent(find_label(lv_screen_active(),"Otwórz")),LV_STATE_DISABLED));
  CHECK(!lv_obj_has_state(lv_obj_get_parent(find_label(lv_screen_active(),"Stop")),LV_STATE_DISABLED));
  if(!lang)frame("timeout",skins[s]);
  d->online=d->has_position=d->can_cover=d->can_position=true;d->error=DSR_OK;d->position_pct=75;d->cover_state=DSR_COVER_CLOSING;dsr_mobile_update(&view);
  CHECK(!deleted&&position_text(75,false)&&!position_text(79,true));
  uint32_t first=visual_hash();lv_tick_inc(150);lv_anim_refr_now();CHECK(visual_hash()!=first);
  for(unsigned i=0;i<8;i++){d->position_pct=74-i;dsr_mobile_update(&view);lv_tick_inc(100);lv_timer_handler();CHECK(!deleted&&rail(lv_screen_active())==art);}
  if(!lang)frame("moving",skins[s]);
  /* A malformed/calibrating position and a rebind must not reuse history. */
  d->has_position=false;d->cover_state=DSR_COVER_CALIBRATING;dsr_mobile_update(&view);CHECK(find_label(lv_screen_active(),"Nieznana pozycja")&&!position_text(67,true));
  d->online=false;d->error=DSR_AUTH_REQUIRED;dsr_mobile_update(&view);CHECK(find_label(lv_screen_active(),"Nieznana pozycja"));
  d->online=d->has_position=true;d->error=DSR_OK;d->cover_state=DSR_COVER_STOPPED;dsr_mobile_update(&view);CHECK(position_text(67,false));
  d->online=d->has_position=false;d->error=DSR_OFFLINE;dsr_mobile_update(&view);CHECK(position_text(67,true));
  strcpy(d->identity,"replacement");dsr_mobile_update(&view);CHECK(deleted&&find_label(lv_screen_active(),"Nieznana pozycja")&&!position_text(67,true));
  dsr_mobile_hide();lv_obj_clean(lv_screen_active());
 }
 printf("PASS: %u cover background-refresh checks; stable artwork, pending, timeout history, recovery, safety and rebind; 3 skins x PL/EN/DE; RPC/motor not exercised\n",checks);lv_deinit();return 0;
}
