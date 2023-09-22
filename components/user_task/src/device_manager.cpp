#include "device_manager.h"
#include <map>
#include "aws_task.h"
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_mqtt_provisioning.h"
#include "ArduinoJson.h"
#include "nvs_header.h"

/***********************************************************************************************************************
 * functions
 ***********************************************************************************************************************/
#define TAG_DEVICE_MANAGER "DEVICE"
// JsonMessage::JsonMessage(/* args */)
// {
// }

// JsonMessage::~JsonMessage()
// {
// }

// {
//   "state": {
//     "reported": {
//       "hubID": "1234",
//       "breakermate": {
// 		"id": 1 {
// 		"voltage": "220",
// 		"current": "10A",
// 		"power": "10KwH"
//         }

//     }

//   }
// }

// scan results
// {
//     "state": {
//         "reported": {
//             "hubID": "44444",
//             "scan_result": [{
//                     "id": "Sensor_1",
//                     "voltage": 220,
//                     "current": 8.001,
//                     "power": 1000
//                 }, {
//                     "id": "Sensor_2",
//                     "voltage": 220,
//                     "current": 8.001,
//                     "power": 1000
//                 }
//             ]
//         }
//     }
// }

// control
// {
//   "state": {
//     "desired": {
//       "command":
// 	  {
// 		"name": "OFF"
// 		"parameter": {"hubID": 44444, "breakermateID": "sensor1"}
// 	  }
//   }
// }

// Scan Command Schema:
// "{ "state": { "desired": { "command":  { "name": "SCAN", "parameter": {"hubID": "44444" } } } }}"

DeviceManager::DeviceManager(const std::string &Hub_id)
{
    m_Hub_Id = Hub_id;
    m_gateway_data.current = 0;
    m_gateway_data.voltage = 0;
    m_gateway_data.power = 0;
}

DeviceManager::~DeviceManager()
{
    m_Hub_Id.clear();
    m_gateway_data.current = 0;
    m_gateway_data.voltage = 0;
    m_gateway_data.power = 0;
}

void DeviceManager::setDeviceId(const std::string &ID)
{
    this->m_Hub_Id = ID;
}

std::string DeviceManager::getDeviceId(void)
{
    return this->m_Hub_Id;
}

static std::map<DeviceManager::MsgType, const char *> GetMsgTypeStringID = {
    {DeviceManager::MsgType::STATE, "state"},
    {DeviceManager::MsgType::REPORTED, "reported"},
    {DeviceManager::MsgType::DESIRED, "desired"},
    {DeviceManager::MsgType::DELTA, "delta"}};

static std::map<DeviceManager::DataType, const char *> GetDataTypeStringID = {
    {DeviceManager::DataType::ID, "id"},
    {DeviceManager::DataType::VOLTAGE, "voltage"},
    {DeviceManager::DataType::CURRENT, "current"},
    {DeviceManager::DataType::POWER, "power"},
    {DeviceManager::DataType::STATUS, "operationalStatus"},
    {DeviceManager::DataType::SOURCE, "source"},
    {DeviceManager::DataType::PRIMARY, "primary"},
    {DeviceManager::DataType::SECONDARY, "secondary"},
};

static std::map<DeviceManager::DataDevice, const char *> GetDeviceNameStringID = {
    {DeviceManager::DataDevice::HUB_ID, "hubID"},
    {DeviceManager::DataDevice::BREAKERMATE, "breakermate"}};

static std::map<DeviceManager::Controltype, const char *> GetControlTypeStringID = {
    {DeviceManager::Controltype::CONTROL, "CONTROL"},
    {DeviceManager::Controltype::SCAN, "SCAN"},
    {DeviceManager::Controltype::ADD, "ADD_SENSOR"},
    {DeviceManager::Controltype::DELETE, "DELETE_SENSOR"}};

void DeviceManager::deviceReportDataPoint(std::string DP, EndDeviceData_t data)
{
    // {"id":"1","voltage":"220","current":"10A","power":"10KwH"}
    DynamicJsonDocument doc(1024);

    doc[GetDataTypeStringID[DeviceManager::DataType::ID]] = DP;
    doc[GetDataTypeStringID[DeviceManager::DataType::VOLTAGE]] = data.voltage;
    doc[GetDataTypeStringID[DeviceManager::DataType::CURRENT]] = data.current;
    doc[GetDataTypeStringID[DeviceManager::DataType::POWER]] = data.power;
    doc[GetDataTypeStringID[DeviceManager::DataType::STATUS]] = data.status;

    setDataContent(doc.as<std::string>());
}

