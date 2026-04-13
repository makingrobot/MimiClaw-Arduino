/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include "display.h"
#include <lvgl.h>
#include <string>
#include "disp_driver.h"
#include "lvgl_style.h"
#include "lvgl_window.h"
#include "lvgl_statusbar.h"
#include "../fonts/font_emoji.h"

#include <atomic>

class LvglDisplay : public Display {
protected:
    DispDriver* driver_ = nullptr;
    DisplayFonts fonts_;

    std::string current_theme_name_;
    ThemeColors current_theme_;

    // 容器
    lv_obj_t* container_ = nullptr;

    // 弹出
    lv_obj_t* low_battery_popup_ = nullptr;
    lv_obj_t* low_battery_label_ = nullptr;
    
    LvglWindow* window_ = nullptr;
    LvglStatusBar* statusbar_ = nullptr;

    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

    std::chrono::system_clock::time_point last_status_update_time_;
    
    virtual void SetupUI();
    
public:
    LvglDisplay(DispDriver* driver, DisplayFonts fonts);
    virtual ~LvglDisplay();

    void Init() override;
    void Rotate(uint8_t rotation) override;
    
    void SetWindow(LvglWindow* window);
    LvglWindow* GetWindow() override { return window_; }

    // override
    void SetStatus(const std::string& status) override;
    void SetText(const std::string& text) override;
    void UpdateStatusBar(bool update_all = false) override;
    void ShowNotification(const std::string &notification, int duration_ms = 3000) override;
    void Sleep() override { }
   
    // Add theme switching function
    virtual void SetTheme(const std::string& theme_name);
    virtual std::string GetTheme() { return current_theme_name_; }

};

#endif // LVGL_DISPLAY_H
#endif // CONFIG_USE_LVGL
