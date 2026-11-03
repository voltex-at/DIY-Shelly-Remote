/* Host render-cost profile of the three skins on production screens, with the
 * device's 320x20 partial buffer. Times are host CPU, not ESP32-S3 latency;
 * image pixels show Flash/PSRAM reads the host does not model. The checks use
 * deterministic counts only: shapes outside the band and per-frame tinting of
 * the Cyber Blue art must not come back. */
#define _POSIX_C_SOURCE 200809L
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "src/draw/lv_draw_private.h"
#include "src/misc/lv_area_private.h"
#include "src/core/lv_obj_class_private.h"
#include "../../components/dsr_ui/dsr_ui_brand.h"
#include "../../components/dsr_ui/dsr_ui_cyber.h"

enum {K_FILL,K_BORDER,K_SHADOW,K_IMAGE,K_LABEL,K_ARC,K_LINE,K_LAYER,K_TRIANGLE,K_MASK,K_COUNT};
static const char *const kind_names[K_COUNT]={"fill","border","shadow","image","label","arc","line","layer","triangle","mask"};
static double spent[K_COUNT],started[8];
static unsigned long calls[K_COUNT],tasks,idle_tasks,image_px,background_px,recolor_px,background_recolor_px;
static int stack[8];
static unsigned depth;
static const char *idle_scene; /* DSR_RENDER_IDLE=<scene> lists idle task areas */
static double now_ms(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec*1e3+ts.tv_nsec/1e6;}
/* Exclusive time: a nested primitive (round line caps are fills) pauses its
 * caller. DSR_RENDER_TOP=<scene> also lists it per drawing object; shapes
 * added from draw events carry no object. */
