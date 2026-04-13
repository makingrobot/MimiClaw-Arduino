/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _BUTTON_H
#define _BUTTON_H

#include <string>

class Button {
public:
    Button(const std::string& name) : name_(name) { }

    const std::string& name() const { return name_; }

    virtual void BindAction(ButtonAction action) { }

    virtual void OnClick(std::function<void()> click_func) { }

    virtual void OnClick(std::function<void(void*)> click_param_func, void *parameter) { }

    virtual void OnDoubleClick(std::function<void()> doubleclick_func) { }

    virtual void OnDoubleClick(std::function<void(void*)> doubleclick_param_func, void *parameter) { }

    virtual void OnLongPress(std::function<void()> longpress_func) { }

    virtual void OnLongPress(std::function<void(void*)> longpress_func, void *parameter) { }

    virtual void SetLongPressIntervalMs(uint32_t interval) { }

    virtual bool IsPressed() { return false; }

    virtual void Tick() { }

private:
    const std::string name_;

};

#endif //_BUTTON_H