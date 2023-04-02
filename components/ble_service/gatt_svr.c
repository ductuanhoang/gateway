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

static const char *manuf_name = "ESP32ReadService";
static const char *model_num = "123456";

bool uart_active_handle = false;
uint8_t gatt_svr_led_static_val = 0;

static int gatt_svr_chr_rmc_read_notify(uint16_t conn_handle, uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt, void *arg);

static int gatt_svr_chr_ctr_led(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int gatt_svr_chr_serial_active(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // NMEA_SERVICE
    {/* Service: receive the UART message */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(NMEA_SERVICE),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(NMEA_RMC_CHAR),
             .access_cb = gatt_svr_chr_rmc_read_notify,
            //  .val_handle = &uart_service_handle,
             .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,
         },
         {
             0, /* No more characteristics in this service */
         },
     }},

    {/* Service: control led */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(LED_SERVICE),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(LED_RED_CHAR),
             .access_cb = gatt_svr_chr_ctr_led,
             .flags = BLE_GATT_CHR_F_WRITE,
         },
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(LED_YLW_CHAR),
             .access_cb = gatt_svr_chr_ctr_led,
             .flags = BLE_GATT_CHR_F_WRITE,
         },
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(LED_BLU_CHAR),
             .access_cb = gatt_svr_chr_ctr_led,
             .flags = BLE_GATT_CHR_F_WRITE,
         },
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(LED_WHT_CHAR),
             .access_cb = gatt_svr_chr_ctr_led,
             .flags = BLE_GATT_CHR_F_WRITE,
         },
         {
             0, /* No more characteristics in this service */
         },
     }},
    {/* Service: active */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(SERIAL_SERVICE),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             /* Characteristic: Heart-rate measurement */
             .uuid = BLE_UUID16_DECLARE(SERIAL_ADCTIVE_CHAR),
             .access_cb = gatt_svr_chr_ctr_led,
             .flags = BLE_GATT_CHR_F_WRITE,
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
static int gatt_svr_chr_rmc_read_notify(uint16_t conn_handle, uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc = -1;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    printf("uuid = %x\n", uuid);
    if (uuid == NMEA_RMC_CHAR)
    {
        // rc = os_mbuf_append(ctxt->om, &uart_message_handle,
        //                     strlen((const char*) uart_message_handle));
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
static int gatt_svr_chr_ctr_led(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    printf("uuid = %x\n", uuid);
    if (uuid == LED_RED_CHAR)
    {
        // rc = gatt_svr_chr_write(ctxt->om, 0,
        //                         sizeof leds_control.led_red,
        //                         &leds_control.led_red, NULL);
        // ESP_LOGI(TAG, "LED_RED_CHAR = %d\n", leds_control.led_red);
        return rc;
    }
    else if (uuid == LED_YLW_CHAR)
    {
        // rc = gatt_svr_chr_write(ctxt->om, 0,
        //                         sizeof leds_control.led_ylw,
        //                         &leds_control.led_ylw, NULL);
        // ESP_LOGI(TAG, "LED_YLW_CHAR = %d\n", leds_control.led_ylw);
        return rc;
    }
    else if (uuid == LED_BLU_CHAR)
    {
        // rc = gatt_svr_chr_write(ctxt->om, 0,
        //                         sizeof leds_control.led_blu,
        //                         &leds_control.led_blu, NULL);
        // ESP_LOGI(TAG, "LED_BLU_CHAR = %d\n", leds_control.led_blu);
        return rc;
    }
    else if (uuid == LED_WHT_CHAR)
    {
        // rc = gatt_svr_chr_write(ctxt->om, 0,
        //                         sizeof leds_control.led_wht,
        //                         &leds_control.led_wht, NULL);
        // ESP_LOGI(TAG, "LED_WHT_CHAR = %d\n", leds_control.led_wht);
        return rc;
    }
    else if (uuid == SERIAL_ADCTIVE_CHAR)
    {
        // rc = gatt_svr_chr_write(ctxt->om, 0,
        //                         sizeof uart_active_receive,
        //                         &uart_active_receive, NULL);
        // ESP_LOGI(TAG, "SERIAL_ADCTIVE_CHAR = %d\n", uart_active_receive);
        return rc;
    }

    // assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief characteristics control active or disable message UART
 *
 * @param conn_handle
 * @param attr_handle
 * @param ctxt
 * @param arg
 * @return int
 */
static int gatt_svr_chr_serial_active(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    printf("uuid = %x\n", uuid);
    if (uuid == NMEA_RMC_CHAR)
    {
        // rc = os_mbuf_append(ctxt->om, &uart_message_handle,
        //                     strlen((const char *)uart_message_handle));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

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

