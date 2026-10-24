/* Actual LVGL responsiveness and the sole fleet scheduler; radios are models. */
#define main original_ui_main
#include "test_ui.c"
#undef main
#include "dsr_fleet.h"
static dsr_mobile_model_t priority_view;
static unsigned deleted,sent;
static uint8_t target_half;
static void view_deleted(lv_event_t *e){(void)e;deleted++;}
static void target_sent(unsigned index,uint8_t half)
{CHECK(index==0);sent++;target_half=half;dsr_mobile_trv_request_state(41,half,1,true);}
static lv_obj_t *input_dial(lv_obj_t *o)
{if(lv_obj_check_type(o,&lv_arc_class)&&lv_obj_has_flag(o,LV_OBJ_FLAG_ADV_HITTEST))return o;for(unsigned i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *a=input_dial(lv_obj_get_child(o,(int)i));if(a)return a;}return NULL;}
static void thermostat(void)
{
 const unsigned skins[]={0,2,3};
 for(unsigned skin=0;skin<3;skin++)for(unsigned lang=0;lang<3;lang++){
  dsr_sc_appearance(DSR_SC_BLUE,skins[skin],lang);
  priority_view=(dsr_mobile_model_t){.wifi=true,.device_count=1};dsr_mobile_device_t *d=&priority_view.devices[0];
  *d=(dsr_mobile_device_t){.id=41,.name="TRV priority",.identity="trv-fixture",.host="192.168.1.80",.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_BLU,.kind=DSR_DEVICE_SENSOR,.online=true,.has_trv_target=true,.trv_target_c=20.5,.has_temperature=true,.temperature_c=21.5,.has_battery=true,.battery_pct=90,.blu={.flags=DSR_BLU_VALID,.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET,.trv_pos=40}};
  dsr_mobile_update(&priority_view);dsr_mobile_show(DSR_MOBILE_DETAILS,0);lv_obj_update_layout(lv_screen_active());
  CHECK(dsr_mobile_visible_devices()==1);
  lv_obj_t *title=find_label(lv_screen_active(),"TRV priority"),*dial=input_dial(lv_screen_active());CHECK(title&&dial);
  lv_obj_add_event_cb(title,view_deleted,LV_EVENT_DELETE,NULL);lv_obj_add_event_cb(dial,view_deleted,LV_EVENT_DELETE,NULL);deleted=0;
  unsigned before=sent;click("+");CHECK(!deleted&&find_label(lv_screen_active(),"21.0 °C")&&sent==before);
  lv_tick_inc(1300);lv_timer_handler();CHECK(!deleted&&sent==before+1&&target_half==42);
  d->busy=d->command_pending=true;dsr_mobile_update(&priority_view);CHECK(!deleted&&find_label(lv_screen_active(),"21.0 °C"));
  d->temperature_c=22;d->battery_pct=89;d->blu.trv_pos=45;dsr_mobile_update(&priority_view);CHECK(!deleted&&find_label(lv_screen_active(),"22.0 °C"));
  d->busy=d->command_pending=false;d->trv_target_c=21;dsr_mobile_update(&priority_view);dsr_mobile_trv_request_state(41,42,1,false);
  CHECK(!deleted&&find_label(lv_screen_active(),"21.0 °C"));
  /* Returning to the reported value is still an explicit manual override. */
  before=sent;click("+");click("-");CHECK(sent==before);
  lv_tick_inc(1300);lv_timer_handler();CHECK(sent==before+1&&target_half==42);
  dsr_mobile_trv_request_state(41,42,1,false);
  if(!lang){char text[64];snprintf(text,sizeof(text),"priority-trv-skin%u",skins[skin]);screenshot(text);}
  /* A screen opened while the worker is busy initially has a disabled dial.
   * Completion must restore that retained input widget, not only +/- buttons. */
  d->busy=d->command_pending=true;dsr_mobile_update(&priority_view);dsr_mobile_show(DSR_MOBILE_DETAILS,0);
  dial=input_dial(lv_screen_active());CHECK(dial&&lv_obj_has_state(dial,LV_STATE_DISABLED));
  deleted=0;lv_obj_add_event_cb(dial,view_deleted,LV_EVENT_DELETE,NULL);
  d->busy=d->command_pending=false;dsr_mobile_update(&priority_view);CHECK(!deleted&&!lv_obj_has_state(dial,LV_STATE_DISABLED));
  /* Authentication/identity changes keep the existing fail-closed path. */
  d->online=false;d->error=DSR_AUTH_REQUIRED;dsr_mobile_update(&priority_view);CHECK(deleted&&find_label(lv_screen_active(),"Błąd logowania"));
  dsr_mobile_hide();lv_obj_clean(lv_screen_active());
  CHECK(!dsr_mobile_visible_devices());
 }
}
static void scheduling(void)
{
 dsr_fleet_t f={0};dsr_job_t job;unsigned at;
 for(unsigned i=0;i<3;i++){
  dsr_connection_t c={.layout_id=i+1,.device={.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_SWITCH,.online=true,.has_switch=true}};snprintf(c.device.host,sizeof(c.device.host),"192.0.2.%u",i+1);snprintf(c.device.device_id,sizeof(c.device.device_id),"fixture-%u",i);CHECK(dsr_fleet_upsert(&f,&c,0)==DSR_OK);f.slots[i].next_poll=0;
 }
 CHECK(dsr_fleet_next_poll(&f,5000,4,&job,&at)&&at==2&&job.kind==DSR_JOB_PROBE);
 CHECK(dsr_fleet_next_poll(&f,5000,3,&job,&at)&&at<2);
 f.slots[2].next_poll=60000;CHECK(!dsr_fleet_next_poll(&f,5000,4,&job,&at));
 f.slots[2].next_poll=0;f.slots[2].blocked=true;CHECK(!dsr_fleet_next_poll(&f,5000,4,&job,&at));
 f.slots[2].blocked=false;f.slots[2].urgent_stop=true;CHECK(!dsr_fleet_next_poll(&f,5000,4,&job,&at));
 f.slots[2].urgent_stop=false;CHECK(dsr_fleet_request(&f,2,true,true,5000)==DSR_OK);CHECK(!dsr_fleet_next_poll(&f,5000,4,&job,&at));CHECK(dsr_fleet_next(&f,5000,false,&job,&at)&&at==2&&job.kind==DSR_JOB_SET_OUTPUT);
 /* A successful TRV write gets one early read, while a later GET returns to
  * normal cadence and HTTP 429 retains its long backoff. */
 dsr_fleet_slot_t *s=&f.slots[2];s->queued=false;s->connection.device.endpoint_kind=DSR_ENDPOINT_BLU;s->in_flight=true;s->operation=DSR_JOB_TRV;s->request_id=9;
 s->trv=(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=40};dsr_event_t e={.id=9,.result=DSR_OK,.device=s->connection.device,.telemetry={.blu={.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET},.trv_target_c=20}};CHECK(dsr_fleet_complete(&f,2,&e,5000)&&s->next_poll==6000);
 s->in_flight=true;s->operation=DSR_JOB_PROBE;s->request_id=10;e.id=10;CHECK(dsr_fleet_complete(&f,2,&e,6000)&&s->next_poll==21000);
 /* A delayed mirror gets at most three early GETs; it is never replaced by
  * the requested target and never causes another temperature write. */
 s->trv=(dsr_trv_intent_t){.command=DSR_TRV_SET_TARGET,.half_c=43};s->in_flight=true;s->operation=DSR_JOB_TRV;s->request_id=20;e.id=20;e.telemetry.blu.errors=DSR_BLU_TRV|DSR_BLU_TRV_TARGET;e.telemetry.trv_target_c=20;
 CHECK(dsr_fleet_complete(&f,2,&e,10000)&&s->next_poll==11000&&s->connection.telemetry.trv_target_c==20);
 for(unsigned n=0;n<3;n++){CHECK(dsr_fleet_next_poll(&f,11000+n*1000,4,&job,&at)&&job.kind==DSR_JOB_PROBE);job.id=21+n;dsr_fleet_started(&f,at,&job);e.id=job.id;CHECK(dsr_fleet_complete(&f,at,&e,11000+n*1000)&&s->next_poll==(n==2?28000:12000+n*1000));}
 s->in_flight=true;s->operation=DSR_JOB_TRV;s->request_id=11;e.id=11;e.result=DSR_RATE_LIMITED;CHECK(dsr_fleet_complete(&f,2,&e,7000)&&s->next_poll>=67000);
}
static void network_priority(void)
{
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=false;test_workspace_enable();cJSON *r=room_document();
 cJSON *devices=cJSON_GetObjectItemCaseSensitive(r,"devices");
 boot_loaded_count=3;
 for(unsigned i=0;i<3;i++){
  char text[192];snprintf(text,sizeof(text),"{\"id\":%u,\"name\":\"Priority %u\",\"kind\":\"relay\",\"host\":\"192.0.2.%u\",\"channel\":0,\"room\":0}",i+1,i+1,i+1);cJSON_AddItemToArray(devices,cJSON_Parse(text));
  boot_loaded[i]=(dsr_connection_t){.layout_id=i+1,.device={.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_SWITCH}};snprintf(boot_loaded[i].device.host,sizeof(boot_loaded[i].device.host),"192.0.2.%u",i+1);snprintf(boot_loaded[i].device.device_id,sizeof(boot_loaded[i].device.device_id),"fixture-%u",i);
 }
 publish_document(r);dsr_ui_init();tick();wifi.connected=true;tick();
 for(unsigned i=0;i<3;i++){CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 CHECK(!in_flight);
 /* All three are overdue, but the ordinary cursor points to #1. Opening
  * #3 must choose it before unrelated background work. */
 dsr_network_device_control(3);CHECK(dsr_mobile_visible_devices()==4);lv_tick_inc(5000);tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.connection.layout_id==3);
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.connection.layout_id!=3); /* due background gets a turn */
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 if(in_flight){response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);}
 CHECK(!in_flight);dsr_mobile_show(DSR_MOBILE_DEVICES,0);CHECK(dsr_mobile_visible_devices()==3);lv_tick_inc(5000);tick();
 for(unsigned i=0;i<3;i++){CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE&&submitted.connection.layout_id==i+1);response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);} /* two visible, then one background */
 CHECK(!in_flight);dsr_battery_profile_defaults(&battery_stored);battery_stored.kind=DSR_BATTERY_LIPO_1S;dsr_ui_battery_init();dsr_ui_battery_show();lv_tick_inc(22000);tick();
 CHECK(in_flight&&submitted.kind==DSR_JOB_BATTERY_READ); /* active page before overdue unrelated probes */
 response=(dsr_event_t){.battery={.has_voltage=true,.bat_mv=4000}};deliver(DSR_OK,DSR_WIFI_IDLE);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 dsr_mobile_actions_t a={.set_trv=target_sent};dsr_mobile_bind(&a);thermostat();scheduling();network_priority();
 printf("PASS: %u focused priority/TRV UI checks, 3 skins x PL/EN/DE; radio and motor are not exercised\n",checks);lv_deinit();return 0;
}
