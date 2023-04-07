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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "gatt_svr.h"
#include "common.h"

#define TAG "GAT "

uint16_t wifi_command_ctr_notify = 0;
uint16_t wifi_command_ressults_notify = 0;

static const char *manuf_name = "ESP32ReadService";
static const char *model_num = "123456";

bool uart_active_handle = false;
uint8_t gatt_svr_led_static_val = 0;

static ble_command_callback_t ble_command_callback = NULL;

static int gatt_svr_chr_wifi_read_notify(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg);

static int gatt_svr_chr_wifi(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg);

static int gatt_svr_chr_wifi_read_notify(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // DEVICE_INFO_SERVICE
    {/* Service: device infor */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_SERVICE),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_SYSTEM_ID_CHAR),
             .access_cb = gatt_svr_chr_wifi_read_notify,
             .flags = BLE_GATT_CHR_F_READ,
         },
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_FIRMWARE_VERSION_CHAR),
             .access_cb = gatt_svr_chr_wifi_read_notify,
             .flags = BLE_GATT_CHR_F_READ,
         },
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_HARDWARE_VERSION_CHAR),
             .access_cb = gatt_svr_chr_wifi_read_notify,
             .flags = BLE_GATT_CHR_F_READ,
         },
         {
             0, /* No more characteristics in this service */
         },
     }},

    {/* Service: wifi service */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(WIFI_SERVICE),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(WIFI_RESPONSE_CHAR),
             .access_cb = gatt_svr_chr_wifi,
             .val_handle = &wifi_command_ctr_notify,
             .flags = BLE_GATT_CHR_F_NOTIFY,
         },
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(WIFI_COMMAND_CHAR),
             .access_cb = gatt_svr_chr_wifi,
             .val_handle = &wifi_command_ressults_notify,
             .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
         },
         {
             0, /* No more characteristics in this service */
         },
     }},

    {
        0, /* No more services */
    },
};

static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                              void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len)
    {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

/**
 * @brief characteristics receive message from UART
 *
 * @param conn_handle
 * @param attr_handle
 * @param ctxt
 * @param arg
 * @return int
 */
static int gatt_svr_chr_wifi_read_notify(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc = -1;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    printf("uuid = %x\n", uuid);
    if (uuid == DEVICE_INFO_SYSTEM_ID_CHAR)
    {
        rc = os_mbuf_append(ctxt->om, &FIRMWARE_VERSION,
                            strlen((const char *)FIRMWARE_VERSION));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    else if (uuid == DEVICE_INFO_FIRMWARE_VERSION_CHAR)
    {
        rc = os_mbuf_append(ctxt->om, &FIRMWARE_VERSION,
                            strlen((const char *)FIRMWARE_VERSION));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    else if (uuid == DEVICE_INFO_HARDWARE_VERSION_CHAR)
    {
        rc = os_mbuf_append(ctxt->om, &HARDWARE_VERSION,
                            strlen((const char *)HARDWARE_VERSION));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief characteristics control leds
 *
 * @param conn_handle
 * @param attr_handle
 * @param ctxt
 * @param arg
 * @return int
 */
static int gatt_svr_chr_wifi(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc = -1;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    printf("uuid = %x\n", uuid);
    uint8_t buffer[126];
    memset(buffer, 0x00, sizeof(buffer));

    if (uuid == WIFI_COMMAND_CHAR)
    {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
        {
            ESP_LOGI(TAG, "WRITE WIFI_COMMAND_CHAR");
            rc = gatt_svr_chr_write(ctxt->om, 0,
                                    sizeof buffer,
                                    &buffer, NULL);
            if (ble_command_callback != NULL)
                ble_command_callback(buffer);
        }
        else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            ESP_LOGI(TAG, "READ WIFI_COMMAND_CHAR");
        }
        return rc;
    }
    else if (uuid == WIFI_RESPONSE_CHAR)
    {
        ESP_LOGI(TAG, "WIFI_RESPONSE_CHAR");
        // rc = gatt_svr_chr_write(ctxt->om, 0,
        //                         sizeof leds_control.led_ylw,
        //                         &leds_control.led_ylw, NULL);
        // ESP_LOGI(TAG, "LED_YLW_CHAR = %d\n", leds_control.led_ylw);
        return rc;
    }

    // assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op)
    {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGI(TAG, "registered service %s with handle=%d\n",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI(TAG, "registering characteristic %s with "
                      "def_handle=%d val_handle=%d\n",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle,
                 ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGI(TAG, "registering descriptor %s with handle=%d\n",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

void ble_command_callback_init(ble_command_callback_t callback)
{
    if (callback)
    {
        ble_command_callback = callback;
    }
    else
    {
        ESP_LOGE(TAG, "uart_recieve_callback register error");
        return;
    }
}
