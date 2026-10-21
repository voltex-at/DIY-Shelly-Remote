/* Actual LVGL/configuration/fleet. The linker counts real cJSON_Parse calls;
 * the workspace fixture retains the production O(1) revision contract. */
#define main original_ui_main
#include "test_ui.c"
#undef main
static unsigned parses;
cJSON *__real_cJSON_Parse(const char *text);
cJSON *__wrap_cJSON_Parse(const char *text){parses++;return __real_cJSON_Parse(text);}
static struct {void *pointer;size_t bytes;} allocations[64];
static size_t calloc_bytes;
static bool fail_arena;
static unsigned arena_failures;
void *__real_calloc(size_t count,size_t bytes);
void __real_free(void *pointer);
void *__wrap_calloc(size_t count,size_t bytes)
{
 if(fail_arena){arena_failures++;return NULL;}
 void *pointer=__real_calloc(count,bytes);
 if(pointer)for(unsigned i=0;i<64;i++)if(!allocations[i].pointer){allocations[i].pointer=pointer;allocations[i].bytes=count*bytes;calloc_bytes+=count*bytes;return pointer;}
 CHECK(!pointer);return NULL;
}
void __wrap_free(void *pointer)
{
 for(unsigned i=0;i<64;i++)if(pointer&&allocations[i].pointer==pointer){calloc_bytes-=allocations[i].bytes;allocations[i].pointer=NULL;break;}
 __real_free(pointer);
}
unsigned test_workspace_snapshot_calls(void);
void test_runtime_capture(bool enabled);
const dsr_runtime_device_t *test_runtime_at(size_t index);
static void add_cache_device(cJSON *root)
{
 cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(root,"devices"),cJSON_Parse("{\"id\":1,\"name\":\"Cache relay\",\"kind\":\"relay\",\"host\":\"192.0.2.20\",\"channel\":0,\"room\":0}"));
}
static void *no_json_memory(size_t size){(void)size;return NULL;}
int main(int argc,char **argv)
{
 CHECK(argc==2);output=argv[1];lv_init();display=lv_display_create(320,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_display(pointer,display);lv_indev_set_read_cb(pointer,read_pointer);
 strcpy(stored.ssid,"Home Wi-Fi");wifi.connected=false;test_workspace_enable();cJSON *root=room_document();add_cache_device(root);publish_document(root);
 boot_loaded_count=1;boot_loaded[0]=(dsr_connection_t){.layout_id=1,.device={.generation=DSR_GEN2_PLUS,.endpoint_kind=DSR_ENDPOINT_SWITCH,.host="192.0.2.20",.device_id="shelly-cache"}};
 test_runtime_capture(true);dsr_ui_init();tick();CHECK(dsr_network_layout_reserved(1)&&!in_flight&&!maintenance_ready);
 unsigned began=parses,snapshots=test_workspace_snapshot_calls();
 for(unsigned i=0;i<120;i++)tick();
 printf("Layout idle 30 s: parses=%u snapshots=%u\n",parses-began,test_workspace_snapshot_calls()-snapshots);
 CHECK(parses==began&&test_workspace_snapshot_calls()==snapshots);
 for(unsigned i=0;i<15;i++)dsr_network_home(NULL);
 CHECK(parses==began&&test_workspace_snapshot_calls()==snapshots); /* repaint reuses the same validated document */

 /* An accepted command waits behind a read. A cosmetic configuration save
  * must not silently cancel it when its exact endpoint mapping is unchanged. */
 wifi.connected=true;tick();CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 dsr_network_device_control(1);click("Odśwież stan");CHECK(in_flight&&submitted.kind==DSR_JOB_PROBE);
 lv_obj_t *power=power_target(lv_screen_active());CHECK(power);lv_obj_send_event(power,LV_EVENT_CLICKED,NULL);
 root=room_document();cJSON_ReplaceItemInObjectCaseSensitive(root,"accent",cJSON_CreateNumber(0x123456));publish_document(root);tick();
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 CHECK(in_flight&&submitted.kind==DSR_JOB_SET_OUTPUT&&submitted.on);
 response=(dsr_event_t){.device=submitted.connection.device};response.device.online=response.device.has_switch=response.device.output=true;deliver(DSR_OK,DSR_WIFI_IDLE);
 wifi.connected=false;tick();dsr_network_home(NULL);

 root=room_document();cJSON *entry=cJSON_GetObjectItemCaseSensitive(root,"devices")->child;cJSON_ReplaceItemInObjectCaseSensitive(entry,"host",cJSON_CreateString("192.0.2.99"));publish_document(root);began=parses;tick();
 CHECK(parses-began==1&&unbind_jobs==0&&dsr_network_layout_reserved(1));
 CHECK(dsr_network_device_settings_request(1,0,0,NULL)==DSR_IDENTITY_MISMATCH); /* changed locator never authorizes old binding */
 root=room_document();entry=cJSON_GetObjectItemCaseSensitive(root,"devices")->child;cJSON_ReplaceItemInObjectCaseSensitive(entry,"host",cJSON_CreateString("192.0.2.20"));publish_document(root);tick();

 unsigned unbinds=unbind_jobs;size_t retained=calloc_bytes;test_workspace_fault(1);tick();CHECK(unbind_jobs==unbinds&&dsr_network_layout_reserved(1)&&calloc_bytes<retained);test_workspace_fault(0);began=parses;tick();CHECK(parses-began==1&&calloc_bytes==retained); /* old arena released; same revision after readiness loss must be revalidated */
 test_workspace_fault(5);tick();CHECK(unbind_jobs==unbinds&&dsr_network_device_settings_request(1,0,0,NULL)==DSR_IDENTITY_MISMATCH); /* root and advertised revisions must agree */
 test_workspace_fault(0);tick();
 root=room_document();publish_document(root);test_workspace_raw_fault(2);tick();CHECK(unbind_jobs==unbinds&&dsr_network_layout_reserved(1));test_workspace_raw_fault(0);tick();

 root=room_document();cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(root,"devices"),0);publish_document(root);
 test_workspace_fault(2);tick();CHECK(unbind_jobs==unbinds&&dsr_network_layout_reserved(1));
 CHECK(dsr_network_device_settings_request(1,0,0,NULL)==DSR_IDENTITY_MISMATCH); /* no cached authorization after new revision snapshot OOM */
 test_workspace_fault(0);cJSON_Hooks hooks={.malloc_fn=no_json_memory,.free_fn=free};cJSON_InitHooks(&hooks);tick();cJSON_InitHooks(NULL);CHECK(unbind_jobs==unbinds&&dsr_network_layout_reserved(1));
 unsigned failures=arena_failures;fail_arena=true;tick();fail_arena=false;CHECK(arena_failures>failures&&unbind_jobs==unbinds&&dsr_network_layout_reserved(1));
 tick();CHECK(unbind_jobs==unbinds+1&&!dsr_network_layout_reserved(1)&&!maintenance_ready&&!in_flight);
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
 printf("PASS: %u layout cache checks; idle/repaint parse counts, changed host, readiness recovery/arena lifetime, snapshot/parser/arena OOM, exact deletion; no device I/O\n",checks);lv_deinit();return 0;
}
