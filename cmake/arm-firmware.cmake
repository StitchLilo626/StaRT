function(start_configure_arm_firmware target name board cpu linker_script)
    set(cpu_flags -mcpu=${cpu} -mthumb)
    if(cpu STREQUAL "cortex-m4")
        list(APPEND cpu_flags -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
    else()
        list(APPEND cpu_flags -mfloat-abi=soft)
    endif()
    set(output "${CMAKE_BINARY_DIR}/${name}")
    set(checker "${PROJECT_SOURCE_DIR}/scripts/check_firmware.py")
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${name}" SUFFIX .elf
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        LINK_DEPENDS "${linker_script};${PROJECT_SOURCE_DIR}/cmake/cortex-m-sections.ld;${checker}")
    target_compile_options(${target} PRIVATE
        ${cpu_flags}
        -ffunction-sections -fdata-sections -fno-common
        $<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Werror>
        $<$<COMPILE_LANGUAGE:ASM>:-Wa,--fatal-warnings>)
    target_link_options(${target} PRIVATE
        ${cpu_flags}
        "-L${PROJECT_SOURCE_DIR}/cmake" "-T${linker_script}"
        -nostartfiles --specs=nano.specs --specs=nosys.specs
        -Wl,--gc-sections,--fatal-warnings,--no-undefined,-z,noexecstack
        "-Wl,-Map=${output}.map,--cref")
    add_custom_command(TARGET ${target} POST_BUILD
        BYPRODUCTS "${output}.hex" "${output}.bin" "${output}.map"
                   "${CMAKE_BINARY_DIR}/firmware-report.json"
        COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${target}>" "${output}.hex"
        COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${target}>" "${output}.bin"
        COMMAND "${ARM_SIZE}" "$<TARGET_FILE:${target}>"
        COMMAND "${Python3_EXECUTABLE}" "${checker}"
            "$<TARGET_FILE:${target}>" --board "${board}"
            --report "${CMAKE_BINARY_DIR}/firmware-report.json"
        VERBATIM)
endfunction()
