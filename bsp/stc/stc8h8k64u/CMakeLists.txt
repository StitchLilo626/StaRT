find_program(SDAS8051 sdas8051 REQUIRED)
set(START_STC8H_CLOCK_HZ 45000000 CACHE STRING "STC8H system clock configured in ISP")
if(NOT START_STC8H_CLOCK_HZ MATCHES "^[1-9][0-9]*$" OR
   START_STC8H_CLOCK_HZ LESS 4000000 OR START_STC8H_CLOCK_HZ GREATER 45000000)
    message(FATAL_ERROR "START_STC8H_CLOCK_HZ must be an integer from 4000000 to 45000000")
endif()

set(port "${PROJECT_SOURCE_DIR}/libcpu/MCS51")

function(add_stc8h_image target entry output)
    # SDCC relists its assembly inputs during linking. Use one object per image
    # so simultaneous links never write the same .rst file.
    set(context "${CMAKE_CURRENT_BINARY_DIR}/${target}_context.rel")
    add_custom_command(OUTPUT "${context}"
        BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${target}_context.lst"
        COMMAND "${SDAS8051}" -logff "${context}" "${port}/context.asm"
        DEPENDS "${port}/context.asm"
        VERBATIM)
    set_source_files_properties("${context}" PROPERTIES EXTERNAL_OBJECT TRUE)
    add_executable(${target}
        "${entry}"
        "${port}/cpuport.c"
        ${START_KERNEL_SOURCES}
        "${context}")
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/include")
    target_compile_options(${target} PRIVATE --std-c99 --opt-code-size --Werror)
    # Leave XDATA address zero available as the first-switch sentinel.
    target_link_options(${target} PRIVATE --iram-size 256 --stack-size 192
        --xram-loc 1 --xram-size 8191 --code-size 65536)
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${output}" SUFFIX .hex
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endfunction()

add_stc8h_image(start_stc8h8k64u main.c StaRT-stc8h8k64u)
target_sources(start_stc8h8k64u PRIVATE board.c)
target_compile_definitions(start_stc8h8k64u PRIVATE START_STC8H_CLOCK_HZ=${START_STC8H_CLOCK_HZ}UL)

if(START_STC8H_SIM_TESTS)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    find_program(S51 s51 REQUIRED)
    add_stc8h_image(start_stc8h8k64u_smoke "${PROJECT_SOURCE_DIR}/tests/mcs51/smoke.c" stc8h-smoke)
    add_test(NAME stc8h-core
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/mcs51/check_sim.py"
            "${S51}" "$<TARGET_FILE:start_stc8h8k64u_smoke>"
            "$<TARGET_FILE:start_stc8h8k64u>")
endif()
