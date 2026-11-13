set(BSP "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(HAL "${BSP}/Drivers/STM32F1xx_HAL_Driver")
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/stm32f103c8.ld")

set(KERNEL_SOURCES
    ${START_KERNEL_SOURCES}
    ${PROJECT_SOURCE_DIR}/libcpu/arm/CM3/cpuport.c
    ${PROJECT_SOURCE_DIR}/libcpu/arm/CM3/arm_gcc.S
)
# Same HAL translation units as the checked-in Keil project, without templates.
set(HAL_MODULES gpio_ex uart hal rcc rcc_ex gpio dma cortex pwr flash flash_ex exti)
set(HAL_SOURCES)
foreach(module IN LISTS HAL_MODULES)
    if(module STREQUAL "hal")
        list(APPEND HAL_SOURCES "${HAL}/Src/stm32f1xx_hal.c")
    else()
        list(APPEND HAL_SOURCES "${HAL}/Src/stm32f1xx_hal_${module}.c")
    endif()
endforeach()

add_executable(start_stm32f103c8
    ${KERNEL_SOURCES} ${HAL_SOURCES}
    ${BSP}/Core/Src/main.c
    ${BSP}/Core/Src/stm32f1xx_it.c
    ${BSP}/Core/Src/stm32f1xx_hal_msp.c
    ${BSP}/Core/Src/system_stm32f1xx.c
    startup_stm32f103c8.S
)
target_compile_definitions(start_stm32f103c8 PRIVATE USE_HAL_DRIVER STM32F103xB)
target_include_directories(start_stm32f103c8 PRIVATE
    ${PROJECT_SOURCE_DIR}/include ${BSP}/Core/Inc
)
target_include_directories(start_stm32f103c8 SYSTEM PRIVATE
    ${HAL}/Inc ${HAL}/Inc/Legacy
    ${BSP}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
    ${BSP}/Drivers/CMSIS/Include
)
start_configure_arm_firmware(start_stm32f103c8 StaRT-stm32f103c8 stm32f103c8
    cortex-m3 "${LINKER_SCRIPT}")
