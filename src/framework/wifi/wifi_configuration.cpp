/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_WIFI_CONFIGURE_ENABLE==1

#include "wifi_configuration.h"
#include <WiFi.h>
#include <cJSON.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include "ssid_manager.h"
#include "../sys/log.h"

static const IPAddress ap_ip(192,168,5,1);
static const IPAddress ap_gateway(192,168,5,1);
static const IPAddress ap_subnet(255,255,255,0);

#define TAG "WifiConfiguration"

void WifiConfiguration::SetLanguage(const char* lang_code) {
    lang_code_ = lang_code;
}

void WifiConfiguration::SetSsidPrefix(const char* ssid_prefix) {
    ssid_prefix_ = ssid_prefix;
}

void WifiConfiguration::Start() {

    std::string ap_ssid = GetSsid();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
    if (!WiFi.softAP(ap_ssid.c_str())) {
        Log::Info(TAG, "Soft AP creation failed.");
        while(1);
    }
    Log::Info(TAG, "Wifi AP %s on %s", ap_ssid.c_str(), ap_ip.toString());

    StartWebServer();
}

std::string WifiConfiguration::GetAvailableAPList() {
    std::string json_str = "[";

    int n = WiFi.scanNetworks();

    for (int i=0; i<n; ++i) {
        Log::Debug(TAG, "SSID: %s, RSSI: %d, Authmode: %d",
            WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.encryptionType(i) );
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"ssid\":\"%s\",\"rssi\":%d,\"authmode\":%d}",
            WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.encryptionType(i) );
        json_str += std::string(buf);
        if (i < n - 1) {
            json_str += ",";
        }
    }

    json_str += "]";
    
    WiFi.scanDelete();

    return json_str;
}

std::string WifiConfiguration::GetSavedAPList() {
    auto ssid_list = SsidManager::GetInstance().GetSsidList();
    std::string json_str = "[";
    for (const auto& ssid : ssid_list) {
        json_str += "\"" + ssid.ssid + "\",";
    }
    if (json_str.length() > 1) {
        json_str.pop_back(); // Remove the last comma
    }
    json_str += "]";
    return json_str;
}

std::string WifiConfiguration::GetSsid() {
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    char ssid[32];
    snprintf(ssid, sizeof(ssid)-1, "%s-%02x%02x", ssid_prefix_, mac[4], mac[5]);
    return std::string(ssid);
}

std::string WifiConfiguration::GetWebServerUrl() {
    return std::string(ap_ip.toString().c_str());
}

bool WifiConfiguration::ConnectToWifi(const std::string &ssid, const std::string &password)
{
    if (ssid.empty()) {
        Log::Warn(TAG, "SSID cannot be empty");
        return false;
    }
    
    if (ssid.length() > 32) {  // WiFi SSID 最大长度
        Log::Warn(TAG, "SSID too long");
        return false;
    }
    
    if (WiFi.isConnected()) {
        WiFi.disconnect();
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    Log::Info(TAG, "connect to WiFi %s", ssid.c_str());
    int n = 0;
    // 最多等待10s
    while (++n < 100) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (WiFi.status() == WL_CONNECTED) {
            break; //已连接，中止等待
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Log::Warn(TAG, "Failed to connect to WiFi %s", ssid.c_str());
        return false;
    }

    Log::Info(TAG, "Connected to WiFi %s", ssid.c_str());
    return true;
}

#endif //CONFIG_WIFI_CONFIGURE_ENABLE