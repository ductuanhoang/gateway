
#include "JsonMessage.h"
#include "Utils.h"
#include "esp_log.h"

static string m_prefix(PREFIX_TOPIC);

map<JsonMessage::Service, const char *> JsonMessage::GetServiceStringID = {
    {JsonMessage::Service::HOME_DATA, "home_data"},
    {JsonMessage::Service::PROVISIONING, "provisioning"}};

map<string, JsonMessage::Service> JsonMessage::GetServiceIntID = {
    {"home_data", JsonMessage::Service::HOME_DATA},
    {"provisioning", JsonMessage::Service::PROVISIONING}};

map<JsonMessage::Action, const char *> JsonMessage::GetActionStringID = {
    {JsonMessage::Action::GET, "get"},
    {JsonMessage::Action::POST, "post"},
    {JsonMessage::Action::DELETE, "delete"},
    {JsonMessage::Action::REPORT, "report"},
    {JsonMessage::Action::SYNC, "sync"},
    {JsonMessage::Action::EXEC, "exec"}};

map<string, JsonMessage::Action> JsonMessage::GetActionIntID = {
    {"get", JsonMessage::Action::GET},
    {"post", JsonMessage::Action::POST},
    {"delete", JsonMessage::Action::DELETE},
    {"report", JsonMessage::Action::REPORT},
    {"sync", JsonMessage::Action::SYNC},
    {"exec", JsonMessage::Action::EXEC},
};

map<JsonMessage::MsgType, const char *> JsonMessage::GetMsgTypeStringID = {
    {JsonMessage::MsgType::DEVICE, "device"},
    {JsonMessage::MsgType::HOME, "home"},
    {JsonMessage::MsgType::ENV, "env"},
    {JsonMessage::MsgType::RULE, "rule"},
    {JsonMessage::MsgType::SENCE, "sence"},
    {JsonMessage::MsgType::SCHEDULE, "schedule"},
    {JsonMessage::MsgType::VERSION, "version"}};

map<string, JsonMessage::MsgType> JsonMessage::GetMsgTypeIntID = {
    {"device", JsonMessage::MsgType::DEVICE},
    {"home", JsonMessage::MsgType::HOME},
    {"env", JsonMessage::MsgType::ENV},
    {"rule", JsonMessage::MsgType::RULE},
    {"sence", JsonMessage::MsgType::SENCE},
    {"schedule", JsonMessage::MsgType::SCHEDULE},
    {"version", JsonMessage::MsgType::VERSION},
};

map<JsonMessage::DataLink, const char *> JsonMessage::GetDataLinkStringID = {
    {JsonMessage::DataLink::UPLINK, "up"},
    {JsonMessage::DataLink::DOWNLINK, "dn"}};

map<string, JsonMessage::DataLink> JsonMessage::GetDataLinkIntID = {
    {"up", JsonMessage::DataLink::UPLINK},
    {"dn", JsonMessage::DataLink::DOWNLINK}};

map<JsonMessage::TransType, const char *> JsonMessage::GetTransStringID = {
    {JsonMessage::TransType::REQUEST, "req"},
    {JsonMessage::TransType::RESPONSE, "res"}};

map<string, JsonMessage::TransType> JsonMessage::GetTransIntID = {
    {"req", JsonMessage::TransType::REQUEST},
    {"res", JsonMessage::TransType::RESPONSE}};

JsonMessage::JsonMessage(
    const string &reqID,
    Service service,
    Action action,
    MsgType msgType,
    DataLink rootDatalink,
    TransType rootTransType)
{
    if (reqID.empty())
    {
        m_reqID = Utils::GenID(32);
    }
    else
    {
        m_reqID = reqID;
    }
    m_service = service;
    m_action = action;
    m_msgType = msgType;
    m_rootDatalink = rootDatalink;
    m_rootTransType = rootTransType;
}

JsonMessage::JsonMessage(
    char *message,
    const string &reqID,
    Service service,
    Action action,
    MsgType msgType,
    DataLink rootDatalink,
    TransType rootTransType)
{
    if (reqID.empty())
    {
        m_reqID = Utils::GenID(32);
    }
    m_service = service;
    m_action = action;
    m_msgType = msgType;
    m_rootDatalink = rootDatalink;
    m_rootTransType = rootTransType;
}

