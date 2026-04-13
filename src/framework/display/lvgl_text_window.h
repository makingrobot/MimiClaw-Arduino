/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#ifndef LVGL_TEXT_WINDOW_H
#define LVGL_TEXT_WINDOW_H

#include "lvgl_window.h"
#include <string>

class LvglTextWindow : public LvglWindow {
public:
    LvglTextWindow();
    virtual ~LvglTextWindow();

    void SetupUI(lv_obj_t* container, const ThemeColors& theme, const DisplayFonts& fonts) override;
    void SetTheme(const ThemeColors& theme) override;
    
    void SetStatus(const std::string& status) override { };
    void SetText(uint8_t line, const std::string& text) override;

protected:

    // 内容区
    lv_obj_t* content_ = nullptr;
    lv_obj_t* text_label_ = nullptr;

    std::string text_;
    
};

#endif //LVGL_TEXT_WINDOW_H

#endif //CONFIG_USE_LVGL