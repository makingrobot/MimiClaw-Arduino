/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */

#include <Arduino.h>
#include "config.h"
#include "board_def.h"
#include "src/framework/sys/log.h"
#include "src/framework/app/application.h"

#define TAG "Main"

Application *app = nullptr;

void setup() {
    Serial.begin(115200);

    Log::Info(TAG, "application start on core %d", xPortGetCoreID());

    // 创建应用实例
    app = &Application::GetInstance();

    app->Init();
}

void loop() {
    app->Loop();
}
