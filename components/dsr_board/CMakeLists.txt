if(DSR_BOARD STREQUAL "lcdwiki-es3c35p")
    idf_component_register(SRCS "dsr_es3c35p.c" INCLUDE_DIRS "include"
        REQUIRES lvgl__lvgl
        PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_driver_ledc
                      esp_driver_i2c esp_lcd esp_timer spi_flash esp_psram
                      espressif__esp_lcd_st77922)
else()
    idf_component_register(SRCS "dsr_board.c" INCLUDE_DIRS "include")
endif()
