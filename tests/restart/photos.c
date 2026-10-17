/* Actual photo_save/cache + genuine SHA256. File/space providers are MODEL;
 * captured calls never reach a filesystem or device. */
#include "dsr_workspace.h"
#include "dsr_activity_internal.h"
#include "dsr_photo_name.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,fault,infos,opens,writes,flushes,syncs,closes;
static int file_token;static unsigned char pixels[DSR_PHOTO_BYTES];static char id[33];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
int mbedtls_sha256(const unsigned char *data,size_t n,unsigned char out[32],int is224){unsigned length;CHECK(!is224&&EVP_Digest(data,n,out,&length,EVP_sha256(),NULL)==1&&length==32);return 0;}
esp_err_t esp_spiffs_info(const char *label,size_t *total,size_t *used){CHECK(!strcmp(label,"dsrdata")&&!dsr_activity_restart_reserve());infos++;*total=0x3e0000;*used=fault==2?*total:0;return fault==1?ESP_FAIL:ESP_OK;}
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n){CHECK(n==DSR_PHOTO_BYTES&&!dsr_activity_restart_reserve());return fault==8?NULL:__real_malloc(n);}
FILE *__wrap_fopen(const char *path,const char *mode){char decoded[33];CHECK(!strncmp(path,"/dsr/",5)&&strlen(path+4)<32&&dsr_photo_filename_id(path+5,decoded)&&!strcmp(decoded,id)&&!strcmp(mode,"wb")&&!dsr_activity_restart_reserve());opens++;return fault==3?NULL:(FILE *)&file_token;}
size_t __wrap_fwrite(const void *data,size_t size,size_t n,FILE *f){CHECK(f==(FILE *)&file_token&&size==1&&n==DSR_PHOTO_BYTES&&!memcmp(data,pixels,n)&&!dsr_activity_restart_reserve());writes++;return fault==4?n-1:n;}
int __wrap_fflush(FILE *f){CHECK(f==(FILE *)&file_token&&!dsr_activity_restart_reserve());flushes++;return fault==5?EOF:0;}
int __wrap_fileno(FILE *f){CHECK(f==(FILE *)&file_token);return 17;}
int __wrap_fsync(int fd){CHECK(fd==17&&!dsr_activity_restart_reserve());syncs++;return fault==6?-1:0;}
int __wrap_fclose(FILE *f){CHECK(f==(FILE *)&file_token&&!dsr_activity_restart_reserve()&&!dsr_photo_pixels(id));closes++;return fault==7?EOF:0;}
static void admission_open(void){CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void reset(void){admission_open();infos=opens=writes=flushes=syncs=closes=0;}
int main(void)
{
 memset(pixels,0x5a,sizeof(pixels));unsigned char digest[32];mbedtls_sha256(pixels,sizeof(pixels),digest,0);for(unsigned i=0;i<16;i++)sprintf(id+i*2,"%02x",digest[i]);
 for(fault=1;fault<=8;fault++){reset();CHECK(!dsr_photo_save(id,pixels,sizeof(pixels))&&!dsr_photo_pixels(id));CHECK(closes==(opens&&fault!=3?1u:0u));admission_open();}
 reset();fault=0;CHECK(dsr_activity_restart_reserve());CHECK(!dsr_photo_save(id,pixels,sizeof(pixels))&&!infos&&!opens);dsr_activity_restart_cancel();admission_open();
 reset();CHECK(!dsr_photo_save("invalid",pixels,sizeof(pixels))&&!dsr_photo_save(id,pixels,sizeof(pixels)-1)&&!infos&&!opens);admission_open();
 reset();CHECK(dsr_photo_save(id,pixels,sizeof(pixels))&&infos==1&&opens==1&&writes==1&&flushes==1&&syncs==1&&closes==1);CHECK(dsr_photo_pixels(id)&&!memcmp(dsr_photo_pixels(id),pixels,sizeof(pixels))&&!strcmp(dsr_photo_id(0),id));admission_open();
 reset();CHECK(dsr_activity_restart_reserve());CHECK(dsr_photo_save(id,pixels,sizeof(pixels))&&!infos&&!opens);dsr_activity_restart_cancel();admission_open();
 printf("PASS: %u actual photo/cache admission checks; SHA genuine; fopen/write/flush/fsync/close/OOM providers MODEL, zero device writes\n",checks);return 0;
}
