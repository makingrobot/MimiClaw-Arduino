/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_WIFI==1

#ifndef _WIFI_STATION_H
#define _WIFI_STATION_H

#include <functional>
#include <string>

class WifiStation {
public:
    static WifiStation& GetInstance() {
        static WifiStation instance;
        return instance;
    }

    void OnScanBegin(std::function<void()> callback);
    void OnConnect(std::function<void(std::string&)> callback);
    void OnConnected(std::function<void(std::string&)> callback);

    void Start();
    void Stop();
    bool WaitForConnected(uint32_t timeout_ms);
    bool WaitForConnected(const std::string& ssid, const std::string& password, uint32_t timeout_ms);

    bool IsConnected() { return is_connected_; }
    std::string GetSsid() { return current_ssid_; }

    int8_t GetRssi();
    uint8_t GetChannel();

    void SetPowerSaveMode(bool enabled);
    std::string GetIpAddress();

private:
    std::string current_ssid_;
    bool is_connected_;

    std::function<void()> on_scanbegin_handler_;
    std::function<void(std::string&)> on_connect_handler_;
    std::function<void(std::string&)> on_connected_handler_;

};

#endif //_WIFI_STATION_H

#endif