void DeviceManager::setGateWayData(EndDeviceData_t data)
{
    m_gateway_data = data;
}

void DeviceManager::clearGateWayData(void)
{
    m_gateway_data.voltage = 0;
    m_gateway_data.current = 0;
    m_gateway_data.power = 0;
    m_gateway_data.status = true;
    m_gateway_data.source_power_1 = false;
    m_gateway_data.source_power_2 = false;
}

void DeviceManager::deviceReportScanDataPoint(std::string id_node)
{
    // {"id":"1","voltage":"220","current":"10A","power":"10KwH"}
    DynamicJsonDocument doc(256);

    doc["id"] = id_node;

    setScanNodeDataContent(doc.as<std::string>());
}

std::string DeviceManager::deviceReportScanResult(void)
{
    DynamicJsonDocument doc(1024);
    JsonObject state = doc.createNestedObject(GetMsgTypeStringID[DeviceManager::MsgType::STATE]);
    JsonObject reported = state.createNestedObject(GetMsgTypeStringID[DeviceManager::MsgType::REPORTED]);
    reported[GetDeviceNameStringID[DeviceManager::DataDevice::HUB_ID]] = this->m_Hub_Id;

    reported[GetDataTypeStringID[DeviceManager::DataType::VOLTAGE]] = m_gateway_data.voltage;
    reported[GetDataTypeStringID[DeviceManager::DataType::CURRENT]] = m_gateway_data.current;
    reported[GetDataTypeStringID[DeviceManager::DataType::POWER]] = m_gateway_data.power;
    reported[GetDataTypeStringID[DeviceManager::DataType::STATUS]] = m_gateway_data.status;
    JsonArray scan_result = reported.createNestedArray("scan_result");
    DynamicJsonDocument dataPointDoc(JSON_OBJECT_SIZE(23));
    for (int i = 0; i < this->m_ScanNodeContent.size(); i++)
    {
        DeserializationError error = deserializeJson(dataPointDoc, this->m_ScanNodeContent.at(i).c_str());
        if (error)
        {
            ESP_LOGI(TAG_DEVICE_MANAGER, "DeserializeJson() response Payload failed: %s", error.c_str());
        }
        else
            scan_result.add(dataPointDoc.as<JsonObject>());
    }

    char buffer[1024];

    serializeJson(doc, &buffer, sizeof(buffer));
    return buffer;
}

std::string DeviceManager::deviceReportAllDataPoints(void)
{
    DynamicJsonDocument doc(1024);
    JsonObject state = doc.createNestedObject(GetMsgTypeStringID[DeviceManager::MsgType::STATE]);
    JsonObject reported = state.createNestedObject(GetMsgTypeStringID[DeviceManager::MsgType::REPORTED]);
    reported[GetDeviceNameStringID[DeviceManager::DataDevice::HUB_ID]] = this->m_Hub_Id;

    reported[GetDataTypeStringID[DeviceManager::DataType::VOLTAGE]] = m_gateway_data.voltage;
    reported[GetDataTypeStringID[DeviceManager::DataType::CURRENT]] = m_gateway_data.current;
    reported[GetDataTypeStringID[DeviceManager::DataType::POWER]] = m_gateway_data.power;
    reported[GetDataTypeStringID[DeviceManager::DataType::STATUS]] = m_gateway_data.status;

    JsonObject source_power = reported.createNestedObject(GetDataTypeStringID[DeviceManager::DataType::SOURCE]);

    source_power[GetDataTypeStringID[DeviceManager::DataType::PRIMARY]] = m_gateway_data.source_power_1;
    source_power[GetDataTypeStringID[DeviceManager::DataType::SECONDARY]] = m_gateway_data.source_power_2;

    JsonArray breakermate = reported.createNestedArray(GetDeviceNameStringID[DeviceManager::DataDevice::BREAKERMATE]);
    DynamicJsonDocument dataPointDoc(JSON_OBJECT_SIZE(23));
    for (int i = 0; i < this->m_dataContent.size(); i++)
    {
        DeserializationError error = deserializeJson(dataPointDoc, this->m_dataContent.at(i).c_str());
        if (error)
        {
            ESP_LOGI(TAG_DEVICE_MANAGER, "DeserializeJson() response Payload failed: %s", error.c_str());
        }
        else
            breakermate.add(dataPointDoc.as<JsonObject>());
    }

    char buffer[1024];

    serializeJson(doc, &buffer, sizeof(buffer));
    return buffer;
}

