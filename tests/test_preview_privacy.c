/* Real panel capture gate/publisher; HTTP authorization invariance separately tested. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static SemaphoreHandle_t lock=(void *)1;
static bool need_preview,preview_ready,held;
static uint32_t preview_generation,preview_capture_generation;
static int64_t pair_until;
static int64_t esp_timer_get_time(void){return 1;}
static unsigned private_flags,checks;
static unsigned char pixels[320*480*2],frame[320*480*2],*preview=pixels;
bool dsr_ap_sensitive(void){return (private_flags&1)!=0;}
bool dsr_ble_rpc_sensitive(void){return (private_flags&2)!=0;}
bool dsr_device_settings_sensitive(void){return (private_flags&4)!=0;}
bool dsr_maintenance_sensitive(void){return (private_flags&8)!=0;}
int xSemaphoreTake(SemaphoreHandle_t m,TickType_t t){assert(m==lock&&(t==0||t==portMAX_DELAY)&&!held);held=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t m){assert(m==lock&&held);held=false;return pdTRUE;}
#include "preview.inc"
typedef int esp_err_t;
enum {ESP_OK,ESP_FAIL,HTTP_GET};
typedef struct {const char *uri;int method;bool fresh;} httpd_req_t;
static int http_status;static unsigned char http_pixel;
static int httpd_req_get_hdr_value_str(httpd_req_t *r,const char *name,char *out,size_t size)
{assert(!strcmp(name,"X-DSR-Preview")&&size>=6);if(!r->fresh)return ESP_FAIL;strcpy(out,"fresh");return ESP_OK;}
static int httpd_resp_set_type(httpd_req_t *r,const char *type){assert(r&&!strcmp(type,"application/octet-stream"));return ESP_OK;}
static int httpd_resp_set_hdr(httpd_req_t *r,const char *name,const char *value){assert(r&&!strcmp(name,"Cache-Control")&&!strcmp(value,"no-store"));return ESP_OK;}
static int httpd_resp_send(httpd_req_t *r,const char *data,size_t size){assert(r&&size==sizeof(frame));http_status=200;http_pixel=(unsigned char)data[0];return ESP_OK;}
static int json(httpd_req_t *r,const char *status,const char *body){assert(r&&body);http_status=atoi(status);return ESP_OK;}
#include "preview_api.inc"
#define CHECK(x) do{checks++;assert(x);}while(0)
int main(void)
{
 memset(frame,0x5a,sizeof(frame));
 for(private_flags=0;private_flags<16;private_flags++){
  memset(pixels,0,sizeof(pixels));need_preview=preview_ready=true;
  CHECK(dsr_panel_preview_requested()==!private_flags);CHECK(!need_preview);CHECK(preview_ready==!private_flags);
  dsr_panel_preview_publish(frame,sizeof(frame));CHECK(preview_ready==!private_flags);CHECK(pixels[0]==(private_flags?0:0x5a));CHECK(!held);
 }
 private_flags=8;need_preview=true;CHECK(!dsr_panel_preview_requested());private_flags=0;CHECK(!preview_ready&&!dsr_panel_preview_requested());
 /* A newer explicit request invalidates a capture already in progress. */
 preview_generation=1;need_preview=true;CHECK(dsr_panel_preview_requested());CHECK(preview_capture_generation==1);
 preview_generation=2;need_preview=true;preview_ready=false;dsr_panel_preview_publish(frame,sizeof(frame));CHECK(!preview_ready);
 CHECK(dsr_panel_preview_requested());dsr_panel_preview_publish(frame,sizeof(frame));CHECK(preview_ready);
 preview_ready=false;
 httpd_req_t request={.uri="/api/v1/preview.raw",.method=HTTP_GET,.fresh=true};preview_ready=true;preview_generation=10;
 CHECK(preview_get(&request)==ESP_OK&&http_status==202&&!preview_ready&&need_preview&&preview_generation==11);
 CHECK(dsr_panel_preview_requested());CHECK(preview_get(&request)==ESP_OK&&http_status==202&&preview_generation==12);
 dsr_panel_preview_publish(frame,sizeof(frame));CHECK(!preview_ready);CHECK(dsr_panel_preview_requested());frame[0]=0xa5;dsr_panel_preview_publish(frame,sizeof(frame));
 request.fresh=false;CHECK(preview_get(&request)==ESP_OK&&http_status==200&&http_pixel==0xa5&&!need_preview);CHECK(!dsr_panel_preview_requested());
 pair_until=2;request.fresh=true;CHECK(preview_get(&request)==ESP_OK&&http_status==202&&!need_preview&&!preview_ready);pair_until=0;
 dsr_panel_preview_publish(frame,1);CHECK(!preview_ready);lock=NULL;CHECK(!dsr_panel_preview_requested());dsr_panel_preview_publish(frame,sizeof(frame));CHECK(!preview_ready);
 printf("PASS: %u genuine preview privacy checks; AP/B1/editor/maintenance combinations, cached-frame invalidation and physical consent capture blocked; mutex MODEL\n",checks);return 0;
}
