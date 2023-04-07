#include "aws_task.h"
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

static const char *AWS_IOT = "Aws-Iot";

#define DELAY_AWS_TASK 15 * 1000

#define MQTT_SETTINGS_SEND_MIN_INTERVAL 5
#define MQTT_SUBSCIBE_QOS QOS1
#define MQTT_PUBLISH_QOS QOS1

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;
/**
 * 
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;
char cPayload[2048] = {0x00};
char receivedPayload[2048] = {0x00};

char aws_root_ca_pem_start[2048] = {0};
char certificate_pem_crt_start[2048] = {0};
char private_pem_key_start[2048] = {0};

IoT_Error_t aws_iot_init()
{
    IoT_Error_t rc = FAILURE;

    ESP_LOGI(AWS_IOT, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;
}
