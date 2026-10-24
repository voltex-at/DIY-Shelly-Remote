#include "dsr_maintenance_io.h"
#include "dsr_platform.h"
#include "dsr_ble.h"
#include "esp_heap_caps.h"
#include "esp_spiffs.h"
#include "esp_random.h"
#include "nvs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
struct dsr_maint_file {int fd;bool writing;};
static bool path(const char *name,char out[64])
{
 if(!name||strlen(name)!=27||name[0]!='m'||memcmp(name+23,".s",2))return false;
 for(unsigned i=1;i<23;i++)if(!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",name[i]))return false;
 unsigned index=0;for(unsigned i=25;i<27;i++){char c=name[i];int n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;if(n<0)return false;index=index*16+(unsigned)n;}
 if(index>16)return false;
 snprintf(out,64,"/dsr/%s",name);return true;
}
dsr_result_t dsr_maint_io_limits(dsr_maint_limits_t *out)
{
 if(!out)return DSR_INVALID;
 memset(out,0,sizeof(*out));nvs_stats_t stats;
 if(esp_spiffs_info("dsrdata",&out->total,&out->used)!=ESP_OK||nvs_get_stats("nvs",&stats)!=ESP_OK)return DSR_IO;
 out->nvs_available=stats.available_entries;out->psram_free=heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);out->psram_largest=heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 out->internal_free=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);out->internal_largest=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);return DSR_OK;
}
dsr_result_t dsr_maint_io_random(uint8_t *out,size_t size)
{
 if(!out||!size||size>64)return DSR_INVALID;
 /* IDF5.5.4 guarantees true entropy with RF enabled. Do not switch ADC/boot
  * entropy sources behind the sole battery worker. Import over HTTPS has WiFi. */
 if(!dsr_wifi_connected()&&!dsr_ble_host_active())return DSR_OFFLINE;
 esp_fill_random(out,size);return DSR_OK;
}
dsr_result_t dsr_maint_io_identifier(uint8_t out[16])
{if(!out)return DSR_INVALID;esp_fill_random(out,16);uint8_t v=0;for(unsigned i=0;i<16;i++)v|=out[i];return v?DSR_OK:DSR_IO;}
void *dsr_maint_io_alloc(size_t size)
{return size&&size<=2097152?heap_caps_calloc(1,size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT):NULL;}
void dsr_maint_io_free(void *owned,size_t size)
{if(!owned)return;volatile uint8_t *p=owned;while(size--)*p++=0;free(owned);}
dsr_result_t dsr_maint_io_exists(const char *name,bool *exists)
{
 char filename[64];if(!exists||!path(name,filename))return DSR_INVALID;
 *exists=false;struct stat st;if(!stat(filename,&st)){*exists=true;return DSR_OK;}return errno==ENOENT?DSR_OK:DSR_IO;
}
static dsr_result_t open_file(const char *name,dsr_maint_file_t **out,size_t *size,bool writing)
{
 if(!out)return DSR_INVALID;
 *out=NULL;char filename[64];if(!path(name,filename)||(!writing&&!size))return DSR_INVALID;
 dsr_maint_file_t *f=calloc(1,sizeof(*f));if(!f)return DSR_FULL;
 f->fd=open(filename,writing?O_WRONLY|O_CREAT|O_EXCL:O_RDONLY,0600);f->writing=writing;
 if(f->fd<0){int e=errno;free(f);return e==EEXIST?DSR_CONFLICT:DSR_IO;}
 if(!writing){struct stat st;if(fstat(f->fd,&st)||st.st_size<0||(uint64_t)st.st_size>115200+DSR_MAINT_RECORD_HEADER){close(f->fd);free(f);return DSR_IO;}*size=(size_t)st.st_size;}
 *out=f;return DSR_OK;
}
dsr_result_t dsr_maint_io_open_write(const char *name,dsr_maint_file_t **out){return open_file(name,out,NULL,true);}
dsr_result_t dsr_maint_io_open_read(const char *name,dsr_maint_file_t **out,size_t *size){return open_file(name,out,size,false);}
dsr_result_t dsr_maint_io_write(dsr_maint_file_t *f,const uint8_t *bytes,size_t size)
{
 if(!f||!f->writing||!bytes||!size||size>DSR_MAINT_IO_CHUNK)return DSR_INVALID;
 size_t at=0;while(at<size){ssize_t n=write(f->fd,bytes+at,size-at);if(n<0&&errno==EINTR)continue;if(n<=0)return DSR_IO;at+=(size_t)n;}return DSR_OK;
}
dsr_result_t dsr_maint_io_read(dsr_maint_file_t *f,uint8_t *bytes,size_t size,size_t *got)
{
 if(!f||f->writing||!bytes||!got||!size||size>DSR_MAINT_IO_CHUNK)return DSR_INVALID;
 *got=0;ssize_t n;do{n=read(f->fd,bytes,size);}while(n<0&&errno==EINTR);if(n<0)return DSR_IO;*got=(size_t)n;return DSR_OK;
}
dsr_result_t dsr_maint_io_close(dsr_maint_file_t *f)
{
 if(!f)return DSR_INVALID;
 bool ok=!f->writing||fsync(f->fd)==0;if(close(f->fd))ok=false;free(f);return ok?DSR_OK:DSR_IO;
}
dsr_result_t dsr_maint_io_remove(const char *name)
{
 char filename[64];if(!path(name,filename))return DSR_INVALID;
 if(unlink(filename)&&errno!=ENOENT)return DSR_IO;
 bool exists;dsr_result_t r=dsr_maint_io_exists(name,&exists);return r==DSR_OK&&!exists?DSR_OK:DSR_IO;
}
