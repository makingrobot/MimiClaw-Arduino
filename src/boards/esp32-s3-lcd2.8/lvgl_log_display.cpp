/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if BOARD_ESP32_S3_LCD_2_8==1 && CONFIG_USE_LVGL == 1

#include "lvgl_log_display.h"

#include <vector>
#include <cstring>

#include "src/framework/sys/log.h"
#include "src/framework/sys/settings.h"

#define TAG "LvglLogDisplay"

LvglLogDisplay::LvglLogDisplay(DispDriver* driver, DisplayFonts fonts)
        : LvglDisplay(driver, fonts) {
            
    Log::Debug(TAG, "LvglMsgDisplay constructor.");

    // Load theme from settings
    Settings settings("display", false);
    current_theme_name_ = settings.GetString("theme", "dark");

    // Update the theme
    if (current_theme_name_ == "dark") {
        current_theme_ = DARK_THEME;
    } else if (current_theme_name_ == "light") {
        current_theme_ = LIGHT_THEME;
    }
}

LvglLogDisplay::~LvglLogDisplay() {
    statusbar_ = nullptr;

    if (textarea_ != nullptr) {
        lv_obj_del(textarea_);
    }

    if (container_ != nullptr) {
        lv_obj_del(container_);
    }

    if( low_battery_popup_ != nullptr ) {
        lv_obj_del(low_battery_popup_);
    }
}

void LvglLogDisplay::OnInit() {
    
    Log::Info(TAG, "Init ......");
    statusbar_ = new LvglStatusBar();
    
    SetupUI();

}

void LvglLogDisplay::Rotate(uint8_t rotation) {

}

void LvglLogDisplay::SetupUI() {
    Log::Info(TAG, "SetupUI ......");
    Log::Info(TAG, "Using %s theme", current_theme_name_.c_str());

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
    lv_label_set_text(low_battery_label_, "charge");
    lv_obj_set_style_text_color(low_battery_label_, lv_color_white(), 0);
    lv_obj_center(low_battery_label_);
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);

    /* 状态栏 */
    if (statusbar_!=nullptr) {
        statusbar_->SetupUI(container_, current_theme_, fonts_);
    }

    /* Content area */
    textarea_ = lv_textarea_create(container_);
    lv_obj_set_flex_flow(textarea_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_radius(textarea_, 0, 0);
    lv_obj_set_size(textarea_, LV_HOR_RES, LV_VER_RES - 36);
    lv_obj_set_style_border_width(container_, 1, 0);
    lv_obj_set_style_text_font(screen, fonts_.text_font, 0);
    lv_obj_set_style_text_color(textarea_, current_theme_.text, 0);
    lv_obj_set_style_bg_color(textarea_, current_theme_.background, 0);
    lv_obj_set_style_border_color(textarea_, current_theme_.border, 0);
   
}

void LvglLogDisplay::SetTheme(const std::string& theme_name) {
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
    
    if (textarea_ != nullptr) {
        lv_obj_set_style_bg_color(textarea_, current_theme_.background, 0);
        lv_obj_set_style_border_color(textarea_, current_theme_.border, 0);
    }

    // Update low battery popup
    if (low_battery_popup_ != nullptr) {
        lv_obj_set_style_bg_color(low_battery_popup_, current_theme_.low_battery, 0);
    }

    // Update status bar colors
    if (statusbar_ != nullptr) {
        statusbar_->SetTheme(current_theme_);
    }
    
}

void LvglLogDisplay::SetStatus(const std::string& status) {
    if (statusbar_!=nullptr) {
        DisplayLockGuard lock(this);
        statusbar_->SetStatus(status);
        
        last_status_update_time_ = std::chrono::system_clock::now();
    }
}

#if CONFIG_IDF_TARGET_ESP32P4
#define  MAX_MESSAGES 50
#else
#define  MAX_MESSAGES 30
#endif

void LvglLogDisplay::SetText(const std::string& text) {
    SetMessage("system", String(text.c_str()));
}

void LvglLogDisplay::SetMessage(const String& kind, const String& text) {

    if (textarea_ == nullptr) {
        return;
    }
    
    //避免出现空的消息框
    if(text.length() == 0) return;
    
    DisplayLockGuard lock(this);
    
    lines_.push_back(text);
    
    lv_textarea_add_text(textarea_, (text + "\n").c_str());
    lv_textarea_set_cursor_pos(textarea_, LV_TEXTAREA_CURSOR_LAST);
}

void LvglLogDisplay::AppendMessage(const String& kind, const String& text) {

    if (textarea_ == nullptr) {
        return;
    }
    
    //避免出现空的消息框
    if(text.length() == 0) return;
    
    DisplayLockGuard lock(this);
    
    lv_textarea_add_text(textarea_, text.c_str());
}

#endif // CONFIG_USE_LVGL
