/* Actual codec/loader and real pixel files. Directory ordering, allocation
 * failures and SDK admission/space providers are MODEL; SHA-256 is genuine. */
#include "dsr_workspace.h"
#include "dsr_photo_name.h"
#include "dsr_activity.h"
#include "esp_spiffs.h"
#include "mbedtls/sha256.h"
#include <openssl/evp.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(condition) do { checks++; if(!(condition)){fprintf(stderr,"Photo names check failed at %s:%d: %s\n",__FILE__,__LINE__,#condition);exit(1);} } while(0)
static const char *directory;
static unsigned char payload[DSR_PHOTO_BYTES];
static char ids[16][33],files[32][38];
static unsigned file_count,cursor,attempts,live,fail_until,read_opens;
static void *owned[16];
static DIR *active_directory;
static FILE *active_file;

int mbedtls_sha256(const unsigned char *data,size_t size,unsigned char out[32],int is224)
{
 unsigned length=0;
 CHECK(!is224&&EVP_Digest(data,size,out,&length,EVP_sha256(),NULL)==1&&length==32);
 return 0;
}
bool dsr_activity_begin(void){return true;}
void dsr_activity_end(void){}
esp_err_t esp_spiffs_info(const char *label,size_t *total,size_t *used)
{CHECK(!strcmp(label,"dsrdata"));*total=0x3e0000;*used=0;return ESP_OK;}

void *__real_malloc(size_t size);
void __real_free(void *pointer);
void *__wrap_malloc(size_t size)
{
 if(size==DSR_PHOTO_BYTES&&++attempts<=fail_until)return NULL;
 void *pointer=__real_malloc(size);
 if(pointer&&size==DSR_PHOTO_BYTES){CHECK(live<16);owned[live++]=pointer;}
 return pointer;
}
void __wrap_free(void *pointer)
{
 for(unsigned i=0;i<live;i++)if(owned[i]==pointer){owned[i]=owned[--live];break;}
 __real_free(pointer);
}

FILE *__real_fopen(const char *name,const char *mode);
int __real_fclose(FILE *file);
DIR *__real_opendir(const char *name);
struct dirent *__real_readdir(DIR *dir);
int __real_closedir(DIR *dir);
FILE *__wrap_fopen(const char *name,const char *mode)
{
 char path[512];CHECK(!strncmp(name,"/dsr/",5)&&!strchr(name+5,'/')&&!strcmp(mode,"rb"));
 int length=snprintf(path,sizeof(path),"%s/%s",directory,name+5);
 CHECK(length>0&&(size_t)length<sizeof(path)&&!active_file);
 active_file=__real_fopen(path,mode);if(active_file)read_opens++;
 return active_file;
}
int __wrap_fclose(FILE *file)
{CHECK(file&&file==active_file);active_file=NULL;return __real_fclose(file);}
DIR *__wrap_opendir(const char *name)
{
 CHECK(!strcmp(name,"/dsr")&&!active_directory);
 active_directory=__real_opendir(directory);CHECK(active_directory);cursor=0;
 /* Verify the real files, then enumerate them in a controlled order so the
  * corrupt/OOM alias is ALWAYS attempted before the valid fallback. */
 bool present[32]={false};unsigned found=0;struct dirent *entry;
 while((entry=__real_readdir(active_directory))){
  if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,".."))continue;
  unsigned i;for(i=0;i<file_count;i++)if(!strcmp(entry->d_name,files[i]))break;
  CHECK(i<file_count&&!present[i]);present[i]=true;found++;
 }
 CHECK(found==file_count);return active_directory;
}
struct dirent *__wrap_readdir(DIR *dir)
{
 static struct dirent entry;CHECK(dir==active_directory);
 if(cursor==file_count)return NULL;
 size_t length=strlen(files[cursor]);CHECK(length<sizeof(entry.d_name));
 memset(&entry,0,sizeof(entry));memcpy(entry.d_name,files[cursor++],length+1);return &entry;
}
int __wrap_closedir(DIR *dir)
{CHECK(dir&&dir==active_directory);active_directory=NULL;return __real_closedir(dir);}

