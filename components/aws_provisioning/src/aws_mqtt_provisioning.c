/**
 * @file user_mqtt.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-07-28
 *
 * @copyright Copyright (c) 2022
 *
 */

/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/
#include "common.h"
#include "aws_mqtt_provisioning.h"
#include "user_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "nvs_header.h"
#include "config.h"
#include "aws_mqtt_config.h"
#include "esp_mac.h"
#include "config.h"
#include "user_wifi.h"
// #include "esp_http_client.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define TAG_PROVISION "MQTT PROVISIONING"

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
char fleet_prov_certificate_Id[100];
char fleet_prov_cert_Owner[600];
char fleet_prov_thing_Name[20];
char fleet_prov_thing_pass[50];
char fleet_prov_thing_type[50];

static e_provisioning_state provisioning_state = E_PROVISION_STATE_IDLE;
/***********************************************************************************************************************
 * Exported global variables
 ***********************************************************************************************************************/
extern const char root_cert_auth_pem_start[] asm("_binary_root_cert_auth_pem_start");
extern const char root_cert_auth_pem_end[] asm("_binary_root_cert_auth_pem_end");

extern const char client_cert_pem_start[] asm("_binary_client_crt_start");
extern const char client_cert_pem_end[] asm("_binary_client_crt_end");

extern const char client_key_pem_start[] asm("_binary_client_key_start");
extern const char client_key_pem_end[] asm("_binary_client_key_end");

char fleet_prov_cert_auth[2000] = {""};

char fleet_prov_client_Cert[2000] = {""};

char fleet_prov_client_Pkey[2000] = {""};

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
void create_accepted_callback(const char *topic, const char *data, int data_len);
void create_rejected_callback(const char *topic, const char *data, int data_len);
void register_accepted_callback(const char *topic, const char *data, int data_len);
void register_rejected_callback(const char *topic, const char *data, int data_len);
static int user_mqtt_provision_register(void);
static void user_mqtt_provision_set_state(e_provisioning_state state);
static void mqtt_provisioning_task(void *pvParameters);
static bool user_provision_save_cert(void);

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

/**
 * @brief init the mqtt provisioning
 *
 */
