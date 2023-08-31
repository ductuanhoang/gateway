

set(pri_req  common user_driver driver log nvs_flash esp_system  bt aws_iot ArduinoJson system ) 

set(include_dirs "include")


set (USER_COMMON_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/EmonLib.cpp
)


idf_component_register(SRCS ${USER_COMMON_SOURCES}
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${pri_req})