/* Real pinned LVGL pixels; memory/radio providers are synthetic host data. */
#define main original_ui_main
#include "test_ui.c"
#undef main
void test_system_runtime(dsr_system_runtime_t value);

static unsigned bars(lv_obj_t *o)
{
 unsigned count=lv_obj_check_type(o,&lv_bar_class);
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++)count+=bars(lv_obj_get_child(o,(int)i));
 return count;
}
static lv_obj_t *bar_at(lv_obj_t *card,unsigned index)
{
 for(unsigned i=0;i<lv_obj_get_child_count(card);i++){
  lv_obj_t *o=lv_obj_get_child(card,(int)i);
  if(lv_obj_check_type(o,&lv_bar_class)&&index--==0)return o;
 }
 CHECK(!"Missing memory bar");return NULL;
}
static lv_area_t memory_bounds(lv_obj_t *o)
{lv_area_t a;CHECK(o);lv_obj_get_coords(o,&a);return a;}
static void audit_card(lv_obj_t *card)
{
 lv_obj_update_layout(lv_screen_active());lv_area_t a=memory_bounds(card);
 lv_obj_t *action=lv_obj_get_parent(find_label(lv_screen_active(),"Zmierz napięcie"));
 CHECK(a.y1>=86&&a.y2+8<memory_bounds(action).y1); /* all rows above fixed actions */
 for(unsigned i=0;i<lv_obj_get_child_count(card);i++){
  lv_obj_t *o=lv_obj_get_child(card,(int)i);lv_area_t b=memory_bounds(o);
  CHECK(b.x1>=a.x1+8&&b.x2<=a.x2-8&&b.y1>=a.y1+4&&b.y2<=a.y2-4);
  if(lv_obj_check_type(o,&lv_label_class)){
   const lv_font_t *font=lv_obj_get_style_text_font(o,0);lv_point_t size;
   lv_text_get_size(&size,lv_label_get_text(o),font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
   CHECK(size.x<=lv_obj_get_width(o)); /* no wrapping/ellipsis in row values */
  }
 }
 for(unsigned i=0;i<5;i++){
  lv_obj_t *bar=bar_at(card,i);lv_area_t b=memory_bounds(bar);
  CHECK(!lv_obj_has_flag(bar,LV_OBJ_FLAG_CLICKABLE));CHECK(lv_obj_get_height(bar)>=6);
  if(i)CHECK(b.y1-memory_bounds(bar_at(card,i-1)).y2>=20);
 }
}
static void refresh(void){lv_tick_inc(1000);dsr_ui_battery_tick();lv_obj_update_layout(lv_screen_active());}
static void closed_cycle(void)
{dsr_ui_battery_open(NULL);refresh();lv_refr_now(display);dsr_ui_open_settings();refresh();lv_refr_now(display);}
static void caption(const char *format,unsigned long free,unsigned long total)
{char text[96];snprintf(text,sizeof(text),dsr_sc_tr(format),free,total);CHECK(find_label(lv_screen_active(),text));}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 test_workspace_disable();dsr_ui_init();tick();
 const unsigned skins[]={DSR_SC_STYLE_GRAPHITE,DSR_SC_STYLE_LIGHT,DSR_SC_STYLE_CYBER_BLUE};
 for(unsigned i=0;i<3;i++)for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,skins[i],lang);dsr_ui_apply_theme();dsr_ui_battery_open(NULL);tick();
  lv_obj_t *heading=find_label(lv_screen_active(),"Pamięć i radio");CHECK(heading);
  lv_obj_t *card=lv_obj_get_parent(heading);CHECK(bars(card)==5);
  lv_obj_scroll_to_view_recursive(card,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());audit_card(card);
  caption("Wolne %lu/%lu KiB",136,320);caption("Wolne %lu/%lu KiB",7812,8192);caption("Min. %lu/%lu B",2304,8192);
  CHECK(lv_bar_get_value(bar_at(card,0))==42&&lv_bar_get_value(bar_at(card,1))==95&&lv_bar_get_value(bar_at(card,3))==28);
  CHECK(lv_bar_get_value(bar_at(card,2))>0&&lv_bar_get_value(bar_at(card,2))<=100);
  CHECK(lv_bar_get_value(bar_at(card,4))==0&&find_label(card,"wyłączone"));
  if(lang==2||skins[i]==DSR_SC_STYLE_CYBER_BLUE){char name[64];snprintf(name,sizeof(name),"memory-%u-%u",skins[i],lang);screenshot(name);}
 }
 lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),"Pamięć i radio"));
 /* Live 0/100% endpoints and binary BLE, using the same existing objects. */
 system_info_model.internal_free=system_info_model.internal_total;system_info_model.psram_free=0;
 test_system_runtime((dsr_system_runtime_t){.internal_min_free=61440,.ui_stack_free=8192,.ble_active=true});refresh();
 CHECK(lv_bar_get_value(bar_at(card,0))==100&&lv_bar_get_value(bar_at(card,1))==0&&lv_bar_get_value(bar_at(card,3))==100);
 CHECK(!lv_obj_has_state(bar_at(card,1),LV_STATE_DISABLED));CHECK(lv_bar_get_max_value(bar_at(card,4))==1&&lv_bar_get_value(bar_at(card,4))==1&&find_label(card,"włączone"));
 caption("Wolne %lu/%lu KiB",320,320);caption("Wolne %lu/%lu KiB",0,8192);audit_card(card);screenshot("memory-endpoints");
 system_info_model.internal_free=0;system_info_model.psram_total=0;
 test_system_runtime((dsr_system_runtime_t){.ui_stack_free=0});refresh();
 CHECK(lv_bar_get_value(bar_at(card,0))==0&&!lv_obj_has_state(bar_at(card,0),LV_STATE_DISABLED));
 CHECK(lv_bar_get_value(bar_at(card,1))==0&&lv_obj_has_state(bar_at(card,1),LV_STATE_DISABLED));
 char unknown[64];snprintf(unknown,sizeof(unknown),dsr_sc_tr("Wolne %lu/— KiB"),0ul);CHECK(find_label(card,unknown));
 CHECK(lv_bar_get_value(bar_at(card,3))==0&&!lv_obj_has_state(bar_at(card,3),LV_STATE_DISABLED));screenshot("memory-unknown");
 /* Inconsistent reads and oversized counts do not fabricate a percentage. */
 system_info_model.internal_free=UINT32_MAX;system_info_model.internal_total=1;refresh();
 CHECK(lv_obj_has_state(bar_at(card,0),LV_STATE_DISABLED)&&lv_bar_get_value(bar_at(card,0))==0);
 system_info_model.internal_free=UINT32_MAX/2;system_info_model.internal_total=UINT32_MAX;refresh();
 CHECK(!lv_obj_has_state(bar_at(card,0),LV_STATE_DISABLED)&&lv_bar_get_value(bar_at(card,0))==49);
 system_info_model.internal_free=140000;system_info_model.internal_total=327680;system_info_model.psram_free=8000000;system_info_model.psram_total=8388608;
 test_system_runtime((dsr_system_runtime_t){.internal_min_free=61440,.ui_stack_free=2304});refresh();
 /* Stable refresh and repeated close/open must not grow the LVGL allocation. */
 lv_mem_monitor_t before,after;refresh();lv_mem_monitor(&before);for(unsigned i=0;i<8;i++)refresh();lv_mem_monitor(&after);CHECK(before.free_size==after.free_size);
 /* Warm the rendered transition before checking retained allocations. */
 for(unsigned i=0;i<3;i++){closed_cycle();}
 lv_mem_monitor(&before);
 for(unsigned i=0;i<8;i++){closed_cycle();}
 lv_mem_monitor(&after);CHECK(before.free_size==after.free_size&&before.used_cnt==after.used_cnt);
 printf("Memory: 8 closed-screen cycles, %lu -> %lu free bytes\n",(unsigned long)before.free_size,(unsigned long)after.free_size);
 dsr_mobile_model_t m={.wifi=true,.configured_layout=true,.tile_count=24,.device_count=4,.room_count=1,
  .rooms={{.name="Salon",.id=1}},.devices={
   {.id=1,.name="Shelly Plus 1PM",.kind=DSR_DEVICE_RELAY,.online=true,.output=true,.can_switch=true,.has_power=true,.has_voltage=true,.power_w=18.6,.voltage_v=232.9},
   {.id=2,.name="SPSH-002PE16EU",.kind=DSR_DEVICE_COVER,.online=true,.has_position=true,.position_pct=84,.can_cover=true,.can_stop_cover=true,.cover_state=DSR_COVER_STOPPED,.has_power=true,.has_voltage=true,.voltage_v=232.9},
   {.id=3,.name="BLU Window",.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.can_refresh=true,.has_contact=true,.has_battery=true,.battery_pct=92,.has_illuminance=true,.illuminance_lux=18,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|DSR_BLU_HAS_RSSI,.rssi=-69}},
   {.id=4,.name="Shelly Plug S",.kind=DSR_DEVICE_PLUG,.online=false}}};
 for(unsigned i=0;i<24;i++)m.tiles[i]=(dsr_mobile_tile_t){.type=0,.index=i%4,.span=2,.height=1,.fields=255};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_refr_now(display);lv_mem_monitor(&after);
 CHECK(after.free_size>4096&&after.free_biggest_size>4096);
 printf("After memory page: 24 tiles, %lu free, largest %lu bytes\n",(unsigned long)after.free_size,(unsigned long)after.free_biggest_size);
 printf("PASS: %u memory UI checks; real LVGL, synthetic SDK counters\n",checks);lv_deinit();return 0;
}
