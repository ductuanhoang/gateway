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

static void currentCalculator(void);
static void user_ble_mesh_event_data(const esp_ble_mesh_unprov_dev_add_t *data, void *event);
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
DeviceManager my_gateway("44444");
UserTime Userlocaltime;
EndDeviceData_t my_end_device_data_1;
EndDeviceData_t my_end_device_data_2;
EndDeviceData_t my_hub_data;
EndDeviceData_t my_end_device_data_old_data;

EnergyMonitor emon1;
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
    emon1.current(111.1);

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
        ble_mesh_message_register_callback(user_ble_mesh_event_data);
        ble_mesh_tasks_init();
        // check if device has provision wifi before
        ESP_LOGI(TAG_MAIN, "device has wifi with: ");
        ESP_LOGI(TAG_MAIN, "                wifi name = %s", device_info.wifi.ssid);
        ESP_LOGI(TAG_MAIN, "                wifi pass = %s", device_info.wifi.pass);
        user_wifi_connect_AP((const char *)device_info.wifi.ssid, (const char *)device_info.wifi.pass);

        // check aws provisioned
        if (mqtt_provision_task_start())
        {
            ESP_LOGI(TAG_MAIN, "device has provisioned");
            if (aws_iot_init() == 0)
                ESP_LOGI(TAG_MAIN, "Connected to AWS...");
        }
    }

    // wifi_scan_report_callback_init(wifible_report_scanned); // set wifi scan callback
    // user_wifi_scan();
    xTaskCreatePinnedToCore(&main_report_task, "main_report_task", 4096, nullptr, 5, nullptr, 1);
}

static void event_mqtt_message(char *topicName, int payloadLen, char *payLoad)
{
    ESP_LOGI(TAG_MAIN, "event_mqtt_message");
    ESP_LOGI(TAG_MAIN, "topicName = %s", topicName);
    // ESP_LOGI(TAG_MAIN, "payLoad = %s", payLoad);
    EndDeviceData_t endDeviceData_controled;
    endDeviceData_controled = my_gateway.devivePasserMessage(payLoad);

    if (endDeviceData_controled.id_command == 1) // control command
    {
        ESP_LOGI(TAG_MAIN, "endDeviceData_controled name = %s", endDeviceData_controled.id_name);
        ESP_LOGI(TAG_MAIN, "my_end_device_data_1 name = %s", my_end_device_data_1.id_name);
        if (strcmp(my_end_device_data_1.id_name, endDeviceData_controled.id_name) == 0)
        {
            my_end_device_data_1.status = endDeviceData_controled.status;
            ESP_LOGI(TAG_MAIN, "setting status sensor1 = %d", my_end_device_data_1.status);
            if (my_end_device_data_1.status == 1)
            {
                my_end_device_data_1.current = 0.08;
                my_end_device_data_1.power = my_end_device_data_1.current * my_end_device_data_1.voltage;
            }
            else
            {
                my_end_device_data_1.current = 0;
                my_end_device_data_1.power = my_end_device_data_1.current * my_end_device_data_1.voltage;
            }
        }
        else if (strcmp(my_end_device_data_2.id_name, endDeviceData_controled.id_name) == 0)
        {
            my_end_device_data_2.status = endDeviceData_controled.status;
            ESP_LOGI(TAG_MAIN, "setting status sensor1 = %d", my_end_device_data_2.status);
            if (my_end_device_data_2.status == 1)
            {
                my_end_device_data_2.current = 0.08;
                my_end_device_data_2.power = my_end_device_data_2.current * my_end_device_data_2.voltage;
            }
            else
            {
                my_end_device_data_2.current = 0;
                my_end_device_data_2.power = my_end_device_data_2.current * my_end_device_data_2.voltage;
            }
        }
    }
    else if (endDeviceData_controled.id_command == 2) // scan command
    {
        // scan command response
        ble_mesh_provisioner_prov_enable(true); // enable ble provisioner
    }
}

/**
 * @brief
 *
 * @param dev_unprov
 * @param data
 */
