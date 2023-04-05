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

#ifdef __cplusplus
extern "C" {
#endif


#define NMEA_SERVICE    0x1000
#define NMEA_RMC_CHAR   0x1001

#define LED_SERVICE     0x2000
#define LED_RED_CHAR    0x2001
#define LED_YLW_CHAR    0x2002
#define LED_BLU_CHAR    0x2003
#define LED_WHT_CHAR    0x2004

#define SERIAL_SERVICE  0x3000
#define SERIAL_ADCTIVE_CHAR 0x3001



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

#ifdef __cplusplus
}
#endif

#endif
