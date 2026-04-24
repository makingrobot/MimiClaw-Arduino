/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#include "lvgl_display.h"

#include <vector>
#include <cstring>

#include "../sys/log.h"
#include "gfx_lvgl_driver.h"

#define TAG "LvglDisplay"

LvglDisplay::LvglDisplay(DispDriver* driver, DisplayFonts fonts)
        : driver_(driver), fonts_(fonts) {
            
    Log::Debug(TAG, "LvglDisplay constructor.");

    width_ = driver->width();
    height_ = driver->height();
}

LvglDisplay::~LvglDisplay() {

    driver_ = nullptr;
}

void LvglDisplay::Init() {
    Log::Info(TAG, "Init ......");

    driver_->Init();
    
    OnInit();

}

void LvglDisplay::Rotate(uint8_t rotation) {

}

bool LvglDisplay::Lock(int timeout_ms) {
    return true;
}

void LvglDisplay::Unlock() {

}

#endif // CONFIG_USE_LVGL
