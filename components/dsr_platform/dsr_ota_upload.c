#include "dsr_ota_upload.h"
#include "dsr_ota_image.h"
#include <stdlib.h>
#include <string.h>

struct upload_buffers {unsigned char chunk[4096],first[512];};
_Static_assert(sizeof(struct upload_buffers)==4608,"bounded uploader heap workspace");
static int hex_digit(char c)
{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
bool dsr_ota_upload_sha256(const char *text,size_t length,uint8_t out[32])
{
 if(!out)return false;
 memset(out,0,32);
 if(!text||length!=64)return false;
 for(size_t i=0;i<32;i++){
  int high=hex_digit(text[2*i]),low=hex_digit(text[2*i+1]);
  if(high<0||low<0){memset(out,0,32);return false;}
  out[i]=(uint8_t)((high<<4)|low);
 }
 return true;
}
esp_err_t dsr_ota_upload_receive(httpd_req_t *r,const esp_partition_t *slot,const uint8_t expected[32])
{
 if(!r||!slot||!expected||r->content_len<512||r->content_len>slot->size
    ||r->content_len>DSR_OTA_IMAGE_MAX_SIZE)return ESP_ERR_INVALID_ARG;
 struct upload_buffers *buffers=malloc(sizeof(*buffers));
 if(!buffers)return ESP_ERR_NO_MEM;
 dsr_ota_image_t *image=NULL;
 /* Explicit legacy checksum path: never pretend X-SHA256 is verified metadata. */
 if(dsr_ota_image_create_dev_checksum(r->content_len,expected,&image)!=DSR_IMAGE_OK){free(buffers);return ESP_FAIL;}
 size_t received=0,first_size=0;esp_ota_handle_t handle=0;bool live=false;
 esp_err_t error=ESP_OK;
 while(received<r->content_len){
  size_t want=r->content_len-received;if(want>sizeof(buffers->chunk))want=sizeof(buffers->chunk);
  int n=httpd_req_recv(r,(char *)buffers->chunk,want);
  /* Preserve timeout policy; refuse impossible return before inspecting data. */
  if(n<=0||(size_t)n>want){error=ESP_FAIL;break;}
  if(dsr_ota_image_update(image,buffers->chunk,(size_t)n)!=DSR_IMAGE_OK){error=ESP_FAIL;break;}
  received+=(size_t)n;
  if(!live){
   size_t take=sizeof(buffers->first)-first_size;if(take>(size_t)n)take=(size_t)n;
   memcpy(buffers->first+first_size,buffers->chunk,take);first_size+=take;
   if(!dsr_ota_image_header_ready(image))continue;
   error=esp_ota_begin(slot,r->content_len,&handle);if(error!=ESP_OK)break;
   live=true;
   error=esp_ota_write(handle,buffers->first,sizeof(buffers->first));if(error!=ESP_OK)break;
   if((size_t)n>take){error=esp_ota_write(handle,buffers->chunk+take,(size_t)n-take);if(error!=ESP_OK)break;}
  }else{
   error=esp_ota_write(handle,buffers->chunk,(size_t)n);if(error!=ESP_OK)break;
  }
 }
 if(error==ESP_OK&&dsr_ota_image_finish(image)!=DSR_IMAGE_OK)error=ESP_FAIL;
 if(error==ESP_OK){
  error=esp_ota_end(handle);
  /* IDF v5.5.4 invalidates/frees the handle even when end fails. */
  live=false;
 }
 if(live)esp_ota_abort(handle);
 dsr_ota_image_destroy(image);free(buffers);return error;
}
