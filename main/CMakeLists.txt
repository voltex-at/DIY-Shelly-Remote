idf_component_register(SRCS "app_main.c" INCLUDE_DIRS "."
    REQUIRES dsr_core dsr_board dsr_platform nvs_flash esp_app_format)
