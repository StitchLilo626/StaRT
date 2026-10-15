set(BSP "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(HAL "${BSP}/Drivers/STM32F1xx_HAL_Driver")
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/stm32f103c8.ld")

set(KERNEL_SOURCES
    ${PROJECT_SOURCE_DIR}/src/board.c
    ${PROJECT_SOURCE_DIR}/src/ipc.c
    ${PROJECT_SOURCE_DIR}/src/list.c
    ${PROJECT_SOURCE_DIR}/src/scheduler.c
    ${PROJECT_SOURCE_DIR}/src/service.c
    ${PROJECT_SOURCE_DIR}/src/thread.c
    ${PROJECT_SOURCE_DIR}/src/timer.c
    ${PROJECT_SOURCE_DIR}/libcpu/CM3/cpuport.c
    ${PROJECT_SOURCE_DIR}/libcpu/CM3/arm_gcc.S
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
set_target_properties(start_stm32f103c8 PROPERTIES
    OUTPUT_NAME StaRT-stm32f103c8 SUFFIX .elf
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LINK_DEPENDS "${LINKER_SCRIPT};${PROJECT_SOURCE_DIR}/scripts/check_firmware.py"
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
target_compile_options(start_stm32f103c8 PRIVATE
    -ffunction-sections -fdata-sections -fno-common
    $<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Werror>
    $<$<COMPILE_LANGUAGE:ASM>:-Wa,--fatal-warnings>
)
target_link_options(start_stm32f103c8 PRIVATE
    -T${LINKER_SCRIPT} -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,--gc-sections,--fatal-warnings,--no-undefined,-z,noexecstack
    -Wl,-Map=${CMAKE_BINARY_DIR}/StaRT-stm32f103c8.map,--cref
)
add_custom_command(TARGET start_stm32f103c8 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:start_stm32f103c8>
        ${CMAKE_BINARY_DIR}/StaRT-stm32f103c8.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:start_stm32f103c8>
        ${CMAKE_BINARY_DIR}/StaRT-stm32f103c8.bin
    COMMAND ${ARM_SIZE} $<TARGET_FILE:start_stm32f103c8>
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_firmware.py
        $<TARGET_FILE:start_stm32f103c8>
        --report ${CMAKE_BINARY_DIR}/firmware-report.json
    VERBATIM
)
