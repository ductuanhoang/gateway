/**
 * @file mqtt_provisioning.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-08-10
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __USER_MQTT_PROVISIONING_H__
#define __USER_MQTT_PROVISIONING_H__

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

    typedef enum
    {
        E_PROVISION_ERR = 0,
        E_PROVISION_OK = 1,
    } e_provisioning_status;

    typedef enum
    {
        E_PROVISION_STATE_IDLE = 0,
        E_PROVISION_STATE_CREATE_CERTIFICATE,
        E_PROVISION_STATE_WAIT_CREATE_CERTIFICATE,
        E_PROVISION_STATE_CREATE_CERTIFICATE_SUCESS,
        E_PROVISION_STATE_REGISTER_THINGS,
        E_PROVISION_STATE_WAIT_REGISTER_THINGS,
        E_PROVISION_STATE_REGISTER_THING_ACCEPTED,
        E_PROVISION_STATE_DONE,
        E_PROVISION_STATE_ERROR
    } e_provisioning_state;

#define PROV_FLEET_TEMPLATE_NAME "Core"

// create certificate file topic
#define PROV_TOPIC_SUB_ACCEPTED "$aws/certificates/create/json/accepted"
#define PROV_TOPIC_SUB_REJECTED "$aws/certificates/create/json/rejected"
#define PROV_TOPIC_PUB_CREATE "$aws/certificates/create/json"

// register thing topic
#define PROV_TOPIC_PUB_REGISTER "$aws/provisioning-templates/" PROV_FLEET_TEMPLATE_NAME "/provision/json"
#define PROV_TOPIC_SUB_REGISTER_ACCEPTED "$aws/provisioning-templates/" PROV_FLEET_TEMPLATE_NAME "/provision/json/accepted"
#define PROV_TOPIC_SUB_REGISTER_REJECTED "$aws/provisioning-templates/" PROV_FLEET_TEMPLATE_NAME "/provision/json/rejected"

    void mqtt_provisioning_init(void);
    void mqtt_provision_task_start(void);

    int mqtt_provision_subscribe(void);
    void mqtt_provision_create_certificate(void);

    bool mqtt_provision_getProvisioned(void);
    void mqtt_provision_setProvisioned(bool provisioned);

    bool user_provision_get_certificate(void);

    extern char fleet_prov_certificate_Id[100];
    extern char fleet_prov_client_Cert[2000];
    extern char fleet_prov_client_Pkey[2000];
    extern char fleet_prov_cert_Owner[600];
    extern char fleet_prov_thing_Name[20];
#ifdef __cplusplus
}
#endif

#endif /*__USER_MQTT_PROVISIONING_H__*/