typedef struct {lv_obj_t *obj;int kind;double ms;unsigned long n;} cost_t;
static cost_t costs[2048];
static unsigned cost_count;
static lv_obj_t *owner[8];
static void credit(lv_obj_t *obj,int kind,double ms,bool call)
{
 unsigned i=0;while(i<cost_count&&(costs[i].obj!=obj||costs[i].kind!=kind))i++;
 if(i==cost_count){if(cost_count==sizeof(costs)/sizeof(costs[0]))return;costs[cost_count++]=(cost_t){obj,kind,0,0};}
 costs[i].ms+=ms;if(call)costs[i].n++;
}
static void enter(int kind,const void *dsc)
{
 double t=now_ms();lv_obj_t *obj=((const lv_draw_dsc_base_t *)dsc)->obj;
 if(depth){spent[stack[depth-1]]+=t-started[depth-1];credit(owner[depth-1],stack[depth-1],t-started[depth-1],false);}
 stack[depth]=kind;owner[depth]=obj;started[depth]=t;depth++;calls[kind]++;credit(obj,kind,0,true);
}
static void leave(void)
{
 double t=now_ms();depth--;spent[stack[depth]]+=t-started[depth];credit(owner[depth],stack[depth],t-started[depth],false);
 if(depth)started[depth-1]=t;
}
#define WRAP(name,kind,params,args) void __real_##name params;void __wrap_##name params;void __wrap_##name params{enter(kind,d);__real_##name args;leave();}
WRAP(lv_draw_sw_fill,K_FILL,(lv_draw_task_t *t,lv_draw_fill_dsc_t *d,const lv_area_t *a),(t,d,a))
WRAP(lv_draw_sw_border,K_BORDER,(lv_draw_task_t *t,const lv_draw_border_dsc_t *d,const lv_area_t *a),(t,d,a))
WRAP(lv_draw_sw_box_shadow,K_SHADOW,(lv_draw_task_t *t,const lv_draw_box_shadow_dsc_t *d,const lv_area_t *a),(t,d,a))
WRAP(lv_draw_sw_label,K_LABEL,(lv_draw_task_t *t,const lv_draw_label_dsc_t *d,const lv_area_t *a),(t,d,a))
WRAP(lv_draw_sw_arc,K_ARC,(lv_draw_task_t *t,const lv_draw_arc_dsc_t *d,const lv_area_t *a),(t,d,a))
WRAP(lv_draw_sw_line,K_LINE,(lv_draw_task_t *t,const lv_draw_line_dsc_t *d),(t,d))
WRAP(lv_draw_sw_layer,K_LAYER,(lv_draw_task_t *t,const lv_draw_image_dsc_t *d,const lv_area_t *a),(t,d,a))
WRAP(lv_draw_sw_triangle,K_TRIANGLE,(lv_draw_task_t *t,const lv_draw_triangle_dsc_t *d),(t,d))
WRAP(lv_draw_sw_mask_rect,K_MASK,(lv_draw_task_t *t,const lv_draw_mask_rect_dsc_t *d),(t,d))
void __real_lv_draw_sw_image(lv_draw_task_t *t,const lv_draw_image_dsc_t *d,const lv_area_t *a);
void __wrap_lv_draw_sw_image(lv_draw_task_t *t,const lv_draw_image_dsc_t *d,const lv_area_t *a);
void __wrap_lv_draw_sw_image(lv_draw_task_t *t,const lv_draw_image_dsc_t *d,const lv_area_t *a)
{
 lv_area_t read;
 if(lv_area_intersect(&read,a,&t->clip_area)){
  unsigned long px=(unsigned long)lv_area_get_size(&read);image_px+=px;
  bool background=d->src==dsr_ui_brand_background()||d->src==dsr_ui_cyber_backdrop();
  if(background)background_px+=px;
  if(d->recolor_opa>LV_OPA_MIN){recolor_px+=px;if(background)background_recolor_px+=px;}
 }
 enter(K_IMAGE,d);__real_lv_draw_sw_image(t,d,a);leave();
}
lv_draw_task_t *__real_lv_draw_add_task(lv_layer_t *layer,const lv_area_t *coords,lv_draw_task_type_t type);
lv_draw_task_t *__wrap_lv_draw_add_task(lv_layer_t *layer,const lv_area_t *coords,lv_draw_task_type_t type);
lv_draw_task_t *__wrap_lv_draw_add_task(lv_layer_t *layer,const lv_area_t *coords,lv_draw_task_type_t type)
{
 /* A task outside the band being redrawn draws nothing but still costs an
  * allocation, a list walk and dispatch (the board renders 20-row bands). */
 lv_area_t common;tasks++;if(!lv_area_intersect(&common,coords,&layer->_clip_area)){idle_tasks++;if(idle_scene&&getenv("DSR_RENDER_IDLE")&&!strcmp(getenv("DSR_RENDER_IDLE"),idle_scene))fprintf(stderr,"idle type %d area %d,%d-%d,%d clip y %d-%d\n",(int)type,(int)coords->x1,(int)coords->y1,(int)coords->x2,(int)coords->y2,(int)layer->_clip_area.y1,(int)layer->_clip_area.y2);}
 return __real_lv_draw_add_task(layer,coords,type);
}

