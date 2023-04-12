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

#ifndef H_DEVICE_MANAGER_
#define H_DEVICE_MANAGER_
#include <stdint.h>
#include <stdbool.h>
#include <iostream>
#include <cstring>
#include <map>
#include "aws_iot_error.h"
using namespace std;

class JsonMessage
{
private:
    /* data */
public:
    enum Service
    {
        HOME_DATA = 0,
        PROVISIONING,
        UNKNOWN,
        SERVICE_MAX_NUM
    };

    enum Action
    {
        GET = 0,
        POST,
        DELETE,
        REPORT,
        SYNC,
        EXEC,
        ACT_UNKNOWN,
        ACTION_MAX_NUM
    };

    enum MsgType
    {
        DEVICE = 0,
        HOME,
        ENV,
        RULE,
        SENCE,
        SCHEDULE,
        VERSION,
        MSG_TYPE_UNKNOWN,
        MSG_TYPE_MAX_NUM
    };

    enum DataLink
    {
        UPLINK = 0,
        DOWNLINK,
        DATA_LINK_UNKNOWN,
        DATA_LINK_MAX_NUM
    };

    enum TransType
    {
        REQUEST = 0,
        RESPONSE,
        TRANS_TYPE_UNKNOWN
    };

JsonMessage(
        const string& reqID,
        Service service,
        Action action, 
        MsgType msgType, 
        DataLink rootDatalink, 
        TransType rootTransType);

    JsonMessage(
        char *message,
        const string& reqID = string(),
        Service service = Service::UNKNOWN,
        Action action = Action::ACT_UNKNOWN, 
        MsgType msgType = MsgType::MSG_TYPE_UNKNOWN, 
        DataLink rootDatalink = DataLink::DATA_LINK_UNKNOWN, 
        TransType rootTransType = TransType::TRANS_TYPE_UNKNOWN);

    JsonMessage(
        Service service = Service::UNKNOWN,
        Action action = Action::ACT_UNKNOWN, 
        MsgType msgType = MsgType::MSG_TYPE_UNKNOWN);

    ~JsonMessage();
    JsonMessage(const JsonMessage& cp);

    JsonMessage& operator= (const JsonMessage& copied);
    bool operator== (const JsonMessage& rhs) const;


    void ClearAll();
    static void SetPrefixTopic(string);

    string GetReqID();
    string GetService();
    string GetAction();
    string GetRootDataLink();
    string GetRootTransType();
    string GetFullMessage();
    string GetDataContent();

    string GetReqTopic();
    string GetResTopic();
    string GetAckTopic();
    string CreateTopic(Service, Action, DataLink, TransType);

    void SetReqID(string reqID);
    void SetConfig(Service, Action, MsgType);
    void SetDataContent(string data);
    void SetMessage(string message);


public:
    static map<Service, const char*> GetServiceStringID;
    static map<Action, const char*> GetActionStringID;
    static map<MsgType, const char*> GetMsgTypeStringID;
    static map<DataLink, const char*> GetDataLinkStringID;
    static map<TransType, const char*> GetTransStringID;

    static map<string, Service> GetServiceIntID;
    static map<string, Action> GetActionIntID;
    static map<string, MsgType> GetMsgTypeIntID;
    static map<string, DataLink> GetDataLinkIntID;
    static map<string, TransType> GetTransIntID;

private:
    Service m_service;
    Action m_action;
    MsgType m_msgType;
    DataLink m_rootDatalink;
    TransType m_rootTransType;
    
    string m_reqID;
    string m_dataContent;
    string m_message;

    bool m_isDataAvailable;

protected:
    void Swap(JsonMessage& other);
};



#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif // H_DEVICE_MANAGER_
