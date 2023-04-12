set(pri_req common bt mbedtls jsmn aws_provisioning)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
message(CMAKE_CURRENT_LIST_DIR="${CMAKE_CURRENT_LIST_DIR}")

set(AWS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_jobs_interface.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_jobs_json.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_jobs_topics.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_jobs_types.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_json_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_mqtt_client.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_mqtt_client_common_internal.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_mqtt_client_connect.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_mqtt_client_publish.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_mqtt_client_subscribe.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_mqtt_client_unsubscribe.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_mqtt_client_yield.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_shadow_actions.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_shadow_json.c
    ${CMAKE_CURRENT_LIST_DIR}/src/aws_iot_shadow_records.c
    ${CMAKE_CURRENT_LIST_DIR}/src/network_mbedtls_wrapper.c
    ${CMAKE_CURRENT_LIST_DIR}/src/threads_freertos.c
    ${CMAKE_CURRENT_LIST_DIR}/src/timer.c
)

set(include_dirs "include")

idf_component_register(SRCS ${AWS_SOURCES}
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES ${pri_req})