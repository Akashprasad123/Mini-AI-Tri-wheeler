idf_component_register(
    SRCS
        api/core/src/vl53l0x_api.c
        api/core/src/vl53l0x_api_calibration.c
        api/core/src/vl53l0x_api_core.c
        api/core/src/vl53l0x_api_ranging.c
        api/core/src/vl53l0x_api_strings.c
        api/platform/src/vl53l0x_platform.c
        api/platform/src/vl53l0x_platform_log.c
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/api/core/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/api/platform/inc
    REQUIRES driver
)

target_compile_definitions(${COMPONENT_TARGET}
    PRIVATE USE_I2C_2V8=1
)
