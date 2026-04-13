/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#include "lvgl_statusbar.h"

#include "config.h"
#include "../sys/log.h"
#include "../board/board.h"
#include "../audio/audio_codec.h"
#include "../app/application.h"
#include "../app/device_state.h"
#include "../fonts/font_awesome_symbols.h"
#include "../lang/lang_zh_cn.h"
#include "../sys/timer.h"

#define TAG "LvglStatusBar"

LvglStatusBar::LvglStatusBar() {
    
    notification_timer_ = TimerFactory::CreateTimer("StatusBar");

}

LvglStatusBar::~LvglStatusBar() {

    if( notification_timer_!=nullptr ) {
        notification_timer_->Stop();
    }

    if (status_bar_ != nullptr) {
        lv_obj_del(status_bar_);
    }
    if (network_label_ != nullptr) {
        lv_obj_del(network_label_);
        lv_obj_del(notification_label_);
        lv_obj_del(status_label_);
        lv_obj_del(mute_label_);
        lv_obj_del(battery_label_);
    }
    if (time_label_ != nullptr) {
        lv_obj_del(time_label_);
    }

}

void LvglStatusBar::SetupUI(lv_obj_t* container, const ThemeColors& theme, const DisplayFonts& fonts) {
    /* Status bar */
    status_bar_ = lv_obj_create(container);
    lv_obj_set_size(status_bar_, LV_HOR_RES, fonts.text_font->line_height);
    lv_obj_set_style_radius(status_bar_, 0, 0);
    lv_obj_set_style_bg_color(status_bar_, theme.background, 0);
    lv_obj_set_style_text_color(status_bar_, theme.text, 0);
    
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(status_bar_, 0, 0);
    lv_obj_set_style_border_width(status_bar_, 0, 0);
    lv_obj_set_style_pad_column(status_bar_, 0, 0);
    lv_obj_set_style_pad_left(status_bar_, 2, 0);
    lv_obj_set_style_pad_right(status_bar_, 2, 0);

    time_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(time_label_, 1);
    lv_obj_set_style_text_color(time_label_, theme.text, 0);
    lv_label_set_text(time_label_, "");

    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(notification_label_, 1);
    lv_obj_set_style_text_align(notification_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(notification_label_, theme.text, 0);
    lv_label_set_text(notification_label_, "");
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);

    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(status_label_, 1);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, theme.text, 0);
    lv_label_set_text(status_label_, Lang::Strings::INITIALIZING);

    network_label_ = lv_label_create(status_bar_);
    lv_label_set_text(network_label_, "");
    lv_obj_set_style_text_font(network_label_, fonts.icon_font, 0);
    lv_obj_set_style_text_color(network_label_, theme.text, 0);

    mute_label_ = lv_label_create(status_bar_);
    lv_label_set_text(mute_label_, "");
    lv_obj_set_style_text_font(mute_label_, fonts.icon_font, 0);
    lv_obj_set_style_text_color(mute_label_, theme.text, 0);

    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, fonts.icon_font, 0);
    lv_obj_set_style_text_color(battery_label_, theme.text, 0);
}

void LvglStatusBar::SetTheme(const ThemeColors& theme) {
    
    // Update status bar colors
    if (status_bar_ != nullptr) {
        lv_obj_set_style_bg_color(status_bar_, theme.background, 0);
        lv_obj_set_style_text_color(status_bar_, theme.text, 0);
        
        // Update status bar elements
        if (network_label_ != nullptr) {
            lv_obj_set_style_text_color(network_label_, theme.text, 0);
        }
        if (status_label_ != nullptr) {
            lv_obj_set_style_text_color(status_label_, theme.text, 0);
        }
        if (notification_label_ != nullptr) {
            lv_obj_set_style_text_color(notification_label_, theme.text, 0);
        }
        if (mute_label_ != nullptr) {
            lv_obj_set_style_text_color(mute_label_, theme.text, 0);
        }
        if (battery_label_ != nullptr) {
            lv_obj_set_style_text_color(battery_label_, theme.text, 0);
        }
    }
    
}

void LvglStatusBar::ShowNotification(const std::string& notification, int duration_ms) {
    if (notification_label_ == nullptr) {
        return;
    }
    lv_label_set_text(notification_label_, notification.c_str());
    lv_obj_clear_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN);

    notification_timer_->Start(1000, [this](){ OnNotificationTimer(); }, true);
}

void LvglStatusBar::OnNotificationTimer() {
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
}

void LvglStatusBar::SetStatus(const std::string& status) {
    if (status_label_ == nullptr) {
        return;
    }
    status_ = status;
    lv_label_set_text(status_label_, status_.c_str());
    lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);
}

