/**
 * @file ble_mesh_process.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-07-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef BLE_MESH_PROCESS_H_
#define BLE_MESH_PROCESS_H_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
// #include <string>
#include "esp_ble_mesh_defs.h"
#include "ble_mesh.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /****************************************************************************/
    /***        Macro Definitions                                             ***/
    /****************************************************************************/

    /****************************************************************************/
    /***        Type Definitions                                              ***/
    /****************************************************************************/
    typedef void (*ble_mesh_report_t)(void *);
    /****************************************************************************/
    /***         Exported global functions                                     ***/
    /****************************************************************************/
    void ble_mesh_report_scan_result_register_callback(ble_mesh_report_t callback);
    /**
     * @brief
     *
     * @param new_dev_unprov
     * @return int
     */
    int ble_mesh_compare_new_prov(esp_ble_mesh_unprov_dev_add_t new_dev_unprov);

    /**
     * @brief
     *
     */
    void ble_mesh_tasks_init(void);

    /**
     * @brief
     *
     * @param index
     */
    void ble_mesh_provision_device_index(uint32_t index);

    /**
     * @brief
     *
     * @return uint8_t
     */
    uint8_t ble_mesh_provision_device_get_num_devices(void);

    /**
     * @brief
     *
     */
    void ble_mesh_powerup_load_flash_devices(void);

    /**
     * @brief
     *
     * @param name
     */
    void ble_mesh_provision_device_with_name(const char *name);
    /**
     * @brief deletes the node sensor with name
     *
     * @param name
     * @return int
     */
    int ble_mesh_provision_delete_with_name(const char *name);
    
    /**
     * @brief remove the device from the list of devices
     *
     * @param name
     */
    void ble_mesh_remove_device_from_list(const char *name);
    /**
     * @brief
     *
     * @return uint8_t
     */
    uint8_t ble_mesh_provisioned_device_get_num_devices(void);

    /**
     * @brief
     *
     * @param index
     * @return char*
     */
    const char *ble_mesh_provisioned_device_get_name(uint8_t index);

    /**
     * @brief
     *
     */
    void ble_mesh_provision_device_show_devices(void);

    /**
     * @brief
     *
     * @param node
     */
    void ble_mesh_add_proved_devices(const char *node_name);

    /**
     * @brief
     *
     * @param index
     * @return const char*
     */
    const char *ble_mesh_get_node_name_with_index(uint16_t index);

    /**
     * @brief
     *
     * @param index
     * @return esp_ble_mesh_unprov_dev_add_t
     */
    esp_ble_mesh_unprov_dev_add_t ble_mesh_get_unprov_device_index(uint32_t index);

    /**
     * @brief
     *
     * @param index
     * @return const char*
     */
    const char *ble_mesh_get_unprov_device_name(uint32_t index);

    /**
     * @brief
     *
     */
    void ble_mesh_free_unprov_table(void);
#ifdef __cplusplus
}
#endif

#endif