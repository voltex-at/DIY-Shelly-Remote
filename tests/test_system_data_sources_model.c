#include "dsr_backup_sources_model.h"
#include "dsr_config.h"
#include "nvs.h"
#include "mbedtls/sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
static unsigned io_attempts;
/* COFF retains uncalled connection functions. Fail-closed providers ensure
 * these pure validators can never perform an accidental persistent write. */
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h){(void)name;(void)mode;(void)h;io_attempts++;return ESP_FAIL;}
esp_err_t nvs_get_u32(nvs_handle_t h,const char *key,uint32_t *out){(void)h;(void)key;(void)out;io_attempts++;return ESP_FAIL;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size){(void)h;(void)key;(void)out;(void)size;io_attempts++;return ESP_FAIL;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size){(void)h;(void)key;(void)data;(void)size;io_attempts++;return ESP_FAIL;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;io_attempts++;return ESP_FAIL;}
void nvs_close(nvs_handle_t h){(void)h;io_attempts++;}
bool dsr_activity_begin(void){io_attempts++;return false;}
void dsr_activity_end(void){io_attempts++;}
int64_t esp_timer_get_time(void){io_attempts++;return 0;}
static const uint8_t device[32]={1};
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"sources:%d %s\n",__LINE__,#x);exit(1);}}while(0)
static cJSON *root(const char *path)
{char data[2048];FILE *f=fopen(path,"rb");CHECK(f);size_t size=fread(data,1,sizeof(data)-1,f);CHECK(feof(f)&&!fclose(f));data[size]=0;cJSON *r=cJSON_Parse(data);CHECK(r);return r;}
static void set(cJSON *o,const char *key,cJSON *value)
{CHECK(value);if(cJSON_GetObjectItemCaseSensitive(o,key))CHECK(cJSON_ReplaceItemInObjectCaseSensitive(o,key,value));else CHECK(cJSON_AddItemToObject(o,key,value));}
static void binding(cJSON *r,unsigned id,const char *kind,unsigned endpoint)
{
 cJSON *cfg=cJSON_GetObjectItemCaseSensitive(r,"config"),*d=cJSON_CreateObject();CHECK(d);
 CHECK(cJSON_AddNumberToObject(d,"id",id)&&cJSON_AddStringToObject(d,"name","Test")&&cJSON_AddStringToObject(d,"host","192.0.2.4")&&cJSON_AddNumberToObject(d,"channel",id)&&cJSON_AddNumberToObject(d,"room",0)&&cJSON_AddStringToObject(d,"kind",kind));
 CHECK(cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(cfg,"devices"),d));
 cJSON *c=cJSON_CreateObject();CHECK(c);
 CHECK(cJSON_AddNumberToObject(c,"layout_id",id)&&cJSON_AddStringToObject(c,"host","192.0.2.4")&&cJSON_AddStringToObject(c,"name","54657374")&&cJSON_AddStringToObject(c,"device_id",id==1?"7368656c6c792d31":"7368656c6c792d32")&&cJSON_AddNumberToObject(c,"generation",2)&&cJSON_AddNumberToObject(c,"channel",id)&&cJSON_AddNumberToObject(c,"endpoint",endpoint)&&cJSON_AddStringToObject(c,"username","61646d696e")&&cJSON_AddStringToObject(c,"password","746573742d6f6e6c79"));
 CHECK(cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"connections"),c));
}
static void heating(cJSON *r)
{
 binding(r,1,"contact",DSR_ENDPOINT_BLU);binding(r,2,"sensor",DSR_ENDPOINT_BLU);
 cJSON *config=cJSON_GetObjectItemCaseSensitive(r,"config");
 set(config,"automations",cJSON_Parse("[{\"id\":1,\"name\":\"Heating\",\"kind\":\"contact_heating\",\"enabled\":true,\"sensors\":[1],\"targets\":[2],\"target_half\":16}]"));
 cJSON *enroll=cJSON_GetObjectItemCaseSensitive(r,"enrollment");
 set(enroll,"rules",cJSON_Parse("[{\"id\":1,\"signature\":3591033476}]"));
 set(enroll,"keys",cJSON_Parse("[{\"layout_id\":1,\"device_id\":\"7368656c6c792d31\",\"host\":\"192.0.2.4\",\"generation\":2,\"channel\":1,\"kind\":1},{\"layout_id\":2,\"device_id\":\"7368656c6c792d32\",\"host\":\"192.0.2.4\",\"generation\":2,\"channel\":2,\"kind\":2}]"));
}
static void all_photos(cJSON *r)
{
 uint8_t *pixels=calloc(1,115200);CHECK(pixels);cJSON *cfg=cJSON_GetObjectItemCaseSensitive(r,"config");
 for(unsigned i=0;i<16;i++){
  uint8_t hash[32];pixels[0]=(uint8_t)i;CHECK(!mbedtls_sha256(pixels,115200,hash,0));char id[33]={0},sha[65]={0};
  for(unsigned j=0;j<32;j++){sprintf(sha+2*j,"%02x",hash[j]);if(j<16)sprintf(id+2*j,"%02x",hash[j]);}
  cJSON *p=cJSON_CreateObject();CHECK(p);CHECK(cJSON_AddStringToObject(p,"id",id)&&cJSON_AddStringToObject(p,"sha256",sha)&&cJSON_AddNumberToObject(p,"bytes",115200)&&cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"photos"),p));
  cJSON *v=cJSON_CreateObject();CHECK(v);CHECK(cJSON_AddNumberToObject(v,"id",i%8+1)&&cJSON_AddStringToObject(v,"name","Photo")&&cJSON_AddStringToObject(v,"photo",id));
  CHECK(i<8?cJSON_AddNumberToObject(v,"icon",0)!=NULL:cJSON_AddArrayToObject(v,"actions")!=NULL);
  CHECK(cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(cfg,i<8?"rooms":"scenes"),v));
 }
 free(pixels);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);dsr_backup_sources_data_t *data=calloc(1,sizeof(*data));CHECK(data);cJSON *r=root(argv[1]);
 CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);CHECK(data->count==0&&data->settings.brightness==70&&data->battery.full_mv==4200&&data->enrollment.generation==1);
 cJSON *settings=cJSON_GetObjectItemCaseSensitive(r,"settings");set(settings,"ssid",cJSON_CreateString("ff74657374"));set(settings,"password",cJSON_CreateString("746573742d6f6e6c79"));set(settings,"wifi_security",cJSON_CreateNumber(DSR_WIFI_WPA2));
 CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);CHECK((unsigned char)data->settings.ssid[0]==255&&!strcmp(data->settings.ssid+1,"test")&&!strcmp(data->settings.password,"test-only"));
 set(settings,"ssid",cJSON_CreateString("746573740078"));CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);set(settings,"ssid",cJSON_CreateString("74657374"));
 set(settings,"wifi_security",cJSON_CreateNumber(DSR_WIFI_OPEN));CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);set(settings,"wifi_security",cJSON_CreateNumber(DSR_WIFI_WPA2));
 cJSON_AddBoolToObject(settings,"unknown",true);CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);cJSON_DeleteItemFromObjectCaseSensitive(settings,"unknown");
 binding(r,1,"relay",DSR_ENDPOINT_SWITCH);CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);CHECK(data->count==1&&!strcmp(data->connections[0].credentials.password,"test-only"));CHECK(!data->connections[0].device.online&&!data->connections[0].device.has_power&&!data->connections[0].device.power_w);
 cJSON *c=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(r,"connections"),0);set(c,"channel",cJSON_CreateNumber(4));CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);set(c,"channel",cJSON_CreateNumber(1));
 cJSON_AddBoolToObject(c,"online",true);CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);cJSON_DeleteItemFromObjectCaseSensitive(c,"online");
 cJSON *duplicate=cJSON_Duplicate(c,true);CHECK(duplicate);CHECK(cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(r,"connections"),duplicate));CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(r,"connections"),1);
 CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);CHECK(dsr_backup_sources_encode(r,data)==DSR_OK);CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);CHECK(!strcmp(data->connections[0].device.device_id,"shelly-1"));
 cJSON_Delete(r);r=root(argv[1]);cJSON *cfg=cJSON_GetObjectItemCaseSensitive(r,"config");
 cJSON *room=cJSON_Parse("{\"id\":1,\"name\":\"Room\",\"icon\":0,\"photo\":\"4bab7d8cbce4fbc18505a095c15ca47c\"}");CHECK(room);CHECK(cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(cfg,"rooms"),room));
 CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);
 set(room,"photo",cJSON_CreateString("ffffffffffffffffffffffffffffffff"));CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);
 cJSON_Delete(r);r=root(argv[1]);heating(r);CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);
 CHECK(data->enrollment.rules.count==1&&data->enrollment.rules.items[0].signature==3591033476u&&data->enrollment.targets[1].baseline==0);
 dsr_auto_record_t *trusted=malloc(sizeof(*trusted)),*fresh=malloc(sizeof(*fresh));CHECK(trusted&&fresh);*trusted=data->enrollment;
 cfg=cJSON_GetObjectItemCaseSensitive(r,"config");CHECK(dsr_backup_enrollment_prepare(cfg,trusted,data->connections,data->count,fresh)==DSR_OK);
 trusted->cycles[0].active=DSR_AUTO_CYCLE_OPEN;trusted->cycles[0].cycle=5;CHECK(dsr_backup_enrollment_prepare(cfg,trusted,data->connections,data->count,fresh)==DSR_BUSY);trusted->cycles[0].active=0;
 trusted->targets[1].phase=DSR_AUTO_UNCERTAIN;trusted->targets[1].baseline=40;trusted->targets[1].desired=16;CHECK(dsr_backup_enrollment_prepare(cfg,trusted,data->connections,data->count,fresh)==DSR_BUSY);
 /* Disabling a rule abandons ownership without commanding the TRV. Old owned
  * state is harmless history, not an outstanding write that blocks backup. */
 trusted->targets[1].phase=DSR_AUTO_OWNED;set(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(cfg,"automations"),0),"enabled",cJSON_CreateBool(false));
 CHECK(dsr_backup_enrollment_prepare(cfg,trusted,data->connections,data->count,fresh)==DSR_OK);CHECK(!fresh->rules.items[0].enabled&&!fresh->targets[1].baseline);
 set(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(cfg,"automations"),0),"enabled",cJSON_CreateBool(true));trusted->targets[1].phase=0;
 strcpy(data->connections[1].device.device_id,"different-device");CHECK(dsr_backup_enrollment_prepare(cfg,trusted,data->connections,data->count,fresh)==DSR_IDENTITY_MISMATCH);
 cJSON *keys=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(r,"enrollment"),"keys");set(cJSON_GetArrayItem(keys,1),"device_id",cJSON_CreateString("646966666572656e74"));CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);CHECK(data->count==0&&!data->connections[0].credentials.password[0]);
 cJSON_Delete(r);free(trusted);free(fresh);r=root(argv[1]);all_photos(r);CHECK(dsr_backup_sources_decode(r,device,data)==DSR_OK);
 cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(r,"photos"),15);CHECK(dsr_backup_sources_decode(r,device,data)==DSR_INVALID);
 cJSON_Delete(r);free(data);CHECK(io_attempts==0);printf("PASS: %u source metadata checks; real validators, no storage/radio\n",checks);return 0;
}
