#include "aws_task.h"
#include "aws_iot_config.h"
#include "aws_iot_error.h"

#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_mqtt_provisioning.h"
#include "ArduinoJson.h"
#include "nvs_header.h"
#include "common.h"
static const char *AWS_IOT = "Aws-Iot";

#define DELAY_AWS_TASK 15 * 1000

#define MQTT_SETTINGS_SEND_MIN_INTERVAL 5
#define MQTT_SUBSCIBE_QOS QOS1
#define MQTT_PUBLISH_QOS QOS1

static bool awsConnectionStatus = false;

static bool aws_iot_load_cert(void);
static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data);
void aws_iot_task(void *param);
static void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                           IoT_Publish_Message_Params *params, void *pData);
static void connect_awsiot(AWS_IoT_Client *client);

TaskHandle_t handle_awsiot_task;
AWS_IoT_Client client;
IoT_Client_Init_Params mqttInitParams = IoT_Client_Init_Params_initializer;
IoT_Client_Connect_Params connectParams = IoT_Client_Connect_Params_initializer;
IoT_Publish_Message_Params paramsQOS0;
pSubCallBackHandler_t subApplCallBackHandler = NULL;

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;
/**
 *
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

char cPayload[2048] = {0x00};
char receivedPayload[2048] = {0x00};

int aws_iot_init(void)
{
    aws_iot_load_cert();
    char clientID[20];
    user_sys_get_deviceName((char *)clientID);

    int rc = FAILURE;

    ESP_LOGI(AWS_IOT, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;

    mqttInitParams.pRootCALocation = const_cast<char *>(fleet_prov_cert_auth);
    mqttInitParams.pDeviceCertLocation = const_cast<char *>(fleet_prov_client_Cert);
    mqttInitParams.pDevicePrivateKeyLocation = const_cast<char *>(fleet_prov_client_Pkey);

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);

    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_IOT, "aws_iot_mqtt_init returned error : %d ", rc);
        return rc; // abort();
    }

    connectParams.keepAliveIntervalInSec = 60;
    connectParams.isCleanSession = false;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    connectParams.pClientID = const_cast<char *>(clientID);
    connectParams.clientIDLen = (uint16_t)strlen(clientID);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(AWS_IOT, "Connecting to AWS...");
    do
    {
        rc = aws_iot_mqtt_connect(&client, &connectParams);

        if (SUCCESS != rc)
        {
            ESP_LOGE(AWS_IOT, "Error(%d) connecting to %s:%d, \n\rTrying to reconnect", rc, mqttInitParams.pHostURL, mqttInitParams.port);
        }

    } while (SUCCESS != rc);

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    // TODO - bock was commented out - check
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_IOT, "Unable to set Auto Reconnect to true - %d", rc);
        // abort();
    }
    aws_iot_mqtt_autoreconnect_set_status(&client, false);
    const size_t stack_size = 36 * 1024;
    if (rc == SUCCESS)
        xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", stack_size, nullptr, 5, nullptr, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    return rc;
}

int aws_publish(const char *pubTopic, const char *pubPayLoad, int payLoadLen)
{
    IoT_Error_t rc;

    paramsQOS0.qos = QOS1;
    paramsQOS0.payload = const_cast<char *>(pubPayLoad);
    paramsQOS0.isRetained = 0;

    paramsQOS0.payloadLen = payLoadLen;
    rc = aws_iot_mqtt_publish(&client, pubTopic, strlen(pubTopic), &paramsQOS0);

    return rc;
}

int aws_subscribe(const char *subTopic, pSubCallBackHandler_t pSubCallBackHandler)
{
    IoT_Error_t rc;

    subApplCallBackHandler = pSubCallBackHandler;

    ESP_LOGI(AWS_IOT, "Subscribing...");
    rc = aws_iot_mqtt_subscribe(&client, subTopic, strlen(subTopic), QOS1, iot_subscribe_callback_handler, nullptr);
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_IOT, "Error subscribing : %d ", rc);
        return rc;
    }
    ESP_LOGI(AWS_IOT, "Subscribing... Successful");

    return rc;
}

int aws_unSubscribe(const char *subTopic)
{
    IoT_Error_t rc;

    rc = aws_iot_mqtt_unsubscribe(&client, subTopic, strlen(subTopic));
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_IOT, "Error unSubscribing : %d ", rc);
        return rc;
    }
    ESP_LOGI(AWS_IOT, "unSubscribing topic: %s... => Successful", subTopic);

    return rc;
}

int aws_subscribeList(void)
{
    IoT_Error_t rc;

    return 0;
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
static void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                           IoT_Publish_Message_Params *params, void *pData)
{
    ESP_LOGI(AWS_IOT, "topicName %s", topicName);
    ESP_LOGI(AWS_IOT, "payload %s", (char *)params->payload);
    if (subApplCallBackHandler != 0) // User call back if configured
        subApplCallBackHandler(topicName, params->payloadLen, (char *)params->payload);
}

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

void json_test(void)
{
    DynamicJsonDocument doc(1000);
}

/**
 *
 */
static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
    ESP_LOGW(AWS_IOT, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;
    awsConnectionStatus = false;
    if (NULL == pClient)
    {
        return;
    }

    if (aws_iot_is_autoreconnect_enabled(pClient))
    {
        ESP_LOGI(AWS_IOT, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    }
    else
    {
        ESP_LOGW(AWS_IOT, "Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if (NETWORK_RECONNECTED == rc)
        {
            ESP_LOGW(AWS_IOT, "Manual Reconnect Successful");
        }
        else
        {
            ESP_LOGW(AWS_IOT, "Manual Reconnect Failed - %d", rc);
        }
    }
}

bool aws_isConnected(void)
{
    bool isConnected;
    isConnected = aws_iot_mqtt_is_client_connected(&client);
    return isConnected;
}

void aws_iot_task(void *param)
{
    IoT_Error_t rc = SUCCESS;
    ESP_LOGW(AWS_IOT, "aws_iot_task start");
    awsConnectionStatus = true;
    while (1)
    {
        // ESP_LOGE(AWS_IOT, "Wifi Connected, processing AWS connection");
        if (!aws_isConnected())
        {
            ESP_LOGE(AWS_IOT, "Disconnected!!, Attempting reconnection...");
            aws_iot_mqtt_disconnect(&client);
            ESP_LOGE(AWS_IOT, "Connecting");

            connect_awsiot(&client);
            aws_iot_mqtt_resubscribe(&client);
        }

        vTaskDelay(/*1000*/ 550 / portTICK_RATE_MS);
    }
}

static void connect_awsiot(AWS_IoT_Client *client)
{
    IoT_Error_t rc = FAILURE;
    do
    {
        rc = aws_iot_mqtt_connect(client, &connectParams);
        if (SUCCESS != rc)
        {
            ESP_LOGE(AWS_IOT, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(10000 / portTICK_RATE_MS);
        }
    } while (SUCCESS != rc);

    ESP_LOGI(AWS_IOT, "Connected to AWS...");
    rc = aws_iot_mqtt_autoreconnect_set_status(client, true);
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_IOT, "Unable to set Auto Reconnect to true - %d", rc);
    }
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/