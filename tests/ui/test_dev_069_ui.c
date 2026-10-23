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
 click("Ponów");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Ponów"));lv_obj_t *notice=find_label(lv_screen_active(),"Nie potwierdzono dodania. Domowe ustawienia pilota zachowane.");CHECK(notice);lv_obj_t *frame=lv_obj_get_parent(notice);CHECK(lv_obj_get_width(frame)==268&&lv_obj_get_x(notice)>=12);if(theme==3)CHECK(lv_obj_get_style_border_width(frame,0)>=2);shot("ap-error",theme);
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
static void lan_form_fields(lv_obj_t *body,lv_obj_t *field,lv_obj_t *channel)
{
 lv_area_t b=rect(body),n=rect(field),c=rect(channel);CHECK(lv_obj_get_width(field)==288&&lv_obj_get_width(channel)==288&&lv_obj_get_height(channel)==44);
 CHECK(n.y1>=b.y1&&c.y2<=b.y2-8&&b.y2<=399);
}
static void lan_form_notice(lv_obj_t *body,const char *text)
{
 lv_obj_t *label=find_label(lv_screen_active(),text);CHECK(label);lv_obj_t *container=lv_obj_get_parent(label);lv_area_t a=rect(label),b=rect(body);
 if(container!=body){
  CHECK(lv_obj_get_parent(container)==body);lv_area_t frame=rect(container);CHECK(frame.x1>=16&&frame.x2<=303&&frame.y1>=b.y1&&frame.y2<=b.y2-8);
  CHECK(a.x1>=frame.x1+10&&a.x2<=frame.x2-10&&a.y1>=frame.y1+8&&a.y2<=frame.y2-8);
  CHECK(lv_color_eq(lv_obj_get_style_border_color(container,0),dsr_sc_color(dsr_sc_style()==DSR_SC_STYLE_LIGHT?0xf0a020:0xffc107)));
 }else CHECK(lv_obj_get_parent(label)==body);
 CHECK(a.y1>=b.y1&&a.y2<=b.y2-8&&a.x1>=16&&a.x2<=303);
}
static void lan_form(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();
 dsr_lan_device_t device={.id="shellyplus1-aabbccddeeff",.name="Shelly Plus 1",.host="192.0.2.20",.model="SNSW-001X16EU",.mac="AABBCCDDEEFF",.generation=2};dsr_lan_capabilities_t caps={.switches=3};
 dsr_ui_lan_adopt(&device,&caps);lv_obj_t *field=find_type(lv_screen_active(),&lv_textarea_class),*body=lv_obj_get_parent(field);unsigned skip=1;lv_obj_t *channel=nth_type(body,&lv_dropdown_class,&skip);lan_form_fields(body,field,channel);
 if(language==2)shot("lan-form-fields",theme);
 lv_obj_t *identity=lv_obj_get_parent(find_label(lv_screen_active(),device.name)),*kb=find_type(lv_screen_active(),&lv_keyboard_class);lv_obj_send_event(field,LV_EVENT_CLICKED,NULL);CHECK(!lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN)&&lv_obj_has_flag(identity,LV_OBJ_FLAG_HIDDEN)&&rect(field).y2<rect(kb).y1);
 if(theme==3&&language==2)shot("lan-form-keyboard",theme);
 lv_obj_send_event(kb,LV_EVENT_READY,NULL);CHECK(lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN)&&!lv_obj_has_flag(identity,LV_OBJ_FLAG_HIDDEN));lan_form_fields(body,field,channel);
 lv_obj_t *help=find_label(lv_screen_active(),"Dodanie zachowuje poprzednie urządzenia i ich kanały.");CHECK(help&&lv_obj_get_parent(help)==body);lv_obj_scroll_to_view(help,LV_ANIM_OFF);lan_form_notice(body,"Dodanie zachowuje poprzednie urządzenia i ich kanały.");
 if(theme==3&&language==2)shot("lan-form-help",theme);
 unsigned writes=job_counts[DSR_JOB_BIND];lv_textarea_set_text(field,"");click("Zapisz");CHECK(job_counts[DSR_JOB_BIND]==writes);lan_form_notice(body,"Wpisz nazwę do 47 bajtów.");
 dsr_ui_lan_bind_result(DSR_IO,DSR_BIND_WORKSPACE);lan_form_notice(body,dsr_network_bind_result_text(DSR_IO,DSR_BIND_WORKSPACE));
 if(language==2)shot("lan-form-save-error",theme);
 dsr_ui_lan_bind_result(DSR_FULL,DSR_BIND_VERIFY);lan_form_notice(body,"Limit 16 urządzeń. Usuń jedno, aby dodać nowe.");
 dsr_ui_lan_bind_result(DSR_AUTH_REQUIRED,DSR_BIND_VERIFY);field=find_type(lv_screen_active(),&lv_textarea_class);body=lv_obj_get_parent(field);kb=find_type(lv_screen_active(),&lv_keyboard_class);CHECK(lv_obj_get_height(body)==184&&find_label(lv_screen_active(),"Błąd logowania. Sprawdź dane."));
 unsigned credential_index=1;lv_obj_t *secret=nth_type(body,&lv_textarea_class,&credential_index);lv_area_t password_area=rect(secret),visible=rect(body);CHECK(password_area.y1>=visible.y1&&password_area.y2<visible.y2-8);lan_form_notice(body,"Błąd logowania. Sprawdź dane.");
 lv_obj_send_event(field,LV_EVENT_CLICKED,NULL);lv_obj_send_event(kb,LV_EVENT_READY,NULL);CHECK(lv_obj_get_height(body)==184&&lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN));
 if(theme==3&&language==2)shot("lan-form-auth",theme);
 dsr_ui_open_settings();
}
typedef struct {char text[96];lv_area_t area;} dashboard_room_t;
static void dashboard_room_capture(lv_event_t *event)
{
 lv_draw_label_dsc_t *label=lv_draw_task_get_label_dsc(lv_event_get_draw_task(event));if(!label)return;
 dashboard_room_t *r=lv_event_get_user_data(event);snprintf(r->text,sizeof(r->text),"%s",label->text);lv_draw_task_get_area(lv_event_get_draw_task(event),&r->area);
}
static void dashboard_card_check(const dsr_mobile_model_t *m)
{
 lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),m->devices[0].name));dashboard_room_t room={0};
 lv_obj_add_flag(card,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);lv_obj_add_event_cb(card,dashboard_room_capture,LV_EVENT_DRAW_TASK_ADDED,&room);lv_obj_invalidate(card);lv_refr_now(display);lv_obj_remove_event_cb(card,dashboard_room_capture);lv_obj_remove_flag(card,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
 CHECK(!strcmp(room.text,m->rooms[0].name));lv_obj_t *state=find_label(card,m->devices[0].output?"Włączone":"Wyłączone"),*value=containing(card," V");CHECK(state&&value);lv_area_t s=rect(state),v=rect(value),pill=rect(lv_obj_get_parent(value));
 CHECK(room.area.y2<s.y1-4&&s.y2<pill.y1-4&&abs(v.y1+v.y2-pill.y1-pill.y2)<=1);
 CHECK(pill.y2<=rect(card).y2-10);lv_point_t size;lv_text_get_size(&size,room.text,dsr_sc_font(&dsr_roboto_14),0,0,lv_area_get_width(&room.area),LV_TEXT_FLAG_NONE);CHECK(lv_area_get_height(&room.area)>=size.y);
}
static void dashboard_review(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();
 dsr_mobile_actions_t a={.add_device=noop,.edit_dashboard=noop,.select_dashboard=noop,.open_settings=noop,.open_battery=noop};dsr_mobile_bind(&a);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard im Erdgeschoss",.room_count=1,.rooms={{.id=1,.name="Küche und Esszimmer"}},.device_count=3};
 m.devices[0]=(dsr_mobile_device_t){.id=1,.identity="survey-switch-1",.name="Deckenbeleuchtung Küche",.room=0,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true,.can_switch=true,.has_power=true,.power_w=0,.has_voltage=true,.voltage_v=231.8,.fields=255};
 m.devices[1]=m.devices[0];m.devices[1].id=2;snprintf(m.devices[1].identity,sizeof(m.devices[1].identity),"survey-switch-2");snprintf(m.devices[1].name,sizeof(m.devices[1].name),"Steckdose Terrasse");m.devices[1].output=true;m.devices[1].power_w=83.4;
 m.devices[2]=m.devices[0];m.devices[2].id=3;snprintf(m.devices[2].identity,sizeof(m.devices[2].identity),"survey-switch-3");snprintf(m.devices[2].name,sizeof(m.devices[2].name),"Beleuchtung Arbeitszimmer");m.devices[2].online=false;m.devices[2].has_sample_time=true;
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);CHECK(dsr_sc_style()==theme);lv_obj_t *selector=find_label(lv_screen_active(),m.dashboard_name);CHECK(selector&&lv_label_get_long_mode(selector)==LV_LABEL_LONG_MODE_SCROLL&&lv_anim_get(selector,NULL));dashboard_card_check(&m);if(language==2)shot("dashboard-review-normal",theme);
 m.device_count=1;snprintf(m.dashboard_name,sizeof(m.dashboard_name),"Erdgeschoss – Wohnen, Küche und Terrasse");snprintf(m.devices[0].name,sizeof(m.devices[0].name),"Wärmepumpe Küche und Hauswirtschaftsraum");snprintf(m.rooms[0].name,sizeof(m.rooms[0].name),"Wohnzimmer und offene Küche im Erdgeschoss");
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);dashboard_card_check(&m);if(language==2)shot("dashboard-review-long-names",theme);
 selector=find_label(lv_screen_active(),m.dashboard_name);CHECK(selector&&lv_label_get_long_mode(selector)==LV_LABEL_LONG_MODE_SCROLL);lv_anim_t *animation=lv_anim_get(selector,NULL);CHECK(animation&&animation->end_value<0);int32_t start=animation->current_value;
 for(unsigned frame=0;frame<8;frame++){lv_tick_inc(750);lv_timer_handler();lv_refr_now(display);if(theme==3&&language==2){char path[48];snprintf(path,sizeof(path),"dashboard-name-scroll-%u",frame);shot(path,theme);}}
 CHECK(lv_anim_get(selector,NULL)&&lv_anim_get(selector,NULL)->current_value!=start);
 m.devices[0].power_w=12345.6;m.devices[0].voltage_v=240.2;dsr_mobile_update(&m);CHECK(containing(lv_screen_active(),"12345.6 W")&&containing(lv_screen_active(),"240.2 V"));dashboard_card_check(&m);if(language==2)shot("dashboard-review-large-values",theme);
 selector=find_label(lv_screen_active(),m.dashboard_name);CHECK(selector);dsr_ui_open_settings();CHECK(!lv_anim_get(selector,NULL));
}
static void onboarding_states(unsigned theme,unsigned language)
{
 cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));publish_document(doc);
 wifi.connected=false;dsr_network_home(NULL);tick();if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}wifi.connected=true;
 unsigned writes=job_counts[DSR_JOB_AP_PROVISION];dsr_ui_ap_scan_open(NULL);CHECK(dsr_sc_style()==theme&&in_flight&&submitted.kind==DSR_JOB_AP_SCAN);if(language==2)shot("review-ap-scanning",theme);
 response=(dsr_event_t){0};deliver(DSR_OK,DSR_WIFI_IDLE);lv_obj_t *empty=find_label(lv_screen_active(),"Nie znaleziono fabrycznego AP Shelly. Włącz tryb konfiguracji urządzenia.");CHECK(empty);lv_obj_t *empty_card=lv_obj_get_parent(empty);CHECK(rect(empty_card).y2<=rect(lv_obj_get_parent(empty_card)).y2);if(language==2)shot("review-ap-empty",theme);
 click("Ponów");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN);response=(dsr_event_t){0};deliver(DSR_IO,DSR_WIFI_IDLE);lv_obj_t *warning=find_label(lv_screen_active(),"Nie potwierdzono dodania. Domowe ustawienia pilota zachowane."),*help=find_label(lv_screen_active(),"Sprawdź tryb konfiguracji Shelly. Jeśli AP zniknął po przekazaniu Wi-Fi, wyszukaj urządzenie w sieci lokalnej.");CHECK(warning&&help);lv_area_t w=rect(lv_obj_get_parent(warning)),h=rect(help),c=rect(lv_obj_get_parent(help));CHECK(rect(warning).y2<=w.y2-8&&h.y1>=w.y2+12&&h.y2<=c.y2-12);if(language==2)shot("review-ap-error",theme);
 click("Ponów");CHECK(in_flight&&submitted.kind==DSR_JOB_AP_SCAN&&job_counts[DSR_JOB_AP_PROVISION]==writes);
 response=(dsr_event_t){.scan={.count=1,.items={{.ssid="ShellyPlus1-112233445566",.security=DSR_WIFI_OPEN,.rssi=-48,.channel=6}}}};deliver(DSR_OK,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"ShellyPlus1-112233445566"));if(language==2)shot("review-ap-found",theme);click("Anuluj");
 if(getenv("DSR_UI_FOCUS_AP_STATES")){dsr_ui_open_settings();return;}
 dsr_ui_lan_open(NULL);CHECK(find_label(lv_screen_active(),"Wyszukiwanie mDNS i weryfikacja API…"));if(language==2)shot("review-lan-scanning",theme);
 /* Provider ends discovery without an identity accepted by the real parser. */
 test_lan_found("{}","192.0.2.20");dsr_ui_lan_tick();CHECK(find_label(lv_screen_active(),"Nie znaleziono Shelly. Sprawdź sieć i ponów."));if(language==2)shot("review-lan-empty",theme);
 click("Ponów");test_lan_found("{\"gen\":2,\"id\":\"shellyplus1-aabbccddeeff\",\"mac\":\"aabbccddeeff\",\"model\":\"SNSW-001X16EU\",\"name\":\"Shelly Plus 1\",\"auth_en\":true}","192.0.2.20");dsr_ui_lan_tick();click("Shelly Plus 1");CHECK(find_type(lv_screen_active(),&lv_textarea_class));click("Sprawdź");test_lan_inspected(DSR_AUTH_REQUIRED,"{}");dsr_ui_lan_tick();CHECK(find_label(lv_screen_active(),"Błąd logowania. Sprawdź dane."));lv_obj_t *mac=containing(lv_screen_active(),"AA:BB:CC:DD:EE:FF");CHECK(mac&&rect(mac).y2<=rect(lv_obj_get_parent(mac)).y2-6);if(language==2)shot("review-lan-auth-error",theme);
 click("Wstecz");CHECK(find_label(lv_screen_active(),"Shelly Plus 1"));click("Wstecz");
 wifi.connected=false;unsigned scans=test_ble_scans();dsr_ui_ble_scan_open(NULL);CHECK(test_ble_scans()==scans+1);lv_obj_t *offline=find_label(lv_screen_active(),"Wi-Fi offline • BLE dostępne");CHECK(offline&&rect(offline).y2<=rect(lv_obj_get_parent(offline)).y2-6);if(language==2)shot("review-ble-scanning",theme);
 test_ble_finish(DSR_BLE_DONE);dsr_ui_ble_tick();CHECK(find_label(lv_screen_active(),"Nie znaleziono urządzeń BLE"));if(language==2)shot("review-ble-empty",theme);
 click("Ponów");CHECK(test_ble_scans()==scans+2);test_ble_finish(DSR_BLE_UNAVAILABLE);dsr_ui_ble_tick();CHECK(find_label(lv_screen_active(),"Bluetooth niedostępny"));if(language==2)shot("review-ble-error",theme);
 click("Ponów");CHECK(test_ble_scans()==scans+3);test_ble_finish(DSR_BLE_DONE);dsr_ui_ble_tick();click("Wstecz");CHECK(!dsr_ui_ble_active());wifi.connected=true;dsr_ui_open_settings();
}
static void header_back_review(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();
 dsr_lan_device_t device={.id="shellyplus1-aabbccddeeff",.name="Shelly Plus 1",.host="192.0.2.20",.model="SNSW-001X16EU",.generation=2};dsr_lan_capabilities_t caps={.switches=1};
 dsr_ui_lan_adopt(&device,&caps);lv_obj_t *back=NULL,*root=lv_screen_active();
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=lv_obj_get_child(root,(int)i);lv_area_t a=rect(o);if(a.x1==12&&a.y1==0&&lv_obj_get_width(o)==44&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))back=o;}
 CHECK(back&&lv_obj_get_height(back)==44);lv_area_t a=rect(back);
 if(language==2)shot("header-back",theme);
 touch=(lv_point_t){(a.x1+a.x2)/2,(a.y1+a.y2)/2};pressed=true;lv_tick_inc(20);lv_indev_read(pointer);lv_refr_now(display);CHECK(lv_obj_has_state(back,LV_STATE_PRESSED));
 if(theme==3&&language==2)shot("header-back-pressed",theme);
 pressed=false;lv_tick_inc(20);lv_indev_read(pointer);tick();CHECK(!find_type(lv_screen_active(),&lv_textarea_class)); /* real header callback returns to results */
 dsr_ui_open_settings();
}
static void settings_swipe(void)
{
 pointer_move(158,362,true);for(int i=1;i<=9;i++)pointer_move(158,362-i*24,true);pointer_move(158,146,false);lv_tick_inc(500);lv_timer_handler();lv_refr_now(display);
}
static void settings_review(unsigned theme,unsigned language)
{
 dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();dsr_ui_open_settings();CHECK(dsr_sc_style()==theme);
 const char *names[]={"Sieć i połączenia","Wygląd i język","Ekran i komfort","Bateria i zasilanie","Panel WWW / bezpieczeństwo","System"};
 for(unsigned i=0;i<6;i++){
  lv_obj_t *label=find_label(lv_screen_active(),names[i]);CHECK(label);lv_area_t l=rect(label),b=rect(lv_obj_get_parent(label));
  CHECK(l.x1>=b.x1+52&&l.x2<=b.x2-36&&l.y1>=b.y1+5&&l.y2<=b.y2-5&&abs(l.y1+l.y2-b.y1-b.y2)<=2);
  CHECK(b.y1>=86&&b.y2<=DSR_SC_NAV_TOP-13&&lv_obj_get_height(lv_obj_get_parent(label))>=44); /* every section is visible before any swipe */
 }
 lv_obj_t *network_label=find_label(lv_screen_active(),names[0]);CHECK(lv_obj_get_height(network_label)==lv_font_get_line_height(lv_obj_get_style_text_font(network_label,0)));
 if(language==2)shot("settings-review-top",theme);
 lv_obj_t *last=lv_obj_get_parent(find_label(lv_screen_active(),"System")),*body=lv_obj_get_parent(last);settings_swipe();CHECK(lv_obj_is_valid(body));CHECK(lv_obj_get_scroll_y(body)==0);lv_area_t b=rect(body),l=rect(last);CHECK(l.y1>=b.y1&&l.y2<=b.y2-6&&l.y2<=DSR_SC_NAV_TOP-12);
 tap((l.x1+l.x2)/2,(l.y1+l.y2)/2);CHECK(find_label(lv_screen_active(),"Firmware Update")&&find_label(lv_screen_active(),"Restart pilota"));if(language==2)shot("settings-review-system",theme);
 click("Wstecz");lv_obj_t *network=lv_obj_get_parent(find_label(lv_screen_active(),"Sieć i połączenia"));l=rect(network);tap((l.x1+l.x2)/2,(l.y1+l.y2)/2);CHECK(find_label(lv_screen_active(),"Wi-Fi setup"));if(language==2)shot("settings-review-network-top",theme);
 if(theme==3)CHECK(has_icon(lv_obj_get_parent(find_label(lv_screen_active(),"Dodaj urządzenie Shelly")),DSR_ICON_PLUS,24));
 body=lv_obj_get_parent(lv_obj_get_parent(find_label(lv_screen_active(),"Odśwież")));settings_swipe();last=lv_obj_get_parent(find_label(lv_screen_active(),"Odśwież"));l=rect(last);b=rect(body);CHECK(l.y1>=b.y1&&l.y2<=b.y2&&l.y2<420);if(language==2)shot("settings-review-network-bottom",theme);
 click("Wstecz");CHECK(find_label(lv_screen_active(),"System"));
}
static unsigned lighting_level_writes,lighting_color_writes;
static uint8_t lighting_level,lighting_color[3];
static void lighting_set_level(unsigned index,uint8_t value)
{CHECK(index==0&&value<=100);lighting_level_writes++;lighting_level=value;}
static void lighting_set_color(unsigned index,uint8_t r,uint8_t g,uint8_t b)
{CHECK(index==0);lighting_color_writes++;lighting_color[0]=r;lighting_color[1]=g;lighting_color[2]=b;}
static void lighting_output(unsigned index,bool on){CHECK(index==0);(void)on;}
static void lighting_refresh(uint32_t id){CHECK(id==31);}
static void lighting_actions(uint8_t kind){(void)kind;}
static lv_obj_t *lighting_dial(lv_obj_t *o)
{
 if(lv_obj_check_type(o,&lv_arc_class)&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))return o;
 for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *dial=lighting_dial(lv_obj_get_child(o,(int)i));if(dial)return dial;}return NULL;
}
static void lighting_review(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();
 dsr_mobile_actions_t actions={.set_brightness=lighting_set_level,.set_rgb=lighting_set_color,.set_output=lighting_output,.refresh=lighting_refresh,.open_usage=lighting_refresh,.open_actions=lighting_actions,.device_settings=lighting_refresh};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.device_count=1,.room_count=1,.rooms={{.id=1,.name="Wohnzimmer"}}};
 dsr_mobile_device_t *d=&m.devices[0];*d=(dsr_mobile_device_t){.id=31,.name="Dimmer Esszimmer",.identity="review-light",.host="192.0.2.31",.generation=DSR_GEN2_PLUS,.room=0,.endpoint_kind=DSR_ENDPOINT_LIGHT,.kind=DSR_DEVICE_LIGHT,.online=true,.output=true,.can_switch=true,.can_light=true,.can_refresh=true,.has_power=true,.power_w=24.8,.has_voltage=true,.voltage_v=232.1,.has_current=true,.current_a=.11,.has_sample_time=true,.sample_ms=lv_tick_get(),.light={.brightness=75,.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS}};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);CHECK(dsr_sc_style()==theme&&find_label(lv_screen_active(),"75%"));
 lv_obj_t *watts=find_label(lv_screen_active(),"24.8 W"),*legend=find_label(lv_screen_active(),"Moc");CHECK(watts&&legend);lv_area_t v=rect(watts),f=rect(lv_obj_get_parent(watts));CHECK(rect(legend).y2<v.y1&&v.y2<=f.y2-5);
 if(language==2)shot("lighting-dimmer",theme);
 lv_obj_t *input=lighting_dial(lv_screen_active());CHECK(input&&!lv_obj_has_state(input,LV_STATE_DISABLED));lv_area_t a=rect(input);int x,y;
 unsigned quick=lighting_level_writes;click("+");CHECK(lighting_level_writes==quick+1&&lighting_level==80);click("-");CHECK(lighting_level_writes==quick+2&&lighting_level==70);click("50%");CHECK(lighting_level_writes==quick+3&&lighting_level==50);
 unsigned before=lighting_level_writes;
 int cx=(a.x1+a.x2)/2,cy=(a.y1+a.y2)/2,r=lv_area_get_width(&a)/2-15;pointer_move(cx-r,cy,true);x=cx-r*7/10;y=cy-r*7/10;
 pointer_move(x,y,true);CHECK(lighting_level_writes==before);if(language==2)shot("lighting-dimmer-preview",theme);pointer_move(x,y,false);
 CHECK(lighting_level_writes==before+1&&lighting_level!=75&&find_label(lv_screen_active(),"75%"));d->light.brightness=lighting_level;dsr_mobile_update(&m);char value[12];snprintf(value,sizeof(value),"%u%%",lighting_level);CHECK(find_label(lv_screen_active(),value));
 strcpy(d->name,"LED Wohnzimmer");d->endpoint_kind=DSR_ENDPOINT_RGB;d->kind=DSR_DEVICE_RGB;d->can_rgb=true;d->rgb=(dsr_rgb_status_t){.r=0,.g=180,.b=255,.brightness=78,.flags=DSR_RGB_VALID};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);CHECK(find_label(lv_screen_active(),"78%"));if(language==2)shot("lighting-rgb",theme);
 lv_obj_t *power=lv_obj_get_parent(find_label(lv_screen_active(),"Wyłącz"));CHECK(lv_obj_get_height(power)>=44&&rect(power).y2<=DSR_SC_NAV_TOP-17);
 {
  before=lighting_color_writes;pointer_move(218,198,true);pointer_move(201,156,true);CHECK(lighting_color_writes==before);pointer_move(201,156,false);CHECK(lighting_color_writes==before+1);
  before=lighting_color_writes;click("Relaks");CHECK(lighting_color_writes==before+1&&lighting_color[0]==150&&lighting_color[1]==60&&lighting_color[2]==255);
  dsr_mobile_show(DSR_MOBILE_DETAILS,0);
 }
 click("Kolor");input=find_type(lv_screen_active(),&lv_slider_class);CHECK(input&&!lv_obj_has_state(input,LV_STATE_DISABLED));CHECK(rect(lv_obj_get_parent(find_label(lv_screen_active(),"Zastosuj"))).y2<=DSR_SC_NAV_TOP-17);if(language==2)shot("lighting-rgb-values",theme);
 a=rect(input);y=(a.y1+a.y2)/2;x=a.x1+lv_area_get_width(&a)*8/10;before=lighting_color_writes;pointer_move((a.x1+a.x2)/2,y,true);pointer_move(x,y,true);pointer_move(x,y,false);int red=lv_slider_get_value(input);
 CHECK(red>0&&lighting_color_writes==before&&!lv_obj_has_state(lv_obj_get_parent(find_label(lv_screen_active(),"Zastosuj")),LV_STATE_DISABLED));if(language==2)shot("lighting-rgb-preview",theme);
 click("Zastosuj");CHECK(lighting_color_writes==before+1&&lighting_color[0]==red&&lighting_color[1]==180&&lighting_color[2]==255);
 d->rgb.r=lighting_color[0];dsr_mobile_update(&m);input=find_type(lv_screen_active(),&lv_slider_class);CHECK(input&&lv_slider_get_value(input)==red);
 d->online=false;d->error=DSR_OFFLINE;d->can_switch=d->can_light=d->can_rgb=false;dsr_mobile_update(&m);input=find_type(lv_screen_active(),&lv_slider_class);CHECK(input&&lv_obj_has_state(input,LV_STATE_DISABLED));before=lighting_color_writes;click("Zastosuj");CHECK(lighting_color_writes==before);if(language==2)shot("lighting-offline",theme);
 dsr_ui_open_settings();
}
static lv_obj_t *status_battery(void)
{
 lv_obj_t *root=lv_screen_active();
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *o=lv_obj_get_child(root,(int)i);lv_area_t a=rect(o);
  if(a.x1==256&&a.y1==0&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE)&&lv_obj_get_width(o)==52)return o;
 }
 CHECK(!"Battery header missing");return NULL;
}
static void status_value(const char *expected)
{
 lv_obj_t *battery=status_battery(),*value=lv_obj_get_child(battery,0);CHECK(value&&lv_obj_check_type(value,&lv_label_class));
 CHECK(!strcmp(lv_label_get_text(value),expected));lv_area_t b=rect(battery),v=rect(value);lv_point_t text;
 lv_text_get_size(&text,expected,lv_obj_get_style_text_font(value,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 CHECK(text.x<=lv_area_get_width(&v)&&text.y==lv_area_get_height(&v));
 CHECK(v.x1>=b.x1+4&&v.x2<=b.x1+43&&v.y1>=b.y1+13&&v.y2<=b.y1+29);
 CHECK(abs((v.x1+v.x2)-(b.x1*2+47))<=2&&abs((v.y1+v.y2)-(b.y1*2+42))<=2);
 lv_obj_t *clock=find_label(lv_screen_active(),"14:34");CHECK(clock);lv_area_t c=rect(clock);
 lv_text_get_size(&text,"23:59",lv_obj_get_style_text_font(clock,0),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
 CHECK(text.x<=lv_area_get_width(&c)&&c.y2<DSR_SC_STATUS_H);
 CHECK(b.x1-248>=8&&b.x2<=307&&lv_obj_get_height(battery)>=44);
}
static uint32_t status_fill_pixels(void)
{
 lv_refr_now(display);uint32_t hash=2166136261u;
 for(int y=13;y<30;y++)for(int x=260;x<300;x++)for(int c=0;c<3;c++)hash=(hash^canvas[(y*320+x)*3+c])*16777619u;
 return hash;
}
static void status_lighting_review(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();
 dsr_mobile_actions_t actions={.add_device=noop,.edit_dashboard=noop,.select_dashboard=noop};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=31,.identity="status-lighting",.name="Deckenbeleuchtung Küche",.kind=DSR_DEVICE_LIGHT,.endpoint_kind=DSR_ENDPOINT_LIGHT,.online=true,.can_light=true,.light={.brightness=75,.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS}};
 dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3902,.has_percent=true,.percent=63,.percent_estimated=true}};
 for(unsigned rgb=0;rgb<2;rgb++){
  if(rgb){m.devices[0].kind=DSR_DEVICE_RGB;m.devices[0].endpoint_kind=DSR_ENDPOINT_RGB;m.devices[0].can_rgb=true;m.devices[0].rgb=(dsr_rgb_status_t){.r=0,.g=180,.b=255,.brightness=78,.flags=DSR_RGB_VALID};}
  dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_DETAILS,0);dsr_ui_battery_result(&e);status_value("~63%");
  if(language==2)shot(rgb?"status-rgb":"status-device",theme);
 }
}
static void status_review(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();
 dsr_mobile_actions_t actions={.add_device=noop,.edit_dashboard=noop,.select_dashboard=noop};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard",.room_count=1,.rooms={{.id=1,.name="Küche"}},.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=31,.identity="status-review",.name="Deckenbeleuchtung Küche",.room=0,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);
 dsr_event_t e={.result=DSR_OK,.battery={.has_voltage=true,.bat_mv=3902,.has_percent=true,.percent=63,.percent_estimated=true}};
 dsr_ui_battery_result(&e);status_value("~63%");if(language==2)shot("status-home",theme);
 e.battery.percent=100;dsr_ui_battery_result(&e);status_value("~100%");if(language==2)shot("status-full",theme);
 e.battery.percent=5;e.battery.percent_estimated=false;dsr_ui_battery_result(&e);status_value("5%");if(language==2)shot("status-low",theme);
 e.battery.percent=0;dsr_ui_battery_result(&e);status_value("0%");
 e.battery.has_percent=false;dsr_ui_battery_result(&e);status_value("3.90V");if(language==2)shot("status-voltage",theme);
 e.result=DSR_IO;dsr_ui_battery_result(&e);status_value("—");
 e.result=DSR_OK;e.battery.has_percent=true;e.battery.percent=63;e.battery.percent_estimated=true;dsr_ui_battery_result(&e);
 lv_tick_inc(30000);dsr_ui_battery_tick();status_value("—");
 m.wifi=false;dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);status_value("—");if(language==2)shot("status-disconnected",theme);
 m.wifi=true;dsr_mobile_update(&m);dsr_ui_open_settings();dsr_ui_battery_result(&e);status_value("~63%");if(language==2)shot("status-settings",theme);
 status_lighting_review(theme,language);
 /* A charger-capable provider is a model here, not a claim about board GPIOs. */
 e.battery.has_charge_state=true;e.battery.charging=true;dsr_ui_battery_result(&e);
 lv_tick_inc((2300-lv_tick_get()%2000)%2000);dsr_ui_battery_tick();status_value("~63%");uint32_t first=status_fill_pixels();
 if(language==2)shot("status-charge-start",theme);
 lv_tick_inc(800);dsr_ui_battery_tick();CHECK(status_fill_pixels()!=first);status_value("~63%");if(language==2)shot("status-charge-end",theme);
 display_sleeping=true;dsr_ui_battery_tick();first=status_fill_pixels();lv_tick_inc(400);dsr_ui_battery_tick();CHECK(status_fill_pixels()==first);display_sleeping=false;
 e.battery.charging=false;dsr_ui_battery_result(&e);dsr_ui_open_settings();
}
static unsigned navigation_calls[6],navigation_issues;
static void navigation_battery(void){navigation_calls[2]++;}
static void navigation_settings(void){navigation_calls[3]++;dsr_ui_open_settings();}
static void navigation_add_device(void){navigation_calls[4]++;}
static void navigation_add_room(void){navigation_calls[5]++;}
static lv_obj_t *navigation_bar(void)
{
 lv_obj_t *root=lv_screen_active();lv_obj_update_layout(root);
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *o=lv_obj_get_child(root,(int)i);
  if(lv_obj_get_y(o)==DSR_SC_NAV_TOP&&lv_obj_get_width(o)==320&&lv_obj_get_height(o)==52)return o;
 }
 CHECK(!"Bottom navigation missing");return NULL;
}
static unsigned navigation_current(void)
{
 lv_obj_t *bar=navigation_bar();
 for(unsigned i=0;i<lv_obj_get_child_count(bar);i++){
  lv_obj_t *o=lv_obj_get_child(bar,(int)i);
  if(lv_obj_get_width(o)==32&&lv_obj_get_height(o)==3)return (unsigned)(lv_obj_get_x(o)-16)/64;
 }
 CHECK(!"Active navigation marker missing");return 5;
}
static uint32_t navigation_pixels(unsigned index)
{
 lv_refr_now(display);uint32_t hash=2166136261u;
 for(int y=432;y<479;y++)for(int x=(int)index*64;x<(int)(index+1)*64;x++)for(int c=0;c<3;c++)hash=(hash^canvas[(y*320+x)*3+c])*16777619u;
 return hash;
}
static void navigation_geometry(void)
{
 lv_obj_t *bar=navigation_bar();lv_area_t b=rect(bar);unsigned count=0;
 CHECK(b.y1==428&&b.y2==479);
 for(unsigned i=0;i<lv_obj_get_child_count(bar);i++){
  lv_obj_t *o=lv_obj_get_child(bar,(int)i);if(!lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))continue;
  lv_area_t a=rect(o);CHECK(count<5&&a.x1==(int)count*64&&a.x2==(int)(count+1)*64-1);
  CHECK(a.y1>=b.y1+4&&a.y2<b.y2&&lv_area_get_width(&a)>=44&&lv_area_get_height(&a)>=44);count++;
 }
 CHECK(count==5);
}
static void navigation_review(unsigned theme)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,2);dsr_ui_apply_theme();memset(navigation_calls,0,sizeof(navigation_calls));
 dsr_mobile_actions_t actions={.open_battery=navigation_battery,.open_settings=navigation_settings,.add_device=navigation_add_device,.add_room=navigation_add_room};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard",.device_count=1,.room_count=1,.rooms={{.id=1,.name="Wohnzimmer"}}};
 m.devices[0]=(dsr_mobile_device_t){.id=31,.identity="navigation-review",.name="Deckenbeleuchtung Küche",.room=0,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);navigation_geometry();CHECK(navigation_current()==0);shot("navigation-home",theme);
 tap(96,455);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS&&navigation_current()==1);navigation_geometry();shot("navigation-rooms",theme);
 tap(288,455);CHECK(navigation_calls[5]==1&&navigation_calls[4]==0); /* + adds a room here */
 tap(32,455);CHECK(dsr_mobile_page()==DSR_MOBILE_HOME&&navigation_current()==0);tap(160,455);CHECK(navigation_calls[2]==1);
 unsigned before=navigation_calls[4];pointer_move(288,455,true);shot("navigation-add-pressed",theme);pointer_move(288,455,false);CHECK(navigation_calls[4]==before+1);
 dsr_mobile_show(DSR_MOBILE_DETAILS,0);navigation_geometry();shot("navigation-home-details",theme);
 if(navigation_current()!=0){navigation_issues++;printf("NAV issue theme=%u: details opened from Dashboard select Rooms\n",theme);}
 tap(96,455);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);dsr_mobile_show(DSR_MOBILE_DETAILS,0);CHECK(navigation_current()==1);
 tap(32,455);dsr_mobile_show(DSR_MOBILE_DEVICE_SETTINGS,0);CHECK(navigation_current()==0);
 tap(32,22);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS&&navigation_current()==0);tap(32,22);CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
 tap(96,455);dsr_mobile_show(DSR_MOBILE_DEVICE_SETTINGS,0);CHECK(navigation_current()==1);
 tap(32,22);CHECK(dsr_mobile_page()==DSR_MOBILE_DETAILS&&navigation_current()==1);tap(32,22);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 tap(224,455);CHECK(navigation_calls[3]==1&&find_label(lv_screen_active(),"Ustawienia pilota"));navigation_geometry();CHECK(navigation_current()==3);shot("navigation-settings",theme);
 /* Real shared shell callbacks return from settings to the mobile pages. */
 tap(32,455);CHECK(dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_HOME);tap(96,455);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);
 /* The real shell reloaded its stored appearance; restore this review skin. */
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,2);dsr_ui_apply_theme();dsr_mobile_bind(&actions);dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);CHECK(dsr_sc_style()==theme);
 before=navigation_calls[4];pointer_move(288,455,true);pointer_move(288,400,true);pointer_move(288,400,false);
 if(navigation_calls[4]!=before){navigation_issues++;printf("NAV issue theme=%u: dragging outside + still invokes add\n",theme);}
 before=navigation_calls[4];unsigned settings_before=navigation_calls[3];pointer_move(288,455,true);pointer_move(224,455,true);pointer_move(224,455,false);
 CHECK(navigation_calls[4]==before&&navigation_calls[3]==settings_before&&dsr_mobile_page()==DSR_MOBILE_HOME);
 /* Adjacent edge points belong to different 64 px targets. */
 tap(63,455);CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);tap(64,455);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS);tap(0,478);CHECK(dsr_mobile_page()==DSR_MOBILE_HOME);
 uint32_t enabled_pixels[3];for(unsigned i=0;i<3;i++)enabled_pixels[i]=navigation_pixels(i+2);
 dsr_mobile_actions_t disabled={0};dsr_mobile_bind(&disabled);dsr_mobile_show(DSR_MOBILE_HOME,0);CHECK(dsr_sc_style()==theme);navigation_geometry();shot("navigation-disabled",theme);
 for(unsigned i=0;i<3;i++)CHECK(navigation_pixels(i+2)!=enabled_pixels[i]);
 unsigned total=navigation_calls[2]+navigation_calls[3]+navigation_calls[4]+navigation_calls[5];tap(160,455);tap(224,455);tap(288,455);
 CHECK(navigation_calls[2]+navigation_calls[3]+navigation_calls[4]+navigation_calls[5]==total&&dsr_mobile_page()==DSR_MOBILE_HOME);
 dsr_ui_open_settings();fflush(stdout);
}
static unsigned relay_spacing_switches,relay_spacing_refreshes;
static bool relay_spacing_desired;
static void relay_spacing_switch(unsigned index,bool on){CHECK(index==0);relay_spacing_switches++;relay_spacing_desired=on;}
static void relay_spacing_refresh(uint32_t id){CHECK(id==31);relay_spacing_refreshes++;}
static lv_obj_t *relay_spacing_power(void)
{
 lv_obj_t *root=lv_screen_active();
 for(unsigned i=0;i<lv_obj_get_child_count(root);i++){
  lv_obj_t *o=lv_obj_get_child(root,(int)i);
  if(lv_obj_get_width(o)==86&&lv_obj_get_height(o)==86&&lv_obj_has_flag(o,LV_OBJ_FLAG_CLICKABLE))return o;
 }
 CHECK(!"Relay power control missing");return NULL;
}
static void relay_spacing_geometry(const char *voltage,const char *current,const char *action)
{
 lv_obj_t *root=lv_screen_active(),*volts=find_label(root,voltage),*amps=find_label(root,current),*legend=find_label(root,"Moc chwilowa");CHECK(volts&&amps&&legend);
 lv_area_t v=rect(lv_obj_get_parent(volts)),a=rect(lv_obj_get_parent(amps)),nav=rect(navigation_bar());
 CHECK(nav.y1-v.y2-1==16&&nav.y1-a.y2-1==16&&v.y1==a.y1&&v.x2<a.x1);
 CHECK(rect(volts).y2<=v.y2-3&&rect(amps).y2<=a.y2-3);
 lv_obj_t *age=NULL;for(unsigned i=0;i<lv_obj_get_child_count(root);i++){lv_obj_t *o=lv_obj_get_child(root,(int)i);if(lv_obj_get_width(o)==304&&lv_obj_get_height(o)==28)age=o;}CHECK(age);
 lv_area_t age_area=rect(age),power_field=rect(lv_obj_get_parent(legend)),panel=rect(lv_obj_get_parent(lv_obj_get_parent(legend))),button=rect(relay_spacing_power());
 CHECK(v.y1-age_area.y2-1>=8&&age_area.y1-panel.y2-1>=6);
 CHECK(button.y1-power_field.y2-1>=8&&panel.y2-button.y2>=12);
 lv_obj_t *refresh_button=lv_obj_get_parent(find_label(root,"Odśwież stan"));lv_area_t r=rect(refresh_button);CHECK(abs((r.y1+r.y2)-(button.y1+button.y2))<=2&&r.x1>button.x2+8);
 if(action){
  lv_obj_t *label=find_label(root,action);CHECK(label);lv_area_t l=rect(label);const lv_font_t *font=lv_obj_get_style_text_font(label,0);
  CHECK(l.x1>=panel.x1+6&&l.x2+8<button.x1&&l.y1>power_field.y2&&l.y2<panel.y2-6);
  CHECK(abs((l.y1+l.y2)-(button.y1+button.y2))<=2&&lv_area_get_height(&l)==lv_font_get_line_height(font));
 }
}
static void relay_spacing_review(unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,3,language);dsr_ui_apply_theme();
 dsr_mobile_actions_t actions={.set_output=relay_spacing_switch,.refresh=relay_spacing_refresh,.open_settings=noop,.open_battery=noop,.add_device=noop};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.device_count=1};dsr_mobile_device_t *d=&m.devices[0];
 *d=(dsr_mobile_device_t){.id=31,.identity="relay-spacing",.name="Deckenbeleuchtung Küche",.host="192.0.2.31",.generation=DSR_GEN2_PLUS,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true,.output=true,.can_switch=true,.can_refresh=true,.has_power=true,.power_w=3210.8,.has_voltage=true,.voltage_v=231.8,.has_current=true,.current_a=13.851,.has_sample_time=true,.sample_ms=lv_tick_get()};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);dsr_mobile_show(DSR_MOBILE_DETAILS,0);relay_spacing_geometry("231.8 V","13.851 A","Wyłącz");
 const char *languages[]={"pl","en","de"};char name[48];snprintf(name,sizeof(name),"relay-spacing-on-%s",languages[language]);shot(name,3);
 unsigned before=relay_spacing_switches;tap(160,262);CHECK(relay_spacing_switches==before+1&&!relay_spacing_desired&&find_label(lv_screen_active(),"Wyłącz"));
 before=relay_spacing_refreshes;tap(259,262);CHECK(relay_spacing_refreshes==before+1);
 d->output=false;d->power_w=0;d->current_a=0;dsr_mobile_update(&m);relay_spacing_geometry("231.8 V","0.000 A","Włącz");if(language==2)shot("relay-spacing-off",3);
 d->busy=d->command_pending=true;dsr_mobile_update(&m);relay_spacing_geometry("231.8 V","0.000 A",NULL);CHECK(lv_obj_has_state(relay_spacing_power(),LV_STATE_DISABLED));if(language==2)shot("relay-spacing-busy",3);
 d->busy=d->command_pending=false;d->online=false;d->error=DSR_OFFLINE;dsr_mobile_update(&m);relay_spacing_geometry("— V","— A",NULL);CHECK(lv_obj_has_state(relay_spacing_power(),LV_STATE_DISABLED));
 before=relay_spacing_switches;tap(160,262);CHECK(relay_spacing_switches==before);if(language==2)shot("relay-spacing-offline",3);
 d->online=true;d->error=DSR_OK;d->has_sample_time=false;dsr_mobile_update(&m);relay_spacing_geometry("— V","— A",NULL);CHECK(lv_obj_has_state(relay_spacing_power(),LV_STATE_DISABLED));if(language==2)shot("relay-spacing-missing",3);
 dsr_ui_open_settings();
}
static unsigned card_notice_issues;
static void dashboard_notice_review(unsigned theme,unsigned language)
{
 dsr_ui_open_settings();dsr_sc_appearance(DSR_SC_BLUE,theme,language);dsr_ui_apply_theme();dsr_mobile_actions_t actions={0};dsr_mobile_bind(&actions);
 dsr_mobile_model_t m={.wifi=true,.dashboard_name="Mein Dashboard",.room_count=1,.rooms={{.id=1,.name="Wohnzimmer"}},.device_count=1};
 m.devices[0]=(dsr_mobile_device_t){.id=1,.identity="survey-notice",.name="Licht Küche",.room=0,.kind=DSR_DEVICE_RELAY,.endpoint_kind=DSR_ENDPOINT_SWITCH,.has_sample_time=true,.has_power=true,.power_w=9.5,.has_voltage=true,.voltage_v=231.8};
 dsr_mobile_update(&m);dsr_mobile_show(DSR_MOBILE_HOME,0);
 lv_obj_t *card=lv_obj_get_parent(find_label(lv_screen_active(),m.devices[0].name)),*notice=find_label(card,"Odczyty niedostępne");CHECK(notice&&lv_obj_get_parent(notice)!=card);
 if(find_label(card,"Odczyt nieaktualny")){card_notice_issues++;printf("CARD issue theme=%u language=%u: stale state duplicates framed notice\n",theme,language);}
 if(language==2)shot("dashboard-notice-stale",theme);
 m.devices[0].error=DSR_OFFLINE;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Brak odpowiedzi"));
 m.devices[0].error=DSR_AUTH_REQUIRED;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Błąd logowania"));
 m.devices[0].error=DSR_OK;m.wifi=false;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Brak Wi-Fi"));
 m.wifi=true;m.devices[0].has_sample_time=false;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Oczekiwanie na odczyt…"));
 /* Without a measurement frame, keep the only notice of an old reading. */
 m.devices[0].has_sample_time=true;m.configured_layout=true;m.tile_count=1;m.tiles[0]=(dsr_mobile_tile_t){.type=0,.index=0,.span=2,.height=1,.fields=0};dsr_mobile_update(&m);
 CHECK(find_label(lv_screen_active(),"Odczyt nieaktualny")&&!find_label(lv_screen_active(),"Odczyty niedostępne"));
 m.configured_layout=false;m.tile_count=0;m.devices[0].online=true;dsr_mobile_update(&m);CHECK(find_label(lv_screen_active(),"Wyłączone")&&containing(lv_screen_active(),"9.5 W"));CHECK(!find_label(lv_screen_active(),"Odczyty niedostępne"));
 dsr_ui_open_settings();fflush(stdout);
}
static unsigned add_save_draft_issues;
static void add_save_idle(void)
{
 wifi.connected=false;dsr_network_home(NULL);tick();
 if(in_flight){CHECK(submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};deliver(DSR_OFFLINE,DSR_WIFI_IDLE);}
 wifi.connected=true;dsr_ui_open_settings();
}
static void add_save_fields(lv_obj_t **field,lv_obj_t **room_field,lv_obj_t **channel_field)
{
 *field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(*field);unsigned skip=0;
 *room_field=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);skip=1;*channel_field=nth_type(lv_screen_active(),&lv_dropdown_class,&skip);CHECK(*room_field&&*channel_field);
}
static void add_save_review(unsigned theme)
{
 add_save_idle();cJSON *r=room_document();cJSON_ReplaceItemInObjectCaseSensitive(r,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString("de"));
 cJSON *rooms=cJSON_GetObjectItemCaseSensitive(r,"rooms"),*devices=cJSON_GetObjectItemCaseSensitive(r,"devices");
 if(!cJSON_GetArraySize(rooms))cJSON_AddItemToArray(rooms,cJSON_Parse("{\"id\":1,\"name\":\"Wohnzimmer\",\"icon\":0,\"photo\":\"\"}"));
 if(!cJSON_GetArraySize(devices))cJSON_AddItemToArray(devices,cJSON_Parse("{\"id\":1,\"name\":\"Licht Flur\",\"host\":\"192.0.2.10\",\"kind\":\"relay\",\"channel\":0,\"room\":1}"));
 cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(r,"dashboards"),0),"name",cJSON_CreateString("Mein Dashboard"));publish_document(r);
 dsr_sc_appearance(DSR_SC_BLUE,theme,2);dsr_ui_apply_theme();
 dsr_lan_device_t device={.name="Shelly Plus 2PM",.model="SNSW-002P16EU",.generation=2};
 snprintf(device.id,sizeof(device.id),"shellyplus2pm-aabbccddee%02u",theme);snprintf(device.mac,sizeof(device.mac),"aabbccddee%02u",theme);snprintf(device.host,sizeof(device.host),"192.0.2.%u",20+theme);
 char identity[240];snprintf(identity,sizeof(identity),"{\"gen\":2,\"id\":\"%s\",\"mac\":\"%s\",\"model\":\"%s\",\"name\":\"%s\",\"auth_en\":false}",device.id,device.mac,device.model,device.name);test_lan_found(identity,device.host);
 dsr_lan_capabilities_t caps={.switches=(1u<<1)|(1u<<4)};dsr_ui_lan_adopt(&device,&caps);
 lv_obj_t *field,*room_field,*channel_field;add_save_fields(&field,&room_field,&channel_field);CHECK(lv_dropdown_get_option_count(channel_field)==2);
 char local_name[48];snprintf(local_name,sizeof(local_name),"Licht Küche %u",theme);lv_textarea_set_text(field,local_name);lv_dropdown_set_selected(room_field,1);lv_dropdown_set_selected(channel_field,1);
 lv_obj_t *save=lv_obj_get_parent(find_label(lv_screen_active(),"Zapisz"));CHECK(save&&!lv_obj_has_state(save,LV_STATE_DISABLED));shot("add-save-form",theme);
 lv_dropdown_open(channel_field);shot("add-save-channels",theme);lv_dropdown_close(channel_field);
 char *before_document=dsr_workspace_snapshot();CHECK(before_document);unsigned before=job_counts[DSR_JOB_BIND],published=publish_jobs;
 test_workspace_defer(true);tap(238,444);CHECK(in_flight&&submitted.kind==DSR_JOB_BIND&&submitted.connection.device.channel==4&&job_counts[DSR_JOB_BIND]==before+1);CHECK(!strcmp(submitted.connection.device.name,local_name));
 tap(238,444);CHECK(job_counts[DSR_JOB_BIND]==before+1);CHECK(lv_obj_has_state(save,LV_STATE_DISABLED));shot("add-save-verifying",theme);
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_voltage=true,.voltage_v=231.8,.has_current=true,.current_a=.041}};response.device.online=response.device.has_switch=response.device.has_power=true;response.device.power_w=9.5;deliver(DSR_OK,DSR_WIFI_IDLE);
 bool pending_save;char status[96];dsr_workspace_status(&pending_save,status);CHECK(pending_save&&dsr_network_bind_busy()&&publish_jobs==published);
 test_workspace_finish(false);test_workspace_defer(false);for(unsigned i=0;i<12&&dsr_network_bind_busy();i++){tick();}CHECK(!dsr_network_bind_busy()&&!in_flight&&publish_jobs==published);
 char *after_document=dsr_workspace_snapshot();CHECK(after_document&&!strcmp(before_document,after_document));free(after_document);free(before_document);
 CHECK(find_label(lv_screen_active(),dsr_network_bind_result_text(DSR_IO,DSR_BIND_WORKSPACE)));CHECK(!lv_obj_has_state(save,LV_STATE_DISABLED));lan_form_notice(lv_obj_get_parent(field),dsr_network_bind_result_text(DSR_IO,DSR_BIND_WORKSPACE));shot("add-save-workspace-error",theme);
 add_save_fields(&field,&room_field,&channel_field);CHECK(!strcmp(lv_textarea_get_text(field),local_name)&&lv_dropdown_get_selected(room_field)==1&&lv_dropdown_get_selected(channel_field)==1);
 hold_maintenance=true;tap(238,444);CHECK(in_flight&&submitted.kind==DSR_JOB_BIND);
 response=(dsr_event_t){.device=submitted.connection.device,.telemetry={.has_voltage=true,.voltage_v=231.8,.has_current=true,.current_a=.041}};response.device.online=response.device.has_switch=response.device.has_power=true;response.device.power_w=9.5;deliver(DSR_OK,DSR_WIFI_IDLE);
 for(unsigned i=0;i<12&&!maintenance_ready;i++){tick();}
 CHECK(maintenance_ready&&dsr_network_bind_busy()&&publish_jobs==published+1);shot("add-save-publishing",theme);
 r=room_document();bool found=false,previous=false;const cJSON *entry;cJSON_ArrayForEach(entry,cJSON_GetObjectItemCaseSensitive(r,"devices")){
  const char *label=cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry,"name"));if(label&&!strcmp(label,"Licht Flur"))previous=true;
  if(label&&!strcmp(label,local_name)){CHECK(cJSON_GetObjectItemCaseSensitive(entry,"channel")->valueint==4&&cJSON_GetObjectItemCaseSensitive(entry,"room")->valueint==1);found=true;}
 }CHECK(found&&previous);cJSON_Delete(r);
 hold_maintenance=false;tick();CHECK(!dsr_network_bind_busy()&&dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_HOME&&dsr_sc_style()==theme);
 lv_obj_t *added=find_label(lv_screen_active(),local_name);CHECK(added);lv_obj_scroll_to_view(lv_obj_get_parent(added),LV_ANIM_OFF);shot("add-save-success",theme);
 add_save_idle();dsr_ui_lan_adopt(&device,&caps);add_save_fields(&field,&room_field,&channel_field);lv_textarea_set_text(field,local_name);lv_dropdown_set_selected(room_field,1);lv_dropdown_set_selected(channel_field,1);
 tap(238,444);CHECK(in_flight&&submitted.kind==DSR_JOB_BIND);response=(dsr_event_t){0};deliver(DSR_AUTH_REQUIRED,DSR_WIFI_IDLE);CHECK(find_label(lv_screen_active(),"Błąd logowania. Sprawdź dane."));lan_form_notice(lv_obj_get_parent(find_type(lv_screen_active(),&lv_textarea_class)),"Błąd logowania. Sprawdź dane.");shot("add-save-auth",theme);
 click("Sprawdź");test_lan_inspected(DSR_OK,"{\"switch:1\":{\"id\":1,\"output\":false},\"switch:4\":{\"id\":4,\"output\":false}}");dsr_ui_lan_tick();CHECK(find_label(lv_screen_active(),"Nazwa i pomieszczenie"));
 add_save_fields(&field,&room_field,&channel_field);shot("add-save-after-auth",theme);
 if(strcmp(lv_textarea_get_text(field),local_name)||lv_dropdown_get_selected(room_field)!=1||lv_dropdown_get_selected(channel_field)!=1){add_save_draft_issues++;printf("ADD issue theme=%u: reauthentication loses name, room or channel draft\n",theme);}
 /* A later API response may remove a channel: use that new list, not the draft. */
 tap(238,444);CHECK(in_flight&&submitted.kind==DSR_JOB_BIND);response=(dsr_event_t){0};deliver(DSR_AUTH_REQUIRED,DSR_WIFI_IDLE);
 click("Sprawdź");test_lan_inspected(DSR_OK,"{\"switch:1\":{\"id\":1,\"output\":false}}");dsr_ui_lan_tick();add_save_fields(&field,&room_field,&channel_field);
 CHECK(lv_dropdown_get_option_count(channel_field)==1&&lv_dropdown_get_selected(channel_field)==0&&!strcmp(lv_textarea_get_text(field),device.name));
 dsr_lan_device_t another=device;strcpy(another.id,"shellyplus1-112233445566");strcpy(another.name,"New Shelly");dsr_lan_capabilities_t unsupported={0};dsr_ui_lan_adopt(&another,&unsupported);
 save=lv_obj_get_parent(find_label(lv_screen_active(),"Zapisz"));CHECK(save&&lv_obj_has_state(save,LV_STATE_DISABLED));field=find_type(lv_screen_active(),&lv_textarea_class);CHECK(field&&!strcmp(lv_textarea_get_text(field),another.name));lan_form_notice(lv_obj_get_parent(field),"Urządzenie rozpoznane. Sterowanie tym typem nie jest wdrożone.");shot("add-save-unsupported",theme);
 dsr_ui_open_settings();fflush(stdout);
}
static void config_review_press(const char *text)
{
 lv_obj_t *root=lv_screen_active();
 for(unsigned i=lv_obj_get_child_count(root);i>0;i--){lv_obj_t *label=find_label(lv_obj_get_child(root,(int)i-1),text);if(!label)continue;lv_obj_t *button=lv_obj_get_parent(label);lv_obj_scroll_to_view(button,LV_ANIM_OFF);lv_area_t a=rect(button);CHECK(a.y1>=0&&a.y2<480);tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);return;}
 CHECK(false);
}
static void config_review_saved(void){config_review_press("Zapisz");dsr_ui_workspace_tick();}
static void config_review_fixture(unsigned theme)
{
 add_save_idle();test_workspace_photos(false);cJSON *r=room_document();
 cJSON_ReplaceItemInObjectCaseSensitive(r,"theme",cJSON_CreateString(theme==3?"cyber_blue":theme==2?"light":"graphite"));cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString("de"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"rooms",cJSON_Parse("[{\"id\":1,\"name\":\"Wohnzimmer und offene Küche\",\"icon\":0,\"photo\":\"\"},{\"id\":2,\"name\":\"Gästezimmer\",\"icon\":2,\"photo\":\"\"}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"devices",cJSON_Parse("[{\"id\":1,\"name\":\"Deckenlicht Küche\",\"kind\":\"relay\",\"host\":\"192.0.2.20\",\"channel\":0,\"room\":1}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"dashboards",cJSON_Parse("[{\"id\":1,\"name\":\"Mein Dashboard\",\"tiles\":[{\"id\":1,\"type\":\"room\",\"ref\":1,\"span\":1,\"height\":1,\"fields\":255},{\"id\":2,\"type\":\"device\",\"ref\":1,\"span\":2,\"height\":1,\"fields\":255}]},{\"id\":2,\"name\":\"Obergeschoss\",\"tiles\":[]}]"));
 cJSON_ReplaceItemInObjectCaseSensitive(r,"groups",cJSON_CreateArray());cJSON_ReplaceItemInObjectCaseSensitive(r,"scenes",cJSON_CreateArray());cJSON_ReplaceItemInObjectCaseSensitive(r,"active_dashboard",cJSON_CreateNumber(1));publish_document(r);
 dsr_network_rooms();CHECK(dsr_sc_style()==theme);
}
static void room_dashboard_review(unsigned theme)
{
 config_review_fixture(theme);cJSON *r;shot("config-rooms",theme);
 char *before=dsr_workspace_snapshot();CHECK(before);dsr_ui_add_room();CHECK(find_label(lv_screen_active(),"Nowe pomieszczenie"));room_name("Arbeitszimmer");config_review_press("Wybierz zdjęcie z pamięci");shot("config-room-new",theme);
 test_workspace_defer(true);config_review_saved();CHECK(find_label(lv_screen_active(),"Zapisywanie…"));test_workspace_finish(false);dsr_ui_workspace_tick();CHECK(find_label(lv_screen_active(),"Zapis nie powiódł się. Zmiany pozostają w edytorze."));shot("config-room-save-error",theme);
 char *after=dsr_workspace_snapshot();CHECK(after&&!strcmp(before,after));free(before);free(after);
 config_review_press("Zapisz");test_workspace_finish(true);dsr_ui_workspace_tick();test_workspace_defer(false);CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS&&find_label(lv_screen_active(),"Arbeitszimmer"));
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==3&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"devices"))==1);cJSON_Delete(r);
 dsr_ui_edit_room(3);room_name("Nicht speichern");config_review_press("Wstecz");CHECK(find_label(lv_screen_active(),"Odrzucić zmiany?"));shot("config-room-discard",theme);config_review_press("Odrzuć");
 dsr_ui_edit_room(3);CHECK(!strcmp(lv_textarea_get_text(find_type(lv_screen_active(),&lv_textarea_class)),"Arbeitszimmer"));room_name("Arbeitszimmer oben");shot("config-room-edit",theme);config_review_saved();CHECK(dsr_mobile_page()==DSR_MOBILE_ROOMS&&find_label(lv_screen_active(),"Arbeitszimmer oben"));
 dsr_ui_edit_room(1);room_delete_button();CHECK(find_label(lv_screen_active(),"Pomieszczenie jest używane"));shot("config-room-delete-blocked",theme);config_review_press("Wstecz");config_review_press("Wstecz");
 dsr_ui_edit_room(2);room_delete_button();CHECK(find_label(lv_screen_active(),"Usunąć pomieszczenie?"));shot("config-room-delete-confirm",theme);config_review_press("Anuluj");room_delete_button();config_review_press("Usuń");
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==3);cJSON_Delete(r);config_review_saved();r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==2&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"devices"))==1);cJSON_Delete(r);
 dsr_ui_select_dashboard();shot("config-dashboard-picker",theme);config_review_press("Obergeschoss");dsr_ui_workspace_tick();CHECK(dsr_mobile_active()&&dsr_mobile_page()==DSR_MOBILE_HOME);
 p0_generic("Pulpity",NULL);shot("config-dashboard-list",theme);config_review_press("+ Dodaj wpis");room_name("Abend");shot("config-dashboard-new",theme);config_review_press("+ Pomieszczenie");shot("config-dashboard-tiles",theme);config_review_saved();
 CHECK(find_label(lv_screen_active(),"Abend"));config_review_press("Abend");room_name("Abend oben");config_review_saved();CHECK(find_label(lv_screen_active(),"Abend oben"));
 config_review_press("Obergeschoss");p0_visible_delete("Usuń pulpit");p0_modal("Obergeschoss");shot("config-dashboard-delete-confirm",theme);config_review_press("Anuluj");p0_visible_delete("Usuń pulpit");config_review_press("Usuń");
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"dashboards"))==3&&cJSON_GetObjectItemCaseSensitive(r,"active_dashboard")->valueint==2);cJSON_Delete(r);config_review_saved();
 r=room_document();CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"dashboards"))==2&&cJSON_GetObjectItemCaseSensitive(r,"active_dashboard")->valueint==1);CHECK(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"devices"))==1&&cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r,"rooms"))==2);cJSON_Delete(r);
 config_review_press("Abend oben");p0_visible_delete("Usuń pulpit");config_review_press("Usuń");config_review_saved();config_review_press("Mein Dashboard");p0_visible_delete("Usuń pulpit");CHECK(find_label(lv_screen_active(),"Zachowaj przynajmniej jeden pulpit."));shot("config-dashboard-delete-last",theme);
 dsr_ui_open_settings();fflush(stdout);
}
static unsigned config_long_issues;
static void room_dashboard_long_review(unsigned theme)
{
 config_review_fixture(theme);
 for(unsigned language=0;language<3;language++){
  cJSON *r=room_document();const char *long_name="Wohnzimmer und offene Küche im Erdgeschoss";CHECK(strlen(long_name)<=47);
  cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"rooms")->child,"name",cJSON_CreateString(long_name));cJSON_ReplaceItemInObjectCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"dashboards")->child,"name",cJSON_CreateString("Startbereich"));cJSON_ReplaceItemInObjectCaseSensitive(r,"language",cJSON_CreateString(language==0?"pl":language==1?"en":"de"));publish_document(r);dsr_sc_appearance(DSR_SC_BLUE,theme,language);
  p0_generic("Pulpity","Startbereich");char title[100];snprintf(title,sizeof(title),"%s • %s",long_name,dsr_sc_tr("standard"));lv_obj_t *label=find_label(lv_screen_active(),title),*object=find_label(lv_screen_active(),"Obiekt");CHECK(label&&object);lv_area_t a=rect(label),b=rect(lv_obj_get_parent(object));
  if(a.y2>=b.y1){config_long_issues++;fprintf(stderr,"CONFIG issue theme=%u language=%u: long tile title overlaps its controls\n",theme,language);}
  if(language==2)shot("config-dashboard-long",theme);
  lv_obj_t *down=lv_obj_get_parent(find_label(lv_screen_active(),"Dół"));char next_title[100];snprintf(next_title,sizeof(next_title),"Deckenlicht Küche • %s",dsr_sc_tr("standard"));CHECK(rect(down).y2<rect(find_label(lv_screen_active(),next_title)).y1);
  lv_obj_t *last=lv_obj_get_parent(find_label(lv_screen_active(),"+ Grupa"));lv_obj_scroll_to_view(last,LV_ANIM_OFF);lv_area_t control=rect(last),viewport=rect(lv_obj_get_parent(last));CHECK(control.y1>=viewport.y1&&control.y2<=viewport.y2&&control.y2<420);
  dsr_ui_open_settings();
 }
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 const unsigned themes[]={0,2,3};
 if(getenv("DSR_UI_FOCUS_CONFIG_LONG")){for(unsigned i=0;i<3;i++)room_dashboard_long_review(themes[i]);CHECK(config_long_issues==0);}
 else if(getenv("DSR_UI_FOCUS_ROOM_DASHBOARD")){for(unsigned i=0;i<3;i++)room_dashboard_review(themes[i]);}
 else if(getenv("DSR_UI_FOCUS_CARD_NOTICE")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)dashboard_notice_review(themes[i],language);CHECK(card_notice_issues==0);}
 else if(getenv("DSR_UI_FOCUS_ADD_SAVE")){for(unsigned i=0;i<3;i++)add_save_review(themes[i]);CHECK(add_save_draft_issues==0);}
 else if(getenv("DSR_UI_FOCUS_RELAY_SPACING")){for(unsigned language=0;language<3;language++)relay_spacing_review(language);}
 else if(getenv("DSR_UI_FOCUS_NAVIGATION")){for(unsigned i=0;i<3;i++)navigation_review(themes[i]);CHECK(navigation_issues==0);}
 else if(getenv("DSR_UI_FOCUS_STATUS_NAME")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)status_lighting_review(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_STATUS")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)status_review(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_LIGHTING")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)lighting_review(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_SETTINGS")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)settings_review(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_DASHBOARD")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)dashboard_review(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_CONTACT")){for(unsigned i=0;i<3;i++)contact_cards(themes[i]);}
 else if(getenv("DSR_UI_FOCUS_TRV")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)thermostat_card(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_COVER_CARD")){for(unsigned i=0;i<3;i++)cover_cards(themes[i]);}
 else if(getenv("DSR_UI_FOCUS_COVER")){for(unsigned i=0;i<3;i++)cover_skin(themes[i]);}
 else if(getenv("DSR_UI_FOCUS_ONBOARDING")){for(unsigned i=0;i<3;i++)onboarding_review(themes[i]);onboarding_labels();}
 else if(getenv("DSR_UI_FOCUS_ONBOARDING_NEXT")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)onboarding_next(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_LAN_FORM")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)lan_form(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_HEADER_BACK")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)header_back_review(themes[i],language);}
 else if(getenv("DSR_UI_FOCUS_ONBOARDING_STATES")||getenv("DSR_UI_FOCUS_AP_STATES")){for(unsigned i=0;i<3;i++)for(unsigned language=0;language<3;language++)onboarding_states(themes[i],language);}
 else{if(!getenv("DSR_UI_FOCUS_BLE"))for(unsigned i=0;i<3;i++){common(themes[i]);dashboard(themes[i]);access_point(themes[i]);cover_skin(themes[i]);onboarding_next(themes[i],2);lan_form(themes[i],2);header_back_review(themes[i],2);onboarding_states(themes[i],2);}onboarding_labels();}
 printf("PASS: dev069 focused UI, %u assertions, %u rendered frames; host providers\n",checks,screenshots);
 lv_deinit();return 0;
}
