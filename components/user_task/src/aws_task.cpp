/**
 * @file aws_task.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "aws_task.h"
#include "user_aws_v2.h"
#include "esp_log.h"
#include "ArduinoJson.h"
#include "nvs_header.h"
#include "common.h"
#include "time.h"
#include "user_wifi.h"
#include "aws_mqtt_provisioning.h"
#include "user_mqtt.h"
/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define AWS_IOT "Aws-Iot"

// These values are defined in the menuconfig of the AWS IoT component.
// However, you can override these constants from your own code.
#define AWS_IOT_MQTT_HOST "a4bbrj5ng2men-ats.iot.us-east-1.amazonaws.com" ///< Customer specific MQTT HOST. The same will be used for Thing Shadow
#define AWS_IOT_MQTT_PORT 8883                                            ///< default port for MQTT/S

#define DELAY_AWS_TASK 15 * 1000
#define MQTT_SETTINGS_SEND_MIN_INTERVAL 5
#define QOS1 1
#define QOS0 0
/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;
/**
 *
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

pSubCallBackHandler_t subApplCallBackHandler = NULL;
esp_mqtt_client_handle_t client;
/***********************************************************************************************************************
 * Static Functions
 ***********************************************************************************************************************/
static void iot_subscribe_callback_handler(const char *topic, void *payload, size_t payload_len, void *priv_data);
static bool aws_iot_load_cert(void);
static void aws_iot_task(void *param);
/***********************************************************************************************************************
 * Global Functions
 ***********************************************************************************************************************/

/**
 * @brief
 *
 * @return int
 */