JsonMessage::~JsonMessage()
{
}

JsonMessage::JsonMessage(
    Service service,
    Action action,
    MsgType msgType)
{
    m_reqID = Utils::GenID(32);
    m_service = service;
    m_action = action;
    m_msgType = msgType;
    m_rootDatalink = DataLink::DATA_LINK_UNKNOWN;
    m_rootTransType = TransType::TRANS_TYPE_UNKNOWN;
}

JsonMessage::JsonMessage(const JsonMessage &cp)
{
}

void JsonMessage::ClearAll()
{
    m_service = Service::UNKNOWN;
    m_action = Action::ACT_UNKNOWN;
    m_msgType = MsgType::MSG_TYPE_UNKNOWN;
    m_rootDatalink = DataLink::DATA_LINK_UNKNOWN;
    m_rootTransType = TransType::TRANS_TYPE_UNKNOWN;

    m_reqID.clear();
}

string JsonMessage::GetService()
{
    return string();
}

string JsonMessage::GetAction()
{
    return string();
}

string JsonMessage::GetRootDataLink()
{
    return string();
}

string JsonMessage::GetRootTransType()
{
    return string();
}

string JsonMessage::GetReqTopic()
{
    string topic(m_prefix);
    topic.append(string("/") + JsonMessage::GetServiceStringID[this->m_service]);
    topic.append(string("/") + JsonMessage::GetMsgTypeStringID[this->m_msgType]);
    topic.append(string("/") + JsonMessage::GetDataLinkStringID[JsonMessage::DataLink::UPLINK]);
    topic.append(string("/") + JsonMessage::GetActionStringID[this->m_action]);
    topic.append(string("/") + JsonMessage::GetTransStringID[JsonMessage::TransType::REQUEST]);
    return topic;
}

string JsonMessage::GetResTopic()
{
    string topic(m_prefix);
    topic.append(string("/") + JsonMessage::GetServiceStringID[this->m_service]);
    topic.append(string("/") + JsonMessage::GetMsgTypeStringID[this->m_msgType]);
    if (m_rootDatalink == JsonMessage::DataLink::DOWNLINK)
    {
        topic.append(string("/") + JsonMessage::GetDataLinkStringID[JsonMessage::DataLink::UPLINK]);
    }
    else
    {
        topic.append(string("/") + JsonMessage::GetDataLinkStringID[JsonMessage::DataLink::DOWNLINK]);
    }

    topic.append(string("/") + JsonMessage::GetActionStringID[this->m_action]);
    topic.append(string("/") + JsonMessage::GetTransStringID[JsonMessage::TransType::RESPONSE]);
    return topic;
}

void JsonMessage::SetPrefixTopic(string Prefix)
{
    m_prefix.clear();
    m_prefix = Prefix;
}

string JsonMessage::GetAckTopic()
{
    return string();
}

string JsonMessage::CreateTopic(
    Service service,
    Action action,
    DataLink rootDatalink,
    TransType rootTransType)
{
    return string();
}

void JsonMessage::SetConfig(Service service, Action action, MsgType msgType)
{
    m_service = service;
    m_action = action;
    m_msgType = msgType;
}

void JsonMessage::SetReqID(string reqID)
{
    m_reqID = reqID;
}

string JsonMessage::GetReqID()
{
    return m_reqID;
}

void JsonMessage::Swap(JsonMessage &other)
{
}

string JsonMessage::GetDataContent()
{
    return m_dataContent;
}

string JsonMessage::GetFullMessage()
{
    string message("{\"");
    message.append(TRANSFER_ID_KEY);
    message.append("\":\"");
    message.append(m_reqID);
    message.append("\",\"");
    message.append(DATA_KEY);
    message.append("\":");
    message.append(m_dataContent);
    message.append("}");
    return message;
}

void JsonMessage::SetDataContent(string data)
{
    m_dataContent.clear();
    m_dataContent.append(data);
    ESP_LOGI(JSON_MESSAGE_MODULE, "SetDataContent ---> %s", m_dataContent.c_str());
}

void JsonMessage::SetMessage(string message)
{
    m_message.clear();
    m_message = message;
}