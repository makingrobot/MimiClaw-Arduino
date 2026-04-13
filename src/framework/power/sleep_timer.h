/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _SLEEP_TIMER_H
#define _SLEEP_TIMER_H

#include <functional>
#include "../sys/timer.h"

class SleepTimer {
public:
    SleepTimer(int seconds_to_light_sleep = 20, int seconds_to_deep_sleep = -1);
    ~SleepTimer();

    void SetEnabled(bool enabled);
    void OnEnterLightSleepMode(std::function<void()> callback);
    void OnExitLightSleepMode(std::function<void()> callback);
    void OnEnterDeepSleepMode(std::function<void()> callback);
    void WakeUp();
    void CheckSleep();

private:

    Timer* timer_ = nullptr;
    bool enabled_ = false;
    int ticks_ = 0;
    int seconds_to_light_sleep_;
    int seconds_to_deep_sleep_;
    bool in_light_sleep_mode_ = false;

    std::function<void()> on_enter_light_sleep_mode_;
    std::function<void()> on_exit_light_sleep_mode_;
    std::function<void()> on_enter_deep_sleep_mode_;
};

#endif //_SLEEP_TIMER_H