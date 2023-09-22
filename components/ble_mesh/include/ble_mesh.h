#ifndef BLE_MESH_H_
#define BLE_MESH_H_
#ifdef __cplusplus
extern "C"
{
#endif

    /****************************************************************************/
    /***        Include files                                                 ***/
    /****************************************************************************/

#include "esp_ble_mesh_defs.h"
    /****************************************************************************/
    /***        Macro Definitions                                             ***/
    /****************************************************************************/
    typedef void (*ble_mesh_prov_complete_t)(void *);
    /****************************************************************************/
    /***        Type Definitions                                              ***/
    /****************************************************************************/
    typedef enum
    {
        E_USER_BLE_MESH_IDLE = 0,
        E_USER_BLE_MESH_BIDING_APP_KEY,
        E_USER_BLE_MESH_BIDING_APP_KEY_ID_SENSOR_SRV,
        E_USER_BLE_MESH_BIDING_APP_KEY_ID_GEN_ONOFF_SRV,
    } e_ble_mesh_user_event;

    /**
     * @brief struct of the sensor data structure
     *
     */
    typedef struct
    {
        uint8_t uuid[16];
        uint16_t unicast;
        uint8_t elem_num;
        uint8_t onoff;
        uint16_t sensor;
    } esp_ble_mesh_node_info_t;

    /**
     * @brief this structure contains the state of the un-provisioned device
     *
     */
    typedef struct
    {
        esp_ble_mesh_unprov_dev_add_t add_dev;
    } ble_mesh_unprov_data_t;

    typedef struct
    {
        char name[32];    // node name
        uint8_t uuid[16]; // unique identifier
        uint16_t unicast; // network address
        uint8_t elem_num; // number of elements in device
        uint8_t onoff;    // onoff state
        uint16_t sensor;  // sensor value
    } ble_mesh_device_data_t;

    typedef struct ble_mesh
    {
        uint16_t net_idx;
        uint16_t app_idx;
        uint8_t app_key[16];
        uint8_t net_key[16];
    } esp_ble_mesh_key_t;

    extern esp_ble_mesh_key_t prov_key;
    /****************************************************************************/
    /***         Exported global functions                                     ***/
    /****************************************************************************/

    /**
     * @brief 
     * 
     * @param callback 
     */
    void user_ble_mesh_prov_complete_register(ble_mesh_prov_complete_t callback);
    
    /**
     * @brief 
     * 
     * @return int 
     */
    int user_ble_mesh_init(void);
    /**
     * @brief provsioner enable or disable
     *
     * @param enable:
     * @return int
     */
    int ble_mesh_provisioner_prov_enable(uint8_t enable);
    
    /**
     * @brief returns the provsion enabled status
     *
     * @return int
     */
    int ble_mesh_provisioner_get_prov_enabled(void);

    /**
     * @brief
     *
     * @return int
     */
    int ble_mesh_node_reset(void);

    /**
     * @brief
     *
     * @param add_dev
     * @return int
     */
    int user_ble_mesh_add_unprov_node(uint8_t node_index);

    /**
     * @brief
     *
     * @param add_dev
     * @return int
     */
    int user_ble_mesh_delete_node(esp_ble_mesh_unprov_dev_add_t add_dev);

    /**
     * @brief
     *
     * @param prov_app_idx
     * @return esp_err_t
     */
    esp_err_t ble_mesh_binding_app_key_model_sensor(uint16_t prov_app_idx);

    /**
     * @brief
     *
     * @param prov_app_idx
     * @return esp_err_t
     */
    esp_err_t ble_mesh_binding_app_key_model_onoff(uint16_t prov_app_idx);

    /**
     * @brief
     *
     * @param addr
     * @return esp_err_t
     */
    esp_err_t ble_mesh_model_app_key_add(uint16_t addr);

    /**
     * @brief
     *
     * @param addr
     * @return esp_err_t
     */
    esp_err_t ble_mesh_binding_model_sensor_service(uint16_t addr);

    /**
     * @brief
     *
     * @param addr
     * @return esp_err_t
     */
    esp_err_t ble_mesh_binding_model_onoff_service(uint16_t addr);

    /**
     * @brief
     *
     * @param opcode
     * @param addr
     */
    void ble_mesh_sensor_timeout(uint32_t opcode, uint32_t addr);

    /**
     * @brief
     *
     * @param opcode
     * @param addr
     */
    void ble_mesh_send_sensor_message(uint32_t opcode, uint16_t addr);

    /**
     * @brief
     *
     * @param state
     * @param addr
     * @return int
     */
    int ble_mesh_send_gen_onoff_set(uint8_t state, uint16_t addr);

    /**
     * @brief
     *
     * @param event
     */
    void user_ble_mesh_process_event(e_ble_mesh_user_event event);

    /**
     * @brief
     *
     * @param enable
     * @return int
     */
    int ble_mesh_enable_recv_hearbeat(uint8_t enable);

    /**
     * @brief erase the flash device from internal flash
     *
     * @param erase
     */
    void ble_mesh_erase_settings(bool erase);

    /**
     * @brief get uuid from name string
     *
     * @param name
     * @return uint16_t
     */
    uint16_t ble_mesh_get_uuid_with_name(const char *name);
#ifdef __cplusplus
}
#endif
#endif /* BLE_MESH_H_ */