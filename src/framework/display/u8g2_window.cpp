/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_U8G2 == 1

#include "u8g2_window.h"

void U8g2Window::Setup(U8G2* driver, const uint8_t* fonts) {
    driver_ = driver;
    driver_->enableUTF8Print();
    driver_->setFont(fonts);

    // 状态栏和内容区的分隔线
    driver_->drawLine(0, 20, 128, 20);
    driver_->drawLine(0, 21, 128, 21);
}

void U8g2Window::SetStatus(const std::string& status) {
    status_ = status;

    Update();
}

void U8g2Window::SetText(uint8_t line, const std::string& text) {
    if (line < 1 || line > 3) {
        return;
    }

    text_line_[line-1] = text;

    Update();
}

void U8g2Window::Update() {
    
    driver_->clearBuffer();

    driver_->setCursor(4, 14);   
    driver_->print(status_.c_str()); 

    driver_->drawLine(0, 16, 128, 16);
    
    for (uint8_t i=0; i<3; i++) {
        driver_->setCursor(4, (i+2)*16-2); 
        driver_->print(text_line_[i].c_str());
    }

    driver_->sendBuffer();
}

#endif //CONFIG_USE_U8G2