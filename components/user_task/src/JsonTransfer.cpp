#include "JsonTransfer.h"
#include "esp_log.h"
#include "Utils.h"

static const char* TAG = "JSON_TRANS";


void responseHandler (AWS_IoT_Client *pClient, char *pTopicName, uint16_t topicNameLen,
									  IoT_Publish_Message_Params *pParams, void *pClientData)

{
    std::string topic(pTopicName, pTopicName + topicNameLen);
    vector<string> action_resp = Utils::Split(topic, "/");
    if ((action_resp[action_resp.size()-1] == "resp")) {
        ESP_LOGI(TAG,"Response handler --> topic: %s", topic.c_str());
        ESP_LOGI(TAG,"Response handler --> payload: %s", (char*)pParams->payload);
    }
}

JsonTransfer::JsonTransfer(
        const string& transferID,
        AwsAgentSession& session,
        JsonMessage::Service service,
        JsonMessage::Action action,
        JsonMessage::MsgType msgType,
        JsonMessage::DataLink rootDatalink, 
        JsonMessage::TransType rootTransType
) :     JsonMessage(transferID, service, action, msgType, rootDatalink, rootTransType)
        , m_session(session)

{
}

JsonTransfer::JsonTransfer(
        AwsAgentSession& session,
        JsonMessage::Service service,
        JsonMessage::Action action,
        JsonMessage::MsgType msgType
) :     JsonMessage(service, action, msgType),
        m_session(session),
        m_reqPayload(string())
{}

string JsonTransfer::Request(string payload, int timeout) {
    string reqTopic = this->GetReqTopic();
    string resTopic = this->GetResTopic();
    int rc = m_session.publish(reqTopic, payload);
    ESP_LOGI(TAG, "Send Request --> Topic: %s", reqTopic.c_str());
    ESP_LOGI(TAG, "Send Request --> Payload: %s", payload.c_str());
    if (rc != 0) {
        ESP_LOGI(TAG, "AWS Request failes --> mqtt error:%d\n", rc);
        return string();
    }
    m_session.subscribe(resTopic, responseHandler);
    Utils::Delay(timeout);
    ESP_LOGI(TAG, "Send Request timeout");
    return string();
}

string JsonTransfer::Request(bool forward, int timeout) {
    string reqTopic = this->GetReqTopic();
    string resTopic;
    string resPayload = string();
    m_reqPayload = this->GetFullMessage();
    int rc = m_session.publish(reqTopic, m_reqPayload);
    ESP_LOGI(TAG, "Send Request --> Topic: %s", reqTopic.c_str());
    ESP_LOGI(TAG, "Send Request --> Payload: %s", m_reqPayload.c_str());
    if (rc != 0) {
        ESP_LOGI(TAG, "AWS Request failes --> mqtt error:%d\n", rc);
    }
    if (!forward) {
        resTopic = this->GetResTopic();
        resPayload = m_session.WaitRes(resTopic);
        if (resPayload.empty()) {
                ESP_LOGI(TAG, "Send Request timeout");
        }
    }
    return resPayload;
}

size_t JsonTransfer::SetRequestPlayload(DynamicJsonDocument dataDoc) {
    return serializeJson(dataDoc, m_reqPayload);
}

size_t JsonTransfer::SetMessage(DynamicJsonDocument payload) {
    return serializeJson(payload, m_reqPayload);
}


void JsonTransfer::Response() {
    string resTopic = this->GetResTopic();
    m_reqPayload = this->GetFullMessage();
    int rc = m_session.publish(resTopic, m_reqPayload);
    ESP_LOGI(TAG, "Send Response --> Topic: %s", resTopic.c_str());
    ESP_LOGI(TAG, "Send Response --> Payload: %s", m_reqPayload.c_str());
    if (rc != 0) {
        ESP_LOGI(TAG, "AWS Response failes --> mqtt error:%d\n", rc);
    }
}

int JsonTransfer::SendAck() {
    return 1;
}
