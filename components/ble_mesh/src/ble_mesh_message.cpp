/**
 * @file ble_mesh_message.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-08-01
 *
 * @copyright Copyright (c) 2023
 *
 */
/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include "ble_mesh_message.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"

#include "ble_mesh_example_init.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "user_console.h"
#define TAG "BLE_MESSAGE"

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/

#define REPORT_PREE_FIX "SENSOR_"
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

// {
//     "state": {
//         "reported": {
//             "hubID": "44444",
//             "voltage": 220,
//             "current": 0,
//             "power": 0,
//             "operationalStatus": true,
//             "scan_result": [{
//                     "id": "Sensor_1",
//                     "voltage": 220,
//                     "current": 8.001,
//                     "power": 1000
//                 }, {
//                     "id": "Sensor_2",
//                     "voltage": 220,
//                     "current": 8.001,
//                     "power": 1000
//                 }
//             ]
//         }
// 	}
// }

// {
//     "state": {
//         "reported": {
//             "hubID": "44444",
//             "voltage": 220,
//             "current": 0,
//             "power": 0,
//             "operationalStatus": true,
//             "scan_result": [{
//                     "id": "Sensor_AABBCCDDEE"
//                 }, {
//                     "id": "Sensor_EEFFAABB"
//                 }
//             ]
//         }
// 	}
// }

void ble_mesh_notifi_new_device(esp_ble_mesh_unprov_dev_add_t device)
{
    ESP_LOGD(TAG, "enter %s \n", __func__);

    // std::string message = "send to AWS service with device name is " + std::string(REPORT_PREE_FIX) + mac;

    // return message;
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/