/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#include "lvgl_text_window.h"
#include "../sys/log.h"
#include "../fonts/font_awesome_symbols.h"
#include "../lang/lang_zh_cn.h"

#define TAG "LvglTextWindow"

LvglTextWindow::LvglTextWindow() {
            
    Log::Debug(TAG, "LvglTextWindow constructor.");

}

LvglTextWindow::~LvglTextWindow() {

    if (content_ != nullptr) {
        lv_obj_del(content_);
    }
   
    if (text_label_ != nullptr) {
        lv_obj_del(text_label_);
    }

}

void LvglTextWindow::SetupUI(lv_obj_t* container, const ThemeColors& theme, const DisplayFonts& fonts) {
    Log::Info(TAG, "SetupUI ......");

    /* 内容区域 */
    content_ = lv_obj_create(container);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_width(content_, LV_HOR_RES);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_set_style_pad_all(content_, 5, 0);
    lv_obj_set_style_bg_color(content_, theme.background, 0);
    lv_obj_set_style_border_color(content_, theme.border, 0); // Border color for content

    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN); // 垂直布局（从上到下）
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY); // 子对象居中对齐，等距分布

    text_label_ = lv_label_create(content_);
    lv_label_set_text(text_label_, "Hello world!");
    lv_obj_set_width(text_label_, LV_HOR_RES * 0.9); // 限制宽度为屏幕宽度的 90%
    lv_label_set_long_mode(text_label_, LV_LABEL_LONG_WRAP); // 设置为自动换行模式
    lv_obj_set_style_text_align(text_label_, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_set_style_text_color(text_label_, theme.text, 0);
}

void LvglTextWindow::SetTheme(const ThemeColors& theme) {
    
    // Update content area colors
    if (content_ != nullptr) {
        lv_obj_set_style_bg_color(content_, theme.background, 0);
        lv_obj_set_style_border_color(content_, theme.border, 0);
        lv_obj_set_style_text_color(text_label_, theme.text, 0);
    }
    
    if (text_label_!=nullptr) {
        lv_obj_set_style_text_color(text_label_, theme.text, 0);
    }
}

void LvglTextWindow::SetText(uint8_t line, const std::string& text) {
    if (text_label_ == nullptr) {
        Log::Warn(TAG, "text label not setup.");
        return;
    }
    text_ = text;
    lv_label_set_text(text_label_, text_.c_str());
}

#endif //CONFIG_USE_LVGL