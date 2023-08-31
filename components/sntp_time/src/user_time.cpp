#include "time.h"
#include "timezone.h"
#include "sntp.h"
#include "geoloc.h"
#include "user_wifi.h"
#include "user_time.h"


static void TimeSynTask(void *param);

UserTime::UserTime(/* args */)
{
    this->m_time = 0;
    // initialize sntp server
    char *tz_location = NULL;

    memset(ip_address, 0x00, sizeof(ip_address));
}

UserTime::~UserTime()
{
}

void UserTime::startTasks(void)
{
    int ret = 0;
    ret = xTaskCreatePinnedToCore(&TimeSynTask, "TimeSynTask", 4096, nullptr, 5, nullptr, 1);
}

uint32_t UserTime::getTime(void)
{
    uint32_t time = 0;
    int ret = -1;
    get_public_ip(ip_address);
    geoloc_result_t result = {
        .timezone = NULL,
    };

    ret = get_timezone(ip_address, &result);
    if (ret == ESP_OK)
    {
        this->tz_location = getTzByLocation(result.timezone);
        obtain_time(this->tz_location);
    }

    return time;
}

static void TimeSynTask(void *param)
{
    int ret = -1;
    UserTime time;
    while (1)
    {
        if (user_wifi_get_connected() == E_USER_WIFI_CONNECTED)
        {
            ret = time.getTime();
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}