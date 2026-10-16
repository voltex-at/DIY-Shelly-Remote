#include "dsr_panel_permission.h"
#include "dsr_boot.h"
#include "esp_timer.h"

bool dsr_panel_ota_enable(SemaphoreHandle_t lock,bool panel_ready,int64_t *until)
{
    if(!until||!dsr_boot_ready()||!panel_ready||!lock||xSemaphoreTake(lock,0)!=pdTRUE)return false;
    *until=esp_timer_get_time()+INT64_C(120000000);
    xSemaphoreGive(lock);
    return true;
}
