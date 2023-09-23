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

#ifndef USER_AWS_V2_EVENT_H
#define USER_AWS_V2_EVENT_H

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

    /****************************************************************************/
    /***        Type Definitions                                              ***/
    /****************************************************************************/
    /** ESP RainMaker Common Event Base */
    ESP_EVENT_DECLARE_BASE(RMAKER_COMMON_EVENT);

    typedef enum
    {
        /** Node reboot has been triggered. The associated event data is the time in seconds
         * (type: uint8_t) after which the node will reboot. Note that this time may not be
         * accurate as the events are received asynchronously.*/
        RMAKER_EVENT_REBOOT,
        /** Wi-Fi credentials reset. Triggered after calling esp_rmaker_wifi_reset() */
        RMAKER_EVENT_WIFI_RESET,
        /** Node reset to factory defaults. Triggered after calling esp_rmaker_factory_reset() */
        RMAKER_EVENT_FACTORY_RESET,
        /** Connected to MQTT Broker */
        RMAKER_MQTT_EVENT_CONNECTED,
        /** Disconnected from MQTT Broker */
        RMAKER_MQTT_EVENT_DISCONNECTED,
        /** MQTT message published successfully.
         * Event data will contain the message ID (integer) of published message.
         */
        RMAKER_MQTT_EVENT_PUBLISHED,
        /** POSIX Timezone Changed. Associated data would be NULL terminated POSIX Timezone
         * Eg. "PST8PDT,M3.2.0,M11.1.0" */
        RMAKER_EVENT_TZ_POSIX_CHANGED,
        /** Timezone Changed. Associated data would be NULL terminated Timezone.
         * Eg. "America/Los_Angeles"
         * Note that whenever this event is received, the RMAKER_EVENT_TZ_POSIX_CHANGED event
         * will also be received, but not necessarily vice versa.
         */
        RMAKER_EVENT_TZ_CHANGED,
        /**
         * MQTT message deleted from the outbox if the message couldn't have been sent and acknowledged.
         * Event data will contain the message ID (integer) of deleted message.
         * Valid only if CONFIG_MQTT_REPORT_DELETED_MESSAGES is enabled.
         */
        RMAKER_MQTT_EVENT_MSG_DELETED,
    } esp_rmaker_common_event_t;

#ifdef __cplusplus
}
#endif
    
#endif /*USER_AWS_V2_EVENT_H*/