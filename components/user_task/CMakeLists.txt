# set(pri_req  common bt ArduinoJson) 
# file(GLOB_RECURSE SOURCES *.c)
# idf_component_register(SRCS ${SOURCES}
#                     INCLUDE_DIRS .
#                     REQUIRES ${pri_req})


set(pri_req  common driver log nvs_flash esp_system  bt aws_iot ArduinoJson system ) 

set(include_dirs "include")


set (USER_COMMON_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_task.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/device_manager.cpp
)


idf_component_register(SRCS ${USER_COMMON_SOURCES}
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${pri_req})