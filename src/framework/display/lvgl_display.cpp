/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#include "lvgl_display.h"

#include <vector>
#include <algorithm>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <cstring>

#if CONFIG_USE_LCD_PANEL==1
#include "src/libs/esp_lvgl_port/esp_lvgl_port.h"
#endif

#include "../sys/log.h"
#include "../sys/settings.h"
#include "../board/board.h"
#include "../app/device_state.h"
#include "../app/application.h"
#include "../sys/time.h"
#include "../lang/lang_zh_cn.h"

#include "lvgl_window.h"
#include "lvgl_statusbar.h"
#include "lvgl_text_window.h"

#define TAG "LvglDisplay"

LvglDisplay::LvglDisplay(DispDriver* driver, DisplayFonts fonts)
        : driver_(driver), fonts_(fonts) {
            
    Log::Debug(TAG, "LvglDisplay constructor.");

    width_ = driver->width();
    height_ = driver->height();

    // Load theme from settings
    Settings settings("display", false);
    current_theme_name_ = settings.GetString("theme", "light");

    // Update the theme
    if (current_theme_name_ == "dark") {
        current_theme_ = DARK_THEME;
    } else if (current_theme_name_ == "light") {
        current_theme_ = LIGHT_THEME;
    }

}

LvglDisplay::~LvglDisplay() {

    window_ = nullptr;
    statusbar_ = nullptr;

    if (container_ != nullptr) {
        lv_obj_del(container_);
    }

    if( low_battery_popup_ != nullptr ) {
        lv_obj_del(low_battery_popup_);
    }

    driver_ = nullptr;
}

void LvglDisplay::Init() {
    Log::Info(TAG, "Init ......");

    driver_->Init();
    
    statusbar_ = new LvglStatusBar();
    
    SetupUI();

    if (window_ == nullptr) {
        window_ = new LvglTextWindow();
    }

    window_->SetupUI(container_, current_theme_, fonts_);    
}

void LvglDisplay::Rotate(uint8_t rotation) {

}

void LvglDisplay::SetWindow(LvglWindow* window) {
    window_ = window;
}

void LvglDisplay::SetupUI() {
    Log::Info(TAG, "SetupUI ......");

    DisplayLockGuard lock(this);

    auto screen = lv_screen_active();
    lv_obj_set_style_text_font(screen, fonts_.text_font, 0);
    lv_obj_set_style_text_color(screen, current_theme_.text, 0);
    lv_obj_set_style_bg_color(screen, current_theme_.background, 0);

    /* Container */
    container_ = lv_obj_create(screen);
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_row(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, current_theme_.background, 0);
    lv_obj_set_style_border_color(container_, current_theme_.border, 0);

    low_battery_popup_ = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, fonts_.text_font->line_height * 2);
    lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(low_battery_popup_, current_theme_.low_battery, 0);
    lv_obj_set_style_radius(low_battery_popup_, 10, 0);

    low_battery_label_ = lv_label_create(low_battery_popup_);
    lv_label_set_text(low_battery_label_, Lang::Strings::BATTERY_NEED_CHARGE);
    lv_obj_set_style_text_color(low_battery_label_, lv_color_white(), 0);
    lv_obj_center(low_battery_label_);
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);

    /* 状态栏 */
    if (statusbar_!=nullptr) {
        statusbar_->SetupUI(container_, current_theme_, fonts_);
    }
}

bool LvglDisplay::Lock(int timeout_ms) {
#if CONFIG_USE_LCD_PANEL==1
    return lvgl_port_lock(timeout_ms);
#else
    return true;
#endif
}

void LvglDisplay::Unlock() {
#if CONFIG_USE_LCD_PANEL==1
    lvgl_port_unlock();
#else
#endif
}

void LvglDisplay::SetTheme(const std::string& theme_name) {
    DisplayLockGuard lock(this);
    
    if (theme_name == "dark" || theme_name == "DARK") {
        current_theme_ = DARK_THEME;
    } else if (theme_name == "light" || theme_name == "LIGHT") {
        current_theme_ = LIGHT_THEME;
    } else {
        // Invalid theme name, return false
        Log::Error( TAG, "Invalid theme name: %s", theme_name.c_str());
        return;
    }
    
    // Get the active screen
    lv_obj_t* screen = lv_screen_active();
    
    // Update the screen colors
    lv_obj_set_style_bg_color(screen, current_theme_.background, 0);
    lv_obj_set_style_text_color(screen, current_theme_.text, 0);
    
    // Update container colors
    if (container_ != nullptr) {
        lv_obj_set_style_bg_color(container_, current_theme_.background, 0);
        lv_obj_set_style_border_color(container_, current_theme_.border, 0);
    }
    
    // Update low battery popup
    if (low_battery_popup_ != nullptr) {
        lv_obj_set_style_bg_color(low_battery_popup_, current_theme_.low_battery, 0);
    }

    // Update status bar colors
    if (statusbar_ != nullptr) {
        statusbar_->SetTheme(current_theme_);
    }
    
    if (window_ != nullptr) {
        window_->SetTheme(current_theme_);
    }
}

void LvglDisplay::SetStatus(const std::string& status) {
    if (statusbar_!=nullptr) {
        DisplayLockGuard lock(this);
        statusbar_->SetStatus(status);
        
        last_status_update_time_ = std::chrono::system_clock::now();
    }
}

void LvglDisplay::SetText(const std::string& text) {
    if (window_!=nullptr) {
        DisplayLockGuard lock(this);
        window_->SetText(text);
        
        last_status_update_time_ = std::chrono::system_clock::now();
    }
}

void LvglDisplay::ShowNotification(const std::string &notification, int duration_ms) { 

    if (statusbar_!=nullptr) {
        DisplayLockGuard lock(this);
        statusbar_->ShowNotification(notification, duration_ms);
    }
}

void LvglDisplay::UpdateStatusBar(bool update_all) {
    if (statusbar_!=nullptr) {
        DisplayLockGuard lock(this);
        statusbar_->Update(update_all);
    }
}

#endif // CONFIG_USE_LVGL
