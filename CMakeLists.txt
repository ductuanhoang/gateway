# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# set(EXTRA_COMPONENT_DIRS "components/ble_service"
# "components/common"
# "components/ArduinoJson"
# )
set(EXTRA_COMPONENT_DIRS
    "components/common"
    "components/user_wifi"
    "components/system"
    "components/ble_service"
    "components/user_driver"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(BreakerMater)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/certs/root_cert_auth.pem" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/certs/client.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/certs/client.key" TEXT)