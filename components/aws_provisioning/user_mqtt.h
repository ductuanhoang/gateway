/**
 * @file user_mqtt.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-07-28
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __USER_MQTT_H__
#define __USER_MQTT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include <stdbool.h>
#include "mqtt_client.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define MQTT_MAX_SUB_TOPIC 10
#define TEST_CONNECT_CLOUD_MQTT 0
#define AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS 10

#define MQTT_BUFFER_TOPIC_MAX_LENGHT 128
#define MQTT_BUFFER_MESSAGE_MAX_LENGHT 4000
    /****************************************************************************/
    /***        Type Definitions                                              ***/
    /****************************************************************************/

    typedef void (*pMqttCallback_t)(const char *topic, const char *data, int len);

    /**
     * @brief MQTT event callback function type
     */
    typedef struct
    {
        struct
        {
            char topic[100];
            void (*callback)(const char *topic, const char *data, int len);
        } sub[MQTT_MAX_SUB_TOPIC];
        uint8_t count;
    } sub_topic_t;

    /**
     * @brief
     *
     */
    typedef struct
    {
        uint16_t buffer_message_length;
        // bool isConnected;
        char mqtt_host_address[128];
        char mqtt_client_id[128];
        // char buffer_message[MQTT_BUFFER_MESSAGE_MAX_LENGHT];
        char *buffer_message;
        char buffer_topic[MQTT_BUFFER_TOPIC_MAX_LENGHT];
        int Port;
        esp_mqtt_client_handle_t mqtt_client;
        sub_topic_t sub_topic;
        pMqttCallback_t client_data[AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS];
    } user_mqtt_client_t;

    /****************************************************************************/
    /***         Exported global functions                                     ***/
    /****************************************************************************/
    /**
     * @brief Initialize MQTT client
     *
     * @param client
     * @param certificate_pem_crt
     * @param private_pem_key
     * @param aws_root_ca_pem
     * @return esp_err_t
     */
    int user_mqtt_init(user_mqtt_client_t *client,
                       const char *certificate_pem_crt,
                       const char *private_pem_key,
                       const char *aws_root_ca_pem);

    /**
     * @brief
     *
     * @param topic
     * @param data
     * @return init
     */
    int user_mqtt_client_publish(const char *pubTopic, const char *pubPayLoad, uint16_t payLoadLen);
    int user_mqtt_client_subscribe(const char *subTopic, pMqttCallback_t pMqttCallback);

    /**
     * @brief
     *
     * @param connected
     */
    void user_mqtt_setConnection(bool connected);

    /**
     * @brief
     *
     * @return bool
     */
    bool user_mqtt_getConnetion(void);

#ifdef __cplusplus
}
#endif

#endif /*__USER_MQTT_H__*/