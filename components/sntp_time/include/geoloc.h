#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"

#define DEBUG 1

#define MAX_HTTP_RECV_BUFFER 1024
#define API_KEY "4a4e79aeed8c48d1843aec3f27088a72" //"3c225810ef6a4ef1865d7dae72eb7bba"

    typedef struct
    {
        char *ip_address;
        char *api_key;
    } geoloc_config_t;

    typedef struct
    {
        char *timezone;
    } geoloc_result_t;

    esp_err_t geoloc_request(geoloc_config_t *config, geoloc_result_t *result);
    esp_err_t get_public_ip(char *ip_address);
    esp_err_t get_timezone(char *ip_address, geoloc_result_t *result);

#ifdef __cplusplus
}
#endif

