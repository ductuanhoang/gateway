/**
 * @file ble_mesh.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-07-14
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

#include <stdio.h>
#include <string.h>
#include "ble_mesh.h"
#include "ble_mesh_message.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"
#include "provisioner_main.h"

#include "ble_mesh_example_init.h"
#include "ble_mesh_process.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "user_console.h"
#include "nvs_flash.h"
#include "settings.h"
/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define TAG "USER_BLE_MESH"

#define PREFIX_NODE_NAME "Sensor_"
#define CID_ESP 0x02E5

#define LED_OFF 0x00
#define LED_ON 0x01

#define PROV_OWN_ADDR 0x0002 // 0x0001
#define PROV_START_ADDR 0x0006
#define MSG_SEND_TTL 3
#define MSG_SEND_REL false
#define MSG_TIMEOUT 0
#define MSG_ROLE ROLE_PROVISIONER

#define COMP_DATA_PAGE_0 0x00

#define APP_KEY_IDX 0x0000
#define APP_KEY_OCTET 0x12

#define COMP_DATA_1_OCTET(msg, offset) (msg[offset])
#define COMP_DATA_2_OCTET(msg, offset) (msg[offset + 1] << 8 | msg[offset])

static uint16_t sensor_prop_id;

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static esp_ble_mesh_node_info_t nodes[CONFIG_BLE_MESH_MAX_PROV_NODES] = {
    [0 ...(CONFIG_BLE_MESH_MAX_PROV_NODES - 1)] = {
        .unicast = ESP_BLE_MESH_ADDR_UNASSIGNED,
        .elem_num = 0,
        .onoff = LED_OFF,
        .sensor = 0,
    }};
static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN];

static esp_ble_mesh_client_t config_client;
static esp_ble_mesh_client_t onoff_client;
static esp_ble_mesh_client_t sensor_client;

static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_CFG_CLI(&config_client),            // to add the Configuration Client model to the `root_models` array.
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(NULL, &onoff_client), // Generic OnOff Client Model
    ESP_BLE_MESH_MODEL_SENSOR_CLI(NULL, &sensor_client),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = {
    .prov_uuid = dev_uuid,
    .prov_unicast_addr = PROV_OWN_ADDR,
    .prov_start_address = PROV_START_ADDR,
    .prov_attention = 0x00,
    .prov_algorithm = 0x00,
    .prov_pub_key_oob = 0x00,
    .prov_static_oob_val = NULL,
    .prov_static_oob_len = 0x00,
    .flags = 0x00,
    .iv_index = 0x00,
};
esp_ble_mesh_key_t prov_key;

static bool ble_mesh_prov_enable_status = false;
static ble_mesh_prov_complete_t prov_complete_service_callback;
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

static esp_err_t ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
                                         esp_ble_mesh_node_info_t *node,
                                         esp_ble_mesh_model_t *model, uint32_t opcode);

// callback function
static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                     esp_ble_mesh_prov_cb_param_t *param);

static void ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
                                      esp_ble_mesh_cfg_client_cb_param_t *param);

static void ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                       esp_ble_mesh_generic_client_cb_param_t *param);

static void ble_mesh_sensor_client_cb(esp_ble_mesh_sensor_client_cb_event_t event,
                                      esp_ble_mesh_sensor_client_cb_param_t *param);
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/

void user_ble_mesh_prov_complete_register(ble_mesh_prov_complete_t callback)
{
    if (callback != NULL)
        prov_complete_service_callback = callback;
}

/**
 * @brief
 *
 * @return int
 */
int user_ble_mesh_init(void)
{
    esp_err_t err = ESP_OK;
    uint8_t match[2] = {0x32, 0x10};
    uint8_t key[16] = {0};
    err = bluetooth_init();
    if (err)
    {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    ble_mesh_get_dev_uuid(dev_uuid);

    prov_key.net_idx = 0x0000;
    prov_key.app_idx = APP_KEY_IDX;
    memset(prov_key.app_key, APP_KEY_OCTET, sizeof(prov_key.app_key));

    // esp_ble_mesh_prov_data_info_t info = {
    //     .net_idx = 0x0000,
    //     .flag = BIT(0),
    // };
    // err = esp_ble_mesh_provisioner_add_local_net_key(key, prov_key.net_idx);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Failed to set esp_ble_mesh_provisioner_add_local_net_key (err %d)", err);
    //     return err;
    // }
    // err = esp_ble_mesh_provisioner_set_prov_data_info(&info);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Failed to set esp_ble_mesh_provisioner_set_prov_data_info (err %d)", err);
    //     return err;
    // }

    esp_ble_mesh_register_prov_callback(ble_mesh_provisioning_cb);           // register BLE Mesh provisioning callback.
    esp_ble_mesh_register_config_client_callback(ble_mesh_config_client_cb); // is the register function for Config Client Model.

    esp_ble_mesh_register_generic_client_callback(ble_mesh_generic_client_cb); // Register BLE Mesh Generic Client Model callback.
    esp_ble_mesh_register_sensor_client_callback(ble_mesh_sensor_client_cb);   // Register BLE Mesh Sensor Client Model callback.

    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
        return err;
    }

    err = esp_ble_mesh_provisioner_set_dev_uuid_match(match, sizeof(match), 0x0, false);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set matching device uuid (err %d)", err);
        return err;
    }

    err = esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable mesh provisioner (err %d)", err);
        return err;
    }

    // add local app key if not already
    err = esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add local AppKey (err %d)", err);
        return err;
    }
    // const uint8_t *local_app_key;
    // local_app_key = esp_ble_mesh_provisioner_get_local_app_key(prov_key.net_idx, prov_key.app_idx);
    // if (local_app_key == NULL)
    // {
    //     ESP_LOGI(TAG, "Failed to get local app key (err %d)", err);
    //     ESP_LOGI(TAG, "Add local app key");
    //     err = esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);
    //     if (err != ESP_OK)
    //     {
    //         ESP_LOGE(TAG, "Failed to add local AppKey (err %d)", err);
    //         return err;
    //     }
    // }

    // err = esp_ble_mesh_provisioner_add_local_net_key(prov_key.net_key, 0x01);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Failed to add local NetKey (err %d)", err);
    //     return err;
    // }

    // err = esp_ble_mesh_provisioner_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Failed to díable mesh provisioner (err %d)", err);
    //     return err;
    // }

    ESP_LOGI(TAG, "BLE Mesh Provisioner initialized");
    return err;
}

