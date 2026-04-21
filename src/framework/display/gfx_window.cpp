/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_GFX_LIBRARY == 1

#include "gfx_window.h"

void GfxWindow::Setup(Arduino_GFX* driver) {
    driver_ = driver;

    driver_->fillScreen(RGB565_BLACK);
    driver_->setTextColor(RGB565_WHITE);
    //driver_->setTextSize(1);
}

void GfxWindow::SetStatus(const std::string& status) {
    status_ = status;

    driver_->setCursor(4, 4);
    driver_->println(status_.c_str());
}

void GfxWindow::SetText(uint8_t line, const std::string& text) {
    gfx_line_t line_t;
    line_t.text = text;

    SetText(line, line_t);
}

void GfxWindow::SetText(uint8_t line, const gfx_line_t& line_t) {
    if (line > text_line_.size()) {
        text_line_.resize(line);
    }
    text_line_[line-1] = line_t;

    if (line > max_line_) {
        max_line_ = line;
    }

    int y_pos = 2;
    for (const gfx_line_t& ln : text_line_) {
        driver_->setCursor(ln.x_pos, y_pos);
        driver_->setTextSize(ln.text_size);
        driver_->setTextColor(ln.text_color, ln.text_bg_color);
        driver_->println(ln.text.c_str());
        y_pos += ln.text_size + 2;
    }
}

void GfxWindow::AppendText(const std::string& text) {
    gfx_line_t line_t;
    line_t.text = text;

    AppendText(line_t);
}

void GfxWindow::AppendText(const gfx_line_t& line_t) { 
    uint8_t line = max_line_;
    if (line > text_line_.size()) {
        text_line_.resize(line);
    }
    text_line_[line-1] = line_t;

    int y_pos = 2;
    for (const gfx_line_t& ln : text_line_) {
        driver_->setCursor(ln.x_pos, y_pos);
        driver_->setTextSize(ln.text_size);
        driver_->setTextColor(ln.text_color, ln.text_bg_color);
        driver_->println(ln.text.c_str());
        y_pos += ln.text_size + 2;
    }
}

#endif //CONFIG_USE_GFX_LIBRARY