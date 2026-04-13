/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_GFX_LIBRARY == 1

#include "gfx_window.h"

void GfxWindow::Setup(Arduino_GFX* driver) {
    driver_ = driver;

    driver_->fillScreen(RGB565_BLACK);
    driver_->setTextColor(RGB565_WHITE);
    //driver_->setTextSize(1);
}

void GfxWindow::SetStatus(const std::string& status) {
    status_ = status;

    Update();
}

void GfxWindow::SetText(uint8_t line, const std::string& text) {
    text_ = text;

    Update();
}

void GfxWindow::Update() {

    driver_->fillScreen(RGB565_BLACK);
    driver_->setCursor(0, 0);

    driver_->println(status_.c_str());
    driver_->println(text_.c_str());

}

#endif //CONFIG_USE_GFX_LIBRARY