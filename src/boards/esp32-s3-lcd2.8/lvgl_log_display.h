/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if BOARD_ESP32_S3_LCD_2_8==1 && CONFIG_USE_LVGL == 1

#ifndef LVGL_LOG_DISPLAY_H
#define LVGL_LOG_DISPLAY_H

#include <lvgl.h>
#include <string>
#include <chrono>
#include "src/framework/display/lvgl_display.h"
#include "src/framework/display/lvgl_statusbar.h"

#define LV_FONT_PUHUI  //覆盖style中定义的字体
LV_FONT_DECLARE(font_puhui_16_4);

#include "src/framework/display/lvgl_style.h"

class LvglLogDisplay : public LvglDisplay {
public:
    LvglLogDisplay(DispDriver* driver, DisplayFonts fonts);
    virtual ~LvglLogDisplay();

    virtual void OnInit() override;
    virtual void Rotate(uint8_t rotation) override;
    
    virtual void SetStatus(const std::string& status) override;
    virtual void SetText(const std::string& text) override;
    virtual void UpdateStatusBar(bool update_all = false) { }
    virtual void Sleep() { }

    void SetMessage(const String& kind, const String& text) override;
    void AppendMessage(const String& kind, const String& text) override;
    void SetTheme(const std::string& theme_name);
    
private:
    lv_obj_t* textarea_ = nullptr;
    std::vector<String> lines_;

    std::string current_theme_name_;
    ThemeColors current_theme_;

    // 容器
    lv_obj_t* container_ = nullptr;

    // 弹出
    lv_obj_t* low_battery_popup_ = nullptr;
    lv_obj_t* low_battery_label_ = nullptr;
    
    LvglStatusBar* statusbar_ = nullptr;

    std::chrono::system_clock::time_point last_status_update_time_;
    
    virtual void SetupUI();

};

#endif // LVGL_LOG_DISPLAY_H
#endif 
