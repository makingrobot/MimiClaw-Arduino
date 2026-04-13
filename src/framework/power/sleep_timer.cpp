/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "sleep_timer.h"
#include "../app/application.h"
#include "../board/board.h"
#include "../display/display.h"
#include "../sys/log.h"
#include "../sys/timer.h"

#include <esp_sleep.h>
#include <esp_pm.h>

#if CONFIG_USE_LVGL==1
#include "src/libs/esp_lvgl_port/esp_lvgl_port.h"
#endif

#define TAG "SleepTimer"

SleepTimer::SleepTimer(int seconds_to_light_sleep, int seconds_to_deep_sleep)
    : seconds_to_light_sleep_(seconds_to_light_sleep), seconds_to_deep_sleep_(seconds_to_deep_sleep) {
    timer_ = TimerFactory::CreateTimer("Sleep");
}

SleepTimer::~SleepTimer() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

void SleepTimer::SetEnabled(bool enabled) {
    if (enabled && !enabled_) {
        ticks_ = 0;
        enabled_ = enabled;
        timer_->Start(1000, [this]() {CheckSleep(); });
        
    } else if (!enabled && enabled_) {
        if (timer_ != nullptr) {
            timer_->Stop();
        }
        enabled_ = enabled;
        WakeUp();
    }
}

void SleepTimer::OnEnterLightSleepMode(std::function<void()> callback) {
    on_enter_light_sleep_mode_ = callback;
}

void SleepTimer::OnExitLightSleepMode(std::function<void()> callback) {
    on_exit_light_sleep_mode_ = callback;
}

void SleepTimer::OnEnterDeepSleepMode(std::function<void()> callback) {
    on_enter_deep_sleep_mode_ = callback;
}

void SleepTimer::CheckSleep() {
    auto& app = Application::GetInstance();
    if (!app.CanEnterSleepMode()) {
        ticks_ = 0;
        return;
    }

    ticks_++;
    if (seconds_to_light_sleep_ != -1 && ticks_ >= seconds_to_light_sleep_) {
        if (!in_light_sleep_mode_) {
            in_light_sleep_mode_ = true;
            if (on_enter_light_sleep_mode_) {
                on_enter_light_sleep_mode_();
            }
        
            app.Schedule([this]() {
                while (in_light_sleep_mode_) {
                    auto& board = Board::GetInstance();
                    board.GetDisplay()->UpdateStatusBar(true);

#if CONFIG_USE_LVGL==1
                    lv_refr_now(nullptr);
                    lvgl_port_stop();
    
                    // 配置timer唤醒源（30秒后自动唤醒）
                    esp_sleep_enable_timer_wakeup(30 * 1000000);
                    
                    // 进入light sleep模式
                    esp_light_sleep_start();
                    lvgl_port_resume();
#endif
                    auto wakeup_reason = esp_sleep_get_wakeup_cause();
                    if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
                        break;
                    }
                }
                WakeUp();
            });
        }
    }
    if (seconds_to_deep_sleep_ != -1 && ticks_ >= seconds_to_deep_sleep_) {
        if (on_enter_deep_sleep_mode_) {
            on_enter_deep_sleep_mode_();
        }

        esp_deep_sleep_start();
    }
}

void SleepTimer::WakeUp() {
    ticks_ = 0;
    if (in_light_sleep_mode_) {
        in_light_sleep_mode_ = false;
        if (on_exit_light_sleep_mode_) {
            on_exit_light_sleep_mode_();
        }
    }
}