/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

/**
 * @brief store mesh node information
 *
 * @param uuid
 * @param unicast
 * @param elem_num
 * @param onoff_state
 * @return esp_err_t
 */
static esp_err_t ble_mesh_store_node_info(const uint8_t uuid[16], uint16_t unicast,
                                          uint8_t elem_num, uint8_t onoff_state)
{
    int i;

    if (!uuid || !ESP_BLE_MESH_ADDR_IS_UNICAST(unicast))
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Judge if the device has been provisioned before */
    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        if (!memcmp(nodes[i].uuid, uuid, 16))
        {
            ESP_LOGW(TAG, "%s: reprovisioned device 0x%04x", __func__, unicast);
            nodes[i].unicast = unicast;
            nodes[i].elem_num = elem_num;
            nodes[i].onoff = onoff_state;
            return ESP_OK;
        }
    }

    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        if (nodes[i].unicast == ESP_BLE_MESH_ADDR_UNASSIGNED)
        {
            memcpy(nodes[i].uuid, uuid, 16);
            nodes[i].unicast = unicast;
            nodes[i].elem_num = elem_num;
            nodes[i].onoff = onoff_state;
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

/**
 * @brief get node from unicast
 *
 * @param unicast
 * @return esp_ble_mesh_node_info_t*
 */
static esp_ble_mesh_node_info_t *ble_mesh_get_node_info(uint16_t unicast)
{
    int i;

    if (!ESP_BLE_MESH_ADDR_IS_UNICAST(unicast))
    {
        return NULL;
    }

    for (i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        if (nodes[i].unicast <= unicast &&
            nodes[i].unicast + nodes[i].elem_num > unicast)
        {
            return &nodes[i];
        }
    }

    return NULL;
}

/**
 * @brief set common message from unicast
 *
 * @param common
 * @param node
 * @param model
 * @param opcode
 * @return esp_err_t
 */
static esp_err_t ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
                                         esp_ble_mesh_node_info_t *node,
                                         esp_ble_mesh_model_t *model, uint32_t opcode)
{
    ESP_LOGI(TAG, "ble_mesh_set_msg_common called");
    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = prov_key.net_idx;
    common->ctx.app_idx = prov_key.app_idx;
    common->ctx.addr = node->unicast;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->ctx.send_rel = MSG_SEND_REL;
    common->msg_timeout = MSG_TIMEOUT;
    common->msg_role = MSG_ROLE;
    ESP_LOGI(TAG, "example_ble_mesh_set_msg_common common->ctx.addr: %x", common->ctx.addr);
    ESP_LOGI(TAG, "example_ble_mesh_set_msg_common node->unicast: %x", node->unicast);
    return ESP_OK;
}

/**
 * @brief
 *
 * @param node_idx
 * @param uuid
 * @param unicast
 * @param elem_num
 * @param net_idx
 * @return esp_err_t
 */
static esp_err_t prov_complete(int node_idx, const esp_ble_mesh_octet16_t uuid,
                               uint16_t unicast, uint8_t elem_num, uint16_t net_idx)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_cfg_client_get_state_t get_state = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    char node_name[30] = {0};
    int err;

    ESP_LOGI(TAG, "prov_complete node index: 0x%x, unicast address: 0x%02x, element num: %d, netkey index: 0x%02x",
             node_idx, unicast, elem_num, net_idx);
    ESP_LOGI(TAG, "device uuid: %s", bt_hex(uuid, 8));

    sprintf(node_name, "%s%s", PREFIX_NODE_NAME, (bt_hex(uuid, 8)));

    err = esp_ble_mesh_provisioner_set_node_name(node_idx, node_name);
    if (err)
    {
        ESP_LOGE(TAG, "%s: Set node name failed", __func__);
        return ESP_FAIL;
    }
    else
    {
        // TODO: nvs save ID
    }
    err = ble_mesh_store_node_info(uuid, unicast, elem_num, LED_OFF);
    if (err)
    {
        ESP_LOGE(TAG, "%s: Store node info failed", __func__);
        return ESP_FAIL;
    }
    else
    {
        // TODO: nvs save data
    }

    node = ble_mesh_get_node_info(unicast);
    if (!node)
    {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return ESP_FAIL;
    }
    ble_mesh_add_proved_devices(node_name);

    ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
    get_state.comp_data_get.page = COMP_DATA_PAGE_0;
    err = esp_ble_mesh_config_client_get_state(&common, &get_state);
    return ESP_OK;
}

