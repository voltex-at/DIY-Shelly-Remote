#include "dsr_board.h"
#include "dsr_core.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "nvs_flash.h"
#ifdef DSR_BOARD_ES3C35P
#include "dsr_ui.h"
#include "dsr_platform.h"
#include "dsr_workspace.h"
#include "dsr_discovery.h"
#include "dsr_ble.h"
#include "dsr_boot.h"
#include "dsr_config_health.h"
#include "esp_heap_caps.h"
static void boot_memory(const char *stage)
{
    ESP_LOGI("dsr_boot", "%s internal_free=%u largest=%u psram_free=%u", stage,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
#endif

void app_main(void)
{
#ifdef DSR_BOARD_ES3C35P
    dsr_boot_begin();
#endif
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
#ifdef DSR_BOARD_ES3C35P
        dsr_boot_diagnostic("nvs", error);
#endif
        return;
    }
    error = dsr_board_init();
    if (error != ESP_OK) {
        ESP_LOGW(tag, "Board startup stopped for profile '%s'; network/BLE not started", dsr_board_hardware_id());
        ESP_LOGE(tag, "Board initialization: %s", esp_err_to_name(error));
#ifdef DSR_BOARD_ES3C35P
        dsr_boot_diagnostic("board", error);
#endif
        return;
    }
#ifdef DSR_BOARD_ES3C35P
    boot_memory("board_ready");
    dsr_settings_t settings;
    error = dsr_settings_load(&settings);
    if (error != ESP_OK) ESP_LOGE(tag, "Settings load: %s; Wi-Fi not started", esp_err_to_name(error));
    esp_err_t worker_error = dsr_worker_start();
    if (worker_error != ESP_OK) { dsr_boot_diagnostic("worker", worker_error); return; }
    boot_memory("worker_ready");
    error = dsr_wifi_start(error == ESP_OK ? &settings : NULL);
    if (error != ESP_OK) ESP_LOGE(tag, "Wi-Fi startup failed: %s", esp_err_to_name(error));
    boot_memory("wifi_ready");
    esp_err_t workspace_error=dsr_workspace_start();
    if(workspace_error!=ESP_OK)ESP_LOGE(tag,"Configuration storage unavailable; existing data preserved: %s",esp_err_to_name(workspace_error));
    else if(error==ESP_OK){esp_err_t panel_error=dsr_panel_start();dsr_config_health_panel(panel_error);if(panel_error!=ESP_OK)ESP_LOGE(tag,"Local HTTPS startup failed: %s",esp_err_to_name(panel_error));}
    boot_memory("storage_panel_ready");
    if(dsr_lan_init()!=ESP_OK)ESP_LOGE(tag,"LAN discovery unavailable");
    if(dsr_ble_init()!=ESP_OK)ESP_LOGE(tag,"BLE scanner unavailable");
    boot_memory("discovery_ready");
    dsr_ui_init();
    boot_memory("ui_constructed");
    error = dsr_board_start_ui_task(dsr_boot_ready);
    if (error != ESP_OK) { dsr_boot_diagnostic("ui_task", error); return; }
    error = dsr_board_set_backlight(70);
    if (error != ESP_OK) { dsr_boot_diagnostic("backlight", error); return; }
    if (!dsr_boot_finish()) { dsr_boot_diagnostic("startup_state", ESP_ERR_INVALID_STATE); return; }
    boot_memory("ui_started");
    ESP_LOGI(tag, "UI started. Configure Wi-Fi, then add one Shelly switch.");
#endif
}
