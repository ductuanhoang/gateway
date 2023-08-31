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
#include <stdint.h>
#include "esp_ble_mesh_defs.h"
#include "ble_mesh.h"
// #include <string.h>
/**
 * @brief 
 * 
 * @param node_name 
 * @return uint16_t 
 */
// uint16_t ble_mesh_get_node_info_with_name(std::string node_name);

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
 * @param node
 */
void ble_mesh_add_proved_devices(esp_ble_mesh_node_info_t node);



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