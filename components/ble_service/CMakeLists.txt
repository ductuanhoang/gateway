set(pri_req  driver common bt) 


# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
message(CMAKE_CURRENT_LIST_DIR="${CMAKE_CURRENT_LIST_DIR}")


set (USER_BLE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src
)

set(USER_BLE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/include
)

file(GLOB_RECURSE SOURCES *.c)
idf_component_register(SRCS ${USER_BLE_SOURCES}
                    INCLUDE_DIRS ${USER_BLE_DIRS}
                    REQUIRES ${pri_req})