static void prov_link_open(esp_ble_mesh_prov_bearer_t bearer)
{
    ESP_LOGI(TAG, "%s link open", bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
}

static void prov_link_close(esp_ble_mesh_prov_bearer_t bearer, uint8_t reason)
{
    ESP_LOGI(TAG, "%s link close, reason 0x%02x",
             bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", reason);
}

/**
 * @brief scan the ble unprovision device
 *
 * @param dev_uuid
 * @param addr
 * @param addr_type
 * @param oob_info
 * @param adv_type
 * @param bearer
 */
static int recv_unprov_adv_pkt(uint8_t dev_uuid[16], uint8_t addr[BD_ADDR_LEN],
                               esp_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
                               uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer)
{
    esp_ble_mesh_unprov_dev_add_t add_dev = {0};
    int err;

    memcpy(add_dev.addr, addr, BD_ADDR_LEN);
    add_dev.addr_type = (uint8_t)addr_type;
    memcpy(add_dev.uuid, dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
    add_dev.oob_info = oob_info;
    add_dev.bearer = (uint8_t)bearer;

    // ESP_LOGI(TAG, "new device scan: %s", bt_hex(add_dev.addr, BD_ADDR_LEN));
    err = ble_mesh_compare_new_prov(add_dev);
    if (err == 0) // new device founded
    {
    }
    return err;
}

/**
 * @brief provsioner enable or disable
 *
 * @param enable:
 * @return int
 */
int ble_mesh_provisioner_prov_enable(uint8_t enable)
{
    ESP_LOGD(TAG, "enter %s\n", __func__);
    esp_err_t result = ESP_OK;
    if (enable)
    {
        ble_mesh_prov_enable_status = true;
        result = esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed prov enable (err %d)", result);
            return result;
        }
        // result = esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);
        // if (result != ESP_OK)
        // {
        //     ESP_LOGE(TAG, "Failed to add local AppKey (err %d)", result);
        //     return result;
        // }
    }
    else
    {
        ble_mesh_prov_enable_status = false;
        result = esp_ble_mesh_provisioner_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    }
    return result;
}

/**
 * @brief returns the provsion enabled status
 *
 * @return int
 */
int ble_mesh_provisioner_get_prov_enabled(void)
{
    return ble_mesh_prov_enable_status;
}
/**
 * @brief
 *
 * @return int
 */
int ble_mesh_node_reset(void)
{
    esp_err_t err;
    ESP_LOGD(TAG, "enter %s\n", __func__);

    err = esp_ble_mesh_node_local_reset();

    ESP_LOGD(TAG, "exit %s\n", __func__);
    return err;
}
/**
 * @brief provsion new device with node index in scan table
 *
 * @param add_dev: unprovion add
 * @return int
 */
int user_ble_mesh_add_unprov_node(uint8_t node_index)
{
    ESP_LOGD(TAG, "enter %s \n", __func__);
    int err = -1;
    ble_mesh_provision_device_index(node_index);

    return err;
}

int user_ble_mesh_delete_node(esp_ble_mesh_unprov_dev_add_t add_dev)
{
    ESP_LOGD(TAG, "enter %s \n", __func__);
    int err = -1;
    /* Note: If unprovisioned device adv packets have not been received, we should not add
             device with ADD_DEV_START_PROV_NOW_FLAG set. */
    err = esp_ble_mesh_provisioner_delete_dev(&add_dev);
    if (err)
    {
        ESP_LOGE(TAG, "%s: Add unprovisioned device into queue failed", __func__);
    }

    return err;
}
/**
 * @brief
 *
 * @param event
 * @param param
 */
static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                     esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT:
        // ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT");
        recv_unprov_adv_pkt(param->provisioner_recv_unprov_adv_pkt.dev_uuid, param->provisioner_recv_unprov_adv_pkt.addr,
                            param->provisioner_recv_unprov_adv_pkt.addr_type, param->provisioner_recv_unprov_adv_pkt.oob_info,
                            param->provisioner_recv_unprov_adv_pkt.adv_type, param->provisioner_recv_unprov_adv_pkt.bearer);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
        prov_link_open(param->provisioner_prov_link_open.bearer);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
        prov_link_close(param->provisioner_prov_link_close.bearer, param->provisioner_prov_link_close.reason);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
        prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid,
                      param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num,
                      param->provisioner_prov_complete.netkey_idx);
        break;
    case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT:
    {
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
        if (param->provisioner_set_node_name_comp.err_code == ESP_OK)
        {
            // b_prov_complete = true;
            // TODO: set event complete provision
            const char *name = NULL;
            name = esp_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index);
            if (!name)
            {
                ESP_LOGE(TAG, "Get node name failed");
                return;
            }
            ESP_LOGI(TAG, "Node %d name is: %s", param->provisioner_set_node_name_comp.node_index, name);
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT:
    {
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
        if (param->provisioner_add_app_key_comp.err_code == ESP_OK)
        {
            // esp_err_t err = 0;
            prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
            ESP_LOGI(TAG, "***** prov_key.app_idx = 0x%02x", prov_key.app_idx);
            // TODO: add local app key to both sensor and on/off services of this device
            user_ble_mesh_process_event(E_USER_BLE_MESH_BIDING_APP_KEY); // TODO: done
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT, err_code %d", param->provisioner_store_node_comp_data_comp.err_code);
        break;
    // add new event listener
    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_NET_KEY_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_NET_KEY_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
        break;
    case ESP_BLE_MESH_HEARTBEAT_MESSAGE_RECV_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_HEARTBEAT_MESSAGE_RECV_EVT");
        break;
    default:
        ESP_LOGI(TAG, "Unknown event type %d", event);
        break;
    }

    return;
}

/**
 * @brief binding sensor data with app index
 *
 * @param app_idx
 */
esp_err_t ble_mesh_binding_app_key_model_sensor(uint16_t prov_app_idx)
{
    ESP_LOGD(TAG, " enter %s\n", __func__);
    esp_err_t err = 0;
    err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_app_idx,
                                                               ESP_BLE_MESH_MODEL_ID_SENSOR_CLI, ESP_BLE_MESH_CID_NVAL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Provisioner bind local model appkey failed");
        return;
    }

    return err;
}

/**
 * @brief binding sensor on/off with app index
 *
 * @param app_idx
 */
esp_err_t ble_mesh_binding_app_key_model_onoff(uint16_t prov_app_idx)
{
    ESP_LOGD(TAG, " enter %s\n", __func__);
    esp_err_t err = 0;
    err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_app_idx,
                                                               ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI, ESP_BLE_MESH_CID_NVAL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Provisioner bind local model appkey failed");
        return;
    }

    return err;
}

/**
 * @brief
 *
 * @return esp_err_t
 */
