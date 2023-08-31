
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_sntp.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"

 void obtain_time(char *tz_location);

#ifdef __cplusplus
}
#endif
