#include "user_wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define USER_DEFAULT_ESP_WIFI_SSID "tuan123"
#define USER_DEFAULT_ESP_WIFI_PASS "123456789"

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/**
 * @brief Construct a new user wifi set wifi status object
 *
 * @param status
 */
static void user_wifi_set_wifi_status(e_wifi_status status);
static void user_wifi_task(void *param);

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static e_wifi_status user_wifi_status = E_USER_WIFI_DISCONNECTED;

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *WIFI_TAG = "USER WIFI";
static int s_retry_num = 0;

static wifi_config_t wifi_config = {
    .sta = {
        .ssid = USER_DEFAULT_ESP_WIFI_SSID,
        .password = USER_DEFAULT_ESP_WIFI_PASS,
        /* Setting a password implies station will connect to all security modes including WEP/WPA.
         * However these modes are deprecated and not advisable to be used. Incase your Access point
         * doesn't support WPA2, these mode can be enabled by commenting below line */
        .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
    },
};

/**
 * @brief
 *
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY)
        {
            user_wifi_set_wifi_status(E_USER_WIFI_DISCONNECTED);
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        user_wifi_set_wifi_status(E_USER_WIFI_CONNECTED);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief init wifi sta
 *
 */
void user_wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

/**
 * @brief
 *
 * @param ssid
 * @param password
 */
void user_wifi_connect_AP(const char *ssid, const char *password)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK)
        ESP_LOGE(WIFI_TAG, "WiFi is not initialized by esp_wifi_init.");

    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", ssid);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", password);

    ESP_LOGI(WIFI_TAG, "wifi_config.sta.ssid = %s", wifi_config.sta.ssid);
    ESP_LOGI(WIFI_TAG, "wifi_config.sta.password = %s", wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 ssid, password);
        user_wifi_set_wifi_status(E_USER_WIFI_CONNECTED);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid, password);
        user_wifi_set_wifi_status(E_USER_WIFI_FAIL);
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }

    xTaskCreate(user_wifi_task, "user_wifi_task", 1024 * 5, NULL, 5, NULL);

    // ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    // vEventGroupDelete(s_wifi_event_group);
}

/**
 * @brief
 *
 * @return e_wifi_status
 */
e_wifi_status user_wifi_get_connected(void)
{
    return user_wifi_status;
}

/**
 * @brief Construct a new user wifi set wifi status object
 *
 * @param status
 */
static void user_wifi_set_wifi_status(e_wifi_status status)
{
    user_wifi_status = status;
}

/**
 *
 */
static void user_wifi_task(void *param)
{
    while (1)
    {
        if (user_wifi_status == E_USER_WIFI_CONNECTED)
        {
        }
        else
        {
            ESP_LOGI(WIFI_TAG, "wifi reconnect task");
            esp_wifi_connect();
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}