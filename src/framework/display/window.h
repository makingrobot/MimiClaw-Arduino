/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef WINDOW_H
#define WINDOW_H

#include <string>

class Window {
public:
    virtual void SetStatus(const std::string& status) = 0;
    virtual void SetText(uint8_t line, const std::string& text) = 0;

};

class NoWindow : public Window {
public:
    virtual void SetStatus(const std::string& status) override { };
    virtual void SetText(uint8_t line, const std::string& text) override { };
    
};

#endif //WINDOW_H