esp_err_t ble_mesh_model_app_key_add(uint16_t addr)
{
    ESP_LOGD(TAG, " enter %s\n", __func__);
    esp_err_t err = 0;
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_cfg_client_set_state_t set_state = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    node = ble_mesh_get_node_info(addr);
    ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
    set_state.app_key_add.net_idx = prov_key.net_idx;
    set_state.app_key_add.app_idx = prov_key.app_idx;
    memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);

    err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    if (err)
    {
        ESP_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
        return;
    }

    return err;
}
/**
 * @brief
 *
 * @return esp_err_t
 */
esp_err_t ble_mesh_binding_model_sensor_service(uint16_t addr)
{
    ESP_LOGD(TAG, " enter %s\n", __func__);
    esp_ble_mesh_cfg_client_set_state_t set_state = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t *node = NULL;

    esp_err_t err = 0;
    node = ble_mesh_get_node_info(addr);

    ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
    // set_state.model_app_bind.element_addr = node->unicast + 1; TODO: check
    set_state.model_app_bind.element_addr = node->unicast;
    set_state.model_app_bind.model_app_idx = prov_key.app_idx;
    set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_SENSOR_SETUP_SRV;
    set_state.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
    err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    return err;
}

/**
 * @brief
 *  bind a model 0x1000
 * @return esp_err_t
 */
esp_err_t ble_mesh_binding_model_onoff_service(uint16_t addr)
{
    ESP_LOGD(TAG, " enter %s\n", __func__);
    esp_ble_mesh_cfg_client_set_state_t set_state = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t *node = NULL;

    esp_err_t err = 0;
    node = ble_mesh_get_node_info(addr);

    ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
    // set_state.model_app_bind.element_addr = node->unicast + 1; TODO: check
    set_state.model_app_bind.element_addr = node->unicast;
    set_state.model_app_bind.model_app_idx = prov_key.app_idx;
    set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
    set_state.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
    err = esp_ble_mesh_config_client_set_state(&common, &set_state);
    return err;
}

/**
 * @brief
 *
 * @param data
 * @param length
 */
static void ble_mesh_parse_node_comp_data(const uint8_t *data, uint16_t length)
{
    uint16_t cid, pid, vid, crpl, feat;
    uint16_t loc, model_id, company_id;
    uint8_t nums, numv;
    uint16_t offset;
    int i;

    cid = COMP_DATA_2_OCTET(data, 0);
    pid = COMP_DATA_2_OCTET(data, 2);
    vid = COMP_DATA_2_OCTET(data, 4);
    crpl = COMP_DATA_2_OCTET(data, 6);
    feat = COMP_DATA_2_OCTET(data, 8);
    offset = 10;

    ESP_LOGI(TAG, "********************** Composition Data Start **********************");
    ESP_LOGI(TAG, "* CID 0x%04x, PID 0x%04x, VID 0x%04x, CRPL 0x%04x, Features 0x%04x *", cid, pid, vid, crpl, feat);
    for (; offset < length;)
    {
        loc = COMP_DATA_2_OCTET(data, offset);
        nums = COMP_DATA_1_OCTET(data, offset + 2);
        numv = COMP_DATA_1_OCTET(data, offset + 3);
        offset += 4;
        ESP_LOGI(TAG, "* Loc 0x%04x, NumS 0x%02x, NumV 0x%02x *", loc, nums, numv);
        for (i = 0; i < nums; i++)
        {
            model_id = COMP_DATA_2_OCTET(data, offset);
            ESP_LOGI(TAG, "* SIG Model ID 0x%04x *", model_id);
            offset += 2;
        }
        for (i = 0; i < numv; i++)
        {
            company_id = COMP_DATA_2_OCTET(data, offset);
            model_id = COMP_DATA_2_OCTET(data, offset + 2);
            ESP_LOGI(TAG, "* Vendor Model ID 0x%04x, Company ID 0x%04x *", model_id, company_id);
            offset += 4;
        }
    }
    ESP_LOGI(TAG, "*********************** Composition Data End ***********************");
}

/**
 * @brief
 *
 * @param event
 * @param param
 */
