/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_GFX_LIBRARY == 1

#ifndef _GFX_DISPLAY_H
#define _GFX_DISPLAY_H

#include "display.h"
#include <Arduino_GFX_Library.h>
#include "gfx_window.h"

class GfxDisplay : public Display {
public:
    GfxDisplay(Arduino_GFX *driver_, int width, int height);

    void Init() override;
    void Rotate(uint8_t rotation) override;
    
    void SetStatus(const std::string& status) override;
    void SetText(const std::string& text) override;
    void UpdateStatusBar(bool update_all = false) override { }
    void ShowNotification(const std::string &notification, int duration_ms = 3000) override { }
    void Sleep() override { }
   
    void SetWindow(GfxWindow* window);
    GfxWindow* GetWindow() override { return window_; }

    const Arduino_GFX* gfx() const { return driver_; }

protected:
    bool Lock(int timeout_ms = 0) override;
    void Unlock() override;

private:
    Arduino_GFX *driver_ = nullptr;
    GfxWindow* window_ = nullptr;

};

#endif //_TFT_DISPLAY_H

#endif //CONFIG_USE_TFT_ESPI