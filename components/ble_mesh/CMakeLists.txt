# set(pri_req  common bt) 
# file(GLOB_RECURSE SOURCES *.c)
# idf_component_register(SRCS ${SOURCES}
#                     INCLUDE_DIRS .
#                     REQUIRES ${pri_req})


set(pri_req  driver esp_system system bt common fatfs console) 

set(include_dirs "include")

set (USER_COMMON_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/ble_mesh.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ble_mesh_example_init.c
    ${CMAKE_CURRENT_LIST_DIR}/src/user_console.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ble_mesh_process.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ble_mesh_message.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/src/storage.cpp
)


idf_component_register(SRCS ${USER_COMMON_SOURCES}
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${pri_req})