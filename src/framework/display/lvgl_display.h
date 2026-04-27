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
#include "../fonts/font_emoji.h"
#include <atomic>

class LvglDisplay : public Display {
protected:
    DispDriver* driver_ = nullptr;
    DisplayFonts fonts_;

    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

    virtual void OnInit() = 0;

public:
    LvglDisplay(DispDriver* driver, DisplayFonts fonts);
    virtual ~LvglDisplay();

    virtual void Init() override;
    virtual void Rotate(uint8_t rotation) override;
    virtual void SetMessage(const String& kind, const String& text) { }
    
private:

};

#endif // LVGL_DISPLAY_H
#endif