int aws_iot_init(void)
{
    // wait wifi connected
    while (!(user_wifi_get_connected() == E_USER_WIFI_CONNECTED))
    {
        ESP_LOGE(AWS_IOT, "wait wifi connecting ....");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(AWS_IOT, "%s:%d, heap: %d, %d", __func__, __LINE__, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

    aws_iot_load_cert();
    char clientID[20];
    user_sys_get_deviceName((char *)clientID);
    char client_buffer_url[150];

    esp_rmaker_mqtt_conn_params_t conn_params;

    conn_params.server_cert = const_cast<char *>(fleet_prov_cert_auth);
    conn_params.server_cert_len = strlen((fleet_prov_cert_auth));
    ESP_LOGI(AWS_IOT, "server_cert_len = %d", conn_params.server_cert_len);

    conn_params.client_cert = const_cast<char *>(fleet_prov_client_Cert);
    conn_params.client_cert_len = strlen((fleet_prov_client_Cert));
    ESP_LOGI(AWS_IOT, "client_cert_len = %d", conn_params.client_cert_len);

    conn_params.client_key = const_cast<char *>(fleet_prov_client_Pkey);
    conn_params.client_key_len = strlen((fleet_prov_client_Pkey));
    ESP_LOGI(AWS_IOT, "client_key_len = %d", conn_params.client_key_len);

    conn_params.client_id = const_cast<char *>(clientID);
    conn_params.mqtt_host = const_cast<char *>(HostAddress);
    conn_params.mqtt_port = AWS_IOT_MQTT_PORT;

    esp_err_t rc = ESP_OK;

    rc = mqtt_v2_init(&conn_params);

    if (ESP_OK != rc)
    {
        ESP_LOGE(AWS_IOT, "aws_iot_mqtt_init returned error : %d ", rc);
        return rc; // abort();
    }

    ESP_LOGI(AWS_IOT, "Connecting to AWS...");
    rc = mqtt_v2_connect();
    if (ESP_OK != rc)
    {
        ESP_LOGE(AWS_IOT, "Connect to the AWS faild : %d ", rc);
        return rc; // abort();
    }

    ESP_LOGI(AWS_IOT, "%s:%d, heap: %d, %d", __func__, __LINE__, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

    // wait for mqtt connected successfully
    while (!mqtt_v2_get_connection())
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (rc == ESP_OK)
    {
        xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", 3 * 1024, nullptr, 5, nullptr, 1);
    }

    ESP_LOGI(AWS_IOT, "Connecting to AWS done ...");
    return rc;
}

/**
 * @brief
 *
 * @param pubTopic
 * @param pubPayLoad
 * @param payLoadLen
 * @return int
 */
int aws_publish(const char *pubTopic, const char *pubPayLoad, int payLoadLen)
{
    esp_err_t rc;
    int msg_id = 0;
    ESP_LOGI(AWS_IOT, " pubTopic: %s", pubTopic);
    ESP_LOGI(AWS_IOT, " pubPayLoad : %s", pubPayLoad);
    rc = mqtt_v2_publish(pubTopic, pubPayLoad, payLoadLen, QOS1, &msg_id);
    return rc;
}

/**
 * @brief
 *
 * @param subTopic
 * @param size_topic
 * @param pSubCallBackHandler
 * @return int
 */
int aws_subscribe(const char *subTopic, uint16_t size_topic, pSubCallBackHandler_t pSubCallBackHandler)
{
    esp_err_t rc;
    subApplCallBackHandler = pSubCallBackHandler;
    rc = mqtt_v2_subscribe(subTopic, iot_subscribe_callback_handler, QOS1, nullptr);
    if (ESP_OK != rc)
    {
        ESP_LOGE(AWS_IOT, "Error subscribing : %d ", rc);
        return rc;
    }
    ESP_LOGI(AWS_IOT, "Subscribing... Successful");

    return rc;
}

/**
 * @brief
 *
 * @param subTopic
 * @return int
 */
int aws_unSubscribe(const char *subTopic)
{
    esp_err_t rc = ESP_OK;
    return rc;
}

/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

/**
 * @brief
 *
 * @param topic
 * @param payload
 * @param payload_len
 * @param priv_data
 */
static void iot_subscribe_callback_handler(const char *topic, void *payload, size_t payload_len, void *priv_data)
{
    ESP_LOGI(AWS_IOT, "topicName %s", topic);
    ESP_LOGI(AWS_IOT, "payload %s", (char *)payload);
    if (subApplCallBackHandler != 0) // User call back if configured
        subApplCallBackHandler((char *)topic, payload_len, (char *)payload);
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
static bool aws_iot_load_cert(void)
{
    bool ret = false;
    esp_err_t err = ESP_OK;
    size_t len = 0;
    // read CA cert
    ESP_LOGI(AWS_IOT, "reading root ca cert at %s:%s", NAMESPACE_CONFIG, KEY_ROOT_CA_CERT);
    err = Read_Blob(NAMESPACE_CONFIG, KEY_ROOT_CA_CERT, (uint8_t *)&fleet_prov_cert_auth, &len);
    if (ESP_OK != err)
    {
        ESP_LOGE(AWS_IOT, "Unable to find %s file", KEY_ROOT_CA_CERT);
        return false;
    }
    ESP_LOGI(AWS_IOT, "Read CA cert of len %d", len);
    ESP_LOGI(AWS_IOT, "Read CA cert %s", fleet_prov_cert_auth);

    // read pem cert
    ESP_LOGI(AWS_IOT, "reading dev cert at %s:%s", NAMESPACE_CONFIG, KEY_PEM_CERT);
    err = Read_Blob(NAMESPACE_CONFIG, KEY_PEM_CERT, (uint8_t *)&fleet_prov_client_Cert, &len);
    if (ESP_OK != err)
    {
        ESP_LOGE(AWS_IOT, "Unable to find %s file", KEY_PEM_CERT);
        return false;
    }
    ESP_LOGI(AWS_IOT, "Read PEM cert of len %d", len);
    ESP_LOGI(AWS_IOT, "Read PEM cert %s", fleet_prov_client_Cert);
    // read pem key
    ESP_LOGI(AWS_IOT, "reading dev key at %s:%s", NAMESPACE_CONFIG, KEY_PEM_KEY);
    err = Read_Blob(NAMESPACE_CONFIG, KEY_PEM_KEY, (uint8_t *)&fleet_prov_client_Pkey, &len);
    if (ESP_OK != err)
    {
        ESP_LOGE(AWS_IOT, "Unable to find %s file", KEY_PEM_KEY);
        return false;
    }
    ESP_LOGI(AWS_IOT, "Read PEM key of len %d", len);
    ESP_LOGI(AWS_IOT, "Read PEM key %s", fleet_prov_client_Pkey);

    return ret;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool aws_isConnected(void)
{
    return mqtt_v2_get_connection();
}

/**
 * @brief
 *
 * @param param
 */
static void aws_iot_task(void *param)
{
    esp_err_t rc = ESP_OK;
    ESP_LOGW(AWS_IOT, "aws_iot_task start");
    while (1)
    {
        // wifi disconnect
        if (user_wifi_get_connected() != E_USER_WIFI_CONNECTED)
        {
        }
        else if (!mqtt_v2_get_connection())
        {
            ESP_LOGW(AWS_IOT, "auto reconnect start");
        }
        else
        {
            // Max time the yield function will wait for read messages
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/