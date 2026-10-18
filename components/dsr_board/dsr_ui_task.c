#include "dsr_ui_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <stdatomic.h>
#include <stddef.h>

/* IDF task depths are BYTES. This object is deliberately internal BSS,
 * not PSRAM: flash/NVS/OTA may temporarily disable the external cache.
 * Static reservation prevents Wi-Fi/mDNS/BLE from consuming the UI stack. */
#define UI_STACK_BYTES 8192u
static StackType_t dsr_ui_stack[UI_STACK_BYTES / sizeof(StackType_t)];
static StaticTask_t dsr_ui_tcb;
static TaskHandle_t handle;
static bool (*ready)(void);
/* Set before reading the gate and cleared after lv_timer_handler returns.
 * With sequentially consistent atomics, a caller that first closes the gate
 * and then waits for this flag to clear can never overlap LVGL work. */
static atomic_bool in_handler;
_Static_assert(sizeof(dsr_ui_stack) == UI_STACK_BYTES, "UI stack byte budget");

static void ui_task(void *argument)
{
    (void)argument;
    for (;;) {
        /* No input, polling, preview or health confirmation while main is
         * still checking startup, or after a latched critical failure. */
        atomic_store(&in_handler, true);
        if (!ready()) { atomic_store(&in_handler, false); vTaskDelay(pdMS_TO_TICKS(20)); continue; }
        uint32_t delay = lv_timer_handler();
        atomic_store(&in_handler, false);
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
    /* Priority 5, above the network worker (4): parsing large Shelly/BLU
     * responses must not time-slice touch handling and drawing (0.7.52). */
    handle = xTaskCreateStatic(ui_task, "dsr_ui", sizeof(dsr_ui_stack), NULL,
                              5, dsr_ui_stack, &dsr_ui_tcb);
    if (!handle) ready = NULL;
    return handle ? ESP_OK : ESP_ERR_NO_MEM;
}

bool dsr_ui_task_handler_active(void) { return atomic_load(&in_handler); }
void dsr_ui_task_quiesce(void)
{
    /* Caller has already made startup_ready() return false. At most one
     * lv_timer_handler pass (bounded by LVGL timers) can still be running. */
    while (atomic_load(&in_handler)) vTaskDelay(1);
}
bool dsr_ui_task_started_with(bool (*startup_ready)(void)) { return handle && ready == startup_ready; }
