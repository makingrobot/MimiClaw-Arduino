/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_WIFI_CONFIGURE_WEBSERVER==1

#ifndef _WIFI_CONFIGURATION_IMPL_H
#define _WIFI_CONFIGURATION_IMPL_H

#include <string>
#include <Arduino.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include "../wifi/wifi_configuration.h"

class WifiConfigurationImpl : public WifiConfiguration {
public:
    WifiConfigurationImpl() { }
    virtual ~WifiConfigurationImpl();

protected:
    void StartWebServer() override;
    virtual void BindSsidRoute();
    virtual void BindAdvancedRoute();

    WebServer* webserver_ = nullptr;
    TaskHandle_t webserver_handle_;
    std::string payload_;

    // 高级配置项
    std::string ota_url_;
    int8_t max_tx_power_;
    bool remember_bssid_;

private:
    void LoadAdvancedConfig();

};

class LogMiddleware : public Middleware {
public:
    bool run(WebServer& server, Callback next) override;
};

#endif //_WIFI_CONFIGURATION_IMPL_H

#endif