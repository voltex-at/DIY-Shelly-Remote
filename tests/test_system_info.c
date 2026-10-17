/* Real snapshot implementation with synthetic read-only SDK providers. */
#include "dsr_system_info.h"
#include "esp_heap_caps.h"
#include "esp_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,free_reads,largest_reads,flash_reads;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static uint32_t flash_size=16u*1024u*1024u;
static esp_err_t flash_error;
static size_t internal_free=147847,internal_largest=73728,psram_free=8239444;
static int chip_token;
esp_flash_t *esp_flash_default_chip=(esp_flash_t *)&chip_token;
size_t heap_caps_get_free_size(uint32_t caps)
{
    free_reads++;
    CHECK(caps==(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)||caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    return caps&(MALLOC_CAP_INTERNAL)?internal_free:psram_free;
}
size_t heap_caps_get_largest_free_block(uint32_t caps)
{largest_reads++;CHECK(caps==(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));return internal_largest;}
esp_err_t esp_flash_get_size(esp_flash_t *chip,uint32_t *out_size)
{flash_reads++;CHECK(chip==esp_flash_default_chip&&out_size);*out_size=flash_size;return flash_error;}
int main(void)
{
    dsr_system_info_snapshot(NULL);CHECK(!free_reads&&!largest_reads&&!flash_reads);
    dsr_system_info_t info;memset(&info,0xa5,sizeof(info));
    dsr_system_info_snapshot(&info);
    CHECK(free_reads==2&&largest_reads==1&&flash_reads==1);
    CHECK(info.internal_free==147847&&info.internal_largest==73728&&info.psram_free==8239444);
    CHECK(info.flash_bytes==16u*1024u*1024u&&info.flash_error==ESP_OK);
    /* No cached counters, hard-coded memory profile or guessed Flash size. */
    internal_free=21;internal_largest=7;psram_free=0;flash_size=8u*1024u*1024u;
    dsr_system_info_snapshot(&info);
    CHECK(info.internal_free==21&&info.internal_largest==7&&!info.psram_free);
    CHECK(info.flash_bytes==8u*1024u*1024u&&info.flash_error==ESP_OK);
    /* Even an SDK that writes its output before failing cannot leak stale
     * or speculative capacity into the UI. Heap reads remain usable. */
    flash_error=ESP_FAIL;dsr_system_info_snapshot(&info);
    CHECK(info.flash_error==ESP_FAIL&&!info.flash_bytes&&info.internal_free==21);
    flash_error=ESP_ERR_INVALID_STATE;dsr_system_info_snapshot(&info);
    CHECK(info.flash_error==ESP_ERR_INVALID_STATE&&!info.flash_bytes);
    flash_error=ESP_OK;flash_size=0;internal_free=internal_largest=0;
    dsr_system_info_snapshot(&info);
    CHECK(!info.internal_free&&!info.internal_largest&&!info.psram_free&&!info.flash_bytes&&info.flash_error==ESP_OK);
    CHECK(free_reads==10&&largest_reads==5&&flash_reads==5);
    printf("PASS: %u system snapshot checks; real capability selection, live counters, Flash availability/error, no NVS/radio/write providers; host SDK fixture only\n",checks);
    return 0;
}
