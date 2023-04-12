#ifndef __USMART_JSON_TRANSFER_H__
#define __USMART_JSON_TRANSFER_H__
#include "common.h"
#include "JsonMessage.h"
#include "AwsAgentSession.h"

class JsonTransfer : public JsonMessage {
public:
    JsonTransfer(
        const string& transferID,
        AwsAgentSession& session,
        JsonMessage::Service service = JsonMessage::Service::UNKNOWN,
        JsonMessage::Action action = JsonMessage::Action::ACT_UNKNOWN, 
        JsonMessage::MsgType msgType = JsonMessage::MsgType::MSG_TYPE_UNKNOWN, 
        JsonMessage::DataLink rootDatalink = JsonMessage::DataLink::DATA_LINK_UNKNOWN, 
        JsonMessage::TransType rootTransType = JsonMessage::TransType::TRANS_TYPE_UNKNOWN);

    JsonTransfer(
        AwsAgentSession& m_session,
        JsonMessage::Service service = JsonMessage::Service::UNKNOWN,
        JsonMessage::Action action = JsonMessage::Action::ACT_UNKNOWN, 
        JsonMessage::MsgType msgType = JsonMessage::MsgType::MSG_TYPE_UNKNOWN);

    string Request(string payload, int timeout = 5);
    string Request(bool forward=false, int timeout=5);
    size_t SetRequestPlayload(DynamicJsonDocument dataDoc);
    size_t SetMessage(DynamicJsonDocument payload);
    void Response();
    int SendAck();
private:
    AwsAgentSession& m_session;
    string m_transferID;
    string m_reqPayload;

};

#endif //__USMART_JSON_TRANSFER_H__