static void reset(void){memset(spent,0,sizeof(spent));memset(calls,0,sizeof(calls));tasks=idle_tasks=image_px=background_px=recolor_px=background_recolor_px=0;cost_count=0;}
static int by_cost(const void *a,const void *b){double x=((const cost_t *)a)->ms,y=((const cost_t *)b)->ms;return (x<y)-(x>y);}
static int by_value(const void *a,const void *b){double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);}
static unsigned runs(void){const char *v=getenv("DSR_RENDER_RUNS");int n=v?atoi(v):25;return n<3?3:n>200?200:(unsigned)n;}
static const char *skin_names[]={"graphite","light","cyber_blue"};
static unsigned skin_at;
/* paint() repeats one refresh; the change callback dirties the same area each time. */
static void paint(const char *scene,void (*change)(unsigned))
{
 unsigned n=runs();double samples[200];
 lv_obj_update_layout(lv_screen_active());
 for(unsigned i=0;i<3;i++){change(i);lv_refr_now(display);}
 reset();idle_scene=scene;
 for(unsigned i=0;i<n;i++){change(i+3);double t=now_ms();lv_refr_now(display);samples[i]=now_ms()-t;}
 qsort(samples,n,sizeof(samples[0]),by_value);
 /* The final frame, painted in 20-row bands, for pixel comparison of changes. */
 char path[512];snprintf(path,sizeof(path),"%s/cost-%s-%s.ppm",output,skin_names[skin_at],scene);
 FILE *f=fopen(path,"wb");CHECK(f);fprintf(f,"P6\n320 480\n255\n");CHECK(fwrite(canvas,1,sizeof(canvas),f)==sizeof(canvas));fclose(f);
 /* Left over: LVGL's own scrollbar and outline margins, at most one per band. */
 if(idle_tasks/n>32)fprintf(stderr,"%s %s: %lu idle tasks per refresh\n",skin_names[skin_at],scene,idle_tasks/n);
 CHECK(idle_tasks/n<=32);
 if(background_px)CHECK(!background_recolor_px); /* Cyber Blue art is tinted once */
 double draw=0;for(unsigned k=0;k<K_COUNT;k++)draw+=spent[k];
 printf("%-10s %-14s min %6.3f median %6.3f ms  tasks %5lu (idle %3lu)  image px %6lu (background %6lu, recolored %6lu)  draw %6.3f ms:",
  skin_names[skin_at],scene,samples[0],samples[n/2],tasks/n,idle_tasks/n,image_px/n,background_px/n,recolor_px/n,draw/n);
 for(unsigned k=0;k<K_COUNT;k++)if(calls[k])printf(" %s %.3f/%lu",kind_names[k],spent[k]/n,calls[k]/n);
 printf("\n");
 const char *top=getenv("DSR_RENDER_TOP");
 if(!top||strcmp(top,scene))return;
 qsort(costs,cost_count,sizeof(costs[0]),by_cost);
 for(unsigned i=0;i<cost_count&&i<24;i++){
  const cost_t *c=&costs[i];lv_area_t a={0};const char *cls="event decoration";
  if(c->obj){lv_obj_get_coords(c->obj,&a);cls=lv_obj_get_class(c->obj)->name?lv_obj_get_class(c->obj)->name:"?";}
  printf("   %7.3f ms %5lu %-6s %-16s %4d,%4d %3dx%3d",c->ms/n,c->n/n,kind_names[c->kind],cls,(int)a.x1,(int)a.y1,(int)lv_area_get_width(&a),(int)lv_area_get_height(&a));
  if(c->obj)printf(" radius %d border %d/%u",(int)lv_obj_get_style_radius(c->obj,0),(int)lv_obj_get_style_border_width(c->obj,0),(unsigned)lv_obj_get_style_border_opa(c->obj,0));
  printf("\n");
 }
}
static void whole(unsigned i){(void)i;lv_obj_invalidate(lv_screen_active());}
static lv_obj_t *scroller(lv_obj_t *o)
{
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_scroll_bottom(o)+lv_obj_get_scroll_top(o)>=80)return o;
 for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *found=scroller(lv_obj_get_child(o,(int32_t)i));if(found)return found;}
 return NULL;
}
static lv_obj_t *scrolled;
static void scroll_step(unsigned i){lv_obj_scroll_by(scrolled,0,i%2?40:-40,LV_ANIM_OFF);}
static dsr_mobile_model_t live;
/* Screen construction alone (objects, styles, layout), as on a tab change. */
static void build(const char *scene,dsr_mobile_page_t page)
{
 unsigned n=runs();double samples[200];
 for(unsigned i=0;i<n+3;i++){
  dsr_mobile_show(DSR_MOBILE_DEVICES,0);lv_refr_now(display);
  double t=now_ms();dsr_mobile_show(page,0);lv_obj_update_layout(lv_screen_active());if(i>=3)samples[i-3]=now_ms()-t;
  lv_refr_now(display);
 }
 qsort(samples,n,sizeof(samples[0]),by_value);
 printf("%-10s %-14s min %6.3f median %6.3f ms  build only\n",skin_names[skin_at],scene,samples[0],samples[n/2]);
}
static void card_value(unsigned i){live.devices[0].power_w=(float)(18+i%5);dsr_mobile_update(&live);}

