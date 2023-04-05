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

#ifndef __AWS_MQTT_CONFIG_H__
#define __AWS_MQTT_CONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define AWS_IOT_ENDPOINT "a204koop60tgw8-ats.iot.eu-west-1.amazonaws.com" // AWS IoT Custom Endpoint Address
#define AWS_IOT_PORT 8883                                                 // AWS IoT Custom Endpoint Port
#define AWS_IOT_CLIENT_ID "esp32_1"                                       // AWS IoT Client ID
#ifdef __cplusplus
}
#endif

#endif /*__USER_MQTT_H__*/