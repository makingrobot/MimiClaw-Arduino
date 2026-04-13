/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LVGL == 1

#ifndef LVGL_STATUSBAR_H
#define LVGL_STATUSBAR_H

#include <lvgl.h>
#include <string>
#include "lvgl_style.h"
#include "../fonts/font_emoji.h"
#include "../sys/timer.h"

class LvglStatusBar {
protected:
    // 状态栏
    lv_obj_t* status_bar_ = nullptr;

    // 状态栏
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t *notification_label_ = nullptr;
    lv_obj_t *mute_label_ = nullptr;

    lv_obj_t *time_label_ = nullptr;
    lv_obj_t *network_label_ = nullptr;
    lv_obj_t *battery_label_ = nullptr;
    
    const char* battery_icon_ = nullptr;
    const char* network_icon_ = nullptr;
    bool muted_ = false;

    Timer* notification_timer_ = nullptr;

public:
    LvglStatusBar();
    virtual ~LvglStatusBar();

    void SetupUI(lv_obj_t* container, const ThemeColors& theme, const DisplayFonts& fonts);
    
    void ShowNotification(const std::string& notification, int duration_ms = 3000);
    void SetStatus(const std::string& status);
    void Update(bool update_all = false);
   
    virtual void SetTheme(const ThemeColors& theme);

    void OnNotificationTimer();

private:
    const char* GetWifiIcon();
    std::string status_;
};

#endif // LVGL_STATUSBAR_H

#endif // CONFIG_USE_LVGL