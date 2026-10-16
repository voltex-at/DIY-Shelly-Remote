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
static unsigned masked_programs;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
enum {DATA_SIZE=0x3e0000};
static uint8_t flash_storage[DATA_SIZE], *flash=flash_storage, original[DATA_SIZE];
static esp_partition_t partition={.address=0xc20000,.size=DATA_SIZE,.label="dsrdata"};
test_flashchip_t g_rom_flashchip={.page_size=256,.sector_size=4096};
static const esp_vfs_fs_ops_t *vfs;
static void *context;
static bool read_failure, write_failure;
static unsigned priority=3;
static size_t interrupted_offset=DATA_SIZE;
static bool gc_workload;
/* Optional process-exit fault hook. No production code or SDK source changes. */
static void (*mutation_hook)(void *,const void *,size_t,bool);
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
 if(mutation_hook)mutation_hook(flash+offset,data,size,false);
 /* Match NOR + the default SDK driver: ones do not restore cleared bits.
  * SPIFFS intentionally programs flag masks over already programmed bytes.
  * CONFIG_SPI_FLASH_VERIFY_WRITE is off in this target's SDK configuration. */
 const uint8_t *in=data;for(size_t i=0;i<size;i++){if((flash[offset+i]&in[i])!=in[i])masked_programs++;flash[offset+i]&=in[i];}writes++;return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t *p,size_t offset,size_t size)
{CHECK(p==&partition&&offset%4096==0&&size%4096==0&&offset<=DATA_SIZE&&size<=DATA_SIZE-offset&&(priority==0||gc_workload));if(write_failure||offset>=interrupted_offset)return ESP_FAIL;if(mutation_hook)mutation_hook(flash+offset,NULL,size,true);memset(flash+offset,0xff,size);erases++;return ESP_OK;}
esp_err_t esp_vfs_register_fs(const char *path,const esp_vfs_fs_ops_t *ops,int flags,void *ctx)
{CHECK(!strcmp(path,"/dsr")&&flags==3&&ops&&ctx&&!vfs);vfs=ops;context=ctx;return ESP_OK;}
esp_err_t esp_vfs_unregister(const char *path)
{CHECK(!strcmp(path,"/dsr")&&vfs);vfs=NULL;context=NULL;return ESP_OK;}
static void unmount(void)
{CHECK(esp_vfs_spiffs_unregister("dsrdata")==ESP_OK&&!vfs&&!locks&&!esp_spiffs_mounted("dsrdata"));}
static void ready(void)
{dsr_config_health_t h;dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_READY&&h.storage_error==ESP_OK&&vfs&&locks==1&&priority==3);}
static void expected_header(unsigned block,uint8_t out[4])
{uint16_t magic=(uint16_t)(0x20140529u^256u^((DATA_SIZE/4096)-block));out[0]=(uint8_t)magic;out[1]=(uint8_t)(magic>>8);out[2]=out[3]=0;}
static void remnant(unsigned complete)
{memset(flash,0xff,DATA_SIZE);for(unsigned block=0;block<complete;block++)expected_header(block,flash+block*4096+252);}
static unsigned roundtrip_count;
static void roundtrip(void)
{
 char name[24];snprintf(name,sizeof(name),"/resume%u.dat",roundtrip_count++);
 const char text[]="resumed format holds a real file";
 int fd=vfs->open_p(context,name,O_WRONLY|O_CREAT|O_TRUNC,0);CHECK(fd>=0);
 CHECK(vfs->write_p(context,fd,text,sizeof(text))==(ssize_t)sizeof(text));
 CHECK(vfs->fsync_p(context,fd)==0&&vfs->close_p(context,fd)==0);unmount();
 const unsigned w=writes,e=erases;
 CHECK(dsr_storage_mount()==ESP_OK);ready();
 fd=vfs->open_p(context,name,O_RDONLY,0);CHECK(fd>=0);char out[sizeof(text)]={0};
 CHECK(vfs->read_p(context,fd,out,sizeof(out))==(ssize_t)sizeof(text)&&!memcmp(out,text,sizeof(text)));
 CHECK(vfs->close_p(context,fd)==0);unmount();CHECK(writes==w&&erases==e);
}
static void refused(void)
{
 memcpy(original,flash,DATA_SIZE);const unsigned w=writes,e=erases;
 CHECK(dsr_storage_mount()==ESP_FAIL&&!vfs&&!locks&&priority==3);
 dsr_config_health_t h;dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_NONEMPTY);
 CHECK(writes==w&&erases==e&&!memcmp(original,flash,DATA_SIZE));
}
int main(int argc,char **argv)
{
 CHECK(argc==2);
 memset(flash,0xff,DATA_SIZE);
 CHECK(dsr_storage_mount()==ESP_OK);ready();
 CHECK(writes>0&&erases==DATA_SIZE/4096); /* Exactly ONE real full format pass. */
 const char payload[]="DSR test checkpoint persists across mount, not a user config";
 int fd=vfs->open_p(context,"/config0.dat",O_WRONLY|O_CREAT|O_TRUNC,0);CHECK(fd>=0);
 CHECK(vfs->write_p(context,fd,payload,sizeof(payload))==(ssize_t)sizeof(payload));
 CHECK(vfs->fsync_p(context,fd)==0&&vfs->close_p(context,fd)==0);unmount();
 memcpy(original,flash,DATA_SIZE);unsigned saved_writes=writes,saved_erases=erases;
 for(unsigned n=0;n<10;n++){
  CHECK(dsr_storage_mount()==ESP_OK);ready();
  fd=vfs->open_p(context,"/config0.dat",O_RDONLY,0);CHECK(fd>=0);char out[sizeof(payload)+1]={0};
  CHECK(vfs->read_p(context,fd,out,sizeof(out))==(ssize_t)sizeof(payload)&&!memcmp(out,payload,sizeof(payload)));
  CHECK(vfs->close_p(context,fd)==0);unmount();
  CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,DATA_SIZE));
 }
 /* A single unknown byte, including the last byte, must prevent formatting. */
 for(unsigned n=0;n<2;n++){
  memset(flash,0xff,DATA_SIZE);flash[n?DATA_SIZE-1:0]=0;memcpy(original,flash,DATA_SIZE);
  CHECK(dsr_storage_mount()==ESP_FAIL&&!vfs&&!locks);
  dsr_config_health_t h;dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_NONEMPTY);
  CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,DATA_SIZE));
 }
 memset(flash,0xff,DATA_SIZE);read_failure=true;
 CHECK(dsr_storage_mount()!=ESP_OK&&!vfs&&!locks&&writes==saved_writes&&erases==saved_erases);
 dsr_config_health_t h;dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_READ_ERROR);
 read_failure=false;write_failure=true;
 CHECK(dsr_storage_mount()!=ESP_OK&&!vfs&&!locks&&writes==saved_writes&&erases==saved_erases);
 dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_PROVISION_ERROR);
 write_failure=false;CHECK(dsr_storage_mount()==ESP_OK);ready();unmount();
 /* Mount the independent IDF spiffsgen fixture already used by offline recovery. */
 FILE *reference=fopen(argv[1],"rb");CHECK(reference);
 CHECK(fread(flash,1,DATA_SIZE,reference)==DATA_SIZE&&fgetc(reference)==EOF);CHECK(fclose(reference)==0);
 memcpy(original,flash,DATA_SIZE);saved_writes=writes;saved_erases=erases;
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
 CHECK(writes==saved_writes&&erases==saved_erases&&!memcmp(original,flash,DATA_SIZE));
 /* Interrupted first format (observed 313/992 and 109/992 prefixes): the
  * remnant holds only formatter headers, so the next boot resumes ONE full
  * format pass instead of refusing forever. Synthetic prefixes, no dump. */
 const unsigned prefixes[]={1,109,313,991};
 for(unsigned p=0;p<sizeof(prefixes)/sizeof(prefixes[0]);p++){
  remnant(prefixes[p]);saved_writes=writes;saved_erases=erases;gc_workload=prefixes[p]==991;
  CHECK(dsr_storage_mount()==ESP_OK);ready();
  /* 991/992: SPIFFS_mount itself repairs a single unformatted block. */
  CHECK(prefixes[p]==991?erases==saved_erases+1:erases==saved_erases+DATA_SIZE/4096);gc_workload=false;
  roundtrip();
 }
 /* The ACTUAL upstream formatter, interrupted by I/O after sector 109/313,
  * produces a remnant that the next boot completes with one pass. */
 const unsigned cuts[]={109,313};
 for(unsigned c=0;c<2;c++){
  memset(flash,0xff,DATA_SIZE);saved_erases=erases;interrupted_offset=cuts[c]*4096;
  CHECK(dsr_storage_mount()==ESP_FAIL&&!vfs&&!locks&&priority==3);
  dsr_config_health_snapshot(&h);CHECK(h.storage==DSR_STORAGE_PROVISION_ERROR);
  CHECK(erases==saved_erases+cuts[c]);
  memcpy(original,flash,DATA_SIZE);remnant(cuts[c]);CHECK(!memcmp(original,flash,DATA_SIZE));
  interrupted_offset=DATA_SIZE;saved_erases=erases;
  CHECK(dsr_storage_mount()==ESP_OK);ready();CHECK(erases==saved_erases+DATA_SIZE/4096);
  roundtrip();
 }
 /* NOR-partial header programs on the first unfinished block (power cut
  * between/within the 2-byte writes), including block 0, also resume. */
 const uint8_t partial[][4]={{0xff,0xff,0x00,0x00},{0xff,0xff,0x7f,0xff},{0xfd,0xff,0x00,0x00}};
 for(unsigned n=0;n<3;n++)for(unsigned at=0;at<2;at++){
  const unsigned block=at?109:0;remnant(block);
  uint8_t want[4];expected_header(block,want);
  for(unsigned i=0;i<4;i++)flash[block*4096+252+i]=partial[n][i]|want[i];
  CHECK(memcmp(flash+block*4096+252,want,4)!=0);
  saved_erases=erases;CHECK(dsr_storage_mount()==ESP_OK);ready();
  CHECK(erases==saved_erases+DATA_SIZE/4096);roundtrip();
 }
 /* Anything a formatter cannot have written is refused byte-for-byte:
  * object-lookup entry, data page byte, last byte, a header on a block after
  * the first unfinished one (gap), a cleared bit outside the header pattern,
  * a nonzero erase count and two partial blocks. */
 const unsigned bad_offsets[]={108*4096+0,108*4096+250,108*4096+256,4096*40+4095,DATA_SIZE-1,200*4096+252,200*4096+254};
 for(unsigned n=0;n<sizeof(bad_offsets)/sizeof(bad_offsets[0]);n++){
  remnant(109);flash[bad_offsets[n]]&=0xfe;
  refused();
 }
 remnant(109);flash[50*4096+254]=1;refused();          /* erase count != 0 */
 remnant(109);flash[50*4096+252]&=0x7f;refused(); /* magic bit cleared */
 remnant(109);flash[109*4096+254]=0;flash[110*4096+254]=0;refused(); /* two partial */
 remnant(109);for(unsigned block=200;block<210;block++){uint8_t w[4];expected_header(block,w);memcpy(flash+block*4096+252,w,4);}refused(); /* gap */
 /* The real file-bearing filesystem from the start of this test is never a remnant. */
 memset(flash,0xff,DATA_SIZE);CHECK(dsr_storage_mount()==ESP_OK);ready();
 fd=vfs->open_p(context,"/keep.dat",O_WRONLY|O_CREAT|O_TRUNC,0);CHECK(fd>=0);
 CHECK(vfs->write_p(context,fd,payload,sizeof(payload))==(ssize_t)sizeof(payload));
 CHECK(vfs->fsync_p(context,fd)==0&&vfs->close_p(context,fd)==0);unmount();
 /* Corrupt block 900's magic so SPIFFS_mount fails with real lookup data present. */
 for(unsigned block=900;block<902;block++)flash[block*4096+252]=flash[block*4096+253]=0;
 refused();
 /* A remnant of another geometry (prior 4 MiB length magic) is not ours. */
 memset(flash,0xff,DATA_SIZE);
 for(unsigned block=0;block<109;block++){
  uint16_t magic=(uint16_t)(0x20140529u^256u^(1024u-block));
  flash[block*4096+252]=(uint8_t)magic;flash[block*4096+253]=(uint8_t)(magic>>8);flash[block*4096+254]=flash[block*4096+255]=0;
 }
 refused();
 printf("PASS: %u IDF/SPIFFS integration checks; single 992-sector format at idle priority, interrupted-format resume (1/109/313/991 prefixes, real 109/313 cuts, partial headers) and byte-exact refusal of anything else, file write/read, ten remounts, I/O failures and upstream image; simulated NOR, not hardware\n",checks);
 return 0;
}
