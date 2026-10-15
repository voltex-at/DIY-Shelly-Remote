if(DSR_BOARD STREQUAL "lcdwiki-es3c35p")
    idf_component_register(SRCS "dsr_ui.c" INCLUDE_DIRS "include"
        REQUIRES dsr_board lvgl__lvgl esp_app_format)
else()
    idf_component_register()
endif()