static void ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
                                      esp_ble_mesh_cfg_client_cb_param_t *param)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    static uint16_t wait_model_id, wait_cid;
    uint32_t opcode;
    uint16_t addr;
    int err;

    opcode = param->params->opcode;
    addr = param->params->ctx.addr;

    ESP_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
             __func__, param->error_code, event, param->params->ctx.addr, opcode);

    if (param->error_code)
    {
        ESP_LOGE(TAG, "Send config client message failed, opcode 0x%04x", opcode);
        return;
    }

    node = ble_mesh_get_node_info(addr);
    if (!node)
    {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return;
    }

    switch (event)
    {
    case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
        switch (opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET:
        {
            ESP_LOGI(TAG, "composition data %s", bt_hex(param->status_cb.comp_data_status.composition_data->data, param->status_cb.comp_data_status.composition_data->len));
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};

            ble_mesh_parse_node_comp_data(param->status_cb.comp_data_status.composition_data->data,
                                          param->status_cb.comp_data_status.composition_data->len);
            ESP_LOGI(TAG, "**** %s:%d, heap: %d, %d", __func__, __LINE__, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

            // err = esp_ble_mesh_provisioner_store_node_comp_data(param->params->ctx.addr,
            //                                                     param->status_cb.comp_data_status.composition_data->data,
            //                                                     param->status_cb.comp_data_status.composition_data->len);
            // if (err != ESP_OK)
            // {
            //     ESP_LOGE(TAG, "Failed to store node composition data");
            //     break;
            // }

            // TODO: check bind network
            // ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            // set_state.app_key_add.net_idx = prov_key.net_idx;
            // set_state.app_key_add.app_idx = prov_key.app_idx;
            // memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);

            // err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            // if (err)
            // {
            //     ESP_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
            //     return;
            // }
            ble_mesh_model_app_key_add(addr);
            break;
        }
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
        switch (opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
        {
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            // set_state.model_app_bind.element_addr = node->unicast + 1; TODO: check
            set_state.model_app_bind.element_addr = node->unicast;
            set_state.model_app_bind.model_app_idx = prov_key.app_idx;
            set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_SENSOR_SRV;
            set_state.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
            ESP_LOGI(TAG, "**** %s:%d, heap: %d, %d", __func__, __LINE__, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err)
            {
                ESP_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
                return;
            }
            wait_model_id = ESP_BLE_MESH_MODEL_ID_SENSOR_SRV;
            wait_cid = ESP_BLE_MESH_CID_NVAL;
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
        {
            ESP_LOGI(TAG, "ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");

            if (param->status_cb.model_app_status.model_id == ESP_BLE_MESH_MODEL_ID_SENSOR_SRV &&
                param->status_cb.model_app_status.company_id == ESP_BLE_MESH_CID_NVAL)
            {
                ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
                ble_mesh_binding_model_onoff_service(addr);
                // esp_ble_mesh_cfg_client_set_state_t set = {0};
                // ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
                // set.model_app_bind.element_addr = node->unicast;
                // set.model_app_bind.model_app_idx = prov_key.app_idx;
                // set.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_SENSOR_SETUP_SRV;
                // // set.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
                // set.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
                // err = esp_ble_mesh_config_client_set_state(&common, &set);
                // if (err)
                // {
                //     ESP_LOGE(TAG, "Failed to send Config Model App Bind");
                //     return;
                // }
                // wait_model_id = ESP_BLE_MESH_MODEL_ID_SENSOR_SETUP_SRV;
                wait_model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
                wait_cid = ESP_BLE_MESH_CID_NVAL;
            }
            else if (param->status_cb.model_app_status.model_id == ESP_BLE_MESH_MODEL_ID_SENSOR_SETUP_SRV &&
                     param->status_cb.model_app_status.company_id == ESP_BLE_MESH_CID_NVAL)
            {
                ESP_LOGW(TAG, "Provision and config sensor service successfully");
            }
            else if (param->status_cb.model_app_status.model_id == ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV &&
                     param->status_cb.model_app_status.company_id == ESP_BLE_MESH_CID_NVAL)
            {
                ESP_LOGW(TAG, "Provision and config on/off services successfully");
                ESP_LOGI(TAG, "***** prov_key.app_idx = 0x%04x", prov_key.app_idx);
                ESP_LOGI(TAG, "***** prov_key.net_idx = 0x%04x", prov_key.net_idx);
                if (prov_complete_service_callback != NULL)
                    prov_complete_service_callback(NULL);
            }

            if (param->status_cb.model_app_status.model_id == ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV)
            {
                esp_ble_mesh_generic_client_get_state_t get_state = {0};
                ble_mesh_set_msg_common(&common, node, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET);
                err = esp_ble_mesh_generic_client_get_state(&common, &get_state);
                if (err)
                {
                    ESP_LOGE(TAG, "%s: Generic OnOff Get failed", __func__);
                    return;
                }
            }
            break;
        }
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
        switch (opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS:
            ESP_LOG_BUFFER_HEX("composition data %s", param->status_cb.comp_data_status.composition_data->data,
                               param->status_cb.comp_data_status.composition_data->len);
            break;
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_STATUS:
            break;
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
        switch (opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET:
        {
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET TIME OUT");
            esp_ble_mesh_cfg_client_get_state_t get_state = {0};
            ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
            get_state.comp_data_get.page = COMP_DATA_PAGE_0;
            err = esp_ble_mesh_config_client_get_state(&common, &get_state);
            if (err)
            {
                ESP_LOGE(TAG, "%s: Config Composition Data Get failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
        {
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set_state.app_key_add.net_idx = prov_key.net_idx;
            set_state.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err)
            {
                ESP_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
        {
            ESP_LOGI(TAG, "ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set_state.model_app_bind.element_addr = node->unicast;
            set_state.model_app_bind.model_app_idx = prov_key.app_idx;
            set_state.model_app_bind.model_id = wait_model_id;
            set_state.model_app_bind.company_id = wait_cid;
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err)
            {
                ESP_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
                return;
            }
            break;
        }
        default:
            break;
        }
        break;
    default:
        ESP_LOGE(TAG, "Not a config client status message event");
        break;
    }
}

/**
 * @brief
 *
 * @param event
 * @param param
 */
static void ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                       esp_ble_mesh_generic_client_cb_param_t *param)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    uint32_t opcode;
    uint16_t addr;
    int err;

    opcode = param->params->opcode;
    addr = param->params->ctx.addr;

    ESP_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
             __func__, param->error_code, event, param->params->ctx.addr, opcode);

    if (param->error_code)
    {
        ESP_LOGE(TAG, "Send generic client message failed, opcode 0x%04x", opcode);
        return;
    }

    node = ble_mesh_get_node_info(addr);
    if (!node)
    {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return;
    }

    ESP_LOGI(TAG, "node info uuid     : %s", bt_hex(node->uuid, 16));
    ESP_LOGI(TAG, "node info unicast  : %d", node->unicast);
    ESP_LOGI(TAG, "node info elem_num : %d", node->elem_num);
    ESP_LOGI(TAG, "node info onoff    : %d", node->onoff);

    switch (event)
    {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        switch (opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
        {
            // esp_ble_mesh_generic_client_set_state_t set_state = {0};
            // node->onoff = param->status_cb.onoff_status.present_onoff;
            // ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET 1 get onoff: 0x%02x", node->onoff);
            // /* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
            // ble_mesh_set_msg_common(&common, node, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
            // set_state.onoff_set.op_en = false;
            // set_state.onoff_set.onoff = common_state_onoff;
            // set_state.onoff_set.tid = 0;
            // ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET 1 set onoff: 0x%02x", set_state.onoff_set.onoff);
            // ESP_LOGI(TAG, "common.net_idx 0x%02x", common.ctx.net_idx);
            // ESP_LOGI(TAG, "common.app_idx 0x%02x", common.ctx.app_idx);
            // err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
            // if (err)
            // {
            //     ESP_LOGE(TAG, "%s: Generic OnOff Set failed", __func__);
            //     return;
            // }
            // TODO: add this function to set the state
            // ble_mesh_send_gen_onoff_set(state, addr);
            break;
        }
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        switch (opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:

            node->onoff = param->status_cb.onoff_status.present_onoff;
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET present_onoff: 0x%02x", param->status_cb.onoff_status.present_onoff);
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET target_onoff: 0x%02x", param->status_cb.onoff_status.target_onoff);
            break;
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
        /* If failed to receive the responses, these messages will be resend */
        switch (opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
        {
            // esp_ble_mesh_generic_client_get_state_t get_state = {0};
            // example_ble_mesh_set_msg_common(&common, node, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET);
            // err = esp_ble_mesh_generic_client_get_state(&common, &get_state);
            // if (err)
            // {
            //     ESP_LOGE(TAG, "%s: Generic OnOff Get failed", __func__);
            //     return;
            // }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
        {
            esp_ble_mesh_generic_client_set_state_t set_state = {0};
            node->onoff = param->status_cb.onoff_status.present_onoff;
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET 2 onoff: 0x%02x", node->onoff);
            // example_ble_mesh_set_msg_common(&common, node, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
            // set_state.onoff_set.op_en = false;
            // set_state.onoff_set.onoff = !node->onoff;
            // set_state.onoff_set.tid = 0;
            // err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
            // if (err)
            // {
            //     ESP_LOGE(TAG, "%s: Generic OnOff Set failed", __func__);
            //     return;
            // }
            break;
        }
        default:
            break;
        }
        break;
    default:
        ESP_LOGE(TAG, "Not a generic client status message event");
        break;
    }
}

/**
 * @brief function used by sensor module
 *
 * @param event
 * @param param
 */
static void ble_mesh_sensor_client_cb(esp_ble_mesh_sensor_client_cb_event_t event,
                                      esp_ble_mesh_sensor_client_cb_param_t *param)
{
    esp_ble_mesh_node_t *node = NULL;

    ESP_LOGI(TAG, "Sensor client, event %u, addr 0x%04x", event, param->params->ctx.addr);

    if (param->error_code)
    {
        ESP_LOGE(TAG, "Send sensor client message failed (err %d)", param->error_code);
        return;
    }

    node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
    if (!node)
    {
        ESP_LOGE(TAG, "Node 0x%04x not exists", param->params->ctx.addr);
        return;
    }

    switch (event)
    {
    case ESP_BLE_MESH_SENSOR_CLIENT_GET_STATE_EVT:
        switch (param->params->opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_SENSOR_DESCRIPTOR_GET:
            ESP_LOGI(TAG, "Sensor Descriptor Status, opcode 0x%04x", param->params->ctx.recv_op);
            if (param->status_cb.descriptor_status.descriptor->len != ESP_BLE_MESH_SENSOR_SETTING_PROPERTY_ID_LEN &&
                param->status_cb.descriptor_status.descriptor->len % ESP_BLE_MESH_SENSOR_DESCRIPTOR_LEN)
            {
                ESP_LOGE(TAG, "Invalid Sensor Descriptor Status length %d", param->status_cb.descriptor_status.descriptor->len);
                return;
            }
            if (param->status_cb.descriptor_status.descriptor->len)
            {
                ESP_LOG_BUFFER_HEX("Sensor Descriptor", param->status_cb.descriptor_status.descriptor->data,
                                   param->status_cb.descriptor_status.descriptor->len);
                /* If running with sensor server example, sensor client can get two Sensor Property IDs.
                 * Currently we use the first Sensor Property ID for the following demonstration.
                 */
                sensor_prop_id = param->status_cb.descriptor_status.descriptor->data[1] << 8 |
                                 param->status_cb.descriptor_status.descriptor->data[0];
            }
            break;
        case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_GET:
            ESP_LOGI(TAG, "Sensor Cadence Status, opcode 0x%04x, Sensor Property ID 0x%04x",
                     param->params->ctx.recv_op, param->status_cb.cadence_status.property_id);
            ESP_LOG_BUFFER_HEX("Sensor Cadence", param->status_cb.cadence_status.sensor_cadence_value->data,
                               param->status_cb.cadence_status.sensor_cadence_value->len);
            break;
        case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTINGS_GET:
            ESP_LOGI(TAG, "Sensor Settings Status, opcode 0x%04x, Sensor Property ID 0x%04x",
                     param->params->ctx.recv_op, param->status_cb.settings_status.sensor_property_id);
            ESP_LOG_BUFFER_HEX("Sensor Settings", param->status_cb.settings_status.sensor_setting_property_ids->data,
                               param->status_cb.settings_status.sensor_setting_property_ids->len);
            break;
        case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_GET:
            ESP_LOGI(TAG, "Sensor Setting Status, opcode 0x%04x, Sensor Property ID 0x%04x, Sensor Setting Property ID 0x%04x",
                     param->params->ctx.recv_op, param->status_cb.setting_status.sensor_property_id,
                     param->status_cb.setting_status.sensor_setting_property_id);
            if (param->status_cb.setting_status.op_en)
            {
                ESP_LOGI(TAG, "Sensor Setting Access 0x%02x", param->status_cb.setting_status.sensor_setting_access);
                ESP_LOG_BUFFER_HEX("Sensor Setting Raw", param->status_cb.setting_status.sensor_setting_raw->data,
                                   param->status_cb.setting_status.sensor_setting_raw->len);
            }
            break;
        case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
            ESP_LOGI(TAG, "Sensor Status, opcode 0x%04x", param->params->ctx.recv_op);
            if (param->status_cb.sensor_status.marshalled_sensor_data->len)
            {
                ESP_LOG_BUFFER_HEX("Sensor Data", param->status_cb.sensor_status.marshalled_sensor_data->data,
                                   param->status_cb.sensor_status.marshalled_sensor_data->len);
                uint8_t *data = param->status_cb.sensor_status.marshalled_sensor_data->data;
                uint16_t length = 0;
                for (; length < param->status_cb.sensor_status.marshalled_sensor_data->len;)
                {
                    uint8_t fmt = ESP_BLE_MESH_GET_SENSOR_DATA_FORMAT(data);
                    uint8_t data_len = ESP_BLE_MESH_GET_SENSOR_DATA_LENGTH(data, fmt);
                    uint16_t prop_id = ESP_BLE_MESH_GET_SENSOR_DATA_PROPERTY_ID(data, fmt);
                    uint8_t mpid_len = (fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ? ESP_BLE_MESH_SENSOR_DATA_FORMAT_A_MPID_LEN : ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN);
                    ESP_LOGI(TAG, "Format %s, length 0x%02x, Sensor Property ID 0x%04x",
                             fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ? "A" : "B", data_len, prop_id);
                    if (data_len != ESP_BLE_MESH_SENSOR_DATA_ZERO_LEN)
                    {
                        ESP_LOG_BUFFER_HEX("Sensor Data", data + mpid_len, data_len + 1);
                        length += mpid_len + data_len + 1;
                        data += mpid_len + data_len + 1;
                    }
                    else
                    {
                        length += mpid_len;
                        data += mpid_len;
                    }
                }
            }
            break;
        case ESP_BLE_MESH_MODEL_OP_SENSOR_COLUMN_GET:
            ESP_LOGI(TAG, "Sensor Column Status, opcode 0x%04x, Sensor Property ID 0x%04x",
                     param->params->ctx.recv_op, param->status_cb.column_status.property_id);
            ESP_LOG_BUFFER_HEX("Sensor Column", param->status_cb.column_status.sensor_column_value->data,
                               param->status_cb.column_status.sensor_column_value->len);
            break;
        case ESP_BLE_MESH_MODEL_OP_SENSOR_SERIES_GET:
            ESP_LOGI(TAG, "Sensor Series Status, opcode 0x%04x, Sensor Property ID 0x%04x",
                     param->params->ctx.recv_op, param->status_cb.series_status.property_id);
            ESP_LOG_BUFFER_HEX("Sensor Series", param->status_cb.series_status.sensor_series_value->data,
                               param->status_cb.series_status.sensor_series_value->len);
            break;
        default:
            ESP_LOGE(TAG, "Unknown Sensor Get opcode 0x%04x", param->params->ctx.recv_op);
            break;
        }
        break;
    case ESP_BLE_MESH_SENSOR_CLIENT_SET_STATE_EVT:
        switch (param->params->opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_SET:
            ESP_LOGI(TAG, "Sensor Cadence Status, opcode 0x%04x, Sensor Property ID 0x%04x",
                     param->params->ctx.recv_op, param->status_cb.cadence_status.property_id);
            ESP_LOG_BUFFER_HEX("Sensor Cadence", param->status_cb.cadence_status.sensor_cadence_value->data,
                               param->status_cb.cadence_status.sensor_cadence_value->len);
            break;
        case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_SET:
            ESP_LOGI(TAG, "Sensor Setting Status, opcode 0x%04x, Sensor Property ID 0x%04x, Sensor Setting Property ID 0x%04x",
                     param->params->ctx.recv_op, param->status_cb.setting_status.sensor_property_id,
                     param->status_cb.setting_status.sensor_setting_property_id);
            if (param->status_cb.setting_status.op_en)
            {
                ESP_LOGI(TAG, "Sensor Setting Access 0x%02x", param->status_cb.setting_status.sensor_setting_access);
                ESP_LOG_BUFFER_HEX("Sensor Setting Raw", param->status_cb.setting_status.sensor_setting_raw->data,
                                   param->status_cb.setting_status.sensor_setting_raw->len);
            }
            break;
        default:
            ESP_LOGE(TAG, "Unknown Sensor Set opcode 0x%04x", param->params->ctx.recv_op);
            break;
        }
        break;
    case ESP_BLE_MESH_SENSOR_CLIENT_PUBLISH_EVT:
        break;
    case ESP_BLE_MESH_SENSOR_CLIENT_TIMEOUT_EVT:
        ble_mesh_sensor_timeout(param->params->opcode, param->params->ctx.addr);
    default:
        break;
    }
}

void ble_mesh_sensor_timeout(uint32_t opcode, uint32_t addr)
{
    switch (opcode)
    {
    case ESP_BLE_MESH_MODEL_OP_SENSOR_DESCRIPTOR_GET:
        ESP_LOGW(TAG, "Sensor Descriptor Get timeout, opcode 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_GET:
        ESP_LOGW(TAG, "Sensor Cadence Get timeout, opcode 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_SET:
        ESP_LOGW(TAG, "Sensor Cadence Set timeout, opcode 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTINGS_GET:
        ESP_LOGW(TAG, "Sensor Settings Get timeout, opcode 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_GET:
        ESP_LOGW(TAG, "Sensor Setting Get timeout, opcode 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_SET:
        ESP_LOGW(TAG, "Sensor Setting Set timeout, opcode 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
        ESP_LOGW(TAG, "Sensor Get timeout 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_COLUMN_GET:
        ESP_LOGW(TAG, "Sensor Column Get timeout, opcode 0x%04x", opcode);
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_SERIES_GET:
        ESP_LOGW(TAG, "Sensor Series Get timeout, opcode 0x%04x", opcode);
        break;
    default:
        ESP_LOGE(TAG, "Unknown Sensor Get/Set opcode 0x%04x", opcode);
        return;
    }

    ble_mesh_send_sensor_message(opcode, addr);
}

/**
 * @brief ble mesh send sensor message
 *
 * @param opcode
 */
void ble_mesh_send_sensor_message(uint32_t opcode, uint16_t addr)
{
    esp_ble_mesh_sensor_client_get_state_t get = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t node;
    esp_ble_mesh_node_t *node_buffer = NULL;
    esp_err_t err = ESP_OK;
    printf("check freeheap 2:%d\n", esp_get_free_heap_size());

    node_buffer = esp_ble_mesh_provisioner_get_node_with_addr(addr);
    if (node_buffer == NULL)
    {
        ESP_LOGE(TAG, "Node 0x%04x not exists", addr);
        return;
    }
    node.unicast = node_buffer->unicast_addr;
    node.elem_num = node_buffer->element_num;

    memcpy(node.uuid, node_buffer->dev_uuid, 16);

    ESP_LOGI(TAG, "ble_mesh_send_sensor_message = %x", node.unicast);

    int ret = ble_mesh_set_msg_common(&common, &node, sensor_client.model, opcode);
    if (ret == ESP_ERR_INVALID_ARG)
        ESP_LOGE(TAG, "****invalist aggr yo");
    switch (opcode)
    {
    case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_GET:
        get.cadence_get.property_id = sensor_prop_id;
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTINGS_GET:
        get.settings_get.sensor_property_id = sensor_prop_id;
        break;
    case ESP_BLE_MESH_MODEL_OP_SENSOR_SERIES_GET:
        get.series_get.property_id = sensor_prop_id;
        break;
    default:
        break;
    }
    printf("check freeheap 1:%d\n", esp_get_free_heap_size());

    ESP_LOGI(TAG, "sensor_prop_id = 0x%04x", sensor_prop_id);
    ESP_LOGI(TAG, "common.ctx.addr = 0x%04x", common.ctx.addr);

    err = esp_ble_mesh_sensor_client_get_state(&common, &get);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send sensor message 0x%04" PRIx32, opcode);
        ESP_LOGE(TAG, "******Failed to send sensor message 0x%04x", err);
    }
}

/**
 * @brief ble mesh set on/off state
 *
 */
uint8_t Transaction_ID = 0;
int ble_mesh_send_gen_onoff_set(uint8_t state, uint16_t addr)
{
    ESP_LOGI(TAG, "ble_mesh_send_gen_onoff_set net index = 0x%04x --- app index = 0x%04x", prov_key.net_idx, prov_key.app_idx);

    ESP_LOG_BUFFER_HEX("AppKey", prov_key.app_key, 16);
    esp_ble_mesh_generic_client_set_state_t set = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_err_t err = ESP_OK;

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET;
    common.model = onoff_client.model;
    common.ctx.net_idx = prov_key.net_idx;
    common.ctx.app_idx = prov_key.app_idx;
    common.ctx.addr = addr; /* to all nodes */
    common.ctx.send_ttl = 3;
    common.ctx.send_rel = false;
    common.msg_timeout = 0;     /* 0 indicates that timeout value from menuconfig will be used */
    common.msg_role = MSG_ROLE; // ROLE_NODE;

    set.onoff_set.op_en = false;
    set.onoff_set.onoff = state;
    set.onoff_set.tid = Transaction_ID++;

    err = esp_ble_mesh_generic_client_set_state(&common, &set);
    if (err)
    {
        ESP_LOGE(TAG, "Send Generic OnOff Set Unack failed");
        return err;
    }
    return err;
}

/**
 * @brief
 *
 * @param event
 */
void user_ble_mesh_process_event(e_ble_mesh_user_event event)
{
    switch (event)
    {
    case E_USER_BLE_MESH_IDLE:
        /* code */
        break;
    case E_USER_BLE_MESH_BIDING_APP_KEY:
    {
        esp_err_t err = 0;
        err = ble_mesh_binding_app_key_model_onoff(prov_key.app_idx);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Provisioner ble_mesh_binding_app_key_model_onoff failed");
            return;
        }
        err = ble_mesh_binding_app_key_model_sensor(prov_key.app_idx);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Provisioner ble_mesh_binding_app_key_model_sensor failed");
            return;
        }
        break;
    }
    case E_USER_BLE_MESH_BIDING_APP_KEY_ID_SENSOR_SRV:

        break;
    case E_USER_BLE_MESH_BIDING_APP_KEY_ID_GEN_ONOFF_SRV:
        break;
    default:
        break;
    }
}

int ble_mesh_enable_recv_hearbeat(uint8_t enable)
{
    ESP_LOGD(TAG, "enter %s\n", __func__);
    esp_err_t result = ESP_OK;

    result = esp_ble_mesh_provisioner_recv_heartbeat(enable);

    return result;
}

/**
 * @brief erase the flash device from internal flash
 *
 * @param erase
 */
void ble_mesh_erase_settings(bool erase)
{
    bt_mesh_settings_reset(erase);
    int err = nvs_flash_erase_partition("ble_mesh");
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) nvs_flash_erase_partition ble_mesh handle!\n", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "nvs_flash_erase_partition done");
    }
    err = nvs_flash_erase_partition("user_ble"); // erase user_ble
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) nvs_flash_erase_partition user_ble handle!\n", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "nvs_flash_erase_partition done");
    }
}

/**
 * @brief get uuid from name string
 *
 * @param name
 * @return uint16_t
 */
uint16_t ble_mesh_get_uuid_with_name(const char *name)
{
    uint16_t uuid;
    esp_ble_mesh_node_t *node_info = NULL;
    node_info = esp_ble_mesh_provisioner_get_node_with_name(name);
    if (!node_info)
    {
        ESP_LOGE(TAG, "Node name %s not exists", name);
        return 0;
    }
    uuid = node_info->unicast_addr;
    return (uint16_t)uuid;
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/