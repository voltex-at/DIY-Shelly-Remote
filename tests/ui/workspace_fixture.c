#include "dsr_system_info.h"
#include "dsr_workspace.h"
#include "dsr_activity.h"
#include "dsr_discovery.h"
#include "dsr_ble.h"
#include "dsr_ble_rpc.h"
#include "dsr_panel_pairing.h"
#include "lvgl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* Disabled for the original Wi-Fi fixture until explicitly enabled. */
static char *stored;
static char *pending;
static uint32_t stored_revision;
static unsigned snapshot_calls;
unsigned test_workspace_snapshot_calls(void){return snapshot_calls;}
static bool defer_save;
static unsigned snapshot_fault;
static bool https_ready=true;
void test_workspace_fault(unsigned fault){snapshot_fault=fault;}
void test_panel_ready(bool ready){https_ready=ready;dsr_config_health_panel(ready?ESP_OK:ESP_FAIL);}
bool dsr_panel_ready(void){return https_ready;}
void test_workspace_defer(bool enabled){defer_save=enabled;}
void test_workspace_finish(bool success){if(success){free(stored);stored=pending;stored_revision=stored?dsr_config_revision(stored):0;}else free(pending);pending=NULL;}
/* Existing owner workspace saved in Polish (most screenshots and labels are PL).
 * A fresh install defaults to English: tests/test_config.c and test_style_ui.c. */
void test_workspace_enable(void)
{
 stored=dsr_config_default();char *language=stored?strstr(stored,"\"language\":\"en\""):NULL;
 if(language)memcpy(language+12,"pl",2);
 stored_revision=stored?1:0;
}
void test_workspace_enable_fresh(void){stored=dsr_config_default();stored_revision=stored?1:0;}
void test_workspace_disable(void){free(stored);free(pending);stored=pending=NULL;stored_revision=0;defer_save=false;}
static unsigned raw_fault;
void test_workspace_raw_fault(unsigned fault){raw_fault=fault;}
char *dsr_workspace_snapshot(void){snapshot_calls++;if(raw_fault==1||!stored)return NULL;const char *text=raw_fault==2?"{broken":stored;char *copy=malloc(strlen(text)+1);if(copy)strcpy(copy,text);return copy;}
dsr_snapshot_result_t dsr_workspace_snapshot_checked(char **out)
{
 *out=NULL;if(snapshot_fault==1)return DSR_SNAPSHOT_UNAVAILABLE;if(snapshot_fault==2)return DSR_SNAPSHOT_NO_MEMORY;
 if(snapshot_fault==3||snapshot_fault==4){const char *bad=snapshot_fault==3?"{broken":"{}";*out=malloc(strlen(bad)+1);if(*out)strcpy(*out,bad);return *out?DSR_SNAPSHOT_OK:DSR_SNAPSHOT_NO_MEMORY;}
 *out=dsr_workspace_snapshot();return *out?DSR_SNAPSHOT_OK:stored?DSR_SNAPSHOT_NO_MEMORY:DSR_SNAPSHOT_UNAVAILABLE;
}
/* The real workspace publishes its revision under a mutex without parsing or
 * allocating. Readiness loss returns zero; allocation failures do not. */
