/* Original IDF SPIFFS wrapper/core + real dsr_storage, simulated NOR only.
 * No replacement of esp_vfs_spiffs_register/esp_spiffs_format/SPIFFS_mount.
 * Not a physical Flash, RTOS, libc VFS, TLS or RF acceptance test. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_rom_spiflash.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "dsr_storage.h"
#include "dsr_config_health.h"

static unsigned checks, writes, erases, locks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
enum {DATA_SIZE=0x3e0000};
static uint8_t flash[DATA_SIZE], original[DATA_SIZE];
static esp_partition_t partition={.address=0xc20000,.size=DATA_SIZE,.label="dsrdata"};
test_flashchip_t g_rom_flashchip={.page_size=256,.sector_size=4096};
static const esp_vfs_fs_ops_t *vfs;
static void *context;
static bool read_failure, write_failure;
static unsigned priority=3;
static size_t interrupted_offset=DATA_SIZE;
UBaseType_t uxTaskPriorityGet(TaskHandle_t task){CHECK(!task);return priority;}
void vTaskPrioritySet(TaskHandle_t task,UBaseType_t value)
{CHECK(!task&&((priority==3&&value==0)||(priority==0&&value==3)));priority=value;}
void vTaskDelay(TickType_t ticks){CHECK(ticks==1);}
int64_t esp_timer_get_time(void){static int64_t now;return now+=1000;}
SemaphoreHandle_t xSemaphoreCreateMutex(void) {locks++;return calloc(1,sizeof(bool));}
int xSemaphoreTake(SemaphoreHandle_t s,TickType_t timeout)
{(void)timeout;CHECK(s&&!*(bool *)s);*(bool *)s=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t s)
{CHECK(s&&*(bool *)s);*(bool *)s=false;return pdTRUE;}
void vSemaphoreDelete(SemaphoreHandle_t s) {CHECK(s&&locks);locks--;free(s);}
const esp_partition_t *esp_partition_find_first(unsigned type,unsigned subtype,const char *label)
{CHECK(type==1&&(subtype==0xff||subtype==0x82)&&label&&!strcmp(label,"dsrdata"));return &partition;}
esp_err_t esp_partition_read(const esp_partition_t *p,size_t offset,void *out,size_t size)
{CHECK(p==&partition&&offset<=DATA_SIZE&&size<=DATA_SIZE-offset);if(read_failure)return ESP_FAIL;memcpy(out,flash+offset,size);return ESP_OK;}
esp_err_t esp_partition_write(const esp_partition_t *p,size_t offset,const void *data,size_t size)
{
 CHECK(p==&partition&&offset<=DATA_SIZE&&size<=DATA_SIZE-offset);if(write_failure||offset>=interrupted_offset)return ESP_FAIL;
 const uint8_t *in=data;for(size_t i=0;i<size;i++){CHECK((flash[offset+i]&in[i])==in[i]);flash[offset+i]&=in[i];}writes++;return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t *p,size_t offset,size_t size)
{CHECK(p==&partition&&offset%4096==0&&size%4096==0&&offset<=DATA_SIZE&&size<=DATA_SIZE-offset&&priority==0);if(write_failure||offset>=interrupted_offset)return ESP_FAIL;memset(flash+offset,0xff,size);erases++;return ESP_OK;}
esp_err_t esp_vfs_register_fs(const char *path,const esp_vfs_fs_ops_t *ops,int flags,void *ctx)
{CHECK(!strcmp(path,"/dsr")&&flags==3&&ops&&ctx&&!vfs);vfs=ops;context=ctx;return ESP_OK;}
esp_err_t esp_vfs_unregister(const char *path)
{CHECK(!strcmp(path,"/dsr")&&vfs);vfs=NULL;context=NULL;return ESP_OK;}
static void unmount(void)
{CHECK(esp_vfs_spiffs_unregister("dsrdata")==ESP_OK&&!vfs&&!locks&&!esp_spiffs_mounted("dsrdata"));}
static void ready(void)
{dsr_config_health_t h;dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_READY&&h.storage_error==ESP_OK&&vfs&&locks==1&&priority==3);}
int main(int argc,char **argv)
{
 CHECK(argc==2);
 memset(flash,0xff,sizeof(flash));
 CHECK(dsr_storage_mount()==ESP_OK);ready();
 CHECK(writes>0&&erases==DATA_SIZE/4096); /* Exactly ONE real full format pass. */
 const char payload[]="DSR test checkpoint persists across mount, not a user config";
 int fd=vfs->open_p(context,"/config0.dat",O_WRONLY|O_CREAT|O_TRUNC,0);CHECK(fd>=0);
 CHECK(vfs->write_p(context,fd,payload,sizeof(payload))==(ssize_t)sizeof(payload));
 CHECK(vfs->fsync_p(context,fd)==0&&vfs->close_p(context,fd)==0);unmount();
 memcpy(original,flash,sizeof(flash));unsigned saved_writes=writes,saved_erases=erases;
 for(unsigned n=0;n<10;n++){
  CHECK(dsr_storage_mount()==ESP_OK);ready();
  fd=vfs->open_p(context,"/config0.dat",O_RDONLY,0);CHECK(fd>=0);char out[sizeof(payload)+1]={0};
  CHECK(vfs->read_p(context,fd,out,sizeof(out))==(ssize_t)sizeof(payload)&&!memcmp(out,payload,sizeof(payload)));
  CHECK(vfs->close_p(context,fd)==0);unmount();
  CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,sizeof(flash)));
 }
 /* A single unknown byte, including the last byte, must prevent formatting. */
 for(unsigned n=0;n<2;n++){
  memset(flash,0xff,sizeof(flash));flash[n?DATA_SIZE-1:0]=0;memcpy(original,flash,sizeof(flash));
  CHECK(dsr_storage_mount()==ESP_FAIL&&!vfs&&!locks);
  dsr_config_health_t h;dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_NONEMPTY);
  CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,sizeof(flash)));
 }
 memset(flash,0xff,sizeof(flash));read_failure=true;
 CHECK(dsr_storage_mount()!=ESP_OK&&!vfs&&!locks&&writes==saved_writes&&erases==saved_erases);
 dsr_config_health_t h;dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_READ_ERROR);
 read_failure=false;write_failure=true;
 CHECK(dsr_storage_mount()!=ESP_OK&&!vfs&&!locks&&writes==saved_writes&&erases==saved_erases);
 dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_PROVISION_ERROR);
 write_failure=false;CHECK(dsr_storage_mount()==ESP_OK);ready();unmount();
 /* Mount the independent IDF spiffsgen fixture already used by offline recovery. */
 FILE *reference=fopen(argv[1],"rb");CHECK(reference);
 CHECK(fread(flash,1,sizeof(flash),reference)==sizeof(flash)&&fgetc(reference)==EOF);CHECK(fclose(reference)==0);
 memcpy(original,flash,sizeof(flash));saved_writes=writes;saved_erases=erases;
 CHECK(dsr_storage_mount()==ESP_OK);ready();
 fd=vfs->open_p(context,"/large.bin",O_RDONLY,0);CHECK(fd>=0);
 for(unsigned block=0;block<200;block++){
  uint8_t bytes[256];CHECK(vfs->read_p(context,fd,bytes,sizeof(bytes))==256);
  for(unsigned n=0;n<256;n++)CHECK(bytes[n]==n);
 }
 uint8_t last;CHECK(vfs->read_p(context,fd,&last,1)==0&&vfs->close_p(context,fd)==0);
 fd=vfs->open_p(context,"/config0.dat",O_RDONLY,0);CHECK(fd>=0);
 uint32_t header[4];CHECK(vfs->read_p(context,fd,header,sizeof(header))==sizeof(header));
 const char json[]="{\"schema\":1,\"revision\":9}";char out[sizeof(json)]={0};
 CHECK(header[0]==0x44535231&&header[1]==sizeof(json)-1&&header[3]==9);
 CHECK(vfs->read_p(context,fd,out,sizeof(out))==sizeof(json)-1&&!strcmp(out,json));
 CHECK(vfs->close_p(context,fd)==0);unmount();
 CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,sizeof(flash)));
 /* Synthetic reproduction of the observed 313/992 formatting prefix; no dump. */
 memset(flash,0xff,sizeof(flash));
 for(unsigned block=0;block<313;block++){
  uint16_t magic=(uint16_t)(0x20140529u^256u^((DATA_SIZE/4096)-block));
  flash[block*4096+252]=(uint8_t)magic;flash[block*4096+253]=(uint8_t)(magic>>8);
  flash[block*4096+254]=flash[block*4096+255]=0;
 }
 memcpy(original,flash,sizeof(flash));CHECK(dsr_storage_mount()==ESP_FAIL&&!vfs&&!locks&&priority==3);
 dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_NONEMPTY);
 CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,sizeof(flash)));
 /* The original formatter interrupted by an I/O error produces that prefix. */
 memset(flash,0xff,sizeof(flash));interrupted_offset=313*4096;
 esp_err_t interrupted=dsr_storage_mount();
 CHECK(interrupted==ESP_FAIL&&!vfs&&!locks&&priority==3);
 dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_PROVISION_ERROR);
 CHECK(erases==saved_erases+313&&!memcmp(original,flash,sizeof(flash)));
 interrupted_offset=DATA_SIZE;saved_writes=writes;saved_erases=erases;
 CHECK(dsr_storage_mount()==ESP_FAIL&&!vfs&&!locks&&priority==3);
 dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_NONEMPTY);
 CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,sizeof(flash)));
 /* A prior 4 MiB length-magic geometry must not be reformatted as 3.875 MiB. */
 for(unsigned block=0;block<DATA_SIZE/4096;block++){
  uint16_t magic=(uint16_t)(0x20140529u^256u^(1024u-block));
  flash[block*4096+252]=(uint8_t)magic;flash[block*4096+253]=(uint8_t)(magic>>8);
 }
 memcpy(original,flash,sizeof(flash));CHECK(dsr_storage_mount()==ESP_FAIL&&!vfs&&!locks);
 dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_NONEMPTY);
 CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,sizeof(flash)));
 printf("PASS: %u IDF/SPIFFS integration checks; single 992-sector format at idle priority, 313-sector interruption/preservation, file write/read, ten remounts, I/O failures and upstream image; simulated NOR, not hardware\n",checks);
 return 0;
}
