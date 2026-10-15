#include "dsr_workspace.h"
#include "dsr_discovery.h"
#include <stdlib.h>
#include <string.h>
/* Disabled for the original Wi-Fi fixture until explicitly enabled. */
static char *stored;
void test_workspace_enable(void){stored=dsr_config_default();}
void test_workspace_disable(void){free(stored);stored=NULL;}
char *dsr_workspace_snapshot(void){if(!stored)return NULL;char *copy=malloc(strlen(stored)+1);if(copy)strcpy(copy,stored);return copy;}
uint32_t dsr_workspace_revision(void){return stored?dsr_config_revision(stored):0;}
bool dsr_workspace_submit(const char *json,uint32_t revision)
{char error[96];char *next=stored?dsr_config_replace(stored,json,revision,error):NULL;if(!next)return false;free(stored);stored=next;return true;}
void dsr_workspace_status(bool *busy,char message[96]){*busy=false;message[0]=0;}
void dsr_panel_pair_begin(char code[17]){strcpy(code,"0123456789abcdef");}
void dsr_panel_pair_cancel(void){}
void dsr_panel_allow_ota(void){}
const void *dsr_photo_pixels(const char *id){(void)id;return NULL;}
const char *dsr_photo_id(unsigned index){(void)index;return NULL;}
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
