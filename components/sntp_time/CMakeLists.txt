set(pri_req common bt mbedtls jsmn aws_provisioning esp_http_client)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
message(CMAKE_CURRENT_LIST_DIR="${CMAKE_CURRENT_LIST_DIR}")

set(include_dirs "include")

set(USER_LOCAL_TIME
    ${CMAKE_CURRENT_LIST_DIR}/src/geoloc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/sntp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/timezone.c
    ${CMAKE_CURRENT_LIST_DIR}/src/user_time.cpp
)



idf_component_register(SRCS ${USER_LOCAL_TIME}
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${pri_req})