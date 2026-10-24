#include "nvs.h"
#include "storage.h"
#include <stdlib.h>
#include <string.h>
unsigned test_nvs_calls,test_nvs_writes;bool test_boot_allowed;
unsigned test_active_writes;
int test_nvs_fail_after=-1;
static bool cut(void){if(test_nvs_fail_after<0)return false;if(test_nvs_fail_after--==0){test_nvs_fail_after=-1;return true;}return false;}
static char namespaces[16][16];static unsigned namespace_count;
static struct {int ns;char key[16];uint8_t *bytes;size_t size;} entries[200];
static void fail(void){abort();}
static int find(nvs_handle_t h,const char *key){for(unsigned i=0;i<200;i++)if(entries[i].ns==h&&!strcmp(entries[i].key,key))return (int)i;return -1;}
void test_nvs_reset(void){for(unsigned i=0;i<200;i++){if(entries[i].bytes){memset(entries[i].bytes,0,entries[i].size);free(entries[i].bytes);}}memset(entries,0,sizeof(entries));memset(namespaces,0,sizeof(namespaces));namespace_count=0;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *out)
{
 test_nvs_calls++;
 if(strcmp(name,"dsr_cfg")&&strcmp(name,"dsr_bindings")&&strcmp(name,"dsr_device")&&strcmp(name,"dsr_battery")&&strcmp(name,"dsr_ui")&&strcmp(name,"dsr_auto")&&strcmp(name,"dsr_maint"))fail();
 for(unsigned i=0;i<namespace_count;i++)if(!strcmp(name,namespaces[i])){*out=(int)i+1;return ESP_OK;}
 if(mode==NVS_READONLY)return ESP_ERR_NVS_NOT_FOUND;
 if(namespace_count==16||strlen(name)>=16)fail();
 strcpy(namespaces[namespace_count++],name);*out=(int)namespace_count;return ESP_OK;
}
void nvs_close(nvs_handle_t h){if(h<=0||(unsigned)h>namespace_count)fail();}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size)
{test_nvs_calls++;int i=find(h,key);if(i<0)return ESP_ERR_NVS_NOT_FOUND;if(!out){*size=entries[i].size;return ESP_OK;}if(*size<entries[i].size)return ESP_ERR_INVALID_SIZE;*size=entries[i].size;memcpy(out,entries[i].bytes,*size);return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *bytes,size_t size)
{
 test_nvs_calls++;test_nvs_writes++;if(strcmp(namespaces[h-1],"dsr_maint"))test_active_writes++;if(cut())return ESP_FAIL;int i=find(h,key);if(i<0){for(i=0;i<200;i++)if(!entries[i].ns)break;}if(i==200||strlen(key)>=16)fail();
 uint8_t *copy=malloc(size);if(!copy)return ESP_ERR_NO_MEM;memcpy(copy,bytes,size);
 if(entries[i].bytes){memset(entries[i].bytes,0,entries[i].size);free(entries[i].bytes);}entries[i].ns=h;entries[i].size=size;entries[i].bytes=copy;strcpy(entries[i].key,key);return ESP_OK;
}
esp_err_t nvs_get_u32(nvs_handle_t h,const char *key,uint32_t *out){size_t n=sizeof(*out);return nvs_get_blob(h,key,out,&n);}
esp_err_t nvs_set_u32(nvs_handle_t h,const char *key,uint32_t v){return nvs_set_blob(h,key,&v,sizeof(v));}
esp_err_t nvs_get_u8(nvs_handle_t h,const char *key,uint8_t *out){size_t n=sizeof(*out);return nvs_get_blob(h,key,out,&n);}
esp_err_t nvs_set_u8(nvs_handle_t h,const char *key,uint8_t v){return nvs_set_blob(h,key,&v,sizeof(v));}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;test_nvs_calls++;return cut()?ESP_FAIL:ESP_OK;}
esp_err_t nvs_erase_all(nvs_handle_t h)
{test_nvs_calls++;test_nvs_writes++;if(strcmp(namespaces[h-1],"dsr_maint"))test_active_writes++;if(cut())return ESP_FAIL;for(unsigned i=0;i<200;i++)if(entries[i].ns==h){memset(entries[i].bytes,0,entries[i].size);free(entries[i].bytes);memset(entries+i,0,sizeof(entries[i]));}return ESP_OK;}
bool dsr_activity_begin(void){return true;}
void dsr_activity_end(void){}
void dsr_activity_data_changed(void){}
int64_t esp_timer_get_time(void){return 0;}
bool dsr_maintenance_boot_writes_allowed(void){return test_boot_allowed;}
esp_err_t nvs_erase_key(nvs_handle_t h,const char *key)
{test_nvs_calls++;test_nvs_writes++;if(strcmp(namespaces[h-1],"dsr_maint"))test_active_writes++;int i=find(h,key);if(i<0)return ESP_ERR_NVS_NOT_FOUND;memset(entries[i].bytes,0,entries[i].size);free(entries[i].bytes);memset(entries+i,0,sizeof(entries[i]));return ESP_OK;}
