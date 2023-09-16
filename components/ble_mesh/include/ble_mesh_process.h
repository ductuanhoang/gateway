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
extern "C" {
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/

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
const char* ble_mesh_get_node_name_with_index(uint16_t index);

/**
 * @brief 
 * 
 * @param index 
 * @return esp_ble_mesh_unprov_dev_add_t 
 */
esp_ble_mesh_unprov_dev_add_t ble_mesh_get_unprov_device_index(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif