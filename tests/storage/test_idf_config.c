/* Actual config store through original IDF SPIFFS. POSIX bridge and simulated
 * NOR only: no physical Flash, RTOS scheduling, libc buffering, or device dump. */
#define main test_original_main
#include "test_idf_spiffs.c"
#undef main
#include <errno.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include "dsr_config.h"
#include "spiffs.h"

typedef struct { int fd; } test_file_t;
static unsigned format_calls;
s32_t __real_SPIFFS_format(spiffs *);
s32_t __wrap_SPIFFS_format(spiffs *fs)
{format_calls++;return __real_SPIFFS_format(fs);}
FILE *test_fopen(const char *path,const char *mode)
{
 CHECK(!strncmp(path,"/dsr/",5)&&vfs);
 CHECK(!strcmp(mode,"rb")||!strcmp(mode,"wb"));
 int flags=!strcmp(mode,"wb")?O_WRONLY|O_CREAT|O_TRUNC:O_RDONLY;
 int fd=vfs->open_p(context,path+4,flags,0);if(fd<0)return NULL;
 test_file_t *file=malloc(sizeof(*file));CHECK(file);file->fd=fd;return (FILE *)file;
}
size_t test_fread(void *out,size_t size,size_t count,FILE *stream)
{
 CHECK(!size||count<=SIZE_MAX/size);if(!size||!count)return 0;
 ssize_t n=vfs->read_p(context,((test_file_t *)stream)->fd,out,size*count);
 return n<0?0:(size_t)n/size;
}
size_t test_fwrite(const void *data,size_t size,size_t count,FILE *stream)
{
 CHECK(!size||count<=SIZE_MAX/size);if(!size||!count)return 0;
 ssize_t n=vfs->write_p(context,((test_file_t *)stream)->fd,data,size*count);
 return n<0?0:(size_t)n/size;
}
int test_fgetc(FILE *stream)
{unsigned char byte;return test_fread(&byte,1,1,stream)==1?byte:EOF;}
int test_fflush(FILE *stream){CHECK(stream);return 0;}
int test_fileno(FILE *stream){return ((test_file_t *)stream)->fd;}
int test_fsync(int fd){return vfs->fsync_p(context,fd);}
int test_fclose(FILE *stream)
{int rc=vfs->close_p(context,((test_file_t *)stream)->fd);free(stream);return rc;}