static void act_index_bool(unsigned i,bool on){(void)i;(void)on;}
static void act_void(void){}
static void act_index_int(unsigned i,int d){(void)i;(void)d;}
static void act_id(uint32_t id){(void)id;}
static void act_index(unsigned i){(void)i;}
static void act_index_u8(unsigned i,uint8_t v){(void)i;(void)v;}
static void act_kind(uint8_t k){(void)k;}
static void act_kind_id(uint8_t k,uint32_t id){(void)k;(void)id;}
static void act_id_bool(uint32_t id,bool on){(void)id;(void)on;}
static void act_rgb(unsigned i,uint8_t r,uint8_t g,uint8_t b){(void)i;(void)r;(void)g;(void)b;}
static const dsr_mobile_actions_t stub_actions={.set_output=act_index_bool,.open_settings=act_void,.add_device=act_void,.edit_dashboard=act_void,
 .set_cover=act_index_int,.select_dashboard=act_void,.refresh=act_id,.edit_device=act_id,.stop_cover=act_index,.set_position=act_index_u8,
 .add_room=act_void,.edit_room=act_id,.open_usage=act_id,.open_battery=act_void,.open_actions=act_kind,.leave_actions=act_void,
 .edit_action=act_kind_id,.run_scene=act_id,.run_group=act_id_bool,.set_brightness=act_index_u8,.set_rgb=act_rgb,.set_trv=act_index_u8,.device_settings=act_id};
