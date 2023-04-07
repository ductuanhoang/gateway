set(pri_req  driver common bt) 


# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
message(CMAKE_CURRENT_LIST_DIR="${CMAKE_CURRENT_LIST_DIR}")


set(include_dirs "include")


set (USER_BLE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/ble_main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/gatt_svr.c
)


idf_component_register(SRCS ${USER_BLE_SOURCES}
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${pri_req})