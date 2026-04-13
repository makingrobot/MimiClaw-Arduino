/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_U8G2 == 1

#include "u8g2_display.h"
#include "../sys/log.h"

#define TAG "U8g2Display"

U8g2Display::U8g2Display(U8G2* driver, int width, int height, const uint8_t* fonts) 
        : driver_(driver), fonts_(fonts) {
            
    Log::Debug(TAG, "U8g2Display constructor.");
    width_ = width; 
    height_ = height;
}

bool U8g2Display::Lock(int timeout_ms) {
    return true;
}
 
void U8g2Display::Unlock() {
    
}

void U8g2Display::Sleep() {
    driver_->sleepOn();
}

void U8g2Display::SetWindow(U8g2Window* window) {
    window_ = window;
}

void U8g2Display::Init() {
    Log::Info(TAG, "Init ......");
    
    driver_->begin();
    if (window_==nullptr) {
        window_ = new U8g2Window();
    }
    window_->Setup(driver_, fonts_);
}
    
void U8g2Display::Rotate(uint8_t rotation) {
    //driver_=>set
}

void U8g2Display::SetStatus(const std::string& status) {
    if (window_!=nullptr) {
        window_->SetStatus(status);
    }
}

void U8g2Display::SetText(const std::string& text) {
    if (window_!=nullptr) {
        window_->SetText(1, text);
    }
}

#endif //CONFIG_USE_U8G2