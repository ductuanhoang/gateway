
/**
 * @file user_aws_v2.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-22
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
#include "user_aws_v2.h"
#include "user_aws_v2_event.h"
#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <mqtt_client.h>
#include <esp_event.h>
#include <esp_idf_version.h>
#include "aws_mqtt_provisioning.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define MEM_ALLOC_EXTRAM(size) malloc(size)
#define MEM_CALLOC_EXTRAM(num, size) calloc(num, size)
#define MEM_REALLOC_EXTRAM(ptr, size) realloc(ptr, size)

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define TAG "MQTT_V2"

/**
 * @brief
 *
 */
typedef struct
{
    char *topic;
    esp_rmaker_mqtt_subscribe_cb_t cb;
    void *priv;
} mqtt_v2_subscription_t;

/**
 * @brief
 *
 */
typedef struct
{
    esp_mqtt_client_handle_t mqtt_client;
    esp_rmaker_mqtt_conn_params_t *conn_params;
    mqtt_v2_subscription_t *subscriptions[MAX_MQTT_SUBSCRIPTIONS];
    int mqtt_connection_state;
} mqtt_v2_data_t;

/**
 * @brief
 *
 */
typedef struct
{
    char *data;
    char *topic;
} mqtt_v2_long_data_t;
/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/

static void mqtt_v2_subscribe_callback(const char *topic, int topic_len, const char *data, int data_len);
static void unsubscribe_helper(mqtt_v2_subscription_t **subscription);
static mqtt_v2_long_data_t *mqtt_v2_free_long_data(mqtt_v2_long_data_t *long_data);
static mqtt_v2_long_data_t *mqtt_v2_manage_long_data(mqtt_v2_long_data_t *long_data, esp_mqtt_event_handle_t event);
static void mqtt_v2_unsubscribe_all(void);

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);

static mqtt_v2_data_t *mqtt_data;

/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/
ESP_EVENT_DEFINE_BASE(RMAKER_COMMON_EVENT);
/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

/**
 * @brief
 *
 * @param conn_params
 */
