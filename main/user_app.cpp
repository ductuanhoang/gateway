/**
 * @file app.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-03-23
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
#include "user_app.h"

// #include "ble_main.h"

#include "common.h"
#include "user_wifi.h"
#include "user_wifi_scan.h"
#include "nvs_header.h"
#include "aws_mqtt_provisioning.h"
#include "button.h"
#include "user_adc.h"
#include "EmonLib.h"
#include "aws_task.h"
#include "device_manager.h"
#include "user_time.h"
#include "ble_mesh.h"
#include "user_ble_gatt.h"

#include "user_console.h"
#include "ble_mesh_process.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define TAG_MAIN "USER APP"
#define TIME_OUT_SCAN_RESULT 5000
#include "ArduinoJson.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void app_init(void);
    void app_process(void);

#ifdef __cplusplus
}
#endif
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static void ble_receive_command(uint8_t *command, uint16_t len);
static void feedback_command(int status);
static void device_get_system_info(void);
static bool user_get_wifi_info(void);
static bool user_app_save_wifi_info(const char *ssid, const char *password);
static void user_app_button_callback(int num, int event);
static void main_report_task(void *param);
static std::string aws_getPublishTopic(void);
static void aws_subscribeAllTopic(void);
static void userReportDataSensor(void);
static void userControlReportDataSensorTest(void);

static void user_ble_mesh_report_scan_result(void *param);
static void set_default_value_new_sensor(void * param);

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
DeviceManager my_gateway("44444");
UserTime Userlocaltime;
EndDeviceData_t my_end_device_data_1;
EndDeviceData_t my_end_device_data_2;
//
// EndDeviceData_t my_end_device_data[CONFIG_BLE_MESH_MAX_PROV_NODES];

std::vector<EndDeviceData_t> my_end_device_data;

EndDeviceData_t my_hub_data;

EnergyMonitor emon1;
static bool bScan_active = false;
/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

void getDeviceInfo(void)
{
    snprintf(gateway_data.HubId, sizeof(gateway_data.HubId), "44444");
    my_gateway.setDeviceId(gateway_data.HubId);
}
/**
 * @brief
 *
 */
void app_init(void)
{
    // get default
    getDeviceInfo();

    nvs_init(); // init internal flash

    // init ble mesh
    device_get_system_info();
    user_button_init();
    adc_init(); // init adc interface
    user_console_init();
    user_button_callback_init(user_app_button_callback);
    my_gateway.setDeviceId(gateway_data.HubId);

    if (user_get_wifi_info() == false)
    {
        user_wifi_init();
        ESP_LOGI(TAG_MAIN, "Start ble for smartconfig");
        user_ble_gatts_init();
        ble_command_callback_init(ble_receive_command); // set ble config wifi AP
    }
    else
    {
        user_wifi_init();

        user_ble_mesh_init();
        ble_mesh_report_scan_result_register_callback(user_ble_mesh_report_scan_result);
        user_ble_mesh_prov_complete_register(set_default_value_new_sensor);
        ble_mesh_tasks_init();
        // check if device has provision wifi before
        ESP_LOGI(TAG_MAIN, "device has wifi with: ");
        ESP_LOGI(TAG_MAIN, "                wifi name = %s", device_info.wifi.ssid);
        ESP_LOGI(TAG_MAIN, "                wifi pass = %s", device_info.wifi.pass);
        user_wifi_connect_AP((const char *)device_info.wifi.ssid, (const char *)device_info.wifi.pass);

        xTaskCreate(main_report_task, "main_report_task", 4096, NULL, 2, NULL);
        // check aws provisioned
        if (mqtt_provision_task_start())
        {
            ESP_LOGI(TAG_MAIN, "device has provisioned");
            if (aws_iot_init() == 0)
            {
                ESP_LOGI(TAG_MAIN, "Connected to AWS...");
            }
        }
    }

    // wifi_scan_report_callback_init(wifible_report_scanned); // set wifi scan callback
    // user_wifi_scan();
}

