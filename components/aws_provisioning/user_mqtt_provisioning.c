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
#include "user_mqtt_provisioning.h"
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
#include "user_nvs.h"
#include "user_common_func.h"
#include "aws_mqtt_config.h"
#include "mqtt_cert.h"
#include "esp_mac.h"
// #include "esp_http_client.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define TAG_PROVISION "MQTT PROVISIONING"

#define NVS_KEY_PROVISIONED "provisioned"
#define NVS_KEY_CERTIFICATE_PEM "certificatePem"
#define NVS_KEY_PRIVATE_KEY "privateKey"

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
char fleet_prov_certificate_Id[100];
// char fleet_prov_client_Cert[2000];
// char fleet_prov_client_Pkey[2000];
char fleet_prov_cert_Owner[600];
char fleet_prov_thing_Name[20];

e_provisioning_state provisioning_state = E_PROVISION_STATE_IDLE;
/***********************************************************************************************************************
 * Exported global variables
 ***********************************************************************************************************************/
extern const char root_cert_auth_pem_start[] asm("_binary_root_cert_auth_pem_start");
extern const char root_cert_auth_pem_end[] asm("_binary_root_cert_auth_pem_end");

// extern const char client_cert_pem_start[] asm("_binary_client_crt_start");
// extern const char client_cert_pem_end[] asm("_binary_client_crt_end");

// extern const char client_key_pem_start[] asm("_binary_client_key_start");
// extern const char client_key_pem_end[] asm("_binary_client_key_end");

char fleet_prov_client_Cert[2000] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDWjCCAkKgAwIBAgIVAM/ZYyI4HMZvJdYqwU2XgILw/QRFMA0GCSqGSIb3DQEB\n"
    "CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"
    "IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMjA2MDkxNDEz\n"
    "MzNaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"
    "dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDoDp8zrFqxxnyqWZiX\n"
    "s8FWZpLMvPUSx7sJ7CHcDO7UxQx3r9QtssRZQyYwoQoDbR/ZrdFPAlmiyZuKRpDd\n"
    "/rKAqbFCd63I0RKglmFHcY22gXHke9ABQY4Oa+BlULjw3fwIBAfkNPvYqrxAIDlV\n"
    "FY1urVbtyrDlLtt0JmagSD+TCAt386sSEq5ZBSwgVbDZc2LP9yJjoeXzzBP3Y4nP\n"
    "NNJHYH9K/Ka0pUDjO0oA26Tqlq+VSAVtaMeN3h8DrU2GeyNvQiRbVTWwebKvfK6N\n"
    "Bk5y2zZvNAZbvKoHZy7utK6uYVW9lH9aCXS5ut2JJVQm9aMupgEoqpQn++o8m7pO\n"
    "Qku9AgMBAAGjYDBeMB8GA1UdIwQYMBaAFGjiycaJZP32RI/Wi6VNqZPlImCEMB0G\n"
    "A1UdDgQWBBSX8uJF1Rt2U9pswruT4Awd+gPk8TAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"
    "DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAFU5ubrTJxDBxRk1NENlB09h8\n"
    "CcYTqoc+plSgATgapiRcpYBhl1kOP0W8WGndK4C7Jp42ClPJdF75F7C3sbl6AgDJ\n"
    "EiBnZdIrLhh1eesW98zNc7cfW1g6eQgMIIVDCO3OPvbATAfsdJuwVcUIU1i5yis2\n"
    "x6vgSUxJ3VoPAZi0kOMm1Oz3/4uwdrz4OiEMdppWyKhmyezpuhPTYdU5Bv1yhG4D\n"
    "3QhY8me/X5bpDSfIvgjNPzC+ZQ8e1y/wW3AcCZZWST3ejKB7+llxzgV5vwPdu7TT\n"
    "nJK4BIMy6wVkhIS9cOeUUkFIFwAlKjaCGLem3BST/C+4uKI7opjgU9QB1Q9L/w==\n"
    "-----END CERTIFICATE-----"};

