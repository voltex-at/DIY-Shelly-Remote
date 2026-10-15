#include "dsr_board.h"
#include "dsr_core.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "nvs_flash.h"
#ifdef DSR_BOARD_ES3C35P
#include "dsr_ui.h"
#include "dsr_platform.h"
#endif

void app_main(void)
{
    const char *tag = "dsr";
    ESP_LOGI(tag, "DIY Shelly Remote %s", esp_app_get_description()->version);
#ifndef DSR_BOARD_ES3C35P
    ESP_LOGW(tag, "Unconfigured diagnostic target only");
#else
    ESP_LOGW(tag, "ES3C35P local switch development build; Shelly validation pending");
#endif
    esp_err_t error = nvs_flash_init();
    if (error != ESP_OK) {
        /* Never erase user settings automatically on version/space errors. */
        ESP_LOGE(tag, "NVS initialization failed: %s; settings preserved", esp_err_to_name(error));
        return;
    }
    error = dsr_board_init();
    if (error != ESP_OK) {
        ESP_LOGW(tag, "Board startup stopped for profile '%s'; network/BLE not started", dsr_board_hardware_id());
        ESP_LOGE(tag, "Board initialization: %s", esp_err_to_name(error));
        return;
    }
#ifdef DSR_BOARD_ES3C35P
    dsr_settings_t settings;
    error = dsr_settings_load(&settings);
    if (error != ESP_OK) ESP_LOGE(tag, "Settings load: %s; Wi-Fi not started", esp_err_to_name(error));
    ESP_ERROR_CHECK(dsr_worker_start());
    error = dsr_wifi_start(error == ESP_OK ? &settings : NULL);
    if (error != ESP_OK) ESP_LOGE(tag, "Wi-Fi startup failed: %s", esp_err_to_name(error));
    dsr_ui_init();
    ESP_ERROR_CHECK(dsr_board_start_ui_task());
    ESP_ERROR_CHECK(dsr_board_set_backlight(70));
    ESP_LOGI(tag, "UI started. Configure Wi-Fi, then add one Shelly switch.");
#endif
}
