/**
 * @file ble_mesh_process.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-07-08
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
#include "ble_mesh_process.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>

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
#include "ble_mesh.h"

#define TAG "BLE_PROCESS"

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define TIMEOUT_SCAN_INTERVAL 10000     // 5s
#define TIMEOUT_PROVIDER_INTERVAL 50000 // 50s
#define TIME_TASK_WORKING 2000

std::vector<std::tuple<esp_ble_mesh_unprov_dev_add_t, uint32_t>> scan_table_dev_unprov;
std::vector<esp_ble_mesh_node_info_t> node_provisioned_table;
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static void ble_mesh_scan_timeout(void *param);
static int provision_device(esp_ble_mesh_unprov_dev_add_t device);
static std::string convertToString(char *a, int size);

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
void ble_mesh_tasks_init(void)
{
    xTaskCreatePinnedToCore(&ble_mesh_scan_timeout, "ble_mesh_scan_timeout", 4096, nullptr, 5, nullptr, 1);
}

/**
 * @brief
 *
 * @param node
 */
void ble_mesh_add_proved_devices(esp_ble_mesh_node_info_t node)
{
    if (node_provisioned_table.size() > CONFIG_BLE_MESH_MAX_PROV_NODES)
        return;
    node_provisioned_table.push_back(node);
}

/**
 * @brief 
 * 
 * @param node_name 
 * @return uint16_t 
 */
uint16_t ble_mesh_get_node_info_with_name(std::string node_name)
{
    uint16_t add = 0;
    std::string node_id;

    node_id = node_name.substr(strlen("SENSOR_"), node_name.length());
    // uint8_t uuid[16];
    // uint16_t unicast;
    for (size_t i = 0; i < node_provisioned_table.size(); i++)
    {
        std::string local_id = convertToString((char *)node_provisioned_table.at(i).uuid, 16);
        if( local_id == node_id)
            add = node_provisioned_table.at(i).unicast;
    }

    return add;
}
void ble_mesh_provision_device_index(uint32_t index)
{
    if (scan_table_dev_unprov.empty() | (index > scan_table_dev_unprov.size()))
    {
        ESP_LOGE(TAG, "Provisioner hasn't this index: %d", index);
        return;
    }

    esp_ble_mesh_unprov_dev_add_t device;
    device = (std::get<0>(scan_table_dev_unprov[index]));
    provision_device(device);
}

/**
 * @brief
 *
 * @return uint8_t
 */
uint8_t ble_mesh_provision_device_get_num_devices(void)
{
    return scan_table_dev_unprov.size();
}
/**
 * @brief
 *
 * @param index
 * @return esp_ble_mesh_unprov_dev_add_t
 */
esp_ble_mesh_unprov_dev_add_t ble_mesh_get_unprov_device_index(uint32_t index)
{
    return (std::get<0>(scan_table_dev_unprov[index]));
}

void ble_mesh_free_unprov_table(void)
{
    scan_table_dev_unprov.clear();
}
/**
 * @brief compare and add a new device mesh to the table
 *
 * @param new_dev_unprov
 * @return int
 */
int ble_mesh_compare_new_prov(esp_ble_mesh_unprov_dev_add_t new_dev_unprov)
{
    int ret = -1;
    uint8_t reject = 0;
    uint8_t number_element = 0;
    number_element = scan_table_dev_unprov.size();

    if (scan_table_dev_unprov.empty())
    {
        // scan_table_dev_unprov.push_back( std::make_tuple(new_dev_unprov, 5000) );
        ESP_LOGI(TAG, "add new device to table: %s", bt_hex(new_dev_unprov.uuid, 8));
        scan_table_dev_unprov.push_back(std::make_tuple(new_dev_unprov, TIMEOUT_SCAN_INTERVAL));
        ret = 0;
        reject = 1;
    }
    else // compare with a new device
    {
        for (size_t i = 0; i < number_element; i++)
        {
            if (strcmp((const char *)(std::get<0>(scan_table_dev_unprov[i])).uuid, (const char *)new_dev_unprov.uuid) == 0)
            {
                // clear timeout
                reject = 1;
                scan_table_dev_unprov[i] = std::make_tuple(new_dev_unprov, TIMEOUT_SCAN_INTERVAL);
            }
        }
    }

    if (reject != 1) // add a new device
    {
        ESP_LOGI(TAG, "add new device to tabble: %s", bt_hex(new_dev_unprov.uuid, 8));
        ret = 0;
        scan_table_dev_unprov.push_back(std::make_tuple(new_dev_unprov, TIMEOUT_SCAN_INTERVAL));
    }

    return ret;
}

