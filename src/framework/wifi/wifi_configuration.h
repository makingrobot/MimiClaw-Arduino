/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_WIFI_CONFIGURE_ENABLE==1

#ifndef _WIFI_CONFIGURATION_H
#define _WIFI_CONFIGURATION_H

#include <string>
#include <IPAddress.h>

class WifiConfiguration {
public:
    WifiConfiguration() { }

    virtual void SetLanguage(const char* lang_code);
    virtual void SetSsidPrefix(const char* ssid_prefix);

    std::string GetSsid();
    std::string GetWebServerUrl();
    
    virtual void Start();

protected:
    virtual void StartWebServer() = 0;

    std::string GetAvailableAPList();
    std::string GetSavedAPList();
    bool ConnectToWifi(const std::string &ssid, const std::string &password);

    const char* lang_code_;
    const char* ssid_prefix_;

};

#endif //_WIFI_CONFIGURATION_H

#endif //CONFIG_WIFI_CONFIGURE_ENABLE