/**
 * @file ble_main.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef H_USER_BLE_MAIN_
#define H_USER_BLE_MAIN_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "nimble/ble.h"
#include "modlog/modlog.h"
#include "gatt_svr.h"
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

    /****************************************************************************/
    /***         Exported global functions                                     ***/
    /****************************************************************************/

    void ble_main_init(void);

    void gatt_report_wifi_scan_notify(const char *message, uint16_t len);

    void gatt_report_reponse_command_notify(const char *message, uint16_t len);
    

#ifdef __cplusplus
}
#endif

#endif /* H_USER_BLE_MAIN_ */