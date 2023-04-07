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

#ifndef H_COMMON_
#define H_COMMON_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


#define FIRMWARE_VERSION    "1.0.0"
#define HARDWARE_VERSION    "1.0.2"
#define DEVICE_MODEL    "HUB"

#define USER_WIFI_SSID_MAX_LENGTH   64
#define USER_WIFI_PASS_MAX_LENGTH   64

typedef struct 
{
    uint8_t ssid[USER_WIFI_SSID_MAX_LENGTH];
    uint8_t pass[USER_WIFI_PASS_MAX_LENGTH];
}wifi_info_t;


typedef struct 
{
    char MAC[20];
    char device_name[20];
    uint8_t ssid;
    wifi_info_t wifi;
}device_info_t;

extern device_info_t device_info;
void user_sys_restart(void);
void user_sys_get_mac(char *mac);
void user_sys_get_deviceName(char *devicename);

#ifdef __cplusplus
}
#endif

#endif
