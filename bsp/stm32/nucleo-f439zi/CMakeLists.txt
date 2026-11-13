add_executable(start_nucleo_f439zi
    main.c
    board.c
    startup_stm32f439zi.S
    ${START_KERNEL_SOURCES}
    ${PROJECT_SOURCE_DIR}/libcpu/arm/CM4F/cpuport.c
    ${PROJECT_SOURCE_DIR}/libcpu/arm/CM4F/arm_gcc.S)
target_include_directories(start_nucleo_f439zi PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libcpu/arm/CM4F")
start_configure_arm_firmware(start_nucleo_f439zi StaRT-nucleo-f439zi nucleo-f439zi
    cortex-m4 "${CMAKE_CURRENT_SOURCE_DIR}/stm32f439zi.ld")
