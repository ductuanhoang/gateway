/**
 * this file storage struct node info
 */

#include "storage.h"
#include "esp_log.h"
#include <string>

#define TAG "USER_STORAGE"
#define NAMESPACE_NODE_INFO "NODE_INFO"
#define NAMESPACE_JSON_MESSAGE "MESSAGE"

#define PREFIX_SENSOR "SENSOR_"
typedef struct
{
    std::string node_name; // key with sensor_id
    uint8_t uuid[16];
    uint8_t onoff; // onoff state
    uint16_t unicast;
} node_info_t;

static int user_storage_save_node_info(std::string name, const node_info_t *node);
static std::string print_hex(uint8_t *a, int size);
static int user_storage_get_node_info(std::string name, node_info_t *node);

node_info_t node_1 = {
    .node_name = "SENSOR_1",
    // 32 10 08 b6 1f 34 db 12
    .uuid = {0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0xfa},
    .unicast = 0x0006};

void user_storage_init(void)
{
    nvs_init();
}

void user_storage_test(void)
{
    node_info_t node, node_2;
    ESP_LOGI(TAG, "node 1 unicast = 0x%02x", node_1.unicast);
    ESP_LOGI(TAG, "node 1 uuid: %s", print_hex(node_1.uuid, 8).c_str());

    user_storage_save_node_info("SENSOR_1", &node_1);

    user_storage_get_node_info("SENSOR_1", &node);
    int ret = user_storage_get_node_info("SENSOR_2", &node_2);
    if (ret != 0)
        ESP_LOGE(TAG, "Error read sensor info 2");

    ESP_LOGI(TAG, "node unicast = 0x%02x", node.unicast);
    ESP_LOGI(TAG, "device uuid: %s", print_hex(node.uuid, 8).c_str());
}

/**
 * save node info to nvs internal flash
 */
static int user_storage_save_node_info(std::string name, const node_info_t *node)
{
    std::string prefix_uuid = "uuid";
    std::string prefix_unicast = "unicast";
    std::string prefix_state = "on/off";

    int ret = -1;
    ret = Write_Blob(NAMESPACE_NODE_INFO, (const char *)((name + prefix_uuid).c_str()), (uint8_t *)node->uuid, 8);
    if (ret == 0)
        ESP_LOGI(TAG, "Save successfull save uuid");

    ret = Write_Byte(NAMESPACE_NODE_INFO, (const char *)((name + prefix_unicast).c_str()), (uint32_t)node->unicast);
    if (ret == 0)
        ESP_LOGI(TAG, "Save successfull save unicast");

    ret = Write_Byte(NAMESPACE_NODE_INFO, (const char *)((name + prefix_state).c_str()), (uint32_t)node->onoff);
    if (ret == 0)
        ESP_LOGI(TAG, "Save successfull save unicast");

    return ret;
}

/**
 *
 */
static int user_storage_get_node_info(std::string name, node_info_t *node)
{
    std::string prefix_uuid = "uuid";
    std::string prefix_state = "on/off";
    std::string prefix_unicast = "unicast";

    int ret = -1;
    size_t length = 0;
    uint32_t test = 0;
    uint32_t state = 0;

    ret = Read_Blob(NAMESPACE_NODE_INFO, (const char *)((name + prefix_uuid).c_str()), (uint8_t *)node->uuid, &length);
    if (ret != 0)
        ESP_LOGE(TAG, "Read fail uuid");

    ret = Read_Byte(NAMESPACE_NODE_INFO, (const char *)((name + prefix_unicast).c_str()), &test);
    node->unicast = test;
    if (ret != 0)
        ESP_LOGE(TAG, "Read fail unicast");

    ret = Read_Byte(NAMESPACE_NODE_INFO, (const char *)((name + prefix_state).c_str()), &state);
    node->onoff = state;
    if (ret != 0)
        ESP_LOGE(TAG, "Read fail state");
    return ret;
}

static int user_storage_delete_node_info(std::string name)
{
    int ret = -1;
    std::string prefix_uuid = "uuid";
    std::string prefix_state = "on/off";
    std::string prefix_unicast = "unicast";

    ret = erase_key(NAMESPACE_NODE_INFO, (const char *)((name + prefix_uuid).c_str()));
    if (ret != 0)
        ESP_LOGE(TAG, "Delete fail uuid");

    ret = erase_key(NAMESPACE_NODE_INFO, (const char *)((name + prefix_state).c_str()));
    if (ret != 0)
        ESP_LOGE(TAG, "Delete fail state");

    ret = erase_key(NAMESPACE_NODE_INFO, (const char *)((name + prefix_unicast).c_str()));
    if (ret != 0)
        ESP_LOGE(TAG, "Delete fail unicast");

    return ret;
}

int user_storage_save_json_message(uint16_t id, std::string message)
{

    int ret = NVS_Write_String(NAMESPACE_JSON_MESSAGE, "id", (char *)(message.c_str()));
    return ret;
}

std::string user_storage_read_json_message(uint16_t id)
{
    std::string message;
    uint16_t length = 0;
    // int ret = NVS_Read_String(NAMESPACE_JSON_MESSAGE, "id", &message, &length);


    return message;
}
/**
 *
 */
static std::string print_hex(uint8_t *a, int size)
{
    char buffer[10];
    int i;
    std::string s = "";
    for (i = 0; i < size; i++)
    {
        sprintf((char *)buffer, "%02x", a[i]);
        s = s + buffer;
    }
    return s;
}