esp_err_t mqtt_v2_init(esp_rmaker_mqtt_conn_params_t *conn_params)
{
    if (mqtt_data)
    {
        ESP_LOGE(TAG, "MQTT already initialized");
        return ESP_OK;
    }
    if (!conn_params)
    {
        ESP_LOGE(TAG, "Connection params are mandatory for mqtt_v2_init");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Initialising MQTT");
    mqtt_data = (mqtt_v2_data_t *)calloc(1, sizeof(mqtt_v2_data_t));
    if (!mqtt_data)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for mqtt_v2_data_t");
        return ESP_ERR_NO_MEM;
    }
    mqtt_data->conn_params = conn_params;

    ESP_LOGI(TAG, "server_cert_len = %d", mqtt_data->conn_params->server_cert_len);
    ESP_LOGI(TAG, "client_cert_len = %d", mqtt_data->conn_params->client_cert_len);
    ESP_LOGI(TAG, "client_key_len = %d", mqtt_data->conn_params->client_key_len);
    char client_buffer_url[150];
    sprintf(client_buffer_url, "mqtts://%s:%d",
            conn_params->mqtt_host,
            conn_params->mqtt_port);

    const esp_mqtt_client_config_t mqtt_client_cfg = {
        .uri = (const char *)client_buffer_url,
        .event_handle = mqtt_event_handler,
        .port = conn_params->mqtt_port,
        .cert_pem = (char *)(fleet_prov_cert_auth),
        .cert_len = conn_params->server_cert_len + 1,

        .client_cert_pem = (char *)(fleet_prov_client_Cert),
        .client_cert_len = conn_params->client_cert_len + 1,

        .client_key_pem = (char *)fleet_prov_client_Pkey,
        .client_key_len = conn_params->client_key_len + 1,

        .client_id = (const char *)conn_params->client_id,
        // .disable_auto_reconnect = true,
        .keepalive = 120,
    };

    ESP_LOGI(TAG, "Read uri %s", (const char *)mqtt_client_cfg.uri);
    ESP_LOGI(TAG, "Read client_id %s", (const char *)mqtt_client_cfg.client_id);

    mqtt_data->mqtt_client = esp_mqtt_client_init(&mqtt_client_cfg);
    if (!mqtt_data->mqtt_client)
    {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        mqtt_v2_deinit();
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief
 *
 * @return esp_err_t
 */
esp_err_t mqtt_v2_connect(void)
{
    if (!mqtt_data)
    {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Connecting to %s", mqtt_data->conn_params->mqtt_host);
    esp_err_t ret = esp_mqtt_client_start(mqtt_data->mqtt_client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_mqtt_client_start() failed with err = %d", ret);
        return ret;
    }
    return ESP_OK;
}

/**
 * @brief
 *
 * @return esp_err_t
 */
esp_err_t mqtt_v2_disconnect(void)
{
    esp_err_t ret;
    ret = esp_mqtt_client_disconnect(mqtt_data->mqtt_client);
    return ret;
}

/**
 * @brief
 *
 * @return esp_err_t
 */
esp_err_t mqtt_v2_reconnect(void)
{
    esp_err_t ret;
    ret = esp_mqtt_client_reconnect(mqtt_data->mqtt_client);
    return ret;
}
/**
 * @brief deinit the mqtt client
 *
 */
void mqtt_v2_deinit(void)
{
    mqtt_v2_unsubscribe_all();
    if (mqtt_data && mqtt_data->mqtt_client)
    {
        esp_mqtt_client_destroy(mqtt_data->mqtt_client);
    }
    if (mqtt_data)
    {
        free(mqtt_data);
        mqtt_data = NULL;
    }
}

/**
 * @brief get status of mqtt connection
 *
 * @return int
 */
int mqtt_v2_get_connection(void)
{
    if (mqtt_data == NULL)
        return 0;
    else
        return mqtt_data->mqtt_connection_state;
}

/**
 * @brief set status of mqtt connection
 *
 * @param connection
 */
void mqtt_v2_set_connection(int connection)
{
    mqtt_data->mqtt_connection_state = connection;
}

/**
 * @brief unsubscribe topic
 *
 * @param topic
 * @return esp_err_t
 */
esp_err_t mqtt_v2_unsubscribe(const char *topic)
{
    if (!mqtt_data || !topic)
    {
        return ESP_FAIL;
    }
    mqtt_v2_subscription_t **subscriptions = mqtt_data->subscriptions;
    int i;
    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++)
    {
        if (subscriptions[i])
        {
            if (strncmp(topic, subscriptions[i]->topic, strlen(topic)) == 0)
            {
                unsubscribe_helper(&subscriptions[i]);
                return ESP_OK;
            }
        }
    }
    return ESP_FAIL;
}

/**
 * @brief subscribe to a topic
 *
 * @param topic
 * @param cb
 * @param qos
 * @param priv_data
 * @return esp_err_t
 */
esp_err_t mqtt_v2_subscribe(const char *topic, esp_rmaker_mqtt_subscribe_cb_t cb, uint8_t qos, void *priv_data)
{
    if (!mqtt_data || !topic || !cb)
    {
        return ESP_FAIL;
    }
    int i;
    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++)
    {
        if (!mqtt_data->subscriptions[i])
        {
            mqtt_v2_subscription_t *subscription = calloc(1, sizeof(mqtt_v2_subscription_t));
            if (!subscription)
            {
                return ESP_FAIL;
            }
            subscription->topic = strdup(topic);
            if (!subscription->topic)
            {
                free(subscription);
                return ESP_FAIL;
            }
            int ret = esp_mqtt_client_subscribe(mqtt_data->mqtt_client, subscription->topic, qos);
            if (ret < 0)
            {
                free(subscription->topic);
                free(subscription);
                return ESP_FAIL;
            }
            subscription->priv = priv_data;
            subscription->cb = cb;
            mqtt_data->subscriptions[i] = subscription;
            ESP_LOGI(TAG, "Subscribed to topic: %s", topic);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

/**
 * @brief
 *
 * @param topic
 * @param data
 * @param data_len
 * @param qos
 * @param msg_id
 * @return esp_err_t
 */
esp_err_t mqtt_v2_publish(const char *topic, const char *data, size_t data_len, uint8_t qos, int *msg_id)
{
    if (!mqtt_data || !topic || !data)
    {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Publishing to %s", topic);
    int ret = esp_mqtt_client_publish(mqtt_data->mqtt_client, topic, data, data_len, qos, 0);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "MQTT Publish failed");
        return ESP_FAIL;
    }
    if (msg_id)
    {
        *msg_id = ret;
    }
    return ESP_OK;
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

/**
 * @brief
 *
 * @param topic
 * @param topic_len
 * @param data
 * @param data_len
 */
static void mqtt_v2_subscribe_callback(const char *topic, int topic_len, const char *data, int data_len)
{
    mqtt_v2_subscription_t **subscriptions = mqtt_data->subscriptions;
    int i;
    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++)
    {
        if (subscriptions[i])
        {
            if ((strncmp(topic, subscriptions[i]->topic, topic_len) == 0) && (topic_len == strlen(subscriptions[i]->topic)))
            {
                subscriptions[i]->cb(subscriptions[i]->topic, (void *)data, data_len, subscriptions[i]->priv);
            }
        }
    }
}

/**
 * @brief
 *
 */
static void mqtt_v2_unsubscribe_all(void)
{
    if (!mqtt_data)
    {
        return;
    }
    int i;
    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++)
    {
        if (mqtt_data->subscriptions[i])
        {
            unsubscribe_helper(&(mqtt_data->subscriptions[i]));
        }
    }
}
/**
 * @brief
 *
 * @param subscription
 */
static void unsubscribe_helper(mqtt_v2_subscription_t **subscription)
{
    if (subscription && *subscription)
    {
        if (esp_mqtt_client_unsubscribe(mqtt_data->mqtt_client, (*subscription)->topic) < 0)
        {
            ESP_LOGW(TAG, "Could not unsubscribe from topic: %s", (*subscription)->topic);
        }
        free((*subscription)->topic);
        free(*subscription);
        *subscription = NULL;
    }
}

/**
 * @brief
 *
 * @param long_data
 * @return mqtt_v2_long_data_t*
 */
static mqtt_v2_long_data_t *mqtt_v2_free_long_data(mqtt_v2_long_data_t *long_data)
{
    if (long_data)
    {
        if (long_data->topic)
        {
            free(long_data->topic);
        }
        if (long_data->data)
        {
            free(long_data->data);
        }
        free(long_data);
    }
    return NULL;
}

/**
 * @brief
 *
 * @param long_data
 * @param event
 * @return mqtt_v2_long_data_t*
 */
static mqtt_v2_long_data_t *mqtt_v2_manage_long_data(mqtt_v2_long_data_t *long_data,
                                                     esp_mqtt_event_handle_t event)
{
    if (event->topic)
    {
        /* This is new data. Free any earlier data, if present. */
        mqtt_v2_free_long_data(long_data);
        long_data = calloc(1, sizeof(mqtt_v2_long_data_t));
        if (!long_data)
        {
            ESP_LOGE(TAG, "Could not allocate memory for mqtt_v2_long_data_t");
            return NULL;
        }
        long_data->data = MEM_CALLOC_EXTRAM(1, event->total_data_len);
        if (!long_data->data)
        {
            ESP_LOGE(TAG, "Could not allocate %d bytes for received data.", event->total_data_len);
            return mqtt_v2_free_long_data(long_data);
        }
        long_data->topic = strndup(event->topic, event->topic_len);
        if (!long_data->topic)
        {
            ESP_LOGE(TAG, "Could not allocate %d bytes for received topic.", event->topic_len);
            return mqtt_v2_free_long_data(long_data);
        }
    }
    if (long_data)
    {
        memcpy(long_data->data + event->current_data_offset, event->data, event->data_len);

        if ((event->current_data_offset + event->data_len) == event->total_data_len)
        {
            mqtt_v2_subscribe_callback(long_data->topic, strlen(long_data->topic),
                                       long_data->data, event->total_data_len);
            return mqtt_v2_free_long_data(long_data);
        }
    }
    return long_data;
}

/**
 * @brief
 *
 * @param event
 * @return esp_err_t
 */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    uint32_t event_id = event->event_id;

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected");
        /* Resubscribe to all topics after reconnection */
        for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++)
        {
            if (mqtt_data->subscriptions[i])
            {
                esp_mqtt_client_subscribe(event->client, mqtt_data->subscriptions[i]->topic, 1);
            }
        }
        esp_event_post(RMAKER_COMMON_EVENT, RMAKER_MQTT_EVENT_CONNECTED, NULL, 0, portMAX_DELAY);
        mqtt_v2_set_connection(1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT Disconnected. Will try reconnecting in a while...");
        mqtt_v2_set_connection(0);
        esp_event_post(RMAKER_COMMON_EVENT, RMAKER_MQTT_EVENT_DISCONNECTED, NULL, 0, portMAX_DELAY);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        esp_event_post(RMAKER_COMMON_EVENT, RMAKER_MQTT_EVENT_PUBLISHED, &event->msg_id, sizeof(event->msg_id), portMAX_DELAY);
        break;
#ifdef CONFIG_MQTT_REPORT_DELETED_MESSAGES
    case MQTT_EVENT_DELETED:
        ESP_LOGI(TAG, "MQTT_EVENT_DELETED, msg_id=%d", event->msg_id);
        esp_event_post(RMAKER_COMMON_EVENT, RMAKER_MQTT_EVENT_MSG_DELETED, &event->msg_id, sizeof(event->msg_id), portMAX_DELAY);
        break;
#endif /* CONFIG_MQTT_REPORT_DELETED_MESSAGES */
    case MQTT_EVENT_DATA:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        static mqtt_v2_long_data_t *long_data;
        /* Topic can be NULL, for data longer than the MQTT buffer */
        if (event->topic)
        {
            ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
        }
        ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
        if (event->data_len == event->total_data_len)
        {
            /* If long_data still exists, it means there was some issue getting the
             * long data, and so, it needs to be freed up.
             */
            if (long_data)
            {
                long_data = mqtt_v2_free_long_data(long_data);
            }
            event->data[event->data_len] = '\0';
            mqtt_v2_subscribe_callback(event->topic, event->topic_len, event->data, event->data_len);
        }
        else
        {
            long_data = mqtt_v2_manage_long_data(long_data, event);
        }
        break;
    }
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    return ESP_OK;
#endif
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/