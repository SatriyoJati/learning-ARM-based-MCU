cmake_minimum_required(VERSION 3.15.3)

# verbose log message
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/arm-none-eabi-gcc.cmake")

message( "project ${CMAKE_SOURCE_DIR}/arm-none-eabi-gcc.cmake" )

project(PROJECT_ARM)

set(EXECUTABLE ${PROJECT_NAME}.elf)
set(LINKER_FILE ${CMAKE_SOURCE_DIR}/stm32f103.ld)

enable_language(C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "CMAKE_TOOLCHAIN_FILE is: ${CMAKE_TOOLCHAIN_FILE}")

set(INCS
    ../CMSIS/Device/ST/STM32F1xx/Include
    ../CMSIS/Core/Include
)

set(SRC_FILES   
    main.c
    CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
    )

# Build executable
add_executable(${EXECUTABLE} ${SRC_FILES})

target_compile_definitions(${EXECUTABLE} PRIVATE  
    STM32F103xB)

target_include_directories(${EXECUTABLE} PRIVATE ${INCS})

# List of compiler defines, prefix with -D compiler
#target_compile_definitions( ${EXECUTABLE} PRIVATE )


target_compile_options( ${EXECUTABLE} PRIVATE 
    -mcpu=cortex-m3
    -mthumb
    # -mfpu=fpv4-sp-d16
    # -mfloat-abi=hard
    -specs=nano.specs

    -fdata-sections
    -ffunction-sections

    -Wall
    -O0
    -g3
)

target_link_options( ${EXECUTABLE} PRIVATE 
    -T ${LINKER_FILE}
   -mcpu=cortex-m3
   -mthumb
#    -mfpu=fpv4-sp-d16
#    -mfloat-abi=hard
    -specs=nano.specs
    -lc
#    -lm
    -lnosys
   -Wl,-Map=${PROJECT_NAME}.map,--cref
   -Wl,--gc-sections
   -Xlinker -print-memory-usage -Xlinker
)