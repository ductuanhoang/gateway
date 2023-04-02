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
#include <string.h>
#include <string>

#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h"
/* BLE */
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "gatt_svr.h"
#include "common.h"
#include "user_app.h"

#include "ble_main.h"

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

void jsonTest(void)
{
    DynamicJsonDocument doc(1024);

    doc["sensor"] = "gps";
    doc["time"] = 1351824120;
    doc["data"][0] = 48.756080;
    doc["data"][1] = 2.302038;
    char buffer[100];

    serializeJson(doc, &buffer, sizeof(buffer));
    ESP_LOGI(TAG_MAIN, "message control: %s", buffer);
    // This prints:
    // {"sensor":"gps","time":1351824120,"data":[48.756080,2.302038]}
}

extern "C" void app_main(void)
{
    int rc;

    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // app init
    app_init();
    jsonTest();
    // ESP_LOGI(TAG_MAIN, "message control: %s", output);
    app_init();
    // start uart service
    // uart_recieve_callback_init(gatt_report_notify);
    // user_uart_init();
}
