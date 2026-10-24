/* Real LVGL cards/photos; state input is a host provider. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static unsigned room_deleted;
static void room_delete(lv_event_t *e){(void)e;room_deleted++;}
static void watch_room(lv_obj_t *card)
{
 CHECK(card);unsigned skip=0;lv_obj_t *image=nth_type(card,&lv_image_class,&skip);CHECK(image);
 lv_obj_add_event_cb(card,room_delete,LV_EVENT_DELETE,NULL);lv_obj_add_event_cb(image,room_delete,LV_EVENT_DELETE,NULL);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 dsr_mobile_model_t m={.wifi=true,.device_count=2,.room_count=4};
 for(unsigned i=0;i<4;i++){m.rooms[i].id=i+1;snprintf(m.rooms[i].name,sizeof(m.rooms[i].name),"Room %u",i+1);snprintf(m.rooms[i].photo,sizeof(m.rooms[i].photo),"%s",dsr_room_photo(0)->id);}
 for(unsigned i=0;i<2;i++){m.devices[i]=(dsr_mobile_device_t){.id=i+1,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.generation=DSR_GEN2_PLUS,.room=(uint8_t)i,.online=true,.has_power=true,.power_w=i?3:1};snprintf(m.devices[i].name,sizeof(m.devices[i].name),"Relay %u",i+1);snprintf(m.devices[i].identity,sizeof(m.devices[i].identity),"room-device-%u",i);}
 const unsigned skins[]={0,2,3};
 for(unsigned s=0;s<3;s++)for(unsigned lang=0;lang<3;lang++){
  m.configured_layout=false;m.tile_count=0;m.devices[0].room=0;m.devices[0].power_w=1;m.devices[1].power_w=3;m.devices[1].online=true;
  dsr_sc_appearance(DSR_SC_BLUE,skins[s],lang);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_ROOMS,0);lv_obj_update_layout(lv_screen_active());
  if(lang==0){char file[40];snprintf(file,sizeof(file),"rooms-grid-%u",skins[s]);screenshot(file);}
  lv_obj_t *first=lv_obj_get_parent(find_label(lv_screen_active(),"Room 1")),*second=lv_obj_get_parent(find_label(lv_screen_active(),"Room 2"));watch_room(first);watch_room(second);
  lv_obj_t *list=lv_obj_get_parent(first);lv_obj_scroll_to_y(list,130,LV_ANIM_OFF);int32_t scroll=lv_obj_get_scroll_y(list);room_deleted=0;
  m.devices[0].power_w=2;dsr_mobile_update(&m);CHECK(!room_deleted&&find_label(first,"2.0 W")&&find_label(second,"3.0 W")&&lv_obj_get_scroll_y(list)==scroll);
  char count_text[32];snprintf(count_text,sizeof(count_text),"1 %s",dsr_sc_tr("urz."));
  m.devices[1].online=false;dsr_mobile_update(&m);CHECK(!room_deleted&&find_label(second,count_text));
  snprintf(count_text,sizeof(count_text),"0 %s",dsr_sc_tr("urz."));
  m.devices[0].room=1;dsr_mobile_update(&m);CHECK(!room_deleted&&find_label(first,count_text));
  if(lang==0){char file[40];snprintf(file,sizeof(file),"rooms-refresh-%u",skins[s]);screenshot(file);}
  dsr_mobile_show(DSR_MOBILE_HOME,0);m.configured_layout=true;m.tile_count=2;m.tiles[0]=(dsr_mobile_tile_t){.type=1,.index=0,.span=1,.height=1};m.tiles[1]=(dsr_mobile_tile_t){.type=1,.index=1,.span=1,.height=1};dsr_mobile_update(&m);lv_obj_update_layout(lv_screen_active());
  first=lv_obj_get_parent(find_label(lv_screen_active(),"Room 1"));second=lv_obj_get_parent(find_label(lv_screen_active(),"Room 2"));watch_room(first);watch_room(second);room_deleted=0;
  m.devices[0].room=0;m.devices[0].power_w=4;dsr_mobile_update(&m);CHECK(!room_deleted&&find_label(first,"4.0 W"));
  snprintf(m.rooms[0].name,sizeof(m.rooms[0].name),"Renamed room");dsr_mobile_update(&m);CHECK(room_deleted&&find_label(lv_screen_active(),"Renamed room"));snprintf(m.rooms[0].name,sizeof(m.rooms[0].name),"Room 1");
 }
 /* Only representative captures: wrapping and the no-photo fallback with
  * content-sized footer frames. Refresh scenarios above keep simple names. */
 m.room_count=2;
 for(unsigned s=0;s<3;s++)for(unsigned lang=0;lang<3;lang++){
  const char *names[]={"Salon i jadalnia na parterze","Living and dining room downstairs","Wohnzimmer und Esszimmer im Erdgeschoss"};
  snprintf(m.rooms[0].name,sizeof(m.rooms[0].name),"%s",names[lang]);snprintf(m.rooms[1].name,sizeof(m.rooms[1].name),"%s",lang==0?"Gabinet i biblioteka":lang==1?"Office and library":"Arbeitszimmer und Bibliothek");m.rooms[1].photo[0]=0;
  dsr_sc_appearance(DSR_SC_BLUE,skins[s],lang);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_ROOMS,0);
  char file[48];snprintf(file,sizeof(file),"rooms-footer-names-%u-%u",skins[s],lang);screenshot(file);
 }
 printf("PASS: room and Dashboard photo objects survive telemetry/room reassignment, aggregates and scroll update, structural rename rebuilds; 3 skins x PL/EN/DE\n");lv_deinit();return 0;
}