static unsigned mutation_count,cut_at,cut_part;
static void power_cut(void *where,const void *bytes,size_t size,bool erase)
{
 if(++mutation_count!=cut_at)return;
 /* A cut before, halfway through, or just after one NOR operation. */
 size_t changed=cut_part==0?0:cut_part==1?size/2:size;
 if(erase)memset(where,0xff,changed);
 else {uint8_t *out=where;const uint8_t *in=bytes;for(size_t i=0;i<changed;i++)out[i]&=in[i];}
 _exit(77); /* No close/unmount/cache flush: process state is really lost. */
}
static char *document(unsigned revision)
{
 char *initial=dsr_config_default();CHECK(initial);cJSON *root=cJSON_Parse(initial);free(initial);CHECK(root);
 cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(root,"revision"),revision);
 cJSON *rooms=cJSON_GetObjectItemCaseSensitive(root,"rooms"),*devices=cJSON_GetObjectItemCaseSensitive(root,"devices");
 for(unsigned i=1;i<=8;i++){
  cJSON *room=cJSON_CreateObject();CHECK(room);cJSON_AddNumberToObject(room,"id",i);
  char name[48];snprintf(name,sizeof(name),"Room %u synthetic checkpoint revision %u",i,revision);
  cJSON_AddStringToObject(room,"name",name);cJSON_AddNumberToObject(room,"icon",i%4);cJSON_AddStringToObject(room,"photo","");cJSON_AddItemToArray(rooms,room);
 }
 for(unsigned i=1;i<=16;i++){
  cJSON *device=cJSON_CreateObject();CHECK(device);cJSON_AddNumberToObject(device,"id",i);
  char name[48];snprintf(name,sizeof(name),"Device %u synthetic checkpoint revision %u",i,revision);
  cJSON_AddStringToObject(device,"name",name);cJSON_AddStringToObject(device,"host","192.0.2.10");
  cJSON_AddNumberToObject(device,"channel",i-1);cJSON_AddStringToObject(device,"kind","relay");cJSON_AddNumberToObject(device,"room",1+(i-1)%8);cJSON_AddItemToArray(devices,device);
 }
 char error[96];CHECK(dsr_config_validate(root,error));char *json=cJSON_PrintUnformatted(root);cJSON_Delete(root);CHECK(json);return json;
}
static void expect_document(unsigned low,unsigned high)
{
 bool found=false;char error[96];char *loaded=dsr_config_store_load("/dsr",&found,error);CHECK(found&&loaded);
 unsigned rev=dsr_config_revision(loaded);CHECK(rev==low||rev==high);
 char *expected=document(rev);CHECK(!strcmp(loaded,expected));free(expected);free(loaded);
}
static void sentinel(bool create)
{
 int fd=vfs->open_p(context,"/sentinel.rgb",create?O_WRONLY|O_CREAT|O_TRUNC:O_RDONLY,0);CHECK(fd>=0);
 for(unsigned block=0;block<256;block++){
  uint8_t data[256];for(unsigned i=0;i<256;i++)data[i]=(uint8_t)((block+i)%251);
  if(create)CHECK(vfs->write_p(context,fd,data,sizeof(data))==(ssize_t)sizeof(data));
  else {uint8_t out[256];CHECK(vfs->read_p(context,fd,out,sizeof(out))==(ssize_t)sizeof(out)&&!memcmp(data,out,sizeof(out)));}
 }
 if(create)CHECK(vfs->fsync_p(context,fd)==0);
 else {uint8_t end;CHECK(vfs->read_p(context,fd,&end,1)==0);}
 CHECK(vfs->close_p(context,fd)==0);
}
static unsigned save_trial(const uint8_t *baseline,uint8_t *shared,const char *next,unsigned cut,unsigned part)
{
 memcpy(shared,baseline,DATA_SIZE);pid_t child=fork();CHECK(child>=0);
 if(!child){
  CHECK(dsr_storage_mount()==ESP_OK);ready();mutation_count=0;cut_at=cut;cut_part=part;mutation_hook=power_cut;
  char error[96];CHECK(dsr_config_store_save("/dsr",next,error));
  mutation_hook=NULL;unmount();_exit(0);
 }
 int status;CHECK(waitpid(child,&status,0)==child&&WIFEXITED(status));
 unsigned code=(unsigned)WEXITSTATUS(status);CHECK(code==0||code==77);return code;
}
int main(int argc,char **argv)
{
 setvbuf(stdout,NULL,_IONBF,0);
 CHECK(argc==2||argc==3);CHECK(test_original_main(2,argv)==0);CHECK(format_calls==4);
 uint8_t *shared=mmap(NULL,DATA_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);CHECK(shared!=MAP_FAILED);
 flash=shared;gc_workload=true;memset(flash,0xff,DATA_SIZE);CHECK(dsr_storage_mount()==ESP_OK);ready();
 sentinel(true);
 unsigned before_gc=erases;
 /* Exercise both real slots until original SPIFFS has actually collected. */
 unsigned revision=1;char error[96];
 for(;revision<=1800;revision++){
  /* Capture the last durable Flash image before the checkpoint that forces
   * real GC, rather than testing only a cheap write immediately after GC. */
  if(revision>=100)memcpy(original,flash,DATA_SIZE);
  char *json=document(revision);CHECK(dsr_config_store_save("/dsr",json,error));free(json);
  if(revision%50==0){unmount();CHECK(dsr_storage_mount()==ESP_OK);ready();expect_document(revision,revision);}
  if(erases>before_gc+10&&revision>=100)break;
 }
 CHECK(revision<=1800&&erases>before_gc+10);expect_document(revision,revision);unmount();
 CHECK(revision>1);unsigned previous=revision-1;char *next=document(revision);memcpy(flash,original,DATA_SIZE);
 /* Count the exact next checkpoint's NOR operations without a cut. */
 CHECK(dsr_storage_mount()==ESP_OK);ready();expect_document(previous,previous);mutation_count=0;cut_at=UINT32_MAX;mutation_hook=power_cut;
 unsigned erases_before=erases;
 CHECK(dsr_config_store_save("/dsr",next,error));mutation_hook=NULL;unsigned operations=mutation_count;
 unsigned gc_erases=erases-erases_before;CHECK(operations>0&&operations<1000&&gc_erases>0);expect_document(revision,revision);unmount();
 unsigned cuts=0;
 for(unsigned operation=1;operation<=operations;operation++)for(unsigned part=0;part<3;part++){
  CHECK(save_trial(original,shared,next,operation,part)==77);cuts++;
  /* Original SPIFFS may finish one interrupted GC erase at mount. That is
   * upstream recovery, distinct from a destructive full format. */
  unsigned old_formats=format_calls;CHECK(dsr_storage_mount()==ESP_OK);ready();CHECK(format_calls==old_formats);
  expect_document(previous,revision);sentinel(false);unmount();
 }
 CHECK(save_trial(original,shared,next,UINT32_MAX,0)==0);CHECK(dsr_storage_mount()==ESP_OK);ready();expect_document(revision,revision);
 char *later=document(revision+1);write_failure=true;CHECK(!dsr_config_store_save("/dsr",later,error));write_failure=false;expect_document(revision,revision);sentinel(false);free(later);unmount();
 free(next);CHECK(munmap(shared,DATA_SIZE)==0);flash=flash_storage;
 printf("PASS: actual config/SPIFFS %u revisions through GC; %u NOR operations including %u GC erases, %u process-exit cuts (before/half/after), prior or exact new checkpoint retained; simulated NOR, not owner dump/hardware\n",revision,operations,gc_erases,cuts);
 if(argc==3){
  FILE *input=fopen(argv[2],"rb");CHECK(input);CHECK(fread(flash,1,DATA_SIZE,input)==DATA_SIZE&&fgetc(input)==EOF&&fclose(input)==0);
  memcpy(original,flash,DATA_SIZE);unsigned saved_writes=writes,saved_erases=erases,saved_formats=format_calls;
  CHECK(dsr_storage_mount()==ESP_OK);ready();bool found=true;char *empty=dsr_config_store_load("/dsr",&found,error);CHECK(!empty&&!found&&!error[0]);unmount();
  CHECK(writes==saved_writes&&erases==saved_erases&&format_calls==saved_formats&&!memcmp(original,flash,DATA_SIZE));
  CHECK(dsr_storage_mount()==ESP_OK);ready();char *initial=dsr_config_default();CHECK(initial&&dsr_config_store_save("/dsr",initial,error));free(initial);
  char *rooms=document(2);CHECK(dsr_config_store_save("/dsr",rooms,error));free(rooms);unmount();
  CHECK(dsr_storage_mount()==ESP_OK);ready();expect_document(2,2);unmount();CHECK(format_calls==saved_formats);
  puts("PASS: exact FACTORY SPIFFS mounts unchanged with zero format/write/erase; actual default + rooms checkpoint persists on simulated NOR copy");
 }
 return 0;
}
