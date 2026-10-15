#include "dsr_workspace.h"
#include "dsr_discovery.h"
#include "dsr_ble.h"
#include "lvgl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* Disabled for the original Wi-Fi fixture until explicitly enabled. */
static char *stored;
static char *pending;
static bool defer_save;
void test_workspace_defer(bool enabled){defer_save=enabled;}
void test_workspace_finish(bool success){if(success){free(stored);stored=pending;}else free(pending);pending=NULL;}
void test_workspace_enable(void){stored=dsr_config_default();}
void test_workspace_disable(void){free(stored);free(pending);stored=pending=NULL;defer_save=false;}
char *dsr_workspace_snapshot(void){if(!stored)return NULL;char *copy=malloc(strlen(stored)+1);if(copy)strcpy(copy,stored);return copy;}
uint32_t dsr_workspace_revision(void){return stored?dsr_config_revision(stored):0;}
bool dsr_workspace_submit(const char *json,uint32_t revision)
{if(pending)return false;char error[96];char *next=stored?dsr_config_replace(stored,json,revision,error):NULL;if(!next)return false;if(defer_save)pending=next;else{free(stored);stored=next;}return true;}
void dsr_workspace_status(bool *busy,char message[96]){*busy=pending!=NULL;message[0]=0;}
void dsr_panel_pair_begin(char code[17]){strcpy(code,"0123456789abcdef");}
void dsr_panel_pair_cancel(void){}
void dsr_panel_allow_ota(void){}
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
const char *dsr_tls_fingerprint(void){return "TEST FIXTURE";}
bool dsr_workspace_take_bind(dsr_connection_t *c){(void)c;return false;}
bool dsr_workspace_take_bind_with_room(dsr_connection_t *c,uint32_t *room){(void)c;(void)room;return false;}
void dsr_workspace_bind_result(dsr_result_t r){(void)r;}

/* Explicit radio fixture. Production dsr_lan.c is not replaced in firmware. */
static dsr_lan_snapshot_t lan;
void dsr_lan_snapshot(dsr_lan_snapshot_t *out){*out=lan;}
bool dsr_lan_scan(void)
{lan.count=0;lan.phase=dsr_wifi_connected()?DSR_LAN_SCANNING:DSR_LAN_NO_WIFI;lan.serial++;return dsr_wifi_connected();}
void dsr_lan_cancel(void){lan.phase=DSR_LAN_CANCELLED;lan.serial++;}
bool dsr_lan_inspect(const char *host,const char *id,const dsr_credentials_t *credentials)
{(void)credentials;for(unsigned i=0;i<lan.count;i++)if(!strcmp(lan.devices[i].host,host)&&!strcmp(lan.devices[i].id,id)){lan.selected=lan.devices[i];lan.phase=DSR_LAN_INSPECTING;lan.serial++;return true;}return false;}
void test_lan_found(const char *identity,const char *host)
{cJSON *r=cJSON_Parse(identity);dsr_lan_device_t d;if(dsr_lan_parse_identity(r,host,&d)==DSR_OK)dsr_lan_upsert(&lan,&d);cJSON_Delete(r);lan.phase=DSR_LAN_DONE;lan.serial++;}
void test_lan_inspected(dsr_result_t result,const char *components)
{cJSON *r=cJSON_Parse(components);if(result==DSR_OK)result=dsr_lan_parse_capabilities(r,lan.selected.generation,&lan.capabilities);cJSON_Delete(r);lan.result=result;lan.phase=result==DSR_OK?DSR_LAN_INSPECTED:DSR_LAN_ERROR;lan.serial++;}

/* Explicit host BLE provider; the production NimBLE adapter is never replaced
 * in the board build. Advertisement bytes still pass the real C parser. */
static dsr_ble_snapshot_t ble;
void dsr_ble_snapshot(dsr_ble_snapshot_t *out){*out=ble;out->snapshot_ms=lv_tick_get();}
bool dsr_ble_scan(void){if(ble.busy)return false;dsr_ble_begin(&ble,lv_tick_get());ble.phase=DSR_BLE_SCANNING;return true;}
void dsr_ble_cancel(void){if(ble.busy)dsr_ble_finish(&ble,ble.generation,DSR_BLE_CANCELLED,lv_tick_get());}
void test_ble_finish(dsr_ble_phase_t phase){dsr_ble_finish(&ble,ble.generation,phase,lv_tick_get());}
void test_ble_packet(const uint8_t *bytes,size_t length,unsigned address)
{uint8_t addr[6]={(uint8_t)address,2,3,4,5,6};dsr_ble_ingest(&ble,ble.generation,addr,0,-48,bytes,length,lv_tick_get());}
