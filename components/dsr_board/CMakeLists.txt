set(board_source "dsr_board.c")
if(DSR_BOARD STREQUAL "lcdwiki-es3c35p")
    set(board_source "dsr_es3c35p.c")
endif()
# IDF expands requirements in an isolated early pass. IDF_TARGET is available
# there; arbitrary project cache variables such as DSR_BOARD are not.
if(IDF_TARGET STREQUAL "esp32s3")
    idf_component_register(SRCS ${board_source} INCLUDE_DIRS "include"
        REQUIRES lvgl__lvgl
        PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_driver_ledc
                      esp_driver_i2c esp_lcd esp_timer spi_flash esp_psram
                      espressif__esp_lcd_st77922)
else()
    idf_component_register(SRCS ${board_source} INCLUDE_DIRS "include")
endif()
