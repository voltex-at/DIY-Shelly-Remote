/* Real panel capture gate/publisher; HTTP authorization invariance separately tested. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static SemaphoreHandle_t lock=(void *)1;
static bool need_preview,preview_ready,held;
static unsigned private_flags,checks;
static unsigned char pixels[320*480*2],frame[320*480*2],*preview=pixels;
bool dsr_ap_sensitive(void){return (private_flags&1)!=0;}
bool dsr_ble_rpc_sensitive(void){return (private_flags&2)!=0;}
bool dsr_device_settings_sensitive(void){return (private_flags&4)!=0;}
int xSemaphoreTake(SemaphoreHandle_t m,TickType_t t){assert(m==lock&&!t&&!held);held=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t m){assert(m==lock&&held);held=false;return pdTRUE;}
#include "preview.inc"
#define CHECK(x) do{checks++;assert(x);}while(0)
int main(void)
{
 memset(frame,0x5a,sizeof(frame));
 for(private_flags=0;private_flags<8;private_flags++){
  memset(pixels,0,sizeof(pixels));need_preview=preview_ready=true;
  CHECK(dsr_panel_preview_requested()==!private_flags);CHECK(!need_preview);CHECK(preview_ready==!private_flags);
  dsr_panel_preview_publish(frame,sizeof(frame));CHECK(preview_ready==!private_flags);CHECK(pixels[0]==(private_flags?0:0x5a));CHECK(!held);
 }
 private_flags=4;need_preview=true;CHECK(!dsr_panel_preview_requested());private_flags=0;CHECK(!preview_ready&&!dsr_panel_preview_requested());
 dsr_panel_preview_publish(frame,1);CHECK(!preview_ready);lock=NULL;CHECK(!dsr_panel_preview_requested());dsr_panel_preview_publish(frame,sizeof(frame));CHECK(!preview_ready);
 printf("PASS: %u genuine preview privacy checks; AP/B1/editor combinations, cached-frame invalidation and PIN/credential publication blocked; mutex MODEL\n",checks);return 0;
}