uint32_t dsr_workspace_revision(void){return snapshot_fault==1||raw_fault==1?0:stored_revision+(snapshot_fault==5?1u:0u);}
bool dsr_workspace_submit(const char *json,uint32_t revision)
{if(pending)return false;char error[96];char *next=stored?dsr_config_replace(stored,json,revision,error):NULL;if(!next)return false;if(defer_save)pending=next;else{free(stored);stored=next;stored_revision=revision+1;}return true;}
void dsr_workspace_status(bool *busy,char message[96]){*busy=pending!=NULL;message[0]=0;}
static bool pair_accepted=true;
static bool pairing_available=true,pairing_paired,pairing_uncertain;
static unsigned pair_code_requests;
void test_pairing_state(bool available,bool paired,bool uncertain){pairing_available=available;pairing_paired=paired;pairing_uncertain=uncertain;}
unsigned test_pair_code_requests(void){return pair_code_requests;}
bool dsr_panel_pairing_snapshot(dsr_panel_pairing_snapshot_t *out)
{if(!out)return false;memset(out,0,sizeof(*out));if(!https_ready||!pairing_available)return false;*out=(dsr_panel_pairing_snapshot_t){pairing_paired,pairing_uncertain};return true;}
void test_pair_accept(bool accepted){pair_accepted=accepted;}
void dsr_panel_pair_begin(char code[17]){pair_code_requests++;if(https_ready&&pair_accepted)strcpy(code,"0123456789abcdef");else code[0]=0;}
static bool ota_accepted=true;
void test_ota_accept(bool accepted){ota_accepted=accepted;}
static unsigned ota_grants;
unsigned test_ota_grants(void){return ota_grants;}
bool dsr_panel_allow_ota(bool stable_usb){ota_grants++;return stable_usb&&https_ready&&ota_accepted;}
/* Licensed review photos, available only in the explicit native fixture. */
static bool photos_enabled;
static uint16_t photos[2][DSR_PHOTO_BYTES/2];
void test_workspace_photos(bool enabled)
{
 photos_enabled=enabled;if(!enabled)return;
 const char *names[]={"salon","kuchnia"};
 for(unsigned i=0;i<2;i++){char path[512];snprintf(path,sizeof(path),"%s/%s.rgb",DSR_TEST_PHOTO_DIR,names[i]);FILE *f=fopen(path,"rb");if(!f)abort();if(fread(photos[i],1,DSR_PHOTO_BYTES,f)!=DSR_PHOTO_BYTES)abort();fclose(f);}
}
const void *dsr_photo_pixels(const char *id){return photos_enabled&&id&&strlen(id)==32&&id[0]>='1'&&id[0]<='2'?photos[id[0]-'1']:NULL;}
const char *dsr_photo_id(unsigned index){static const char *ids[]={"11111111111111111111111111111111","22222222222222222222222222222222"};return photos_enabled&&index<2?ids[index]:NULL;}
const char *dsr_tls_fingerprint(void){return "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";}
bool dsr_workspace_take_bind_with_room(dsr_connection_t *c,uint32_t *room){(void)c;(void)room;return false;}
void dsr_workspace_bind_result(dsr_result_t r){(void)r;}
void dsr_workspace_bind_result_stage(dsr_result_t r,dsr_bind_stage_t stage){(void)r;(void)stage;}

/* Explicit radio fixture. Production dsr_lan.c is not replaced in firmware. */
static dsr_lan_snapshot_t lan;
static unsigned lan_cancels,lan_scans;
unsigned test_lan_scans(void){return lan_scans;}
unsigned test_lan_cancels(void){return lan_cancels;}
void dsr_lan_snapshot(dsr_lan_snapshot_t *out){*out=lan;}
bool dsr_lan_scan(void)
{lan_scans++;lan.count=0;lan.phase=dsr_wifi_connected()?DSR_LAN_SCANNING:DSR_LAN_NO_WIFI;lan.serial++;return dsr_wifi_connected();}
void dsr_lan_cancel(void){lan_cancels++;lan.phase=DSR_LAN_CANCELLED;lan.serial++;}
bool dsr_lan_inspect(const char *host,const char *id,const dsr_credentials_t *credentials)
{(void)credentials;for(unsigned i=0;i<lan.count;i++)if(!strcmp(lan.devices[i].host,host)&&!strcmp(lan.devices[i].id,id)){lan.selected=lan.devices[i];lan.phase=DSR_LAN_INSPECTING;lan.serial++;return true;}return false;}
void test_lan_found(const char *identity,const char *host)
{cJSON *r=cJSON_Parse(identity);dsr_lan_device_t d;if(dsr_lan_parse_identity(r,host,&d)==DSR_OK)dsr_lan_upsert(&lan,&d);cJSON_Delete(r);lan.phase=DSR_LAN_DONE;lan.serial++;}
static dsr_blu_labels_t lan_labels;
void dsr_lan_blu_labels(dsr_blu_labels_t *out){*out=lan.phase==DSR_LAN_INSPECTED?lan_labels:(dsr_blu_labels_t){0};}
/* Gateway page through the real GetComponents parser; call after test_lan_inspected. */
void test_lan_components(const char *page)
{cJSON *r=cJSON_Parse(page);unsigned next,total;if(dsr_lan_parse_components(r,0,&lan.capabilities.blu,&lan_labels,&next,&total)!=DSR_OK)lan_labels=(dsr_blu_labels_t){0};cJSON_Delete(r);lan.serial++;}
void test_lan_inspected(dsr_result_t result,const char *components)
{memset(&lan_labels,0,sizeof(lan_labels));cJSON *r=cJSON_Parse(components);if(result==DSR_OK)result=dsr_lan_parse_capabilities(r,lan.selected.generation,&lan.capabilities);cJSON_Delete(r);lan.result=result;lan.phase=result==DSR_OK?DSR_LAN_INSPECTED:DSR_LAN_ERROR;lan.serial++;}

/* Explicit host BLE provider; the production NimBLE adapter is never replaced
 * in the board build. Advertisement bytes still pass the real C parser. */
