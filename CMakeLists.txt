cmake_minimum_required(VERSION 3.20)
project(StaRT LANGUAGES C ASM)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "Use -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake")
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Firmware build type" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

add_subdirectory(bsp/stm32/stm32f103c8/GNU)
