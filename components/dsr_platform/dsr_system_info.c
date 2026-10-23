#include "dsr_system_info.h"
#include "esp_heap_caps.h"
#include "esp_flash.h"

void dsr_system_info_snapshot(dsr_system_info_t *out)
{
    if(!out)return;
    *out=(dsr_system_info_t){0};
    const uint32_t internal=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
    out->internal_free=heap_caps_get_free_size(internal);
    out->internal_total=heap_caps_get_total_size(internal);
    out->internal_largest=heap_caps_get_largest_free_block(internal);
    out->psram_free=heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    out->psram_total=heap_caps_get_total_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    uint32_t bytes=0;
    out->flash_error=esp_flash_get_size(esp_flash_default_chip,&bytes);
    if(out->flash_error==ESP_OK)out->flash_bytes=bytes;
}
