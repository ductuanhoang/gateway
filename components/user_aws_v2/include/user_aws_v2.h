/**
 * @file user_aws_v2.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef USER_AWS_V2_H
#define USER_AWS_V2_H

#ifdef __cplusplus
extern "C"
{
#endif

    /****************************************************************************/
    /***        Include files                                                 ***/
    /****************************************************************************/
#include <stdint.h>
#include <esp_err.h>
#include <esp_event.h>
    /****************************************************************************/
    /***        Macro Definitions                                             ***/
    /****************************************************************************/

#define RMAKER_MQTT_QOS0 0
#define RMAKER_MQTT_QOS1 1

#define MAX_MQTT_SUBSCRIPTIONS 10
    /****************************************************************************/
    /***        Type Definitions                                              ***/
    /****************************************************************************/

    /** MQTT Connection parameters */
    typedef struct
    {
        /** MQTT Host */
        char *mqtt_host;
        /** Client ID */
        char *client_id;
        /** Client Certificate in DER format or NULL-terminated PEM format */
        char *client_cert;
        /** Client Certificate length */
        size_t client_cert_len;
        /** Client Key in DER format or NULL-terminated PEM format */
        char *client_key;
        /** Client Key length */
        size_t client_key_len;
        /** Server Certificate in DER format or NULL-terminated PEM format */
        char *server_cert;
        /** Server Certificate length */
        size_t server_cert_len;
        /** Pointer for digital signature peripheral context */
        void *ds_data;
        /** Mqtt port*/
        uint16_t mqtt_port;
    } esp_rmaker_mqtt_conn_params_t;

    /** MQTT Get Connection Parameters function prototype
     *
     * @return Pointer to \ref esp_rmaker_mqtt_conn_params_t on success.
     * @return NULL on failure.
     */
    typedef esp_rmaker_mqtt_conn_params_t *(*esp_rmaker_mqtt_get_conn_params_t)(void);

    /** MQTT Subscribe callback prototype
     *
     * @param[in] topic Topic on which the message was received
     * @param[in] payload Data received in the message
     * @param[in] payload_len Length of the data
     * @param[in] priv_data The private data passed during subscription
     */
    typedef void (*esp_rmaker_mqtt_subscribe_cb_t)(const char *topic, void *payload, size_t payload_len, void *priv_data);

    /** MQTT Init function prototype
     *
     * @param[in] conn_params The MQTT connection parameters. If NULL is passed, it should internally use the
     * \ref esp_rmaker_mqtt_get_conn_params call if registered.
     *
     * @return ESP_OK on success.
     * @return error in case of any error.
     */
    typedef esp_err_t (*esp_rmaker_mqtt_init_t)(esp_rmaker_mqtt_conn_params_t *conn_params);

    /** MQTT Deinit function prototype
     *
     * Call this function after MQTT has disconnected.
     */
    typedef void (*esp_rmaker_mqtt_deinit_t)(void);

    /** MQTT Connect function prototype
     *
     * Starts the connection attempts to the MQTT broker.
     * This should ideally be called after successful network connection.
     *
     * @return ESP_OK on success.
     * @return error in case of any error.
     */
    typedef esp_err_t (*esp_rmaker_mqtt_connect_t)(void);

    /** MQTT Disconnect function prototype
     *
     * Disconnects from the MQTT broker.
     *
     * @return ESP_OK on success.
     * @return error in case of any error.
     */
    typedef esp_err_t (*esp_rmaker_mqtt_disconnect_t)(void);

    /** MQTT Publish Message function prototype
     *
     * @param[in] topic The MQTT topic on which the message should be published.
     * @param[in] data Data to be published.
     * @param[in] data_len Length of the data.
     * @param[in] qos Quality of service for the message.
     * @param[out] msg_id If a non NULL pointer is passed, the id of the published message will be returned in this.
     *
     * @return ESP_OK on success.
     * @return error in case of any error.
     */
    typedef esp_err_t (*esp_rmaker_mqtt_publish_t)(const char *topic, void *data, size_t data_len, uint8_t qos, int *msg_id);

    /** MQTT Subscribe function prototype
     *
     * @param[in] topic The topic to be subscribed to.
     * @param[in] cb The callback to be invoked when a message is received on the given topic.
     * @param[in] qos Quality of service for the subscription.
     * @param[in] priv_data Optional private data to be passed to the callback.
     *
     * @return ESP_OK on success.
     * @return error in case of any error.
     */
    typedef esp_err_t (*esp_rmaker_mqtt_subscribe_t)(const char *topic, esp_rmaker_mqtt_subscribe_cb_t cb, uint8_t qos, void *priv_data);

    /** MQTT Unsubscribe function prototype
     *
     * @param[in] topic Topic from which to unsubscribe.
     *
     * @return ESP_OK on success.
     * @return error in case of any error.
     */
    typedef esp_err_t (*esp_rmaker_mqtt_unsubscribe_t)(const char *topic);

    /**
     * @brief
     *
     * @param conn_params
     * @return esp_err_t
     */
    esp_err_t mqtt_v2_init(esp_rmaker_mqtt_conn_params_t *conn_params);

    /**
     * @brief
     *
     * @return esp_err_t
     */
    esp_err_t mqtt_v2_connect(void);

    /**
     * @brief
     *
     * @return esp_err_t
     */
    esp_err_t mqtt_v2_disconnect(void);

    /**
     * @brief
     *
     * @return esp_err_t
     */
    esp_err_t mqtt_v2_reconnect(void);

    /**
     * @brief deinit the mqtt client
     *
     */
    void mqtt_v2_deinit(void);

    /**
     * @brief get status of mqtt connection
     *
     * @return int
     */
    int mqtt_v2_get_connection(void);

    /**
     * @brief set status of mqtt connection
     *
     * @param connection
     */
    void mqtt_v2_set_connection(int connection);

    /**
     * @brief subscribe to a topic
     *
     * @param topic
     * @param cb
     * @param qos
     * @param priv_data
     * @return esp_err_t
     */
    esp_err_t mqtt_v2_subscribe(const char *topic, esp_rmaker_mqtt_subscribe_cb_t cb, uint8_t qos, void *priv_data);

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
    esp_err_t mqtt_v2_publish(const char *topic, const char *data, size_t data_len, uint8_t qos, int *msg_id);

#ifdef __cplusplus
}
#endif

#endif /*USER_AWS_V2_H*/