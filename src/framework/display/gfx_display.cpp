/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_GFX_LIBRARY == 1

#include "gfx_display.h"
#include "../sys/log.h"

#define TAG "GfxDisplay"

GfxDisplay::GfxDisplay(Arduino_GFX* driver, int width, int height) 
        : driver_(driver) {
            
    Log::Debug(TAG, "GfxDisplay constructor.");

    width_ = width;
    height_ = height;

}

void GfxDisplay::SetWindow(GfxWindow* window) {
    window_ = window;
}

bool GfxDisplay::Lock(int timeout_ms) {
    return true;
}
 
void GfxDisplay::Unlock() {
}

void GfxDisplay::Init() {
    Log::Info(TAG, "Init ......");

    if (!driver_->begin()) {
        Log::Error(TAG, "gfx-begin() failed!");
        return;
    }

    if (window_ == nullptr) {
        window_ = new GfxWindow();
    }
    window_->Setup(driver_);
}
    
void GfxDisplay::Rotate(uint8_t rotation) {
    driver_->setRotation(rotation);
}

void GfxDisplay::SetStatus(const std::string& status) {
    if (window_!=nullptr) {
        window_->SetStatus(status);
    }
}

void GfxDisplay::SetText(const std::string& text) {
    if (window_!=nullptr) {
        window_->SetText(text);
    }
}

#endif //CONFIG_USE_GFX_LIBRARY