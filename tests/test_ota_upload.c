/* Actual receiver + image helper, real OpenSSL SHA; SDK Flash calls captured
 * only in host memory. This fixture performs no device writes or HTTP auth. */
#include "dsr_ota_upload.h"
#include "dsr_ota_image.h"
#include "dsr_activity.h"
#include "dsr_activity_internal.h"
#include "mbedtls/sha256.h"
#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static unsigned checks,allocations,allocation_fail,owned;
static void *pointers[16];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
void *__real_malloc(size_t size);void *__real_calloc(size_t count,size_t size);void __real_free(void *pointer);
static void *track(void *pointer){if(pointer){unsigned i;for(i=0;i<16&&pointers[i];i++);CHECK(i<16);pointers[i]=pointer;owned++;}return pointer;}
void *__wrap_malloc(size_t size){allocations++;return allocation_fail&&allocations==allocation_fail?NULL:track(__real_malloc(size));}
void *__wrap_calloc(size_t count,size_t size){allocations++;return allocation_fail&&allocations==allocation_fail?NULL:track(__real_calloc(count,size));}
void __wrap_free(void *pointer){if(pointer){unsigned i;for(i=0;i<16&&pointers[i]!=pointer;i++);CHECK(i<16&&owned);pointers[i]=NULL;owned--;}__real_free(pointer);}

#define IMAGE_SIZE 8192u
static unsigned char image[IMAGE_SIZE],source[IMAGE_SIZE],written[IMAGE_SIZE];
static const unsigned char marker[]="DSR-OTA:lcdwiki-es3c35p-n16r8:config-api-1";
static size_t read_offset,source_length,write_length,first_read,next_read;
static unsigned receives,begins,writes,ends,aborts,boots,fail_receive,fail_write;
static int receive_error;
static bool fail_begin,fail_end,fail_boot,oversize_receive,live,no_slot;
static esp_partition_t slot={.size=0x600000};static httpd_req_t request={.content_len=IMAGE_SIZE};
static char header[80],response_status[40],response_body[160];
static bool header_error;
static void make_image(size_t marker_at)
{memset(image,0x5a,sizeof(image));memset(image,0,512);image[0]=0xe9;image[1]=1;image[12]=9;image[29]=1;image[32]=0x32;image[33]=0x54;image[34]=0xcd;image[35]=0xab;memcpy(image+80,"diy_shelly_remote",18);if(marker_at+sizeof(marker)<=sizeof(image))memcpy(image+marker_at,marker,sizeof(marker));}
static void digest(const unsigned char *bytes,size_t length,uint8_t out[32])
{unsigned n=0;CHECK(EVP_Digest(bytes,length,out,&n,EVP_sha256(),NULL)==1&&n==32);}
static void reset(void)
{CHECK(!owned&&!dsr_test_sha_live()&&!live);allocations=allocation_fail=0;read_offset=write_length=0;source_length=IMAGE_SIZE;first_read=next_read=4096;receives=begins=writes=ends=aborts=boots=fail_receive=fail_write=0;receive_error=HTTPD_SOCK_ERR_TIMEOUT;fail_begin=fail_end=fail_boot=oversize_receive=no_slot=header_error=false;slot.size=0x600000;request.content_len=IMAGE_SIZE;response_status[0]=response_body[0]=0;dsr_test_sha_fault(0);memcpy(source,image,sizeof(image));}
int httpd_req_recv(httpd_req_t *r,char *buffer,size_t length)
{CHECK(!dsr_activity_restart_reserve());CHECK(r==&request&&length>0&&length<=4096);receives++;if(receives==fail_receive)return receive_error;if(oversize_receive)return (int)length+1;size_t n=receives==1?first_read:next_read;if(n>length)n=length;if(n>source_length-read_offset)n=source_length-read_offset;memcpy(buffer,source+read_offset,n);read_offset+=n;return (int)n;}
esp_err_t esp_ota_begin(const esp_partition_t *p,size_t size,esp_ota_handle_t *handle)
{CHECK(!dsr_activity_restart_reserve());CHECK(p==&slot&&size==request.content_len&&read_offset>=512&&!live&&!begins);begins++;*handle=17;if(fail_begin)return ESP_FAIL;live=true;return ESP_OK;}
esp_err_t esp_ota_write(esp_ota_handle_t handle,const void *data,size_t length)
{CHECK(!dsr_activity_restart_reserve());CHECK(handle==17&&live&&length&&write_length+length<=sizeof(written));writes++;if(writes==fail_write)return ESP_FAIL;memcpy(written+write_length,data,length);write_length+=length;return ESP_OK;}
esp_err_t esp_ota_end(esp_ota_handle_t handle)
{CHECK(!dsr_activity_restart_reserve());CHECK(handle==17&&live&&write_length==request.content_len&&!ends&&!aborts);ends++;live=false;return fail_end?ESP_FAIL:ESP_OK;}
esp_err_t esp_ota_abort(esp_ota_handle_t handle)
{CHECK(!dsr_activity_restart_reserve());CHECK(handle==17&&live&&!ends&&!aborts);aborts++;live=false;return ESP_OK;}
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start)
{CHECK(!start);return no_slot?NULL:&slot;}
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *p)
{CHECK(!dsr_activity_restart_reserve());CHECK(p==&slot&&ends==1&&!aborts&&!live);boots++;return fail_boot?ESP_FAIL:ESP_OK;}
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *r,const char *name,char *out,size_t capacity)
{CHECK(r==&request&&!strcmp(name,"X-SHA256"));if(header_error||strlen(header)>=capacity)return ESP_FAIL;strcpy(out,header);return ESP_OK;}