EndDeviceData_t DeviceManager::devicePasserMessage_02(std::string messge_read)
{
    EndDeviceData_t end_device_data_buffer;
    memset(&end_device_data_buffer, 0x00, sizeof(end_device_data_buffer));

    DynamicJsonDocument doc(500);
    DeserializationError error = deserializeJson(doc, messge_read);
    if (error)
    {
        ESP_LOGI(TAG_DEVICE_MANAGER, "deserializeJson() failed: ");
        return end_device_data_buffer;
    }
    ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read() : %s", messge_read.c_str());

    const char *Hub_id = doc[GetMsgTypeStringID[DeviceManager::MsgType::STATE]][GetMsgTypeStringID[DeviceManager::MsgType::DESIRED]][GetDeviceNameStringID[DeviceManager::DataDevice::HUB_ID]];
    this->m_Hub_Id = Hub_id;
    if (strcmp(this->m_Hub_Id.c_str(), Hub_id) == 0)
    {
        end_device_data_buffer.id = doc[GetMsgTypeStringID[DeviceManager::MsgType::STATE]][GetMsgTypeStringID[DeviceManager::MsgType::DESIRED]][GetDeviceNameStringID[DeviceManager::DataDevice::BREAKERMATE]][GetDataTypeStringID[DeviceManager::DataType::ID]];
        end_device_data_buffer.voltage = doc[GetMsgTypeStringID[DeviceManager::MsgType::STATE]][GetMsgTypeStringID[DeviceManager::MsgType::DESIRED]][GetDeviceNameStringID[DeviceManager::DataDevice::BREAKERMATE]][GetDataTypeStringID[DeviceManager::DataType::VOLTAGE]];
        end_device_data_buffer.current = doc[GetMsgTypeStringID[DeviceManager::MsgType::STATE]][GetMsgTypeStringID[DeviceManager::MsgType::DESIRED]][GetDeviceNameStringID[DeviceManager::DataDevice::BREAKERMATE]][GetDataTypeStringID[DeviceManager::DataType::CURRENT]];
        end_device_data_buffer.power = doc[GetMsgTypeStringID[DeviceManager::MsgType::STATE]][GetMsgTypeStringID[DeviceManager::MsgType::DESIRED]][GetDeviceNameStringID[DeviceManager::DataDevice::BREAKERMATE]][GetDataTypeStringID[DeviceManager::DataType::POWER]];

        ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read id : %d", end_device_data_buffer.id);
        ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read voltage : %lf", end_device_data_buffer.voltage);
        ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read current : %lf", end_device_data_buffer.current);
        ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read power : %lf", end_device_data_buffer.power);
    }
    else
    {
        ESP_LOGE(TAG_DEVICE_MANAGER, "Wrong Hub ID the current is %s but the message is %s", this->m_Hub_Id.c_str(), Hub_id);
    }
    return end_device_data_buffer;
}

