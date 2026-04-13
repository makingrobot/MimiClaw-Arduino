/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_TFT_ESPI == 1

#ifndef _TFT_DISPLAY_H
#define _TFT_DISPLAY_H

#include "display.h"
#include <TFT_eSPI.h>
#include "tft_window.h"

class TftDisplay : public Display {
public:
    TftDisplay(TFT_eSPI *driver_, int width, int height);

    void Init() override;
    void Rotate(uint8_t rotation) override;

    void SetWindow(TftWindow* window);
    TftWindow* GetWindow() override { return window_; }
    
    void SetStatus(const std::string& status) override;
    void SetText(const std::string& text) override;
    void UpdateStatusBar(bool update_all = false) override { }
    void ShowNotification(const std::string &notification, int duration_ms = 3000) override { }
    void Sleep() override { }
   
    const TFT_eSPI* tft() const { return driver_; }

protected:
    bool Lock(int timeout_ms = 0) override;
    void Unlock() override;

private:
    TFT_eSPI *driver_ = nullptr;
    TftWindow* window_ = nullptr;

};

#endif //_TFT_DISPLAY_H

#endif //CONFIG_USE_TFT_ESPI