static void event_mqtt_message(char *topicName, int payloadLen, char *payLoad)
{
    ESP_LOGI(TAG_MAIN, "event_mqtt_message");
    ESP_LOGI(TAG_MAIN, "topicName = %s", topicName);
    // ESP_LOGI(TAG_MAIN, "payLoad = %s", payLoad);
    EndDeviceData_t device_control;
    device_control = my_gateway.devivePasserMessage(payLoad);

    if (device_control.id_command == 1) // control command
    {
        ESP_LOGI(TAG_MAIN, "device_control name = %s", device_control.id_name);
         ESP_LOGI(TAG_MAIN, "get_num_devices = %d", ble_mesh_provisioned_device_get_num_devices());
        for (size_t i = 0; i < ble_mesh_provisioned_device_get_num_devices(); i++)
        {
            if (strcmp(my_end_device_data.at(i).id_name, device_control.id_name) == 0)
            {
                my_end_device_data.at(i).status = device_control.status;
                ESP_LOGI(TAG_MAIN, "setting status = %d with name :%s", my_end_device_data.at(i).status, my_end_device_data.at(i).id_name);
                uint16_t node_index = ble_mesh_get_uuid_with_name((const char *)my_end_device_data.at(i).id_name);
                if (my_end_device_data.at(i).status == 1)
                {
                    ble_mesh_send_gen_onoff_set(1, node_index);
                    my_end_device_data.at(i).current = 0.00;
                    my_end_device_data.at(i).power = my_end_device_data.at(i).current * my_end_device_data.at(i).voltage;
                }
                else
                {
                    ble_mesh_send_gen_onoff_set(0, node_index);
                    my_end_device_data.at(i).current = 0;
                    my_end_device_data.at(i).power = my_end_device_data.at(i).current * my_end_device_data.at(i).voltage;
                }
            }
        }
    }
    else if (device_control.id_command == 2) // scan command
    {
        // scan command response
        ble_mesh_provisioner_prov_enable(false); // disable provision
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ble_mesh_provisioner_prov_enable(true); // enable ble provisioner
    }
    else if (device_control.id_command == 3) // delete command
    {
        // ble_mesh_provision_delete_with_name(device_control.id_name + strlen("Sensor_"));
        // remove from the vector
    }
    else if (device_control.id_command == 4) // add a new sensor command
    {
        bScan_active = true;
        ble_mesh_provision_device_with_name(device_control.id_name);
        ble_mesh_free_unprov_table();
    }
}

/**
 * @brief
 *
 */
static void user_ble_mesh_report_scan_result(void *param)
{
    ESP_LOGI(TAG_MAIN, "user_ble_mesh_report_scan_result called");
    for (size_t i = 0; i < ble_mesh_provision_device_get_num_devices(); i++)
    {
        my_gateway.deviceReportScanDataPoint((std::string)ble_mesh_get_unprov_device_name(i));
        // ESP_LOGI(TAG_MAIN, "name %s", ble_mesh_get_unprov_device_name(i));
    }

    std::string message = my_gateway.deviceReportScanResult();
    ESP_LOGI(TAG_MAIN, "HUB report message %s", message.c_str());
    // report to the AWS
    ESP_LOGI(TAG_MAIN, "HUB report TOPIC %s", aws_getPublishTopic().c_str());
    aws_publish(aws_getPublishTopic().c_str(), message.c_str(), strlen(message.c_str()));
    my_gateway.freeScanContent();
}

static void get_state_sensor_power_up(void)
{
    // get
}
// #define TEST_SENSOR

/**
 * @brief Set the default value new sensor object
 *
 */
static void set_default_value_new_sensor(void * param)
{
    ESP_LOGI(TAG_MAIN, "set_default_value_new_sensor called");
    EndDeviceData_t end_device_data;
    uint8_t number_device_node = ble_mesh_provisioned_device_get_num_devices();

    end_device_data.current = 0;
    end_device_data.id = 1;
    sprintf(end_device_data.id_name, "%s", ble_mesh_provisioned_device_get_name(number_device_node - 1));
    ESP_LOGI(TAG_MAIN, "add new sensor end_device_data[%d] with name :%s", number_device_node - 1, end_device_data.id_name);
    end_device_data.power = 0;
    end_device_data.voltage = 220;
    end_device_data.status = false;
    // add to vector
    my_end_device_data.push_back(end_device_data);
}

/**
 * @brief Set the default value of sensor node object
 *
 */