// {"state":{"desired":{"command":{"name":"OFF","parameter":{"hubID":44444,"breakermateID":"Sensor_1"}}}},"metadata":{"desired":{"command":{"name":{"timestamp":1685881405},"parameter":{"hubID":{"timestamp":1685881405},"breakermateID":{"timestamp":1685881405}}}}},"version":1142,"timestamp":1685881405}
EndDeviceData_t DeviceManager::devivePasserMessage(std::string messge_read)
{
    EndDeviceData_t end_device_data_buffer;
    memset(&end_device_data_buffer, 0x00, sizeof(end_device_data_buffer));

    DynamicJsonDocument doc(500);
    DeserializationError error = deserializeJson(doc, messge_read);
    if (error)
    {
        ESP_LOGI(TAG_DEVICE_MANAGER, "deserializeJson() failed: ");
        return end_device_data_buffer;
    }
    ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read() : %s", messge_read.c_str());

    const char *Hub_id = doc[GetMsgTypeStringID[DeviceManager::MsgType::STATE]][GetMsgTypeStringID[DeviceManager::MsgType::DESIRED]]["command"]["parameter"][GetDeviceNameStringID[DeviceManager::DataDevice::HUB_ID]];
    // const char *Hub_id = doc["state"]["desired"]["command"]["parameter"]["hubID"];
    if (Hub_id)
    {
        this->m_Hub_Id = Hub_id;
        ESP_LOGI(TAG_DEVICE_MANAGER, "Hub id : %s", Hub_id);
        if (strcmp(this->m_Hub_Id.c_str(), Hub_id) == 0)
        {
            ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read() this->m_Hub_Id: %s", this->m_Hub_Id.c_str());
            const char *command = doc["state"]["desired"]["command"]["name"];
            if (command != NULL)
                ESP_LOGI(TAG_DEVICE_MANAGER, "command id : %s", command);
            if (strcmp(command, GetControlTypeStringID[DeviceManager::Controltype::CONTROL]) == 0)
            {
                end_device_data_buffer.id_command = 1;
                end_device_data_buffer.status = doc["state"]["desired"]["command"]["parameter"]["state"];

                const char *id_name = doc["state"]["desired"]["command"]["parameter"]["breakermateID"];
                snprintf(end_device_data_buffer.id_name, sizeof(end_device_data_buffer.id_name), "%s", id_name);
                ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read id : %s", end_device_data_buffer.id_name);
                ESP_LOGI(TAG_DEVICE_MANAGER, "messge_read status : %d", end_device_data_buffer.status);
            }
            else if (strcmp(command, GetControlTypeStringID[DeviceManager::SCAN]) == 0)
            {
                // set event scanning
                end_device_data_buffer.id_command = 2;
                ESP_LOGI(TAG_DEVICE_MANAGER, "call event scan");
            }
            else if (strcmp(command, GetControlTypeStringID[DeviceManager::DELETE]) == 0)
            {
                end_device_data_buffer.id_command = 3;
                ESP_LOGI(TAG_DEVICE_MANAGER, "call event delete");
                const char *id_name = doc["state"]["desired"]["command"]["parameter"]["breakermateID"];
                snprintf(end_device_data_buffer.id_name, sizeof(end_device_data_buffer.id_name), "%s", id_name);

                ESP_LOGI(TAG_DEVICE_MANAGER, "delete command id : %s", end_device_data_buffer.id_name);
            }
            else if (strcmp(command, GetControlTypeStringID[DeviceManager::ADD]) == 0)
            {
                end_device_data_buffer.id_command = 4;
                const char *id_name = doc["state"]["desired"]["command"]["parameter"]["breakermateID"];
                snprintf(end_device_data_buffer.id_name, sizeof(end_device_data_buffer.id_name), "%s", id_name);
                ESP_LOGI(TAG_DEVICE_MANAGER, "add command id : %s", end_device_data_buffer.id_name);
            }
        }
        else
        {
            ESP_LOGE(TAG_DEVICE_MANAGER, "Wrong Hub ID the current is %s but the message is %s", this->m_Hub_Id.c_str(), Hub_id);
        }
    }
    else
        ESP_LOGE(TAG_DEVICE_MANAGER, "Failed to get device Id");

    return end_device_data_buffer;
}

// Publish to : $aws/things/<hub_id>/shadow/update
// subscribe to $aws/things/<hub_id>/shadow/update/delta
std::string DeviceManager::deviceReportTopic(std::string Hub_id)
{
    std::string Topic;
    Topic = "$aws/things/" + Hub_id + "/shadow/update";
    return Topic;
}

std::string DeviceManager::deviceSubTopic(std::string Hub_id)
{
    std::string Topic;
    Topic = "$aws/things/" + Hub_id + "/shadow/name/command/update/accepted";
    return Topic;
}

void DeviceManager::setDataContent(std::string data)
{
    this->m_dataContent.push_back(data);
    // ESP_LOGI(TAG_DEVICE_MANAGER, "Array ---> %s", this->m_dataContent.c_str());
}

void DeviceManager::setScanNodeDataContent(std::string data)
{
    this->m_ScanNodeContent.push_back(data);
    // ESP_LOGI(TAG_DEVICE_MANAGER, "Array ---> %s", this->m_dataContent.c_str());
}

void DeviceManager::freeDataContent(void)
{
    this->m_dataContent.clear();
}

void DeviceManager::freeScanContent(void)
{
    this->m_ScanNodeContent.clear();
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/