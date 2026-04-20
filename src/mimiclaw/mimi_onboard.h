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

class MimiOnboard {
public:
    MimiOnboard();
    bool start(bool admin=false);
    void stop();
    
private:
    WebServer *_webserver = nullptr;
    TaskHandle_t _taskHandle;

    void handleRoot();
    void handleScan();
    void handleConfig();
    void handleSave();
    String _saved_data;

    static void webTask(void* arg);
    static String getSsid();
};

#endif // MIMI_ONBOARD_H