/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_TFT_ESPI == 1

#include "tft_display.h"
#include "../sys/log.h"

#define TAG "TftDisplay"

TftDisplay::TftDisplay(TFT_eSPI* driver, int width, int height) {
    Log::Debug(TAG, "TftDisplay constructor.");

    driver_ = driver;
    width_ = width;
    height_ = height;
}

void TftDisplay::SetWindow(TftWindow* window) {
    window_ = window;
}

bool TftDisplay::Lock(int timeout_ms) {
    return true;
}
 
void TftDisplay::Unlock() {
}

void TftDisplay::Init() {
    Log::Info(TAG, "Init ......");

    driver_->begin();
    
    if (window_ == nullptr) {
        window_ = new TftWindow();
    }
    window_->Setup(driver_);
}
    
void TftDisplay::Rotate(uint8_t rotation) {
    driver_->setRotation(rotation);
}

void TftDisplay::SetStatus(const std::string& status) {
    if (window_!=nullptr) {
        window_->SetStatus(status);
    }
}

void TftDisplay::SetText(const std::string& text) {
    if (window_!=nullptr) {
        window_->SetText(1, text);
    }
}

#endif //CONFIG_USE_TFT_ESPI