void mqtt_provisioning_init(void)
{
    uint8_t time_try_to_connect = 0;
    ESP_LOGI(TAG_PROVISION, "MQTT provisioning init");
    user_mqtt_client_t mqtt_client;
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    sprintf(mqtt_client.mqtt_host_address, "%s", AWS_IOT_ENDPOINT);
    sprintf(mqtt_client.mqtt_client_id, "%02x%02x%02x%02x%02x%02x", eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    mqtt_client.Port = AWS_IOT_PORT;
    int ret = user_mqtt_init(&mqtt_client,
                             client_cert_pem_start,
                             client_key_pem_start,
                             root_cert_auth_pem_start);
    if (ret == -1)
    {
        ESP_LOGE(TAG_PROVISION, "Mqtt init failed");
    }
    else
    {
        ESP_LOGI(TAG_PROVISION, "Mqtt init success");
    }
    bool mqtt_connecttion = false;
    mqtt_connecttion = user_mqtt_getConnetion();
    while ((mqtt_connecttion == false) && (time_try_to_connect < 10))
    {
        mqtt_connecttion = user_mqtt_getConnetion();
        time_try_to_connect++;
        vTaskDelay(2000 / portTICK_PERIOD_MS); // 20 second
    }
    ESP_LOGI(TAG_PROVISION, "Mqtt connected = %d", mqtt_connecttion);
}

/**
 * @brief mqtt subscribe to the topic
 *
 * @return int
 */
int mqtt_provision_subscribe(void)
{
    int ret = -1;
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ret = user_mqtt_client_subscribe(PROV_TOPIC_SUB_ACCEPTED, create_accepted_callback);
    if (ret == -1)
    {
        ESP_LOGE(TAG_PROVISION, "Mqtt subscribe accepted failed");
        return ret;
    }
    ret = user_mqtt_client_subscribe(PROV_TOPIC_SUB_REJECTED, create_rejected_callback);
    if (ret == -1)
    {
        ESP_LOGE(TAG_PROVISION, "Mqtt subscribe rejected failed");
        return ret;
    }
    ret = user_mqtt_client_subscribe(PROV_TOPIC_SUB_REGISTER_ACCEPTED, register_accepted_callback);
    if (ret == -1)
    {
        ESP_LOGE(TAG_PROVISION, "Mqtt subscribe register accepted failed");
        return ret;
    }
    ret = user_mqtt_client_subscribe(PROV_TOPIC_SUB_REGISTER_REJECTED, register_rejected_callback);
    if (ret == -1)
    {
        ESP_LOGE(TAG_PROVISION, "Mqtt subscribe register rejected failed");
        return ret;
    }
    return ret = 0;
}

/**
 * @brief mqtt send message provisioning
 *
 */
void mqtt_provision_create_certificate(void)
{
    // send null message to topic to create certificate
    int ret = -1;
    ESP_LOGI(TAG_PROVISION, "MQTT provisioning create certificate");
    ret = user_mqtt_client_publish(PROV_TOPIC_PUB_CREATE, "", 0);
    if (ret == -1)
        ESP_LOGE(TAG_PROVISION, "Mqtt publish failed");
}

/**
 * @brief mqtt get provsioned
 *
 * @return true
 * @return false
 */
bool mqtt_provision_getProvisioned(void)
{
    uint8_t provisioned = 0;
    if (Read_Byte(NAMESPACE_CONFIG, NVS_KEY_PROVISIONED, &provisioned) == 1)
    {
        provisioned = 0;
    }

    return provisioned;
}

/**
 * @brief mqtt provisioning set provisioned
 *
 * @param provisioned
 */
void mqtt_provision_setProvisioned(bool provisioned)
{
    Write_Byte(NAMESPACE_CONFIG, NVS_KEY_PROVISIONED, provisioned);
}

/**
 * @brief start mqtt provisioning task
 *
 */
bool mqtt_provision_task_start(void)
{
    bool ret = false;
    // check device provisioned
    if (!mqtt_provision_getProvisioned())
    {
        xTaskCreate(mqtt_provisioning_task, "mqtt_provisioning_task", 1024 * 10, NULL, 3, NULL);
        ret = false;
    }
    else
    {
        ret = true;
        ESP_LOGI(TAG_PROVISION, "device provisioned");
    }
    return ret;
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
static void mqtt_provisioning_task(void *pvParameters)
{
    wifi_mode_t wifi_mode;

    esp_wifi_get_mode(&wifi_mode);
    // wait wifi connected
    while (!(wifi_mode == WIFI_MODE_STA && (user_wifi_get_connected() == E_USER_WIFI_CONNECTED)))
    {
        ESP_LOGE(TAG_PROVISION, "wait wifi connecting ....");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    mqtt_provisioning_init();

    int ret = mqtt_provision_subscribe();
    if (ret == -1)
        ESP_LOGE(TAG_PROVISION, "mqtt_provision_subscribe failed");
    else if (ret == 0)
        ESP_LOGI(TAG_PROVISION, "mqtt_provision_subscribe success");
    while (1)
    {
        switch (provisioning_state)
        {
        case E_PROVISION_STATE_IDLE:
            // get connection status from aws
            if (user_mqtt_getConnetion() == true)
            {
                provisioning_state = E_PROVISION_STATE_CREATE_CERTIFICATE;
            }
            break;
        case E_PROVISION_STATE_CREATE_CERTIFICATE:
            // send null message to topic to create certificate
            mqtt_provision_create_certificate();
            provisioning_state = E_PROVISION_STATE_WAIT_CREATE_CERTIFICATE;
            break;
        case E_PROVISION_STATE_WAIT_CREATE_CERTIFICATE:
            // wait for create certificate message
            // event set by call back function
            break;
        case E_PROVISION_STATE_CREATE_CERTIFICATE_SUCESS:
            provisioning_state = E_PROVISION_STATE_REGISTER_THINGS;
            break;
        case E_PROVISION_STATE_REGISTER_THINGS:
            user_mqtt_provision_register();
            provisioning_state = E_PROVISION_STATE_WAIT_REGISTER_THINGS;
            break;
        case E_PROVISION_STATE_WAIT_REGISTER_THINGS:

            break;
        case E_PROVISION_STATE_REGISTER_THING_ACCEPTED:
            // save certificate to nvs
            ret = user_provision_save_cert();
            if (ret)
            {
                mqtt_provision_setProvisioned(true);
                ESP_LOGI(TAG_PROVISION, "E_PROVISION_STATE_REGISTER_THING_ACCEPTED save cert success");
                provisioning_state = E_PROVISION_STATE_DONE;
            }
            else
            {
                mqtt_provision_setProvisioned(false);
                ESP_LOGI(TAG_PROVISION, "E_PROVISION_STATE_REGISTER_THING_ACCEPTED save cert failed");
                provisioning_state = E_PROVISION_STATE_ERROR;
            }

            break;
        case E_PROVISION_STATE_ERROR:
            ESP_LOGE(TAG_PROVISION, "***** Provision Error restart the device");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
            break;
        case E_PROVISION_STATE_DONE:
            ESP_LOGI(TAG_PROVISION, "***** Provision Done");
            ESP_LOGI(TAG_PROVISION, "Restart the device");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
            break;
        default:
            break;
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void create_accepted_callback(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG_PROVISION, "Create accepted: %s", data);
    cJSON *root = cJSON_ParseWithLength(data, data_len);
    cJSON *Id = cJSON_GetObjectItem(root, "certificateId");
    cJSON *client_Cert = cJSON_GetObjectItem(root, "certificatePem");
    cJSON *client_Pkey = cJSON_GetObjectItem(root, "privateKey");
    cJSON *cert_Owner = cJSON_GetObjectItem(root, "certificateOwnershipToken");

    if (Id != NULL)
    {
        snprintf(fleet_prov_certificate_Id, sizeof(fleet_prov_certificate_Id), "%s",
                 cJSON_GetObjectItem(root, "certificateId")->valuestring);
    }
    if (client_Cert != NULL)
    {
        snprintf(fleet_prov_client_Cert, sizeof(fleet_prov_client_Cert), "%s",
                 cJSON_GetObjectItem(root, "certificatePem")->valuestring);
    }
    if (client_Pkey != NULL)
    {
        snprintf(fleet_prov_client_Pkey, sizeof(fleet_prov_client_Pkey), "%s",
                 cJSON_GetObjectItem(root, "privateKey")->valuestring);
    }
    if (cert_Owner != NULL)
    {
        snprintf(fleet_prov_cert_Owner, sizeof(fleet_prov_cert_Owner), "%s",
                 cJSON_GetObjectItem(root, "certificateOwnershipToken")->valuestring);
    }

    // ESP_LOGI(TAG_PROVISION, "fleet_prov_certificate_Id: %s", fleet_prov_certificate_Id);
    // ESP_LOGI(TAG_PROVISION, "fleet_prov_client_Cert: %s", fleet_prov_client_Cert);
    // ESP_LOGI(TAG_PROVISION, "fleet_prov_client_Pkey: %s", fleet_prov_client_Pkey);
    // ESP_LOGI(TAG_PROVISION, "fleet_prov_cert_Owner: %s", fleet_prov_cert_Owner);

    cJSON_Delete(root);

    user_mqtt_provision_set_state(E_PROVISION_STATE_CREATE_CERTIFICATE_SUCESS);
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
static bool user_provision_save_cert(void)
{
    // save CA cert
    esp_err_t ret = Write_Blob(NAMESPACE_CONFIG, KEY_ROOT_CA_CERT, (uint8_t *)root_cert_auth_pem_start, strlen(root_cert_auth_pem_start));
    if (ret != ESP_OK)
        return false;

    ESP_LOGI("CONFIG", "Write dev cert at %s:%s", NAMESPACE_CONFIG, KEY_ROOT_CA_CERT);

    ret = Write_Blob(NAMESPACE_CONFIG, KEY_PEM_CERT, (uint8_t *)fleet_prov_client_Cert, strlen(fleet_prov_client_Cert));
    if (ret != ESP_OK)
        return false;
    ESP_LOGI("CONFIG", "Write dev cert at %s:%s", NAMESPACE_CONFIG, KEY_PEM_CERT);

    // ret = user_nvs_set_string(NVS_KEY_PRIVATE_KEY, fleet_prov_client_Pkey); // client private key
    ret = Write_Blob(NAMESPACE_CONFIG, KEY_PEM_KEY, (uint8_t *)fleet_prov_client_Pkey, strlen(fleet_prov_client_Pkey));
    if (ret != ESP_OK)
        return false;
    ESP_LOGI("CONFIG", "Write dev cert at %s:%s", NAMESPACE_CONFIG, KEY_PEM_KEY);

    return true;
}
/**
 * @brief Create a rejected callback object
 *
 * @param topic
 * @param data
 * @param data_len
 */
void create_rejected_callback(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG_PROVISION, "Create rejected: %s", data);
    user_mqtt_provision_set_state(E_PROVISION_STATE_ERROR);
}

/**
 * @brief
 *
 * @param topic
 * @param data
 * @param data_len
 */
void register_accepted_callback(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG_PROVISION, "Register accepted: %s", data);

    cJSON *root = cJSON_ParseWithLength(data, data_len);
    cJSON *thingName = cJSON_GetObjectItem(root, "thingName");
    cJSON *ThingPassword = cJSON_GetObjectItem(root, "ThingPassword");
    cJSON *deviceConfiguration = cJSON_GetObjectItem(root, "deviceConfiguration");
    if (thingName != NULL)
    {
        memset(fleet_prov_thing_Name, 0x00, sizeof(fleet_prov_thing_Name));
        snprintf(fleet_prov_thing_Name, sizeof(fleet_prov_thing_Name), "%s",
                 cJSON_GetObjectItem(root, "thingName")->valuestring);
    }
    else
    {
        ESP_LOGI(TAG_PROVISION, "error get thing name");
    }

    if (deviceConfiguration != NULL)
    {
        cJSON *ThingPassword = cJSON_GetObjectItem(deviceConfiguration, "ThingPassword");
        cJSON *ThingType = cJSON_GetObjectItem(deviceConfiguration, "ThingType");
        if (ThingPassword != NULL)
        {
            memset(fleet_prov_thing_pass, 0x00, sizeof(fleet_prov_thing_pass));
            snprintf(fleet_prov_thing_pass, sizeof(fleet_prov_thing_pass), "%s",
                     cJSON_GetObjectItem(deviceConfiguration, "ThingPassword")->valuestring);
        }
        if (ThingType != NULL)
        {
            memset(fleet_prov_thing_type, 0x00, sizeof(fleet_prov_thing_type));
            snprintf(fleet_prov_thing_type, sizeof(fleet_prov_thing_type), "%s",
                     cJSON_GetObjectItem(deviceConfiguration, "ThingType")->valuestring);
        }
    }
    else
    {
        ESP_LOGI(TAG_PROVISION, "error get thing pass");
    }

    ESP_LOGI(TAG_PROVISION, "fleet_prov_thing_Name: %s", fleet_prov_thing_Name);
    ESP_LOGI(TAG_PROVISION, "fleet_prov_thing_pass: %s", fleet_prov_thing_pass);
    ESP_LOGI(TAG_PROVISION, "fleet_prov_thing_type: %s", fleet_prov_thing_type);

    user_mqtt_provision_set_state(E_PROVISION_STATE_REGISTER_THING_ACCEPTED);
}

/**
 * @brief
 *
 * @param topic
 * @param data
 * @param data_len
 */
void register_rejected_callback(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG_PROVISION, "Register rejected: %s", data);
    user_mqtt_provision_set_state(E_PROVISION_STATE_ERROR);
}

/**
 * @brief register the device to the cloud
 *
 * @return true
 * @return false
 */
static int user_mqtt_provision_register(void)
{
    int ret = -1;
    char *buffer;
    buffer = (char *)malloc(1024 + 1);
    if (buffer == NULL)
        return ret;
    memset(buffer, 0, 1024 + 1);

    sprintf(buffer, "{\"certificateOwnershipToken\":\"%s\",\"parameters\":{\"serialNumber\":\"%s\",\"deviceType\":\"%s\"}}",
            fleet_prov_cert_Owner,
            device_info.device_name,
            DEVICE_MODEL);

    ESP_LOGI(TAG_PROVISION, "MQTT provisioning register: %s", buffer);

    if (user_mqtt_client_publish(PROV_TOPIC_PUB_REGISTER, (const char *)buffer, strlen((const char *)buffer)) == -1)
    {
        free(buffer);
        return (ret = -1);
    }

    free(buffer);
    return (ret = 0);
}

/**
 * @brief set provision state
 *
 * @param state
 */
static void user_mqtt_provision_set_state(e_provisioning_state state)
{
    provisioning_state = state;
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