static dsr_ble_snapshot_t ble;
static unsigned ble_scans;
unsigned test_ble_scans(void){return ble_scans;}
void dsr_ble_snapshot(dsr_ble_snapshot_t *out){*out=ble;out->snapshot_ms=lv_tick_get();}
bool dsr_ble_scan(void){ble_scans++;if(ble.busy)return false;dsr_ble_begin(&ble,lv_tick_get());ble.phase=DSR_BLE_SCANNING;return true;}
void dsr_ble_cancel(void){if(ble.busy)dsr_ble_finish(&ble,ble.generation,DSR_BLE_CANCELLED,lv_tick_get());}
void test_ble_finish(dsr_ble_phase_t phase){dsr_ble_finish(&ble,ble.generation,phase,lv_tick_get());}
void test_ble_packet(const uint8_t *bytes,size_t length,unsigned address)
{uint8_t addr[6]={(uint8_t)address,2,3,4,5,6};dsr_ble_ingest(&ble,ble.generation,addr,0,-48,bytes,length,lv_tick_get());}

static bool runtime_capture;static size_t runtime_size;static dsr_runtime_device_t runtime_values[DSR_MAX_DEVICES];
void test_runtime_capture(bool enabled){runtime_capture=enabled;runtime_size=0;}
size_t test_runtime_count(void){return runtime_size;}
const dsr_runtime_device_t *test_runtime_at(size_t index){return index<runtime_size?runtime_values+index:NULL;}
void dsr_workspace_runtime_publish_at(size_t index,size_t count,const dsr_runtime_device_t *device){if(runtime_capture){if(index>=count||count!=runtime_size)abort();runtime_values[index]=*device;}}
static bool cover_waiting;static unsigned cover_requests;static uint32_t cover_serial=1000;static dsr_output_request_t cover_request;
unsigned test_cover_requests(void){return cover_requests;}
dsr_result_t dsr_workspace_cover_request(const dsr_device_t *device,dsr_cover_intent_t intent,uint32_t *id)
{if(cover_waiting||!dsr_activity_begin())return DSR_BUSY;cover_request=(dsr_output_request_t){.device=*device,.cover=intent,.operation_id=++cover_serial};*id=cover_serial;cover_requests++;cover_waiting=true;return DSR_OK;}
static unsigned light_requests;
unsigned test_light_requests(void){return light_requests;}
dsr_result_t dsr_workspace_light_request(const dsr_device_t *device,dsr_light_intent_t intent,uint32_t *id)
{if(cover_waiting||!dsr_activity_begin())return DSR_BUSY;cover_request=(dsr_output_request_t){.device=*device,.light=intent,.operation_id=++cover_serial};*id=cover_serial;light_requests++;cover_waiting=true;return DSR_OK;}
static unsigned rgb_requests;
unsigned test_rgb_requests(void){return rgb_requests;}
dsr_result_t dsr_workspace_rgb_request(const dsr_device_t *device,dsr_rgb_intent_t intent,uint32_t *id)
{if(cover_waiting||!dsr_activity_begin())return DSR_BUSY;cover_request=(dsr_output_request_t){.device=*device,.rgb=intent,.operation_id=++cover_serial};*id=cover_serial;rgb_requests++;cover_waiting=true;return DSR_OK;}
static unsigned ht_requests;
unsigned test_ht_requests(void){return ht_requests;}
dsr_result_t dsr_workspace_ht_refresh(const dsr_device_t *device,uint32_t *id)
{if(cover_waiting||!dsr_activity_begin())return DSR_BUSY;cover_request=(dsr_output_request_t){.device=*device,.operation_id=++cover_serial};*id=cover_serial;ht_requests++;cover_waiting=true;return DSR_OK;}
bool dsr_workspace_take_output(dsr_output_request_t *request){if(!cover_waiting)return false;*request=cover_request;cover_waiting=false;memset(&cover_request,0,sizeof(cover_request));dsr_activity_end();return true;}

bool dsr_workspace_runtime_publish_begin(size_t count){if(!runtime_capture)return false;if(count>DSR_MAX_DEVICES)abort();runtime_size=count;return true;}
void dsr_workspace_runtime_publish_end(void){}

static bool ap_sensitive;
static bool ap_temporary;
static unsigned ap_cancels;
void test_ap_temporary(bool value){ap_temporary=value;}
unsigned test_ap_cancels(void){return ap_cancels;}
void dsr_ap_visibility(bool visible){ap_sensitive=visible;}
bool dsr_ap_sensitive(void){return ap_sensitive;}
void dsr_ap_cancel(void){ap_cancels++;}
bool dsr_wifi_temporary_active(void){return ap_temporary;}
/* Handoff provider MODEL; actual workspace TTL/admission tested separately. */
static bool ap_open_requested,ap_open_available;
static uint32_t timer_boot_offset;
void test_timer_boot_offset(uint32_t offset){timer_boot_offset=offset;}
int64_t esp_timer_get_time(void){return ((int64_t)lv_tick_get()+timer_boot_offset)*1000;}
void test_ap_handoff_request(void){ap_open_requested=true;}
bool test_ap_handoff_available(void){return ap_open_available;}
void dsr_workspace_ap_publish(bool available,bool active,uint32_t now_ms)
{(void)now_ms;ap_open_available=available&&!active;if(active||!available)ap_open_requested=false;}
bool dsr_workspace_take_ap_open(uint32_t now_ms)
{(void)now_ms;bool take=ap_open_requested&&ap_open_available;if(take)ap_open_requested=false;return take;}

