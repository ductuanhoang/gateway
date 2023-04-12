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
#include "user_mqtt.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "mqtt_client.h"
// #include "esp_http_client.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define TAG_MQTT "MQTT"

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global variables
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static volatile user_mqtt_client_t user_mqtt_client;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static bool isConnected = false;
static uint8_t user_mqtt_sub_count = 0;
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
int user_mqtt_init(user_mqtt_client_t *client,
                   const char *certificate_pem_crt,
                   const char *private_pem_key,
                   const char *aws_root_ca_pem)
{
    // ESP_LOGI(TAG_MQTT, "certificate_pem_crt: %s", certificate_pem_crt);
    int ret = -1;
    user_mqtt_setConnection(false);
    user_mqtt_client.buffer_message = NULL;
    user_mqtt_sub_count = 0;
    if (user_mqtt_client.mqtt_client == NULL)
        memcpy(&user_mqtt_client, client, sizeof(user_mqtt_client));
    else
    {
        ESP_LOGE(TAG_MQTT, "client is used");
        return ret;
    }
#if TEST_CONNECT_CLOUD_MQTT == 1
    // using cloud mqt
    const esp_mqtt_client_config_t mqtt_cfg =
        {
            .host = user_mqtt_client.mqtt_host_address, // "m13.cloudmqtt.com",
            .port = user_mqtt_client.Port,
            .client_id = user_mqtt_client.mqtt_client_id, // client_id,
            .username = "wcewiofp",
            .password = "fyFZMCLNvoD9",
            .keepalive = 60,
            .event_handle = mqtt_event_handler,
        };
    ESP_LOGI(TAG_MQTT, "mqtt_host_address: %s", mqtt_cfg.host);
    ESP_LOGI(TAG_MQTT, "mqtt_client_id: %s", mqtt_cfg.client_id);

    user_mqtt_client.mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ret = esp_mqtt_client_start(user_mqtt_client.mqtt_client);
#elif TEST_CONNECT_CLOUD_MQTT == 0
    if ((aws_root_ca_pem == NULL) || (certificate_pem_crt == NULL) || (private_pem_key == NULL))
    {
        ESP_LOGE(TAG_MQTT, "aws_root_ca_pem or certificate_pem_crt or private_pem_key is NULL");
        ret = -1;
    }
    else
    {
        // "mqtts://a3o2ltl9whgl65-ats.iot.ap-southeast-1.amazonaws.com:8883"
        char client_buffer_url[150];
        sprintf(client_buffer_url, "mqtts://%s:%d",
                user_mqtt_client.mqtt_host_address,
                user_mqtt_client.Port);

        ESP_LOGI(TAG_MQTT, "connecting to server %s", user_mqtt_client.mqtt_host_address);
        const esp_mqtt_client_config_t mqtt_cfg =
            {
                .uri = client_buffer_url,
                .event_handle = mqtt_event_handler,
                .client_cert_pem = (const char *)certificate_pem_crt,
                .client_key_pem = (const char *)private_pem_key,
                .cert_pem = (const char *)aws_root_ca_pem,
                .client_id = user_mqtt_client.mqtt_client_id,
                .keepalive = 60,
            };
        user_mqtt_client.mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        ret = esp_mqtt_client_start(user_mqtt_client.mqtt_client);
    }
#endif
    ESP_LOGI(TAG_MQTT, "indx2 mqtt subscribe count = %d", user_mqtt_sub_count);
    return ret;
}

/**
 * @brief
 *
 * @param topic
 * @param data
 * @return init
 */
int user_mqtt_client_publish(const char *pubTopic, const char *pubPayLoad, uint16_t payLoadLen)
{
    int ret = -1;
    if (NULL == pubTopic)
    {
        ESP_LOGE(TAG_MQTT, "mqtt publish data failed please check param again");
        return ret;
    }

    else if (user_mqtt_getConnetion() == true)
    {
        ret = esp_mqtt_client_publish(user_mqtt_client.mqtt_client,
                                      pubTopic,
                                      pubPayLoad,
                                      payLoadLen,
                                      0, 0); // default QOS is 0
        if (ret == -1)
        {
            ESP_LOGE(TAG_MQTT, "mqtt publish data failed");
        }
        else
            ESP_LOGI(TAG_MQTT, "mqtt publish data success");
    }
    else
        ESP_LOGE(TAG_MQTT, "mqtt publish data failed mqtt disconnected");

    return ret;
}

