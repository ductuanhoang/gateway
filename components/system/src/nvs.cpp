#include "common.h"
#include "nvs_header.h"
#include "esp_log.h"
#define TAG "NVS"

void nvs_init(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/********************************************************************
 *  name        Namespace name.
 *  key         Key name.
 *  Out_Byte    Read Byte.
 * ******************************************************************/
uint8_t Read_Byte(const char *name, const char *key, uint32_t *Out_Byte)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return 1;
    }
    else
    {
        err = nvs_get_u32(my_handle, key, (uint32_t *)Out_Byte);
        if (err != ESP_OK)
        {
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                // ESP_LOGE(TAG, "%s:%s is not initialized yet!\n", name, key);
            }
            nvs_close(my_handle);
            return 1;
        }
        nvs_close(my_handle);
    }
    return 0;
}

esp_err_t Read_Blob(const char *name, const char *key, uint8_t *data, size_t *length)
{
    nvs_handle_t my_handle;
    uint8_t *data_t = NULL;
    esp_err_t err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        err = nvs_get_blob(my_handle, key, NULL, length);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            return err;
        if (*length == 0)
        {
            ESP_LOGE(TAG, "%s:%s Nothing saved yet!\n", name, key);
        }
        else
        {
            data_t = (uint8_t *)malloc(*length);
            err = nvs_get_blob(my_handle, key, data_t, length);
            switch (err)
            {
            case ESP_OK:
                memcpy(data, data_t, *length);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE(TAG, "%s:%s not initialized yet!\n", name, key);
                break;
            default:
                ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
            }
            free(data_t);
        }
        nvs_close(my_handle);
    }
    return err;
}

void NVS_Read_String(const char *name, const char *key, char *Out_String, size_t *length)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        err = nvs_get_str(my_handle, key, NULL, length);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            return;
        if (length == 0)
        {
            ESP_LOGE(TAG, "%s:%s Nothing saved yet!\n", name, key);
        }
        else
        {
            err = nvs_get_str(my_handle, key, Out_String, length);
            switch (err)
            {
            case ESP_OK:
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE(TAG, "%s:%s not initialized yet!\n", name, key);
                break;
            default:
                ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
            }
        }
        nvs_close(my_handle);
    }
}

/********************************************************************
 *  name        Namespace name.
 *  key         Key name.
 *  Out_Byte    Byte to write.
 * ******************************************************************/
uint8_t Write_Byte(const char *name, const char *key, uint32_t Byte)
{
    nvs_handle_t my_handle;

    esp_err_t err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed Open!");
        return 1;
    }
    else
    {
        err = nvs_set_u32(my_handle, key, Byte);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed write!");
            nvs_close(my_handle);
            return 1;
        }
        err = nvs_commit(my_handle); // Commit written value.
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed commit!");
            nvs_close(my_handle);
            return 1;
        }
        nvs_close(my_handle);
    }
    return 0;
}

esp_err_t Write_Blob(const char *name, const char *key, uint8_t *data, uint16_t len)
{
    nvs_handle_t my_handle;
    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    ESP_LOGD(TAG, "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)", nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    esp_err_t err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        err = nvs_set_blob(my_handle, key, data, len);
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Failed write!\n");
        err = nvs_commit(my_handle);
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Failed commit!\n");
        nvs_close(my_handle);
    }
    return err;
}

void NVS_Write_String(const char *name, const char *key, char *string)
{
    nvs_handle_t my_handle;
    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    ESP_LOGD(TAG, "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)", nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    esp_err_t err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        err = nvs_set_str(my_handle, key, string);
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Failed write!\n");
        err = nvs_commit(my_handle);
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Failed commit!\n");

        nvs_close(my_handle);
    }
}

uint8_t erase_key(const char *name, const char *key)
{
    nvs_handle_t my_handle;

    esp_err_t err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed Open!");
        return 1;
    }
    else
    {
        err = nvs_erase_key(my_handle, key);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Erase Failed!");
            nvs_close(my_handle);
            return 1;
        }
        err = nvs_commit(my_handle); // Commit written value.
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Erase Commit Failed");
            nvs_close(my_handle);
            return 1;
        }
        nvs_close(my_handle);
    }
    return 0;
}

void Erase_Flash(void)
{
    //   ESP_ERROR_CHECK(nvs_flash_erase());
    erase_key(NAMESPACE_EPC, KEY_SENSOR_SETTINGS);
    erase_key(PAIR_NS, PAIR_KEY);
    erase_key(NAMESPACE_WIFI_CRED, WIFI_SSID_KEY);
}
