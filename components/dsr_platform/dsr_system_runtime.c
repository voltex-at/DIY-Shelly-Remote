#include "dsr_system_info.h"
#include "dsr_ble.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/* Read-only counters; no allocation, NVS, RF or Flash access. */
void dsr_system_runtime(dsr_system_runtime_t *out)
{
    if(!out)return;
    *out=(dsr_system_runtime_t){0};
    out->internal_min_free=heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    out->ui_stack_free=(uint32_t)uxTaskGetStackHighWaterMark(NULL);
    out->ble_active=dsr_ble_host_active();
}
