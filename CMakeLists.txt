cmake_minimum_required(VERSION 3.20)
project(StaRT LANGUAGES C)

set(START_KERNEL_SOURCES
    ${PROJECT_SOURCE_DIR}/src/board.c
    ${PROJECT_SOURCE_DIR}/src/ipc.c
    ${PROJECT_SOURCE_DIR}/src/list.c
    ${PROJECT_SOURCE_DIR}/src/scheduler.c
    ${PROJECT_SOURCE_DIR}/src/service.c
    ${PROJECT_SOURCE_DIR}/src/thread.c
    ${PROJECT_SOURCE_DIR}/src/timer.c
)

if(CMAKE_C_COMPILER_ID STREQUAL "SDCC" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "mcs51")
    # SDCC 4.2 emits header dependencies separately from compilation.
    set(CMAKE_C_COMPILE_OBJECT
        "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -M -o <DEP_FILE> <SOURCE>"
        "${CMAKE_C_COMPILE_OBJECT}")
    option(START_STC8H_SIM_TESTS "Build the uCsim core tests" OFF)
    if(START_STC8H_SIM_TESTS)
        enable_testing()
    endif()
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    add_subdirectory(bsp/stc/stc8h8k64u)
    return()
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "Select a firmware preset with cmake --list-presets")
endif()
enable_language(ASM)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Firmware build type" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

set(START_ARM_BOARD stm32f103c8 CACHE STRING "StaRT GNU Arm board")
set_property(CACHE START_ARM_BOARD PROPERTY STRINGS stm32f103c8 nucleo-f439zi)
include(cmake/arm-firmware.cmake)
if(START_ARM_BOARD STREQUAL "stm32f103c8")
    add_subdirectory(bsp/stm32/stm32f103c8/GNU)
elseif(START_ARM_BOARD STREQUAL "nucleo-f439zi")
    add_subdirectory(bsp/stm32/nucleo-f439zi)
else()
    message(FATAL_ERROR "Unknown START_ARM_BOARD: ${START_ARM_BOARD}")
endif()
