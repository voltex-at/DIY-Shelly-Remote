#include "dsr_workspace.h"
#include "dsr_photo_name.h"
#include "dsr_activity.h"
#include "esp_spiffs.h"
#include "mbedtls/sha256.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

#define PHOTO_BYTES DSR_PHOTO_BYTES
/* Immutable, content-addressed cache. No pointer is freed while LVGL can use
 * it. At most 16 photos = 1.76 MiB of the confirmed 8 MiB PSRAM. */
static struct {char id[33];void *pixels;} photos[16];
static atomic_uint count;
static bool valid_id(const char *id)
{if(!id||strlen(id)!=32)return false;for(unsigned i=0;i<32;i++)if(!((id[i]>='0'&&id[i]<='9')||(id[i]>='a'&&id[i]<='f')))return false;return true;}
const void *dsr_photo_pixels(const char *id)
{if(!valid_id(id))return NULL;for(unsigned i=0;i<count;i++)if(!strcmp(photos[i].id,id))return photos[i].pixels;return NULL;}
const char *dsr_photo_id(unsigned index){return index<atomic_load(&count)?photos[index].id:NULL;}
static bool content_id(const char *id,const void *data)
{unsigned char digest[32];char actual[33];mbedtls_sha256(data,PHOTO_BYTES,digest,0);for(unsigned i=0;i<16;i++)sprintf(actual+2*i,"%02x",digest[i]);return !strcmp(id,actual);}
bool dsr_photo_save(const char *id,const void *data,size_t size)
{
 if(!valid_id(id)||size!=PHOTO_BYTES||!content_id(id,data))return false;
 if(dsr_photo_pixels(id))return true;
 if(count>=16)return false;
 if(!dsr_activity_begin())return false;
 size_t total,used;if(esp_spiffs_info("dsrdata",&total,&used)!=ESP_OK||used+PHOTO_BYTES+65536>total*3/4){dsr_activity_end();return false;}
 void *copy=malloc(size);if(!copy){dsr_activity_end();return false;}memcpy(copy,data,size);
 char name[38],path[64];if(!dsr_photo_filename(id,name)){free(copy);dsr_activity_end();return false;}
 dsr_activity_persistent_write();snprintf(path,sizeof(path),"/dsr/%s",name);FILE *f=fopen(path,"wb");bool ok=false;
 if(f){ok=fwrite(data,1,size,f)==size&&fflush(f)==0&&fsync(fileno(f))==0;if(fclose(f))ok=false;}
 if(!ok){free(copy);dsr_activity_end();return false;}
 memcpy(photos[count].id,id,33);photos[count].pixels=copy;count++;dsr_activity_end();return true;
}
void dsr_photos_load(void)
{
 DIR *dir=opendir("/dsr");if(!dir)return;struct dirent *entry;
 while(count<16&&(entry=readdir(dir))){
  char id[33];if(!dsr_photo_filename_id(entry->d_name,id)||dsr_photo_pixels(id))continue;
  char path[64];snprintf(path,sizeof(path),"/dsr/%.37s",entry->d_name);FILE *f=fopen(path,"rb");if(!f)continue;
  void *data=malloc(PHOTO_BYTES);bool ok=data&&fread(data,1,PHOTO_BYTES,f)==PHOTO_BYTES&&fgetc(f)==EOF;fclose(f);
  if(!ok||!content_id(id,data)){free(data);continue;}memcpy(photos[count].id,id,33);photos[count++].pixels=data;
 }closedir(dir);
}
