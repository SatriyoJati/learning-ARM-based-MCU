set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/arm-none-eabi-gcc.cmake")

enable_language(C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE VL53L1X_SOURCES 
    "core/*.c"
    "platform/*.c"
)

add_library(vl53l1x STATIC  ${VL53L1X_SOURCES})

target_link_libraries(vl53l1x PRIVATE cpu_flags)


target_include_directories(vl53l1x PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../CMSIS/Device/ST/STM32F1xx/Include   
    ${CMAKE_SOURCE_DIR}/../CMSIS/Core/Include     
    ${CMAKE_SOURCE_DIR}/../CMSIS/Include              
)

target_compile_definitions(vl53l1x PRIVATE STM32F103xB) 