static void write_photo(const char *name,bool corrupt,bool extra)
{
 char path[512];int length=snprintf(path,sizeof(path),"%s/%s",directory,name);
 CHECK(length>0&&(size_t)length<sizeof(path)&&file_count<32);
 memcpy(files[file_count++],name,strlen(name)+1);
 FILE *file=__real_fopen(path,"wb");CHECK(file);
 payload[0]^=(unsigned char)corrupt;
 CHECK(fwrite(payload,1,sizeof(payload),file)==sizeof(payload));
 payload[0]^=(unsigned char)corrupt;
 if(extra)CHECK(fputc(0,file)==0);
 CHECK(__real_fclose(file)==0);
}
static void loader(unsigned mode)
{
 CHECK(mode<7);unsigned count=mode==6?16:1;
 for(unsigned i=0;i<count;i++){
  memset(payload,(int)(10+i),sizeof(payload));unsigned char hash[32];
  CHECK(mbedtls_sha256(payload,sizeof(payload),hash,0)==0);
  for(unsigned n=0;n<16;n++)snprintf(ids[i]+n*2,3,"%02x",hash[n]);
  char compact[38],legacy[38];CHECK(dsr_photo_filename(ids[i],compact));
  CHECK(snprintf(legacy,sizeof(legacy),"p%s.rgb",ids[i])==37);
  if(mode==2){write_photo(legacy,true,false);write_photo(compact,false,false);}
  else {write_photo(compact,mode==1,mode==5);write_photo(legacy,false,false);}
 }
 fail_until=mode==3?1:mode==4?2:0;dsr_photos_load();
 if(mode==4){
  CHECK(!live&&!dsr_photo_pixels(ids[0])&&!dsr_photo_id(0));
  CHECK(attempts==2&&read_opens==2&&!active_file&&!active_directory);
  fail_until=0;dsr_photos_load();
 }
 unsigned expected_attempts=mode==6?16:mode==4?3:mode==0?1:2;
 CHECK(attempts==expected_attempts&&read_opens==attempts);
 CHECK(live==count&&!active_file&&!active_directory&&dsr_photo_id(count)==NULL);
 for(unsigned i=0;i<count;i++){
  const void *pixels=dsr_photo_pixels(ids[i]);CHECK(pixels);
  memset(payload,(int)(10+i),sizeof(payload));CHECK(!memcmp(pixels,payload,sizeof(payload)));
  unsigned matches=0;for(unsigned n=0;n<count;n++)if(!strcmp(dsr_photo_id(n),ids[i]))matches++;
  CHECK(matches==1);
 }
 printf("Actual photo loader mode%u: %u checks PASS; %u unique IDs, %u allocation attempts (ordered directory/allocator/SDK MODEL, real files and SHA-256)\n",mode,checks,count,attempts);
}
static void codec(const char *vectors)
{
 FILE *file=__real_fopen(vectors,"r");CHECK(file);unsigned count=0;
 char expected[33],name[38];int fields;
 while((fields=fscanf(file,"%32s %37s",expected,name))==2){
  char id[33],encoded[38],alias[38];bool accepted=dsr_photo_filename_id(name,id);
  if(!strcmp(expected,"-")){CHECK(!accepted);continue;}
  CHECK(accepted&&!strcmp(id,expected));
  CHECK(dsr_photo_filename(id,encoded)&&!strcmp(encoded,name));
  memcpy(alias,id,33);CHECK(dsr_photo_filename(alias,alias)&&!strcmp(alias,name));
  CHECK(dsr_photo_filename_id(alias,alias)&&!strcmp(alias,id));count++;
 }
 CHECK(fields==EOF&&feof(file)&&!ferror(file)&&__real_fclose(file)==0&&count>=1000);
 printf("Actual photo codec: %u checks PASS (1000 deterministic Python-oracle IDs, all alphabet positions, canonical bits, in-place aliases)\n",checks);
}
int main(int argc,char **argv)
{
 CHECK(argc==3);
 if(!strcmp(argv[1],"codec"))codec(argv[2]);
 else {directory=argv[1];loader((unsigned)strtoul(argv[2],NULL,10));}
 return 0;
}