/* A typical home: relays, covers, dimmer, RGB, H&T, contact and TRV. */
static dsr_mobile_model_t home_model(void)
{
 dsr_mobile_model_t m={.wifi=true,.home_name="Mój dom",.room_count=2,.rooms={{.name="Salon",.id=1},{.name="Sypialnia",.id=2}}};
 uint32_t now=lv_tick_get();unsigned n=0;dsr_mobile_device_t *d=m.devices;
#define BASE .generation=DSR_GEN2_PLUS,.can_refresh=true,.fields=255,.has_sample_time=true,.sample_ms=now
 d[n++]=(dsr_mobile_device_t){.id=1,.name="Lampa salon",.identity="cost-1",.endpoint_kind=DSR_ENDPOINT_SWITCH,.kind=DSR_DEVICE_RELAY,.room=0,.online=true,.output=true,.can_switch=true,.has_power=true,.power_w=18.6,BASE};
 d[n++]=(dsr_mobile_device_t){.id=2,.name="Roleta salon",.identity="cost-2",.endpoint_kind=DSR_ENDPOINT_COVER,.kind=DSR_DEVICE_COVER,.room=0,.online=true,.can_cover=true,.can_position=true,.can_stop_cover=true,.has_position=true,.position_pct=45,.cover_state=DSR_COVER_STOPPED,BASE};
 d[n++]=(dsr_mobile_device_t){.id=3,.name="Roleta sypialnia",.identity="cost-3",.endpoint_kind=DSR_ENDPOINT_COVER,.kind=DSR_DEVICE_COVER,.room=1,.online=true,.can_cover=true,.can_position=true,.can_stop_cover=true,.has_position=true,.position_pct=100,.cover_state=DSR_COVER_STOPPED,BASE};
 d[n++]=(dsr_mobile_device_t){.id=4,.name="Ściemniacz jadalnia",.identity="cost-4",.endpoint_kind=DSR_ENDPOINT_LIGHT,.kind=DSR_DEVICE_LIGHT,.room=0,.online=true,.output=true,.can_switch=true,.can_light=true,.light={.brightness=75,.flags=DSR_LIGHT_VALID|DSR_LIGHT_BRIGHTNESS},BASE};
 d[n++]=(dsr_mobile_device_t){.id=5,.name="Taśma LED",.identity="cost-5",.endpoint_kind=DSR_ENDPOINT_RGB,.kind=DSR_DEVICE_RGB,.room=1,.online=true,.output=true,.can_switch=true,.can_rgb=true,.rgb={.r=0,.g=180,.b=255,.brightness=78,.flags=DSR_RGB_VALID},BASE};
 d[n++]=(dsr_mobile_device_t){.id=6,.name="Termometr H&T",.identity="cost-6",.endpoint_kind=DSR_ENDPOINT_HT,.kind=DSR_DEVICE_SENSOR,.room=1,.online=true,.has_temperature=true,.temperature_c=21.4,.has_humidity=true,.humidity_pct=46,.has_battery=true,.battery_pct=85,.ht={.flags=DSR_HT_VALID},BASE};
 d[n++]=(dsr_mobile_device_t){.id=7,.name="Drzwi tarasowe",.identity="cost-7",.endpoint_kind=DSR_ENDPOINT_BLU,.kind=DSR_DEVICE_CONTACT,.room=0,.online=true,.has_contact=true,.has_battery=true,.battery_pct=72,.blu_age_s=12,.blu={.flags=DSR_BLU_VALID|DSR_BLU_HAS_OPEN},BASE};
 d[n++]=(dsr_mobile_device_t){.id=8,.name="Grzejnik salon",.identity="cost-8",.endpoint_kind=DSR_ENDPOINT_BLU,.kind=DSR_DEVICE_SENSOR,.room=0,.online=true,.has_temperature=true,.temperature_c=20.4,.has_trv_target=true,.trv_target_c=21.5,.has_battery=true,.battery_pct=64,.blu_age_s=30,.blu={.flags=DSR_BLU_VALID,.trv_pos=35,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET|DSR_BLU_TRV_CHECKED},BASE};
#undef BASE
 m.device_count=(uint8_t)n;return m;
}
static void skin(unsigned at)
{
 skin_at=at;cJSON *doc=room_document();cJSON_ReplaceItemInObjectCaseSensitive(doc,"theme",cJSON_CreateString(skin_names[at]));
 cJSON_ReplaceItemInObjectCaseSensitive(doc,"language",cJSON_CreateString("pl"));publish_document(doc);dsr_network_home(NULL);tick();
}
int main(int argc,char **argv)
{
 /* dsr_es3c35p.c: BUFFER_PIXELS = WIDTH * 20; DSR_RENDER_ROWS compares other bands. */
 static uint16_t band[320*80];const char *rows_env=getenv("DSR_RENDER_ROWS");
 unsigned rows=rows_env?(unsigned)atoi(rows_env):20;if(rows<10||rows>80)rows=20;
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,band,NULL,320*rows*2,LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=true;test_workspace_enable();dsr_ui_init();tick();
 const char *only=getenv("DSR_RENDER_SKIN");
 for(unsigned at=0;at<3;at++){
  if(only&&*only&&strcmp(only,skin_names[at]))continue;
  skin(at);dsr_ui_open_settings();paint("settings",whole);
  dsr_mobile_bind(&stub_actions);live=home_model();dsr_mobile_update(&live);
  build("build-home",DSR_MOBILE_HOME);build("build-rooms",DSR_MOBILE_ROOMS);
  dsr_mobile_show(DSR_MOBILE_HOME,0);paint("home",whole);paint("home-card",card_value);
  scrolled=scroller(lv_screen_active());CHECK(scrolled);paint("home-scroll",scroll_step);
  dsr_mobile_show(DSR_MOBILE_DEVICES,0);paint("devices",whole);
  dsr_mobile_show(DSR_MOBILE_ROOMS,0);paint("rooms",whole);
  dsr_mobile_show(DSR_MOBILE_DETAILS,1);paint("cover",whole);
  dsr_mobile_show(DSR_MOBILE_DETAILS,0);paint("relay",whole);
  dsr_mobile_show(DSR_MOBILE_DETAILS,3);paint("dimmer",whole);
  dsr_mobile_show(DSR_MOBILE_DETAILS,7);paint("trv",whole);
  dsr_network_home(NULL);tick();
 }
 printf("PASS: render-cost profile, %u runs per scene, 320x20 band; host CPU only\n",runs());
 lv_deinit();return 0;
}