/* RPC transport provider MODEL; actual client/callback faults tested separately. */
static dsr_ble_rpc_snapshot_t rpc;
static bool rpc_visible;
static unsigned rpc_starts,rpc_confirms,rpc_cancels;
unsigned test_rpc_starts(void){return rpc_starts;}
unsigned test_rpc_confirms(void){return rpc_confirms;}
unsigned test_rpc_cancels(void){return rpc_cancels;}
bool dsr_ble_rpc_active(void){return rpc.active;}
bool dsr_ble_rpc_sensitive(void){return rpc.active||rpc_visible;}
void dsr_ble_rpc_visibility(bool visible){rpc_visible=visible;}
void dsr_ble_rpc_snapshot(dsr_ble_rpc_snapshot_t *out){*out=rpc;}
dsr_result_t dsr_ble_rpc_start(const dsr_ble_device_t *candidate)
{if(rpc.active||candidate->kind!=DSR_BLE_SHELLY_RPC)return DSR_BUSY;rpc_starts++;rpc=(dsr_ble_rpc_snapshot_t){.active=true,.generation=rpc.generation+1,.serial=rpc.serial+1,.phase=DSR_BLE_RPC_CONNECTING,.result=DSR_BUSY};return DSR_OK;}
dsr_result_t dsr_ble_rpc_confirm(uint32_t generation,const char *ssid)
{if(generation!=rpc.generation||rpc.phase!=DSR_BLE_RPC_CONFIRM||!ssid[0])return DSR_BUSY;rpc_confirms++;rpc.phase=DSR_BLE_RPC_WRITING;rpc.serial++;return DSR_OK;}
void dsr_ble_rpc_cancel(uint32_t generation){if(generation==rpc.generation&&rpc.active)rpc_cancels++;}
void test_rpc_update(dsr_ble_rpc_phase_t phase,dsr_result_t result,bool active,const dsr_lan_device_t *identity)
{rpc.phase=phase;rpc.result=result;rpc.active=active;rpc.serial++;if(identity){rpc.device=*identity;rpc.has_identity=true;}if(phase==DSR_BLE_RPC_LAN_READY)rpc.write_possible=true;}

void dsr_ble_handoff_publish(bool available,bool active,uint32_t now){(void)available;(void)active;(void)now;}
bool dsr_ble_handoff_take(dsr_ble_device_t *candidate,uint32_t now){(void)candidate;(void)now;return false;}
/* Actual actions actor/API linked in UI tests; SDK allocation/RTOS are models. */
#include "esp_heap_caps.h"
#include "freertos/semphr.h"
void *heap_caps_calloc(size_t count,size_t size,unsigned caps){if(caps!=(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT))abort();return calloc(count,size);}
static unsigned action_lock;
SemaphoreHandle_t xSemaphoreCreateMutex(void){return &action_lock;}
int xSemaphoreTake(SemaphoreHandle_t handle,TickType_t wait){(void)wait;if(handle!=&action_lock||action_lock)abort();action_lock=1;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t handle){if(handle!=&action_lock||!action_lock)abort();action_lock=0;return pdTRUE;}
uint32_t esp_random(void){static uint32_t serial=41000;return ++serial;}

/* Diagnostics counters MODEL (real SDK reads only on target). */
static dsr_system_runtime_t runtime_model={.internal_min_free=61440,.ui_stack_free=2304,.ble_active=false};
void test_system_runtime(dsr_system_runtime_t value){runtime_model=value;}
void dsr_system_runtime(dsr_system_runtime_t *out){*out=runtime_model;}

void dsr_workspace_runtime_info_at(size_t index,const dsr_device_info_t *info){(void)index;(void)info;}
bool dsr_workspace_runtime_info(uint32_t id,const char *identity,dsr_device_info_t *info){(void)id;(void)identity;memset(info,0,sizeof(*info));return false;}

bool dsr_workspace_runtime_name(uint32_t id,const char *host,uint8_t channel,char out[48])
{for(size_t i=0;i<runtime_size;i++)if(runtime_values[i].layout_id==id&&runtime_values[i].device.channel==channel&&!strcmp(runtime_values[i].device.host,host)){snprintf(out,48,"%s",runtime_values[i].device.name);return out[0]!=0;}return false;}
