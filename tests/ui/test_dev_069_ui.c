/* Owner's LCD reports, real LVGL 9.3 pixels and pointer events.
 * Network/ADC/storage remain fixture providers. No claim of physical RF. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include <math.h>

static lv_area_t rect(lv_obj_t *o)
{lv_area_t a;CHECK(o);lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(o,&a);return a;}
static void shot(const char *name,unsigned theme)
{char path[96];snprintf(path,sizeof(path),"dev069-%u-%s",theme,name);screenshot(path);}
static void common(unsigned theme)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString("de"));publish_document(doc);dsr_network_home(NULL);tick();dsr_ui_open_settings();CHECK(dsr_sc_style()==theme);
 dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3902,.has_percent=true,.percent=63,.percent_estimated=true}};
 dsr_ui_battery_result(&e);
 lv_obj_t *percent=find_label(lv_screen_active(),"~63%");CHECK(percent);
 lv_area_t a=rect(percent),b=rect(lv_obj_get_parent(percent));
 CHECK(a.y1>=b.y1+8&&a.y2<=b.y1+32); /* percentage belongs INSIDE the battery */
 CHECK(a.x1>=b.x1+4&&a.x2<=b.x2-4);
 lv_obj_t *last=find_label(lv_screen_active(),"System");CHECK(last);
 lv_obj_t *row=lv_obj_get_parent(last);lv_obj_scroll_to_view(row,LV_ANIM_OFF);a=rect(row);
 CHECK(a.y2<=420&&a.x1>=12&&a.x2<=307); /* full last row, before bottom navigation */
 shot("settings-bottom",theme);
}
static lv_obj_t *containing(lv_obj_t *o,const char *text)
{
 if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o),text))return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=containing(lv_obj_get_child(o,(int)i),text);if(r)return r;}return NULL;
}
static void noop(void){}
static void stop_noop(unsigned index){(void)index;}
static void trv_noop(unsigned index,uint8_t target){(void)index;(void)target;}
static bool frame_pixel(int x,int y)
{
 uint32_t expected=lv_color_to_u32(dsr_sc_color(DSR_SC_GLOW));int distance=0;
 for(unsigned c=0;c<3;c++)distance+=abs((int)canvas[(y*320+x)*3+(int)c]-(int)((expected>>(16-c*8))&255));
 return distance<100; /* RGB565 + Cyber's translucent contour */
}
static bool centered_symbol(lv_obj_t *label)
{
 lv_area_t button=rect(lv_obj_get_parent(label)),ink={320,480,-1,-1};uint32_t color=lv_color_to_u32(lv_obj_get_style_text_color(label,0));
 for(int y=button.y1+8;y<=button.y2-8;y++)for(int x=button.x1+8;x<=button.x2-8;x++){
  int distance=0;for(unsigned c=0;c<3;c++)distance+=abs((int)canvas[(y*320+x)*3+(int)c]-(int)((color>>(16-c*8))&255));
  if(distance<70){if(x<ink.x1)ink.x1=x;if(x>ink.x2)ink.x2=x;if(y<ink.y1)ink.y1=y;if(y>ink.y2)ink.y2=y;}
 }
 bool centered=ink.x2>=ink.x1&&abs(ink.x1+ink.x2-button.x1-button.x2)<=2&&abs(ink.y1+ink.y2-button.y1-button.y2)<=2;
 if(!centered)fprintf(stderr,"Symbol %s: ink %d,%d..%d,%d; button %d,%d..%d,%d\n",lv_label_get_text(label),(int)ink.x1,(int)ink.y1,(int)ink.x2,(int)ink.y2,(int)button.x1,(int)button.y1,(int)button.x2,(int)button.y2);
 return centered;
}
typedef struct {char target[48],battery[16];lv_area_t target_area,battery_area,icon_area;const lv_font_t *font,*battery_font;bool icon;} trv_readouts_t;
static void capture_trv_draw(lv_event_t *event)
{
 trv_readouts_t *r=lv_event_get_user_data(event);lv_draw_task_t *task=lv_event_get_draw_task(event);
 lv_draw_label_dsc_t *label=lv_draw_task_get_label_dsc(task);
 if(label){
  if(strstr(label->text,"°C")){snprintf(r->target,sizeof(r->target),"%s",label->text);lv_draw_task_get_area(task,&r->target_area);r->font=label->font;}
  else{snprintf(r->battery,sizeof(r->battery),"%s",label->text);lv_draw_task_get_area(task,&r->battery_area);r->battery_font=label->font;}
 }
 lv_draw_image_dsc_t *image=lv_draw_task_get_image_dsc(task);
 if(image){const lv_image_dsc_t *src=image->src;r->icon=true;lv_draw_task_get_area(task,&r->icon_area);r->icon_area.x2=r->icon_area.x1+(int)(src->header.w*image->scale_x/256)-1;r->icon_area.y2=r->icon_area.y1+(int)(src->header.h*image->scale_y/256)-1;}
}
static lv_obj_t *capture_trv(lv_obj_t *card,trv_readouts_t *r)
{
 lv_obj_t *frame=NULL;for(unsigned i=0;i<lv_obj_get_child_count(card);i++){lv_obj_t *o=lv_obj_get_child(card,(int)i);if(lv_obj_get_width(o)==104&&lv_obj_get_height(o)==24)frame=o;}
 CHECK(frame);memset(r,0,sizeof(*r));lv_obj_add_flag(card,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);lv_obj_add_flag(frame,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
 lv_obj_add_event_cb(card,capture_trv_draw,LV_EVENT_DRAW_TASK_ADDED,r);lv_obj_add_event_cb(frame,capture_trv_draw,LV_EVENT_DRAW_TASK_ADDED,r);
 lv_obj_invalidate(card);lv_refr_now(display);
 lv_obj_remove_event_cb(card,capture_trv_draw);lv_obj_remove_event_cb(frame,capture_trv_draw);lv_obj_remove_flag(card,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);lv_obj_remove_flag(frame,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
 return frame;
}
static void thermostat_card(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);
 dsr_mobile_actions_t actions={.add_device=noop,.edit_dashboard=noop,.select_dashboard=noop,.set_trv=trv_noop};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard im Erdgeschoss",.room_count=1,.rooms={{.id=1,.name="Küche und Esszimmer"}},.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=2,.identity="dev069-trv",.name="Thermostat Schlafzimmer",.room=0,.kind=DSR_DEVICE_SENSOR,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_temperature=true,.temperature_c=21.5,.has_trv_target=true,.trv_target_c=21,.has_battery=true,.battery_pct=64,.blu_age_s=1,.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET|DSR_BLU_TRV_CHECKED}};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);
 lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));lv_area_t a=rect(card);
 lv_obj_t *minus=find_label(card,"-"),*plus=find_label(card,"+");CHECK(minus&&plus);
 CHECK(lv_obj_get_width(lv_obj_get_parent(minus))>=44&&lv_obj_get_height(lv_obj_get_parent(plus))>=44);
 lv_area_t controls=rect(lv_obj_get_parent(lv_obj_get_parent(minus)));
 CHECK(lv_area_get_width(&controls)==104&&lv_area_get_height(&controls)==56);
 CHECK(controls.x1>=a.x1+8&&controls.x2<a.x2-5&&controls.y1>rect(find_label(card,m.devices[0].name)).y2);
 trv_readouts_t readout;lv_obj_t *frame=capture_trv(card,&readout);CHECK(strstr(readout.target,"21.0 °C")&&!strstr(readout.target,"Solltemperatur"));
 bool minus_centered=centered_symbol(minus),plus_centered=centered_symbol(plus);CHECK(minus_centered&&plus_centered);
 CHECK(strstr(readout.target,dsr_sc_tr("Nastawa")));if(language==2)CHECK(!strcmp(readout.target,"Soll 21.0 °C"));
 lv_area_t f=rect(frame),t=readout.target_area;lv_point_t text_size;
 lv_text_get_size(&text_size,readout.target,readout.font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 CHECK(lv_area_get_width(&t)>=text_size.x&&lv_area_get_height(&t)==text_size.y);
 CHECK(t.x1>=f.x1+4&&t.x2<=f.x2-4&&t.y1>=f.y1+3&&t.y2<=f.y2-3);
 CHECK(f.y1-controls.y2>=4&&a.y2-f.y2>=12&&f.x1==controls.x1&&f.x2==controls.x2);
 CHECK(readout.icon&&lv_area_get_width(&readout.icon_area)==24&&lv_area_get_height(&readout.icon_area)==24);
 CHECK(!strcmp(readout.battery,"64%"));lv_area_t b=readout.battery_area;
 CHECK(readout.icon_area.x2<b.x1-4&&readout.icon_area.y1==f.y1&&readout.icon_area.y2==f.y2);
 CHECK(b.x1>=a.x1+40&&b.x2<=f.x1-12&&b.y1>=f.y1&&b.y2<=f.y2&&a.y2-b.y2>=12);
 lv_obj_t *reading=containing(card,"21.5 °C");CHECK(reading);
 lv_area_t reading_area=rect(reading),reading_frame=rect(lv_obj_get_parent(reading));
 CHECK(abs(reading_area.x1+reading_area.x2-reading_frame.x1-reading_frame.x2)<=1);
 CHECK(abs(reading_area.y1+reading_area.y2-reading_frame.y1-reading_frame.y2)<=1);
 CHECK(lv_obj_get_height(reading)==lv_font_get_line_height(lv_obj_get_style_text_font(reading,0)));
 lv_text_get_size(&text_size,readout.battery,readout.battery_font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 CHECK(lv_area_get_width(&b)==text_size.x); /* no empty tail after the percentage */
 int frame_left=readout.icon_area.x1-6,frame_right=b.x2+6;
 CHECK(abs(frame_left+frame_right-reading_frame.x1-reading_frame.x2)<=1);
 int inset=theme==3?1:0,mid_x=(frame_left+frame_right)/2,mid_y=(f.y1+f.y2)/2;
 CHECK(frame_pixel(mid_x,f.y1+inset)&&frame_pixel(mid_x,f.y2-inset));
 CHECK(frame_pixel(frame_left+inset,mid_y)&&frame_pixel(frame_right-inset,mid_y));
 CHECK(frame_right-frame_left+1<92&&!frame_pixel(frame_left+91-inset,mid_y));
 CHECK(containing(card,"21.5 °C")&&lv_obj_get_height(card)<=170&&!containing(card,"Ventil"));
 char name[48];snprintf(name,sizeof(name),"dashboard-trv-%u",language);shot(name,theme);
 if(language==2){
  /* The same symbol helper also owns the detail page's 48 px controls. */
  dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_refr_now(display);
  CHECK(centered_symbol(find_label(lv_screen_active(),"-"))&&centered_symbol(find_label(lv_screen_active(),"+")));
  dsr_mobile_show(DSR_MOBILE_HOME,0);
 }
 /* Device battery, not the remote's status-bar estimate; redraw on new readings. */
 const double levels[]={0,100,NAN,-1,101};
 for(unsigned k=0;k<sizeof(levels)/sizeof(levels[0]);k++){
  m.devices[0].battery_pct=levels[k];dsr_mobile_update(&m);card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));
  capture_trv(card,&readout);CHECK(!strcmp(readout.battery,k==0?"0%":k==1?"100%":"—"));
  lv_text_get_size(&text_size,readout.battery,readout.battery_font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);CHECK(lv_area_get_width(&readout.battery_area)==text_size.x);
  CHECK(abs(readout.icon_area.x1-6+readout.battery_area.x2+6-reading_frame.x1-reading_frame.x2)<=1);
 }
 m.devices[0].has_battery=false;dsr_mobile_update(&m);card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));capture_trv(card,&readout);CHECK(!strcmp(readout.battery,"—")&&readout.icon);
 if(theme==3&&language==2)shot("dashboard-trv-battery-unknown",theme);
 m.devices[0].has_battery=true;m.devices[0].battery_pct=64;m.devices[0].online=false;dsr_mobile_update(&m);card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));capture_trv(card,&readout);CHECK(!strcmp(readout.battery,"—"));
 if(theme==3&&language==2){
  m.devices[0].online=true;m.device_count=DSR_UI_MAX_DEVICES;
  for(unsigned i=1;i<m.device_count;i++){m.devices[i]=m.devices[0];m.devices[i].id=10+i;snprintf(m.devices[i].name,sizeof(m.devices[i].name),"Thermostat %u",i);}
  dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_refr_now(display);
  lv_mem_monitor_t memory;lv_mem_monitor(&memory);CHECK(memory.free_size>4096&&memory.free_biggest_size>4096);
  printf("Thermostat dashboard: %u devices, %lu bytes free, largest %lu; unchanged 128 KiB LVGL pool\n",m.device_count,(unsigned long)memory.free_size,(unsigned long)memory.free_biggest_size);
 }
}
typedef struct {char room[96],lux[40],battery[16];lv_area_t room_area,lux_area,battery_area,icons[2];const lv_font_t *room_font,*lux_font,*battery_font;unsigned icon_count;} contact_content_t;
static lv_obj_t *contact_graphic(lv_obj_t *card)
{
 for(unsigned i=0;i<lv_obj_get_child_count(card);i++){lv_obj_t *o=lv_obj_get_child(card,(int)i);if(lv_obj_get_width(o)==84&&lv_obj_get_height(o)==64)return o;}return NULL;
}
static void contact_content_drawn(lv_event_t *event)
{
 contact_content_t *r=lv_event_get_user_data(event);lv_draw_task_t *task=lv_event_get_draw_task(event);lv_draw_label_dsc_t *label=lv_draw_task_get_label_dsc(task);
 if(label){
  if(strstr(label->text," lx")){snprintf(r->lux,sizeof(r->lux),"%s",label->text);r->lux_font=label->font;lv_draw_task_get_area(task,&r->lux_area);}
  else if(strchr(label->text,'%')||!strcmp(label->text,"—")){snprintf(r->battery,sizeof(r->battery),"%s",label->text);r->battery_font=label->font;lv_draw_task_get_area(task,&r->battery_area);}
  else{snprintf(r->room,sizeof(r->room),"%s",label->text);r->room_font=label->font;lv_draw_task_get_area(task,&r->room_area);}
 }
 lv_draw_image_dsc_t *image=lv_draw_task_get_image_dsc(task);
 if(image&&r->icon_count<2){const lv_image_dsc_t *src=image->src;lv_area_t *a=&r->icons[r->icon_count++];lv_draw_task_get_area(task,a);a->x2=a->x1+(int)(src->header.w*image->scale_x/256)-1;a->y2=a->y1+(int)(src->header.h*image->scale_y/256)-1;}
}
static void contact_content_check(lv_obj_t *card,const char *room,const char *lux,const char *battery)
{
 contact_content_t r={0};lv_obj_add_flag(card,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);lv_obj_add_event_cb(card,contact_content_drawn,LV_EVENT_DRAW_TASK_ADDED,&r);
 lv_obj_invalidate(card);lv_refr_now(display);lv_obj_remove_event_cb(card,contact_content_drawn);lv_obj_remove_flag(card,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
 CHECK(!strcmp(r.room,room)&&!strcmp(r.lux,lux)&&!strcmp(r.battery,battery));CHECK(r.icon_count==2&&r.room_font&&r.lux_font&&r.battery_font);
 lv_area_t a=rect(card),title=rect(lv_obj_get_child(card,0)),thumb=rect(lv_obj_get_child(card,1)),graphic=rect(contact_graphic(card));
 CHECK(r.room_area.x1>=thumb.x2+8&&r.room_area.x2<=graphic.x1-4&&r.room_area.y1-title.y2>=4);
 CHECK(r.lux_area.y1-r.room_area.y2>=4&&r.battery_area.y1-r.lux_area.y2>=6&&a.y2-r.battery_area.y2>=12);
 lv_point_t size;lv_text_get_size(&size,room,r.room_font,0,0,lv_area_get_width(&r.room_area),LV_TEXT_FLAG_NONE);CHECK(lv_area_get_height(&r.room_area)>=size.y);
 const lv_area_t values[]={r.lux_area,r.battery_area};const char *texts[]={lux,battery};const lv_font_t *fonts[]={r.lux_font,r.battery_font};
 for(unsigned i=0;i<2;i++){
  CHECK(lv_area_get_width(&r.icons[i])==24&&lv_area_get_height(&r.icons[i])==24&&r.icons[i].x2<values[i].x1-4);
  CHECK(r.icons[i].x1-6>=r.room_area.x1&&values[i].x2+6<=r.room_area.x2);
  CHECK(abs(r.icons[i].x1-6+values[i].x2+6-r.room_area.x1-r.room_area.x2)<=1);
  lv_text_get_size(&size,texts[i],fonts[i],0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);CHECK(lv_area_get_width(&values[i])>=size.x&&lv_area_get_height(&values[i])>=size.y);
 }
}
static void contact_cards(unsigned theme)
{
 dsr_ui_open_settings();dsr_mobile_actions_t actions={.add_device=noop,.edit_dashboard=noop,.select_dashboard=noop};dsr_mobile_bind(&actions);
 unsigned door=0;for(unsigned k=0;k<DSR_SC_DEVICE_ICONS;k++)if(dsr_sc_device_icons[k]==DSR_ICON_DOOR)door=k+1;CHECK(door);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard",.room_count=1,.rooms={{.id=1,.name="Schlafzimmer"}},.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=3,.identity="contact-review",.room=0,.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_contact=true,.has_illuminance=true,.illuminance_lux=0,.has_battery=true,.battery_pct=92,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN}};
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,theme,language);
  for(unsigned kind=0;kind<2;kind++)for(unsigned open=0;open<2;open++){
   snprintf(m.devices[0].name,sizeof(m.devices[0].name),"%s",kind?"Tür Schlafzimmer":"Fenster Schlafzimmer");
   m.devices[0].icon=kind?door:0;m.devices[0].contact_open=open;m.devices[0].blu.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|(open?DSR_BLU_OPEN:0);
   dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);
   lv_obj_t *name=find_label(lv_screen_active(),m.devices[0].name),*card=lv_obj_get_parent(name),*box=contact_graphic(card);
   lv_area_t a=rect(card),b=rect(box);CHECK(b.y1>rect(name).y2&&a.y2-b.y2>=12&&a.x2-b.x2>=8);
   contact_content_check(card,m.rooms[0].name,"0 lx","92%");
   CHECK(has_icon(box,kind?(open?DSR_ICON_DOOR_OPEN:DSR_ICON_DOOR_CLOSED):(open?DSR_ICON_WINDOW_OPEN:DSR_ICON_WINDOW_CLOSED),52));
   CHECK(!find_label(box,open?"Otwarte":"Zamknięte"));
   CHECK(!find_label(card,"Brak dodatkowych pomiarów"));
   if(language==2){char path[48];snprintf(path,sizeof(path),"contact-%s-%s",kind?"door":"window",open?"open":"closed");shot(path,theme);}
  }
 }
 m.devices[0].has_battery=true;m.devices[0].battery_pct=92;m.devices[0].has_illuminance=true;m.devices[0].illuminance_lux=0;
 snprintf(m.devices[0].name,sizeof(m.devices[0].name),"Terrassentür im großen Wohnzimmer");snprintf(m.rooms[0].name,sizeof(m.rooms[0].name),"Wohnzimmer und offene Küche im Erdgeschoss");
 dsr_mobile_update(&m);contact_content_check(lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name)),m.rooms[0].name,"0 lx","92%");shot("contact-long-name-readings",theme);
 memset(m.rooms[0].name,'W',sizeof(m.rooms[0].name)-1);m.rooms[0].name[sizeof(m.rooms[0].name)-1]=0;dsr_mobile_update(&m);contact_content_check(lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name)),m.rooms[0].name,"0 lx","92%");
 m.devices[0].has_battery=m.devices[0].has_illuminance=false;dsr_mobile_update(&m);contact_content_check(lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name)),m.rooms[0].name,"— lx","—");
 m.devices[0].online=false;m.devices[0].has_sample_time=true;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),theme==3?"Brak danych":"Odczyt nieaktualny"));CHECK(!contact_graphic(lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name))));
 if(theme==3){
  m.devices[0].online=true;m.devices[0].has_battery=m.devices[0].has_illuminance=true;m.devices[0].battery_pct=100;m.device_count=DSR_UI_MAX_DEVICES;
  snprintf(m.rooms[0].name,sizeof(m.rooms[0].name),"Schlafzimmer");
  for(unsigned i=1;i<m.device_count;i++){m.devices[i]=m.devices[0];m.devices[i].id=30+i;snprintf(m.devices[i].name,sizeof(m.devices[i].name),"Fenster %u",i);}
  dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_refr_now(display);lv_mem_monitor_t memory;lv_mem_monitor(&memory);CHECK(memory.free_size>4096&&memory.free_biggest_size>4096);
  printf("Contact dashboard: %u devices, %lu bytes free, largest %lu; unchanged 128 KiB LVGL pool\n",m.device_count,(unsigned long)memory.free_size,(unsigned long)memory.free_biggest_size);
 }
}
static void dashboard(unsigned theme)
{
 dsr_ui_open_settings();dsr_mobile_actions_t actions={.add_device=noop,.edit_dashboard=noop,.select_dashboard=noop,.set_trv=trv_noop,.stop_cover=stop_noop};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard im Erdgeschoss",.room_count=1,.rooms={{.id=1,.name="Küche und Esszimmer"}},.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=1,.identity="dev069-relay",.name="Deckenbeleuchtung Küche",.room=0,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true,.can_switch=true,.has_power=true,.power_w=0,.has_voltage=true,.voltage_v=231.8,.fields=255};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);lv_obj_update_layout(lv_screen_active());
 lv_obj_t *name=find_label(lv_screen_active(),m.dashboard_name);CHECK(name);lv_area_t selector=rect(lv_obj_get_parent(name));CHECK(selector.y1<60&&selector.x2<170);
 lv_obj_t *measurement=containing(lv_screen_active(),"231.8 V");CHECK(measurement&&strstr(lv_label_get_text(measurement),"0.0 W")&&!strstr(lv_label_get_text(measurement),"..."));
 lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));lv_area_t a=rect(card),v=rect(lv_obj_get_parent(measurement));
 CHECK(v.x1>=a.x1+60&&v.x2<=a.x2-80&&v.y2<a.y2-5&&v.y1<a.y1+100);
 lv_obj_t *primary=lv_obj_get_child(card,3);CHECK(lv_obj_has_flag(primary,LV_OBJ_FLAG_CLICKABLE)&&lv_obj_get_width(primary)>=44);v=rect(primary);CHECK(a.y2-v.y2>=12);shot("dashboard-relay",theme);
 m.devices[0].power_w=12345.6;dsr_mobile_update(&m);measurement=containing(lv_screen_active(),"231.8 V");CHECK(measurement&&strstr(lv_label_get_text(measurement),"12345.6 W"));
 thermostat_card(theme,2);
 dsr_mobile_bind(&actions);
 m.devices[0]=(dsr_mobile_device_t){.id=3,.identity="dev069-window",.name="Fenster Schlafzimmer",.room=0,.kind=DSR_DEVICE_CONTACT,.endpoint_kind=DSR_ENDPOINT_BLU,.online=true,.has_contact=true,.contact_open=true,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|DSR_BLU_OPEN}};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);CHECK(!find_label(lv_screen_active(),"Brak dodatkowych pomiarów"));card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));lv_obj_t *box=contact_graphic(card);a=rect(card);v=rect(box);CHECK(lv_obj_get_height(box)>=56&&lv_obj_get_height(card)<=156&&v.y1>a.y1+24);CHECK(has_icon(box,DSR_ICON_WINDOW_OPEN,52)&&!find_label(box,"Otwarte"));CHECK(a.y2-v.y2>=12);shot("dashboard-window",theme);
 dsr_mobile_show(DSR_MOBILE_DEVICES,0);shot("devices-tabs",theme);
 /* Both states and both owner-selected contact icons share the same graphic tile. */
 unsigned door=0;for(unsigned k=0;k<DSR_SC_DEVICE_ICONS;k++)if(dsr_sc_device_icons[k]==DSR_ICON_DOOR)door=k+1;CHECK(door);
 for(unsigned kind=0;kind<2;kind++)for(unsigned open=0;open<2;open++){
  snprintf(m.devices[0].name,sizeof(m.devices[0].name),"%s",kind?"Tür Schlafzimmer":"Fenster Schlafzimmer");
  m.devices[0].icon=kind?door:0;m.devices[0].contact_open=open;
  m.devices[0].blu.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN|(open?DSR_BLU_OPEN:0);
  dsr_mobile_update(&m);card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name));box=contact_graphic(card);
  CHECK(has_icon(box,kind?(open?DSR_ICON_DOOR_OPEN:DSR_ICON_DOOR_CLOSED):(open?DSR_ICON_WINDOW_OPEN:DSR_ICON_WINDOW_CLOSED),52));
  CHECK(!find_label(box,open?"Otwarte":"Zamknięte"));
  if(theme==3){char state_shot[40];snprintf(state_shot,sizeof(state_shot),"%s-%s",kind?"door":"window",open?"open":"closed");shot(state_shot,theme);}
 }
 m.devices[0].has_illuminance=true;m.devices[0].illuminance_lux=0;strcpy(m.devices[0].name,"Deckenbeleuchtung im großen Wohnzimmer");dsr_mobile_update(&m);contact_content_check(lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name)),m.rooms[0].name,"0 lx","—");shot("window-long-name-real-reading",theme);
 m.devices[0]=(dsr_mobile_device_t){.id=4,.identity="dev069-moving-cover",.name="Rollladen Terrasse",.kind=DSR_DEVICE_COVER,.endpoint_kind=DSR_ENDPOINT_COVER,.online=true,.can_stop_cover=true,.cover_state=DSR_COVER_OPENING};
 dsr_mobile_update(&m);lv_obj_t *stop=lv_obj_get_parent(find_label(lv_screen_active(),"Stop"));CHECK(stop);a=rect(stop);v=rect(lv_obj_get_parent(stop));CHECK(lv_area_get_height(&a)>=44&&a.x1-v.x1>=3&&v.x2-a.x2>=3&&a.y1-v.y1>=3&&v.y2-a.y2>=3);
}
static void access_point(unsigned theme)
{
 wifi.connected=false;dsr_network_home(NULL);tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 wifi.connected=true;dsr_ui_ap_open(NULL);
 lv_obj_update_layout(lv_screen_active());
 lv_obj_t *battery=NULL;for(unsigned i=0;i<lv_obj_get_child_count(lv_screen_active());i++){lv_obj_t *o=lv_obj_get_child(lv_screen_active(),(int)i);if(lv_obj_get_x(o)==256)battery=o;}
 CHECK(battery&&lv_obj_is_visible(battery)&&lv_obj_get_child_count(battery)==1);
 char step[40];snprintf(step,sizeof(step),dsr_sc_tr("Krok %u z 3"),2u);lv_obj_t *progress=find_label(lv_screen_active(),step);CHECK(progress&&rect(progress).y1>=70);lv_obj_t *scan_button=lv_obj_get_parent(find_label(lv_screen_active(),"Szukaj AP"));CHECK(rect(scan_button).y2<=463);
 shot("ap-intro",theme);click("Szukaj AP");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN);shot("ap-scanning",theme);
 response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Nie znaleziono fabrycznego AP Shelly. Włącz tryb konfiguracji urządzenia."));shot("ap-empty",theme);
 click("Ponów");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Ponów"));lv_obj_t *notice=find_label(lv_screen_active(),"Nie potwierdzono dodania. Domowe ustawienia pilota zachowane.");CHECK(notice);lv_obj_t *frame=lv_obj_get_parent(notice);CHECK(lv_obj_get_width(frame)==296&&lv_obj_get_x(notice)>=12);if(theme==3)CHECK(lv_obj_get_style_border_width(frame,0)>=2);shot("ap-error",theme);
 unsigned writes=job_counts[DSR_JOB_AP_PROVISION];click("Ponów");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN&&job_counts[DSR_JOB_AP_PROVISION]==writes);response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);click("Anuluj");
 ap_pick();shot("ap-connect-consent",theme);ap_disclose();shot("ap-share-consent",theme);ap_cancel_finish();CHECK(job_counts[DSR_JOB_AP_PROVISION]==writes&&dsr_sc_style()==theme);
}
static unsigned position_writes,position_target;
static void position_set(unsigned index,uint8_t target){CHECK(index==0);position_writes++;position_target=target;}
static void cover_set(unsigned index,int direction){CHECK(index==0&&(direction==1||direction==-1));}
static void pointer_move(int x,int y,bool down)
{touch=(lv_point_t){x,y};pressed=down;lv_tick_inc(20);lv_indev_read(pointer);lv_timer_handler();lv_refr_now(display);}
static void cover_room_check(const dsr_mobile_model_t *m,const char *room)
{
 lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),m->devices[0].name)),*label=containing(card,room);CHECK(label&&!strcmp(lv_label_get_text(label),room));
 lv_point_t size;lv_text_get_size(&size,room,lv_obj_get_style_text_font(label,0),0,0,lv_obj_get_width(label),LV_TEXT_FLAG_NONE);
 CHECK(lv_label_get_long_mode(label)==LV_LABEL_LONG_MODE_WRAP&&lv_obj_get_height(label)>=size.y);
 lv_obj_t *position=containing(card,"50%");CHECK(position);lv_area_t r=rect(label),p=rect(lv_obj_get_parent(position)),c=rect(card);
 CHECK(r.x1>=c.x1+76&&r.x2<=c.x2-116&&p.y1>=r.y2+6&&c.y2-p.y2>=12);
 CHECK(!find_label(card,"Zatrzymano")&&!find_label(card,"Otwieranie")&&!find_label(card,"Zamykanie"));
}
static void cover_cards(unsigned theme)
{
 dsr_ui_open_settings();dsr_mobile_actions_t a={.set_cover=cover_set,.stop_cover=stop_noop};dsr_mobile_bind(&a);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard",.room_count=1,.rooms={{.id=1,.name="Wohnzimmer"}},.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=4,.identity="dev069-cover",.name="Rollladen Terrasse",.kind=DSR_DEVICE_COVER,.endpoint_kind=DSR_ENDPOINT_COVER,.online=true,.can_cover=true,.can_stop_cover=true,.has_position=true,.position_pct=50};
 const uint8_t states[]={DSR_COVER_STOPPED,DSR_COVER_OPENING,DSR_COVER_CLOSING};
 for(unsigned language=0;language<3;language++){
  dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);
  for(unsigned k=0;k<3;k++){m.devices[0].cover_state=states[k];dsr_mobile_update(&m);cover_room_check(&m,m.rooms[0].name);if(language==2&&k<2)shot(k?"cover-card-moving":"cover-card-stopped",theme);}
 }
 snprintf(m.rooms[0].name,sizeof(m.rooms[0].name),"Schlafzimmer im oberen Stockwerk");dsr_mobile_update(&m);cover_room_check(&m,m.rooms[0].name);shot("cover-card-long-room",theme);
 memset(m.rooms[0].name,'W',sizeof(m.rooms[0].name)-1);m.rooms[0].name[sizeof(m.rooms[0].name)-1]=0;dsr_mobile_update(&m);cover_room_check(&m,m.rooms[0].name);
 m.devices[0].room=DSR_UI_NO_ROOM;dsr_mobile_update(&m);cover_room_check(&m,dsr_sc_tr("Nieprzypisane"));
 m.devices[0].online=false;m.devices[0].has_sample_time=true;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Odczyt nieaktualny"));
 m.devices[0].online=true;m.devices[0].error=DSR_AUTH_REQUIRED;dsr_mobile_update(&m);
 CHECK(find_label(lv_screen_active(),"Błąd logowania"));
}
static void cover_animation(unsigned theme,const char *direction)
{
 unsigned previous=0;
 for(unsigned frame=0;frame<=6;frame++){
  if(frame){lv_tick_inc(50);lv_timer_handler();}
  char name[48];snprintf(name,sizeof(name),"cover-%s-%u",direction,frame);shot(name,theme);
  unsigned signature=0;for(int y=202;y<310;y++)for(int x=80;x<225;x++)for(int c=0;c<3;c++)signature=signature*33+canvas[(y*320+x)*3+c];
  if(frame)CHECK(signature!=previous);
  previous=signature;
 }
}
static void cover_skin(unsigned theme)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,2);dsr_mobile_actions_t a={.set_position=position_set,.set_cover=cover_set,.stop_cover=stop_noop};dsr_mobile_bind(&a);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard",.room_count=1,.rooms={{.id=1,.name="Wohnzimmer"}},.device_count=1};m.devices[0]=(dsr_mobile_device_t){.id=4,.identity="dev069-cover",.name="Rollladen Terrasse",.kind=DSR_DEVICE_COVER,.endpoint_kind=DSR_ENDPOINT_COVER,.online=true,.can_cover=true,.can_stop_cover=true,.can_position=true,.has_position=true,.position_pct=50,.cover_state=DSR_COVER_STOPPED,.has_power=true,.power_w=0,.has_voltage=true,.voltage_v=231.8};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_t *title=find_label(lv_screen_active(),m.devices[0].name);CHECK(title&&lv_obj_get_y(lv_obj_get_parent(title))==52&&lv_obj_get_style_text_align(title,0)==LV_TEXT_ALIGN_CENTER);CHECK(!has_icon(lv_screen_active(),DSR_ICON_EDIT,24));CHECK(rect(lv_obj_get_parent(find_label(lv_screen_active(),"Otwórz"))).y2<=DSR_SC_NAV_TOP-17);shot("cover-stopped",theme);
 char confirmed[40];snprintf(confirmed,sizeof(confirmed),"%s 50%%",dsr_sc_tr("Pozycja"));
 unsigned before=position_writes;pointer_move(150,226,true);CHECK(position_writes==before);CHECK(find_label(lv_screen_active(),confirmed));shot("cover-preview",theme);pointer_move(150,226,false);CHECK(position_writes==before+1&&position_target>=75&&position_target<=85);
 m.devices[0].position_pct=80;m.devices[0].cover_state=DSR_COVER_OPENING;dsr_mobile_update(&m);
 if(getenv("DSR_UI_FOCUS_COVER")){
  cover_animation(theme,"opening");snprintf(confirmed,sizeof(confirmed),"%s 80%%",dsr_sc_tr("Pozycja"));CHECK(find_label(lv_screen_active(),confirmed));
  m.devices[0].position_pct=20;m.devices[0].cover_state=DSR_COVER_CLOSING;dsr_mobile_update(&m);cover_animation(theme,"closing");snprintf(confirmed,sizeof(confirmed),"%s 20%%",dsr_sc_tr("Pozycja"));CHECK(find_label(lv_screen_active(),confirmed));
 }else{lv_tick_inc(150);lv_timer_handler();}
 shot("cover-moving",theme);
 m.devices[0].has_position=false;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Nieznana pozycja"));shot("cover-unknown",theme);
 if(getenv("DSR_UI_FOCUS_COVER")){
  m.devices[0].has_position=true;m.devices[0].position_pct=50;m.devices[0].cover_state=DSR_COVER_STOPPED;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);shot("cover-card-stopped",theme);
  m.devices[0].cover_state=DSR_COVER_OPENING;m.devices[0].power_w=96.2;dsr_mobile_update(&m);shot("cover-card-moving",theme);
 }
}
static void onboarding_labels(void)
{
 const uint8_t blu[]={6,0xff,0xa9,0x0b,0x0b,2,0},unknown[]={6,0x16,0xd2,0xfc,0x40,1,92};
 for(unsigned lang=0;lang<3;lang++){
  dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,3,lang);dsr_ui_ble_scan_open(NULL);
  CHECK(find_label(lv_screen_active(),"Wyszukiwanie urządzeń Bluetooth…"));
  test_ble_packet(blu,sizeof(blu),2);test_ble_packet(unknown,sizeof(unknown),3);test_ble_finish(DSR_BLE_DONE);tick();
  CHECK(find_label(lv_screen_active(),"Pozostałe urządzenia"));CHECK(!containing(lv_screen_active(),"Werbung")&&!containing(lv_screen_active(),"reklam"));
  char name[40];snprintf(name,sizeof(name),"ble-devices-%u",lang);shot(name,3);
  click("Shelly BLU DoorWindow");CHECK(find_label(lv_screen_active(),"Wykryto • nie sparowano"));
  CHECK(find_label(lv_screen_active(),"Shelly BLU • urządzenie BLE"));
  lv_obj_t *signal=containing(lv_screen_active(),"RSSI"),*device_type=find_label(lv_screen_active(),"Shelly BLU • czujnik/przycisk");CHECK(signal&&device_type);
  lv_point_t signal_size;lv_text_get_size(&signal_size,lv_label_get_text(signal),lv_obj_get_style_text_font(signal,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);CHECK(lv_obj_get_width(signal)>=signal_size.x);CHECK(rect(lv_obj_get_parent(signal)).y2-rect(signal).y2>=3);
  CHECK(rect(lv_obj_get_parent(signal)).y2<rect(lv_obj_get_parent(device_type)).y1);
  snprintf(name,sizeof(name),"ble-device-%u",lang);shot(name,3);
  click("Wstecz");click("Nierozpoznane urządzenie");signal=containing(lv_screen_active(),"RSSI");device_type=find_label(lv_screen_active(),"BTHome v2 • jawne");CHECK(signal&&device_type);CHECK(rect(lv_obj_get_parent(signal)).y2<rect(lv_obj_get_parent(device_type)).y1);
  CHECK(find_label(lv_screen_active(),"Nie potwierdzono modelu ani RPC. Tego urządzenia nie można dodać do Wi-Fi przez BLE."));
  snprintf(name,sizeof(name),"ble-unknown-%u",lang);shot(name,3);
  dsr_network_device(NULL);snprintf(name,sizeof(name),"add-methods-%u",lang);shot(name,3);choose_method("Dodaj ręcznie (IP)");CHECK(find_type(lv_screen_active(),&lv_textarea_class));
  snprintf(name,sizeof(name),"manual-form-%u",lang);shot(name,3);
 }
}
static void onboarding_review(unsigned theme)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString("de"));publish_document(doc);dsr_network_home(NULL);tick();CHECK(dsr_sc_style()==theme);
 dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3902,.has_percent=true,.percent=63,.percent_estimated=true}};dsr_ui_battery_result(&e);
 dsr_network_device(NULL);shot("add-methods",theme);choose_method("Dodaj ręcznie (IP)");shot("manual-ip",theme);
 access_point(theme);
 wifi.connected=true;dsr_network_home(NULL);tick();dsr_ui_lan_open(NULL);shot("lan-scanning",theme);
 test_lan_found("{\"gen\":2,\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-001X16EU\",\"name\":\"Shelly Plus 1\",\"auth_en\":false}","192.0.2.20");dsr_ui_lan_tick();CHECK(find_label(lv_screen_active(),"Shelly Plus 1"));shot("lan-results",theme);
 click("Shelly Plus 1");test_lan_inspected(DSR_OK,"{\"switch:0\":{\"id\":0,\"output\":false}}");dsr_ui_lan_tick();CHECK(find_label(lv_screen_active(),"Nazwa i pomieszczenie"));shot("lan-configure",theme);
 dsr_network_home(NULL);tick();
}
static void method_selection_check(int selected)
{
 const char *names[]={"Bluetooth","Wi-Fi AP","Sieć lokalna"};
 for(unsigned i=0;i<3;i++){
  lv_obj_t *row=lv_obj_get_parent(find_label(lv_screen_active(),names[i])),*radio=NULL;
  for(unsigned k=0;k<lv_obj_get_child_count(row);k++){lv_obj_t *child=lv_obj_get_child(row,(int)k);if(lv_obj_get_width(child)==18&&lv_obj_get_height(child)==18)radio=child;}
  lv_area_t a=rect(radio);lv_refr_now(display);int x=(a.x1+a.x2)/2,y=(a.y1+a.y2)/2;bool white=true;
  for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)for(int c=0;c<3;c++)if(canvas[((y+dy)*320+x+dx)*3+c]<245)white=false;
  CHECK(white==((int)i==selected));
 }
}
static void onboarding_next(unsigned theme,unsigned language)
{
 wifi.connected=true; /* AP/LAN choices need the fixture's home connection. */
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));publish_document(doc);dsr_network_home(NULL);tick();
 dsr_network_device(NULL);lv_obj_t *label=find_label(lv_screen_active(),"Dalej"),*button=lv_obj_get_parent(label);CHECK(label&&lv_obj_has_state(button,LV_STATE_DISABLED));
 unsigned scans=test_ble_scans(),jobs=requests;click("Dalej");CHECK(test_ble_scans()==scans&&requests==jobs&&dsr_ui_onboarding_active());
 method_selection_check(-1);
 if(language==2)shot("next-disabled",theme);
 click("Bluetooth");CHECK(!lv_obj_has_state(button,LV_STATE_DISABLED)&&test_ble_scans()==scans&&requests==jobs);
 lv_obj_t *reference=lv_obj_create(NULL),*save=dsr_ui_button(reference,"Zapisz",0,0,140,NULL,NULL);dsr_ui_primary(save);
 CHECK(lv_color_eq(lv_obj_get_style_bg_color(button,0),lv_obj_get_style_bg_color(save,0))&&lv_color_eq(lv_obj_get_style_bg_grad_color(button,0),lv_obj_get_style_bg_grad_color(save,0))&&lv_obj_get_style_bg_grad_dir(button,0)==lv_obj_get_style_bg_grad_dir(save,0));
 lv_obj_delete(reference);char name[40];snprintf(name,sizeof(name),"next-selected-%u",language);if(language==2||theme==3)shot(name,theme);
 CHECK(has_icon(button,DSR_ICON_NEXT,theme==3?24:28));lv_area_t b=rect(button),t=rect(label),icon=rect(lv_obj_get_child(button,-1));lv_point_t size;lv_text_get_size(&size,lv_label_get_text(label),lv_obj_get_style_text_font(label,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);CHECK(size.x<=lv_obj_get_width(label)&&t.x1>=b.x1+4&&t.x2+4<icon.x1&&icon.x2<=b.x2-8&&abs(icon.y1+icon.y2-b.y1-b.y2)<=2&&b.y2<=463);
 const char *methods[]={"Bluetooth","Wi-Fi AP","Sieć lokalna"};
 for(unsigned i=0;i<3;i++){click(methods[i]);method_selection_check((int)i);CHECK(test_ble_scans()==scans&&requests==jobs);if(language==2&&i){snprintf(name,sizeof(name),"next-method-%u",i);shot(name,theme);}}
 click("Bluetooth");method_selection_check(0);
 click("Dalej");CHECK(test_ble_scans()==scans+1&&!dsr_ui_onboarding_active());dsr_ui_open_settings();
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 const unsigned themes[]={0,2,3};
 if(getenv("DSR_UI_FOCUS_CONTACT")){for(unsigned i=0;i<3;i++)contact_cards(themes[i]);}
 else if(getenv("DSR_UI_FOCUS_TRV")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)thermostat_card(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_COVER_CARD")){for(unsigned i=0;i<3;i++)cover_cards(themes[i]);}
 else if(getenv("DSR_UI_FOCUS_COVER")){for(unsigned i=0;i<3;i++)cover_skin(themes[i]);}
 else if(getenv("DSR_UI_FOCUS_ONBOARDING")){for(unsigned i=0;i<3;i++)onboarding_review(themes[i]);onboarding_labels();}
 else if(getenv("DSR_UI_FOCUS_ONBOARDING_NEXT")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)onboarding_next(themes[i],language);}
 else{if(!getenv("DSR_UI_FOCUS_BLE"))for(unsigned i=0;i<3;i++){common(themes[i]);dashboard(themes[i]);access_point(themes[i]);cover_skin(themes[i]);onboarding_next(themes[i],2);}onboarding_labels();}
 printf("PASS: dev069 focused UI, %u assertions, %u rendered frames; host providers\n",checks,screenshots);
 lv_deinit();return 0;
}
