/*
 * MimiClaw-Arduino - WiFi Manager
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_WIFI_H
#define MIMI_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include "mimi_prefs.h"

class MimiWiFi {
public:
    MimiWiFi();
    bool begin(MimiPrefs* prefs);
    bool start();
    bool waitConnected(uint32_t timeout_ms = 30000);
    bool isConnected();
    const char* getIP();

    void setCredentials(const char* ssid, const char* pass);
    void scanAndPrint();

private:
    String _ssid;
    String _pass;
    String _ipStr;
    bool _connected;
    MimiPrefs* _prefs;

    static void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    static MimiWiFi* _instance;
    void _onConnected(IPAddress ip);
    void _onDisconnected(uint8_t reason);

    int _retryCount;
};

#endif // MIMI_WIFI_H
