/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_TFT_ESPI == 1

#ifndef TFT_WINDOW_H
#define TFT_WINDOW_H

#include <TFT_eSPI.h>
#include <string>
#include <vector>
#include "window.h"

typedef struct  {
    std::string text = "";
    uint16_t text_size = 4;
    uint16_t text_color = TFT_WHITE;
    uint16_t text_bg_color = TFT_BLACK;
    uint16_t y_pos = 0;
    uint16_t x_pos = 2;
} tft_line_t;

class TftWindow : public Window {
public:
    TftWindow() { }

    virtual void Setup(TFT_eSPI* driver);
    virtual void SetStatus(const std::string& status) override;
    virtual void SetText(uint8_t line, const std::string& text) override;

    void SetText(uint8_t line, const tft_line_t& line_t);

protected:
    TFT_eSPI* driver_ = nullptr;

private:
    std::string status_ = "";
    std::vector<tft_line_t> text_line_;
};

#endif //TFT_WINDOW_H

#endif //CONFIG_USE_TFT_ESPI