#ifndef NVS_HEADER_H_
#define NVS_HEADER_H_

#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_header.h"

#define NAMESPACE_CONFIG "DEV_CONFIG"
#define KEY_DEVICE_CONFIG "NVS_CONFIG"
#define KEY_ROOT_CA_CERT "NVS_CRT_ROOT"
#define KEY_PEM_CERT "NVS_CRT_DCERT"
#define KEY_PEM_KEY "NVS_CRT_DKEY"
#define NVS_KEY_PROVISIONED "DEV_PROVISIONED"
#define NVS_DEVICE_PASSWORD "DEV_PASSWORD"

#define NAMESPACE_EPC "DEV_SETTINGS"
#define KEY_SENSOR_SETTINGS "SENSOR_SETTINGS"

#define PAIR_NS "PAIRNS"
#define PAIR_KEY "PAIRKEY"

#define NAMESPACE_WIFI_CRED "nvs.net80211"
#define WIFI_SSID_KEY "sta.ssid"
#define WIFI_PASS_KEY "sta.pass"

#define NAMESPACE_BLE_MESH "BLE_MESH"
#define NUMBER_OF_DEVICES "ble_num" // number of devices provisioned
#define BLE_DEVICE_KEY_1 "info_1"
#define BLE_DEVICE_KEY_2 "info_2"
#define BLE_DEVICE_KEY_3 "info_3"
#define BLE_DEVICE_KEY_4 "info_4"
#define BLE_DEVICE_KEY_5 "info_5"
#define BLE_DEVICE_KEY_6 "info_6"
#define BLE_DEVICE_KEY_7 "info_7"
#define BLE_DEVICE_KEY_8 "info_8"
#define BLE_DEVICE_KEY_9 "info_9"
#define BLE_DEVICE_KEY_10 "info_10"

#ifdef __cplusplus
extern "C"
{
#endif

    void nvs_init(void);
    void Erase_Flash(void);
    uint8_t erase_key(const char *name, const char *key);
    uint8_t Read_Byte(const char *name, const char *key, uint32_t *Out_Byte);
    uint8_t Write_Byte(const char *name, const char *key, uint32_t Byte);
    void NVS_Write_String(const char *name, const char *key, char *string);
    void NVS_Read_String(const char *name, const char *key, char *Out_String, size_t *length);
    void write_deviceID(uint32_t deviceID);
    char *read_deviceID();
    esp_err_t Write_Blob(const char *name, const char *key, uint8_t *data, uint16_t len);
    esp_err_t Read_Blob(const char *name, const char *key, uint8_t *data, size_t *length);

    /**
     * @brief
     *
     * @param name
     * @param key
     * @param Out_String
     * @param length
     * @return int
     */
    int nvs_read_node_config_name(const char *name, const char *key, char *Out_String, size_t *length);

    /**
     * @brief
     *
     * @param name
     * @param key
     * @param string
     */
    void nvs_save_node_config_name(const char *name, const char *key, char *string);
    /**
     * @brief
     *
     * @param name
     * @param key
     * @param out
     * @return int
     */
    int nvs_read_node_config_number(const char *name, const char *key, uint32_t *out);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // NVS_HEADER_H_