static void set_default_value_of_sensor_node(void)
{
#ifdef TEST_SENSOR

    my_end_device_data_1.current = 0;
    my_end_device_data_1.id = 1;
    sprintf(my_end_device_data_1.id_name, "%s", "Sensor_1");
    my_end_device_data_1.power = 0;
    my_end_device_data_1.voltage = 220;
    my_end_device_data_1.status = true;
    my_end_device_data_1.source_power_1 = true;
    my_end_device_data_1.source_power_2 = false;

    my_end_device_data_2.current = 0;
    my_end_device_data_2.id = 1;
    sprintf(my_end_device_data_2.id_name, "%s", "Sensor_2");
    my_end_device_data_2.power = 0;
    my_end_device_data_2.voltage = 220;
    my_end_device_data_2.status = true;
    my_end_device_data_2.source_power_1 = true;
    my_end_device_data_2.source_power_2 = false;

    my_hub_data.current = 0;
    my_hub_data.id = 1;
    sprintf(my_hub_data.id_name, "%s", "Hub");
    my_hub_data.power = 0;
    my_hub_data.voltage = 220;
    my_hub_data.status = true;
    my_hub_data.source_power_1 = true;
    my_hub_data.source_power_2 = false;

#else
    uint8_t number_device_node = ble_mesh_provisioned_device_get_num_devices();
    if (number_device_node == 0)
        return;
    EndDeviceData_t end_device_data;
    for (size_t i = 0; i < number_device_node; i++)
    {
        end_device_data.current = 0;
        end_device_data.id = 1;
        sprintf(end_device_data.id_name, "%s", ble_mesh_provisioned_device_get_name(i));
        ESP_LOGI(TAG_MAIN, "my_end_device_data[%d] with name :%s", i, end_device_data.id_name);
        end_device_data.power = 0;
        end_device_data.voltage = 220;
        end_device_data.status = false;
        // add to vector
        my_end_device_data.push_back(end_device_data);
    }

    my_hub_data.current = 0;
    my_hub_data.id = 1;
    sprintf(my_hub_data.id_name, "%s", "Hub");
    my_hub_data.power = 0;
    my_hub_data.voltage = 220;
    my_hub_data.status = true;
    my_hub_data.source_power_1 = true;
    my_hub_data.source_power_2 = false;
#endif
}

