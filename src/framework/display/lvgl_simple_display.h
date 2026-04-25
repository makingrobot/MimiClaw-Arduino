/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#ifndef LVGL_SIMPLE_DISPLAY_H
#define LVGL_SIMPLE_DISPLAY_H

#include "lvgl_display.h"
#include <lvgl.h>
#include <string>
#include "disp_driver.h"
#include "lvgl_style.h"
#include "lvgl_statusbar.h"
#include "../fonts/font_emoji.h"

#include <atomic>

class LvglSimpleDisplay : public LvglDisplay {
protected:
    std::string current_theme_name_;
    ThemeColors current_theme_;

    // 容器
    lv_obj_t* container_ = nullptr;

    lv_obj_t* content_ = nullptr;
    lv_obj_t* text_label_ = nullptr;

    // 弹出
    lv_obj_t* low_battery_popup_ = nullptr;
    lv_obj_t* low_battery_label_ = nullptr;
    
    LvglStatusBar* statusbar_ = nullptr;

    std::chrono::system_clock::time_point last_status_update_time_;
    
    virtual void SetupUI();
    
public:
    LvglSimpleDisplay(DispDriver* driver, DisplayFonts fonts);
    virtual ~LvglSimpleDisplay();

    virtual void OnInit() override;
    virtual void Rotate(uint8_t rotation) override;
    
    // override
    virtual void SetStatus(const std::string& status) override;
    virtual void SetText(const std::string& text) override;
    virtual void UpdateStatusBar(bool update_all = false) override;
    virtual void Sleep() override { }
   
    // Add theme switching function
    virtual void SetTheme(const std::string& theme_name);
    virtual std::string GetTheme() { return current_theme_name_; }

};

#endif // LVGL_SIMPLE_DISPLAY_H
#endif 
