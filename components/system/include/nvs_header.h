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

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // NVS_HEADER_H_