static void user_ble_mesh_event_data(const esp_ble_mesh_unprov_dev_add_t *data, void *event)
{
    ESP_LOGI(TAG_MAIN, "callback a new device: %s", bt_hex(data->uuid, 8));
    uint8_t buffer[30]; //
    memset(buffer, 0, sizeof(buffer));
    sprintf((char *)buffer, "Sensor_%s", bt_hex(data->uuid, 8));

    my_gateway.deviceReportScanDataPoint((char *)buffer);

    std::string message = my_gateway.deviceReportScanResult();
    ESP_LOGI(TAG_MAIN, "HUB report scan example %s", message.c_str());
    // report to the AWS
    ESP_LOGI(TAG_MAIN, "HUB report example %s", aws_getPublishTopic().c_str());
    aws_publish(aws_getPublishTopic().c_str(), message.c_str(), strlen(message.c_str()));
    my_gateway.freeDataContent();
}

static void main_report_task(void *param)
{
    while (!aws_isConnected())
    {
        ESP_LOGI(TAG_MAIN, " waiting for connection ...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    if (aws_isConnected())
        aws_subscribeAllTopic();
    ESP_LOGI(TAG_MAIN, "%s:%d, heap: %d, %d", __func__, __LINE__, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

    my_hub_data.current = 0;
    my_hub_data.id = 1;
    sprintf(my_hub_data.id_name, "%s", "Sensor_1");
    my_hub_data.power = 0;
    my_hub_data.voltage = 220;
    my_hub_data.status = true;
    my_hub_data.source_power_1 = true;
    my_hub_data.source_power_2 = false;

    my_end_device_data_1.current = 0;
    my_end_device_data_1.id = 1;
    sprintf(my_end_device_data_1.id_name, "%s", "Sensor_1");
    my_end_device_data_1.power = 0;
    my_end_device_data_1.voltage = 220;
    my_end_device_data_1.status = false;

    my_end_device_data_2.current = 0;
    my_end_device_data_2.id = 1;
    sprintf(my_end_device_data_2.id_name, "%s", "Sensor_2");
    my_end_device_data_2.power = 0;
    my_end_device_data_2.voltage = 220;
    my_end_device_data_2.status = false;

    // uint8_t _state_test = 0;
    while (1)
    {
        if (aws_isConnected())
        {
            userReportDataSensor();
            // userControlReportDataSensorTest();
            currentCalculator();
        }

        // _state_test ^= 1;
        // ESP_LOGI(TAG_MAIN, "state_test = %d", _state_test);
        // user_control_relay(_state_test);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

static void aws_subscribeAllTopic(void)
{
    std::string TopicSub = my_gateway.deviceSubTopic(my_gateway.getDeviceId());
    // std::string TopicSub = "$aws/things/44444/shadow/name/command/update/accepted"; // just test
    ESP_LOGI(TAG_MAIN, "Sub to topic: %s", TopicSub.c_str());
    aws_subscribe(TopicSub.c_str(), event_mqtt_message);
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
    //     "wifi_passs" : "123456789"
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
            if (doc.containsKey("wifi_name") && doc.containsKey("wifi_passs"))
            {
                const char *ssid = doc["wifi_name"];
                const char *pass = doc["wifi_passs"];

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
    // {
    // "wifi_command": 1,
    // "status": 0,
    // }
    DynamicJsonDocument doc(1024);
    char buffer[100];
    doc["wifi_command"] = 2;

    if (status == E_USER_WIFI_FAIL)
    {
        doc["status"] = 0;
    }
    else if (status == E_USER_WIFI_CONNECTED)
        doc["status"] = 1;

    serializeJson(doc, &buffer, sizeof(buffer));
    ESP_LOGI(TAG_MAIN, "message control: %s", buffer);

    // gatt_report_reponse_command_notify((const char *)&buffer, strlen(buffer));
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
}

/**
 * @brief report data from sensors
 *
 */
static void userReportDataSensor(void)
{
    // check sensor data
    my_gateway.deviceReportDataPoint("Sensor_1", my_end_device_data_1);
    my_gateway.deviceReportDataPoint("Sensor_2", my_end_device_data_2);
    my_gateway.setGateWayData(my_hub_data);

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

/**
 * @brief
 *
 */
static void currentCalculator(void)
{
    double Irms = emon1.calcIrms(1480);
    double current1 = (Irms * 1.875 - 0.2 - 4.6) / 3.16;
    if (current1 < 0)
        current1 = 0;
    ESP_LOGI(TAG_MAIN, "Current %f", current1);
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/