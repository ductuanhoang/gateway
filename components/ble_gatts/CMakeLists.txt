# set(pri_req  common bt) 
# file(GLOB_RECURSE SOURCES *.c)
# idf_component_register(SRCS ${SOURCES}
#                     INCLUDE_DIRS .
#                     REQUIRES ${pri_req})


set(pri_req  driver esp_system  bt common) 

set(include_dirs "include")

set (USER_COMMON_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/user_ble_gatt.c
)


idf_component_register(SRCS ${USER_COMMON_SOURCES}
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${pri_req})