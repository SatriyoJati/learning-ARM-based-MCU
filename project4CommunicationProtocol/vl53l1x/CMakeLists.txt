file(GLOB_RECURSE VL53L1X_SOURCES 
    "core/*.c"
    "platform/*.c"
)

add_library(vl53l1x STATIC  ${VL53L1X_SOURCES})

target_include_directories(vl53l1x PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_SOURCE_DIR}
)

