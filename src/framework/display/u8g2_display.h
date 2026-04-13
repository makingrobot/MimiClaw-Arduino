/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_U8G2 == 1

#ifndef _U8G2_DISPLAY_H
#define _U8G2_DISPLAY_H

#include "display.h"
#include <U8g2lib.h>
#include <string>
#include "u8g2_window.h"

class U8g2Display : public Display {
public:
    U8g2Display(U8G2* u8g2, int width, int height, const uint8_t* fonts);

    void Init() override;
    void Rotate(uint8_t rotation) override;
    
    void SetStatus(const std::string& status) override;
    void SetText(const std::string& text) override;
    void UpdateStatusBar(bool update_all = false) override { }
    void ShowNotification(const std::string &notification, int duration_ms = 3000) override { }
    void Sleep() override;
   
    void SetWindow(U8g2Window* window);
    U8g2Window* GetWindow() override { return window_; }

    const U8G2* u8g2() const { return driver_; }

protected:
    bool Lock(int timeout_ms = 0) override;
    void Unlock() override;

private:
    U8G2* driver_ = nullptr;
    const uint8_t* fonts_ = nullptr;
    U8g2Window* window_ = nullptr;

};

#endif //_U8G2_DISPLAY_H

#endif //_CONFIG_USE_U8G2