/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _POWER_SAVE_TIMER_H
#define _POWER_SAVE_TIMER_H

#include <functional>
#include "../sys/timer.h"

/**
 * 节能定时器
 * 当设备空闲时（kDeviceStateIdle状态），根据时间分别进入睡眠状态和关闭状态。
 */
class PowerSaveTimer {
public:
    PowerSaveTimer(int cpu_max_freq, int seconds_to_sleep = 20, int seconds_to_shutdown = -1);
    ~PowerSaveTimer();

    void SetEnabled(bool enabled);
    void OnEnterSleepMode(std::function<void()> callback);
    void OnExitSleepMode(std::function<void()> callback);
    void OnShutdownRequest(std::function<void()> callback);
    void WakeUp();
    void CheckPowerSave();

private:

    Timer* timer_ = nullptr;
    bool enabled_ = false;
    bool in_sleep_mode_ = false;
    int ticks_ = 0;
    int cpu_max_freq_;
    int seconds_to_sleep_;
    int seconds_to_shutdown_;

    std::function<void()> on_enter_sleep_mode_;
    std::function<void()> on_exit_sleep_mode_;
    std::function<void()> on_shutdown_request_;
};

#endif //_POWER_SAVE_TIMER_H