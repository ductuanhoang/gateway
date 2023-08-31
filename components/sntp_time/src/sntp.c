#include "sntp.h"

static const char *TAG = "sntp";

void initialize_sntp()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void obtain_time(char* tz_location)
{
    initialize_sntp();
    if (tz_location)
    {
        ESP_LOGE(TAG, "Setting timezone to %s", tz_location);
        setenv("TZ", tz_location, 1);
        tzset();
    }
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2021 - 1900) && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    if (retry == retry_count)
    {
        ESP_LOGE(TAG, "Unable to set system time from SNTP server");
    }
    else
    {
        ESP_LOGI(TAG, "Time is set from SNTP server to %s", asctime(&timeinfo));
    }
}