void LvglStatusBar::Update(bool update_all) {
    
    auto& board = Board::GetInstance();

#if CONFIG_USE_AUDIO==1
    {
        if (mute_label_ == nullptr) {
            return;
        }

        AudioCodec* codec = board.GetAudioCodec();
        // 如果静音状态改变，则更新图标
        if (codec->output_volume() == 0 && !muted_) {
            muted_ = true;
            lv_label_set_text(mute_label_, FONT_AWESOME_VOLUME_MUTE);
        } else if (codec->output_volume() > 0 && muted_) {
            muted_ = false;
            lv_label_set_text(mute_label_, "");
        }
    }
#endif //CONFIG_USE_AUDIO

    static int seconds_counter = 0;

    // Update time
    // Time* time = board.GetTime();
    // if (time != nullptr) {
    //     time->Update();
    //     lv_label_set_text(time_label_, time->GetHourMinute().c_str());
    // }
    // if (last_status_update_time_ + std::chrono::seconds(10) < std::chrono::system_clock::now()) {
    //     // Set status to clock "HH:MM"
    //     time_t now = time(NULL);
    //     struct tm* tm = localtime(&now);
    //     // Check if the we have already set the time
    //     if (tm->tm_year >= 2025 - 1900) {
    //         char time_str[16];
    //         strftime(time_str, sizeof(time_str), "%H:%M  ", tm);
    //         SetStatus(time_str);
    //     } else {
    //         // 每 60 秒输出一次
    //         if (seconds_counter++ % 60 == 0) {
    //             Log::Warn( TAG, "System time is not set, tm_year: %d", tm->tm_year);
    //         }
    //     }
    // }

    // 更新电池图标
    int battery_level;
    bool charging, discharging;
    const char* icon = nullptr;
    if (board.GetBatteryLevel(battery_level, charging, discharging)) {
        if (charging) {
            icon = FONT_AWESOME_BATTERY_CHARGING;
        } else {
            const char* levels[] = {
                FONT_AWESOME_BATTERY_EMPTY, // 0-19%
                FONT_AWESOME_BATTERY_1,    // 20-39%
                FONT_AWESOME_BATTERY_2,    // 40-59%
                FONT_AWESOME_BATTERY_3,    // 60-79%
                FONT_AWESOME_BATTERY_FULL, // 80-99%
                FONT_AWESOME_BATTERY_FULL, // 100%
            };
            icon = levels[battery_level / 20];
        }
        
        if (battery_label_ != nullptr && battery_icon_ != icon) {
            battery_icon_ = icon;
            lv_label_set_text(battery_label_, battery_icon_);
        }

        // if (low_battery_popup_ != nullptr) {
        //     if (strcmp(icon, FONT_AWESOME_BATTERY_EMPTY) == 0 && discharging) {
        //         if (lv_obj_has_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN)) { // 如果低电量提示框隐藏，则显示
        //             lv_obj_clear_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);
        //         }
        //     } else {
        //         // Hide the low battery popup when the battery is not empty
        //         if (!lv_obj_has_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN)) { // 如果低电量提示框显示，则隐藏
        //             lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);
        //         }
        //     }
        // }
    }

    //每 10 秒更新一次网络图标
    if (update_all || seconds_counter++ % 10 == 0) {
        // 升级固件时，不读取 4G 网络状态，避免占用 UART 资源
        DeviceState* device_state = Application::GetInstance().GetDeviceState();
        if (device_state == kDeviceStateIdle ||
                device_state == kDeviceStateStarting ||
                device_state == kDeviceStateWifiConfiguring || 
                device_state == kDeviceStateWorking) {
            const char* icon = GetWifiIcon();
            if (network_label_ != nullptr && icon != nullptr && network_icon_ != icon) {
                network_icon_ = icon;
                lv_label_set_text(network_label_, network_icon_);
            }
        }
    }

}

const char* LvglStatusBar::GetWifiIcon() {
    auto& board = Board::GetInstance();
    const char* icon_name = board.GetNetworkStateIconName();
    if (strcmp(icon_name, "WIFI") == 0) {
        return FONT_AWESOME_WIFI;
    } else if (strcmp(icon_name, "WIFI_FAIR") == 0) {
        return FONT_AWESOME_WIFI_FAIR;
    } else if (strcmp(icon_name, "WIFI_WEAK") == 0) {
        return FONT_AWESOME_WIFI_WEAK;
    } else if (strcmp(icon_name, "WIFI_OFF") == 0) {
        return FONT_AWESOME_WIFI_OFF;
    }
    return nullptr;
}

#endif //CONFIG_USE_LVGL