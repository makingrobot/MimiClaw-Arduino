/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_TFT_ESPI == 1

#include "tft_window.h"

void TftWindow::Setup(TFT_eSPI* driver) {
    driver_ = driver;

    driver_->fillScreen(TFT_BLACK);
}

void TftWindow::SetStatus(const std::string& status) {
    status_ = status;

    driver_->setCursor(4, 20, 2);
    driver_->println(status_.c_str());
}

void TftWindow::SetText(uint8_t line, const std::string& text) {
    tft_line_t line_t;
    line_t.text = text;

    SetText(line, line_t);
}

void TftWindow::SetText(uint8_t line, const tft_line_t& line_t) {
    if (line > text_line_.size()) {
        text_line_.resize(line);
    }
    text_line_[line-1] = line_t;

    int y_pos = 2;
    for (const tft_line_t& ln : text_line_) {
        driver_->setCursor(ln.x_pos, y_pos, ln.text_size);
        driver_->setTextColor(ln.text_color, ln.text_bg_color);
        driver_->println(ln.text.c_str());
        y_pos += driver_->fontHeight(ln.text_size) + 2;
    }
}

#endif //CONFIG_USE_TFT_ESPI