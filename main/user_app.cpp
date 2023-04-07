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

#include "ble_main.h"

#include "common.h"
#include "user_wifi.h"
#include "user_wifi_scan.h"
#include "nvs_header.h"
#include "aws_mqtt_provisioning.h"
#include "button.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define TAG_MAIN "USER APP"
#include "ArduinoJson.h"

device_info_t device_info;

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
static void wifible_report_scanned(uint8_t *ssid, int rssid);
static void ble_receive_command(uint8_t *command);
static void feedback_command(int status);
static void device_get_system_info(void);
static bool user_get_wifi_info(void);
static bool user_app_save_wifi_info(const char *ssid, const char *password);
static void user_app_button_callback(int num, int event);
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

/**
 * @brief
 *
 */
void app_init(void)
{
    // get default
    nvs_init(); // init internal flash
    // check if device has provision wifi before
    user_wifi_init();
    device_get_system_info();
    user_button_init();
    user_button_callback_init(user_app_button_callback);
    if (user_get_wifi_info() == false)
    {
        ESP_LOGI(TAG_MAIN, "Start ble for smartconfig");
        ble_main_init();
        ble_command_callback_init(ble_receive_command); // set ble config wifi AP
    }
    else
    {
        ESP_LOGI(TAG_MAIN, "device has wifi with: ");
        ESP_LOGI(TAG_MAIN, "                wifi name = %s", device_info.wifi.ssid);
        ESP_LOGI(TAG_MAIN, "                wifi pass = %s", device_info.wifi.pass);
        user_wifi_connect_AP((const char *)device_info.wifi.ssid, (const char *)device_info.wifi.pass);
    }

    // check aws provisioned
    // if (mqtt_provision_task_start())
    // {
    //     ESP_LOGI(TAG_MAIN, "device has provisioned");
    // }
        // aws_task_init();
    // wifi_scan_report_callback_init(wifible_report_scanned); // set wifi scan callback
    // user_wifi_scan();
}

/**
 * @brief
 *
 */
void wifi_task_init(void)
{
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
 * @param ssid
 * @param rssid
 */
static void wifible_report_scanned(uint8_t *ssid, int rssid)
{
    ESP_LOGI(TAG_MAIN, "wifi: %s -- rssid = %d", ssid, rssid);
}

/**
 * @brief
 *
 * @param command
 */
static void ble_receive_command(uint8_t *command)
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
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/