bool first_time_init_aws = false;
static void main_report_task(void *param)
{
    ESP_LOGI(TAG_MAIN, "*******aws main_report_task called");
    ble_mesh_powerup_load_flash_devices();
    set_default_value_of_sensor_node();
    uint8_t _state_test = 0;
    uint32_t current_time = 0;
    while (1)
    {
        if (aws_isConnected())
        {
            if (first_time_init_aws == false)
            {
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                aws_subscribeAllTopic();
                first_time_init_aws = true;
            }
            userReportDataSensor();
        }
        else
        {
            ESP_LOGE(TAG_MAIN, "aws_isConnected = %d", aws_isConnected());
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

static void aws_subscribeAllTopic(void)
{
    std::string TopicSub = my_gateway.deviceSubTopic(my_gateway.getDeviceId());
    // std::string TopicSub = "$aws/things/44444/shadow/name/command/update/accepted"; // just test
    ESP_LOGI(TAG_MAIN, "Sub to topic: %s", TopicSub.c_str());
    aws_subscribe(TopicSub.c_str(), TopicSub.length(), event_mqtt_message);
}

static std::string aws_getPublishTopic(void)
{
    std::string TopicPub = my_gateway.deviceReportTopic(my_gateway.getDeviceId());
    ESP_LOGI(TAG_MAIN, "Pub to topic: %s", TopicPub.c_str());
    return TopicPub;
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
static void device_get_system_info(void)
{
    user_sys_get_mac((char *)device_info.MAC);
    user_sys_get_deviceName((char *)device_info.device_name);

    ESP_LOGI(TAG_MAIN, "device_info.MAC = %s", device_info.MAC);
    ESP_LOGI(TAG_MAIN, "device_info.device_name = %s", device_info.device_name);
}

/**
 * @brief
 *
 * @param command
 */
static void ble_receive_command(uint8_t *command, uint16_t len)
{
    ESP_LOGI(TAG_MAIN, "message = %s", command);
    // {
    //     "wifi_command" : 1,
    //     "wifi_name" : "test123",
    //     "wifi_pass" : "123456789"
    // }
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, command);
    if (doc.containsKey("wifi_command"))
    {
        int wifi_command = doc["wifi_command"];
        if (wifi_command == 2) // scan wifi command
        {
        }
        else if (wifi_command == 1)
        {
            if (doc.containsKey("wifi_name") && doc.containsKey("wifi_pass"))
            {
                const char *ssid = doc["wifi_name"];
                const char *pass = doc["wifi_pass"];

                ESP_LOGI(TAG_MAIN, "command = %d", wifi_command);
                ESP_LOGI(TAG_MAIN, "ssid = %s", ssid);
                ESP_LOGI(TAG_MAIN, "pass = %s", pass);
                if (user_app_save_wifi_info(ssid, pass))
                    ESP_LOGI(TAG_MAIN, "wifi save done ");
                else
                    ESP_LOGE(TAG_MAIN, "wifi save error ");
                user_wifi_connect_AP(ssid, pass);
                feedback_command(user_wifi_get_connected());
            }
            else
                ESP_LOGE(TAG_MAIN, "unknown command");
        }
    }
    else
        ESP_LOGE(TAG_MAIN, "unknown format");
}

/**
 * @brief
 *
 * @param status
 */
static void feedback_command(int status)
{
    DynamicJsonDocument doc(512);
    char buffer[100];
    doc["wifi_command"] = 2;

    if (status == E_USER_WIFI_FAIL)
    {
        doc["status"] = 0;
    }
    else if (status == E_USER_WIFI_CONNECTED)
        doc["status"] = 0;

    serializeJson(doc, &buffer, sizeof(buffer));
    ESP_LOGI(TAG_MAIN, "message control: %s", buffer);

    gatt_report_reponse_command_notify((const char *)&buffer, strlen(buffer));
}

/**
 * @brief get wifi infor from nvs flash
 *
 * @return true
 * @return false
 */
static bool user_get_wifi_info(void)
{
    esp_err_t ret = ESP_OK;
    size_t len = 0;
    ret = Read_Blob(NAMESPACE_CONFIG, WIFI_SSID_KEY, (uint8_t *)&device_info.wifi.ssid, &len);
    if (ret != ESP_OK)
        return false;

    ret = Read_Blob(NAMESPACE_CONFIG, WIFI_PASS_KEY, (uint8_t *)&device_info.wifi.pass, &len);
    if (ret != ESP_OK)
        return false;
    return true;
}

/**
 * @brief save wifi and pass info
 *
 * @param ssid
 * @param password
 * @return true
 * @return false
 */
static bool user_app_save_wifi_info(const char *ssid, const char *password)
{
    esp_err_t ret = ESP_OK;
    ret = Write_Blob(NAMESPACE_CONFIG, WIFI_SSID_KEY, (uint8_t *)ssid, strlen(ssid));
    if (ret != ESP_OK)
        return false;

    ret = Write_Blob(NAMESPACE_CONFIG, WIFI_PASS_KEY, (uint8_t *)password, strlen(password));
    if (ret != ESP_OK)
        return false;
    return true;
}

/**
 * @brief event buttons
 *
 * @param num
 * @param event
 */
static void user_app_button_callback(int num, int event)
{
    ESP_LOGI(TAG_MAIN, "button num %d with event %d", num, event);
    if (event == E_USER_BUTTON_HOLD)
    {
        ESP_LOGI(TAG_MAIN, "erase flash device");
        ble_mesh_erase_settings(true);
        ESP_LOGI(TAG_MAIN, "erase user ble flash device");

        ESP_LOGI(TAG_MAIN, "restart device");
        esp_restart();
    }
}

/**
 * @brief report data from sensors
 *
 */
static void userReportDataSensor(void)
{
    std::string node_name;
    uint8_t number_device_node = 0;
    number_device_node = ble_mesh_provisioned_device_get_num_devices();
#ifdef TEST_SENSOR
    my_gateway.deviceReportDataPoint("Sensor_1", my_end_device_data_1);
    my_gateway.deviceReportDataPoint("Sensor_2", my_end_device_data_2);
    my_gateway.setGateWayData(my_hub_data);
#else
    if (number_device_node != 0)
    {
        // check sensor data
        for (size_t i = 0; i < number_device_node; i++)
        {
            my_gateway.deviceReportDataPoint(ble_mesh_provisioned_device_get_name(i), my_end_device_data.at(i));
        }
    }
    my_hub_data.status = true;
    my_gateway.setGateWayData(my_hub_data);
    ESP_LOGI(TAG_MAIN, "my_hub_data status: %d", my_hub_data.status);
#endif
    std::string message = my_gateway.deviceReportAllDataPoints();
    ESP_LOGI(TAG_MAIN, "HUB report example %s", aws_getPublishTopic().c_str());
    aws_publish(aws_getPublishTopic().c_str(), message.c_str(), strlen(message.c_str()));
    my_gateway.freeDataContent();
}

/**
 * @brief
 *
 */
static void userControlReportDataSensorTest(void)
{
    std::string TopicSub = "$aws/things/44444/shadow/name/command/update";
    char buffer_send[300];
    memset(buffer_send, 0x00, sizeof(buffer_send));
    sprintf(buffer_send, "%s", "{\"state\":{\"desired\":{\"command\":{\"name\":\"OFF\",\"parameter\":{\"hubID\":44444,\"breakermateID\":\"Sensor_1\"}}}},\"metadata\":{\"desired\":{\"command\":{\"name\":{\"timestamp\":1685881405},\"parameter\":{\"hubID\":{\"timestamp\":1685881405},\"breakermateID\":{\"timestamp\":1685881405}}}}},\"version\":1142,\"timestamp\":1685881405}");

    aws_publish(TopicSub.c_str(), buffer_send, strlen(buffer_send));
}

/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/