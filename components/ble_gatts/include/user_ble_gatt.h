/**
 * @file user_ble_gatt.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-05-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef USER_BLE_GATTS_H_
#define USER_BLE_GATTS_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stdbool.h>

    /****************************************************************************/
    /***        Include files                                                 ***/
    /****************************************************************************/

    /****************************************************************************/
    /***        Macro Definitions                                             ***/
    /****************************************************************************/

#define BLE_MAIN_PREFIX "Breaker_"

#define DEVICE_INFO_SERVICE 0x2A00
#define DEVICE_INFO_SYSTEM_ID_CHAR 0x2A01
#define DEVICE_INFO_FIRMWARE_VERSION_CHAR 0x2A02
#define DEVICE_INFO_HARDWARE_VERSION_CHAR 0x2A03
#define DEVICE_INFO_NUM_HANDLE 4

#define DEVICE_WIFI_SERVICE 0x1A00
#define DEVICE_WIFI_RESPONSE_CHAR 0x1A01
#define DEVICE_WIFI_COMMAND_CHAR 0x1A02
#define DEVICE_WIFI_NUM_HANDLE 4

    /****************************************************************************/
    /***        Type Definitions                                              ***/
    /****************************************************************************/
    typedef void (*ble_command_callback_t)(uint8_t *, uint16_t);
    /****************************************************************************/
    /***         Exported global functions                                     ***/
    /****************************************************************************/
    void user_ble_gatts_init(void);

    void ble_command_callback_init(ble_command_callback_t callback);
#ifdef __cplusplus
}
#endif
#endif /* USER_BLE_GATTS_H_ */