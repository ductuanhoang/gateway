#include "Utils.h"
#include "common.h"
#include <esp_netif.h>
#include "esp_system.h"

int generate_seed_ = 0;

string Utils::GenID(int length) {
    char sessionId[length + 1];
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    srandom((int)time(nullptr) * 10000 + generate_seed_); generate_seed_++;
    for (auto& character : sessionId) {
        character = alphanum[random() % (sizeof(alphanum) - 1)];
    }

    sessionId[length] = 0;
    return string(sessionId);
}

void Utils::DelayMs(int time) {
    vTaskDelay(time / portTICK_PERIOD_MS);
}

void Utils::Delay(int time) {
    DelayMs(time*1000);
}

string Utils::GetMac() {
    //Get the derived MAC address for each network interface
    uint8_t derived_mac_addr[6] = {0};
    //Get MAC address for WiFi Station interface
    ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA));
    char mac_char[13];
    sprintf(mac_char, 
        "%x%x%x%x%x%x", 
        derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
        derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);
    mac_char[12] = 0;
    string mac(mac_char);
    return mac;
}

vector<string> Utils::Split(string str_in, const char* str_split) {
    vector<string> out;
    string delimiter = str_split;
    size_t pos = 0;
    string token;
    while ((pos = str_in.find(delimiter)) != string::npos) {
        token = str_in.substr(0, pos);
        out.push_back(token);
        str_in.erase(0, pos + delimiter.length());
    }
    out.push_back(str_in);
    return out;
}

string Utils::GetCurrentIP() {
    tcpip_adapter_ip_info_t ipInfo; 
    char str[256];
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    sprintf(str, "" IPSTR "", IP2STR(&ipInfo.ip));
    string ip(str);
    return ip;
}

string Utils::GetCurrentGw() {
    tcpip_adapter_ip_info_t ipInfo; 
    char str[256];
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    sprintf(str, "" IPSTR "", IP2STR(&ipInfo.gw));
    string ip(str);
    return ip;
}

string Utils::GetCurrentNetmask() {
    tcpip_adapter_ip_info_t ipInfo; 
    char str[256];
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    sprintf(str, "" IPSTR "", IP2STR(&ipInfo.netmask));
    string ip(str);
    return ip;
}