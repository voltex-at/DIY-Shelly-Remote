#include "dsr_backup_sources.h"
#include "dsr_backup_sources_model.h"
#include "dsr_backup_bonds_model.h"
#include "dsr_backup_storage.h"
#include "dsr_maintenance_private.h"
#include "dsr_workspace.h"
#include "mbedtls/sha256.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_app_desc.h"
#include "esp_mac.h"
static void *allocate(size_t n){return heap_caps_calloc(1,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
dsr_result_t dsr_backup_device_id(uint8_t out[32])
{
 if(!out)return DSR_INVALID;
 uint8_t domain[sizeof(DSR_BACKUP_HARDWARE)+6];memcpy(domain,DSR_BACKUP_HARDWARE,sizeof(DSR_BACKUP_HARDWARE));
 if(esp_efuse_mac_get_default(domain+sizeof(DSR_BACKUP_HARDWARE))!=ESP_OK)return DSR_IO;
 int e=mbedtls_sha256(domain,sizeof(domain),out,0);dsr_backup_wipe(domain,sizeof(domain));return e?DSR_IO:DSR_OK;
}
static const char *version(void){return esp_app_get_description()->version;}
#else
static void *allocate(size_t n){return calloc(1,n);}
static const char *version(void){return "host-model";}
#endif
typedef struct {
 dsr_backup_sources_data_t data;dsr_auto_record_t trusted;
 dsr_backup_photo_t photos[DSR_BACKUP_MAX_PHOTOS];unsigned photo_count;
} capture_t;
static void hex(const uint8_t *bytes,size_t n,char *out)
{static const char digits[]="0123456789abcdef";for(size_t i=0;i<n;i++){out[2*i]=digits[bytes[i]>>4];out[2*i+1]=digits[bytes[i]&15];}out[2*n]=0;}
static dsr_result_t collect(const cJSON *node,capture_t *capture,cJSON *photos)
{
 for(const cJSON *v=node;v;v=v->next){
  if(v->string&&!strcmp(v->string,"photo")&&cJSON_IsString(v)&&v->valuestring[0]&&!dsr_backup_photo_builtin(v->valuestring)){
   unsigned i=0;for(;i<capture->photo_count;i++)if(!strcmp(capture->photos[i].id,v->valuestring))break;
   if(i==capture->photo_count){
    if(i==DSR_BACKUP_MAX_PHOTOS||strlen(v->valuestring)!=32)return DSR_INVALID;
    dsr_backup_photo_t *p=&capture->photos[i];p->pixels=dsr_photo_pixels(v->valuestring);if(!p->pixels)return DSR_IO;
    if(mbedtls_sha256(p->pixels,DSR_BACKUP_PHOTO_BYTES,p->sha256,0))return DSR_IO;
    hex(p->sha256,16,p->id);if(strcmp(p->id,v->valuestring))return DSR_INVALID;
    char digest[65];hex(p->sha256,32,digest);cJSON *row=cJSON_CreateObject();
    if(!row||!cJSON_AddStringToObject(row,"id",p->id)||!cJSON_AddStringToObject(row,"sha256",digest)||!cJSON_AddNumberToObject(row,"bytes",DSR_BACKUP_PHOTO_BYTES)||!cJSON_AddItemToArray(photos,row)){cJSON_Delete(row);return DSR_FULL;}
    capture->photo_count++;
   }
  }
  if(v->child){dsr_result_t r=collect(v->child,capture,photos);if(r!=DSR_OK)return r;}
 }return DSR_OK;
}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static dsr_result_t frame(cJSON *root,const capture_t *capture,const uint8_t device[32],dsr_backup_image_t **out)
{
 char *metadata=cJSON_PrintUnformatted(root);if(!metadata)return DSR_FULL;
 size_t n=strlen(metadata),size=24+n+capture->photo_count*(56+DSR_BACKUP_PHOTO_BYTES);uint8_t *raw=NULL;
 dsr_result_t r=DSR_FULL;if(n<=DSR_BACKUP_METADATA_BYTES&&size<=DSR_BACKUP_MAX_BYTES)raw=allocate(size);
 if(raw){
  memcpy(raw,"DSRSNP1",8);put16(raw+8,1);put16(raw+10,24);put32(raw+12,(uint32_t)n);put16(raw+16,(uint16_t)capture->photo_count);put32(raw+20,(uint32_t)size);memcpy(raw+24,metadata,n);
  size_t at=24+n;for(unsigned i=0;i<capture->photo_count;i++){
   const dsr_backup_photo_t *p=&capture->photos[i];memcpy(raw+at,p->sha256,16);memcpy(raw+at+16,p->sha256,32);put32(raw+at+48,DSR_BACKUP_PHOTO_BYTES);
   memcpy(raw+at+56,p->pixels,DSR_BACKUP_PHOTO_BYTES);at+=56+DSR_BACKUP_PHOTO_BYTES;
  }
  r=dsr_backup_image_open(raw,size,device,out);
 }
 dsr_backup_wipe(metadata,n);free(metadata);return r;
}
bool dsr_backup_sources_quiet(void){return dsr_automation_backup_record(NULL)==DSR_OK;}
dsr_result_t dsr_backup_sources_capture(const dsr_maint_lease_t *lease,dsr_backup_image_t **out)
{
 if(!out)return DSR_INVALID;
 *out=NULL;if(!dsr_maintenance_lease_valid(lease))return DSR_BUSY;
 capture_t *capture=allocate(sizeof(*capture));if(!capture)return DSR_FULL;
 cJSON *root=cJSON_CreateObject(),*config=NULL;char *json=NULL;uint8_t device[32];dsr_result_t r=dsr_backup_device_id(device);
 if(!root){r=DSR_FULL;goto done;}
 if(r!=DSR_OK)goto done;
 if(dsr_workspace_snapshot_checked(&json)!=DSR_SNAPSHOT_OK){r=DSR_BUSY;goto done;}
 config=cJSON_Parse(json);free(json);json=NULL;char error[96];if(!dsr_config_validate(config,error)){r=DSR_INVALID;goto done;}
 if(!cJSON_AddItemToObject(root,"config",config)){r=DSR_FULL;goto done;}config=NULL;
 char id[65];hex(device,32,id);time_t now=time(NULL);
 if(!cJSON_AddNumberToObject(root,"format",1)||!cJSON_AddStringToObject(root,"hardware",DSR_BACKUP_HARDWARE)||!cJSON_AddStringToObject(root,"device",id)||!cJSON_AddStringToObject(root,"firmware",version())||
   !(now>=1735689600&&(uint64_t)now<=UINT32_MAX?cJSON_AddNumberToObject(root,"created",(double)now):cJSON_AddNullToObject(root,"created"))){r=DSR_FULL;goto done;}
 cJSON *photos=cJSON_AddArrayToObject(root,"photos");if(!photos){r=DSR_FULL;goto done;}
 const cJSON *cfg=cJSON_GetObjectItemCaseSensitive(root,"config");r=collect(cfg->child,capture,photos);if(r!=DSR_OK)goto done;
 if(dsr_settings_load(&capture->data.settings)!=ESP_OK||dsr_connections_load(capture->data.connections,DSR_MAX_DEVICES,&capture->data.count)!=ESP_OK||dsr_battery_profile_load(&capture->data.battery)!=ESP_OK){r=DSR_IO;goto done;}
 const char *theme=cJSON_GetObjectItemCaseSensitive(cfg,"theme")->valuestring;capture->data.style=!strcmp(theme,"cyber_blue")?3:!strcmp(theme,"light")?2:0;
 r=dsr_automation_backup_record(&capture->trusted);if(r!=DSR_OK)goto done;
 r=dsr_backup_enrollment_prepare(cfg,&capture->trusted,capture->data.connections,capture->data.count,&capture->data.enrollment);if(r!=DSR_OK)goto done;
 r=dsr_backup_sources_encode(root,&capture->data);if(r!=DSR_OK)goto done;
 r=dsr_maintenance_bonds_capture(lease,root);if(r!=DSR_OK)goto done;
 /* Validate the composed result through the same path as an import. */
 dsr_backup_wipe(&capture->data,sizeof(capture->data));r=dsr_backup_sources_decode(root,device,&capture->data);
 if(r==DSR_OK)r=dsr_backup_bonds_validate(cJSON_GetObjectItemCaseSensitive(root,"bonds"));
 if(r==DSR_OK&&!dsr_maintenance_lease_valid(lease))r=DSR_BUSY;
 if(r==DSR_OK)r=frame(root,capture,device,out);
 if(r==DSR_OK&&!dsr_maintenance_lease_valid(lease)){dsr_backup_image_free(*out);*out=NULL;r=DSR_BUSY;}
done:
 free(json);cJSON_Delete(config);dsr_backup_json_discard(root);dsr_backup_wipe(capture,sizeof(*capture));free(capture);return r;
}
dsr_result_t dsr_backup_sources_apply(const cJSON *root)
{
 if(!dsr_maintenance_boot_writes_allowed())return DSR_BUSY;
 uint8_t device[32];dsr_result_t r=dsr_backup_device_id(device);if(r!=DSR_OK)return r;
 dsr_backup_sources_data_t *data=allocate(sizeof(*data));if(!data)return DSR_FULL;
 r=dsr_backup_sources_decode(root,device,data);
 if(r==DSR_OK)r=dsr_backup_bonds_validate(cJSON_GetObjectItemCaseSensitive(root,"bonds"));
 if(r==DSR_OK&&(dsr_settings_restore(&data->settings)!=ESP_OK||dsr_connections_restore(data->connections,data->count)!=ESP_OK||dsr_battery_profile_restore(&data->battery)!=ESP_OK||dsr_style_hint_restore(data->style)!=ESP_OK||dsr_auto_store_restore(&data->enrollment)!=ESP_OK))r=DSR_IO;
 dsr_backup_wipe(data,sizeof(*data));free(data);return r;
}
