/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_BLEHR_SENSOR_
#define H_BLEHR_SENSOR_

#include "nimble/ble.h"
#include "modlog/modlog.h"
#include <stdio.h>
#include <string.h>
#define DEVICE_INFO_SERVICE 0x2A00
#define DEVICE_INFO_SYSTEM_ID_CHAR 0x2A01
#define DEVICE_INFO_FIRMWARE_VERSION_CHAR 0x2A02
#define DEVICE_INFO_HARDWARE_VERSION_CHAR 0x2A03

#define WIFI_SERVICE 0x1A00
#define WIFI_RESPONSE_CHAR 0x1A01
#define WIFI_COMMAND_CHAR 0x1A02

#ifdef __cplusplus
extern "C"
{
#endif
    typedef void (*ble_command_callback_t)(uint8_t *);

    extern uint16_t wifi_command_ctr_notify;
    extern uint16_t wifi_command_ressults_notify;
    
    struct ble_hs_cfg;
    struct ble_gatt_register_ctxt;

    void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
    int gatt_svr_init(void);

    /**
     * @brief report uart message
     *
     * @param message
     * @param lenght
     */
    void gatt_report_notify(const char *message, uint16_t len);

    void ble_command_callback_init(ble_command_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif
