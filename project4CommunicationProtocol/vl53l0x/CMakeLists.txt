set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/arm-none-eabi-gcc.cmake")

enable_language(C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE VL53L0X_SOURCES 
    "core/src/*.c"
    "platform/src/*.c"
)

# list(REMOVE_ITEM VL53L0X_SOURCES
#     ${CMAKE_CURRENT_SOURCE_DIR}/platform/src/vl53l0x_i2c_platform.c
# )

add_library(vl53l0x STATIC  ${VL53L0X_SOURCES})

target_link_libraries(vl53l0x PRIVATE cpu_flags)


target_include_directories(vl53l0x PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/inc
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../CMSIS/Device/ST/STM32F1xx/Include   
    ${CMAKE_SOURCE_DIR}/../CMSIS/Core/Include     
    ${CMAKE_SOURCE_DIR}/../CMSIS/Include              
)

target_compile_definitions(vl53l0x PRIVATE STM32F103xB  
# VL53L0X_LOG_ENABLE=0
) 
