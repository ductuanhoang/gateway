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

#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h"
/* BLE */
// #include "esp_nimble_hci.h"
// #include "nimble/nimble_port.h"
// #include "nimble/nimble_port_freertos.h"

// #include "host/ble_hs.h"
// #include "host/util/util.h"
// #include "console/console.h"
// #include "services/gap/ble_svc_gap.h"
// #include "gatt_svr.h"

#include "common.h"
#include "user_app.h"
#include "device_manager.h"

#define TAG_MAIN "MAIN"
#include "ArduinoJson.h"
#ifdef __cplusplus
extern "C"
{
#endif

    void app_main(void);

#ifdef __cplusplus
}
#endif

device_info_t device_info;

EndDeviceData_t end_device_data;

GateWayData_t gateway_data;

extern "C" void app_main(void)
{
    app_init();
}
