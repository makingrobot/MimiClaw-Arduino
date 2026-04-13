/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "power_save_timer.h"
#include "../app/application.h"
#include "../sys/log.h"
#include "../sys/timer.h"

#include <esp_pm.h>

#define TAG "PowerSaveTimer"

PowerSaveTimer::PowerSaveTimer(int cpu_max_freq, int seconds_to_sleep, int seconds_to_shutdown)
    : cpu_max_freq_(cpu_max_freq), seconds_to_sleep_(seconds_to_sleep), seconds_to_shutdown_(seconds_to_shutdown) {
    timer_ = TimerFactory::CreateTimer("Power_Save");
}

PowerSaveTimer::~PowerSaveTimer() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

void PowerSaveTimer::SetEnabled(bool enabled) {
    if (enabled && !enabled_) {
        ticks_ = 0;
        enabled_ = enabled;
        timer_->Start(1000, [this](){ CheckPowerSave(); });

    } else if (!enabled && enabled_) {
        if (timer_ != nullptr) {
            timer_->Stop();
        }
        enabled_ = enabled;
        WakeUp();
    }
}

void PowerSaveTimer::OnEnterSleepMode(std::function<void()> callback) {
    on_enter_sleep_mode_ = callback;
}

void PowerSaveTimer::OnExitSleepMode(std::function<void()> callback) {
    on_exit_sleep_mode_ = callback;
}

void PowerSaveTimer::OnShutdownRequest(std::function<void()> callback) {
    on_shutdown_request_ = callback;
}

void PowerSaveTimer::CheckPowerSave() {
    auto& app = Application::GetInstance();
    if (!in_sleep_mode_ && !app.CanEnterSleepMode()) {
        ticks_ = 0;
        return;
    }

    ticks_++;
    if (seconds_to_sleep_ != -1 && ticks_ >= seconds_to_sleep_) {
        if (!in_sleep_mode_) {
            in_sleep_mode_ = true;
            if (on_enter_sleep_mode_) {
                Log::Info( TAG, "Entering sleep mode" );
                on_enter_sleep_mode_();
            }

            if (cpu_max_freq_ != -1) {
                esp_pm_config_t pm_config = {
                    .max_freq_mhz = cpu_max_freq_,
                    .min_freq_mhz = 40,
                    .light_sleep_enable = true,
                };
                esp_pm_configure(&pm_config);
            }
        }
    }
    if (seconds_to_shutdown_ != -1 && ticks_ >= seconds_to_shutdown_ && on_shutdown_request_) {
        Log::Info( TAG, "Entering deep sleep mode" );
        on_shutdown_request_();
    }
}

void PowerSaveTimer::WakeUp() {
    ticks_ = 0;
    if (in_sleep_mode_) {
        in_sleep_mode_ = false;

        if (cpu_max_freq_ != -1) {
            esp_pm_config_t pm_config = {
                .max_freq_mhz = cpu_max_freq_,
                .min_freq_mhz = cpu_max_freq_,
                .light_sleep_enable = false,
            };
            esp_pm_configure(&pm_config);
        }

        if (on_exit_sleep_mode_) {
            Log::Info( TAG, "Exiting sleep mode" );
            on_exit_sleep_mode_();
        }
    }
}
