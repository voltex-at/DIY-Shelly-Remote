#include "nvs_fixture.h"
#include "nvs.h"
#include <assert.h>
#include <string.h>
uint8_t auto_flash[2][3072];size_t auto_lengths[2];
unsigned auto_fault,auto_commits,auto_sets,auto_opens;
static uint8_t staged[3072];static size_t staged_size;static unsigned staged_slot;
static bool readback;
void auto_flash_reset(void){memset(auto_flash,0,sizeof(auto_flash));memset(auto_lengths,0,sizeof(auto_lengths));
 auto_fault=auto_commits=auto_sets=auto_opens=0;staged_size=0;readback=false;}
void auto_flash_fault(unsigned fault){auto_fault=fault;readback=false;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h){
 assert(!strcmp(name,"dsr_auto")&&(mode==NVS_READONLY||mode==NVS_READWRITE));auto_opens++;
 if(auto_fault==1)return ESP_FAIL;
 if(mode==NVS_READONLY&&!auto_lengths[0]&&!auto_lengths[1])return ESP_ERR_NVS_NOT_FOUND;
 if(readback&&auto_fault==4)return ESP_FAIL;
 *h=mode+1;return ESP_OK;
}
static unsigned slot(const char *key){assert(!strcmp(key,"a")||!strcmp(key,"b"));return !strcmp(key,"b");}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size){
 assert(h==1||h==2);unsigned i=slot(key);if(!auto_lengths[i])return ESP_ERR_NVS_NOT_FOUND;
 if(!out){*size=auto_lengths[i];return ESP_OK;}
 if(*size<auto_lengths[i])return ESP_ERR_INVALID_SIZE;
 memcpy(out,auto_flash[i],auto_lengths[i]);*size=auto_lengths[i];
 if(readback&&auto_fault==5&&*size)((uint8_t*)out)[*size-1]^=1;
 return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size){
 assert(h==2&&size<=sizeof(staged));auto_sets++;
 if(auto_fault==2)return ESP_FAIL;
 staged_slot=slot(key);memcpy(staged,data,size);staged_size=size;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h){
 assert(h==2&&staged_size);auto_commits++;
 if(auto_fault==3)return ESP_FAIL;
 memcpy(auto_flash[staged_slot],staged,staged_size);auto_lengths[staged_slot]=staged_size;
 readback=true;return ESP_OK;
}
void nvs_close(nvs_handle_t h){assert(h==1||h==2);staged_size=0;}