int user_mqtt_client_subscribe(const char *subTopic, pMqttCallback_t pMqttCallback)
{
    int ret = -1;
    int indx = user_mqtt_sub_count;
    ESP_LOGI(TAG_MQTT, "indx mqtt subscribe count = %d", user_mqtt_sub_count);
    if (NULL == subTopic)
    {
        ESP_LOGE(TAG_MQTT, "mqtt subscribe data failed please check param again");
        return ret;
    }
    else if (user_mqtt_getConnetion())
    {
        ESP_LOGI(TAG_MQTT, "user_mqtt_getConnetion");
        for (int i = 0; i < MQTT_MAX_SUB_TOPIC; i++)
        {
            // if (sub_topic.sub[i].topic == String(topic))
            if (strcmp(user_mqtt_client.sub_topic.sub[i].topic, subTopic) == 0)
            {
                indx = i;
                break;
            }
        }

        if (indx == user_mqtt_sub_count)
        {
            if (user_mqtt_sub_count >= MQTT_MAX_SUB_TOPIC)
            {
                ESP_LOGE(TAG_MQTT, "mqtt subscribe topic number is max is %d > MQTT_MAX_SUB_TOPIC", user_mqtt_sub_count);
                return -1;
            }
        }
        sprintf(user_mqtt_client.sub_topic.sub[indx].topic, "%.*s", strlen(subTopic), subTopic);
        ESP_LOGI(TAG_MQTT, "add topic sub %s -- %d",
                 user_mqtt_client.sub_topic.sub[indx].topic,
                 user_mqtt_sub_count);

        user_mqtt_client.sub_topic.sub[indx].callback = pMqttCallback;
        ret = esp_mqtt_client_subscribe(user_mqtt_client.mqtt_client, subTopic, 0);
        if (ret == -1)
            ESP_LOGE(TAG_MQTT, "mqtt subscribe data failed");
        user_mqtt_sub_count++;
        ESP_LOGE(TAG_MQTT, "mqtt subscribe count = %d", user_mqtt_sub_count);
    }
    else
        ESP_LOGE(TAG_MQTT, "mqtt subscribe data failed mqtt disconnected");

    return ret;
}
/**
 * @brief
 *
 * @param connected
 */
void user_mqtt_setConnection(bool connected)
{
    isConnected = connected;
}

/**
 * @brief
 *
 * @param pClient
 * @return int
 */
bool user_mqtt_getConnetion(void)
{
    return isConnected;
}

/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Function Name:
 * Description  :
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    user_mqtt_client.mqtt_client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        // msg_id = esp_mqtt_client_subscribe(client, mqtt_config.mqtt_topic_jobsub, 0);
        // ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);
        user_mqtt_setConnection(true);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        user_mqtt_setConnection(false);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA : %d", event->total_data_len);

        if (event->current_data_offset == 0)
        {
            // if (user_mqtt_client.buffer_message != NULL)
            // {
            //     ESP_LOGE(TAG_MQTT, "go 1");
            //     free(user_mqtt_client.buffer_message);
            // }
            user_mqtt_client.buffer_message = (char *)malloc(event->total_data_len + 1);
            if (event->topic_len <= MQTT_BUFFER_TOPIC_MAX_LENGHT)
            {
                user_mqtt_client.buffer_message_length = 0;
                sprintf(user_mqtt_client.buffer_topic, "%.*s", event->topic_len, event->topic);
            }
            else
                ESP_LOGE(TAG_MQTT, "length of topic is too large: %d", event->topic_len);
        }

        user_mqtt_client.buffer_message_length += sprintf(user_mqtt_client.buffer_message + user_mqtt_client.buffer_message_length,
                                                          "%.*s",
                                                          event->data_len,
                                                          (char *)event->data);

        if (user_mqtt_client.buffer_message_length > MQTT_BUFFER_MESSAGE_MAX_LENGHT)
        {
            ESP_LOGE(TAG_MQTT, "length of message is too large: %d", user_mqtt_client.buffer_message_length);
            break;
        }

        // ESP_LOGI(TAG_MQTT, "event->total_data_len = %d", event->total_data_len);
        // ESP_LOGI(TAG_MQTT, "user_mqtt_client.buffer_message_length = %d", user_mqtt_client.buffer_message_length);
        // ESP_LOGI(TAG_MQTT, "user_mqtt_client.buffer_topic = %s", user_mqtt_client.buffer_topic);
        if (user_mqtt_client.buffer_message_length == event->total_data_len)
        {
            // ESP_LOGI(TAG_MQTT, "received = %s", user_mqtt_client.buffer_message);
            for (int i = 0; i < user_mqtt_sub_count; i++)
            {
                if (strcmp(user_mqtt_client.sub_topic.sub[i].topic, user_mqtt_client.buffer_topic) == 0)
                {
                    // ESP_LOGI(TAG_MQTT, "call back %s", user_mqtt_client.sub_topic.sub[i].topic);

                    // if (!user_mqtt_client.sub_topic.sub[i].callback)
                    // {
                    user_mqtt_client.sub_topic.sub[i].callback(user_mqtt_client.buffer_topic, user_mqtt_client.buffer_message, user_mqtt_client.buffer_message_length);
                    // }

                    break;
                }
            }
            if (user_mqtt_client.buffer_message != NULL)
            {
                // ESP_LOGE(TAG_MQTT, "go 3");
                free(user_mqtt_client.buffer_message);
            }
            user_mqtt_client.buffer_message_length = 0;
            memset(user_mqtt_client.buffer_topic, 0, MQTT_BUFFER_TOPIC_MAX_LENGHT);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
