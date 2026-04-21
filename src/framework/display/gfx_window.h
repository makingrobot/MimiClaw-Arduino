/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_GFX_LIBRARY == 1

#ifndef TFT_WINDOW_H
#define TFT_WINDOW_H

#include "src/libs/GFX_Library/Arduino_GFX_Library.h"
#include <string>
#include "window.h"

typedef struct  {
    std::string text = "";
    uint16_t text_size = 4;
    uint16_t text_color;
    uint16_t text_bg_color;
    uint16_t y_pos = 0;
    uint16_t x_pos = 2;
} gfx_line_t;

class GfxWindow : public Window {
public:
    virtual void Setup(Arduino_GFX* driver);
    virtual void SetStatus(const std::string& status) override;
    virtual void SetText(uint8_t line, const std::string& text) override;
   
    void SetText(uint8_t line, const gfx_line_t& line_t);
    void AppendText(const std::string& text);
    void AppendText(const gfx_line_t& line_t);

protected:
    Arduino_GFX* driver_ = nullptr;

private:
    void Update();
    
    std::string status_ = "";
    std::vector<gfx_line_t> text_line_;

    uint8_t max_line_ = 1;  //start from 1.
};

#endif //TFT_WINDOW_H

#endif //CONFIG_USE_TFT_ESPI