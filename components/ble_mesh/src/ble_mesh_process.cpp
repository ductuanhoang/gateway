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
#include "nvs_header.h"

#define TAG "BLE_PROCESS"

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define TIMEOUT_SCAN_INTERVAL 5000      // 5s
#define TIMEOUT_PROVIDER_INTERVAL 10000 // 10s
#define TIME_TASK_WORKING 2000

std::vector<std::tuple<esp_ble_mesh_unprov_dev_add_t, uint32_t>> scan_table_dev_unprov;
std::vector<std::string> node_provisioned_table; // only need storage name
static ble_mesh_report_t report_callback = NULL; // report callback
/**
 * @brief this struct use save name of the sensor node provisioned
 *
 */
typedef struct
{
    uint8_t num_devices;
    std::string name[CONFIG_BLE_MESH_MAX_PROV_NODES];
} node_provisioned_info_t;

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static void ble_mesh_scan_timeout(void *param);
static int provision_device(esp_ble_mesh_unprov_dev_add_t device);
static std::string convertToString(char *a, int size);

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
static node_provisioned_info_t node_provisioned_info;
/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
void ble_mesh_tasks_init(void)
{
    xTaskCreatePinnedToCore(&ble_mesh_scan_timeout, "ble_mesh_scan_timeout", 4096, nullptr, 5, nullptr, 1);
}

void ble_mesh_report_scan_result_register_callback(ble_mesh_report_t callback)
{
    if (callback != NULL)
        report_callback = callback;
}
/**
 * @brief
 *
 * @param node
 */
void ble_mesh_add_proved_devices(const char *node_name)
{
    char key_name[10];
    memset(&key_name, 0, sizeof(key_name));
    if (node_provisioned_table.size() > CONFIG_BLE_MESH_MAX_PROV_NODES)
        return;
    // compare with previous devices name
    for (size_t i = 0; i < node_provisioned_table.size(); i++)
    {
        if (strcmp(node_provisioned_table.at(i).c_str(), node_name) == 0)
        {
            ESP_LOGI(TAG, "Node name: %s has exits", node_name);
            return;
        }
    }

    node_provisioned_table.push_back(node_name);
    // TODO: save this new node to flash memory
    for (size_t i = 0; i < node_provisioned_table.size(); i++)
    {
        sprintf(key_name, "info_%d", i);
        nvs_save_node_config_name(NAMESPACE_BLE_MESH, (const char *)key_name, (char *)node_provisioned_table.at(i).c_str());
        ESP_LOGI(TAG, "Save to flash: %s", key_name);
    }
}

/**
 * @brief
 *
 */
void ble_mesh_powerup_load_flash_devices(void)
{
    // test save and read string values
    int error = -1;
    char key_name[10];
    char value_name[30];
    memset(&key_name, 0, sizeof(key_name));
    std::string message;
    size_t length = 0;
    for (size_t i = 0; i < 10; i++)
    {
        sprintf(key_name, "info_%d", i);
        error = nvs_read_node_config_name(NAMESPACE_BLE_MESH, (const char *)key_name, (char *)value_name, &length);
        if (error == ESP_OK)
        {
            if (length != 0)
                ESP_LOGI(TAG, "Read from flash: %s -- and name is: %s", key_name, value_name);
            node_provisioned_table.push_back(value_name);
        }
    }
}

/**
 * @brief
 *
 * @param name
 */
void ble_mesh_remove_device_from_list(const char *name)
{
    for (size_t i = 0; i < node_provisioned_table.size(); i++)
    {
        if (strcmp(node_provisioned_table.at(i).c_str(), name) == 0)
        {
            node_provisioned_table.erase(node_provisioned_table.begin() + i);
        }
    }
}

/**
 * @brief
 *
 * @param name
 */
void ble_mesh_provision_device_with_name(const char *name)
{
    ESP_LOGI(TAG, "provision_device_with_name: %s", name);

    for (size_t i = 0; i < scan_table_dev_unprov.size(); i++)
    {
        ESP_LOGI(TAG, "compare between %s and %s", ble_mesh_get_unprov_device_name(i), name);
        if (strcmp(ble_mesh_get_unprov_device_name(i), name) == 0)
        {
            ESP_LOGI(TAG, "ble_mesh_provision_device_index[%d]", i);
            ble_mesh_provision_device_index(i);
            return;
        }
    }
}

/**
 * @brief deletes the node sensor with name
 *
 * @param name
 * @return int
 */
int ble_mesh_provision_delete_with_name(const char *name)
{
    esp_err_t result = ESP_OK;
    esp_ble_mesh_device_delete_t del_dev = {
        .flag = BIT(0),
    };

    ble_mesh_remove_device_from_list(name);
    esp_ble_mesh_provisioner_delete_dev(&del_dev);
    return result;
}

/**
 * @brief
 *
 * @param index
 */
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
 * @param index
 * @return const char*
 */
const char *ble_mesh_get_node_name_with_index(uint16_t index)
{
    return esp_ble_mesh_provisioner_get_node_name(index);
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
 */
void ble_mesh_provision_device_show_devices(void)
{
    esp_ble_mesh_node_info_t node;
    uint8_t num_devices = 0;
    num_devices = node_provisioned_table.size();
    if (num_devices == 0)
    {
        ESP_LOGE(TAG, "No provision devices");
    }
    else
    {
        for (size_t i = 0; i < num_devices; i++)
        {
            ESP_LOGI(TAG, "Device name: %s", node_provisioned_table.at(i).c_str());
        }
    }
}
/**
 * @brief
 *
 * @return uint8_t
 */
uint8_t ble_mesh_provisioned_device_get_num_devices(void)
{
    return node_provisioned_table.size();
}

/**
 * @brief
 *
 * @param index
 * @return char*
 */
const char *ble_mesh_provisioned_device_get_name(uint8_t index)
{
    return node_provisioned_table.at(index).c_str();
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

/**
 * @brief
 *
 * @param index
 * @return const char*
 */
const char *ble_mesh_get_unprov_device_name(uint32_t index)
{
    char *unprov_name;
    unprov_name = (char *)calloc(30, sizeof(char));
    if (unprov_name == NULL)
    {
        ESP_LOGI(TAG, "****unprov_name allocation failed");
        exit(1);
    }

    if (index > ble_mesh_provision_device_get_num_devices())
        return NULL;
    esp_ble_mesh_unprov_dev_add_t node = ble_mesh_get_unprov_device_index(index);
    sprintf(unprov_name, "Sensor_%s", bt_hex(node.uuid, 8));
    // ESP_LOGI(TAG, "unprov_name = %s", unprov_name);
    return unprov_name;
}

/**
 * @brief
 *
 */
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
    // ret = ble_mesh_provisioner_prov_enable(true);
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
 * @brief get sensor values from sensor node provisioned by gateway
 *
 */
static void ble_mesh_get_sensors(void *param)
{
    uint16_t number_of_sensors;
    while (1)
    {
        /* code */
        number_of_sensors = node_provisioned_table.size();
        for (size_t i = 0; i < number_of_sensors; i++)
        {
            /* code */
        }

        vTaskDelay(TIME_TASK_WORKING / portTICK_PERIOD_MS);
    }
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
                // call back report scan table
                if (report_callback != NULL)
                    report_callback(NULL);
                //
                // ble_mesh_free_unprov_table();
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