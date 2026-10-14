cmake_minimum_required(VERSION 3.16)

# The real board profile must be implemented and reviewed before enabling flash.
# This flag is for compilation checks only; it identifies no physical board.
option(DSR_DIAGNOSTIC_BUILD "Build the unconfigured serial diagnostic target" OFF)
if(NOT DSR_DIAGNOSTIC_BUILD)
    message(FATAL_ERROR "No confirmed board profile. See docs/HARDWARE.md. For CI only: -DDSR_DIAGNOSTIC_BUILD=ON")
endif()
set(PROJECT_VER "0.0.1-dev")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(diy_shelly_remote)
