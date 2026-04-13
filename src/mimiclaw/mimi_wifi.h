/*
 * MimiClaw - WiFi Manager
 */
#ifndef MIMI_WIFI_H
#define MIMI_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

class MimiWiFi {
public:
    MimiWiFi();
    bool begin();
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
    Preferences _prefs;

    static void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);
    static MimiWiFi* _instance;
    void _onConnected(IPAddress ip);
    void _onDisconnected(uint8_t reason);

    int _retryCount;
};

#endif // MIMI_WIFI_H
