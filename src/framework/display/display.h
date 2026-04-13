/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <string>
#include <chrono>
#include <string>
#include "config.h"
#include "../sys/log.h"
#include "window.h"

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

    virtual void SetStatus(const std::string& status) = 0;
    virtual void SetText(const std::string& text) = 0;
    virtual void UpdateStatusBar(bool update_all = false) = 0;
    virtual void ShowNotification(const std::string &notification, int duration_ms = 3000) = 0;
    virtual void Sleep() = 0;
    virtual Window* GetWindow() = 0;

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
    NoDisplay() { window_ = new NoWindow(); }
    ~NoDisplay() { delete window_; }

    void Init() override { }
    void Rotate(uint8_t rotation) override { }
    void SetStatus(const std::string& status) override { }
    void SetText(const std::string& text) override { }
    void UpdateStatusBar(bool update_all = false) override { }
    void ShowNotification(const std::string &notification, int duration_ms = 3000) override { }
    void Sleep() override { }
    Window* GetWindow() override { return window_; }

protected:
    bool Lock(int timeout_ms = 0) override { return true; }
    void Unlock() override { }

private:
    Window *window_;

};

#endif
