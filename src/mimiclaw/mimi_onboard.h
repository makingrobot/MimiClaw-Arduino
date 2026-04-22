/*
 * MimiClaw-Arduino - Onboard
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_ONBOARD_H
#define MIMI_ONBOARD_H

#include <Arduino.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "mimi_prefs.h"

class MimiOnboard {
public:
    MimiOnboard();
    bool begin(MimiPrefs* prefs);
    bool start(bool admin=false);
    void stop();
    
private:
    MimiPrefs *_prefs = nullptr;
    WebServer *_webserver = nullptr;
    TaskHandle_t _taskHandle;

    void handleRoot();
    void handleScan();
    void handleConfig();
    void handleSave();
    String _saved_data;

    static void webTask(void* arg);
    static String getSsid();

    void addConfigItem(JsonDocument &doc, const char *json_key,
                                      const char *ns, const char *key, const char *build_val);
    void addConfigItemUint(JsonDocument &doc, const char *json_key,
                                      const char *ns, const char *key, const char *build_val);
};

#endif // MIMI_ONBOARD_H