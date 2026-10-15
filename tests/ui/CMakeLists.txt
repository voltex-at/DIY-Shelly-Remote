cmake_minimum_required(VERSION 3.16)
project(dsr_ui_checks C CXX)
if(NOT DEFINED LVGL_SOURCE)
    message(FATAL_ERROR "Pass -DLVGL_SOURCE=<LVGL v9.3.0 checkout>")
endif()
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory("${LVGL_SOURCE}" lvgl-build)
set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
add_executable(test_ui test_ui.c
    ${ROOT}/components/dsr_ui/dsr_ui.c
    ${ROOT}/components/dsr_ui/dsr_ui_network.c
    ${ROOT}/components/dsr_ui/dsr_ui_wifi.c
    ${ROOT}/components/dsr_platform/dsr_wifi_policy.c)
target_include_directories(test_ui PRIVATE . ../support
    ${ROOT}/components/dsr_core/include ${ROOT}/components/dsr_platform/include
    ${ROOT}/components/dsr_board/include ${ROOT}/components/dsr_ui/include
    ${ROOT}/components/dsr_ui)
target_compile_options(test_ui PRIVATE -Wall -Wextra -Werror -Wpedantic)
target_compile_definitions(test_ui PRIVATE DSR_BOARD_ES3C35P=1)
target_link_libraries(test_ui PRIVATE lvgl m)
