idf_component_register(
    SRCS "dsr_http.c" "dsr_settings.c" "dsr_wifi.c" "dsr_worker.c"
    INCLUDE_DIRS "include"
    REQUIRES dsr_core esp_http_client json nvs_flash esp_wifi esp_event
             esp_netif esp_timer freertos
)
