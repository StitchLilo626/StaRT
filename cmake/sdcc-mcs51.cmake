set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mcs51)

find_program(CMAKE_C_COMPILER sdcc REQUIRED)
set(CMAKE_C_FLAGS_INIT "-mmcs51 --model-large --stack-auto")
