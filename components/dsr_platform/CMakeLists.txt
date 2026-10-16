set(platform_source "dsr_http.c" "dsr_discovery_model.c" "dsr_connection.c" "dsr_fleet.c" "dsr_settings.c" "dsr_wifi.c" "dsr_wifi_policy.c" "dsr_worker.c" "dsr_activity.c")
set(panel_assets "")
if(DSR_BOARD STREQUAL "lcdwiki-es3c35p")
    list(APPEND platform_source "dsr_panel_permission.c" "dsr_system_info.c" "dsr_battery.c" "dsr_workspace.c" "dsr_config_health.c" "dsr_storage.c" "dsr_boot.c" "dsr_panel.c" "dsr_runtime_api.c" "dsr_ota_upload.c" "dsr_photos.c" "dsr_tls.c" "dsr_lan.c" "dsr_ble.c" "dsr_ble_model.c" "dsr_restart.c")
    set(panel_assets "web/panel.html" "web/panel.js" "web/panel.css" "web/runtime.js")
endif()
idf_component_register(
    SRCS ${platform_source}
    INCLUDE_DIRS "include"
    REQUIRES dsr_core dsr_ota esp_http_client json nvs_flash esp_wifi esp_event
             esp_netif esp_adc esp_timer spi_flash heap freertos dsr_config esp_http_server esp_https_server app_update spiffs mbedtls espressif__mdns bt
    EMBED_TXTFILES ${panel_assets}
)
set_source_files_properties(dsr_ota_upload.c dsr_panel.c dsr_activity.c dsr_restart.c dsr_connection.c dsr_fleet.c dsr_worker.c dsr_workspace.c dsr_runtime_api.c PROPERTIES COMPILE_OPTIONS "-fstack-usage")
