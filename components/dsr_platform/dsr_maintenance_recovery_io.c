#include "dsr_maintenance_recovery_io.h"
#include "dsr_maintenance_private.h"
#include "dsr_config_restore.h"
#include "dsr_photo_name.h"
#include "esp_spiffs.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static const char *const reset_names[]={"dsr_cfg","dsr_bindings","dsr_device","dsr_battery","dsr_ui","dsr_auto","dsr_panel","dsr_tls","nimble_bond"};
dsr_result_t dsr_maint_recovery_mount(void)
{
 if(esp_spiffs_mounted("dsrdata"))return DSR_OK;
 const esp_vfs_spiffs_conf_t conf={.base_path="/dsr",.partition_label="dsrdata",.max_files=8,.format_if_mount_failed=false};
 return esp_vfs_spiffs_register(&conf)==ESP_OK?DSR_OK:DSR_IO;
}
static bool photo_path(const char *id,char out[64])
{char name[38];if(!dsr_photo_filename(id,name))return false;int n=snprintf(out,64,"/dsr/%s",name);return n>0&&n<64;}
dsr_result_t dsr_maint_recovery_photo_read(const char id[33],uint8_t out[115200])
{
 if(!out)return DSR_INVALID;
 char path[64];if(!photo_path(id,path))return DSR_INVALID;FILE *f=fopen(path,"rb");if(!f)return DSR_IO;
 bool ok=true;for(size_t at=0;ok&&at<115200;){size_t n=115200-at;if(n>4096)n=4096;ok=fread(out+at,1,n,f)==n;at+=n;}
 if(ok)ok=fgetc(f)==EOF;
 if(fclose(f))ok=false;
 return ok?DSR_OK:DSR_IO;
}
dsr_result_t dsr_maint_recovery_photo_write(const char id[33],const uint8_t bytes[115200])
{
 if(!dsr_maintenance_boot_writes_allowed())return DSR_BUSY;
 if(!bytes)return DSR_INVALID;
 char path[64];if(!photo_path(id,path))return DSR_INVALID;FILE *f=fopen(path,"wb");if(!f)return DSR_IO;
 bool ok=true;for(size_t at=0;ok&&at<115200;){size_t n=115200-at;if(n>4096)n=4096;ok=fwrite(bytes+at,1,n,f)==n;at+=n;}
 if(ok)ok=fflush(f)==0&&fsync(fileno(f))==0;
 if(fclose(f))ok=false;
 return ok?DSR_OK:DSR_IO;
}
dsr_result_t dsr_maint_recovery_checkpoint(const cJSON *config,uint32_t revision,unsigned slot)
{
 if(!dsr_maintenance_boot_writes_allowed())return DSR_BUSY;
 if(!config||!revision||revision>=UINT32_MAX||slot>1)return DSR_INVALID;
 cJSON *copy=cJSON_Duplicate(config,true);if(!copy)return DSR_FULL;
 cJSON *rev=cJSON_GetObjectItemCaseSensitive(copy,"revision");if(!cJSON_IsNumber(rev)){cJSON_Delete(copy);return DSR_INVALID;}cJSON_SetNumberValue(rev,revision);
 char *json=cJSON_PrintUnformatted(copy);cJSON_Delete(copy);if(!json)return DSR_FULL;
 bool ok=dsr_config_store_restore_slot("/dsr",slot,json);free(json);return ok?DSR_OK:DSR_IO;
}
static dsr_result_t clear_namespace(const char *name)
{
 nvs_handle_t h;esp_err_t e=nvs_open(name,NVS_READWRITE,&h);if(e!=ESP_OK)return DSR_IO;
 e=nvs_erase_all(h);if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);if(e!=ESP_OK)return DSR_IO;
 nvs_iterator_t it=NULL;e=nvs_entry_find("nvs",name,NVS_TYPE_ANY,&it);if(it)nvs_release_iterator(it);return e==ESP_ERR_NVS_NOT_FOUND?DSR_OK:DSR_IO;
}
dsr_result_t dsr_maint_recovery_unpair(void)
{
 if(!dsr_maintenance_boot_writes_allowed())return DSR_BUSY;
 /* Legacy single-device fallback must not reappear on an otherwise empty
  * restored registry. Keep this remote's current TLS identity on restore. */
 dsr_result_t r=clear_namespace("dsr_device");return r==DSR_OK?clear_namespace("dsr_panel"):r;
}
static dsr_result_t format_data(void)
{
 /* Explicit committed full reset only. Register without format first. Pinned
  * SDK may need its unmounted formatter if the old FS is damaged; neither
  * path touches another partition. Share idle-priority CPU as first-format. */
 (void)dsr_maint_recovery_mount();UBaseType_t priority=uxTaskPriorityGet(NULL);vTaskPrioritySet(NULL,tskIDLE_PRIORITY);vTaskDelay(1);
 esp_err_t error=esp_spiffs_format("dsrdata");vTaskPrioritySet(NULL,priority);if(error!=ESP_OK)return DSR_IO;
 dsr_result_t r=dsr_maint_recovery_mount();if(r!=DSR_OK)return r;
 DIR *d=opendir("/dsr");if(!d)return DSR_IO;struct dirent *entry;bool empty=true;
 while((entry=readdir(d)))if(strcmp(entry->d_name,".")&&strcmp(entry->d_name,"..")){empty=false;break;}
 closedir(d);return empty?DSR_OK:DSR_IO;
}
dsr_result_t dsr_maint_recovery_reset_part(unsigned part)
{
 if(!dsr_maintenance_boot_writes_allowed())return DSR_BUSY;
 if(part<sizeof(reset_names)/sizeof(*reset_names))return clear_namespace(reset_names[part]);
 return part==9?format_data():DSR_INVALID;
}
