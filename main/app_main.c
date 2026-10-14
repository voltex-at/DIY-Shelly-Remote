#include "dsr_board.h"
#include "dsr_core.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "nvs_flash.h"

void app_main(void)
{
    const char *tag = "dsr";
    ESP_LOGI(tag, "DIY Shelly Remote %s", esp_app_get_description()->version);
    ESP_LOGW(tag, "Diagnostic target only; physical board is unconfirmed");
    esp_err_t error = nvs_flash_init();
    if (error != ESP_OK) {
        /* Never erase user settings automatically on version/space errors. */
        ESP_LOGE(tag, "NVS initialization failed: %s; settings preserved", esp_err_to_name(error));
        return;
    }
    if (dsr_board_init() != ESP_OK) {
        ESP_LOGW(tag, "Hardware profile '%s': LCD/touch/Wi-Fi/BLE not started", dsr_board_hardware_id());
        return;
    }
    /* Production boot sequence is gated on a confirmed board implementation. */
}