/* Compile the actual endpoint body extracted by the host tool. Its existing
 * surrounding cookie/origin/boot dispatcher is deliberately outside this test. */
typedef void *SemaphoreHandle_t;
static int lock_token;static SemaphoreHandle_t lock=&lock_token;
static int64_t now=1000000,ota_until;
#define portMAX_DELAY UINT32_MAX
static int xSemaphoreTake(SemaphoreHandle_t semaphore,unsigned delay){CHECK(semaphore==lock&&delay==portMAX_DELAY);return 1;}
static int xSemaphoreGive(SemaphoreHandle_t semaphore){CHECK(semaphore==lock);return 1;}
static int64_t esp_timer_get_time(void){return now;}
static esp_err_t json(httpd_req_t *r,const char *status,const char *body)
{CHECK(r==&request);snprintf(response_status,sizeof(response_status),"%s",status);snprintf(response_body,sizeof(response_body),"%s",body);return ESP_OK;}
#include "ota_endpoint.inc"

static void clean(void){CHECK(!owned&&!dsr_test_sha_live()&&!live);CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();}
static void accepted(const uint8_t hash[32])
{CHECK(dsr_ota_upload_receive(&request,&slot,hash)==ESP_OK);CHECK(read_offset==request.content_len&&write_length==request.content_len&&!memcmp(written,source,write_length));CHECK(begins==1&&ends==1&&!aborts&&!boots);CHECK(dsr_test_sha_calls()==receives+3);clean();}
int main(void)
{
 uint8_t hash[32],parsed[32];make_image(100);digest(image,sizeof(image),hash);
 for(unsigned split=1;split<=4096;split++){reset();first_read=split;next_read=IMAGE_SIZE;accepted(hash);}
 const size_t chunks[]={1,2,16,41,42,43,44,127,128,511,512,513,4096};
 for(unsigned i=0;i<sizeof(chunks)/sizeof(chunks[0]);i++){reset();first_read=next_read=chunks[i];accepted(hash);}
 make_image(600);digest(image,sizeof(image),hash);reset();first_read=512;next_read=1;accepted(hash);
 reset();first_read=next_read=1;accepted(hash);
 /* Exact marker including NUL is required, even with a correct checksum. */
 reset();source[642]=1;digest(source,sizeof(source),parsed);CHECK(dsr_ota_upload_receive(&request,&slot,parsed)!=ESP_OK);CHECK(begins==1&&aborts==1&&!ends&&!boots);clean();
 reset();source[900]^=1;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(aborts==1&&!ends&&!boots);clean();
 const size_t faults[]={0,1,12,13,29,32,33,34,35,80,97};
 for(unsigned i=0;i<sizeof(faults)/sizeof(faults[0]);i++){reset();source[faults[i]]=faults[i]==1?0:255;digest(source,sizeof(source),parsed);CHECK(dsr_ota_upload_receive(&request,&slot,parsed)!=ESP_OK);CHECK(!begins&&!writes&&!ends&&!aborts);clean();}
 for(unsigned n=1;n<=2;n++){reset();allocation_fail=n;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(!receives&&!begins&&!aborts);clean();}
 for(unsigned n=1;n<=4;n++){reset();dsr_test_sha_fault(n);CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(!ends&&!boots);CHECK(aborts==(begins?1u:0u));clean();}
 /* SHA finish follows two chunk updates for this 8192-byte fixture. */
 reset();dsr_test_sha_fault(5);CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(begins==1&&aborts==1&&!ends);clean();
 for(unsigned n=1;n<=2;n++)for(unsigned error=0;error<2;error++){reset();fail_receive=n;receive_error=error?HTTPD_SOCK_ERR_TIMEOUT:0;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(!ends&&!boots&&aborts==(begins?1u:0u));clean();}
 reset();source_length=511;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(!begins&&!aborts);clean();
 reset();source_length=8191;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(begins==1&&aborts==1&&!ends);clean();
 reset();oversize_receive=true;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(!begins&&!aborts);clean();
 reset();fail_begin=true;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(begins==1&&!writes&&!aborts&&!ends);clean();
 for(unsigned n=1;n<=3;n++){reset();fail_write=n;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(begins==1&&aborts==1&&!ends);clean();}
 reset();fail_end=true;CHECK(dsr_ota_upload_receive(&request,&slot,hash)!=ESP_OK);CHECK(ends==1&&!aborts);clean();
 reset();CHECK(dsr_ota_upload_receive(NULL,&slot,hash)==ESP_ERR_INVALID_ARG);CHECK(dsr_ota_upload_receive(&request,NULL,hash)==ESP_ERR_INVALID_ARG);CHECK(dsr_ota_upload_receive(&request,&slot,NULL)==ESP_ERR_INVALID_ARG);
 request.content_len=511;CHECK(dsr_ota_upload_receive(&request,&slot,hash)==ESP_ERR_INVALID_ARG);request.content_len=0x600001;CHECK(dsr_ota_upload_receive(&request,&slot,hash)==ESP_ERR_INVALID_ARG);request.content_len=8192;slot.size=8191;CHECK(dsr_ota_upload_receive(&request,&slot,hash)==ESP_ERR_INVALID_ARG);CHECK(!receives&&!begins);clean();
 /* Header parser: failure always clears all output bytes, including late hex. */
 for(unsigned i=0;i<32;i++)sprintf(header+i*2,"%02x",hash[i]);
 CHECK(dsr_ota_upload_sha256(header,64,parsed)&&!memcmp(parsed,hash,32));
 for(unsigned i=0;i<64;i++){char saved=header[i];header[i]='G';memset(parsed,0xa5,32);CHECK(!dsr_ota_upload_sha256(header,64,parsed));CHECK(!memcmp(parsed,(uint8_t[32]){0},32));header[i]=saved;}
 CHECK(!dsr_ota_upload_sha256(NULL,64,parsed)&&!dsr_ota_upload_sha256(header,63,parsed)&&!dsr_ota_upload_sha256(header,65,parsed)&&!dsr_ota_upload_sha256(header,64,NULL));
 char saved=header[63];header[63]=0;CHECK(!dsr_ota_upload_sha256(header,64,parsed)&&!memcmp(parsed,(uint8_t[32]){0},32));header[63]=saved;
 reset();ota_until=0;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"403 Forbidden"));CHECK(!receives&&!begins&&!boots);
 reset();ota_until=now;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"403 Forbidden"));
 reset();ota_until=now+120000000;header_error=true;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"400 Bad Request")&&!ota_until);CHECK(!begins);header_error=false;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"403 Forbidden"));
 reset();ota_until=now+120000000;memset(header,'A',64);header[64]=0;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"400 Bad Request")&&!begins&&!receives);
 for(unsigned i=0;i<32;i++)sprintf(header+i*2,"%02x",hash[i]);
 reset();ota_until=now+120000000;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"200 OK")&&strstr(response_body,"restart_required"));CHECK(boots==1&&ends==1&&!aborts&&!ota_until);clean();CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"403 Forbidden")&&boots==1);
 reset();ota_until=now+120000000;fail_end=true;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"400 Bad Request")&&!boots&&!aborts);clean();
 reset();ota_until=now+120000000;fail_boot=true;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"500 Internal Server Error")&&boots==1);clean();
 reset();ota_until=now+120000000;no_slot=true;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"400 Bad Request")&&!begins);clean();
 reset();ota_until=now+120000000;request.content_len=511;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"400 Bad Request")&&!begins&&!receives&&!ota_until);clean();
 reset();ota_until=now+120000000;slot.size=8191;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"400 Bad Request")&&!begins&&!receives&&!ota_until);clean();
 reset();CHECK(dsr_activity_restart_reserve());CHECK(dsr_ota_upload_receive(&request,&slot,hash)==ESP_ERR_INVALID_STATE&&!receives&&!begins);dsr_activity_restart_cancel();clean();
 reset();CHECK(dsr_activity_restart_reserve());ota_until=now+120000000;CHECK(ota(&request)==ESP_OK&&!strcmp(response_status,"409 Conflict")&&!receives&&!begins&&!boots&&!ota_until);dsr_activity_restart_cancel();clean();
 printf("PASS: %u actual legacy upload/endpoint checks; every first-read boundary, exact captured bytes, real SHA/header/marker, pre-begin rejection, recv/write/end/abort/OOM faults, one-shot approval, atomic lease incl bootslot; NO device Flash/auth/rollback proof\n",checks);return 0;
}
