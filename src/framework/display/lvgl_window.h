/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#ifndef LVGL_WINDOW_H
#define LVGL_WINDOW_H

#include <string>
#include <lvgl.h>
#include "lvgl_style.h"
#include "window.h"

class LvglWindow : public Window {
public:
    virtual ~LvglWindow() = default;

    virtual void SetupUI(lv_obj_t* container, const ThemeColors& theme, const DisplayFonts& fonts) = 0;
    virtual void SetTheme(const ThemeColors& theme) = 0;

};

#endif //LVGL_WINDOW_H

#endif //CONFIG_USE_LVGL