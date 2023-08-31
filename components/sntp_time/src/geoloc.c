#include "geoloc.h"


static const char *TAG = "geo location";

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

esp_err_t geoloc_request(geoloc_config_t *config, geoloc_result_t *result)
{
    char output_buffer[MAX_HTTP_RECV_BUFFER] = {0}; // Buffer to store response of http request
    int content_length = 0;
    esp_http_client_config_t http_config = {
        .url = "https://api.ipgeolocation.io/timezone",
        .event_handler = http_event_handler,
    };
    char api_url[256];
    snprintf(api_url, sizeof(api_url), "%s?apiKey=%s&ip=%s", http_config.url, config->api_key, config->ip_address);
    http_config.url = api_url;
    #if DEBUG
    printf("%s\n", http_config.url);
    #endif
    esp_http_client_handle_t client = esp_http_client_init(&http_config);

    // GET Request
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    }
    else
    {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0)
        {
            ESP_LOGE(TAG, "HTTP client fetch headers failed");
        }
        else
        {
            int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_RECV_BUFFER);
            if (data_read >= 0)
            {
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                         esp_http_client_get_status_code(client),
                         esp_http_client_get_content_length(client));
                #if DEBUG
                printf("\n%s\n", output_buffer);
                #endif
                output_buffer[data_read] = '\0';
                cJSON *root = cJSON_Parse(output_buffer);
                if (root != NULL)
                {
                    cJSON *timezone_json = cJSON_GetObjectItem(root, "timezone");
                    if (timezone_json != NULL)
                    {
                        result->timezone = strdup(timezone_json->valuestring);
                        ESP_LOGE(TAG, "Timezone: %s", result->timezone);
                        cJSON_Delete(root);
                        esp_http_client_cleanup(client);
                        return ESP_OK;
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Timezone field not found in JSON");
                    }
                    cJSON_Delete(root);
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
                }
            }
            else
            {
                ESP_LOGE(TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_close(client);
    return ESP_FAIL;
}

esp_err_t get_public_ip(char* ip_address)
{
    char output_buffer[100] = {0};
    int content_length = 0;
    esp_http_client_config_t config = {
        .url = "https://httpbin.org/ip",
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    // GET Request
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    }
    else
    {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0)
        {
            ESP_LOGE(TAG, "HTTP client fetch headers failed");
        }
        else
        {
            int data_read = esp_http_client_read_response(client, output_buffer, 100);
            if (data_read >= 0)
            {
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                         esp_http_client_get_status_code(client),
                         esp_http_client_get_content_length(client));
                #if DEBUG
                printf("%s\n", output_buffer);
                #endif
                cJSON *root = cJSON_Parse(output_buffer);
                if (root != NULL)
                {
                    cJSON *origin_json = cJSON_GetObjectItem(root, "origin");
                    if (origin_json != NULL)
                    {
                        char *origin = origin_json->valuestring;
                        ESP_LOGE(TAG, "Public IP address: %s", origin);
                        strcpy(ip_address, origin);
                    }
                    cJSON_Delete(root);
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
                }

                esp_http_client_cleanup(client);
                return ESP_OK;
            }
        }
    }
    esp_http_client_close(client);
    return ESP_FAIL;
}

esp_err_t get_timezone(char* ip_address, geoloc_result_t* result)
{
    geoloc_config_t config = {
        .ip_address = ip_address,
        .api_key = API_KEY,
    };
    esp_err_t err = geoloc_request(&config, result);
    if (err == ESP_OK)
    {
        if (result->timezone != NULL)
        {
            ESP_LOGI(TAG, "Successfully get timezone");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get timezone");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to request timezone");
    }
    return err;
}