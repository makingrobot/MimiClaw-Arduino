/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <Arduino.h>
#include <chrono>
#include <string>
#include "config.h"
#include "../sys/log.h"

class Display {
public:
    /**
     * 初始化处理
     */
    virtual void Init() = 0;
    /**
     * 旋转屏幕
     */
    virtual void Rotate(uint8_t rotation) = 0;

    virtual void SetStatus(const String& status) = 0;
    virtual void SetText(const String& text) = 0;
    virtual void UpdateStatusBar(bool update_all = false) = 0;
    virtual void Sleep() = 0;

    inline int width() const { return width_; }
    inline int height() const { return height_; }

protected:
    int width_ = 0;
    int height_ = 0;
    
    friend class DisplayLockGuard;
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;

private:

};

class DisplayLockGuard {
public:
    DisplayLockGuard(Display *display) : display_(display) {
        if (!display_->Lock(30000)) {
            Log::Error("Display", "Failed to lock display");
        }
    }
    ~DisplayLockGuard() {
        display_->Unlock();
    }

private:
    Display *display_;
};

class NoDisplay : public Display {
public:
    void Init() override { }
    void Rotate(uint8_t rotation) override { }
    void SetStatus(const String& status) override { }
    void SetText(const String& text) override { }
    void UpdateStatusBar(bool update_all = false) override { }
    void Sleep() override { }

protected:
    bool Lock(int timeout_ms = 0) override { return true; }
    void Unlock() override { }

};

#endif