/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

/**
 * @brief provision device with device add
 *
 * @param device
 */
static int provision_device(esp_ble_mesh_unprov_dev_add_t device)
{
    int ret = -1;
    /* Note: If unprovisioned device adv packets have not been received, we should not add
             device with ADD_DEV_START_PROV_NOW_FLAG set. */
    ret = esp_ble_mesh_provisioner_add_unprov_dev(&device,
                                                  ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
    if (ret)
    {
        ESP_LOGE(TAG, "%s: Add unprovisioned device into queue failed", __func__);
    }

    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // ESP_LOGD(TAG, "**** provisioning device again ****\n");
    // ret = esp_ble_mesh_provisioner_add_unprov_dev(&device,
    //                                               ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
    // if (ret)
    // {
    //     ESP_LOGE(TAG, "%s: Add unprovisioned device into queue failed", __func__);
    // }

    return ret;
}

/**
 * @brief task scan ble device mesh
 *
 * @param param
 */
uint8_t b_check_empty_device_mesh = 0;

static void ble_mesh_scan_timeout(void *param)
{
    // this task will check and show the time out.
    esp_ble_mesh_unprov_dev_add_t buffer_dev_unprov;
    uint32_t buff_timeout = 0;
    uint32_t ble_prov_timeout = TIMEOUT_PROVIDER_INTERVAL;
    while (1)
    {
        if (!scan_table_dev_unprov.empty())
        {
            b_check_empty_device_mesh = 1;
            // time out clear
            for (size_t i = 0; i < scan_table_dev_unprov.size(); i++)
            {
                buffer_dev_unprov = (std::get<0>(scan_table_dev_unprov[i]));
                buff_timeout = (std::get<1>(scan_table_dev_unprov[i]));
                buff_timeout = buff_timeout - TIME_TASK_WORKING;
                if (buff_timeout == 0)
                {
                    // removed from table
                    scan_table_dev_unprov.erase(scan_table_dev_unprov.begin() + i);
                }
                else
                {
                    // scan_table_dev_unprov.insert(scan_table_dev_unprov.begin() + i, std::make_tuple(buffer_dev_unprov, buff_timeout));
                    scan_table_dev_unprov[i] = std::make_tuple(buffer_dev_unprov, buff_timeout);
                    buff_timeout = (std::get<1>(scan_table_dev_unprov[i]));
                    // ESP_LOGI(TAG, "updated scan_table_dev_unprov ID: %s and timeout is: %d", bt_hex((std::get<0>(scan_table_dev_unprov[i])).uuid, 16), buff_timeout);
                }
            }
        }
        else
        {
            if (b_check_empty_device_mesh == 1)
                ESP_LOGI(TAG, "empty table");
            b_check_empty_device_mesh = 0;
        }
        // ble provisioner timeout
        if (ble_mesh_provisioner_get_prov_enabled() == true)
        {

            ble_prov_timeout -= TIME_TASK_WORKING;
            if (ble_prov_timeout == 0)
            {
                // timeout provider timeout disabled ble provisioner
                ble_mesh_provisioner_prov_enable(0);
                ble_mesh_free_unprov_table();
                ESP_LOGI(TAG, "ble provisioner timeout disabled");
            }
        }
        else
        {
            ble_prov_timeout = TIMEOUT_PROVIDER_INTERVAL;
        }
        // ESP_LOGI(TAG, "ble_mesh_scan_timeout task %d", scan_table_dev_unprov.size());
        vTaskDelay(TIME_TASK_WORKING / portTICK_PERIOD_MS);
    }
}

/**
 * @brief conver char array to string
 * 
 * @param a 
 * @param size 
 * @return std::string 
 */
static std::string convertToString(char *a, int size)
{
    int i;
    std::string s = "";
    for (i = 0; i < size; i++)
    {
        s = s + a[i];
    }
    return s;
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/