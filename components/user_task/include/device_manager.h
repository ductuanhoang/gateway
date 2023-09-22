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

#pragma once
#undef max
#undef min

// #include "aws_iot_error.h"
#include "common.h"
#include "ArduinoJson.h"
#include <cstring>
#include <string.h>
#include <string>
#include <vector>
#define JSON_MAX_STRING_LENGTH 500
class DeviceManager
{
public:
    enum MsgType
    {
        STATE = 0,
        REPORTED,
        DESIRED,
        DELTA
    };

    enum DataType
    {
        ID = 0,
        VOLTAGE,
        CURRENT,
        POWER,
        STATUS,
        SOURCE,
        PRIMARY,
        SECONDARY
    };

    enum DataDevice
    {
        BREAKERMATE = 0,
        HUB_ID
    };

    enum Controltype
    {
        CONTROL,
        SCAN,
        ADD,
        DELETE
    };

private:
    std::string m_version;
    std::string m_Hub_Id;
    std::string m_message;
    std::vector<std::string> m_dataContent;
    std::vector<std::string> m_ScanNodeContent;
    EndDeviceData_t m_gateway_data;

public:
    DeviceManager(const std::string &Hub_id);
    ~DeviceManager();

    void setDeviceId(const std::string &ID);
    std::string getDeviceId(void);
    void deviceReportDataPoint(std::string DP, EndDeviceData_t data);
    std::string deviceReportAllDataPoints(void);

    std::string deviceReportScanResult(void);
    void deviceReportScanDataPoint(std::string id_node);

    EndDeviceData_t devivePasserMessage(std::string messge_read);
    EndDeviceData_t devicePasserMessage_02(std::string message);

    std::string deviceReportTopic(std::string Hub_id);
    std::string deviceSubTopic(std::string Hub_id);
    void setDataContent(std::string data);
    void setScanNodeDataContent(std::string data);
    void freeDataContent(void);
    void freeScanContent(void);
    void setGateWayData(EndDeviceData_t data);
    void clearGateWayData(void);
};