char fleet_prov_client_Pkey[2000] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEowIBAAKCAQEA6A6fM6xascZ8qlmYl7PBVmaSzLz1Ese7Cewh3Azu1MUMd6/U\n"
    "LbLEWUMmMKEKA20f2a3RTwJZosmbikaQ3f6ygKmxQnetyNESoJZhR3GNtoFx5HvQ\n"
    "AUGODmvgZVC48N38CAQH5DT72Kq8QCA5VRWNbq1W7cqw5S7bdCZmoEg/kwgLd/Or\n"
    "EhKuWQUsIFWw2XNiz/ciY6Hl88wT92OJzzTSR2B/SvymtKVA4ztKANuk6pavlUgF\n"
    "bWjHjd4fA61Nhnsjb0IkW1U1sHmyr3yujQZOcts2bzQGW7yqB2cu7rSurmFVvZR/\n"
    "Wgl0ubrdiSVUJvWjLqYBKKqUJ/vqPJu6TkJLvQIDAQABAoIBAHyOhKNuysuLV5T0\n"
    "PHSI9qiSrA8bCYyICnv78/yyMGuiTNvdU80nnD6xTUli1wYZx7PGmYeIIma/Qi6J\n"
    "S+7sbBGlg9DCflRA7Hen4BVB/SfN+T0DvBVAg6h3/N1E48/dxw7iYv359ohItHzT\n"
    "DxL9W4nhnrxKRILvkLmadxFnVkBrVlVA5zyOE4VeIm+ZZ0Z9fXA9d+ZAjOEWkyfG\n"
    "s6Knl/kxpiJ08qB6xax7mRn4SoLS8ayCgvQS235EYcCyWa95rYdRb6DE6JX35kAS\n"
    "o37FMLLQE9/WjGjKViHHZITUB1tNQ25i1TsZW1Wr3toO1g4m5kLFhIhx2xYnndeD\n"
    "aK4R/6ECgYEA94r0IEJL0y9qNKSeVA+0OO70EU3BeELxmvJ84uMPnDu0THl8JUVt\n"
    "nx6QsCt2mVcCHk7WEW05zCsIj52GhXjXH+9ATyiCCULLwp0bzm2r68otEDPAgtbW\n"
    "Jp6aTV8hAfMABLkjC20jyI7A1RP3VO5IWgv3sQ9wOll9KOto1zPccIkCgYEA7/w6\n"
    "b3Ho12WWv0naJHfpxDjFlebLnSobhj1h79cY4sGghAjH+EmEXVQ0m1m2BoolOcR1\n"
    "7jfX1HrQfrB5xWX1Ir/jn9aqjip5w5Aa4kB164MOrEFFJ4WjJ0wlIgmo8C1gnMzi\n"
    "jDX5Lih2508vq4IzniOMbTzkXQ1jqT4JDCbjbJUCgYByJixvF8M5blruTZ8JKniz\n"
    "7FS2CgIWP+CD0CAQzB3tsIZy4W6DDWIAhyq3YkN9cGb0rOv/+zs0z/9RopVexRuG\n"
    "iWBBYG+eb6PgaWxiI6asw31GRGcYrpLwGiETXrOs3255vxnO8hQXLuTzHRLnoj3Q\n"
    "8EXW60SYU53oma2t1ydm6QKBgHkCQBzt3TCvgwHVpW96H5/X2JmrlMQc9WANF5Mu\n"
    "bNf6NowdOWgS+fFtglFLPSOzCO5GCAkkk2oKu5MGPYCx1pe9FnQEdF/XPyrDQSsQ\n"
    "weUNYpqtFsfE9O3d0WOGZ28kEnE+RoXYuVrzAHHGVg0vTAPTfP2gFK5vD+/I3hsI\n"
    "iOi5AoGBAIh2HVqq77HOeJr65F19R8LIr3m6Y2yt1jZ/BV1HmEAA2Aeqv1jf4k+n\n"
    "OlcT/JagnO7hFFSckb0+txJAUgVx4akeR4AWBDFHDCQT0dpM1hwR7gYxhydxfS5L\n"
    "aG3E7bZXa2nXnLKNQe7o0rUuYZ9yr4AwYAzzEyO6Av2khDcZFUDu\n"
    "-----END RSA PRIVATE KEY-----"};

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
    ESP_LOGI(TAG_PROVISION, "MQTT provisioning init");
    user_mqtt_client_t mqtt_client;
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    sprintf(mqtt_client.mqtt_host_address, "%s", AWS_IOT_ENDPOINT);
    sprintf(mqtt_client.mqtt_client_id, "%02x%02x%02x%02x%02x%02x", eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    // sprintf(mqtt_client.mqtt_client_id, "%s", AWS_IOT_CLIENT_ID);
    mqtt_client.Port = AWS_IOT_PORT;
    int ret = user_mqtt_init(&mqtt_client,
                             fleet_prov_client_Cert,
                             fleet_prov_client_Pkey,
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
    while (mqtt_connecttion == false)
    {
        mqtt_connecttion = user_mqtt_getConnetion();
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
    int provisioned = 0;
    user_nvs_get_int(NVS_KEY_PROVISIONED, &provisioned);
    return provisioned;
}

/**
 * @brief mqtt provisioning set provisioned
 *
 * @param provisioned
 */
void mqtt_provision_setProvisioned(bool provisioned)
{
    int ret = user_nvs_set_int(NVS_KEY_PROVISIONED, &provisioned);
}

/**
 * @brief start mqtt provisioning task
 *
 */
void mqtt_provision_task_start(void)
{
    xTaskCreate(mqtt_provisioning_task, "mqtt_provisioning_task", 1024 * 10, NULL, 3, NULL);
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
static void mqtt_provisioning_task(void *pvParameters)
{
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
            ESP_LOGE(TAG_PROVISION, "***** Provision Error");
            break;
        case E_PROVISION_STATE_DONE:
            ESP_LOGI(TAG_PROVISION, "***** Provision Done");
            ESP_LOGI(TAG_PROVISION, "Restart the device");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            user_esp32_restart();
            // reset
            break;
        default:
            break;
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void create_accepted_callback(const char *topic, const char *data, int data_len)
{
    // ESP_LOGI(TAG_PROVISION, "Create accepted: %s", data);
    cJSON *root = cJSON_ParseWithLength(data, data_len);
    cJSON *Id = cJSON_GetObjectItem(root, "certificateId");
    cJSON *client_Cert = cJSON_GetObjectItem(root, "certificatePem");
    cJSON *client_Pkey = cJSON_GetObjectItem(root, "privateKey");
    cJSON *cert_Owner = cJSON_GetObjectItem(root, "certificateOwnershipToken");

    if (Id != NULL)
    {
        // fleet_prov_certificate_Id = cJSON_GetObjectItem(root, "certificateId")->valuestring;
        snprintf(fleet_prov_certificate_Id, sizeof(fleet_prov_certificate_Id), "%s",
                 cJSON_GetObjectItem(root, "certificateId")->valuestring);
    }
    if (client_Cert != NULL)
    {
        // fleet_prov_client_Cert = cJSON_GetObjectItem(root, "certificatePem")->valuestring;
        snprintf(fleet_prov_client_Cert, sizeof(fleet_prov_client_Cert), "%s",
                 cJSON_GetObjectItem(root, "certificatePem")->valuestring);
    }
    if (client_Pkey != NULL)
    {
        // fleet_prov_client_Pkey = cJSON_GetObjectItem(root, "privateKey")->valuestring;
        snprintf(fleet_prov_client_Pkey, sizeof(fleet_prov_client_Pkey), "%s",
                 cJSON_GetObjectItem(root, "privateKey")->valuestring);
    }
    if (cert_Owner != NULL)
    {
        // fleet_prov_cert_Owner = cJSON_GetObjectItem(root, "certificateOwnershipToken")->valuestring;
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
 */
static bool user_provision_save_cert(void)
{
    // user_nvs_set_string(NVS_KEY_CERTIFICATE_ID, fleet_prov_certificate_Id);
    bool ret = user_nvs_set_string(NVS_KEY_CERTIFICATE_PEM, fleet_prov_client_Cert); // client certificate
    if (ret == false)
        return false;

    ret = user_nvs_set_string(NVS_KEY_PRIVATE_KEY, fleet_prov_client_Pkey); // client private key
    if (ret == false)
        return false;

    return true;
    // user_nvs_set_string(NVS_KEY_CERTIFICATE_OWNERSHIP_TOKEN, fleet_prov_cert_Owner);
}

bool user_provision_get_certificate(void)
{
    // fleet_prov_certificate_Id = user_nvs_get_string(NVS_KEY_CERTIFICATE_ID);
    size_t length = 0;
    int ret = -1;
    ret = user_nvs_get_string(NVS_KEY_CERTIFICATE_PEM, &fleet_prov_client_Cert, &length); // client certificate
    if (ret == -1)
        return false;

    ret = user_nvs_get_string(NVS_KEY_PRIVATE_KEY, &fleet_prov_client_Pkey, &length); // client certificate
    if (ret == -1)
        return false;

    return true;
}

void create_rejected_callback(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG_PROVISION, "Create rejected: %s", data);
    user_mqtt_provision_set_state(E_PROVISION_STATE_ERROR);
}

void register_accepted_callback(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG_PROVISION, "Register accepted: %s", data);
    user_mqtt_provision_set_state(E_PROVISION_STATE_REGISTER_THING_ACCEPTED);
}

void register_rejected_callback(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG_PROVISION, "Register rejected: %s", data);
    user_mqtt_provision_set_state(E_PROVISION_STATE_ERROR);
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
static int user_mqtt_provision_register(void)
{
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    int ret = -1;
    uint8_t eth_mac[6];
    // esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    // esp_read_mac(eth_mac, ESP_MAC_WIFI_STA);
    esp_efuse_mac_get_default(eth_mac);
    snprintf(fleet_prov_thing_Name, sizeof(fleet_prov_thing_Name), "%02X%02X%02X",
             eth_mac[3], eth_mac[4], eth_mac[5]);

    sprintf(buf, "{\"certificateOwnershipToken\":\"%s\",\"parameters\":{\"SerialNumber\":\"%s\"}}",
            fleet_prov_cert_Owner,
            fleet_prov_thing_Name);

    ESP_LOGI(TAG_PROVISION, "MQTT provisioning register: %s", buf);

    if (user_mqtt_client_publish(PROV_TOPIC_PUB_REGISTER, (const char *)buf, strlen((const char *)buf)) == -1)
    {
        return (ret = -1);
    }

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
