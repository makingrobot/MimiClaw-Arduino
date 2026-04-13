/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_U8G2 == 1

#ifndef U8G2_WINDOW_H
#define U8G2_WINDOW_H

#include <U8g2lib.h>
#include <string>
#include "window.h"

class U8g2Window : public Window {
public:
    U8g2Window() { }

    virtual void Setup(U8G2* driver, const uint8_t* fonts);
    virtual void SetStatus(const std::string& status) override;
    virtual void SetText(uint8_t line, const std::string& text) override;

protected:
    U8G2* driver_ = nullptr;

private:
    void Update();

    std::string status_ = "";
    std::string text_line_[3] = {"", "", ""};
};

#endif //U8g2_WINDOW_H

#endif //CONFIG_USE_U8G2