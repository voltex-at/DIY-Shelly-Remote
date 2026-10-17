/* Real production consent algorithm, without HTTP/TLS server substitutes. */
#include "dsr_panel_permission.h"
#include "dsr_boot.h"
#include "esp_timer.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned checks,takes,gives,clocks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static bool boot_ready,occupied;
static int token;
static int64_t now=INT64_C(8765432100);
bool dsr_boot_ready(void){return boot_ready;}
int xSemaphoreTake(SemaphoreHandle_t lock,TickType_t wait)
{takes++;CHECK(lock==&token&&wait==0);if(occupied)return pdFALSE;occupied=true;return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t lock)
{gives++;CHECK(lock==&token&&occupied);occupied=false;return pdTRUE;}
int64_t esp_timer_get_time(void){clocks++;CHECK(occupied);return now;}
int main(void)
{
    int64_t deadline=INT64_C(123456789);
    CHECK(!dsr_panel_ota_enable(&token,true,NULL));
    CHECK(!dsr_panel_ota_enable(&token,true,&deadline));
    CHECK(!takes&&!gives&&!clocks&&deadline==INT64_C(123456789));
    boot_ready=true;
    CHECK(!dsr_panel_ota_enable(&token,false,&deadline));
    CHECK(!dsr_panel_ota_enable(NULL,true,&deadline));
    CHECK(!takes&&!gives&&!clocks&&deadline==INT64_C(123456789));
    occupied=true;
    CHECK(!dsr_panel_ota_enable(&token,true,&deadline));
    CHECK(takes==1&&!gives&&!clocks&&occupied&&deadline==INT64_C(123456789));
    occupied=false;
    CHECK(dsr_panel_ota_enable(&token,true,&deadline));
    CHECK(takes==2&&gives==1&&clocks==1&&!occupied);
    CHECK(deadline==now+INT64_C(120000000));
    /* A repeated explicit action renews from the current clock, never adds
     * another two minutes onto the previous deadline. */
    now+=INT64_C(7000000);
    CHECK(dsr_panel_ota_enable(&token,true,&deadline));
    CHECK(deadline==now+INT64_C(120000000)&&takes==3&&gives==2&&clocks==2&&!occupied);
    int64_t accepted=deadline;boot_ready=false;
    CHECK(!dsr_panel_ota_enable(&token,true,&deadline)&&deadline==accepted);
    CHECK(takes==3&&gives==2&&clocks==2);
    printf("PASS: %u panel consent checks; boot/panel readiness, missing/busy lock, unchanged refusal deadline, accepted 120-second physical window and renewal; host SDK fixture only\n",checks);
    return 0;
}
