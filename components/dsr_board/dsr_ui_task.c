#include "dsr_ui_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <stddef.h>

/* IDF task depths are BYTES. This object is deliberately internal BSS,
 * not PSRAM: flash/NVS/OTA may temporarily disable the external cache.
 * Static reservation prevents Wi-Fi/mDNS/BLE from consuming the UI stack. */
#define UI_STACK_BYTES 8192u
static StackType_t dsr_ui_stack[UI_STACK_BYTES / sizeof(StackType_t)];
static StaticTask_t dsr_ui_tcb;
static TaskHandle_t handle;
static bool (*ready)(void);
_Static_assert(sizeof(dsr_ui_stack) == UI_STACK_BYTES, "UI stack byte budget");

static void ui_task(void *argument)
{
    (void)argument;
    for (;;) {
        /* No input, polling, preview or health confirmation while main is
         * still checking startup, or after a latched critical failure. */
        if (!ready()) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }
        uint32_t delay = lv_timer_handler();
        if (delay < 10) delay = 10;
        if (delay > 20) delay = 20;
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

esp_err_t dsr_ui_task_start(bool (*startup_ready)(void))
{
    if (handle) return ESP_ERR_INVALID_STATE;
    if (!startup_ready) return ESP_ERR_INVALID_ARG;
    ready = startup_ready;
    handle = xTaskCreateStatic(ui_task, "dsr_ui", sizeof(dsr_ui_stack), NULL,
                              4, dsr_ui_stack, &dsr_ui_tcb);
    if (!handle) ready = NULL;
    return handle ? ESP_OK : ESP_ERR_NO_MEM;
}
